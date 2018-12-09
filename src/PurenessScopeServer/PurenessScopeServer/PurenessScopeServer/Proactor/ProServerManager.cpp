#include "ProServerManager.h"
#include "Frame_Logging_Strategy.h"

CProServerManager::CProServerManager(void)
{
    m_pFrameLoggingStrategy = NULL;
}

CProServerManager::~CProServerManager(void)
{
}

bool CProServerManager::Init()
{
    if(App_MainConfig::instance()->GetDebugTrunOn() == 1)
    {
        m_pFrameLoggingStrategy = new Frame_Logging_Strategy();

        //�Ƿ��ACE_DEBUG�ļ��洢
        Logging_Config_Param objParam;

        sprintf_safe(objParam.m_strLogFile, 256, "%s", App_MainConfig::instance()->GetDebugFileName());
        objParam.m_iChkInterval    = App_MainConfig::instance()->GetChkInterval();
        objParam.m_iLogFileMaxCnt  = App_MainConfig::instance()->GetLogFileMaxCnt();
        objParam.m_iLogFileMaxSize = App_MainConfig::instance()->GetLogFileMaxSize();
        sprintf_safe(objParam.m_strLogLevel, 128, "%s", App_MainConfig::instance()->GetDebugLevel());

        m_pFrameLoggingStrategy->InitLogStrategy(objParam);
    }

    int nServerPortCount    = App_MainConfig::instance()->GetServerPortCount();
    //int nUDPServerPortCount = App_MainConfig::instance()->GetUDPServerPortCount();
    int nReactorCount       = App_MainConfig::instance()->GetReactorCount();

    bool blState = false;

    //��ʼ��ģ��������ز���
    App_MessageManager::instance()->Init(App_MainConfig::instance()->GetMaxModuleCount(), App_MainConfig::instance()->GetMaxCommandCount());

    //��ʼ������ģ�����Ϣ
    App_ModuleLoader::instance()->Init(App_MainConfig::instance()->GetMaxModuleCount());

    //��ʼ����ֹIP�б�
    App_ForbiddenIP::instance()->Init(FORBIDDENIP_FILE);

    //��ʼ����־ϵͳ�߳�
    CFileLogger* pFileLogger = new CFileLogger();

    if(NULL == pFileLogger)
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Init]pFileLogger new is NULL.\n"));
        return false;
    }

    pFileLogger->Init();
    AppLogManager::instance()->Init(1, MAX_MSG_THREADQUEUE, App_MainConfig::instance()->GetConnectAlert()->m_u4MailID);

    if(0 != AppLogManager::instance()->RegisterLog(pFileLogger))
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Init]AppLogManager::instance()->RegisterLog error.\n"));
        return false;
    }
    else
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Init]AppLogManager is OK.\n"));
    }

    //��ʼ��������ϵͳ
    App_IPAccount::instance()->Init(App_MainConfig::instance()->GetMaxHandlerCount());

    App_ConnectAccount::instance()->Init(App_MainConfig::instance()->GetConnectAlert()->m_u4ConnectMin,
                                         App_MainConfig::instance()->GetConnectAlert()->m_u4ConnectMax,
                                         App_MainConfig::instance()->GetConnectAlert()->m_u4DisConnectMin,
                                         App_MainConfig::instance()->GetConnectAlert()->m_u4DisConnectMax);

    //��ʼ��BuffPacket�����.Ĭ�϶��ǵ�ǰ�����������2��
    App_BuffPacketManager::instance()->Init(BUFFPACKET_MAX_COUNT, App_MainConfig::instance()->GetMsgMaxBuffSize(), App_MainConfig::instance()->GetByteOrder());

    //��ʼ�����������첽���ն���
    App_ServerMessageInfoPool::instance()->Init(App_MainConfig::instance()->GetServerConnectCount());

    //��ʼ��PacketParse�����
    App_PacketParsePool::instance()->Init(MAX_PACKET_PARSE);

    //��ʼ��ProConnectHandler�����
    if(App_MainConfig::instance()->GetMaxHandlerCount() <= 0)
    {
        App_ProConnectHandlerPool::instance()->Init(MAX_HANDLE_POOL);
    }
    else
    {
        App_ProConnectHandlerPool::instance()->Init(App_MainConfig::instance()->GetMaxHandlerCount());
    }

    //��ʼ�����ӹ�����
    App_ProConnectManager::instance()->Init(App_MainConfig::instance()->GetSendQueueCount());

    //��ʼ����DLL�Ķ���ӿ�
    App_ServerObject::instance()->SetMessageManager(dynamic_cast<IMessageManager*>(App_MessageManager::instance()));
    App_ServerObject::instance()->SetLogManager(dynamic_cast<ILogManager*>(AppLogManager::instance()));
    App_ServerObject::instance()->SetConnectManager(dynamic_cast<IConnectManager*>(App_ProConnectManager::instance()));
    App_ServerObject::instance()->SetPacketManager(dynamic_cast<IPacketManager*>(App_BuffPacketManager::instance()));
    App_ServerObject::instance()->SetClientManager(dynamic_cast<IClientManager*>(App_ClientProConnectManager::instance()));
    App_ServerObject::instance()->SetUDPConnectManager(dynamic_cast<IUDPConnectManager*>(App_ProUDPManager::instance()));
    App_ServerObject::instance()->SetTimerManager(reinterpret_cast<ActiveTimer*>(App_TimerManager::instance()));
    App_ServerObject::instance()->SetModuleMessageManager(dynamic_cast<IModuleMessageManager*>(App_ModuleMessageManager::instance()));
    App_ServerObject::instance()->SetControlListen(dynamic_cast<IControlListen*>(App_ProControlListen::instance()));
    App_ServerObject::instance()->SetModuleInfo(dynamic_cast<IModuleInfo*>(App_ModuleLoader::instance()));
    App_ServerObject::instance()->SetMessageBlockManager(dynamic_cast<IMessageBlockManager*>(App_MessageBlockManager::instance()));
    App_ServerObject::instance()->SetFrameCommand(dynamic_cast<IFrameCommand*>(&m_objFrameCommand));
    App_ServerObject::instance()->SetServerManager(this);

    //��ʼ����Ϣ�����߳�
    App_MessageServiceGroup::instance()->Init(App_MainConfig::instance()->GetThreadCount(),
            App_MainConfig::instance()->GetMsgMaxQueue(),
            App_MainConfig::instance()->GetMsgLowMark(),
            App_MainConfig::instance()->GetMgsHighMark());

    //��ʼ��ģ����أ���Ϊ������ܰ������м���������Ӽ���
    uint16 u2ModuleVCount = App_MainConfig::instance()->GetModuleInfoCount();

    for (uint16 i = 0; i < u2ModuleVCount; i++)
    {
        _ModuleConfig* pModuleConfig = App_MainConfig::instance()->GetModuleInfo(i);

        if (NULL != pModuleConfig)
        {
            bool blState = App_ModuleLoader::instance()->LoadModule(pModuleConfig->m_szModulePath,
                           pModuleConfig->m_szModuleName,
                           pModuleConfig->m_szModuleParam);

            if (false == blState)
            {
                OUR_DEBUG((LM_INFO, "[CProServerManager::Start]LoadModule (%s)is error.\n", pModuleConfig->m_szModuleName));
                return false;
            }
        }
    }

    //�����е��߳̿�ͬ������
    App_MessageServiceGroup::instance()->CopyMessageManagerList();

    //��ʼ��������
    uint32 u4ClientProactorCount = (uint32)nReactorCount - 3;

    if (!App_ProConnectAcceptManager::instance()->InitConnectAcceptor(nServerPortCount, u4ClientProactorCount))
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Init]%s.\n", App_ProConnectAcceptManager::instance()->GetError()));
        return false;
    }

    //��ʼ����Ӧ������
    App_ProactorManager::instance()->Init((uint16)nReactorCount);

    //��ʼ����Ӧ��
    for (int i = 0; i < nReactorCount; i++)
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Init()]... i=[%d].\n", i));

        if (App_MainConfig::instance()->GetNetworkMode() == NETWORKMODE_PRO_IOCP)
        {
            blState = App_ProactorManager::instance()->AddNewProactor(i, Proactor_WIN32, 1);
            OUR_DEBUG((LM_INFO, "[CProServerManager::Init]AddNewProactor NETWORKMODE = Proactor_WIN32.\n"));
        }
        else
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Init]AddNewProactor NETWORKMODE Error.\n"));
            return false;
        }

        if (!blState)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Init]AddNewProactor [%d] Error.\n", i));
            return false;
        }
    }

    return true;
}

bool CProServerManager::Start()
{
    //ע���ź���
    //if(0 != App_SigHandler::instance()->RegisterSignal(NULL))
    //{
    //  return false;
    //}

    //����TCP����
    int nServerPortCount = App_MainConfig::instance()->GetServerPortCount();
    //bool blState = false;

    //��ʼ������Զ������
    for(int i = 0 ; i < nServerPortCount; i++)
    {
        ACE_INET_Addr listenAddr;

        _ServerInfo* pServerInfo = App_MainConfig::instance()->GetServerPort(i);

        if(NULL == pServerInfo)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start]pServerInfo [%d] is NULL.\n", i));
            return false;
        }

        //�ж�IPv4����IPv6
        int nErr = 0;

        if(pServerInfo->m_u1IPType == TYPE_IPV4)
        {
            if(ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, pServerInfo->m_szServerIP);
            }
        }
        else
        {
            if(ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, pServerInfo->m_szServerIP, 1, PF_INET6);
            }
        }

        if(nErr != 0)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start](%d)set_address error[%d].\n", i, errno));
            return false;
        }

        //�õ�������
        ProConnectAcceptor* pConnectAcceptor = App_ProConnectAcceptManager::instance()->GetConnectAcceptor(i);

        if(NULL == pConnectAcceptor)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start]pConnectAcceptor[%d] is NULL.\n", i));
            return false;
        }

        //���ü���IP��Ϣ
        pConnectAcceptor->SetPacketParseInfoID(pServerInfo->m_u4PacketParseInfoID);
        pConnectAcceptor->SetListenInfo(pServerInfo->m_szServerIP, (uint32)pServerInfo->m_nPort);

        ACE_Proactor* pProactor = App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE);

        if(NULL == pProactor)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start]App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE) is NULL.\n"));
            return false;
        }

        int nRet = pConnectAcceptor->open(listenAddr, 0, 1, App_MainConfig::instance()->GetBacklog(), 1, pProactor);

        if(-1 == nRet)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start] pConnectAcceptor->open[%d] is error.\n", i));
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start] Listen from [%s:%d] error(%d).\n",listenAddr.get_host_addr(), listenAddr.get_port_number(), errno));
            return false;
        }

        OUR_DEBUG((LM_INFO, "[CProServerManager::Start] Listen from [%s:%d] OK.\n", listenAddr.get_host_addr(), listenAddr.get_port_number()));
    }

    //����UDP����
    int nUDPServerPortCount = App_MainConfig::instance()->GetUDPServerPortCount();

    for(int i = 0 ; i < nUDPServerPortCount; i++)
    {
        ACE_INET_Addr listenAddr;

        _ServerInfo* pServerInfo = App_MainConfig::instance()->GetUDPServerPort(i);

        if(NULL == pServerInfo)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start]UDP pServerInfo [%d] is NULL.\n", i));
            return false;
        }

        CProactorUDPHandler* pProactorUDPHandler = App_ProUDPManager::instance()->Create();

        if(NULL == pProactorUDPHandler)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start] pProactorUDPHandler is NULL[%d] is error.\n", i));
            return false;
        }
        else
        {
            pProactorUDPHandler->SetPacketParseInfoID(pServerInfo->m_u4PacketParseInfoID);
            int nErr = 0;

            if(pServerInfo->m_u1IPType == TYPE_IPV4)
            {
                if(ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
                {
                    nErr = listenAddr.set(pServerInfo->m_nPort, (uint32)INADDR_ANY);
                }
                else
                {
                    nErr = listenAddr.set(pServerInfo->m_nPort, pServerInfo->m_szServerIP);
                }
            }
            else
            {
                if(ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
                {
                    nErr = listenAddr.set(pServerInfo->m_nPort, (uint32)INADDR_ANY);
                }
                else
                {
                    nErr = listenAddr.set(pServerInfo->m_nPort, pServerInfo->m_szServerIP, 1, PF_INET6);
                }
            }

            if(nErr != 0)
            {
                OUR_DEBUG((LM_INFO, "[CProServerManager::Start](%d)UDP set_address error[%d].\n", i, errno));
                return false;
            }

            ACE_Proactor* pProactor = App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE);

            if(NULL == pProactor)
            {
                OUR_DEBUG((LM_INFO, "[CProServerManager::Start]UDP App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE) is NULL.\n"));
                return false;
            }

            if(0 != pProactorUDPHandler->OpenAddress(listenAddr, pProactor))
            {
                OUR_DEBUG((LM_INFO, "[CProServerManager::Start] UDP Listen from [%s:%d] error(%d).\n",listenAddr.get_host_addr(), listenAddr.get_port_number(), errno));
                return false;
            }

            OUR_DEBUG((LM_INFO, "[CProServerManager::Start] UDP Listen from [%s:%d] OK.\n", listenAddr.get_host_addr(), listenAddr.get_port_number()));
        }
    }

    //������̨����˿ڼ���
    if(App_MainConfig::instance()->GetConsoleSupport() == CONSOLE_ENABLE)
    {
        ACE_INET_Addr listenConsoleAddr;

        int nErr = 0;

        if(App_MainConfig::instance()->GetConsoleIPType() == TYPE_IPV4)
        {
            if(ACE_OS::strcmp(App_MainConfig::instance()->GetConsoleIP(), "INADDR_ANY") == 0)
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(),
                                             (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(),
                                             App_MainConfig::instance()->GetConsoleIP());
            }
        }
        else
        {
            if(ACE_OS::strcmp(App_MainConfig::instance()->GetConsoleIP(), "INADDR_ANY") == 0)
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(),
                                             (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(),
                                             App_MainConfig::instance()->GetConsoleIP(), 1, PF_INET6);
            }
        }

        if(nErr != 0)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start]listenConsoleAddr set_address error[%d].\n", errno));
            return false;
        }

        ACE_Proactor* pProactor = App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE);

        if(NULL == pProactor)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start]App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE) is NULL.\n"));
            return false;
        }

        int nRet = m_ProConsoleConnectAcceptor.open(listenConsoleAddr, 0, 1, MAX_ASYNCH_BACKLOG, 1, pProactor, true);

        if(-1 == nRet)
        {
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start] m_ProConsoleConnectAcceptor.open is error.\n"));
            OUR_DEBUG((LM_INFO, "[CProServerManager::Start] Listen from [%s:%d] error(%d).\n",listenConsoleAddr.get_host_addr(), listenConsoleAddr.get_port_number(), errno));
            return false;
        }
    }

    //������־�����߳�
    if(0 != AppLogManager::instance()->Start())
    {
        AppLogManager::instance()->WriteLog(LOG_SYSTEM, "[CProServerManager::Init]AppLogManager is ERROR.");
    }
    else
    {
        AppLogManager::instance()->WriteLog(LOG_SYSTEM, "[CProServerManager::Init]AppLogManager is OK.");
    }

    //������ʱ��
    if(0 != App_TimerManager::instance()->activate())
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Start]App_TimerManager::instance()->Start() is error.\n"));
        return false;
    }

    //������Ӧ��
    if(!App_ProactorManager::instance()->StartProactor())
    {
        OUR_DEBUG((LM_INFO, "[CProServerManager::Start]App_ProactorManager::instance()->StartProactor is error.\n"));
        return false;
    }

    //�����м���������ӹ�����
    App_ClientProConnectManager::instance()->Init(App_ProactorManager::instance()->GetAce_Proactor(REACTOR_POSTDEFINE));
    App_ClientProConnectManager::instance()->StartConnectTask(App_MainConfig::instance()->GetConnectServerCheck());

    //��ʼ��Ϣ�����߳�
    App_MessageServiceGroup::instance()->Start();

    if(App_MainConfig::instance()->GetConnectServerRunType() == 1)
    {
        //�����첽�������������Ϣ���Ĺ���
        App_ServerMessageTask::instance()->Start();
    }

    //��ʼ�������ӷ��Ͷ�ʱ��
    App_ProConnectManager::instance()->StartTimer();
    return true;
}

bool CProServerManager::Close()
{
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close begin....\n"));
    App_ProConnectAcceptManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ProConnectAcceptManager OK.\n"));

    m_ProConsoleConnectAcceptor.cancel();
    App_TimerManager::instance()->deactivate();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_TimerManager OK.\n"));

    App_ProUDPManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ProUDPManager OK.\n"));

    App_ClientProConnectManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ClientProConnectManager OK.\n"));

    App_ModuleLoader::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ModuleLoader OK.\n"));

    App_ServerMessageTask::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ServerMessageTask OK.\n"));

    App_MessageServiceGroup::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_MessageServiceGroup OK.\n"));

    App_ProConnectManager::instance()->CloseAll();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ProConnectManager OK.\n"));

    AppLogManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close AppLogManager OK\n"));

    App_MessageManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_MessageManager OK.\n"));

    App_BuffPacketManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_BuffPacketManager OK\n"));

    App_ProactorManager::instance()->StopProactor();
    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close App_ReactorManager OK.\n"));

    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]Close end....\n"));

    if(NULL != m_pFrameLoggingStrategy)
    {
        m_pFrameLoggingStrategy->EndLogStrategy();
        SAFE_DELETE(m_pFrameLoggingStrategy);
    }

    OUR_DEBUG((LM_INFO, "[CProServerManager::Close]EndLogStrategy end....\n"));

    return true;
}
