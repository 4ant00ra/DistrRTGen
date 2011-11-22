#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

class CThread
{
protected:
	int bTerminateThreadFlag;
	void *Params;
	pthread_t threadHandle;
public:
	int IsTerminated() { return bTerminateThreadFlag; }
	CThread(void);
	void Start(void *Parameters);
	void Stop();
	virtual void threadProc() = 0;
public:
	virtual ~CThread(void);
};

#endif
