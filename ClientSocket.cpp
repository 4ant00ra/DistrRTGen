#include <iostream>
#include <sstream>
#include "ClientSocket.h"
#include "Public.h"

CClientSocket::CClientSocket(int nSocketType, int nProtocol, std::string szHost, int nPort) : CBaseSocket(nSocketType, nProtocol)
{
	gethostname(szHostname, 64);
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
	std::string line[10];
	std::string info;
	*this << "work\n";
	*this >> info;

	getNext(line,info);

	Work->nPartID = ston(line[0]);
	Work->nMinLetters = ston(line[1]);
	Work->nMaxLetters = ston(line[2]);
	Work->nOffset = ston(line[3]);
	Work->nChainLength = ston(line[4]);
	Work->nChainCount = ston(line[5]);

	Work->nChainStart = ston(line[6]);
	Work->sHashRoutine = line[7];
	Work->sCharset = line[8];
	Work->sSalt = "";
	return 0;
}

void CClientSocket::Progress(void)
{
	*this << "progress\n";
}

void CClientSocket::Progress(int nPart, int nRate, int nPerc)
{
	*this << nPart << ":" << szHostname << ":" << nRate << ":" << nPerc << "\n";
}

void CClientSocket::Close(void)
{
	*this << "done\n";
	*this << "quit\n";
}

int CClientSocket::SendFinishedWork(unsigned int& n, std::basic_stringstream<char>::__string_type x)
{
	return -1;
}
