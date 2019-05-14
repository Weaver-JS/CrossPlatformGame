#include "GameInterface.hpp"

#include <thread>
#include <future>
#include <chrono>
#include <mutex>

//#define EULER_SINGLE_THREAD 
#define RESULT_SINGLE_THREAD

namespace Physics {

	static const float RESTITUTION_COEFICIENT = 1.0f;
	
	struct ContactData {
		Game::Ball *a, *b;
		glm::dvec2 point, normal;
		double penetration, restitution, friction;
	};
	
	struct ContactGroup {
		std::vector<Game::Ball*> balls;
		std::vector<ContactData> contacts;
	};

	struct Line {
		glm::dvec2 normal;
		double distance;
	};

	struct Circle {
		glm::dvec2 position;
		double radius;
	};

	std::vector<ContactGroup> GenerateContactGroups(std::vector<ContactData> contactData)
	{
		std::vector<ContactGroup> result;
		std::unordered_map<Game::Ball*, ContactGroup*> createdGroups;

		result.reserve(contactData.size());

		for (size_t i = 0; i < contactData.size(); ++i)
		{
			auto it = createdGroups.find(contactData[i].a); // busquem si ja tenim alguna colisió amb l'objecte "a"
			if (it == createdGroups.end())
			{
				it = createdGroups.find(contactData[i].b); // busquem si ja tenim alguna colisió amb l'objecte "b"

				if (it == createdGroups.end())
				{
					ContactGroup group;
					group.balls.push_back(contactData[i].a);
					group.balls.push_back(contactData[i].b);
					group.contacts.push_back(contactData[i]);

					result.push_back(group); // creem la llista de colisions nova

					createdGroups[contactData[i].a] = &result[result.size() - 1]; // guardem referència a aquesta llista
					createdGroups[contactData[i].b] = &result[result.size() - 1]; // per cada objecte per trobarla
				}
				else
				{
					it->second->contacts.push_back(contactData[i]); // afegim la colisió a la llista
					it->second->balls.push_back(contactData[i].a);
					createdGroups[contactData[i].a] = it->second; // guardem referència de l'objecte que no hem trobat abans
				}
			}
			else
			{
				ContactGroup *groupA = it->second;
				groupA->contacts.push_back(contactData[i]);

				auto itB = createdGroups.find(contactData[i].b);
				if (itB == createdGroups.end())
				{
					groupA->balls.push_back(contactData[i].b);
					createdGroups[contactData[i].b] = groupA;
				}
				else
				{
					ContactGroup *groupB = itB->second;

					if (groupA != groupB)
					{
						// el objecte b ja és a un grup diferent, fem merge dels 2 grups.

						// 1 - copiem tot el grup anterior
						for (auto& contactData : groupB->contacts)
						{
							groupA->contacts.push_back(contactData);
						}

						// 2 - copiem els elements del segon grup i actualitzem el mapa
						//     de grups
						for (Game::Ball* go : groupB->balls)
						{
							if (go != contactData[i].a)
								groupA->balls.push_back(go);
							createdGroups[go] = groupA;
						}

						// 3 - marquem el grup com a buit
						groupB->balls.clear();
						groupB->contacts.clear();
					}
				}
			}
		}

		return result;
	}

	void CalculateSeparationForce(Game::Ball &b1, Game::Ball &b2, ContactData contactData) {

		double separationVelocity = glm::dot(b1.vel - b2.vel,  glm::normalize(b1.pos - b2.pos)); // Aproaching Force
		
		if (-separationVelocity > 0)
		{
			
			double invMassA = (1 / b1.mass);
			double invMassB = (1 / b2.mass);
			double newSeparationVelocity;
			newSeparationVelocity = -RESTITUTION_COEFICIENT * separationVelocity;
			double totalInvMass = invMassA + invMassB;
			double deltaV = newSeparationVelocity - separationVelocity;
			double impulse = deltaV / totalInvMass;
			glm::dvec2 impulsPerIMass = contactData.normal * impulse;
			b1.vel = b1.vel + impulsPerIMass * invMassA;
			b2.vel = b2.vel - impulsPerIMass * invMassB;	
			
			double interpendetracio = glm::length((b1.pos + b1.radius) - (b2.pos + b2.radius));
			if (interpendetracio > 0) {
				glm::dvec2 movePerIMass = contactData.normal *((b1.mass + b2.mass) / (b1.mass * b2.mass));// contactData.normal * (interpendetracio / totalInvMass);
				b1.pos += movePerIMass * invMassA;
				b2.pos -= movePerIMass * invMassB;
				
			}
			
		}
	}
	/**
	* Calculates the penetration and the velocity
	*/
	void CalculateSeparationForce(ContactData * contactData)
	{
		double separationVelocity = glm::dot(contactData->a->vel - contactData->b->vel, glm::normalize(contactData->a->pos - contactData->b->pos)); // Aproaching Force

		if (-separationVelocity > 0)
		{
			/*Velocity*/
			double invMassA = (1 / contactData->a->mass);
			double invMassB = (1 / contactData->b->mass);
			double newSeparationVelocity;
			newSeparationVelocity = -RESTITUTION_COEFICIENT * separationVelocity;
			double totalInvMass = invMassA + invMassB;
			double deltaV = newSeparationVelocity - separationVelocity;
			double impulse = deltaV / totalInvMass;
			glm::dvec2 impulsPerIMass = contactData->normal * impulse;
			contactData->a->vel = contactData->a->vel + impulsPerIMass * invMassA;
			contactData->b->vel = contactData->b->vel - impulsPerIMass * invMassB;

			/*Penetration*/
			//double interpendetracio = glm::length((contactData->a->pos + contactData->a->radius) - (contactData->b->pos + contactData->b->radius));
			if (contactData->penetration > 0) {
				glm::dvec2 movePerIMass = contactData->normal *((contactData->a->mass + contactData->b->mass) / (contactData->a->mass * contactData->b->mass));// contactData.normal * (interpendetracio / totalInvMass);
				contactData->a->pos += movePerIMass * invMassA;
				contactData->b->pos -= movePerIMass * invMassB;
			}
		}
	}
}

namespace Collision {
	
	struct PossibleCollision {
		 Game::Ball *a, *b;
	};

	/// Deprecated
	void doPhysics(Game::Ball* hero, glm::vec2 screenSize, Utilities::Profiler &profiler)
	{
		glm::dvec2 screen = screenSize;
		Game::Ball ball;
		ball.mass =40.0f;
		ball.pos = screen;
		ball.radius = 0.5f;
		ball.vel = glm::dvec2(0.0f);
		if (glm::distance(hero->pos.x, screen.x) <= hero->radius || glm::distance(hero->pos.x, -screen.x) <= hero->radius) {
			Physics::ContactData contactData = {};
			contactData.point = { hero->pos / 2.0 + screen / 2.0 };
			contactData.normal = glm::normalize(hero->pos - screen);
			contactData.a = hero;
			contactData.b = &ball;

			contactData.penetration = glm::length((contactData.a->pos) - (contactData.b->pos)) / (contactData.a->radius / 2 + contactData.b->radius / 2);
		}
	}

	static bool ResultSingleThread = false;

	std::vector<PossibleCollision> SortAndSweep(const std::vector< Game::Ball*>& balls, Utilities::Profiler & profiler, Utilities::TaskManager::JobContext &context)
	{
		struct Extreme {
			
			Game::Ball *o;
			double p;
			bool min;
		};
		std::vector<Extreme> list;
		list.resize(balls.size() * 2);

		//   1 - Trobar els extrems de cada objecte respecte a un eix. (p.e. les la x min i max)
		std::atomic_int counter;
		counter.store(0);

		auto extremJob = Utilities::TaskManager::CreateLambdaBatchedJob([&counter, &list, &balls](int i, const Utilities::TaskManager::JobContext context)
		{
			Game::Ball* go = balls[i];
			list[counter++] = { go, glm::dot(go->GetExtreme({ -1,0 }),{ 1,0 }), true }; 
			list[counter++] = { go, glm::dot(go->GetExtreme({ 1,0 }),{ 1,0 }),false };

		}, "Extrems", balls.size() / THREADS, balls.size());
		context.scheduler->DoAndWait(&extremJob, &context);
		
		//   2 - Crear una llista ordenada amb cada extrem anotat ( o1min , o1max, o2min, o3min, o2max, o3max )
		std::sort(list.begin(), list.end(), [](const Extreme & first, const Extreme & second)
		{
			return first.p < second.p;
		}); 
		
		//   3 - Des de cada "min" anotar tots els "min" que es trobin abans de trobar el "max" corresponent a aquest objecte.
		//        aquests son les possibles colisions
		std::vector<PossibleCollision> result;
		
		if (ResultSingleThread) {
			context.CreateProfileMarkGuard("result Single");
			for (size_t i = 0; i < list.size(); ++i)
			{
				if (list[i].min)
				{
					for (size_t j = i + 1; list[i].o != list[j].o; ++j)
					{
						if (list[j].min)
						{
							result.push_back({ list[i].o, list[j].o });
						}
					}
				}
			}

		} else {
			std::atomic_int count;
			count.store(0);

			std::vector<std::vector<PossibleCollision>> resultThreads;
			resultThreads.resize(THREADS);


			auto resultJob = Utilities::TaskManager::CreateLambdaBatchedJob([&count, &list, &resultThreads](int i, const Utilities::TaskManager::JobContext context)
			{
				if (list[i].min)
				{
					for (size_t j = i + 1; list[i].o != list[j].o; ++j)
					{
						if (list[j].min)
						{
							resultThreads[context.threadIndex].push_back({ list[i].o, list[j].o });
						}
					}
				}
			}, "Result", list.size() / THREADS, list.size(), -1, Utilities::TaskManager::Job::Priority::LOW);
			context.scheduler->DoAndWait(&resultJob, &context);
			for (std::vector<PossibleCollision> pc : resultThreads) {
				result.insert(result.end(), pc.begin(), pc.end());
			}
		}
		return result;
	}

	bool BallAndBall(Game::Ball &b1, Game::Ball &b2) {

		if (glm::distance(b1.pos, b2.pos) <= b1.radius) {
			Physics::ContactData contactData = {};
			contactData.point = { b1.pos / 2.0 + b2.pos / 2.0 };
			contactData.normal = glm::normalize(b1.pos - b2.pos);
			// Old method
			// Physics::CalculateSeparationForce(b1, b2, contactData);
			contactData.a = &b1;
			contactData.b = &b2;
			Physics::CalculateSeparationForce(&contactData);
			return true;
		}
		return false;
	}

	Physics::ContactData CBallAndBall(Game::Ball &b1, Game::Ball &b2) {

		Physics::ContactData contactData = {};
		if (glm::distance(b1.pos, b2.pos) <= b1.radius) {
			contactData.point = { b1.pos / 2.0 + b2.pos / 2.0 };
			contactData.normal = glm::normalize(b1.pos - b2.pos);
			// Old method
			// Physics::CalculateSeparationForce(b1, b2, contactData);
			contactData.a = &b1;
			contactData.b = &b2;
			
			Physics::CalculateSeparationForce(&contactData);

		}
		return contactData;
	}
	
	void SolveCollisionGroup(const Physics::ContactGroup & contactGroup)
	{
		std::vector<Physics::ContactData> contacts = contactGroup.contacts;
		size_t iterations = 0;
		while (contacts.size() > 0 && iterations < contactGroup.contacts.size() * 3)
		{
			// busquem la penetració més gran
			Physics::ContactData* contactData = nullptr;
			for (Physics::ContactData& candidate : contacts)
			{
				if (contactData == nullptr || contactData->penetration < candidate.penetration)
				{
					contactData = &candidate;
				}
			}

			if (contactData->penetration < 1e-5)
			{
				break;
			}
			Physics::CalculateSeparationForce(contactData);
			++iterations;
		}
	}

	struct Bound {
		glm::dvec2 position;
		glm::dvec2 normal;
		
	};

	void calculateSeparationForceWithBounds(Game::Ball * b,glm::dvec2 screenCoords)
	{
		Bound up, down, left, right;
		up.position = glm::dvec2(0.0f, screenCoords.y);
		up.normal = glm::dvec2(0.0f, -1.0f);
		down.position = glm::dvec2(0.0f, -screenCoords.y);
		down.normal = glm::dvec2(0.0f, 1.0f);
		right.position = glm::dvec2(screenCoords.x,0.0f);
		right.normal = glm::dvec2(-1.0f, 0.0f);
		left.position = glm::dvec2(-screenCoords.x, 0.0f);
		left.normal = glm::dvec2(1.0f, 0.0f);
		double distanceUp = glm::dot((b->pos - up.position) ,up.normal);
		double distanceDown = glm::dot((b->pos - down.position), down.normal);
		double distanceRight = glm::dot((b->pos - right.position), right.normal);
		double distanceLeft = glm::dot((b->pos - left.position), left.normal);
		
		
		static const double pot = 0.1f;
		
		if (distanceUp <= b->radius)
		{
			b->pos += up.normal * glm::normalize(up.position - b->pos) * distanceUp * pot;
			b->vel = glm::length(b->vel) *  glm::normalize(glm::reflect(b->vel, up.normal));
		}
		else if (distanceDown <= b->radius)
		{
			b->pos += -down.normal * glm::normalize(down.position - b->pos) * distanceDown * pot;
			b->vel = glm::length(b->vel) * glm::normalize(glm::reflect(b->vel, down.normal));
		}
		if (distanceRight <= b->radius)
		{
			b->pos += right.normal * glm::normalize(right.position - b->pos) * distanceRight * pot;
			b->vel = glm::length(b->vel) * glm::normalize(glm::reflect(b->vel, right.normal));
		}
		else if (distanceLeft <= b->radius)
		{
			b->pos += -left.normal * glm::normalize(left.position - b->pos) * distanceLeft * pot;
			b->vel = glm::length(b->vel) * glm::normalize(glm::reflect(b->vel, left.normal));
		}
	}

	static bool eulerSingle = false;
	void eulerUpdate(std::vector <Game::Ball*> & balls, double dt, Utilities::TaskManager::JobContext & context)
	{

		static const double k0 = 0.95f;
		double breakValue = glm::pow(k0, dt);

		const double k1 = 0.1, k2 = 0.01;
		if (eulerSingle) {
			for (Game::Ball* b : balls) {
				double speed = glm::length(b->vel);
				if (speed > 0.0f)
				{
					double fv = k1 * speed + k2 * speed * speed;
					glm::dvec2 friction = -fv * glm::normalize(b->vel);
					b->force += friction;
				}
				b->force *= 1 / b->mass;
				b->pos = b->pos + b->vel * dt + b->force * (0.5f * dt * dt);
				b->vel = breakValue * b->vel + b->force * dt;
				if (glm::length(b->vel) < 1e-3)
				{
					b->vel = glm::dvec2(0.0f);
				}
			}
		} else {
			auto eulerJob = Utilities::TaskManager::CreateLambdaBatchedJob([=, &balls](int i, const Utilities::TaskManager::JobContext context)
			{
				double speed = glm::length(balls[i]->vel);
				if (speed > 0.0f)
				{
					double fv = k1 * speed + k2 * speed * speed;
					glm::dvec2 friction = -fv * glm::normalize(balls[i]->vel);
					balls[i]->force += friction;
				}
				balls[i]->force *= 1 / balls[i]->mass;
				balls[i]->pos = balls[i]->pos + balls[i]->vel * dt + balls[i]->force * (0.5f * dt * dt);
				balls[i]->vel = breakValue * balls[i]->vel + balls[i]->force * dt;
				if (glm::length(balls[i]->vel) < 1e-3)
				{
					balls[i]->vel = glm::dvec2(0.0f);
				}

			}, "Euler", balls.size() / THREADS, balls.size());
			context.scheduler->DoAndWait(&eulerJob, &context);
		}
	}

	std::vector<Game::Ball*> doPhysics(std::vector<Game::Ball*>& balls,double dt,glm::dvec2 screenSize, Utilities::Profiler& profiler, Utilities::TaskManager::JobContext & context) {
		// Sort and sweep 
		std::vector<Game::Ball*> newBalls = std::move(balls);
		{
			eulerUpdate(newBalls, dt, context);
		}
		
		std::vector<PossibleCollision> sortedCollisions = SortAndSweep(newBalls, profiler, context);
		std::vector<Physics::ContactData> contacts;
		std::vector<Physics::ContactGroup> contactGroup;
		contacts.resize(sortedCollisions.size());
		int contactsCount = 0;
		/*std::atomic_int  contactsCount;
		contactsCount.store(0);
		auto sortJob = Utilities::TaskManager::CreateLambdaBatchedJob([&contactsCount, &sortedCollisions, &contacts](int i, const Utilities::TaskManager::JobContext context)
		{
			Game::Ball* b1 = sortedCollisions[i].a;
			Game::Ball* b2 = sortedCollisions[i].b;
			if (glm::distance(b1->pos, b2->pos) <= b1->radius) {
				Physics::ContactData contactData = {};
				contactData.point = { b1->pos * 0.5 + b2->pos * 0.5 };
				contactData.normal = glm::normalize(b1->pos - b2->pos);
				contactData.a = b1;
				contactData.b = b2;
				contactData.penetration = glm::length(contactData.a->pos - contactData.b->pos) / (contactData.a->radius * 0.5 + contactData.b->radius * 0.5);
				contacts[contactsCount++] = contactData;
			}
				
		}, "Sorted Collisions", sortedCollisions.size() / THREADS, sortedCollisions.size(), -1, Utilities::TaskManager::Job::Priority::HIGH);*/
		//context.scheduler->DoAndWait(&sortJob, &context);

		for (Collision::PossibleCollision  pCollision : sortedCollisions)
		{
			Game::Ball* b1 = pCollision.a;
			Game::Ball* b2 = pCollision.b;
			if (glm::distance(b1->pos, b2->pos) <= b1->radius) {
				Physics::ContactData contactData = {};
				contactData.point = { b1->pos * 0.5 + b2->pos * 0.5 };
				contactData.normal = glm::normalize(b1->pos - b2->pos);
				contactData.a = b1;
				contactData.b = b2;
				contactData.penetration = glm::length(contactData.a->pos - contactData.b->pos) / (contactData.a->radius * 0.5 + contactData.b->radius * 0.5);
				contacts[contactsCount++] = contactData;
			}
		}
		contacts.resize(contactsCount);

		contactGroup = GenerateContactGroups(contacts);
		
		//auto solveJob = Utilities::TaskManager::CreateLambdaBatchedJob([&contactGroup](int i, const Utilities::TaskManager::JobContext context)
		//{
		//	SolveCollisionGroup(contactGroup[i]);

		//}, "Solve Collisions", contactGroup.size() / THREADS, contactGroup.size(), -1, Utilities::TaskManager::Job::Priority::HIGH);
		//context.scheduler->DoAndWait(&solveJob, &context);
			

		for (Physics::ContactGroup& group : contactGroup)
		{
			SolveCollisionGroup(group);
		}

		for (size_t i = 0; i < newBalls.size(); ++i)
		{
			calculateSeparationForceWithBounds(newBalls[i], screenSize);
		}
		return newBalls;
	}
	
}