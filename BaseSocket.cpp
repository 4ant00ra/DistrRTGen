#include <iostream>
#include <sstream>
#include "BaseSocket.h"
using namespace std;

int CBaseSocket::nAmountSockets = 0;
CBaseSocket::CBaseSocket(int nSocketType, int nProtocol)
{
	if(verbose)
		cout << "| Creating new socket         |" << endl;
	#ifdef WIN32
	if(verbose)
		cout << "| Windows socket type used    |" << endl;
	if(nAmountSockets == 0)
	{
		WSADATA info;
		WSAStartup(MAKEWORD(2,0), &info);
	}
	#else
	if(verbose)
		cout << "| Unix socket type used       |" << endl;
	#endif
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
	if(verbose)
		cout << "| Destroying socket           |" << endl;
	if(rSocket != INVALID_SOCKET)
	{
		#ifdef WIN32
		closesocket(rSocket);
		#else
		close(rSocket);
		#endif
	}
	--nAmountSockets;
}

std::string CBaseSocket::GetPeerName()
{
	if(verbose)
		cout << "| Resolving hostname          |" << endl;
	sockaddr_in name;
	std::string ip;
	int namelen = sizeof(name);
	#ifdef WIN32
	if(getpeername(rSocket, (sockaddr *)&name, &namelen) == SOCKET_ERROR)
	#else
	if(getpeername(rSocket, (sockaddr *)&name, (socklen_t *)&namelen) == SOCKET_ERROR)
	#endif
	{
		std::cout << "Error while getting peer name: " << GetSocketError();
	}
	ip.assign(inet_ntoa(name.sin_addr));
	return ip;
}


const CBaseSocket& CBaseSocket::operator <<(int n) const
{
	if(verbose)
		cout << "| Sending integer data        |" << endl;
	std::stringstream ss;
	ss << n;
	*this << ss.str();
	return *this;
}

const CBaseSocket& CBaseSocket::operator <<(std::string Line) const
{
	if(verbose)
		cout << "| Sending string data         |" << endl;
	if(send(rSocket, Line.c_str(), (int)Line.length(), 0) == SOCKET_ERROR)
	{
		std::cout << "Error while sending data\n";
	}
	return *this;
}

const CBaseSocket& CBaseSocket::operator <<(std::vector<unsigned char> Data) const
{
	if(verbose)
		cout << "| Sending binary data         |" << endl;
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
	if(verbose)
		cout << "| Receiving line data         |" << endl;
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
	if(verbose)
		cout << "| Receiving binary data       |" << endl;
	u_long arg = 0;
	while(arg == 0)
	{
		#ifdef WIN32
		if (ioctlsocket(rSocket, FIONREAD, &arg) == SOCKET_ERROR)
		#else
		if (ioctl(rSocket, FIONREAD, &arg) == SOCKET_ERROR)
		#endif
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
	if(verbose)
		cout << "| Low-level byte receiving    |" << endl;
	std::string ret;
	char buf[8192];
	for ( ; ; )
	{
		u_long arg = 0;
		#ifdef WIN32
		if (ioctlsocket(rSocket, FIONREAD, &arg) == SOCKET_ERROR)
		#else
		if (ioctl(rSocket, FIONREAD, &arg) == SOCKET_ERROR)
		#endif
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
	if(verbose)
		cout << "| Low-level byte sending      |" << endl;
	if(send(rSocket,s,length,0) == SOCKET_ERROR)
	{

		std::cout << "Error while running ioctl(): " << GetSocketError();
	}
}
