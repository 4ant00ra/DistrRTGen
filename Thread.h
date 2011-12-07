#ifndef THREAD_H
#define THREAD_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "Public.h"

class CThread
{
	protected:
	int bTerminateThreadFlag;
	DataGenerationThreadParameters *Params;;

	#ifdef WIN32
	HANDLE threadHandle;
	#else
	pthread_t threadHandle;
	#endif
	public:
	int IsTerminated() { return bTerminateThreadFlag; }
	CThread(void);
	void Start(DataGenerationThreadParameters *Parameters);
	void Stop();
	virtual void threadProc() = 0;
	public:
	virtual ~CThread(void);
};

#endif
