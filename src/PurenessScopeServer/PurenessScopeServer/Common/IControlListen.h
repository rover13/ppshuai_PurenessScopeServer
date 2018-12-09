#ifndef _ICONTROLLISTEN_H
#define _ICONTROLLISTEN_H

#include "define.h"

typedef struct _ControlInfo
{
    char m_szListenIP[MAX_BUFF_50];
    uint32 m_u4Port;

    _ControlInfo()
    {
        m_szListenIP[0] = '\0';
        m_u4Port        = 0;
    }
}ControlInfo, *PControlInfo, **PPControlInfo;

class IControlListen
{
public:
    virtual ~IControlListen() {}
    virtual bool   AddListen(const char* pListenIP, uint32 u4Port, uint8 u1IPType, int nPacketParseID) = 0; //��һ���µļ����˿�
    virtual bool   DelListen(const char* pListenIP, uint32 u4Port)                                     = 0; //�ر�һ����֪������
	virtual PControlInfo CreateListenSnapshot(int & nControlInfoNum)							       = 0; //�����Ѵ򿪵ļ����˿��б����
	virtual void ReleaseListenSnapshot(PPControlInfo ppControlInfo)								   = 0;	//�ͷ��Ѵ򿪵ļ����˿��б����
	virtual uint32 GetServerID()                                                                       = 0; //�õ���ǰ��������ID
};

#endif
