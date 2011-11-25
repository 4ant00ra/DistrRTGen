#include <curl/curl.h>
#include <curl/easy.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include "config.h"
#include "DistrRTGenBinding.nsmap"
#include "ServerConnector.h"
#include "tinyxml.h"

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

struct MemoryStruct {
    char *memory;
    size_t size;
};

// There might be a realloc() out there that doesn't like reallocing
// NULL pointers, so we take care of it here 
static void *myrealloc(void *ptr, size_t size)
{
    if(ptr)
      return realloc(ptr, size);
    else
      return malloc(size);
}

// Static function to retrieve server XML answers on HTTP protocol - used with CURL
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
  
    mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
      memcpy(&(mem->memory[mem->size]), ptr, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
    }
    return realsize;
}

ServerConnector::ServerConnector(void)
{
	bLoggedIn = false;
	soap_init(&m_soap);

	s = NULL;
}

ServerConnector::~ServerConnector(void)
{
	if(s != NULL)
	{
		Disconnect();
		delete s; 
	}
}

void ServerConnector::Disconnect()
{
	delete s;
	s = NULL;
}

int ServerConnector::RequestWork(stWorkInfo *stWork)
{
		ns1__RequestWorkResponse response;
		soap_call_ns1__RequestWork(&m_soap, NULL, NULL, &m_machineInfo, 1, response);
		if(response.ErrorCode > 0)
		{
			return response.ErrorCode;
		}
		if(response.WorkUnit->WorkUnitElement.size() > 0)
		{
			stWork->nPartID = response.WorkUnit->WorkUnitElement[0]->PartID;
			stWork->nMinLetters = response.WorkUnit->WorkUnitElement[0]->MinLetters;
			stWork->nMaxLetters = response.WorkUnit->WorkUnitElement[0]->MaxLetters;
			stWork->nOffset = response.WorkUnit->WorkUnitElement[0]->Index;
			stWork->nChainLength = response.WorkUnit->WorkUnitElement[0]->ChainLength;
			stWork->nChainCount = response.WorkUnit->WorkUnitElement[0]->ChainCount;
			stWork->sHashRoutine = response.WorkUnit->WorkUnitElement[0]->HashRoutine;
			stWork->sCharset = std::string(response.WorkUnit->WorkUnitElement[0]->Charset);
			if(response.WorkUnit->WorkUnitElement[0]->Salt != "")
				stWork->sSalt = std::string(response.WorkUnit->WorkUnitElement[0]->Salt);	
			stWork->nChainStart = response.WorkUnit->WorkUnitElement[0]->ChainStart;	
		}
		else throw new ConnectionException(0, "No work left on server");
		
		return response.ErrorCode;
}

int ServerConnector::SendFinishedWork(int nPartID, std::string Filename)
{
	try
	{
		struct curl_httppost *post=NULL;
 		struct curl_httppost *last=NULL;
		struct curl_slist *headers=NULL;
		struct MemoryStruct xmlresponse;
  		CURL *curl;
		CURLcode res;
		std::stringstream szPartname;
		std::stringstream szUrlpost;
		std::string sPartname;
		std::string sUrlpost;
		
		//Limitrate seems to not work with POST FORM DATA
		curl_off_t limitrate = 0;
		
		xmlresponse.memory=NULL; 
  		xmlresponse.size = 0;    
		
		// build filename
		szPartname << nPartID << ".part.zip";
		sPartname = szPartname.str();
		
		//build url upload.php?username=blablabla
		szUrlpost << UPLOAD_URL << "?compressed2=1";
		sUrlpost = szUrlpost.str();
		
		curl = curl_easy_init();
		if (curl)
		{
			curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "name",
              CURLFORM_COPYCONTENTS, "file", CURLFORM_END);
			curl_formadd(&post, &last,
	 	      CURLFORM_COPYNAME, "filename",
              CURLFORM_COPYCONTENTS, sPartname.c_str(), CURLFORM_END); 			 			
			curl_formadd(&post, &last,
              CURLFORM_COPYNAME, "file",
              CURLFORM_FILE, Filename.c_str(), CURLFORM_END); //Path of the part file
			
			// Disable HTTP1.1 Expect:
			headers = curl_slist_append(headers, "Expect:");

 			// Header content-disposition
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
			
			if(nTalkative <= TK_ALL)
				std::cout << "Uploading part : " << sPartname.c_str() << std::endl;
			// Set the form info
 			curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, (long)limitrate);
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
			curl_easy_setopt(curl, CURLOPT_URL, sUrlpost.c_str());
 			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&xmlresponse);
    		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    		curl_easy_setopt(curl, CURLOPT_MAX_SEND_SPEED_LARGE, limitrate);
			curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, limitrate);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 0);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 0);
			
			if(nTalkative <= TK_ALL)
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
			
			
			res = curl_easy_perform(curl); /* post away! */
			if (res == 0)
			{
				// Retrieve Server XML answer
				TiXmlDocument xUploadAnswer;
				if(xUploadAnswer.Parse(xmlresponse.memory))
				{
					TiXmlElement * pElem;
					pElem = xUploadAnswer.FirstChildElement();
					if(pElem->FirstChildElement("ok"))
					{
						curl_formfree(post);
						curl_easy_cleanup(curl);
						if(xmlresponse.memory)
          					free(xmlresponse.memory);
						return TRANSFER_OK;
					}
	 				else if(pElem->FirstChildElement("error"))
					{
						curl_formfree(post);
						curl_easy_cleanup(curl);
						if(xmlresponse.memory)
          					free(xmlresponse.memory);
						std::cout << pElem->FirstChildElement("error")->GetText() << std::endl;
						return TRANSFER_NOTREGISTERED;
					}
					else
					{
						throw new ConnectionException(EL_ERROR, "Unreadable error from server side");
					}	
				}
				else
				{
					throw new ConnectionException(EL_ERROR, "Invalid XML response received from server");
					curl_formfree(post);
					curl_easy_cleanup(curl);
					if(xmlresponse.memory)
          				free(xmlresponse.memory);
					return TRANSFER_GENERAL_ERROR;
				}
				curl_formfree(post);
				curl_easy_cleanup(curl);
				if(xmlresponse.memory)
          			free(xmlresponse.memory);
				return TRANSFER_GENERAL_ERROR;
			}
			else
			{
				curl_formfree(post);
				curl_easy_cleanup(curl);
				if(xmlresponse.memory)
          			free(xmlresponse.memory);
				std::cout << res << std::endl;
				throw new ConnectionException(EL_ERROR, "Error while uploading part content");
			}
		}
		else
		{	
			curl_formfree(post);
			curl_easy_cleanup(curl);
				if(xmlresponse.memory)
          			free(xmlresponse.memory);			
			throw new ConnectionException(EL_ERROR, "Error while creating CURL object");
		}		
 		// free the post data again
 		curl_formfree(post);
		curl_easy_cleanup(curl);
		if(xmlresponse.memory)
          free(xmlresponse.memory);
		return TRANSFER_GENERAL_ERROR;
	}
	catch(SocketException* ex)
	{
		throw new ConnectionException(EL_NOTICE, ex->GetErrorMessage());
	}
	return TRANSFER_GENERAL_ERROR;
}
