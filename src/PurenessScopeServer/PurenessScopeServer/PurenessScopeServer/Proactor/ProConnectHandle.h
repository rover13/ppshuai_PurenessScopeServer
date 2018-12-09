// ConnectHandle.h
// �����ͻ�������
// ���ò�˵��������һ�����Ŀ��飬���������������Ż���reactor�ܹ�����Ȼ������Ӧ����1024������������
// ����ACE�ܹ������⣬ʹ�����µ���ʶ����Ȼ�����ڵ��ǣ��滻�ܹ��Ĳ��֣�ֻ�з��ͺͽ��ܲ��֡��������ֶ����Ա�����
// �Ѿ��Ļ��������Ķ�������Ȼ���Լ��񶨣���һ����ʹ��Ĺ��̣���Ȼ����������ĸ��ã����Ǳ���Ĵ��ۡ�
// �����Լ��ܹ����ĸ��ã��������⣬������⼴�ɡ�
// ���Ͱɣ����������ġ�
// add by freeeyes
// 2009-08-23
//Bobo�����һ�������⣬���IOCP�£����ͺͽ��ղ��Եȣ�������ڴ����������������
//�������ܺã���������ǣ���������ˮλ�꣬�������ˮλ���೬����һ������ֵ����ô�ͶϿ����ӡ�
//linux�²�����������⣬ֻ��IOCP�³��֡�
//add by freeyes
//2013-09-18

#ifndef _PROCONNECTHANDLE_H
#define _PROCONNECTHANDLE_H

#include "ace/Svc_Handler.h"
#include "ace/Synch.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Asynch_IO.h"
#include "ace/Asynch_Acceptor.h"
#include "ace/Proactor.h"

#include "HashTable.h"
#include "AceProactorManager.h"
#include "IConnectManager.h"
#include "TimerManager.h"
#include "MakePacket.h"
#include "PacketParsePool.h"
#include "BuffPacketManager.h"
#include "Fast_Asynch_Read_Stream.h"
#include "ForbiddenIP.h"
#include "IPAccount.h"
#include "SendMessage.h"
#include "CommandAccount.h"
#include "SendCacheManager.h"
#include "LoadPacketParse.h"
#include "TimeWheelLink.h"
#include "FileTest.h"

#include <vector>

class CProConnectHandle : public ACE_Service_Handler
{
public:
    CProConnectHandle(void);
    ~CProConnectHandle(void);

    //��д�̳з���
    virtual void open(ACE_HANDLE h, ACE_Message_Block&);                                             //�û�����һ������
    virtual void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);                   //�������ܵ��û����ݰ���Ϣ�¼�
    virtual void handle_write_stream(const ACE_Asynch_Write_Stream::Result& result);                 //�������͵��û�������ɵ��¼�
    virtual void addresses(const ACE_INET_Addr& remote_address, const ACE_INET_Addr& local_address); //��õ�ǰԶ�̿ͻ��˵�IP��ַ��Ϣ

    uint32 file_open(IFileTestManager* pFileTest);                                           //�ļ���ڴ򿪽ӿ�
    int handle_write_file_stream(const char* pData, uint32 u4Size, uint8 u1ParseID);         //�ļ��ӿ�ģ�����ݰ����

    void Init(uint16 u2HandlerID);                                            //Connect Pool��ʼ�����õĺ���

    bool SendMessage(uint16 u2CommandID, IBuffPacket* pBuffPacket, uint8 u1State, uint8 u1SendType, uint32& u4PacketSize, bool blDelete, int nMessageID);   //���͸��ͻ������ݵĺ���
    void Close(int nIOCount = 1, int nErrno = 0);                                                  //��ǰ���Ӷ���ر�
    bool ServerClose(EM_Client_Close_status emStatus, uint8 u1OptionEvent = PACKET_SDISCONNECT);   //�������رտͻ������ӵĺ���
    void SetLocalIPInfo(const char* pLocalIP, uint32 u4LocalPort);            //���ü���IP�Ͷ˿���Ϣ

    uint32             GetHandlerID();                                        //�õ���ǰ��ʼ����HanddlerID
    const char*        GetError();                                            //�õ���ǰ���Ӵ�����Ϣ
    void               SetConnectID(uint32 u4ConnectID);                      //���õ�ǰ���ӵ�ID
    uint32             GetConnectID();                                        //��õ�ǰ���ӵ�ID
    uint8              GetConnectState();                                     //�õ�����״̬
    uint8              GetSendBuffState();                                    //�õ�����״̬
    _ClientConnectInfo GetClientInfo();                                       //�õ��ͻ�����Ϣ
    _ClientIPInfo      GetClientIPInfo();                                     //�õ��ͻ���IP��Ϣ
    _ClientIPInfo      GetLocalIPInfo();                                      //�õ�����IP��Ϣ
    void               SetConnectName(const char* pName);                     //���õ�ǰ��������
    char*              GetConnectName();                                      //�õ�����
    void               SetIsLog(bool blIsLog);                                //���õ�ǰ���������Ƿ�д����־
    bool               GetIsLog();                                            //�õ��Ƿ����д��־
    void               SetHashID(int nHashID);                                //����HashID
    int                GetHashID();                                           //�õ���ǰHashID

    void SetRecvQueueTimeCost(uint32 u4TimeCost);                             //��¼��ǰ�������ݵ�ģ�鴦����ɵľ���ʱ������
    void SetSendQueueTimeCost(uint32 u4TimeCost);                             //��¼��ǰ�ӷ��Ͷ��е����ݷ�����ɵľ���ʱ������
    void SetSendCacheManager(ISendCacheManager* pSendCacheManager);           //���÷��ͻ���ӿ�
    void SetPacketParseInfoID(uint32 u4PacketParseInfoID);                    //���ö�Ӧ��m_u4PacketParseInfoID
    uint32 GetPacketParseInfoID();                                            //�����Ӧ��m_u4PacketParseInfoID

private:
    bool RecvClinetPacket(uint32 u4PackeLen);                                 //�������ݰ�
    bool CheckMessage();                                                      //�������յ�����
    bool PutSendPacket(ACE_Message_Block* pMbData);                           //���������ݷ������
    void ClearPacketParse(ACE_Message_Block& mbCurrBlock);                    //��������ʹ�õ�PacketParse
    void PutSendPacketError(ACE_Message_Block* pMbData);                      //����ʧ�ܻص�

private:
    uint64             m_u8RecvQueueTimeCost;          //�ɹ��������ݵ����ݴ�����ɣ�δ���ͣ����ѵ�ʱ���ܺ�
    uint64             m_u8SendQueueTimeCost;          //�ɹ��������ݵ����ݴ�����ɣ�ֻ���ͣ����ѵ�ʱ���ܺ�
    uint32             m_u4MaxPacketSize;              //�������ݰ�����󳤶�
    uint32             m_u4RecvQueueCount;             //��ǰ���ӱ����������ݰ���
    uint32             m_u4HandlerID;                  //��Hander����ʱ��ID
    uint32             m_u4ConnectID;                  //��ǰConnect����ˮ��
    uint32             m_u4AllRecvCount;               //��ǰ���ӽ������ݰ��ĸ���
    uint32             m_u4AllSendCount;               //��ǰ���ӷ������ݰ��ĸ���
    uint32             m_u4AllRecvSize;                //��ǰ���ӽ����ֽ�����
    uint32             m_u4AllSendSize;                //��ǰ���ӷ����ֽ�����
    uint32             m_u4SendMaxBuffSize;            //����������󻺳峤��
    uint32             m_u4ReadSendSize;               //׼�����͵��ֽ�����ˮλ�꣩
    uint32             m_u4SuccessSendSize;            //ʵ�ʿͻ��˽��յ������ֽ�����ˮλ�꣩
    uint32             m_u4LocalPort;                  //���ؼ����˿�
    uint32             m_u4SendThresHold;              //���ͷ�ֵ(��Ϣ���ĸ���)
    uint32             m_u4SendCheckTime;              //���ͼ��ʱ��ķ�ֵ
    uint32             m_u4PacketParseInfoID;          //��Ӧ����packetParse��ģ��ID
    uint32             m_u4PacketDebugSize;            //��¼�ܴ���������ݰ�������ֽ�
    int                m_nHashID;                      //��ӦHash�б��е�ID
    int                m_u4RecvPacketCount;            //���ܰ��ĸ���
    int                m_nIOCount;                     //��ǰIO�����ĸ���
    uint16             m_u2SendQueueMax;               //���Ͷ�����󳤶�
    uint16             m_u2MaxConnectTime;             //�������ʱ���ж�
    uint16             m_u2SendQueueTimeout;           //���ͳ�ʱʱ��,�������ʱ��Ķ��ᱻ��¼����־��
    uint16             m_u2RecvQueueTimeout;           //���ܳ�ʱʱ�䣬�������ʱ��Ķ��ᱻ��¼����־��
    uint16             m_u2TcpNodelay;                 //Nagle�㷨����
    uint8              m_u1ConnectState;               //Ŀǰ���Ӵ���״̬
    uint8              m_u1SendBuffState;              //Ŀǰ�������Ƿ��еȴ����͵�����
    uint8              m_u1IsActive;                   //�����Ƿ�Ϊ����״̬��0Ϊ��1Ϊ��
    bool               m_blIsLog;                      //�Ƿ�д����־��falseΪ��д�룬trueΪд��

    ACE_INET_Addr      m_addrRemote;                   //Զ�����ӿͻ��˵�ַ
    ACE_Time_Value     m_atvConnect;                   //��ǰ���ӽ���ʱ��
    ACE_Time_Value     m_atvInput;                     //���һ�ν�������ʱ��
    ACE_Time_Value     m_atvOutput;                    //���һ�η�������ʱ��
    ACE_Time_Value     m_atvSendAlive;                 //���Ӵ��ʱ��
    CBuffPacket        m_AlivePacket;                  //�����������
    CPacketParse*      m_pPacketParse;                 //���ݰ�������
    char               m_szConnectName[MAX_BUFF_100];  //�������ƣ����Կ��Ÿ��߼����ȥ����
    char               m_szError[MAX_BUFF_500];        //������Ϣ��������
    char               m_szLocalIP[MAX_BUFF_50];       //���ؼ���IP


    ACE_Recursive_Thread_Mutex m_ThreadWriteLock;
    _TimeConnectInfo    m_TimeConnectInfo;              //���ӽ��������
    ACE_Message_Block*  m_pBlockMessage;                //��ǰ���ͻ���ȴ����ݿ�
    EM_Client_Close_status m_emStatus;                  //��ǰ�������رձ��

    CPacketParse        m_objSendPacketParse;           //�������ݰ���֯�ṹ
    char*               m_pPacketDebugData;             //��¼���ݰ���Debug�����ַ���

    EM_IO_TYPE          m_emIOType;                     //��ǰIO�������
    IFileTestManager*   m_pFileTest;                    //�ļ����Խӿ����

    Fast_Asynch_Read_Stream  m_Reader;
    Fast_Asynch_Write_Stream m_Writer;
};

//���������Ѿ�����������
class CProConnectManager : public ACE_Task<ACE_MT_SYNCH>
{
public:
    CProConnectManager(void);
    ~CProConnectManager(void);

    void Init(uint16 u2Index);

    static void TimeWheel_Timeout_Callback(void* pArgsContext, vector<CProConnectHandle*> vecProConnectHandle);

    virtual int open(void* args = 0);
    virtual int svc (void);
    virtual int close (u_long);
    virtual int handle_timeout(const ACE_Time_Value& tv, const void* arg);

    void CloseAll();                                                                                         //�ر�����������Ϣ
    bool AddConnect(uint32 u4ConnectID, CProConnectHandle* pConnectHandler);                                 //����һ���µ�������Ϣ
    bool SetConnectTimeWheel(CProConnectHandle* pConnectHandler);                                            //������Ϣ����
    bool DelConnectTimeWheel(CProConnectHandle* pConnectHandler);                                            //ɾ����Ϣ����
    bool SendMessage(uint32 u4ConnectID, IBuffPacket* pBuffPacket, uint16 u2CommandID, uint8 u1SendState, uint8 u1SendType, ACE_Time_Value& tvSendBegin, bool blDelete, int nMessageID);          //��������
    bool PostMessage(uint32 u4ConnectID, IBuffPacket* pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDelete = true, int nMessageID = 0);    //�첽����
    bool PostMessageAll(IBuffPacket* pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                        uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDelete = true, int nMessageID = 0);    //�첽Ⱥ��
    bool Close(uint32 u4ConnectID);                                                                          //�ͻ��˹ر�
    bool CloseConnect(uint32 u4ConnectID, EM_Client_Close_status emStatus);                                  //�������ر�
    bool CloseConnect_By_Queue(uint32 u4ConnectID);                                                          //�������ر�(�����ر��͵���Ϣ�����йر�)
    void GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo);                                         //���ص�ǰ������ӵ���Ϣ
    void SetRecvQueueTimeCost(uint32 u4ConnectID, uint32 u4TimeCost);                                        //��¼ָ���������ݴ���ʱ��

    bool SetConnectName(uint32 u4ConnectID, const char* pName);                                              //���õ�ǰ��������
    bool SetIsLog(uint32 u4ConnectID, bool blIsLog);                                                         //���õ�ǰ���������Ƿ�д����־
    void GetClientNameInfo(const char* pName, vecClientNameInfo& objClientNameInfo);                         //�õ�ָ������������������Ϣ
    _CommandData* GetCommandData(uint16 u2CommandID);                                                        //�õ����������Ϣ

    _ClientIPInfo GetClientIPInfo(uint32 u4ConnectID);                                                       //�õ�ָ��������Ϣ
    _ClientIPInfo GetLocalIPInfo(uint32 u4ConnectID);                                                        //�õ�����������Ϣ
    uint32 GetCommandFlowAccount();                                                                          //�õ�����������Ϣ
    EM_Client_Connect_status GetConnectState(uint32 u4ConnectID);                                            //�õ�ָ��������״̬
    CSendCacheManager* GetSendCacheManager();                                                                //�õ��ڴ�������

    bool StartTimer();
    bool KillTimer();

    int handle_write_file_stream(uint32 u4ConnectID, const char* pData, uint32 u4Size, uint8 u1ParseID);     //�ļ��ӿ�ģ�����ݰ����

    int         GetCount();
    const char* GetError();

private:
    virtual int CloseMsgQueue();

private:
    //�ر���Ϣ������������
    ACE_Thread_Mutex m_mutex;
    ACE_Condition<ACE_Thread_Mutex> m_cond;
private:
    uint32                             m_u4SendQueuePutTime;    //���Ͷ�����ӳ�ʱʱ��
    uint32                             m_u4TimeConnect;         //��λʱ�����ӽ�����
    uint32                             m_u4TimeDisConnect;      //��λʱ�����ӶϿ���
    uint32                             m_u4TimeCheckID;         //��ʱ������TimerID
    bool                               m_blRun;                 //�߳��Ƿ�������
    char                               m_szError[MAX_BUFF_500]; //������Ϣ����
    CHashTable<CProConnectHandle>      m_objHashConnectList;    //��¼��ǰ�Ѿ����ӵĽڵ㣬ʹ�ù̶��ڴ�ṹ
    ACE_Recursive_Thread_Mutex         m_ThreadWriteLock;       //����ѭ����غͶϿ�����ʱ���������
    ACE_Time_Value                     m_tvCheckConnect;        //��ʱ����һ�μ������ʱ��
    CSendMessagePool                   m_SendMessagePool;       //���Ͷ����
    CCommandAccount                    m_CommandAccount;        //��ǰ�߳�����ͳ������
    CSendCacheManager                  m_SendCacheManager;      //���ͻ����
    CTimeWheelLink<CProConnectHandle>  m_TimeWheelLink;         //����ʱ������
};

//����ConnectHandler�ڴ��
class CProConnectHandlerPool
{
public:
    CProConnectHandlerPool(void);
    ~CProConnectHandlerPool(void);

    void Init(int nObjcetCount);
    void Close();

    CProConnectHandle* Create();
    bool Delete(CProConnectHandle* pObject);

    int GetUsedCount();
    int GetFreeCount();

private:
    ACE_Recursive_Thread_Mutex    m_ThreadWriteLock;                     //���ƶ��߳���
    CHashTable<CProConnectHandle> m_objHashHandleList;                   //Hash������
    uint32                        m_u4CurrMaxCount;                      //��ǰ����Handler����
};

//����˼������ѷ��Ͷ�����ڼ����߳���ȥ����������ܡ������ﳢ��һ�¡�(���߳�ģʽ��һ���߳�һ�����У��������ֲ�������)
class CProConnectManagerGroup : public IConnectManager
{
public:
    CProConnectManagerGroup();
    ~CProConnectManagerGroup();

    void Init(uint16 u2SendQueueCount);
    void Close();

    bool AddConnect(CProConnectHandle* pConnectHandler);

    bool SetConnectTimeWheel(CProConnectHandle* pConnectHandler);                                            //������Ϣ����
    bool DelConnectTimeWheel(CProConnectHandle* pConnectHandler);                                            //ɾ����Ϣ����

    bool PostMessage(uint32 u4ConnectID, IBuffPacket*& pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDlete = true, int nMessageID = 0);    //�첽����
    bool PostMessage(uint32 u4ConnectID, const char*& pData, uint32 nDataLen, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDlete = true, int nMessageID = 0);    //�첽����
    bool PostMessage(vector<uint32> vecConnectID, IBuffPacket*& pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDlete = true, int nMessageID = 0);    //�첽Ⱥ��ָ����ID
    bool PostMessage(vector<uint32> vecConnectID, const char*& pData, uint32 nDataLen, uint8 u1SendType = SENDMESSAGE_NOMAL,
                     uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDlete = true, int nMessageID = 0);    //�첽Ⱥ��ָ����ID
    bool PostMessageAll(IBuffPacket*& pBuffPacket, uint8 u1SendType = SENDMESSAGE_NOMAL,
                        uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDlete = true, int nMessageID = 0);    //�첽Ⱥ��
    bool PostMessageAll(const char*& pData, uint32 nDataLen, uint8 u1SendType = SENDMESSAGE_NOMAL,
                        uint16 u2CommandID = 0, uint8 u1SendState = 0, bool blDlete = true, int nMessageID = 0);    //�첽Ⱥ��
    bool CloseConnect(uint32 u4ConnectID);                                                                   //�������ر�
    _ClientIPInfo GetClientIPInfo(uint32 u4ConnectID);                                                       //�õ�ָ��������Ϣ
    _ClientIPInfo GetLocalIPInfo(uint32 u4ConnectID);                                                        //�õ�����������Ϣ
    void GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo);                                         //���ص�ǰ������ӵ���Ϣ
    void SetRecvQueueTimeCost(uint32 u4ConnectID, uint32 u4TimeCost);                                        //��¼ָ���������ݴ���ʱ��
    bool SetConnectName(uint32 u4ConnectID, const char* pName);                                              //���õ�ǰ��������
    bool SetIsLog(uint32 u4ConnectID, bool blIsLog);                                                         //���õ�ǰ���������Ƿ�д����־
    void GetClientNameInfo(const char* pName, vecClientNameInfo& objClientNameInfo);                         //�õ�ָ������������������Ϣ

    int  GetCount();
    void CloseAll();
    bool Close(uint32 u4ConnectID);                                                                          //�ͻ����ر�

    bool StartTimer();                                                                                       //������ʱ��
    const char* GetError();                                                                                  //�õ�������Ϣ
    void GetCommandData(uint16 u2CommandID, _CommandData& objCommandData);                                   //���ָ������ͳ����Ϣ
    void GetCommandFlowAccount(_CommandFlowAccount& objCommandFlowAccount);                                  //�õ�����������Ϣ
    EM_Client_Connect_status GetConnectState(uint32 u4ConnectID);                                            //�õ�����״̬

    int handle_write_file_stream(uint32 u4ConnectID, const char* pData, uint32 u4Size, uint8 u1ParseID);     //�ļ��ӿ�ģ�����ݰ����

private:
    uint32 GetGroupIndex();                                                                                  //�õ���ǰ���ӵ�ID������

private:
    ACE_Recursive_Thread_Mutex  m_ThreadWriteLock;                                                           //���ƶ��߳���
    CProConnectManager** m_objProConnnectManagerList;                                                        //�������ӹ�����
    uint16            m_u2ThreadQueueCount;                                                                  //��ǰ�����̶߳��и���
    uint32            m_u4CurrMaxCount;                                                                      //��ǰ����������
};


typedef ACE_Singleton<CProConnectManagerGroup, ACE_Null_Mutex> App_ProConnectManager;
typedef ACE_Singleton<CProConnectHandlerPool, ACE_Null_Mutex> App_ProConnectHandlerPool;

#endif