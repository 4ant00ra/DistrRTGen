#ifndef THREAD_H
#define THREAD_H

#include <windows.h>
#include "Public.h"

class CThread
{
	protected:
	int bTerminateThreadFlag;
	DataGenerationThreadParameters *Params;;

	HANDLE threadHandle;
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
