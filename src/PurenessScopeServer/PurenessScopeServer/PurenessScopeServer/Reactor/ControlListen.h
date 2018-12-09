#ifndef _CONTROLLISTEN_H
#define _CONTROLLISTEN_H

#include "ConnectAccept.h"
#include "AceReactorManager.h"
#include "IControlListen.h"

class CControlListen : public IControlListen
{
public:
    CControlListen();
    virtual ~CControlListen();

    bool   AddListen(const char* pListenIP, uint32 u4Port, uint8 u1IPType, int nPacketParseID);  //��һ���µļ����˿�
    bool   DelListen(const char* pListenIP, uint32 u4Port);                                      //�ر�һ����֪������
	PControlInfo CreateListenSnapshot(int & nControlInfoNum);									 //�����Ѵ򿪵ļ����˿��б����
	void ReleaseListenSnapshot(PPControlInfo ppControlInfo);									 //�ͷ��Ѵ򿪵ļ����˿��б����								
    uint32 GetServerID();                                                                        //�õ�������ID
};

typedef ACE_Singleton<CControlListen, ACE_Null_Mutex> App_ControlListen;

#endif
