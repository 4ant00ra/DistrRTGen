#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>
#include "ClientSocket.h"
using namespace std;

CClientSocket::CClientSocket(int nSocketType, int nProtocol, string szHost, int nPort) : CBaseSocket(nSocketType, nProtocol)
{
	gethostname(szHostname, 64);
	string error;

	hostent *he;
	if ((he = gethostbyname(szHost.c_str())) == 0)
	{
		cout << "Connection error: ";
		cout << GetSocketError() << endl;
		exit(2);
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nPort);
	addr.sin_addr = *((in_addr *)he->h_addr);
	memset(&(addr.sin_zero), 0, 8);

	if (connect(rSocket, (sockaddr *) &addr, sizeof(sockaddr)) == SOCKET_ERROR)
	{
		cout << "Connection error: ";
		cout << GetSocketError() << endl;
		exit(2);

	}
}

int CClientSocket::RequestWork(stWorkInfo* Work)
{
	string line[10];
	string info;
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

int CClientSocket::SendFinishedWork(string filename)
{
	filename.append(".zip");
        typedef istream_iterator<char> istream_iterator;
        ifstream file(filename.c_str(), ifstream::binary);
        vector<unsigned char> input;

        file >> noskipws;
        copy(istream_iterator(file), istream_iterator(), back_inserter(input));
	*this << input;
	return 0;
}
