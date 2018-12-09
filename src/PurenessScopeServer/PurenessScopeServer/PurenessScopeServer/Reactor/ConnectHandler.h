
// ConnectHandle.h
// ����ͻ�������
// �ܶ�ʱ�䣬�����������������ˣ������������������������ˡ�û��ʲô��ںý���
// ������2009��Ĵ����һ�����ټ������������˼ά����������������ϵĶ����������������ļ̳�������
// �����ң��������Щ�ѣ������Ҳ����ں�����Ϊ��֪��������ĵ�·�Ӳ�ƽ̹���������¿�ʼ����Ϊ���ܸе�������ҵ�������
// ��Ӷ��������������ݰ����Ĺܿء�
// add by freeeyes
// 2008-12-22

#ifndef _CONNECTHANDLE_H
#define _CONNECTHANDLE_H

#include "define.h"

#include "ace/Reactor.h"
#include "ace/Svc_Handler.h"
#include "ace/Synch.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Reactor_Notification_Strategy.h"

#include "HashTable.h"
#include "AceReactorManager.h"
#include "MessageService.h"
#include "IConnectManager.h"
#include "MakePacket.h"
#include "MessageBlockManager.h"
#include "PacketParsePool.h"
#include "BuffPacketManager.h"
#include "ForbiddenIP.h"
#include "IPAccount.h"
#include "TimerManager.h"
#include "SendMessage.h"
#include "CommandAccount.h"
#include "SendCacheManager.h"
#include "LoadPacketParse.h"
#include "TimeWheelLink.h"
#include "FileTest.h"

#ifdef __LINUX__
#include "netinet/tcp.h"
#endif

class CConnectHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
public:
    CConnectHandler(void);
    ~CConnectHandler(void);

    //��д�̳з���
    virtual int open(void*);                                                 //�û�����һ������
    virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);            //���ܿͻ����յ������ݿ�
    virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);           //���Ϳͻ�������
    virtual int handle_close(ACE_HANDLE h, ACE_Reactor_Mask mask);           //���ӹر��¼�

    uint32 file_open(IFileTestManager* pFileTest);                                           //�ļ���ڴ򿪽ӿ�
    int handle_write_file_stream(const char* pData, uint32 u4Size, uint8 u1ParseID);         //�ļ��ӿ�ģ�����ݰ����

    void Init(uint16 u2HandlerID);                                           //Connect Pool��ʼ������ʱ����õķ���
    void SetPacketParseInfoID(uint32 u4PacketParseInfoID);                   //���ö�Ӧ��m_u4PacketParseInfoID
    uint32 GetPacketParseInfoID();                                           //�����Ӧ��m_u4PacketParseInfoID


    bool CheckSendMask(uint32 u4PacketLen);                                  //���ָ�������ӷ��������Ƿ񳬹�������ֵ
    bool SendMessage(uint16 u2CommandID, IBuffPacket* pBuffPacket, uint8 u1State, uint8 u1SendType, uint32& u4PacketSize, bool blDelete, int nServerID);  //���͵�ǰ����
    bool SendCloseMessage();                                                 //�������ӹر���Ϣ

    void SetRecvQueueTimeCost(uint32 u4TimeCost);                            //��¼��ǰ�������ݵ�ģ�鴦����ɵľ���ʱ������
    void SetSendQueueTimeCost(uint32 u4TimeCost);                            //��¼��ǰ�ӷ��Ͷ��е����ݷ�����ɵľ���ʱ������
    void SetLocalIPInfo(const char* pLocalIP, uint32 u4LocalPort);           //���ü���IP�Ͷ˿���Ϣ

    void Close();                                                            //�رյ�ǰ����

    uint32      GetHandlerID();                                              //�õ���ǰ��handlerID
    const char* GetError();                                                  //�õ���ǰ������Ϣ
    void        SetConnectID(uint32 u4ConnectID);                            //���õ�ǰ����ID
    uint32      GetConnectID();                                              //�õ���ǰ����ID
    uint8       GetConnectState();                                           //�õ�����״̬
    uint8       GetSendBuffState();                                          //�õ�����״̬
    bool        CheckAlive(ACE_Time_Value& tvNow);                           //��ʱ�������Ӵ��״̬
    _ClientConnectInfo GetClientInfo();                                      //�õ��ͻ�����Ϣ
    _ClientIPInfo      GetClientIPInfo();                                    //�õ��ͻ���IP��Ϣ
    _ClientIPInfo      GetLocalIPInfo();                                     //�õ�����IP��Ϣ
    void SetConnectName(const char* pName);                                  //���õ�ǰ��������
    char* GetConnectName();                                                  //�õ�����
    void SetIsLog(bool blIsLog);                                             //���õ�ǰ���������Ƿ�д����־
    bool GetIsLog();                                                         //��õ�ǰ�����Ƿ����д����־
    void SetHashID(int nHashID);                                             //����Hash�����±�
    int  GetHashID();                                                        //�õ�Hash�����±�
    void SetSendCacheManager(CSendCacheManager* pSendCacheManager);

private:
    bool CheckMessage();                                                     //������յ�����
    bool PutSendPacket(ACE_Message_Block* pMbData);                          //��������
    void ClearPacketParse();                                                 //��������ʹ�õ�PacketParse

    int RecvData();                                                          //�������ݣ�����ģʽ
    int RecvData_et();                                                       //�������ݣ�etģʽ

    int Dispose_Recv_Data();                                                 //�����������

private:
    uint64                     m_u8RecvQueueTimeCost;          //�ɹ��������ݵ����ݴ�����ɣ�δ���ͣ����ѵ�ʱ���ܺ�
    uint64                     m_u8SendQueueTimeCost;          //�ɹ��������ݵ����ݴ�����ɣ�ֻ���ͣ����ѵ�ʱ���ܺ�
    uint64                     m_u8SendQueueTimeout;           //���ͳ�ʱʱ�䣬�������ʱ��Ķ��ᱻ��¼����־��
    uint64                     m_u8RecvQueueTimeout;           //���ܳ�ʱʱ�䣬�������ʱ��Ķ��ᱻ��¼����־��
    uint32                     m_u4HandlerID;                  //��Hander����ʱ��ID
    uint32                     m_u4ConnectID;                  //���ӵ�ID
    uint32                     m_u4AllRecvCount;               //��ǰ���ӽ������ݰ��ĸ���
    uint32                     m_u4AllSendCount;               //��ǰ���ӷ������ݰ��ĸ���
    uint32                     m_u4AllRecvSize;                //��ǰ���ӽ����ֽ�����
    uint32                     m_u4AllSendSize;                //��ǰ���ӷ����ֽ�����
    uint32                     m_u4MaxPacketSize;              //�������ݰ�����󳤶�
    uint32                     m_u4RecvQueueCount;             //��ǰ���ӱ���������ݰ���
    uint32                     m_u4SendMaxBuffSize;            //����������󻺳峤��
    uint32                     m_u4SendThresHold;              //���ͷ�ֵ(��Ϣ���ĸ���)
    uint32                     m_u4SendCheckTime;              //���ͼ��ʱ��ķ�ֵ
    uint32                     m_u4ReadSendSize;               //׼�����͵��ֽ�����ˮλ�꣩
    uint32                     m_u4SuccessSendSize;            //ʵ�ʿͻ��˽��յ������ֽ�����ˮλ�꣩
    uint32                     m_u4LocalPort;                  //�����Ķ˿ں�
    uint32                     m_u4PacketParseInfoID;          //��Ӧ����packetParse��ģ��ID
    uint32                     m_u4CurrSize;                   //��ǰMB�����ַ�����
    uint32                     m_u4PacketDebugSize;            //��¼�ܴ���������ݰ�������ֽ�
    int                        m_nBlockCount;                  //���������Ĵ���
    int                        m_nBlockMaxCount;               //������������������
    int                        m_nBlockSize;                   //��������ʱ�������Ĵ�С
    int                        m_nHashID;                      //��Ӧ��Pool��Hash�����±�
    uint16                     m_u2SendQueueMax;               //���Ͷ�����󳤶�
    uint16                     m_u2SendCount;                  //��ǰ���ݰ��ĸ���
    uint16                     m_u2MaxConnectTime;             //���ʱ�������ж�
    uint16                     m_u2TcpNodelay;                 //Nagle�㷨����
    uint8                      m_u1ConnectState;               //Ŀǰ���Ӵ���״̬
    uint8                      m_u1SendBuffState;              //Ŀǰ�������Ƿ��еȴ����͵�����
    uint8                      m_u1IsActive;                   //�����Ƿ�Ϊ����״̬��0Ϊ��1Ϊ��
    bool                       m_blBlockState;                 //�Ƿ�������״̬ falseΪ��������״̬��trueΪ������״̬
    bool                       m_blIsLog;                      //�Ƿ�д����־��falseΪ��д�룬trueΪд��
    char                       m_szError[MAX_BUFF_500];        //������Ϣ��������
    char                       m_szLocalIP[MAX_BUFF_50];       //������IP��ַ
    char                       m_szConnectName[MAX_BUFF_100];  //�������ƣ����Կ��Ÿ��߼����ȥ����
    ACE_INET_Addr              m_addrRemote;                   //Զ�����ӿͻ��˵�ַ
    ACE_Time_Value             m_atvConnect;                   //��ǰ���ӽ���ʱ��
    ACE_Time_Value             m_atvInput;                     //���һ�ν�������ʱ��
    ACE_Time_Value             m_atvOutput;                    //���һ�η�������ʱ��
    ACE_Time_Value             m_atvSendAlive;                 //���Ӵ��ʱ��
    EM_Client_Close_status     m_emStatus;                     //�������رձ��λ
    CBuffPacket                m_AlivePacket;                  //�����������
    CPacketParse*              m_pPacketParse;                 //���ݰ�������
    ACE_Message_Block*         m_pCurrMessage;                 //��ǰ��MB����
    ACE_Message_Block*         m_pBlockMessage;                //��ǰ���ͻ���ȴ����ݿ�
    CPacketParse               m_objSendPacketParse;           //�������ݰ���֯�ṹ
    _TimeConnectInfo           m_TimeConnectInfo;              //���ӽ��������
    char*                      m_pPacketDebugData;             //��¼���ݰ���Debug�����ַ���
    EM_IO_TYPE                 m_emIOType;                     //��ǰIO�������
    IFileTestManager*          m_pFileTest;                    //�ļ����Խӿ����
};

//���������Ѿ�����������
class CConnectManager : public ACE_Task<ACE_MT_SYNCH>
{
public:
    CConnectManager(void);
    ~CConnectManager(void);

    virtual int handle_timeout(const ACE_Time_Value& tv, const void* arg);   //��ʱ�����

    static void TimeWheel_Timeout_Callback(void* pArgsContext, vector<CConnectHandler*> vecConnectHandle);

    virtual int open(void* args = 0);
    virtual int svc(void);
    virtual int close(u_long);

    void Init(uint16 u2Index);

    void CloseAll();
    bool AddConnect(uint32 u4ConnectID, CConnectHandler* pConnectHandler);
    bool SetConnectTimeWheel(CConnectHandler* pConnectHandler);                                            //������Ϣ����
    bool DelConnectTimeWheel(CConnectHandler* pConnectHandler);                                            //ɾ����Ϣ����
    bool SendMessage(uint32 u4ConnectID, IBuffPacket* pBuffPacket,  uint16 u2CommandID, uint8 u1SendState, uint8 u1SendType, ACE_Time_Value& tvSendBegin, bool blDelete = true, int nServerID = 0);  //ͬ������                                                                     //���ͻ�������
    bool PostMessage(uint32 u4ConnectID, IBuffPacket* pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL, uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0); //�첽����
    bool PostMessageAll(IBuffPacket* pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL, uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);                  //�첽Ⱥ��
    bool Close(uint32 u4ConnectID);                                                                          //�ͻ����ر�
    bool CloseUnLock(uint32 u4ConnectID);                                                                    //�ر����ӣ��������汾
    bool CloseConnect(uint32 u4ConnectID);                                                                   //�������ر�
    bool CloseConnect_By_Queue(uint32 u4ConnectID);                                                          //�������ر�(�����ر��͵���Ϣ�����йر�)
    void GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo);                                         //���ص�ǰ������ӵ���Ϣ
    void SetRecvQueueTimeCost(uint32 u4ConnectID, uint32 u4TimeCost);                                        //��¼ָ���������ݴ���ʱ��
    void GetClientNameInfo(const char* pName, vecClientNameInfo& objClientNameInfo);                         //�õ�ָ������������������Ϣ

    _ClientIPInfo GetClientIPInfo(uint32 u4ConnectID);                                                       //�õ�ָ��������Ϣ
    _ClientIPInfo GetLocalIPInfo(uint32 u4ConnectID);                                                        //�õ�����������Ϣ

    bool StartTimer();                                                                                       //������ʱ��
    bool KillTimer();                                                                                        //�رն�ʱ��
    _CommandData* GetCommandData(uint16 u2CommandID);                                                        //�õ����������Ϣ
    uint32 GetCommandFlowAccount();                                                                          //�õ�����������Ϣ

    int         GetCount();
    const char* GetError();

    bool SetConnectName(uint32 u4ConnectID, const char* pName);                                              //���õ�ǰ��������
    bool SetIsLog(uint32 u4ConnectID, bool blIsLog);                                                         //���õ�ǰ���������Ƿ�д����־
    EM_Client_Connect_status GetConnectState(uint32 u4ConnectID);                                            //�õ�ָ��������״̬

    int handle_write_file_stream(uint32 u4ConnectID, const char* pData, uint32 u4Size, uint8 u1ParseID);     //�ļ��ӿ�ģ�����ݰ����

private:
    virtual int CloseMsgQueue();

private:
    //�ر���Ϣ������������
    ACE_Thread_Mutex m_mutex;
    ACE_Condition<ACE_Thread_Mutex> m_cond;

private:
    uint32                             m_u4TimeCheckID;         //��ʱ������TimerID
    uint32                             m_u4SendQueuePutTime;    //���Ͷ�����ӳ�ʱʱ��
    uint32                             m_u4TimeConnect;         //��λʱ�����ӽ�����
    uint32                             m_u4TimeDisConnect;      //��λʱ�����ӶϿ���
    bool                               m_blRun;                 //�߳��Ƿ�������
    char                               m_szError[MAX_BUFF_500]; //������Ϣ����
    CHashTable<CConnectHandler>        m_objHashConnectList;    //��¼��ǰ�Ѿ����ӵĽڵ㣬ʹ�ù̶��ڴ�ṹ
    ACE_Recursive_Thread_Mutex         m_ThreadWriteLock;       //����ѭ����غͶϿ�����ʱ���������
    _TimerCheckID*                     m_pTCTimeSendCheck;      //��ʱ���Ĳ����ṹ�壬����һ����ʱ��ִ�в�ͬ���¼�
    ACE_Time_Value                     m_tvCheckConnect;        //��ʱ����һ�μ������ʱ��
    CSendMessagePool                   m_SendMessagePool;       //������Ϣ��
    CCommandAccount                    m_CommandAccount;        //��ǰ�߳�����ͳ������
    CSendCacheManager                  m_SendCacheManager;      //���ͻ������
    CTimeWheelLink<CConnectHandler>    m_TimeWheelLink;         //����ʱ������
};

//����ConnectHandler�ڴ��
class CConnectHandlerPool
{
public:
    CConnectHandlerPool(void);
    ~CConnectHandlerPool(void);

    void Init(int nObjcetCount);
    void Close();

    CConnectHandler* Create();
    bool Delete(CConnectHandler* pObject);

    int GetUsedCount();
    int GetFreeCount();

private:
    uint32                      m_u4CurrMaxCount;                      //��ǰ����Handler����
    CHashTable<CConnectHandler> m_objHashHandleList;                   //Hash�����
    ACE_Recursive_Thread_Mutex  m_ThreadWriteLock;                     //���ƶ��߳���
};

//����˼������ѷ��Ͷ�����ڼ����߳���ȥ����������ܡ������ﳢ��һ�¡�(���߳�ģʽ��һ���߳�һ�����У��������ֲ�������)
class CConnectManagerGroup : public IConnectManager
{
public:
    CConnectManagerGroup();
    ~CConnectManagerGroup();

    void Init(uint16 u2SendQueueCount);
    void Close();

    bool AddConnect(CConnectHandler* pConnectHandler);
    bool SetConnectTimeWheel(CConnectHandler* pConnectHandler);                                            //������Ϣ����
    bool DelConnectTimeWheel(CConnectHandler* pConnectHandler);                                            //ɾ����Ϣ����
    bool PostMessage(uint32 u4ConnectID, IBuffPacket*& pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);            //�첽����
    bool PostMessage(uint32 u4ConnectID, const char*& pData, uint32 nDataLen, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);            //�첽����
    bool PostMessage(vector<uint32> vecConnectID, IBuffPacket*& pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);            //�첽Ⱥ��ָ����ID
    bool PostMessage(vector<uint32> vecConnectID, const char*& pData, uint32 nDataLen, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);   //�첽Ⱥ��ָ����ID
    bool PostMessageAll(IBuffPacket*& pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                        uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);
    bool PostMessageAll(const char*& pData, uint32 nDataLen, uint8 u1SendType = SENDMESSAGE_NOMAL,
                        uint16 u2CommandID = 0, uint8 u1SendState = true, bool blDelete = true, int nServerID = 0);
    bool CloseConnect(uint32 u4ConnectID);                                                                   //�������ر�
    bool CloseConnectByClient(uint32 u4ConnectID);                                                           //�ͻ��˹ر�
    _ClientIPInfo GetClientIPInfo(uint32 u4ConnectID);                                                       //�õ�ָ��������Ϣ
    _ClientIPInfo GetLocalIPInfo(uint32 u4ConnectID);                                                        //�õ�����������Ϣ
    void GetClientNameInfo(const char* pName, vecClientNameInfo& objClientNameInfo);                         //�õ�ָ������������������Ϣ
    void GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo);                                         //���ص�ǰ������ӵ���Ϣ
    void SetRecvQueueTimeCost(uint32 u4ConnectID, uint32 u4TimeCost);                                        //��¼ָ���������ݴ���ʱ��

    int  GetCount();
    void CloseAll();
    bool Close(uint32 u4ConnectID);                                                                          //�ͻ����ر�
    bool CloseUnLock(uint32 u4ConnectID);                                                                    //�ر����ӣ��������汾
    bool SetConnectName(uint32 u4ConnectID, const char* pName);                                              //���õ�ǰ��������
    bool SetIsLog(uint32 u4ConnectID, bool blIsLog);                                                         //���õ�ǰ���������Ƿ�д����־
    void GetCommandData(uint16 u2CommandID, _CommandData& objCommandData);                                   //���ָ������ͳ����Ϣ

    bool StartTimer();                                                                                       //������ʱ��
    const char* GetError();
    void GetCommandFlowAccount(_CommandFlowAccount& objCommandFlowAccount);                                  //�õ�����������Ϣ
    EM_Client_Connect_status GetConnectState(uint32 u4ConnectID);

    int handle_write_file_stream(uint32 u4ConnectID, const char* pData, uint32 u4Size, uint8 u1ParseID);     //�ļ��ӿ�ģ�����ݰ����

private:
    uint32 GetGroupIndex();                                                                                  //�õ���ǰ���ӵ�ID������
private:
    uint32            m_u4CurrMaxCount;                                                                      //��ǰ����������
    uint16            m_u2ThreadQueueCount;                                                                  //��ǰ�����̶߳��и���
    ACE_Recursive_Thread_Mutex  m_ThreadWriteLock;                                                           //���ƶ��߳���
    CConnectManager** m_objConnnectManagerList;                                                              //�������ӹ�����
};

typedef ACE_Singleton<CConnectManagerGroup, ACE_Recursive_Thread_Mutex> App_ConnectManager;
typedef ACE_Singleton<CConnectHandlerPool, ACE_Null_Mutex> App_ConnectHandlerPool;

#endif
