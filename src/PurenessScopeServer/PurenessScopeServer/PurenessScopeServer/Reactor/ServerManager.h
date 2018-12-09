// Define.h
// ���ﶨ��Server����Ҫ�ӿڣ����������˿ڵȵȣ�
// ��Ȼ�Եã���ʹʲô��������Ҳ��һ�����ˡ�
// add by freeeyes
// 2008-12-23


#ifndef _SERVERMANAGER_H
#define _SERVERMANAGER_H

#include "IServerManager.h"
#include "define.h"
#include "MainConfig.h"
#include "ForbiddenIP.h"
#include "ConnectAccept.h"
#include "ConsoleAccept.h"
#include "MessageService.h"
#include "LoadModule.h"
#include "LogManager.h"
#include "FileLogger.h"
#include "IObject.h"
#include "BuffPacketManager.h"
#include "ClientReConnectManager.h"
#include "ReUDPManager.h"
#include "CommandAccount.h"
#include "ModuleMessageManager.h"
#include "ControlListen.h"
#include "FrameCommand.h"

class Frame_Logging_Strategy;

class CServerManager : public IServerManager
{
public:
    CServerManager(void);
    ~CServerManager(void);

    bool Init();
    bool Start();
    bool Close();


private:
    bool Init_Reactor(uint8 u1ReactorCount, uint8 u1NetMode);
    bool Run();

private:
    ConnectConsoleAcceptor  m_ConnectConsoleAcceptor;    //��̨��������
    Frame_Logging_Strategy* m_pFrameLoggingStrategy;     //�������
    CFrameCommand           m_objFrameCommand;           //�������
};


typedef ACE_Singleton<CServerManager, ACE_Null_Mutex> App_ServerManager;

#endif
