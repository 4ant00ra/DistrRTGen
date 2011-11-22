#ifndef _BASESOCKET_H__
#define _BASESOCKET_H__

#include <arpa/inet.h>      /* inet_ntoa() to format IP address */
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>     /* in_addr structure */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <vector>

#include "SocketException.h"

#ifndef INVALID_SOCKET
	#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR	
	#define SOCKET_ERROR -1
#endif

#define SOCKET int

class CBaseSocket
{
public:
	CBaseSocket(int nSocketType, int nProtocol);
	CBaseSocket(SOCKET rNewSocket);
	~CBaseSocket(void);
public:
	std::string GetPeerName();
	// Used to send test with
	void operator << (std::string Line);
	// Used to send binary data
	void operator << (std::vector<unsigned char> Data);
	// Used to recieve data
	void operator >> (std::string &Line);
	// Used to recieve data
	void operator >> (std::vector<unsigned char> &Data);
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
