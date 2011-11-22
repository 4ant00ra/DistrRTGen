// rtgen_client.cpp : Defines the entry point for the console application.
//
#include <errno.h>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <stdio.h>
#include <sys/resource.h> //renice main thread
#include <sys/stat.h> // For mkdir()
#include <sys/types.h>
#include <time.h>

#include "config.h"
#include "Public.h"
#include "RainbowTableGenerator.h"
#include "ServerConnector.h"

#define CPU_INFO_FILENAME "/proc/cpuinfo"
#define MAX_PART_SIZE 8000000 //size of PART file
#define CLIENT_WAIT_TIME_SECONDS 600 // Wait 10 min and try again
#define VERSION "1.0 LX"

enum TALKATIVE
{
	TK_ALL = 0,
	TK_WARNINGS,
	TK_ERRORS
};


int main(int argc, char* argv[])
{	
	double nFrequency;
	std::string sHomedir;
	int nNumProcessors = 0;
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
	
	
	// Try to catch cpu Frequency from /proc/cpuinfo
	const char* cpuprefix = "cpu MHz";
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
	
	ServerConnector *Con = new ServerConnector();
	stWorkInfo stWork;
	
	// Check to see if there is something to resume from
	std::ostringstream sResumeFile;
	sResumeFile << sHomedir << "/.distrrtgen/";
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

		szFileName << sHomedir << "/.distrrtgen/" << stWork.nPartID << ".part"; // Store it in the users home directory
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
						int nResult = Con->SendFinishedWork(stWork.nPartID, szFileName.str());
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

		}
		else
		{
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
			setpriority(PRIO_PROCESS, 0, 0);
			try
			{
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
				szFileName << sHomedir << "/.distrrtgen/" << stWork.nPartID << ".part"; // Store it in the users home directory

				// do the work
				int nReturn;
				if(nTalkative <= TK_ALL)
					std::cout << "Starting multithreaded rainbowtable generator..." << std::endl;

				if(nTalkative >= TK_WARNINGS)
					std::freopen("/dev/null", "w", stdout);	


				if((nReturn = pGenerator->CalculateTable(szFileName.str(), stWork.nChainCount, stWork.sHashRoutine, stWork.sCharset, stWork.nMinLetters, stWork.nMaxLetters, stWork.nOffset, stWork.nChainLength, stWork.nChainStart, stWork.sSalt)) != 0)
				{
					if(nTalkative >= TK_WARNINGS)
						std::freopen("/dev/stdout", "w", stdout);	
					std::cout << "Error id " << nReturn << " received while generating table";
					return nReturn;
				}

				if(nTalkative >= TK_WARNINGS)
					std::freopen("/dev/stdout", "w", stdout);	

				if(nTalkative <= TK_ALL)
					std::cout << "Calculations of part " << stWork.nPartID << " completed. Sending data..." << std::endl;
				while(1)
				{
					try
					{
						int nResult = Con->SendFinishedWork(stWork.nPartID, szFileName.str());
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
