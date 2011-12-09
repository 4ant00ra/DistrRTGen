#ifndef _BASESOCKET_H__
#define _BASESOCKET_H__

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#endif

#include <vector>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR	
#define SOCKET_ERROR -1
#endif

#define SOCKET int

extern bool verbose;

class CBaseSocket
{
	public:
	CBaseSocket(int nSocketType, int nProtocol);
	CBaseSocket(SOCKET rNewSocket);
	~CBaseSocket(void);
	public:
	std::string GetPeerName();
	// Used to send test with
	const CBaseSocket& operator << (int) const;
	const CBaseSocket& operator << (std::string Line) const;
	// Used to send binary data
	const CBaseSocket& operator << (std::vector<unsigned char> Data) const;
	// Used to recieve data
	const CBaseSocket& operator >> (std::string &Line) const;
	// Used to recieve data
	const CBaseSocket& operator >> (std::vector<unsigned char> &Data) const;
	std::string ReceiveBytes(void *argPtr, void (*callback)(void *arg, size_t TotalByteCount), int amountBytes);
	void SendBytes(const char *s, int length);
	// Used for FD_SET to return the socket
	operator SOCKET() const {
		return rSocket;
	}
	protected:
	SOCKET rSocket;
	private:
	static int nAmountSockets;
	protected:
	inline std::string GetSocketError()
	{
		std::string szErrorDesc;
		szErrorDesc = strerror(errno);
		return szErrorDesc;
	}
	inline int GetSocketErrorCode()
	{
		return errno;
	}
};

#endif
