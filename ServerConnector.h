#ifndef __SERVERCONNECTOR_H__
#define __SERVERCONNECTOR_H__

#include <string>

#include "ClientSocket.h"
#include "Public.h"

#define TRANSFER_NOTREGISTERED 2
#define TRANSFER_GENERAL_ERROR 0
#define TRANSFER_OK 1

typedef struct
{
	unsigned int nPartID;
	unsigned int nMinLetters;
	unsigned int nMaxLetters;
	unsigned int nOffset;
	unsigned int nChainLength;
	unsigned int nChainCount;
	uint64 nChainStart;
	std::string sHashRoutine;
	std::string sCharset;
	std::string sSalt;
	
} stWorkInfo;

enum ERRRORLEVEL
{
	EL_NOTICE = 0,
	EL_WARNING,
	EL_ERROR
};

class ConnectionException :
	public Exception
{
public:
	ConnectionException(int nErrorLevel, std::string szErrorMessage) : Exception(szErrorMessage) { this->m_nErrorLevel = nErrorLevel; }
public:
	int GetErrorLevel() { return m_nErrorLevel; }
private:
	int m_nErrorLevel;
};

class ServerConnector
{
private:
public:
	ServerConnector(void);
public:
	~ServerConnector(void);
	CClientSocket *s;
	int bLoggedIn;
public:
	int RegisterNewClient(int &nClientID);
	void Disconnect();
	int Login(double nFrequency);
	int SendFinishedWork(int PartID, std::string Filename);
	int RequestWork(stWorkInfo *stWork);
};

#endif
