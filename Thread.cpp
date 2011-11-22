#include <iostream>

#include "config.h"
#include "Thread.h"
CThread::CThread(void)
{
	bTerminateThreadFlag = false;
	Params = NULL;
}

CThread::~CThread(void)
{
	if(Params != NULL)
		delete Params;
}

void CThread::Stop() {
	// set our terminate flag and let our threadProc exit naturally
	bTerminateThreadFlag = true;

	// Why don't we just wait here until our function finishes before we continue processing.
	// Note: our end() function will stall the calling thread until this thread finishes executing.

	// remember our waitForSingleObject function?  Let's use it here to wait for our thread to finish.
	pthread_join(threadHandle, NULL);
}

void StartThreadFunction(CThread* pThread)
{
	pThread->threadProc();	
}
void CThread::Start(DataGenerationThreadParameters *Parameters)
{
	if(this->Params != NULL)
		delete this->Params;
	this->Params = Parameters;
	this->bTerminateThreadFlag = false;
	int nRet = pthread_create(&threadHandle, NULL, (void*(*)(void*))StartThreadFunction, (void*) this);
	if(nRet != 0)
	{
		std::cout << "ERROR: pthread_create() returned " << nRet;
	}	
}
