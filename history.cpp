#include "history.h"
#include "tinyxml.h"
#include <string>
#include <iostream>

using namespace std;

TiXmlDocument histDoc;
TiXmlElement* histElem;


CHistory::CHistory(){
	if(!histDoc.LoadFile(histFilename))
			clearAndCreateHist();
	if(histDoc.RootElement()!=NULL){
		histElem = histDoc.RootElement();
		while(histElem->NextSiblingElement()!=0){
			histElem = histElem->NextSiblingElement();
		}
	}
}

CHistory::~CHistory(){
}

bool CHistory::addPointToHist(int id, int value, bool jump){
	TiXmlElement* pidElem;
	if(!jump)
		pidElem = new TiXmlElement("pId");
	else
		pidElem = new TiXmlElement("jId");
	char ptempId[48];
	_itoa_s(id,ptempId,10);
	TiXmlText* pidText = new TiXmlText(ptempId);
	TiXmlElement* ptextElem = new TiXmlElement("value");
	TiXmlText* ptextText;
	if(value==0)
		ptextText = new TiXmlText("false");
	else
		ptextText = new TiXmlText("true");
	histElem->LinkEndChild(ptextElem);
	ptextElem->LinkEndChild(ptextText);
	histElem->LinkEndChild(pidElem);
	pidElem->LinkEndChild(pidText);

	histDoc.SaveFile();

	return true;
}

bool CHistory::addPJNToHist(int pjn){
	TiXmlElement* pidElem = new TiXmlElement("preJumpNum");
	char ptempId[48];
	_itoa_s(pjn,ptempId,10);
	TiXmlText* pidText = new TiXmlText(ptempId);

	histElem->LinkEndChild(pidElem);
	pidElem->LinkEndChild(pidText);

	histDoc.SaveFile();

	return true;
}

bool CHistory::addFloatToHist(int id, float value){
	TiXmlElement* pidElem = new TiXmlElement("posID");
	char ptempId[48];
	_itoa_s(id,ptempId,10);
	TiXmlText* pidText = new TiXmlText(ptempId);
	TiXmlElement* ptextElem = new TiXmlElement("value");
	char buf[48];
	sprintf_s(buf, "%.3f", value);
	TiXmlText* ptextText = new TiXmlText(buf);
	histElem->LinkEndChild(ptextElem);
	ptextElem->LinkEndChild(ptextText);
	histElem->LinkEndChild(pidElem);
	pidElem->LinkEndChild(pidText);

	histDoc.SaveFile();

	return true;
}

void CHistory::clearAndCreateHist(){
	TiXmlDocument tempDoc;
	TiXmlDeclaration* dec1 = new TiXmlDeclaration("1.0","","");
	tempDoc.LinkEndChild(dec1);
	
	TiXmlElement* ptextElem = new TiXmlElement("container");
	tempDoc.LinkEndChild(ptextElem);

	histDoc = tempDoc;
	histDoc.SaveFile(histFilename);
	histDoc.LoadFile(histFilename);
}

bool CHistory::addPointArray(int* pArray, int pNum){
	for(int i=0;i<pNum;i++)
		addPointToHist(i,pArray[i],false);
	return true;
}

bool CHistory::addJumpArray(int* jArray, int jNum){
	for(int i=0;i<jNum;i++)
		addPointToHist(i,jArray[i],true);
	return true;
}

bool CHistory::addPosArray(float* posArray, int posNum){
	for(int i=0;i<posNum;i++)
		addFloatToHist(i,posArray[i]);
	return true;
}

bool CHistory::addPreJumpNum(int preJumpNum){
	addPJNToHist(preJumpNum);
	return true;
}