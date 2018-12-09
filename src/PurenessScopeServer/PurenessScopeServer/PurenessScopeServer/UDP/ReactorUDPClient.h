#ifndef _REAACTORUDPCLIENT_H
#define _REAACTORUDPCLIENT_H

#include "ace/ACE.h"
#include "ace/OS_main.h"
#include "ace/OS_NS_string.h"
#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Dgram.h"

#include "define.h"
#include "MessageBlockManager.h"
#include "ClientUDPMassage.h"

class CReactorUDPClient : public ACE_Event_Handler
{
public:
    CReactorUDPClient(void);
    ~CReactorUDPClient(void);

    virtual ACE_HANDLE get_handle(void) const;
    virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
    virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

    int  OpenAddress(const ACE_INET_Addr& AddrRemote, EM_UDP_TYPE emType, ACE_Reactor* pReactor, IClientUDPMessage* pClientUDPMessage);
    void Close();
    bool SendMessage(const char* pMessage, uint32 u4Len, const char* szIP, int nPort);
    _ClientConnectInfo GetClientConnectInfo();

private:
    bool CheckMessage(const char* pData, uint32 u4Len);     //����������ݰ����������ݶ���

private:
    ACE_SOCK_Dgram          m_skRemote;
    ACE_INET_Addr           m_addrRemote;                   //���ݷ��ͷ���IP��Ϣ
    IClientUDPMessage*      m_pClientUDPMessage;            //��Ϣ������

    ACE_Time_Value          m_atvInput;                     //���հ���ʱ��
    ACE_Time_Value          m_atvOutput;                    //���Ͱ���ʱ��
    uint32                  m_u4RecvPacketCount;            //�������ݰ�������
    uint32                  m_u4SendPacketCount;            //�������ݰ�������
    uint32                  m_u4RecvSize;                   //�������ݵ��ܴ�С
    uint32                  m_u4SendSize;                   //�������ݵ��ܴ�С
};
#endif