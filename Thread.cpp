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
	WaitForSingleObject(threadHandle, INFINITE);
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
	threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) StartThreadFunction, this, NULL, NULL);
}
