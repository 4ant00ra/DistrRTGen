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

void CThread::Stop()
{
	bTerminateThreadFlag = true;
	#ifdef WIN32
	WaitForSingleObject(threadHandle, INFINITE);
	#endif
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
	#ifdef WIN32
	threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) StartThreadFunction, this, NULL, NULL);
	#else
	int nRet = pthread_create(&threadHandle, NULL, (void*(*)(void*))StartThreadFunction, (void*) this);
	if(nRet != 0)
	{
		std::cout << "+-----------------------------+" << std::endl;
		std::cout << "| Thread failed to create...  |" << std::endl;
		std::cout << "+-----------------------------+" << std::endl;
		exit(3);
	}
	pthread_detach(threadHandle);
	#endif
}
