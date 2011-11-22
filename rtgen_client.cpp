// rtgen_client.cpp : Defines the entry point for the console application.
//

#ifdef _WIN32
	#pragma warning(disable : 4786)
#endif
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <stdio.h>
#include "Public.h"
#include "ServerConnector.h"
#include "RainbowTableGenerator.h"
#include "WU_mgr.h"
#ifndef WIN32
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h> // For mkdir()
#include <sys/resource.h> //renice main thread
#define CPU_INFO_FILENAME "/proc/cpuinfo"
#endif
#define MAX_PART_SIZE 8000000 //size of PART file
#define CLIENT_WAIT_TIME_SECONDS 600 // Wait 10 min and try again
#define VERSION "4.0 LX"

enum TALKATIVE
{
	TK_ALL = 0,
	TK_WARNINGS,
	TK_ERRORS
};


int main(int argc, char* argv[])
{	

	std::string sUsername;
	std::string sPassword;
	double nFrequency;
	std::string sHostname;
	std::string sHomedir;
	int nNumProcessors = 0;
	int nClientID;
    int nTalkative = TK_ALL;
	
	if(argc > 1)
	{
		if(strcmp(argv[1], "-q") == 0)
		{
			nTalkative = TK_WARNINGS;
		}
		else if(strcmp(argv[1], "-Q") == 0)
		{
			nTalkative = TK_ERRORS;
		}
	}
	nTalkative = TK_ALL;
	// with which MACRO I have been compiled with..
	#ifdef _FAST_HASH_
		if(nTalkative <= TK_ALL)
			std::cout << "Compiled with Fast and thread safe Hashroutines" << std::endl;
	#endif
	#ifdef _FAST_MD5_
		if(nTalkative <= TK_ALL)
			std::cout << "Compiled with Fast and thread unsafe MD5 Hashroutine" << std::endl;
	#endif
	// First load the client identification information
	std::ostringstream sClientInfo;
#ifndef WIN32
	struct passwd *userinfo;
	userinfo = getpwuid(getuid());
	sHomedir = userinfo->pw_dir;
	sClientInfo << sHomedir  << "/.distrrtgen/";
#endif
	
	sClientInfo << ".client";
	std::fstream fClientInfo(sClientInfo.str().c_str(), std::fstream::in);

	if(fClientInfo.is_open() == false) nClientID = 0;
	else 
	{
		fClientInfo >> nClientID;
		fClientInfo.close();
	}
	// Then load the client configuration
	std::ostringstream sConf;
#ifndef WIN32
	sConf << sHomedir << "/.distrrtgen/";
#endif
	sConf << "distrrtgen.conf";
	std::fstream fConfig(sConf.str().c_str(), std::ifstream::in);
	if(fConfig.is_open() == false)
	{
		std::cout << sConf.str() << " could not be opened - " << strerror(errno) << std::endl;
		return 1;
	}
//	std::string sUsername, sPassword;
	if( !(fConfig >> sUsername) )
	{
		std::cout << "Missing username line, check version of " << sConf.str() << std::endl;
		return 1;
	}
	if( !(fConfig >> sPassword) )
	{
		std::cout << "Missing password line, check version of " << sConf.str() << std::endl;
		return 1;
	}
	if( !(fConfig >> sHostname) )
	{
		std::cout << "Missing hostname line, check version of " << sConf.str() << std::endl;
		return 1;
	}
	fConfig.close();
	sUsername = sUsername.substr(9);
	sPassword = sPassword.substr(9);
	sHostname = sHostname.substr(9);
	if(sUsername == "")
	{
		std::cout << "No username configured in " << sConf.str() << std::endl;
		return 1;
	}
	if(sPassword == "")
	{
		std::cout << "No password configured in " << sConf.str() << std::endl;
		return 1;
	}
	if(sHostname == "")
	{
		std::cout << "No Hostname configured in " << sConf.str() << std::endl;
		return 1;
	}
	
	wu_mgr *WUFile = new wu_mgr();
	//WUFile->read("test.xml");
	
	
	//if(sProcessors.substr(7).length() > 0) // Check if processor count is manually configured
	//{
	//	nNumProcessors = atoi(sProcessors.substr(7).c_str());
	//}
	// If numprocessors is 0, RainbowTableGenerator.cpp will try to detect it itself
	
	// Try to catch cpu Frequency from /proc/cpuinfo
#ifdef WIN32
	nFrequency = 0;
#else
	const char* cpuprefix = "cpu MHz";
	const char* cpunumber = "processor";
	FILE* F;
	char cpuline[300+1];
	char* pos;
	int ok = 0;
	nNumProcessors = 0;
	
	// open cpuinfo system file
	F = fopen(CPU_INFO_FILENAME,"r");
	if (!F) return 0;
	
	//read lines
	while (!feof(F))
  	{
    fgets (cpuline, sizeof(cpuline), F);
	//test if it's a processor id line
	/* deprecated
	if (!strncmp(cpuline, cpunumber, strlen(cpunumber)))
	{
		
	}
    */
    // test if it's the frequency line
    if (!strncmp(cpuline, cpuprefix, strlen(cpuprefix)))
    	{
      		// Yes, grep the frequency
      		pos = strrchr (cpuline, ':') +2;
      		if (!pos) break;
      		if (pos[strlen(pos)-1] == '\n') pos[strlen(pos)-1] = '\0';
      		strcpy (cpuline, pos);
      		strcat (cpuline,"e6");
      		nFrequency = atof (cpuline)/1000000;
      		ok = 1;
      		//break;  //bug : http://www.freerainbowtables.com/phpBB3/viewtopic.php?f=4&p=916&sid=53804aa79a7bc4bb06cff38481889cf7#p909
    	}
  	}
	nNumProcessors = sysconf(_SC_NPROCESSORS_ONLN);
	if(nTalkative <= TK_ALL)
		 std::cout << nNumProcessors <<" processor(s) found." << std::endl;
	
	if (ok == 1)
	{
		if(nTalkative <= TK_ALL)
			std::cout << "CPU frequency has been found : " << nFrequency << " MHz" << std::endl;
	}
	else
	{
		if(nTalkative <= TK_ALL)
			std::cout << "Unable to get cpu frequency from /proc/cpuinfo." << std::endl;
		exit(-1);
	}
	
	#endif
	
	ServerConnector *Con = new ServerConnector();
	Con->Login(sUsername, sPassword, sHostname, nClientID, nFrequency);
	stWorkInfo stWork;
	
	// Check to see if there is something to resume from
	std::ostringstream sResumeFile;
#ifndef WIN32
	sResumeFile << sHomedir << "/.distrrtgen/";
#endif
	sResumeFile << ".resume";
	FILE *file = fopen(sResumeFile.str().c_str(), "rb");
	if(file != NULL)
	{
		// Bingo.. There is a resume file.
		fread(&stWork, sizeof(unsigned int), 6, file);
		fread(&stWork.nChainStart, sizeof(uint64), 1, file);
		char buf[8096];
		memset(buf, 0x00, sizeof(buf));
		fread(&buf[0], sizeof(buf), 1, file);
		fclose(file);
		char szCharset[8096], szHR[8096];
		strcpy(&szCharset[0], &buf[0]);
		stWork.sCharset.assign(szCharset);
		const char *pHR = strchr(&buf[0], 0x00);
		pHR++;
		strcpy(&szHR[0], pHR);
		stWork.sHashRoutine.assign(szHR);
		pHR = strchr(pHR, 0x00);
		pHR++;
		strcpy(&szHR[0], pHR);
		stWork.sSalt.assign(szHR);
		//before continuing, test if part file is <8MB sized
		const char * cFileName;
		std::string sFileName;
		std::stringstream szFileName;
#ifdef WIN32
		szFileName << stWork.nPartID << ".part"; // Store it in the users home directory
#else
		szFileName << sHomedir << "/.distrrtgen/" << stWork.nPartID << ".part"; // Store it in the users home directory
#endif
		sFileName = szFileName.str();
		cFileName = sFileName.c_str();
		FILE *partfile = fopen(cFileName,"rb");
		long size;
		if(partfile != NULL)
		{
			fseek(partfile,0,SEEK_END);
			size=ftell(partfile);
			rewind(partfile);
			fclose(partfile);
			if(nTalkative <= TK_ALL)
				std::cout << "Part file size (in bytes) : " << size << std::endl;
			
			if (size != MAX_PART_SIZE)
			{
				if(nTalkative <= TK_ALL)
					std::cout << "Deleting " << cFileName << std::endl;
				if( remove(cFileName) != 0 )
				{
					if(nTalkative <= TK_ALL)
						std::cout << "Error deleting file, please manually delete it." << std::endl;
					exit(-1);
				}
  				else
    				if(nTalkative <= TK_ALL)
						std::cout << "File successfully deleted." << std::endl;
			}
		}
		else
		{
			if(nTalkative <= TK_ALL)
				std::cout << "No unfinished part file." << std::endl;
		}
			
		if (size==MAX_PART_SIZE)
		{
			if(nTalkative <= TK_ALL)
				std::cout << "File already completed... try uploading" << std::endl;
			while(1)
				{
					try
					{
						int nResult = Con->SendFinishedWork(stWork.nPartID, szFileName.str(), sUsername, sPassword);
						switch(nResult)			
						{
						case TRANSFER_OK:
							if(nTalkative <= TK_ALL)
								std::cout << "Data delivered!" << std::endl;
								remove(szFileName.str().c_str());		
								stWork.sCharset = ""; // Blank out the charset to indicate the work is complete
								unlink(sResumeFile.str().c_str());
							break;
						case TRANSFER_NOTREGISTERED:
							if(nTalkative <= TK_ALL)
								std::cout << "Data was not accepted by the server. Dismissing" << std::endl;
								remove(szFileName.str().c_str());		
								stWork.sCharset = ""; //We let the charset is order to retry
								unlink(sResumeFile.str().c_str()); //We remove the part but not the resume file, to restart
							break;
						case TRANSFER_GENERAL_ERROR:
							if(nTalkative <= TK_ALL)
								std::cout << "Could not transfer data to server. Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
							Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
							continue;
						}
						break;
					}
					catch(SocketException *ex)
					{
						std::cout << "Error connecting to server: " << ex->GetErrorMessage() << ". Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
						delete ex;
						Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
					}
					catch(ConnectionException *ex)
					{
						if(ex->GetErrorLevel() >= nTalkative)
							std::cout << ex->GetErrorMessage() << ". Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
						delete ex;
						Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
					}
				}
				
				//remove(szFileName.str().c_str());		
				//stWork.sCharset = ""; // Blank out the charset to indicate the work is complete
				/*std::ostringstream sResumeFile;
				#ifndef WIN32
					sResumeFile << sHomedir << "/.distrrtgen/";
				#endif
				sResumeFile << ".resume";*/
				//unlink(sResumeFile.str().c_str());
				
		}
		else
		{
			
			// We delete the old part file
			/*char * cDeleteFile;
			sprintf(cDeleteFile, "/bin/rm -f %s", cFileName);
			system(cDeleteFile);*/
			if(nTalkative <= TK_ALL)	
				std::cout << "Delete Part file and restart interrupted computations..." << std::endl;
			remove(szFileName.str().c_str());
		}
	}
	try
	{
		if(nTalkative <= TK_ALL)
			std::cout << "Initializing DistrRTgen " << VERSION << std::endl;
		CRainbowTableGenerator *pGenerator = new CRainbowTableGenerator(nNumProcessors);
		if(nTalkative <= TK_ALL)
			std::cout << "Generating using " << pGenerator->GetProcessorCount() << " processor(s)..." << std::endl;

		while(1)
		{
			//renice main thread to 0.
#ifndef WIN32
			setpriority(PRIO_PROCESS, 0, 0);
#endif
			try
			{
				if(nClientID == 0) // This client doesn't have an ID. 
				{   // We connect to the server and register ourself
					//ServerConnector *Con = new ServerConnector();
					if(nTalkative <= TK_ALL)
						std::cout << "Connecting to server to perform first time registration...";
					if(nTalkative <= TK_ALL)
					{
						std::cout << "OK" << std::endl;
						std::cout << "Performing logon...";
					}
					int errorCode = Con->RegisterNewClient(nClientID);
					if(errorCode > 1)
					{
						if(errorCode == 1)
						{
							std::cout << "Failed to register new client. Invalid username/password combination" << std::endl;
						}
						else
						{
							std::cout << "Failed to register new client. Unknown error code " << errorCode << " recieved" << std::endl;
						}
						return 1;
					}
					if(nTalkative <= TK_ALL)
						std::cout << "client num : " << nClientID << std::endl;
					std::fstream fClient(sClientInfo.str().c_str(), std::fstream::out);
					if(fClient.is_open() == false)
					{
						std::cout << "Could not open " << sClientInfo.str() << " for writing" << std::endl;
						nClientID = 0;
					}
					else 
					{
						fClient << nClientID;
						fClient.close();
					}
				}

				// If there is no work to do, request some!
				if(stWork.sCharset == "")
				{
					if(nTalkative <= TK_ALL)
					{
						std::cout << "OK" << std::endl;
						std::cout << "Requesting work...";
					}
					int errorCode = Con->RequestWork(&stWork);

					if(errorCode > 1)
					{
						if(errorCode == 1)
						{
							std::cout << "Failed to request work. Invalid username/password combination" << std::endl;
						}
						else
						{
							std::cout << "Failed to request work. Unknown error code recieved" << errorCode << " recieved" << std::endl;
						}
						return 1;						
					}					
					if(nTalkative <= TK_ALL)
						std::cout << "work received !" << std::endl;
					FILE *fileResume = fopen(sResumeFile.str().c_str(), "wb");
					if(fileResume == NULL)
					{
						std::cout << "Unable to open " << sResumeFile.str() << " for writing" << std::endl;
						return 1;
					}
					fwrite(&stWork, sizeof(unsigned int), 6, fileResume); // Write the 6 unsigned ints
					fwrite(&stWork.nChainStart, 1, 8, fileResume); // Write nChainStart uint64
					fwrite(stWork.sCharset.c_str(), stWork.sCharset.length(), 1, fileResume);
					fputc(0x00, fileResume);
					fwrite(stWork.sHashRoutine.c_str(), stWork.sHashRoutine.length(), 1, fileResume);
					fclose(fileResume);
					
				}
				std::stringstream szFileName;
#ifdef WIN32
				szFileName << stWork.nPartID << ".part";
#else
				szFileName << sHomedir << "/.distrrtgen/" << stWork.nPartID << ".part"; // Store it in the users home directory
#endif
				// do the work
				int nReturn;
				if(nTalkative <= TK_ALL)
					std::cout << "Starting multithreaded rainbowtable generator..." << std::endl;
#ifndef WIN32
				if(nTalkative >= TK_WARNINGS)
					std::freopen("/dev/null", "w", stdout);	
#endif				

				if((nReturn = pGenerator->CalculateTable(szFileName.str(), stWork.nChainCount, stWork.sHashRoutine, stWork.sCharset, stWork.nMinLetters, stWork.nMaxLetters, stWork.nOffset, stWork.nChainLength, stWork.nChainStart, stWork.sSalt)) != 0)
				{
					if(nTalkative >= TK_WARNINGS)
						std::freopen("/dev/stdout", "w", stdout);	
					std::cout << "Error id " << nReturn << " received while generating table";
					return nReturn;
				}
#ifndef WIN32
				if(nTalkative >= TK_WARNINGS)
					std::freopen("/dev/stdout", "w", stdout);	
#endif
				if(nTalkative <= TK_ALL)
					std::cout << "Calculations of part " << stWork.nPartID << " completed. Sending data..." << std::endl;
				while(1)
				{
					try
					{
						int nResult = Con->SendFinishedWork(stWork.nPartID, szFileName.str(), sUsername, sPassword);
						switch(nResult)			
						{
						case TRANSFER_OK:
							if(nTalkative <= TK_ALL)
								std::cout << "Data delivered!" << std::endl;
							remove(szFileName.str().c_str());		
							stWork.sCharset = ""; // Blank out the charset to indicate the work is complete
							unlink(sResumeFile.str().c_str());
							break;
						case TRANSFER_NOTREGISTERED:
							if(nTalkative <= TK_ALL)
								std::cout << "Data was not accepted by the server. Dismissing" << std::endl;
							remove(szFileName.str().c_str());
							stWork.sCharset = ""; // Blank out the charset to indicate the work is complete
							unlink(sResumeFile.str().c_str());							
							break;
						case TRANSFER_GENERAL_ERROR:
							if(nTalkative <= TK_ALL)
								std::cout << "Could not transfer data to server. Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
							Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
							continue;
						}
						break;
					}
					catch(SocketException *ex)
					{
						std::cout << "Error connecting to server: " << ex->GetErrorMessage() << ". Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
						delete ex;
						Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
					}
					catch(ConnectionException *ex)
					{
						if(ex->GetErrorLevel() >= nTalkative)
							std::cout << ex->GetErrorMessage() << ". Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
						delete ex;
						Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
					}
				}
				//remove(szFileName.str().c_str());		
				//stWork.sCharset = ""; // Blank out the charset to indicate the work is complete
				//unlink(sResumeFile.str().c_str());
			}
			catch(SocketException *ex)
			{
				if(nTalkative <= TK_WARNINGS)
					std::cout << "Could not connect to server: " << ex->GetErrorMessage() << ". Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
				delete ex;
				Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
			}
			catch(ConnectionException *ex)
			{
				if(ex->GetErrorLevel() >= nTalkative)
					std::cout << ex->GetErrorMessage() << ". Retrying in " << CLIENT_WAIT_TIME_SECONDS / 60 << " minutes" << std::endl;
				delete ex;
				Sleep(CLIENT_WAIT_TIME_SECONDS * 1000);
			}
		}	
	} 
	catch (...) 
	{
		if(nTalkative <= TK_ERRORS)
			std::cerr << "Unhandled exception :(" << std::endl;
	}
  return 0;
}
