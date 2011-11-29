#include <iostream>
#include <sstream>
#include "ClientSocket.h"
#include "Public.h"

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

int CClientSocket::RequestWork(stWorkInfo* Work)
{
	int part;
	std::string line;
	stringstream ss;
	*this << "work\n";
	*this >> line;

	part = ston(line.substr(0,line.find('\n')));
	
	Work->nPartID = 1;
	Work->nMinLetters = 1;
	Work->nMaxLetters = 6;
	Work->nOffset = 0;
	Work->nChainLength = 500;
	Work->nChainCount = 2000000;
	
	Work->nChainStart = 0;
	Work->sHashRoutine = "md5";
	Work->sCharset = "numeric";
	Work->sSalt = "";
	return 0;
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

int CClientSocket::SendFinishedWork(unsigned int& n, std::basic_stringstream<char>::__string_type x)
{
	return -1;
}
