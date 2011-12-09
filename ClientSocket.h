#ifndef _CLIENTSOCKET_H__
#define _CLIENTSOCKET_H__

#include <sstream>

#include "BaseSocket.h"
#include "Public.h"

class CClientSocket :
	public CBaseSocket
{
	private:
		char szHostname[64];
		bool isProg;
	public:
		CClientSocket(void);
		CClientSocket(int, int);
		int Connect(std::string, int);
		void Progress(void);
		void Progress(int,int,int);
		std::string GetWork(void);
		void Done(void);
		void Close(void);
		void Abort(void);
		int SendFinishedWork(std::string);
		int RequestWork(stWorkInfo*);
};

#endif

