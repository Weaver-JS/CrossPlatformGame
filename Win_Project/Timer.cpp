#ifndef TIMER_CPP
#define TIMER_CPP

#include "Framework.h"

namespace Timer {
	LARGE_INTEGER l_LastFrameTime;
	int64_t l_PerfCountFrequency;
	int64_t l_TicksPerFrame;

	void initTimer() {
		LARGE_INTEGER PerfCountFrequencyResult;
		QueryPerformanceFrequency(&PerfCountFrequencyResult);
		l_PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
		QueryPerformanceCounter(&l_LastFrameTime);
		l_TicksPerFrame = l_PerfCountFrequency / 60;
	}
	
	void update() {

		LARGE_INTEGER l_CurrentTime;
		QueryPerformanceCounter(&l_CurrentTime);
		//float Result = ((float)(l_CurrentTime.QuadPart - l_LastFrameTime.QuadPart) / (float)l_PerfCountFrequency);

		if (l_LastFrameTime.QuadPart + l_TicksPerFrame > l_CurrentTime.QuadPart)
		{
			int64_t ticksToSleep = l_LastFrameTime.QuadPart + l_TicksPerFrame - l_CurrentTime.QuadPart;
			int64_t msToSleep = 1000 * ticksToSleep / l_PerfCountFrequency;
			if (msToSleep > 0)
			{
				Sleep((DWORD)msToSleep);
			}
			return;
		}
		while (l_LastFrameTime.QuadPart + l_TicksPerFrame <= l_CurrentTime.QuadPart)
		{
			l_LastFrameTime.QuadPart += l_TicksPerFrame;
		}
	}

	double deltaTime() {
		return (double)l_TicksPerFrame / (double)l_PerfCountFrequency;
	}
};

#endif // !TIMER_CPP