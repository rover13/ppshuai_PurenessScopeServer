#ifndef _MAINCONFIG_H
#define _MAINCONFIG_H

#include "define.h"
#include "XmlOpeation.h"

#include "ace/Singleton.h"
#include <vector>

#include "PacketParse.h"

//PacketParse�����Ϣ
struct _PacketParseInfo
{
    uint32 m_u4PacketID;
    uint32 m_u4OrgLength;
    uint8  m_u1Type;
    char   m_szPacketParsePath[MAX_BUFF_200];
    char   m_szPacketParseName[MAX_BUFF_100];

    _PacketParseInfo()
    {
        m_u4PacketID           = 0;
        m_szPacketParsePath[0] = '\0';
        m_szPacketParseName[0] = '\0';
        m_u1Type               = (uint8)PACKET_WITHHEAD;
        m_u4OrgLength          = 0;
    }
};

//��������Ϣ
//���Ӷ�IPv4��IPv6��֧��
struct _ServerInfo
{
    uint32 m_u4PacketParseInfoID;
    int32    m_nPort;
    uint8  m_u1IPType;
    char   m_szServerIP[MAX_BUFF_50];

    _ServerInfo()
    {
        m_szServerIP[0]       = '\0';
        m_nPort               = 0;
        m_u1IPType            = TYPE_IPV4;
        m_u4PacketParseInfoID = 0;
    }
};

//����������������Ϣ
struct _ModuleConfig
{
    char m_szModuleName[MAX_BUFF_100];
    char m_szModulePath[MAX_BUFF_200];
    char m_szModuleParam[MAX_BUFF_200];

    _ModuleConfig()
    {
        m_szModuleName[0]  = '\0';
        m_szModulePath[0]  = '\0';
        m_szModuleParam[0] = '\0';
    }
};

typedef vector<_ModuleConfig> vecModuleConfig;

//Զ�̹�����֧��
//��¼����Զ��ά���ӿڽ��������key�����ݡ�
struct _ConsoleKey
{
    char m_szKey[MAX_BUFF_100];

    _ConsoleKey()
    {
        m_szKey[0] = '\0';
    }
};

typedef vector<_ConsoleKey> vecConsoleKey;

//Console�����̨����IP��Ϣ
struct _ConsoleClientIP
{
    char m_szServerIP[MAX_BUFF_20];

    _ConsoleClientIP()
    {
        m_szServerIP[0] =  '\0';
    }
};

//���Ӹ澯��ֵ�������
struct _ConnectAlert
{
    uint32 m_u4ConnectMin;
    uint32 m_u4ConnectMax;
    uint32 m_u4DisConnectMin;
    uint32 m_u4DisConnectMax;
    uint32 m_u4ConnectAlert;
    uint32 m_u4MailID;

    _ConnectAlert()
    {
        m_u4ConnectMin    = 0;
        m_u4ConnectMax    = 0;
        m_u4DisConnectMin = 0;
        m_u4DisConnectMax = 0;
        m_u4ConnectAlert  = 0;
        m_u4MailID        = 0;
    }
};

//����IP�澯��ֵ�������
struct _IPAlert
{
    uint32 m_u4IPMaxCount;
    uint32 m_u4IPTimeout;
    uint32 m_u4MailID;

    _IPAlert()
    {
        m_u4IPMaxCount = 0;
        m_u4IPTimeout  = 0;
        m_u4MailID     = 0;
    }
};

//�����Ӹ澯��ֵ�������
struct _ClientDataAlert
{
    uint32 m_u4RecvPacketCount;
    uint32 m_u4RecvDataMax;
    uint32 m_u4SendPacketCount;
    uint32 m_u4SendDataMax;
    uint32 m_u4MailID;

    _ClientDataAlert()
    {
        m_u4RecvPacketCount = 0;
        m_u4RecvDataMax     = 0;
        m_u4SendPacketCount = 0;
        m_u4SendDataMax     = 0;
        m_u4MailID          = 0;
    }
};

//�����и澯��ֵ����
struct _CommandAlert
{
    uint32 m_u4CommandCount;
    uint32 m_u4MailID;
    uint16 m_u2CommandID;

    _CommandAlert()
    {
        m_u2CommandID    = 0;
        m_u4CommandCount = 0;
        m_u4MailID       = 0;
    }
};
typedef vector<_CommandAlert> vecCommandAlert;

//�ʼ����������Ϣ
struct _MailAlert
{
    uint32 m_u4MailPort;
    uint32 m_u4MailID;
    char   m_szFromMailAddr[MAX_BUFF_200];
    char   m_szToMailAddr[MAX_BUFF_200];
    char   m_szMailPass[MAX_BUFF_200];
    char   m_szMailUrl[MAX_BUFF_200];

    _MailAlert()
    {
        m_u4MailID          = 0;
        m_szFromMailAddr[0] = '\0';
        m_szToMailAddr[0]   = '\0';
        m_szMailPass[0]     = '\0';
        m_szMailUrl[0]      = '\0';
        m_u4MailPort        = 0;
    }
};
typedef vector<_MailAlert> vecMailAlert;

//��Ⱥ���������Ϣ
struct _GroupListenInfo
{
    uint32 m_u4GroupPort;
    uint8  m_u1GroupNeed;
    uint8  m_u1IPType;
    char   m_szGroupIP[MAX_BUFF_50];

    _GroupListenInfo()
    {
        m_u1GroupNeed  = 0;
        m_szGroupIP[0] = '\0';
        m_u4GroupPort  = 0;
        m_u1IPType     = TYPE_IPV4;
    }
};

enum ENUM_CHAR_ORDER
{
    SYSTEM_LITTLE_ORDER = 0,   //С������
    SYSTEM_BIG_ORDER,          //�������
};

class CMainConfig
{
public:
    CMainConfig(void);
    ~CMainConfig(void);

    bool Init();
    bool Init_Alert(const char* szConfigPath);
    bool Init_Main(const char* szConfigPath);
    void Display();
    const char* GetError();

    const char* GetServerName();
    const char* GetServerVersion();
    const char* GetPacketVersion();
    const char* GetWindowsServiceName();
    const char* GetDisplayServiceName();
    uint16 GetServerID();
    uint16 GetServerPortCount();
    _ServerInfo* GetServerPort(int32 nIndex);

    uint32 GetMgsHighMark();
    uint32 GetMsgLowMark();
    uint32 GetMsgMaxBuffSize();
    uint32 GetThreadCount();
    uint8  GetProcessCount();
    uint32 GetMsgMaxQueue();
    uint16 GetHandleCount();

    int32 GetEncryptFlag();
    const char* GetEncryptPass();
    int32 GetEncryptOutFlag();

    uint32 GetSendTimeout();

    uint32 GetRecvBuffSize();
    uint16 GetSendQueueMax();
    uint16 GetThreadTimuOut();
    uint16 GetThreadTimeCheck();
    uint16 GetPacketTimeOut();
    uint16 GetCheckAliveTime();
    uint16 GetMaxHandlerCount();
    void   SetMaxHandlerCount(uint16 u2MaxHandlerCount);
    uint16 GetMaxConnectTime();
    uint8  GetConsoleSupport();
    int32    GetConsolePort();
    uint8  GetConsoleIPType();
    const char* GetConsoleIP();
    vecConsoleKey* GetConsoleKey();
    uint16 GetRecvQueueTimeout();
    uint16 GetSendQueueTimeout();
    uint16 GetSendQueueCount();

    bool CompareConsoleClinetIP(const char* pConsoleClientIP);

    _ServerInfo* GetUDPServerPort(int32 nIndex);

    uint16 GetUDPServerPortCount();
    uint32 GetReactorCount();
    uint8  GetCommandAccount();
    uint32 GetConnectServerTimeout();
    uint16 GetConnectServerCheck();
    uint8  GetConnectServerRunType();
    uint16 GetSendQueuePutTime();
    uint16 GetWorkQueuePutTime();
    uint8  GetServerType();
    uint8  GetDebug();
    uint32 GetDebugSize();
    void   SetDebug(uint8 u1Debug);
    uint8  GetNetworkMode();
    uint32 GetConnectServerRecvBuffer();
    uint8  GetMonitor();
    uint32 GetServerRecvBuff();
    uint8  GetCommandFlow();
    uint32 GetSendDataMask();
    uint32 GetCoreFileSize();
    uint16 GetTcpNodelay();
    uint16 GetBacklog();
    uint16 GetTrackIPCount();
    ENUM_CHAR_ORDER GetCharOrder();
    double GetCpuMax();
    uint64 GetMemoryMax();
    uint8  GetWTAI();
    uint32 GetWTCheckTime();
    uint32 GetWTTimeoutCount();
    uint32 GetWTStopTime();
    uint8  GetWTReturnDataType();
    char*  GetWTReturnData();
    bool   GetByteOrder();
    uint8  GetDebugTrunOn();
    char*  GetDebugFileName();
    uint32 GetChkInterval();
    uint32 GetLogFileMaxSize();
    uint32 GetLogFileMaxCnt();
    char*  GetDebugLevel();
    uint32 GetBlockSize();
    uint32 GetBlockCount();
    uint8  GetServerClose();
    uint32 GetMaxCommandCount();
    uint32 GetServerConnectCount();
    uint16 GetMaxModuleCount();

    uint16 GetModuleInfoCount();
    _ModuleConfig* GetModuleInfo(uint16 u2Index);

    _ConnectAlert*    GetConnectAlert();
    _IPAlert*         GetIPAlert();
    _ClientDataAlert* GetClientDataAlert();
    uint32            GetCommandAlertCount();
    _CommandAlert*    GetCommandAlert(int32 nIndex);
    _MailAlert*       GetMailAlert(uint32 u4MailID);
    _GroupListenInfo* GetGroupListenInfo();
    _PacketParseInfo* GetPacketParseInfo(uint8 u1Index = 0);
    uint8             GetPacketParseCount();

private:
    uint32     m_u4MsgHighMark;                        //��Ϣ�ĸ�ˮλ��ֵ
    uint32     m_u4MsgLowMark;                         //��Ϣ�ĵ�ˮλ��ֵ
    uint32     m_u4MsgMaxBuffSize;                     //��Ϣ������С
    uint32     m_u4MsgThreadCount;                     //����Ĺ����̸߳���
    uint32     m_u4MsgMaxQueue;                        //��Ϣ���е�������
    uint32     m_u4DebugSize;                          //���õ�ǰ��¼���ݰ����ȵ���󻺳��С
    uint32     m_u4SendTimeout;                        //���ͳ�ʱʱ��
    uint32     m_u4RecvBuffSize;                       //�������ݻ���صĴ�С
    uint32     m_u4ServerConnectCount;                 //�����������ӻ���������
    uint32     m_u4MaxCommandCount;                    //��ǰ��������������
    uint32     m_u4ReactorCount;                       //ϵͳ�������ķ�Ӧ���ĸ���
    uint32     m_u4ConnectServerTimerout;              //����Զ�̷��������ʱ��
    uint32     m_u4ConnectServerRecvBuff;              //������������ݰ����ջ����С
    uint32     m_u4ServerRecvBuff;                     //���մӿͻ��˵�������ݿ������С��ֻ��PacketPrase��ģʽ�Ż���Ч
    uint32     m_u4SendDatamark;                       //���Ͳ�ֵ��ˮλ�꣨Ŀǰֻ��Proactorģʽ�������
    uint32     m_u4BlockSize;                          //���ͻ�����С����
    uint32     m_u4CoreFileSize;                       //Core�ļ��ĳߴ��С
    uint32     m_u4TrackIPCount;                       //���IP�������ʷ��¼��
    uint32     m_u4SendBlockCount;                     //��ʼ�����ͻ������
    double     m_d8MaxCpu;                             //���CPU����߷�ֵ
    uint64     m_u4MaxMemory;                          //����ڴ�ķ�ֵ
    uint32     m_u4WTCheckTime;                        //�����̳߳�ʱ����ʱ�䷶Χ����λ����
    uint32     m_u4WTTimeoutCount;                     //�����̳߳�ʱ���ĵ�λʱ���ڵĳ�ʱ��������
    uint32     m_u4WTStopTime;                         //ֹͣ����������ʱ��
    uint32     m_u4ChkInterval;                        //����ļ�ʱ��
    uint32     m_u4LogFileMaxSize;                     //����ļ����ߴ�
    uint32     m_u4LogFileMaxCnt;                      //����ļ������������ﵽ�������Զ�ѭ��
    int32      m_nServerID;                            //������ID
    int32      m_nEncryptFlag;                         //0�����ܷ�ʽ�رգ�1Ϊ���ܷ�ʽ����
    int32      m_nEncryptOutFlag;                      //��Ӧ���ݰ���0��Ϊ�����ܣ�1Ϊ����
    int32      m_nConsolePort;                         //Console�������Ķ˿�
    uint16     m_u2SendQueueMax;                       //���Ͷ�����������ݰ�����
    uint16     m_u2ThreadTimuOut;                      //�̳߳�ʱʱ���ж�
    uint16     m_u2ThreadTimeCheck;                    //�߳��Լ�ʱ��
    uint16     m_u2PacketTimeOut;                      //�������ݳ�ʱʱ��
    uint16     m_u2SendAliveTime;                      //���ʹ�����ʱ��
    uint16     m_u2HandleCount;                        //handle����صĸ���
    uint16     m_u2MaxHanderCount;                     //���ͬʱ����Handler������
    uint16     m_u2MaxConnectTime;                     //��ȴ���������ʱ�䣨��ʱ���ڣ�������պͷ��Ͷ�û�з��������ɷ������ر�������ӣ�
    uint16     m_u2RecvQueueTimeout;                   //���ն��д���ʱʱ���޶�
    uint16     m_u2SendQueueTimeout;                   //���Ͷ��д���ʱʱ���޶�
    uint16     m_u2SendQueueCount;                     //��ܷ����߳���
    uint16     m_u2SendQueuePutTime;                   //���÷��Ͷ��е���ӳ�ʱʱ��
    uint16     m_u2WorkQueuePutTime;                   //���ù������е���ӳ�ʱʱ��
    uint16     m_u2MaxModuleCount;                     //��ǰ�����������ģ������
    uint16     m_u2ConnectServerCheck;                 //�����������ӵ�λ���ʱ��
    uint16     m_u2TcpNodelay;                         //TCP��Nagle�㷨���أ�0Ϊ�򿪣�1Ϊ�ر�
    uint16     m_u2Backlog;                            //���õ�Backlogֵ
    bool       m_blByteOrder;                          //��ǰ���ʹ������falseΪ������trueΪ������
    uint8      m_u1MsgProcessCount;                    //��ǰ�Ķ��������(��Linux֧��)
    uint8      m_u1Debug;                              //�Ƿ���Debugģʽ��1�ǿ�����0�ǹر�
    uint8      m_u1ServerClose;                        //�������Ƿ�����Զ�̹ر�
    uint8      m_u1CommandAccount;                     //�Ƿ���Ҫͳ������������������Ϣ��0�ǹرգ�1�Ǵ򿪡��򿪺��������Ӧ�ı���
    uint8      m_u1ServerType;                         //���÷���������״̬
    uint8      m_u1ConsoleSupport;                     //�Ƿ�֧��Console���������1����֧�֣�0�ǲ�֧��
    uint8      m_u1ConsoleIPType;                      //Console��IPType
    uint8      m_u1CommandFlow;                        //�������ͳ�ƣ�0Ϊ��ͳ�ƣ�1Ϊͳ��
    uint8      m_u1ConnectServerRunType;               //�������䷵�ذ�����ģʽ��0Ϊͬ����1Ϊ�첽
    uint8      m_u1NetworkMode;                        //��ǰ�������õ�����ģʽ
    uint8      m_u1Monitor;                            //���õ�ǰ�ļ�ؿ����Ƿ�򿪣�0�ǹرգ�1�Ǵ�
    uint8      m_u1WTAI;                               //�����߳�AI���أ�0Ϊ�رգ�1Ϊ��
    uint8      m_u1WTReturnDataType;                   //���ش������ݵ����ͣ�1Ϊ�����ƣ�2Ϊ�ı�
    uint8      m_u1DebugTrunOn;                        //ACE_DEBUG�ļ�������أ�0Ϊ�رգ�1Ϊ��


    char       m_szError[MAX_BUFF_500];
    char       m_szServerName[MAX_BUFF_20];            //����������
    char       m_szServerVersion[MAX_BUFF_20];         //�������汾
    char       m_szWindowsServiceName[MAX_BUFF_50];    //windows��������
    char       m_szDisplayServiceName[MAX_BUFF_50];    //windows������ʾ����
    char       m_szPacketVersion[MAX_BUFF_20];         //���ݽ�����ģ��İ汾��
    char       m_szEncryptPass[MAX_BUFF_9];            //�8λ�ļ������룬3DES�㷨
    char       m_szConsoleIP[MAX_BUFF_100];            //Console������IP
    char       m_szWTReturnData[MAX_BUFF_1024];        //���ص������壬���1K
    char       m_szDeubgFileName[MAX_BUFF_100];        //����ļ���
    char       m_szDebugLevel[MAX_BUFF_100];           //����ļ�����

    CXmlOpeation     m_MainConfig;
    _ConnectAlert    m_ConnectAlert;                   //���Ӹ澯���������Ϣ
    _IPAlert         m_IPAlert;                        //IP�澯��ֵ�������
    _ClientDataAlert m_ClientDataAlert;                //�����ӿͻ��˸澯��ֵ�������
    _GroupListenInfo m_GroupListenInfo;                //��Ⱥ��ط�������ַ����

    typedef vector<_PacketParseInfo> vecPacketParseInfo;
    vecPacketParseInfo m_vecPacketParseInfo;

    ENUM_CHAR_ORDER m_u1CharOrder;                 //��ǰ�ֽ���

    typedef vector<_ServerInfo> vecServerInfo;
    vecServerInfo m_vecServerInfo;
    vecServerInfo m_vecUDPServerInfo;

    typedef vector<_ConsoleClientIP> vecConsoleClientIP;
    vecConsoleClientIP m_vecConsoleClientIP;                  //��������̨�����IP
    vecConsoleKey      m_vecConsoleKey;                       //�����������keyֵ
    vecCommandAlert    m_vecCommandAlert;                     //�����и澯��ֵ�������
    vecMailAlert       m_vecMailAlert;                        //�����ʼ��������
    vecModuleConfig    m_vecModuleConfig;                     //����ģ����������Ϣ
};

typedef ACE_Singleton<CMainConfig, ACE_Null_Mutex> App_MainConfig;

#endif
