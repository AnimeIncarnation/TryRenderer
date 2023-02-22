#pragma once
#ifndef _TIMER_H_
#define _TIMER_H_

#include "windows.h"
class Timer
{
public:
	Timer():prevCount(0), currCount(0), stopped(true),baseCount(0),stopCount(0),pausedCount(0)
	{
		UINT64 countsPerSec;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
		secondsPerCount = 1.0 / (double)countsPerSec;
	}
	void Reset()
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&baseCount);
		QueryPerformanceFrequency((LARGE_INTEGER*)&currCount);
		prevCount = currCount;
		stopped = false;
	}
	void Start()
	{
		if (stopped)
		{
			UINT64 now;
			pausedCount += QueryPerformanceCounter((LARGE_INTEGER*)&now) - stopCount;
			stopCount = 0;
			stopped = false;
		}
	}
	void Stop()
	{
		if (!stopped)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&stopCount);
			stopped = true;
		}
	}
	float GetTotalTime() const;
	float CalDeltaTime()
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&currCount);
		return (currCount - prevCount) * secondsPerCount > 0 ? (currCount - prevCount) * secondsPerCount:0;
	}


private:
	UINT64 prevCount;
	UINT64 currCount;
	bool stopped;
	double secondsPerCount;
	UINT64 baseCount;
	UINT64 stopCount;
	UINT64 pausedCount;
};

#endif