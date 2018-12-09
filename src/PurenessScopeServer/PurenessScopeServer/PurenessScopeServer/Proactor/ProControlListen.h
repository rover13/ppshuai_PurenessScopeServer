#ifndef _PROCONTROLLISTEN_H
#define _PROCONTROLLISTEN_H

#include "ProConnectAccept.h"
#include "AceProactorManager.h"
#include "IControlListen.h"

//���ӹ����࣬���ڴ���ɾ���Ͳ鿴����
//add by freeeyes
class CProControlListen : public IControlListen
{
public:
    CProControlListen();
    virtual ~CProControlListen();

    bool   AddListen(const char* pListenIP, uint32 u4Port, uint8 u1IPType, int nPacketParseID);  //��һ���µļ����˿�
    bool   DelListen(const char* pListenIP, uint32 u4Port);                                      //�ر�һ����֪������
	PControlInfo CreateListenSnapshot(int & nControlInfoNum);									 //�����Ѵ򿪵ļ����˿��б����
	void ReleaseListenSnapshot(PPControlInfo ppControlInfo);									 //�ͷ��Ѵ򿪵ļ����˿��б����
    uint32 GetServerID();                                                                        //�õ�������ID
};

typedef ACE_Singleton<CProControlListen, ACE_Null_Mutex> App_ProControlListen;

#endif
