#include <curl/curl.h>
#include <curl/easy.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include "config.h"
#include "ServerConnector.h"

#define SERVER_PORT 80
#define SERVER_NAME "http://distributed.zwibits.org:8080/server.php"
#define UPLOAD_URL "http://distributed.zwibits.org:8080/upload.php"
#define _FILE_OFFSET_BITS 64
#ifndef VERSION
	#define VERSION "1.0 LX"
#endif

enum TALKATIVE
{
	TK_ALL = 0,
	TK_WARNINGS,
	TK_ERRORS
};

int nTalkative = TK_ALL;

struct MemoryStruct
{
    char *memory;
    size_t size;
};

ServerConnector::ServerConnector(void)
{
	bLoggedIn = false;

	s = NULL;
}

ServerConnector::~ServerConnector(void)
{
	if(s != NULL)
	{
		delete s;
		s = NULL;
	}
}

int ServerConnector::RequestWork(stWorkInfo *stWork)
{
	// TODO: Request work socket method
	return -1;
}

int ServerConnector::SendFinishedWork(int nPartID, std::string Filename)
{
	// TODO: Upload binary file method
	return -1;
}
