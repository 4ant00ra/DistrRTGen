
#include "RainbowTableGenerator.h"
#include "ChainWalkContext.h"
#include <iostream>
#include <sstream>
#include <time.h>
#ifndef WIN32
	#include <sys/resource.h>
#endif
#include "Exception.h"
#include "zlib.h"

int QuickSortPartition(RainbowChain* pChain, int nLow, int nHigh)
{
	int nRandomIndex = nLow + ((unsigned int)rand() * (RAND_MAX + 1) + (unsigned int)rand()) % (nHigh - nLow + 1);
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
		// Get amount of logical processors in Linux
		char cpuinfo[1024];
		FILE* fileCPU = fopen("/proc/cpuinfo", "r");
		if(fileCPU == NULL)
		{
			throw new Exception("Unable to autodetect the processor count. Please edit distrrtgen.conf and manually set the amount of processors");
		}
		m_nProcessorCount = 0;
		while(!feof(fileCPU))
		{
			fgets(cpuinfo, sizeof(cpuinfo), fileCPU);
			if(strstr(cpuinfo, "processor"))
				m_nProcessorCount++;
		}
		fclose(fileCPU);
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

int CRainbowTableGenerator::CalculateTable(std::string sFilename, int nRainbowChainCount, std::string sHashRoutineName, std::string sCharsetName, int nPlainLenMin, int nPlainLenMax, int nRainbowTableIndex, int nRainbowChainLen, uint64 nChainStart, std::string sSalt)
{
	// Setup CChainWalkContext
	if (!CChainWalkContext::SetHashRoutine(sHashRoutineName))
	{
		std::cout << "hash routine " << sHashRoutineName << " not supported" << std::endl;
		return 1;
	}
	if (!CChainWalkContext::SetPlainCharset(sCharsetName, nPlainLenMin, nPlainLenMax))
		return 2;
	if (!CChainWalkContext::SetRainbowTableIndex(nRainbowTableIndex))
	{
		std::cout << "invalid rainbow table index " << nRainbowTableIndex << std::endl;
		return 3;
	}

	if(sHashRoutineName == "mscache")
	{
		int salt_length = 0;
		unsigned char UnicodePlain[MAX_PLAIN_LEN * 2];
		int i;
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
	CChainWalkContext::Dump();	

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
	int nCalculatedChains = nDataLen / 16;
	m_nCurrentCalculatedChains = nCalculatedChains;
	time_t tStart = time(NULL);
	time_t tEnd;
	int nOldCalculatedchains = GetCurrentCalculatedChains();
	int nTotalChainSpeed = 0;
	//renice main thread to +19.
	setpriority(PRIO_PROCESS, 0, 19);
	while(nCalculatedChains < nRainbowChainCount)
	{
		tEnd = time(NULL);
		if(tEnd - tStart > 10)
		{
			system("clear");
			float nPercent = (float)nCalculatedChains / (float)nRainbowChainCount;
			nPercent *= 100;
			std::cout << "Working on: Part " << sFilename << "," << nRainbowChainCount << "," << sHashRoutineName << "," << sCharsetName << "," << nPlainLenMin << "," << nPlainLenMax << "," << nRainbowTableIndex << "," << nRainbowChainLen << std::endl;
			std::cout << "Current chain speed: " << ((GetCurrentCalculatedChains() - nOldCalculatedchains) / 10) << std::endl;			
			std::cout.precision(2);
			std::cout << "Percent completed: " << nPercent << "%" << std::endl;
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
				m_pThreads[i]->Start(&options);			
				nChainStart += 50000;
			}
		}
		Sleep(1);

	}
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
	std::cout << "Sorting file...";
	QuickSort(chains, 0, nRainbowChainCount - 1);
	std::cout << "ok!" << std::endl << "Compressing content before sending";

	uLongf  len = nRainbowChainCount*1.1+12;
	unsigned char *buffer = new unsigned char[len];
	compress((Bytef *)buffer, &len, (Bytef *)chains, nRainbowChainCount * 16);
	FILE *zipFile = fopen(sFilename.append(".zip").c_str(), "wb");
	fwrite(buffer, 1, len, zipFile);
	fclose(zipFile);
	std::cout << "ok!" << std::endl;
	// Done sorting
	return 0;
}
