#ifndef _CLIENTSOCKET_H__
#define _CLIENTSOCKET_H__

#include <sstream>

#include "BaseSocket.h"
#include "Public.h"

class CClientSocket :
	public CBaseSocket
{
public:
	CClientSocket(void);
	CClientSocket(int, int, std::string, int);
	void Progress(void);
	void Progress(int, char*, float);
	std::string GetWork(void);
	void Close(void);
	int SendFinishedWork(unsigned int&, std::basic_stringstream<char>::__string_type);
	int RequestWork(stWorkInfo*);

};

#endif

