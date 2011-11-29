#include <iostream>
#include <sstream>
#include "ClientSocket.h"


CClientSocket::CClientSocket(int nSocketType, int nProtocol, std::string szHost, int nPort) : CBaseSocket(nSocketType, nProtocol)
{
	std::string error;

	hostent *he;
	if ((he = gethostbyname(szHost.c_str())) == 0)
	{
		std::ostringstream szError;
		std::cout << "Error while trying to resolve hostname '" << szHost << "' : " << GetSocketError();
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nPort);
	addr.sin_addr = *((in_addr *)he->h_addr);
	memset(&(addr.sin_zero), 0, 8);

	if (connect(rSocket, (sockaddr *) &addr, sizeof(sockaddr)) == SOCKET_ERROR)
	{
		std::ostringstream szError;
		std::cout << "Error while trying to connect to '" << szHost << "' : " << GetSocketError();
	}

}

std::string CClientSocket::GetWork(void)
{
	std::string part;
	*this << "work\n";
	*this >> part;

	part = part.substr(0,part.find('\n'));
	
	return part;
}

void CClientSocket::Progress(void)
{
	*this << "progress\n";
}

void CClientSocket::Progress(int nPart, char* czHostname, float fProg)
{
	std::stringstream ssData;
	ssData << nPart << ":" << czHostname << ":" << fProg << "\n";
	*this << ssData.str();
}

void CClientSocket::Close(void)
{
	*this << "quit\n";
}