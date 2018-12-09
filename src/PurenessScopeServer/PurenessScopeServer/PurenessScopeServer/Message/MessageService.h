#ifndef _MESSAGESERVICE_H
#define _MESSAGESERVICE_H

#include "define.h"
#include "ace/Task.h"
#include "ace/Synch.h"
#include "ace/Malloc_T.h"
#include "ace/Singleton.h"
#include "ace/Thread_Mutex.h"
#include "ace/Date_Time.h"

#include "Message.h"
#include "MessageManager.h"
#include "LogManager.h"
#include "ThreadInfo.h"
#include "BuffPacket.h"
#include "MainConfig.h"
#include "TimerManager.h"
#include "RandomNumber.h"
#include "WorkThreadAI.h"
#include "CommandAccount.h"
#include "MessageDyeingManager.h"

#ifdef WIN32
#include "ProConnectHandle.h"
#include "WindowsCPU.h"
#else
#include "ConnectHandler.h"
#include "LinuxCPU.h"
#endif

//AI������Ϣ��
typedef vector<_WorkThreadAIInfo> vecWorkThreadAIInfo;

enum MESSAGE_SERVICE_THREAD_STATE
{
    THREAD_RUN = 0,               //�߳���������
    THREAD_MODULE_UNLOAD,         //ģ�����أ���Ҫ�߳�֧�ִ˹���
    THREAD_STOP,                  //�߳�ֹͣ
};

//�����߳�ʱ��ģʽ
class CMessageService : public ACE_Task<ACE_MT_SYNCH>
{
public:
    CMessageService();
    ~CMessageService();

    virtual int handle_signal (int signum,
                               siginfo_t*   = 0,
                               ucontext_t* = 0);

    virtual int open(void* args = 0);
    virtual int svc (void);
    int Close();

    bool SaveThreadInfo();

    void Init(uint32 u4ThreadID, uint32 u4MaxQueue = MAX_MSG_THREADQUEUE, uint32 u4LowMask = MAX_MSG_MASK, uint32 u4HighMask = MAX_MSG_MASK);

    bool Start();

    bool PutMessage(CMessage* pMessage);
    bool PutUpdateCommandMessage(uint32 u4UpdateIndex);

    _ThreadInfo* GetThreadInfo();

    void GetAIInfo(_WorkThreadAIInfo& objAIInfo);           //�õ����й����̵߳�AI����
    void GetAITO(vecCommandTimeout& objTimeout);            //�õ����е�AI��ʱ���ݰ���Ϣ
    void GetAITF(vecCommandTimeout& objTimeout);            //�õ����е�AI������ݰ���Ϣ
    void SetAI(uint8 u1AI, uint32 u4DisposeTime, uint32 u4WTCheckTime, uint32 u4WTStopTime);  //����AI

    _CommandData* GetCommandData(uint16 u2CommandID);                      //�õ����������Ϣ
    _CommandFlowAccount GetCommandFlowAccount();                           //�õ����������Ϣ
    void GetCommandTimeOut(vecCommandTimeOut& CommandTimeOutList);         //�õ����г�ʱ����
    void GetCommandAlertData(vecCommandAlertData& CommandAlertDataList);   //�õ����г����澯��ֵ������
    void ClearCommandTimeOut();                                            //������еĳ�ʱ�澯
    void SaveCommandDataLog();                                             //�洢ͳ����־
    void SetThreadState(MESSAGE_SERVICE_THREAD_STATE emState);             //�����߳�״̬
    MESSAGE_SERVICE_THREAD_STATE GetThreadState();                         //�õ���ǰ�߳�״̬
    uint32 GetStepState();                                                 //�õ���ǰ���������Ϣ
    uint32 GetUsedMessageCount();                                          //�õ�����ʹ�õ�Message�������

    uint32 GetThreadID();

    void CopyMessageManagerList();                                         //��MessageManager�л�������б���

    CMessage* CreateMessage();
    void DeleteMessage(CMessage* pMessage);

private:
    bool ProcessMessage(CMessage* pMessage, uint32 u4ThreadID);
    bool SaveThreadInfoData();
    void CloseCommandList();                                                //����ǰ�����б���
    CClientCommandList* GetClientCommandList(uint16 u2CommandID);
    bool DoMessage(ACE_Time_Value& tvBegin, IMessage* pMessage, uint16& u2CommandID, uint32& u4TimeCost, uint16& u2Count, bool& bDeleteFlag);

    virtual int CloseMsgQueue();

private:
    uint64                         m_u8TimeCost;           //Put��������Ϣ�����ݴ���ʱ��
    uint32                         m_u4ThreadID;           //��ǰ�߳�ID
    uint32                         m_u4MaxQueue;           //�߳��������Ϣ�������
    uint32                         m_u4HighMask;
    uint32                         m_u4LowMask;
    uint32                         m_u4Count;              //��Ϣ���н��ܸ���
    uint32                         m_u4WorkQueuePutTime;   //��ӳ�ʱʱ��
    uint16                         m_u2ThreadTimeOut;
    uint16                         m_u2ThreadTimeCheck;
    bool                           m_blRun;                //�߳��Ƿ�������

    MESSAGE_SERVICE_THREAD_STATE   m_emThreadState;        //��ǰ�����߳�״̬

    _ThreadInfo                    m_ThreadInfo;           //��ǰ�߳���Ϣ
    CWorkThreadAI                  m_WorkThreadAI;         //�߳����Ҽ�ص�AI�߼�
    CCommandAccount                m_CommandAccount;       //��ǰ�߳�����ͳ������
    CMessagePool                   m_MessagePool;          //��Ϣ��

    CHashTable<CClientCommandList> m_objClientCommandList; //��ִ�е������б�

    ACE_Thread_Mutex m_mutex;
    ACE_Condition<ACE_Thread_Mutex> m_cond;
};

//add by freeeyes
//����̹߳����û����Դ������ɸ�ACE_Task��ÿ��Task��Ӧһ���̣߳�һ��Connectidֻ��Ӧһ���̡߳�
class CMessageServiceGroup : public ACE_Task<ACE_MT_SYNCH>
{
public:
    CMessageServiceGroup();
    ~CMessageServiceGroup();

    virtual int handle_timeout(const ACE_Time_Value& tv, const void* arg);

    bool Init(uint32 u4ThreadCount = MAX_MSG_THREADCOUNT, uint32 u4MaxQueue = MAX_MSG_THREADQUEUE, uint32 u4LowMask = MAX_MSG_MASK, uint32 u4HighMask = MAX_MSG_MASK);
    bool PutMessage(CMessage* pMessage);                                                     //���͵���Ӧ���߳�ȥ����
    bool PutUpdateCommandMessage(uint32 u4UpdateIndex);                                      //������Ϣͬ�����еĹ����߳������
    void Close();

    bool Start();
    CThreadInfo* GetThreadInfo();
    uint32 GetUsedMessageCount();

    uint32 GetWorkThreadCount();                                                              //�õ���ǰ�����̵߳�����
    uint32 GetWorkThreadIDByIndex(uint32 u4Index);                                            //�õ�ָ�������̵߳��߳�ID
    void GetWorkThreadAIInfo(vecWorkThreadAIInfo& objvecWorkThreadAIInfo);                    //�õ��̹߳���AI������Ϣ
    void GetAITO(vecCommandTimeout& objTimeout);                                              //�õ����е�AI��ʱ���ݰ���Ϣ
    void GetAITF(vecCommandTimeout& objTimeout);                                              //�õ����е�AI������ݰ���Ϣ
    void SetAI(uint8 u1AI, uint32 u4DisposeTime, uint32 u4WTCheckTime, uint32 u4WTStopTime);  //����AI

    void GetCommandData(uint16 u2CommandID, _CommandData& objCommandData);                    //���ָ������ͳ����Ϣ
    void GetFlowInfo(_CommandFlowAccount& objCommandFlowAccount);                             //���ָ�����������Ϣ

    void GetCommandTimeOut(vecCommandTimeOut& CommandTimeOutList);                            //�õ����г�ʱ����
    void GetCommandAlertData(vecCommandAlertData& CommandAlertDataList);                      //�õ����г����澯��ֵ������
    void ClearCommandTimeOut();                                                               //�������еĳ�ʱ�澯
    void SaveCommandDataLog();                                                                //�洢ͳ����־

    CMessage* CreateMessage(uint32 u4ConnectID, uint8 u1PacketType);                          //�����߳��л�ȡһ��Message����
    void DeleteMessage(uint32 u4ConnectID, CMessage* pMessage);                               //�����߳��л���һ��Message����

    void CopyMessageManagerList();                                                            //��MessageManager�л�������б���

    void AddDyringIP(const char* pClientIP, uint16 u2MaxCount);                               //Ⱦɫָ����IP
    bool AddDyeingCommand(uint16 u2CommandID, uint16 u2MaxCount);                             //Ⱦɫָ����CommandID
    void GetDyeingCommand(vec_Dyeing_Command_list& objList);                                  //��õ�ǰ����Ⱦɫ״̬

private:
    bool StartTimer();
    bool KillTimer();

    bool CheckWorkThread();                                                                  //������еĹ����߳�״̬
    bool CheckPacketParsePool();                                                             //�������ʹ�õ���Ϣ��������
    bool CheckCPUAndMemory();                                                                //���CPU���ڴ�
    bool CheckPlugInState();                                                                 //������в��״̬
    int32 GetWorkThreadID(uint32 u4ConnectID, uint8 u1PacketType);                           //���ݲ������ͺ�ConnectID������Ǹ������߳�ID

private:
    typedef vector<CMessageService*> vecMessageService;
    vecMessageService m_vecMessageService;

public:
    uint32                     m_u4MaxQueue;              //�߳��������Ϣ�������
    uint32                     m_u4HighMask;              //�̸߳�ˮλ
    uint32                     m_u4LowMask;               //�̵߳�ˮλ
    uint32                     m_u4TimerID;               //��ʱ��ID
    uint16                     m_u2ThreadTimeCheck;       //�߳��Լ�ʱ��
    CThreadInfo                m_objAllThreadInfo;        //��ǰ�����߳���Ϣ
    CRandomNumber              m_objRandomNumber;         //���������UDPʹ��
    CMessageDyeingManager      m_objMessageDyeingManager; //����Ⱦɫ��
};

typedef ACE_Singleton<CMessageServiceGroup,ACE_Null_Mutex> App_MessageServiceGroup;
#endif
