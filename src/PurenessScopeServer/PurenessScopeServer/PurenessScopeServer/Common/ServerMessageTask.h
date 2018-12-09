#ifndef _SERVERMESSAGETASK_H
#define _SERVERMESSAGETASK_H

#include "define.h"
#include "ace/Task.h"
#include "ace/Synch.h"
#include "ace/Malloc_T.h"
#include "ace/Singleton.h"
#include "ace/Thread_Mutex.h"
#include "ace/Date_Time.h"

#include "IClientManager.h"
#include "MessageBlockManager.h"
#include "HashTable.h"

using namespace std;

//�����������������ݰ����̴���
//������������̴߳�������ˣ��᳢����������
//add by freeeyes

#define MAX_SERVER_MESSAGE_QUEUE 1000    //���������г���
#define MAX_DISPOSE_TIMEOUT      30      //�������ȴ�����ʱ��  

#define ADD_SERVER_CLIENT  ACE_Message_Block::MB_USER + 1    //���ClientMessage�첽����
#define DEL_SERVER_CLIENT  ACE_Message_Block::MB_USER + 2    //ɾ��ClientMessage�첽����

//��������ͨѶ�����ݽṹ�����հ���
struct _Server_Message_Info
{
    IClientMessage*    m_pClientMessage;
    uint16             m_u2CommandID;
    ACE_Message_Block* m_pRecvFinish;
    _ClientIPInfo      m_objServerIPInfo;
    int                m_nHashID;

    ACE_Message_Block* m_pmbQueuePtr;        //��Ϣ����ָ���

    _Server_Message_Info()
    {
        m_nHashID        = 0;
        m_u2CommandID    = 0;
        m_pClientMessage = NULL;
        m_pRecvFinish    = NULL;

        //����������Ϣ����ģ��ָ�����ݣ������Ͳ��ط�����new��delete����������
        //ָ���ϵҲ����������ֱ��ָ��������ʹ�õ�ʹ����ָ��
        m_pmbQueuePtr  = new ACE_Message_Block(sizeof(_Server_Message_Info*));

        _Server_Message_Info** ppMessage = (_Server_Message_Info**)m_pmbQueuePtr->base();
        *ppMessage = this;
    }

    //�������캯��
    _Server_Message_Info(const _Server_Message_Info& ar)
    {
        this->m_pClientMessage           = ar.m_pClientMessage;
        this->m_u2CommandID              = ar.m_u2CommandID;
        this->m_pRecvFinish              = ar.m_pRecvFinish;
        sprintf_safe(this->m_objServerIPInfo.m_szClientIP, MAX_BUFF_50, "%s", ar.m_objServerIPInfo.m_szClientIP);
        this->m_objServerIPInfo.m_nPort  = ar.m_objServerIPInfo.m_nPort;
        this->m_nHashID                  = ar.m_nHashID;
        this->m_pmbQueuePtr              = new ACE_Message_Block(sizeof(_Server_Message_Info*));
        _Server_Message_Info** ppMessage = (_Server_Message_Info**)m_pmbQueuePtr->base();
        *ppMessage = this;
    }

    _Server_Message_Info& operator = (const _Server_Message_Info& ar)
    {
        this->m_pClientMessage = ar.m_pClientMessage;
        this->m_u2CommandID = ar.m_u2CommandID;
        this->m_pRecvFinish = ar.m_pRecvFinish;
        this->m_objServerIPInfo = ar.m_objServerIPInfo;
        this->m_nHashID = ar.m_nHashID;

        memcpy_safe((char* )ar.m_pmbQueuePtr->base(), (uint32)ar.m_pmbQueuePtr->length(), m_pmbQueuePtr->base(), (uint32)m_pmbQueuePtr->length());
        _Server_Message_Info** ppMessage = (_Server_Message_Info**)m_pmbQueuePtr->base();
        *ppMessage = this;
        return *this;
    }

    ~_Server_Message_Info()
    {
        if(NULL != m_pmbQueuePtr)
        {
            m_pmbQueuePtr->release();
            m_pmbQueuePtr = NULL;
        }
    }

    ACE_Message_Block* GetQueueMessage()
    {
        return m_pmbQueuePtr;
    }

    void SetHashID(int nHashID)
    {
        m_nHashID = nHashID;
    }

    int GetHashID()
    {
        return m_nHashID;
    }

};

#define MAX_SERVER_MESSAGE_INFO_COUNT 100

//_Server_Message_Info�����
class CServerMessageInfoPool
{
public:
    CServerMessageInfoPool();
    ~CServerMessageInfoPool();

    void Init(uint32 u4PacketCount = MAX_SERVER_MESSAGE_INFO_COUNT);
    void Close();

    _Server_Message_Info* Create();
    bool Delete(_Server_Message_Info* pMakePacket);

    int GetUsedCount();
    int GetFreeCount();

private:
    CHashTable<_Server_Message_Info> m_objServerMessageList;           //Server Message�����
    ACE_Recursive_Thread_Mutex       m_ThreadWriteLock;                //���ƶ��߳���
};

//�����������ݰ���Ϣ���д������
class CServerMessageTask : public ACE_Task<ACE_MT_SYNCH>
{
public:
    CServerMessageTask();
    ~CServerMessageTask();

    virtual int open(void* args = 0);
    virtual int svc (void);

    virtual int handle_signal (int signum,siginfo_t*   = 0,ucontext_t* = 0);

    bool Start();
    int  Close();

    uint32 GetThreadID();

    bool PutMessage(_Server_Message_Info* pMessage);

    bool PutMessage_Add_Client(IClientMessage* pClientMessage);

    bool PutMessage_Del_Client(IClientMessage* pClientMessage);

    bool CheckServerMessageThread(ACE_Time_Value tvNow);

private:
    bool CheckValidClientMessage(IClientMessage* pClientMessage);
    bool ProcessMessage(_Server_Message_Info* pMessage, uint32 u4ThreadID);

    virtual int CloseMsgQueue();

private:
    //�ر���Ϣ������������
    ACE_Thread_Mutex m_mutex;
    ACE_Condition<ACE_Thread_Mutex> m_cond;
private:
    uint32               m_u4ThreadID;  //��ǰ�߳�ID
    bool                 m_blRun;       //��ǰ�߳��Ƿ�����
    uint32               m_u4MaxQueue;  //�ڶ����е�����������
    EM_Server_Recv_State m_emState;     //����״̬
    ACE_Time_Value       m_tvDispose;   //�������ݰ�����ʱ��

    //��¼��ǰ��Ч��IClientMessage����Ϊ���첽�Ĺ�ϵ��
    //������뱣֤�ص���ʱ��IClientMessage�ǺϷ��ġ�
    typedef vector<IClientMessage*> vecValidIClientMessage;
    vecValidIClientMessage m_vecValidIClientMessage;
};

class CServerMessageManager
{
public:
    CServerMessageManager();
    ~CServerMessageManager();

    void Init();

    bool Start();
    int  Close();
    bool PutMessage(_Server_Message_Info* pMessage);
    bool CheckServerMessageThread(ACE_Time_Value tvNow);

    bool AddClientMessage(IClientMessage* pClientMessage);
    bool DelClientMessage(IClientMessage* pClientMessage);

private:
    CServerMessageTask*         m_pServerMessageTask;
    ACE_Recursive_Thread_Mutex  m_ThreadWritrLock;
};

typedef ACE_Singleton<CServerMessageManager, ACE_Recursive_Thread_Mutex> App_ServerMessageTask;
typedef ACE_Singleton<CServerMessageInfoPool, ACE_Recursive_Thread_Mutex> App_ServerMessageInfoPool;
#endif
