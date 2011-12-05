#include <iostream>
#include <sstream>
#include "BaseSocket.h"

int CBaseSocket::nAmountSockets = 0;
CBaseSocket::CBaseSocket(int nSocketType, int nProtocol)
{
	++nAmountSockets;
	rSocket = socket(AF_INET, nSocketType, nProtocol);
	if(rSocket == INVALID_SOCKET)
	{
		std::cout << "Unable to create new socket. Reason: " << GetSocketError();
	}
}

CBaseSocket::CBaseSocket(SOCKET rNewSocket)
{
	++nAmountSockets;
	rSocket = rNewSocket;
}
CBaseSocket::~CBaseSocket()
{
	if(rSocket != INVALID_SOCKET)
	{
		close(rSocket);
	}
	--nAmountSockets;
}

std::string CBaseSocket::GetPeerName()
{
	sockaddr_in name;
	std::string ip;
	int namelen = sizeof(name);
	if(getpeername(rSocket, (sockaddr *)&name, (socklen_t *)&namelen) == SOCKET_ERROR)
	{
		std::cout << "Error while getting peer name: " << GetSocketError();
	}
	ip.assign(inet_ntoa(name.sin_addr));
	return ip;
}


const CBaseSocket& CBaseSocket::operator <<(int n) const
{
	std::stringstream ss;
	ss << n;
	*this << ss.str();
	return *this;
}

const CBaseSocket& CBaseSocket::operator <<(std::string Line) const
{
	if(send(rSocket, Line.c_str(), (int)Line.length(), 0) == SOCKET_ERROR)
	{
		std::cout << "Error while sending data\n";
	}
	return *this;
}

const CBaseSocket& CBaseSocket::operator <<(std::vector<unsigned char> Data) const
{
	char *szData = new char[Data.size()];
	unsigned int i;
	for (i=0; i < Data.size(); i++)
	{
		szData[i] = Data[i];
	}
	if(send(rSocket, szData, Data.size(), 0) == SOCKET_ERROR)
	{
		delete szData;

		std::cout << "Error while sending data\n";
	}	
	delete szData;
	return *this;
}

const CBaseSocket& CBaseSocket::operator >>(std::string &Line) const
{
	char buf[8096];
	memset(buf, 0x00, sizeof(buf));
	int nBytes = recv(rSocket, buf, sizeof(buf), 0);
	if(nBytes == SOCKET_ERROR)
	{

		std::cout << "Error while recieving data from client\n";
	}
	Line.assign(buf, nBytes);
	return *this;
}

const CBaseSocket& CBaseSocket::operator >>(std::vector<unsigned char> &Data) const
{
	u_long arg = 0;
	while(arg == 0)
	{
		if (ioctl(rSocket, FIONREAD, &arg) == SOCKET_ERROR)
		{
			std::cout << "Error while ioctlsocket()\n";
		}
	}
	char buf[8096];
	int nBytes = recv(rSocket, buf, sizeof(buf), 0);
	if(nBytes == SOCKET_ERROR)
	{

		std::cout << "Error while recieving data from client\n";
	}
	for (int i=0; i < nBytes; i++)
	{
		Data.push_back(buf[i]); 	
	}
	return *this;
}

std::string CBaseSocket::ReceiveBytes(void *argPtr, void (*callback)(void *arg, size_t TotalByteCount), int amountBytes)
{
	std::string ret;
	char buf[8192];
	for ( ; ; )
	{
		u_long arg = 0;
		if (ioctl(rSocket, FIONREAD, &arg) == SOCKET_ERROR)
		{
			std::cout << "Error while running ioctl(): " << GetSocketError();
		}

		if (arg > 8192) arg = 8192;

		int rv = recv(rSocket, buf, sizeof(buf), 0);
		if (rv <= 0) break;
		std::string t;
		t.assign (buf, rv);
		ret += t;
		callback(argPtr, ret.size());
		if(amountBytes == (int)ret.size()) break;
	}
	return ret;
}

void CBaseSocket::SendBytes(const char *s, int length)
{
	if(send(rSocket,s,length,0) == SOCKET_ERROR)
	{

		std::cout << "Error while running ioctl(): " << GetSocketError();
	}
}
