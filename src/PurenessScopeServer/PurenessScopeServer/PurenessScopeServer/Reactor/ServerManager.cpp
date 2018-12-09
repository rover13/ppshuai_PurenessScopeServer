#include "ServerManager.h"

#include "Frame_Logging_Strategy.h"

CServerManager::CServerManager(void)
{
    m_pFrameLoggingStrategy = NULL;
}

CServerManager::~CServerManager(void)
{
    OUR_DEBUG((LM_INFO, "[CServerManager::~CServerManager].\n"));
}

bool CServerManager::Init()
{
    if (App_MainConfig::instance()->GetDebugTrunOn() == 1)
    {
        m_pFrameLoggingStrategy = new Frame_Logging_Strategy();

        //�Ƿ��ACE_DEBUG�ļ��洢
        Logging_Config_Param objParam;

        sprintf_safe(objParam.m_strLogFile, 256, "%s", App_MainConfig::instance()->GetDebugFileName());
        objParam.m_iChkInterval = App_MainConfig::instance()->GetChkInterval();
        objParam.m_iLogFileMaxCnt = App_MainConfig::instance()->GetLogFileMaxCnt();
        objParam.m_iLogFileMaxSize = App_MainConfig::instance()->GetLogFileMaxSize();
        sprintf_safe(objParam.m_strLogLevel, 128, "%s", App_MainConfig::instance()->GetDebugLevel());

        m_pFrameLoggingStrategy->InitLogStrategy(objParam);
    }

    int nServerPortCount = App_MainConfig::instance()->GetServerPortCount();
    int nReactorCount = App_MainConfig::instance()->GetReactorCount();

    //��ʼ��ģ��������ز���
    App_MessageManager::instance()->Init(App_MainConfig::instance()->GetMaxModuleCount(), App_MainConfig::instance()->GetMaxCommandCount());

    //��ʼ������ģ�����Ϣ
    App_ModuleLoader::instance()->Init(App_MainConfig::instance()->GetMaxModuleCount());

    //��ʼ����ֹIP�б�
    App_ForbiddenIP::instance()->Init(FORBIDDENIP_FILE);

    OUR_DEBUG((LM_INFO, "[CServerManager::Init]nReactorCount=%d.\n", nReactorCount));

    //Ϊ�������׼�������epoll��epollet��ʼ������������ȥ��,��Ϊ�ڶ������epoll_create�������ӽ�����ȥ����
    if (NETWORKMODE_RE_EPOLL != App_MainConfig::instance()->GetNetworkMode() && NETWORKMODE_RE_EPOLL_ET != App_MainConfig::instance()->GetNetworkMode())
    {
        //��ʼ����Ӧ������
        App_ReactorManager::instance()->Init((uint16)nReactorCount);

        OUR_DEBUG((LM_INFO, "[CServerManager::Init]****1*******.\n"));
        Init_Reactor((uint8)nReactorCount, App_MainConfig::instance()->GetNetworkMode());
    }

    //��ʼ����־ϵͳ�߳�
    CFileLogger* pFileLogger = new CFileLogger();

    if (NULL == pFileLogger)
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Init]pFileLogger new is NULL.\n"));
        return false;
    }

    pFileLogger->Init();
    AppLogManager::instance()->Init(1, MAX_MSG_THREADQUEUE, App_MainConfig::instance()->GetConnectAlert()->m_u4MailID);

    if (0 != AppLogManager::instance()->RegisterLog(pFileLogger))
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Init]AppLogManager::instance()->RegisterLog error.\n"));
        return false;
    }
    else
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Init]AppLogManager is OK.\n"));
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

    //��ʼ��ConnectHandler�����
    if (App_MainConfig::instance()->GetMaxHandlerCount() <= 0)
    {
        App_ConnectHandlerPool::instance()->Init(MAX_HANDLE_POOL);
    }
    else
    {
        App_ConnectHandlerPool::instance()->Init(App_MainConfig::instance()->GetMaxHandlerCount());
    }

    //��ʼ�����ӹ�����
    App_ConnectManager::instance()->Init(App_MainConfig::instance()->GetSendQueueCount());

    //��ʼ����DLL�Ķ���ӿ�
    App_ServerObject::instance()->SetMessageManager(dynamic_cast<IMessageManager*>(App_MessageManager::instance()));
    App_ServerObject::instance()->SetLogManager(dynamic_cast<ILogManager*>(AppLogManager::instance()));
    App_ServerObject::instance()->SetConnectManager(dynamic_cast<IConnectManager*>(App_ConnectManager::instance()));
    App_ServerObject::instance()->SetPacketManager(dynamic_cast<IPacketManager*>(App_BuffPacketManager::instance()));
    App_ServerObject::instance()->SetClientManager(dynamic_cast<IClientManager*>(App_ClientReConnectManager::instance()));
    App_ServerObject::instance()->SetUDPConnectManager(dynamic_cast<IUDPConnectManager*>(App_ReUDPManager::instance()));
    App_ServerObject::instance()->SetTimerManager(reinterpret_cast<ActiveTimer*>(App_TimerManager::instance()));
    App_ServerObject::instance()->SetModuleMessageManager(dynamic_cast<IModuleMessageManager*>(App_ModuleMessageManager::instance()));
    App_ServerObject::instance()->SetControlListen(dynamic_cast<IControlListen*>(App_ControlListen::instance()));
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
                OUR_DEBUG((LM_INFO, "[CServerManager::Run]LoadModule (%s)is error.\n", pModuleConfig->m_szModuleName));
                return false;
            }
        }
    }

    //�����е��߳̿�ͬ������
    App_MessageServiceGroup::instance()->CopyMessageManagerList();

    //��ʼ��������
    uint32 u4ClientReactorCount = (uint32)nReactorCount - 3;

    if (!App_ConnectAcceptorManager::instance()->InitConnectAcceptor(nServerPortCount, u4ClientReactorCount))
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Init]%s.\n", App_ConnectAcceptorManager::instance()->GetError()));
        return false;
    }

    return true;
}

bool CServerManager::Start()
{
    //����TCP������ʼ��
    int nServerPortCount = App_MainConfig::instance()->GetServerPortCount();

    for (int i = 0; i < nServerPortCount; i++)
    {
        ACE_INET_Addr listenAddr;
        _ServerInfo* pServerInfo = App_MainConfig::instance()->GetServerPort(i);

        if (NULL == pServerInfo)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start]pServerInfo [%d] is NULL.\n", i));
            return false;
        }

        //�ж�IPv4����IPv6
        int nErr = 0;

        if (pServerInfo->m_u1IPType == TYPE_IPV4)
        {
            if (ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
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
            if (ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, pServerInfo->m_szServerIP, 1, PF_INET6);
            }

        }

        if (nErr != 0)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start](%d)set_address error[%d].\n", i, errno));
            return false;
        }

        //�õ�������
        ConnectAcceptor* pConnectAcceptor = App_ConnectAcceptorManager::instance()->GetConnectAcceptor(i);

        if (NULL == pConnectAcceptor)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start]pConnectAcceptor[%d] is NULL.\n", i));
            return false;
        }

        pConnectAcceptor->SetPacketParseInfoID(pServerInfo->m_u4PacketParseInfoID);
        int nRet = pConnectAcceptor->Init_Open(listenAddr, 0, 1, 1, (int)App_MainConfig::instance()->GetBacklog());

        if (-1 == nRet)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start] pConnectAcceptor->open[%d] is error.\n", i));
            OUR_DEBUG((LM_INFO, "[CServerManager::Start] Listen from [%s:%d] error(%d).\n", listenAddr.get_host_addr(), listenAddr.get_port_number(), errno));
            return false;
        }

        OUR_DEBUG((LM_INFO, "[CServerManager::Start] Listen from [%s:%d] OK.\n", listenAddr.get_host_addr(), listenAddr.get_port_number()));
    }

    //����UDP����
    int nUDPServerPortCount = App_MainConfig::instance()->GetUDPServerPortCount();

    for (int i = 0; i < nUDPServerPortCount; i++)
    {
        ACE_INET_Addr listenAddr;
        _ServerInfo* pServerInfo = App_MainConfig::instance()->GetUDPServerPort(i);

        if (NULL == pServerInfo)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start]UDP pServerInfo [%d] is NULL.\n", i));
            return false;
        }

        int nErr = 0;

        if (pServerInfo->m_u1IPType == TYPE_IPV4)
        {
            if (ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
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
            if (ACE_OS::strcmp(pServerInfo->m_szServerIP, "INADDR_ANY") == 0)
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenAddr.set(pServerInfo->m_nPort, pServerInfo->m_szServerIP, 1, PF_INET6);
            }
        }

        if (nErr != 0)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start]UDP (%d)set_address error[%d].\n", i, errno));
            return false;
        }

        //�õ�������
        CReactorUDPHander* pReactorUDPHandler = App_ReUDPManager::instance()->Create();

        if (NULL == pReactorUDPHandler)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start]UDP pReactorUDPHandler[%d] is NULL.\n", i));
            return false;
        }

        pReactorUDPHandler->SetPacketParseInfoID(pServerInfo->m_u4PacketParseInfoID);
        int nRet = pReactorUDPHandler->OpenAddress(listenAddr);

        if (-1 == nRet)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start] UDP Listen from [%s:%d] error(%d).\n", listenAddr.get_host_addr(), listenAddr.get_port_number(), errno));
            return false;
        }

        OUR_DEBUG((LM_INFO, "[CServerManager::Start] UDP Listen from [%s:%d] OK.\n", listenAddr.get_host_addr(), listenAddr.get_port_number()));
    }

    //������̨����˿ڼ���
    if (App_MainConfig::instance()->GetConsoleSupport() == CONSOLE_ENABLE)
    {
        ACE_INET_Addr listenConsoleAddr;
        int nErr = 0;

        if (App_MainConfig::instance()->GetConsoleIPType() == TYPE_IPV4)
        {
            if (ACE_OS::strcmp(App_MainConfig::instance()->GetConsoleIP(), "INADDR_ANY") == 0)
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(), (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(), App_MainConfig::instance()->GetConsoleIP());
            }
        }
        else
        {
            if (ACE_OS::strcmp(App_MainConfig::instance()->GetConsoleIP(), "INADDR_ANY") == 0)
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(), (uint32)INADDR_ANY);
            }
            else
            {
                nErr = listenConsoleAddr.set(App_MainConfig::instance()->GetConsolePort(), App_MainConfig::instance()->GetConsoleIP(), 1, PF_INET6);
            }
        }

        if (nErr != 0)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start]listenConsoleAddr set_address error[%d].\n", errno));
            return false;
        }

        int nRet = m_ConnectConsoleAcceptor.Init_Open(listenConsoleAddr);

        if (-1 == nRet)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Start] pConnectAcceptor->open is error.\n"));
            OUR_DEBUG((LM_INFO, "[CServerManager::Start] Listen from [%s:%d] error(%d).\n", listenConsoleAddr.get_host_addr(), listenConsoleAddr.get_port_number(), errno));
            return false;
        }
    }

    if (App_MainConfig::instance()->GetProcessCount() > 1)
    {
#ifndef WIN32
        //��ǰ������̸߳���
        int nNumChlid = App_MainConfig::instance()->GetProcessCount();

        //���ʱ��������
        //�����̼��ʱ����������ÿ��5��һ�Σ�
        ACE_Time_Value tvMonitorSleep(5, 0);

        //�ļ���
        int fd_lock = 0;

        int nRet = 0;

        //��õ�ǰ·��
        char szWorkDir[MAX_BUFF_500] = { 0 };

        if (!ACE_OS::getcwd(szWorkDir, sizeof(szWorkDir)))
        {
            exit(1);
        }

        //��Linux�²��ö���̵ķ�ʽ����
        // �򿪣����������ļ�
        char szFileName[200] = {'\0'};
        //memset(szFileName, 0, sizeof(szFileName));
        sprintf(szFileName, "%s/lockwatch.lk", szWorkDir);
        fd_lock = open(szFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        if (fd_lock < 0)
        {
            printf("open the flock and exit, errno = %d.\n", errno);
            exit(1);
        }

        //�鿴��ǰ�ļ����Ƿ�����
        nRet = SeeLock(fd_lock, 0, sizeof(int));

        if (nRet == -1 || nRet == 2)
        {
            printf("file is already exist!\n");
            exit(1);
        }

        //����ļ���û��������ס��ǰ�ļ���
        if (AcquireWriteLock(fd_lock, 0, sizeof(int)) != 0)
        {
            printf("lock the file failure and exit, idx = 0.\n");
            exit(1);
        }

        //д���ӽ�������Ϣ
        lseek(fd_lock, 0, SEEK_SET);

        for (int nIndex = 0; nIndex <= nNumChlid; nIndex++)
        {
            ssize_t stWrite = write(fd_lock, &nIndex, sizeof(nIndex));

            if (stWrite <= 0)
            {
                printf("lock write fail.\n");
            }
        }

        //����������ӽ���
        while (1)
        {
            for (int nChlidIndex = 1; nChlidIndex <= nNumChlid; nChlidIndex++)
            {
                //����100ms
                ACE_Time_Value tvSleep(0, 100000);
                ACE_OS::sleep(tvSleep);

                //����ÿ���ӽ��̵����Ƿ񻹴���
                nRet = SeeLock(fd_lock, nChlidIndex * sizeof(int), sizeof(int));

                if (nRet == -1 || nRet == 2)
                {
                    continue;
                }

                //����ļ���û�б������������ļ������������ӽ���
                int npid = ACE_OS::fork();

                if (npid == 0)
                {
                    //���ļ���
                    if (AcquireWriteLock(fd_lock, nChlidIndex * sizeof(int), sizeof(int)) != 0)
                    {
                        printf("child %d AcquireWriteLock failure.\n", nChlidIndex);
                        exit(1);
                    }

                    //�����ӽ���
                    if (false == Run())
                    {
                        printf("child %d Run failure.\n", nChlidIndex);
                        exit(1);
                    }

                    //�ӽ�����ִ�������������˳�ѭ�����ͷ���
                    ReleaseLock(fd_lock, nChlidIndex * sizeof(int), sizeof(int));
                }
            }

            //printf("child count(%d) is ok.\n", nNumChlid);
            //�����
            ACE_OS::sleep(tvMonitorSleep);
        }

#endif
    }
    else
    {
        if (false == Run())
        {
            printf("child Run failure.\n");
            return false;
        }
    }

    return true;
}

bool CServerManager::Init_Reactor(uint8 u1ReactorCount, uint8 u1NetMode)
{
    bool blState = true;

    //��ʼ����Ӧ��
    for (uint8 i = 0; i < u1ReactorCount; i++)
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]... i=[%d].\n", i));

        if (u1NetMode == NETWORKMODE_RE_SELECT)
        {
            blState = App_ReactorManager::instance()->AddNewReactor(i, Reactor_Select, 1);
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]AddNewReactor REACTOR_CLIENTDEFINE = Reactor_Select.\n"));
        }
        else if (u1NetMode == NETWORKMODE_RE_TPSELECT)
        {
            blState = App_ReactorManager::instance()->AddNewReactor(i, Reactor_TP, 1);
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]AddNewReactor REACTOR_CLIENTDEFINE = Reactor_TP.\n"));
        }
        else if (u1NetMode == NETWORKMODE_RE_EPOLL)
        {
            blState = App_ReactorManager::instance()->AddNewReactor(i, Reactor_DEV_POLL, 1, App_MainConfig::instance()->GetMaxHandlerCount());
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]AddNewReactor REACTOR_CLIENTDEFINE = Reactor_DEV_POLL.\n"));
        }
        else if (u1NetMode == NETWORKMODE_RE_EPOLL_ET)
        {
            blState = App_ReactorManager::instance()->AddNewReactor(i, Reactor_DEV_POLL_ET, 1, App_MainConfig::instance()->GetMaxHandlerCount());
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]AddNewReactor REACTOR_CLIENTDEFINE = Reactor_DEV_POLL_ET.\n"));
        }
        else
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]AddNewProactor NETWORKMODE Error.\n"));
            return false;
        }

        if (!blState)
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]AddNewReactor [%d] Error.\n", i));
            OUR_DEBUG((LM_INFO, "[CServerManager::Init_Reactor]Error=%s.\n", App_ReactorManager::instance()->GetError()));
            return false;
        }
    }

    return blState;
}

bool CServerManager::Run()
{
    //��Ӧ����̣�epoll�������ӽ�������г�ʼ��
    if (NETWORKMODE_RE_EPOLL == App_MainConfig::instance()->GetNetworkMode() || NETWORKMODE_RE_EPOLL_ET == App_MainConfig::instance()->GetNetworkMode())
    {
        //��ʼ����Ӧ������
        App_ReactorManager::instance()->Init((uint16)App_MainConfig::instance()->GetReactorCount());

        if (false == Init_Reactor((uint8)App_MainConfig::instance()->GetReactorCount(), App_MainConfig::instance()->GetNetworkMode()))
        {
            OUR_DEBUG((LM_INFO, "[CServerManager::Run]Init_Reactor Error.\n"));
            return false;
        }
    }

    int nServerPortCount = App_MainConfig::instance()->GetServerPortCount();

    //����������TCP��Ӧ��
    for (int i = 0; i < nServerPortCount; i++)
    {
        //�õ�������
        ConnectAcceptor* pConnectAcceptor = App_ConnectAcceptorManager::instance()->GetConnectAcceptor(i);

        //�򿪼�����Ӧ�¼�
        pConnectAcceptor->Run_Open(App_ReactorManager::instance()->GetAce_Reactor(REACTOR_CLIENTDEFINE));
    }

    m_ConnectConsoleAcceptor.Run_Open(App_ReactorManager::instance()->GetAce_Reactor(REACTOR_CLIENTDEFINE));

    //����������UDP��Ӧ��
    uint16 u2UDPServerPortCount = App_MainConfig::instance()->GetUDPServerPortCount();

    for (uint16 i = 0; i < u2UDPServerPortCount; i++)
    {
        CReactorUDPHander* pReactorUDPHandler = App_ReUDPManager::instance()->GetUDPHandle((uint8)i);

        if (NULL != pReactorUDPHandler)
        {
            pReactorUDPHandler->Run_Open(App_ReactorManager::instance()->GetAce_Reactor(REACTOR_CLIENTDEFINE));
        }
    }

    //������־�����߳�
    if (0 != AppLogManager::instance()->Start())
    {
        AppLogManager::instance()->WriteLog(LOG_SYSTEM, "[CServerManager::Init]AppLogManager is ERROR.");
    }
    else
    {
        AppLogManager::instance()->WriteLog(LOG_SYSTEM, "[CServerManager::Init]AppLogManager is OK.");
    }

    //������ʱ��
    if (0 != App_TimerManager::instance()->activate())
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Run]App_TimerManager::instance()->Start() is error.\n"));
        return false;
    }

    //�����м���������ӹ�����
    App_ClientReConnectManager::instance()->Init(App_ReactorManager::instance()->GetAce_Reactor(REACTOR_POSTDEFINE));
    App_ClientReConnectManager::instance()->StartConnectTask(App_MainConfig::instance()->GetConnectServerCheck());

    //�������з�Ӧ��
    if (!App_ReactorManager::instance()->StartReactor())
    {
        OUR_DEBUG((LM_INFO, "[CServerManager::Run]App_ReactorManager::instance()->StartReactor is error.\n"));
        return false;
    }

    //��ʼ��Ϣ�����߳�
    App_MessageServiceGroup::instance()->Start();

    if (App_MainConfig::instance()->GetConnectServerRunType() == 1)
    {
        //�����첽�������������Ϣ���Ĺ���
        App_ServerMessageTask::instance()->Start();
    }

    //��ʼ�������ӷ��Ͷ�ʱ��
    App_ConnectManager::instance()->StartTimer();

    ACE_Thread_Manager::instance()->wait();

    return true;
}

bool CServerManager::Close()
{
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close begin....\n"));
    App_ConnectAcceptorManager::instance()->Close();
    m_ConnectConsoleAcceptor.close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_TimerManager OK.\n"));
    App_TimerManager::instance()->deactivate();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_ReUDPManager OK.\n"));
    App_ReUDPManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_ModuleLoader OK.\n"));
    App_ClientReConnectManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_ClientReConnectManager OK.\n"));
    App_ModuleLoader::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_MessageManager OK.\n"));
    App_ServerMessageTask::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_ServerMessageTask OK.\n"));
    App_MessageServiceGroup::instance()->Close();
    OUR_DEBUG((LM_INFO, "[App_MessageServiceGroup::Close]Close App_MessageServiceGroup OK.\n"));
    App_ConnectManager::instance()->CloseAll();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_ConnectManager OK.\n"));
    AppLogManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]AppLogManager OK\n"));
    App_MessageManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_MessageManager OK.\n"));
    App_BuffPacketManager::instance()->Close();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]BuffPacketManager OK\n"));
    App_ReactorManager::instance()->StopReactor();
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close App_ReactorManager OK.\n"));
    OUR_DEBUG((LM_INFO, "[CServerManager::Close]Close end....\n"));

    if (NULL != m_pFrameLoggingStrategy)
    {
        m_pFrameLoggingStrategy->EndLogStrategy();
        SAFE_DELETE(m_pFrameLoggingStrategy);
    }

    return true;
}


