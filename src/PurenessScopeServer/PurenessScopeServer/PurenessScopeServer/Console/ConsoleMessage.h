#ifndef _CONSOLEMESSAGE_H
#define _CONSOLEMESSAGE_H

#include <ace/OS_NS_sys_resource.h>

#include "define.h"
#include "IBuffPacket.h"
#include "LoadModule.h"
#include "MessageManager.h"
#include "MessageService.h"
#include "MakePacket.h"
#include "ForbiddenIP.h"
#include "ace/Message_Block.h"
#include "IPAccount.h"
#include "IObject.h"
#include "FileTestManager.h"
#include "ConsolePromiss.h"

#ifdef WIN32
#include "ProConnectHandle.h"
#include "ClientProConnectManager.h"
#include "ProUDPManager.h"
#include "WindowsCPU.h"
#include "ProControlListen.h"
#else
#include "ConnectHandler.h"
#include "ClientReConnectManager.h"
#include "ReUDPManager.h"
#include "LinuxCPU.h"
#include "ControlListen.h"
#endif

//�������ֵ���Ͷ���
enum
{
    CONSOLE_MESSAGE_SUCCESS = 0,
    CONSOLE_MESSAGE_FAIL    = 1,
    CONSOLE_MESSAGE_CLOSE   = 2,
};

#define COMMAND_SPLIT_STRING " "

//���ö�Ӧ�����������ƣ����ڽ��ն˰�����
#define CONSOLE_COMMAND_UNKNOW             0x1000
#define CONSOLE_COMMAND_LOADMOUDLE         0x1001
#define CONSOLE_COMMAND_UNLOADMOUDLE       0x1002
#define CONSOLE_COMMAND_RELOADMOUDLE       0x1003
#define CONSOLE_COMMAND_SHOWMOUDLE         0x1004
#define CONSOLE_COMMAND_CLIENTCOUNT        0x1005
#define CONSOLE_COMMAND_COMMANDINFO        0x1005
#define CONSOLE_COMMAND_COMMANDTIMEOUT     0x1007
#define CONSOLE_COMMAND_COMMANDTIMEOUTCLR  0x1008
#define CONSOLE_COMMAND_COMMANDDATALOG     0x1009
#define CONSOLE_COMMAND_THREADINFO         0x100A
#define CONSOLE_COMMAND_CLIENTINFO         0x100B
#define CONSOLE_COMMAND_UDPCONNECTINFO     0x100C
#define CONSOLE_COMMAND_COLSECLIENT        0x100D
#define CONSOLE_COMMAND_FORBIDDENIP        0x100E
#define CONSOLE_COMMAND_FORBIDDENIPSHOW    0x100F
#define CONSOLE_COMMAND_LIFTED             0x1010
#define CONSOLE_COMMAND_SERVERCONNECT_TCP  0x1011
#define CONSOLE_COMMAND_SERVERCONNECT_UDP  0x1012
#define CONSOLE_COMMAND_PROCESSINFO        0x1013
#define CONSOLE_COMMAND_CLIENTHISTORY      0x1014
#define CONSOLE_COMMAND_ALLCOMMANDINFO     0x1015
#define CONSOLE_COMMAND_SERVERINFO         0x1016
#define CONSOLE_COMMAND_SERVERRECONNECT    0x1017
#define CONSOLE_COMMAND_SETDEBUG           0x1018
#define CONSOLE_COMMAND_SHOWDEBUG          0x1019
#define CONSOLE_COMMAND_SETTRACKIP         0x101A
#define CONSOLE_COMMAND_SETTRACECOMMAND    0x101B
#define CONSOLE_COMMAND_GETTRACKCOMMAND    0x101C
#define CONSOLE_COMMAND_GETCONNECTIPINFO   0x101D
#define CONSOLE_COMMAND_GETLOGINFO         0x101E
#define CONSOLE_COMMAND_SETLOGLEVEL        0x101F
#define CONSOLE_COMMAND_GETWTAI            0x1020
#define CONSOLE_COMMAND_GETWTTIMEOUT       0x1021
#define CONSOLE_COMMAND_SETWTAI            0x1022
#define CONSOLE_COMMAND_GETNICKNAMEINFO    0x1023
#define CONSOLE_COMMAND_SETCONNECTLOG      0x1024
#define CONSOLE_COMMAND_SETMAXCONNECTCOUNT 0x1025
#define CONSOLE_COMMAND_ADD_LISTEN         0x1026
#define CONSOLE_COMMAND_DEL_LISTEN         0x1027
#define CONSOLE_COMMAND_SHOW_LISTEN        0x1028
#define CONSOLE_COMMAND_MONITOR_INFO       0x1029
#define CONSOLE_COMMAND_CLOSE_SERVER       0x1030
#define CONSOLE_COMMAND_FILE_TEST_START    0x1031
#define CONSOLE_COMMAND_FILE_TEST_STOP     0x1032

//Ŀǰ֧�ֵ�����
#define CONSOLEMESSAHE_LOADMOUDLE         "LoadModule"          //����ģ��
#define CONSOLEMESSAHE_UNLOADMOUDLE       "UnLoadModule"        //ж��ģ��
#define CONSOLEMESSAHE_RELOADMOUDLE       "ReLoadModule"        //���¼���ģ��
#define CONSOLEMESSAHE_SHOWMOUDLE         "ShowModule"          //��ʾ���������Ѿ����ص�ģ��
#define CONSOLEMESSAHE_CLIENTCOUNT        "ClientCount"         //��ǰ�ͻ���������
#define CONSOLEMESSAHE_COMMANDINFO        "CommandInfo"         //��ǰĳһ�������״̬��Ϣ
#define CONSOLEMESSAHE_COMMANDTIMEOUT     "CommandTimeout"      //���г�ʱ�������б�
#define CONSOLEMESSAHE_COMMANDTIMEOUTCLR  "CommandTimeoutclr"   //�����ʱ�������б�
#define CONSOLEMESSAGE_COMMANDDATALOG     "CommandDataLog"      //�洢CommandDataLog
#define CONSOLEMESSAHE_THREADINFO         "WorkThreadState"     //��ǰ�����̺߳͹����߳�״̬
#define CONSOLEMESSAHE_CLIENTINFO         "ConnectClient"       //��ǰ�ͻ������ӵ���Ϣ
#define CONSOLEMESSAHE_UDPCONNECTINFO     "UDPConnectClient"    //��ǰUDP�ͻ��˵�������Ϣ
#define CONSOLEMESSAHE_COLSECLIENT        "CloseClient"         //�رտͻ���
#define CONSOLEMESSAHE_FORBIDDENIP        "ForbiddenIP"         //��ֹIP����
#define CONSOLEMESSAHE_FORBIDDENIPSHOW    "ShowForbiddenIP"     //�鿴��ֹ����IP�б�
#define CONSOLEMESSAHE_LIFTED             "LiftedIP"            //���ĳIP
#define CONSOLEMESSAHE_SERVERCONNECT_TCP  "ServerConnectTCP"    //��������ͨѶ(TCP)
#define CONSOLEMESSAHE_SERVERCONNECT_UDP  "ServerConnectUDP"    //��������ͨѶ(UDP)
#define CONSOLEMESSAGE_PROCESSINFO        "ShowCurrProcessInfo" //�鿴��ǰ������������״̬ShowServerInfo
#define CONSOLEMESSAGE_CLIENTHISTORY      "ShowConnectHistory"  //�鿴��������ʷ����״̬
#define CONSOLEMESSAGE_ALLCOMMANDINFO     "ShowAllCommand"      //�鿴����������ע��ģ��������Ϣ
#define CONSOLEMESSAGE_SERVERINFO         "ShowServerInfo"      //�鿴������������Ϣ
#define CONSOLEMESSAGE_SERVERRECONNECT    "ReConnectServer"     //Զ�˿�������ĳһ��Զ�˷�����
#define CONSOLEMESSAGE_SETDEBUG           "SetDebug"            //���õ�ǰDEBUG״̬
#define CONSOLEMESSAGE_SHOWDEBUG          "ShowDebug"           //�鿴��ǰDEBUG״̬
#define CONSOLEMESSAGE_SETTRACKIP         "SetTrackIP"          //����ҪȾɫ��IP
#define CONSOLEMESSAGE_SETTRACECOMMAND    "SetTrackCommand"     //����ҪȾɫ��CommandID
#define CONSOLEMESSAGE_GETTRACKIPINFO     "GetTrackCommandInfo" //�õ�Ⱦɫ��CommandID��Ϣ
#define CONSOLEMESSAGE_GETCONNECTIPINFO   "GetConnectIPInfo"    //ͨ��ConnectID�����ص�IP��Ϣ
#define CONSOLEMESSAGE_GETLOGINFO         "GetLogInfo"          //�õ���־�ȼ�
#define CONSOLEMESSAGE_SETLOGLEVEL        "SetLogLevel"         //������־�ȼ�
#define CONSOLEMESSAGE_GETWTAI            "GetWorkThreadAI"     //�õ�Thread��AI������Ϣ
#define CONSOLEMESSAGE_GETWTTIMEOUT       "GetWorkThreadTO"     //�õ�Thread�����г�ʱ���ݰ���Ϣ
#define CONSOLEMESSAGE_SETWTAI            "SetWorkThreadAI"     //����ThreadAI��������Ϣ
#define CONSOLEMESSAGE_GETNICKNAMEINFO    "GetNickNameInfo"     //�õ�������Ϣ
#define CONSOLEMESSAGE_SETCONNECTLOG      "SetConnectLog"       //����������־����״̬ 
#define CONSOLEMESSAGE_SETMAXCONNECTCOUNT "SetMaxConnectCount"  //�������������
#define CONSOLEMESSAGE_ADD_LISTEN         "AddListen"           //���һ���µļ����˿�
#define CONSOLEMESSAGE_DEL_LISTEN         "DelListen"           //ɾ��һ���µļ����˿�
#define CONSOLEMESSATE_SHOW_LISTEN        "ShowListen"          //�鿴���ڴ򿪵ļ����˿� 
#define CONSOLEMESSATE_MONITOR_INFO       "Monitor"             //���������в����ӿ�
#define CONSOLEMESSATE_SERVER_CLOSE       "ServerClose"         //�رյ�ǰ������
#define CONSOLEMESSATE_FILE_TEST_START    "TestFileStart"       //�����������ļ���������
#define CONSOLEMESSATE_FILE_TEST_STOP     "TestFileStop"        //ֹͣ�������ļ���������

//��������
struct _CommandInfo
{
    uint8 m_u1OutputType;                 //������ͣ�0Ϊ�����ƣ�1Ϊ�ı�
    char m_szCommandTitle[MAX_BUFF_100];  //��������ͷ
    char m_szCommandExp[MAX_BUFF_100];    //����������չ����
    char m_szUser[MAX_BUFF_50];           //�û���Ϣ

    _CommandInfo()
    {
        m_u1OutputType      = 0;
        m_szCommandTitle[0] = '\0';
        m_szCommandExp[0]   = '\0';
        m_szUser[0]         = '\0';
    }
};

//�ļ����ṹ
struct _FileInfo
{
    char m_szFilePath[MAX_BUFF_100];
    char m_szFileName[MAX_BUFF_100];
    char m_szFileParam[MAX_BUFF_200];

    _FileInfo()
    {
        m_szFilePath[0]  = '\0';
        m_szFileName[0]  = '\0';
        m_szFileParam[0] = '\0';
    }
};

//�����˿���Ϣ
struct _ListenInfo
{
    uint32 m_u4Port;
    uint32 m_u4PacketParseID;
    uint8  m_u1IPType;
    char   m_szListenIP[MAX_BUFF_20];

    _ListenInfo()
    {
        m_szListenIP[0]   = '\0';
        m_u4Port          = 0;
        m_u1IPType        = TYPE_IPV4;
        m_u4PacketParseID = 0;
    }
};

//ȾɫIP��Ϣ
struct _DyeIPInfo
{
    char   m_szClientIP[MAX_BUFF_20];   //Ⱦɫ�ͻ���IP
    uint16 m_u2MaxCount;                //�������
};

//Ⱦɫ��CommandID
struct _DyeCommandInfo
{
    uint16 m_u2CommandID;               //Ⱦɫ�ͻ�������
    uint16 m_u2MaxCount;                //�������
};

class CConsoleMessage
{
public:
    CConsoleMessage();
    ~CConsoleMessage();

    int Dispose(ACE_Message_Block* pmb, IBuffPacket* pBuffPacket, uint8& u1OutputType);     //Ҫ����������ֽ���, pBuffPacketΪ����Ҫ���͸��ͻ��˵�����
    int ParsePlugInCommand(const char* pCommand, IBuffPacket* pBuffPacket);                 //ִ������

    //��ʼ������
    bool SetConsoleKey(vecConsoleKey* pvecConsoleKey);       //�����֤�����keyֵ

    //�������ݲ���
private:
    int  ParseCommand_Plugin(const char* pCommand, IBuffPacket* pBuffPacket, uint8& u1OutputType);            //ִ������(����ڲ�����)
    int  ParseCommand(const char* pCommand, IBuffPacket* pBuffPacket, uint8& u1OutputType);                   //ִ������
    int  DoCommand(_CommandInfo& CommandInfo, IBuffPacket* pCurrBuffPacket, IBuffPacket* pReturnBuffPacket);  //������������
    bool GetCommandInfo(const char* pCommand, _CommandInfo& CommandInfo);                                     //�������и��Ӧ���е����ݸ�ʽ
    bool GetFileInfo(const char* pFile, _FileInfo& FileInfo);                                                 //��һ��ȫ·���зֳ��ļ���
    bool GetForbiddenIP(const char* pCommand, _ForbiddenIP& ForbiddenIP);                                     //�õ���ֹ��IP�б�
    bool GetConnectServerID(const char* pCommand, int& nServerID);                                            //�õ�һ��ָ���ķ�����ID
    bool GetDebug(const char* pCommand, uint8& u1Debug);                                                      //�õ���ǰ���õ�BUDEG
    bool CheckConsoleKey(const char* pKey);                                                                   //��֤key
    bool GetTrackIP(const char* pCommand, _ForbiddenIP& ForbiddenIP);                                         //�õ����õ�׷��IP
    bool GetLogLevel(const char* pCommand, int& nLogLevel);                                                   //�õ���־�ȼ�
    bool GetAIInfo(const char* pCommand, int& nAI, int& nDispose, int& nCheck, int& nStop);                   //�õ�AI����
    bool GetNickName(const char* pCommand, char* pName);                                                      //�õ����ӱ���
    bool GetConnectID(const char* pCommand, uint32& u4ConnectID, bool& blFlag);                               //�õ�ConnectID
    bool GetMaxConnectCount(const char* pCommand, uint16& u2MaxConnectCount);                                 //�õ�������������
    bool GetListenInfo(const char* pCommand, _ListenInfo& objListenInfo);                                     //�õ������˿���Ϣ
    bool GetTestFileName(const char* pCommand, char* pFileName);                                              //��ü��ز����ļ���
    bool GetDyeingIP(const char* pCommand, _DyeIPInfo& objDyeIPInfo);                                         //���ȾɫIP�������Ϣ
    bool GetDyeingCommand(const char* pCommand, _DyeCommandInfo& objDyeCommandInfo);                          //���ȾɫCommand�������Ϣ

    //�������ʵ�ֲ���
private:
    void DoMessage_LoadModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_UnLoadModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ReLoadModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ClientMessageCount(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_CommandInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_WorkThreadState(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ClientInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_CloseClient(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ForbiddenIP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowForbiddenList(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_LifedIP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_UDPClientInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ServerConnectTCP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ServerConnectUDP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowProcessInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowClientHisTory(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowAllCommandInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowServerInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ReConnectServer(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_CommandTimeout(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_CommandTimeoutclr(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_CommandDataLog(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetDebug(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowDebug(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetTrackIP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetTraceCommand(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_GetTrackCommand(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_GetConnectIPInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_GetLogLevelInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetLogLevelInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_GetThreadAI(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_GetWorkThreadTO(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetWorkThreadAI(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_GetNickNameInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetConnectLog(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_SetMaxConnectCount(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_AddListen(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_DelListen(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ShowListen(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_MonitorInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_ServerClose(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_TestFileStart(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);
    void DoMessage_TestFileStop(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID);

private:
    vecConsoleKey*      m_pvecConsoleKey;
    CConsolePromissions m_objConsolePromissions;
};

typedef ACE_Singleton<CConsoleMessage, ACE_Null_Mutex> App_ConsoleManager;
#endif
