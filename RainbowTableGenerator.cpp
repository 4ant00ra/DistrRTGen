#include <iostream>
#include <sstream>
#ifndef WIN32
#include <sys/resource.h>
#endif
#include <time.h>
#include <zlib.h>

#include "ClientSocket.h"
#include "ChainWalkContext.h"
#include "RainbowTableGenerator.h"

extern bool debug;

int QuickSortPartition(RainbowChain* pChain, int nLow, int nHigh)
{
	int nRandomIndex = nLow + ((unsigned int)rand() * (RAND_MAX) + 1 + (unsigned int)rand()) % (nHigh - nLow + 1);
	RainbowChain TempChain;
	TempChain = pChain[nLow];
	pChain[nLow] = pChain[nRandomIndex];
	pChain[nRandomIndex] = TempChain;

	TempChain = pChain[nLow];
	uint64 nPivotKey = pChain[nLow].nIndexE;
	while (nLow < nHigh)
	{
		while (nLow < nHigh && pChain[nHigh].nIndexE >= nPivotKey)
			nHigh--;
		pChain[nLow] = pChain[nHigh];
		while (nLow < nHigh && pChain[nLow].nIndexE <= nPivotKey)
			nLow++;
		pChain[nHigh] = pChain[nLow];
	}
	pChain[nLow] = TempChain;
	return nLow;
}

void QuickSort(RainbowChain* pChain, int nLow, int nHigh)
{
	if (nLow < nHigh)
	{
		int nPivotLoc = QuickSortPartition(pChain, nLow, nHigh);
		QuickSort(pChain, nLow, nPivotLoc - 1);
		QuickSort(pChain, nPivotLoc + 1, nHigh);
	}
}

CRainbowTableGenerator::CRainbowTableGenerator(int nNumProcessors)
{
	m_nCalculationSpeed = 0;
	m_nProcessorCount = 0;
	m_nCurrentCalculatedChains = 0;
	if(nNumProcessors == 0)
	{
		#ifdef WIN32
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		m_nProcessorCount = sysInfo.dwNumberOfProcessors;
		#else
		// Get amount of logical processors in Linux
		char cpuinfo[1024];
		FILE* fileCPU = fopen("/proc/cpuinfo", "r");
		if(fileCPU == NULL)
		{
			std::cout << "| Failed to detect CPUs       |" << std::endl;
			std::cout << "+-----------------------------+" << std::endl;
			exit(1);
		}
		m_nProcessorCount = 0;
		while(!feof(fileCPU))
		{
			fgets(cpuinfo, sizeof(cpuinfo), fileCPU);
			if(strstr(cpuinfo, "processor"))
				m_nProcessorCount++;
		}
		fclose(fileCPU);
		#endif
	}
	else
	{
		m_nProcessorCount = nNumProcessors;
	}

	m_pThreads = new CDataGenerationThread*[m_nProcessorCount];
	for(int i = 0; i < m_nProcessorCount; i++)
	{
		m_pThreads[i] = NULL;
	}

}

CRainbowTableGenerator::~CRainbowTableGenerator(void)
{
	delete [] m_pThreads;
}

int CRainbowTableGenerator::CalculateTable(std::string sFilename, stWorkInfo* stWork, CClientSocket** Con)
{
	unsigned int nPlainLenMin 	= stWork->nMinLetters;
	unsigned int nPlainLenMax 	= stWork->nMaxLetters;
	unsigned int nRainbowTableIndex = stWork->nOffset;
	unsigned int nRainbowChainLen 	= stWork->nChainLength;
	unsigned int nRainbowChainCount	= stWork->nChainCount;
	uint64 nChainStart		= stWork->nChainStart;
	std::string sHashRoutineName	= stWork->sHashRoutine;
	std::string sCharsetName	= stWork->sCharset;
	std::string sSalt		= stWork->sSalt;

	// Setup CChainWalkContext
	if (!CChainWalkContext::SetHashRoutine(sHashRoutineName))
		exit(4);
	if (!CChainWalkContext::SetPlainCharset(sCharsetName, nPlainLenMin, nPlainLenMax))
		exit(4);
	if (!CChainWalkContext::SetRainbowTableIndex(nRainbowTableIndex))
		exit(4);

	if(sHashRoutineName == "mscache")
	{
		unsigned char UnicodePlain[MAX_PLAIN_LEN * 2];
		unsigned int i;
		for (i = 0; i < sSalt.length(); i++)
		{
			UnicodePlain[i * 2] = ((unsigned char *)sSalt.c_str())[i];
			UnicodePlain[i * 2 + 1] = 0x00;
		}


		CChainWalkContext::SetSalt((unsigned char*)UnicodePlain, i*2);
	}
	else if(sHashRoutineName == "halflmchall")
	{
	}

	// FileName
	std::ofstream Partfile;
	Partfile.open(sFilename.c_str(), std::ios::out | std::ios::binary | std::ios::app);

	// Open file

	if (Partfile.is_open() == false)
	{
		std::cout << "failed to create " << sFilename << std::endl;
		return 4;
	}


	// Check existing chains

	long begin,end;

	begin = Partfile.tellp();
	Partfile.seekp(0, std::ios::end);
	end = Partfile.tellp();
	unsigned int nDataLen = end - begin;

	nDataLen = nDataLen / 16 * 16;
	if ((int)nDataLen == nRainbowChainCount * 16)
	{
		std::cout << "precomputation of this rainbow table already finished" << std::endl;
		Partfile.close();
		return 0;
	}
	if (nDataLen > 0)
	{
		std::cout << "continuing from interrupted precomputation..." << std::endl;
	}

	Partfile.seekp(0, std::ios::end);

	(*Con)->Progress();
	// Generate rainbow table
	for(int i = 0; i < m_nProcessorCount; i++)
	{
		DataGenerationThreadParameters *options = new DataGenerationThreadParameters();;
		options->nRainbowChainLen = nRainbowChainLen;
		options->nChainCount = 50000; // Split it into 50000 chain chunks
		options->nChainStart = nChainStart;
		m_pThreads[i] = new CDataGenerationThread();
		m_pThreads[i]->Start(options);
		nChainStart += 50000;
	}
	unsigned int nCalculatedChains = nDataLen / 16;
	m_nCurrentCalculatedChains = nCalculatedChains;
	time_t tStart = time(NULL);
	time_t tEnd;
	int nOldCalculatedchains = GetCurrentCalculatedChains();
	//renice main thread to +19.
	#ifndef WIN32
	setpriority(PRIO_PROCESS, 0, 19);
	#endif
	while(nCalculatedChains < nRainbowChainCount)
	{
		tEnd = time(NULL);
		if(tEnd - tStart > 1)
		{

			int nPercent = ((float)nCalculatedChains / (float)nRainbowChainCount) * 100;
			int nRate = (GetCurrentCalculatedChains() - nOldCalculatedchains) / (tEnd - tStart);

			// Display
			if(verbose)
				std::cout << "\n";
			else
				std::cout << "\r";
			std::cout << "| [";
			std::cout << string((int)nPercent/5,'=');
			if(nPercent%5 > 2)
				std::cout << "-";
			std::cout << string(20-(int)nPercent/5 - (nPercent%5 > 2 ? 1:0),'.');
			std::cout <<"] ";
			std::cout.fill(' ');
			std::cout.width(3);
			std::cout << nPercent;
			std::cout << "% |";
			std::cout.flush();
		

			(*Con)->Progress(ston(sFilename),nRate*nRainbowChainLen,nPercent);
			nOldCalculatedchains = m_nCurrentCalculatedChains;
			tStart = time(NULL);
		}

		for(int i = 0; i < m_nProcessorCount; i++)
		{
			if(m_pThreads[i]->GetIsDataReadyFlag() > 0)
			{

				// Retrieve the calculated data
				const char *data = (const char *)m_pThreads[i]->GetData();
				// Write it to the file
				int nNewChains = DATA_CHUNK_SIZE / 16;
				// Check if too many chains is calculated
				if(nRainbowChainCount < (nCalculatedChains + nNewChains))
				{
					// If this is the fact, reduce the amount of chains we copy
					nNewChains = nRainbowChainCount - nCalculatedChains;
				}
				if(nNewChains == 0)
					break;
				nCalculatedChains += nNewChains;

				Partfile.write(data, nNewChains * 16);
				Partfile.flush();

				// And mark the buffer as empty
				m_pThreads[i]->ClearDataReadyFlag();
			}
			if(m_pThreads[i]->IsTerminated() == 1)
			{
				DataGenerationThreadParameters *options = new DataGenerationThreadParameters();;
				options->nRainbowChainLen = nRainbowChainLen;
				options->nChainCount = 50000; // Split it into 50000 chain chunks
				options->nChainStart = nChainStart;
				m_pThreads[i]->Start(options);			
				nChainStart += 50000;
			}
		}

	}
	std::cout << "\r| [====================] 100% |\n";
	// Stop the threads again and destroy them
	for(int i = 0; i < m_nProcessorCount; i++)
	{
		m_pThreads[i]->Stop();
		delete m_pThreads[i];
		m_pThreads[i] = NULL;
	}	

	Partfile.close();
	// Perform a sorting of the generated chains
	FILE *partFile = fopen(sFilename.c_str(), "rb");
	RainbowChain *chains = new RainbowChain[nRainbowChainCount];
	fread(chains, 16, nRainbowChainCount, partFile);
	fclose(partFile);

	if(!debug)
		QuickSort(chains, 0, nRainbowChainCount - 1);

	uLongf len = compressBound(nRainbowChainCount * 16);
	unsigned char *buffer = new unsigned char[len];
	compress2((Bytef *)buffer, &len, (Bytef *)chains, nRainbowChainCount * 16, 9);
	FILE *zipFile = fopen(sFilename.append(".zip").c_str(), "wb");
	fwrite(buffer, 1, len, zipFile);
	fclose(zipFile);
	// Done sorting*/

	// De-allocate memory ;)
	delete[] chains;
	delete[] buffer;	
	(*Con)->Done();
	return 0;
}
