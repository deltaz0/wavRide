#ifndef HISTORY_H
#define HISTORY_H
#include "tinyxml.h"
#include <string>

static const char* histFilename = "stats.xml";

class CHistory
{
public:
	CHistory();

	~CHistory();

	TiXmlDocument histDoc;
	TiXmlElement* histElem;

	bool addPointToHist(int id, int value, bool jump);
	bool addPointArray(int* pArray, int pNum);
	bool addFloatToHist(int id, float value);
	bool addPJNToHist(int pjn);
	bool addJumpArray(int* jArray, int jNum);
	bool addPosArray(float* posArray, int posNum);
	bool addPreJumpNum(int preJumpNum);
	void clearAndCreateHist();
};

#endif