#pragma once

#if defined(max)
#undef max
#endif

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <chrono>

#include "ThreadsafeStructures.hpp"
#include "profiler.hpp"
//#define DUMMY_SCHEDULER 

namespace Utilities
{
	namespace TaskManager
	{
		static constexpr int NumSmallStackFibers = 128;
		static constexpr int NumLargeStackFibers = 32;
		static constexpr int NumFibers = NumSmallStackFibers + NumLargeStackFibers;
		static constexpr int AllThreadsMask = 0xffff;
		class Job;
		class JobScheduler;
		struct JobContext;
		


		// aquesta estructura ens permetrà fàcil accés a estructures pseudo-globals per
		// interactuar amb el sistema de tasques.
		struct JobContext
		{


			JobScheduler *scheduler;
			Profiler *profiler;
			// dades locals a la nostra tasca. Serviràn al JobScheduler per a tractar correctament la tasca.
			int threadIndex;
			int fiberIndex;

			void Do(Job* job) const;
			void Wait(Job* job) const;
			void DoAndWait(Job* job) const;

			void PrintDebug(const char*) const; // implementar en un fitxer platform-dependant.
			
			// wrapper al profiler per a indicar correctament el thread actual.
			Profiler::MarkGuard CreateProfileMarkGuard(const char* functionName, int systemID = -1) const
			{
				return profiler->CreateProfileMarkGuard(functionName, threadIndex, systemID);
			}
		};

		// La base del sistema de tasques: la unitat de feina a fer.
		// Sobre-escriure aquesta classe per a distribuir la feina en diferents threads.
		
		// virtual void DoTask(int taskIndex, const JobContext& context) = 0;
		//   - taskIndex:  ens permetrà distingir entre diferents instàncies de la mateixa tasca
		//   - context:    ens permetrà instanciar noves tasques.
		class Job
		{
		public:
			std::atomic_short numPendingTasks;
			const enum class Priority : uint8_t
			{
				HIGH, MEDIUM, LOW
			} priority;
			const bool needsLargeStack;
			const int systemID;
			const char* jobName;

			Job(const Job& other) : numPendingTasks(other.numPendingTasks.load()), priority(other.priority), needsLargeStack(other.needsLargeStack), systemID(other.systemID), jobName(other.jobName) {};
			Job(Job&& other) : numPendingTasks(other.numPendingTasks.load()), priority(other.priority), needsLargeStack(other.needsLargeStack), systemID(other.systemID), jobName(other.jobName) {};
			Job& operator=(Job& other) = delete;
			Job& operator=(Job&& other) = delete;

			virtual void DoTask(int taskIndex, const JobContext& context) = 0;

			void TaskFinished() { --numPendingTasks; }

			bool HasFinished() const { return 0 == numPendingTasks.load(); }
			int GetNumPendingTasks() const { return numPendingTasks.load(); }

		protected:
			Job(const char* _jobName, short _numTasks = 1, int _systemID = -1, Priority _priority = Priority::MEDIUM, bool _needsLargeStack = false)
				: numPendingTasks(_numTasks)
				, priority(_priority)
				, needsLargeStack(_needsLargeStack)
				, systemID(_systemID)
				, jobName(_jobName)
			{}
		};
#ifndef DUMMY_SCHEDULER
		void __stdcall WorkerFiber(void* param);
#endif
		class WaitJob : public Job
		{
		public:
			void DoTask(int taskIndex, const JobContext& context) override
			{
				context.Wait(jobToWait);

				externalThreadCV.notify_all();
			}

			WaitJob(JobScheduler &_jobScheduler, Job* _jobToWait, std::mutex &_externalThreadMutex, std::condition_variable &_externalThreadCV)
				: Job("External Thread Wait Job")
				, jobToWait(_jobToWait)
				, externalThreadCV(_externalThreadCV)
			{}

			Job* jobToWait;
			std::condition_variable &externalThreadCV;
		};


#ifdef DUMMY_SCHEDULER
		class JobScheduler
		{
		public:

			JobScheduler() {}

			void Init(int _numThreads, Profiler *_profiler/*, DefaultAllocator* _allocator*/)
			{
				profiler = _profiler;
				//allocator = _allocator;
			}

			void Do(Job* job, const JobContext* context)
			{
				int n = job->GetNumPendingTasks();

				JobContext newContext;

				newContext.scheduler = this;
				newContext.profiler = profiler;
				//newContext.allocator = allocator;
				newContext.threadIndex = 0;
				newContext.fiberIndex = 0;

				Job* oldJob = currentJob;
				if (oldJob != nullptr)
				{
					profiler->AddProfileMark(Profiler::MarkerType::PAUSE_WAIT_FOR_JOB, oldJob, oldJob->jobName, 0, oldJob->systemID);
				}

				currentJob = job;

				for (int i = 0; i < n; ++i)
				{
					profiler->AddProfileMark(Profiler::MarkerType::BEGIN, job, job->jobName, 0, job->systemID);

					job->DoTask(i, newContext);
					job->TaskFinished();

					profiler->AddProfileMark(Profiler::MarkerType::END, job, job->jobName, 0, job->systemID);
				}

				if (oldJob != nullptr)
				{
					profiler->AddProfileMark(Profiler::MarkerType::RESUME_FROM_PAUSE, oldJob, oldJob->jobName, 0, oldJob->systemID);
				}
				currentJob = oldJob;
			}

			void Wait(Job* job, const JobContext* context)
			{
				assert(job->GetNumPendingTasks() == 0);
			}

			void DoAndWait(Job* job, const JobContext* context)
			{
				if (job->GetNumPendingTasks() == 1 && context != nullptr)
				{
					job->DoTask(0, *context); // TODO check this
					job->TaskFinished();
				}
				else
				{
					Do(job, context);
					Wait(job, context);
				}
			}

			void NotifyWaitingThreads()
			{
			}

			void RunScheduler(int idx, Profiler &profiler) {}

			void SetRootFiber(void* fiberId, int idx) {  }
			void FinishTasks() { }

		private:
			Profiler * profiler;
			//DefaultAllocator* allocator;
			Job* currentJob = nullptr;
		};

#else
		// classe que s'encarrega de gestionar la feina entre diferents threads i tasques.
		class JobScheduler
		{
		public:

			JobScheduler()
				: runTasks(true)
				, numThreadsIdle(0)
				, goingToNotify(false)
				, fiberContexts{}
			{}

			// funció que inicialitza totes les dades dels Fibers
			void Init(int _numThreads, Profiler *profiler/*, DefaultAllocator* allocator*/)
			{
				numThreads = _numThreads;
				// creem els Fibers que farem servir per a les tasques.
				for (intptr_t i = 0; i < (intptr_t)NumSmallStackFibers; ++i)
				{
					fibers[i] = CreateFiber(64 * 1024, WorkerFiber, reinterpret_cast<void*>(&fiberContexts[i]));
					smallStackFiberIndexs.Push(static_cast<short>(i));
				}

				for (intptr_t i = 0; i < (intptr_t)NumLargeStackFibers; ++i)
				{
					fibers[i + NumSmallStackFibers] = CreateFiber(512 * 1024, WorkerFiber, reinterpret_cast<void*>(&fiberContexts[i + NumSmallStackFibers]));
					largeStackFiberIndexs.Push(static_cast<short>(i + NumSmallStackFibers));
				}

				// inicialitzem les dades  per cada fiber
				for (int i = 0; i < NumFibers; ++i)
				{
					// dades comunes per al funcionament de la tasca
					fiberContexts[i].scheduler = this;
					fiberContexts[i].profiler = profiler;
					//fiberContexts[i].allocator = allocator;
					fiberContexts[i].threadIndex = -1;
					fiberContexts[i].fiberIndex = i;
					// dades per al scheduler
					fiberContexts[i].job = nullptr;
					fiberContexts[i].fiberWaitingForJobCompletion = nullptr;
					fiberContexts[i].taskIndex = 0;
				}
			}

			// afegeix una tasca a ser executada
			void Do(Job* job, const JobContext* context)
			{
				int tasks = job->GetNumPendingTasks();

				NotifyWaitingThreads();

				for (int index = 0; index = queues[(int)job->priority].AddJob(job, index, tasks), index < tasks; )
				{
					assert(context != nullptr);

					fiberContexts[context->fiberIndex].fiberWaitingForJobCompletion = fiberContexts[context->fiberIndex].job; // wait for ourselves. this marks that we can be awaited at any moment

					SwitchToFiber(rootFibers[context->threadIndex]);

					NotifyWaitingThreads();
				}

				NotifyWaitingThreads();
			}

			// adorm la tasca actual fins que la tasca indicada finalitzi
			void Wait(Job* job, const JobContext& context)
			{
				if (!job->HasFinished())
				{
					if((context.threadIndex >= 0 && context.threadIndex < numThreads))
					{
						short fiberIndex = context.fiberIndex;

						fiberContexts[context.fiberIndex].fiberWaitingForJobCompletion = job;

						SwitchToFiber(rootFibers[context.threadIndex]);
					}
					else
					{
						auto guard = context.CreateProfileMarkGuard("External thread Wait");
						std::mutex externalThreadMutex;
						std::condition_variable externalThreadCV;
						WaitJob waitJob(*this, job, externalThreadMutex, externalThreadCV);
						Do(&waitJob, &context);

						{
							std::unique_lock<std::mutex> lock(externalThreadMutex);

							while (!job->HasFinished())
							{
								externalThreadCV.wait(lock);
							}
							while (!waitJob.HasFinished())
							{
								std::this_thread::yield();
							}
						}
					}
				}
			}

			void DoAndWait(Job* job, const JobContext* context)
			{
				//if (job->GetNumPendingTasks() == 1 && context != nullptr)
				{
					//job->DoTask(0, *context); // TODO check this
				}
				//else
				{
					Do(job, context);
					Wait(job, *context);
				}
			}

			// indica a tots els threads que estan adormits que possiblement hi ha tasques per a executar.
			void NotifyWaitingThreads()
			{
				goingToNotify = AllThreadsMask;

				if (numThreadsIdle > 0)
				{
					std::unique_lock<std::mutex> lock(noWorkMutex);
					noWorkCV.notify_all();
				}
			}

			// fil principal del scheduler. Això caldrà que es cridi des de cada thread.
			void RunScheduler(int idx, Profiler &profiler);

			// indica als threads que es tanquin.
			void FinishTasks() { runTasks.store(false); }
			
			void SetRootFiber(void* fiberId, int idx) { rootFibers[idx] = fiberId; }

		protected:

			// funcions a implementar per plataforma sobre Fibers
			virtual void* CreateFiber(size_t stackSize, void(__stdcall*call) (void*), void* parameter) = 0;
			virtual void SwitchToFiber(void*) = 0;
			virtual void* GetFiberData() const = 0;

		private:

			// estructura que controla tasques individuals en una cua.
			struct JobQueue
			{
				struct Task // cada tasca la forma una "Job" + un índex.
				{
					Job *job;
					int taskIndex;
				};
				SpinlockQueue<Task, 1024> innerQueue;

				// afegeix tasques des de "begin" a "end". Retorna aquella tasca on s'ha quedat per no poder-la afegir.
				int AddJob(Job *job, int begin, int end)
				{
					for (int i = begin; i < end; ++i)
					{
						Task task = { job, i };
						if (!innerQueue.Add(task))
							return i;
					}
					return end;
				}

				// agafa una tasca de la cua, si n'hi ha.
				bool GetPendingTask(Job* &jobPointer_, int &taskIndex_)
				{
					Task task;
					if (innerQueue.Remove(task))
					{
						jobPointer_ = task.job;
						taskIndex_ = task.taskIndex;
						return true;
					}
					return false;
				}
			};

			// ampliació del contexte de feines per incloure dades rellevants per al Scheduler
			struct FiberContext : JobContext
			{
				Job *job, *fiberWaitingForJobCompletion;
				int taskIndex;
			};

			// cues de feina. Cada una per a una prioritat.
			JobQueue queues[3];

			// fibers corresponents als Schedulers
			void* rootFibers[8];

			// fibers
			void* fibers[NumFibers];
			// fibers amb stack petit lliures
			LockfreeStack<short, NumSmallStackFibers> smallStackFiberIndexs;
			// fibers amb stack gros lliures
			LockfreeStack<short, NumLargeStackFibers> largeStackFiberIndexs;

			// contextes
			FiberContext fiberContexts[NumFibers];

			// indicador de quan tancar el Scheduler
			std::atomic_bool runTasks;

			// quants threads tenim disponibles
			int numThreads;

			// variables de sincronització
			std::mutex noWorkMutex;
			std::condition_variable noWorkCV;
			std::atomic_int numThreadsIdle;
			std::atomic_int goingToNotify;

			// funció base de cada WorkerFiber
			friend void __stdcall WorkerFiber(void* param);

			// funció on cada thread s'esperarà si no hi ha feina disponible.
			void WaitForNotification(int threadId)
			{
				std::unique_lock<std::mutex> lock(noWorkMutex);
				++numThreadsIdle;

				int notifyMask = goingToNotify.load();

				if ((notifyMask & (1 << threadId)) == 0)
					noWorkCV.wait(lock);


				int newMask = (notifyMask & ~(1 << threadId));
				while (!goingToNotify.compare_exchange_weak(notifyMask, newMask))
				{
					newMask = (notifyMask & ~(1 << threadId));
				}

				--numThreadsIdle;
			}

		};
#endif
	}
}

#include "TaskManager.inl.hpp"
