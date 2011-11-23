#ifndef _CLIENTSOCKET_H__
#define _CLIENTSOCKET_H__

#include <sstream>

#include "BaseSocket.h"

class CClientSocket :
	public CBaseSocket
{
public:
	CClientSocket(void);
	CClientSocket(int nSocketType, int nProtocol, std::string szHost, int nPort);
	std::string test(void);
public:
};

#endif

