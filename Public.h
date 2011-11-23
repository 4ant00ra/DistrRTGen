/*
   RainbowCrack - a general propose implementation of Philippe Oechslin's faster time-memory trade-off technique.

   Copyright (C) Zhu Shuanglei <shuanglei@hotmail.com>
*/

#ifndef _PUBLIC_H
#define _PUBLIC_H

#include <cstdlib>
#include <cstring>
#include <list>
#include <stdio.h>
#include <string>
#include <vector>
using namespace std;

#define MAX_HASH_LEN  256
#define MAX_PLAIN_LEN 256
#define MAX_SALT_LEN  256
#define MIN_HASH_LEN  8
#define uint64 u_int64_t

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

struct RainbowChain
{
	uint64 nIndexS;
	uint64 nIndexE;
};

unsigned int GetFileLen(FILE* file);
string TrimString(string s);
bool ReadLinesFromFile(string sPathName, vector<string>& vLine);
bool SeperateString(string s, string sSeperator, vector<string>& vPart);
string uint64tostr(uint64 n);
string uint64tohexstr(uint64 n);
string HexToStr(const unsigned char* pData, int nLen);
unsigned int GetAvailPhysMemorySize();
void ParseHash(string sHash, unsigned char* pHash, int& nHashLen);

int ston(string);

void Logo();

typedef struct
{
        int nRainbowChainLen;
        int nChainCount;
        uint64 nChainStart;
} DataGenerationThreadParameters;


#endif
