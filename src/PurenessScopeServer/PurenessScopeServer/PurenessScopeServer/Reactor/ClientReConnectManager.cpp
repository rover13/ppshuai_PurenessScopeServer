#include "ClientReConnectManager.h"

CReactorClientInfo::CReactorClientInfo()
{
    m_pConnectClient         = NULL;
    m_pClientMessage         = NULL;
    m_pReactorConnect        = NULL;
    m_pReactor               = NULL;
    m_nServerID              = 0;
    m_emConnectState         = SERVER_CONNECT_READY;
    m_AddrLocal              = (ACE_INET_Addr&)ACE_Addr::sap_any;
}

CReactorClientInfo::~CReactorClientInfo()
{
    OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::~CReactorClientInfo].\n"));
}

bool CReactorClientInfo::Init(int nServerID, const char* pIP, int nPort, uint8 u1IPType, CConnectClientConnector* pReactorConnect, IClientMessage* pClientMessage, ACE_Reactor* pReactor)
{
    int nRet = 0;

    if (u1IPType == TYPE_IPV4)
    {
        nRet = m_AddrServer.set(nPort, pIP);
    }
    else
    {
        nRet = m_AddrServer.set(nPort, pIP, 1, PF_INET6);
    }

    if (-1 == nRet)
    {
        OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Init]adrClient(%s:%d) error.\n", pIP, nPort));
        return false;
    }

    m_pClientMessage    = pClientMessage;
    m_pReactorConnect   = pReactorConnect;
    m_nServerID         = nServerID;
    m_pReactor          = pReactor;
    return true;
}

bool CReactorClientInfo::Run(bool blIsReady, EM_Server_Connect_State emState)
{
    if (NULL != m_pConnectClient)
    {
        OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Run]Connect is exist.\n"));
        return false;
    }

    if (NULL == m_pReactorConnect)
    {
        OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Run]m_pAsynchConnect is NULL.\n"));
        return false;
    }

    m_pConnectClient = new CConnectClient();

    if (NULL == m_pConnectClient)
    {
        OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Run]pConnectClient new is NULL(%d).\n", emState));
        return false;
    }

    m_pConnectClient->SetServerID(m_nServerID);
    m_pConnectClient->reactor(m_pReactor);

    if (blIsReady == true && SERVER_CONNECT_FIRST != m_emConnectState && SERVER_CONNECT_RECONNECT != m_emConnectState)
    {
        if (m_pReactorConnect->connect(m_pConnectClient, m_AddrServer, ACE_Synch_Options::defaults, m_AddrLocal) == -1)
        {
            m_emConnectState = SERVER_CONNECT_FAIL;
            OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Run](%s:%d) connection fails(ServerID=%d) error(%d).\n", m_AddrServer.get_host_addr(), m_AddrServer.get_port_number(), m_nServerID, ACE_OS::last_error()));
            //��������ΪTrue��Ϊ�����Զ�����������
            return true;
        }

        m_emConnectState = SERVER_CONNECT_OK;
    }

    return true;
}

bool CReactorClientInfo::SendData(ACE_Message_Block* pmblk)
{
    if (NULL == m_pConnectClient)
    {
        //����������ڽ��������У��ȴ�5���룬���
        if(SERVER_CONNECT_FIRST == m_emConnectState || SERVER_CONNECT_RECONNECT == m_emConnectState)
        {
            return false;
        }

        if (NULL == m_pConnectClient)
        {
            if(SERVER_CONNECT_FIRST != m_emConnectState && SERVER_CONNECT_RECONNECT != m_emConnectState)
            {
                //������Ӳ����ڣ��������ӡ�
                if (false == Run(true, SERVER_CONNECT_RECONNECT))
                {
                    OUR_DEBUG((LM_INFO, "[CReactorClientInfo::SendData]Run error.\n"));
                }
            }

            if (NULL != pmblk)
            {
                App_MessageBlockManager::instance()->Close(pmblk);
            }

            //�����Ϣ�д���ӿڣ��򷵻�ʧ�ܽӿ�
            if (NULL != m_pClientMessage)
            {
                //�������Ѿ��Ͽ�����Ҫ�ȴ��������ӵĽ��
                _ClientIPInfo objServerIPInfo;
                sprintf_safe(objServerIPInfo.m_szClientIP, MAX_BUFF_20, "%s", m_AddrServer.get_host_addr());
                objServerIPInfo.m_nPort = m_AddrServer.get_port_number();
                m_pClientMessage->ConnectError(101, objServerIPInfo);
            }

            return false;
        }
    }

    if(true == m_pClientMessage->Need_Send_Format())
    {
        //�������ݷ�����װ
        ACE_Message_Block* pSend = NULL;
        bool blRet = m_pClientMessage->Send_Format_data(pmblk->rd_ptr(), (uint32)pmblk->length(), App_MessageBlockManager::instance(), pSend);

        if(false == blRet)
        {
            App_MessageBlockManager::instance()->Close(pmblk);
            App_MessageBlockManager::instance()->Close(pSend);
            return false;
        }
        else
        {
            App_MessageBlockManager::instance()->Close(pmblk);
        }

        //��������
        return m_pConnectClient->SendData(pSend);
    }
    else
    {
        //��������
        return m_pConnectClient->SendData(pmblk);
    }
}

int CReactorClientInfo::GetServerID()
{
    return m_nServerID;
}

bool CReactorClientInfo::Close()
{
    //OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Close]Begin.\n"));
    if (NULL != m_pConnectClient)
    {
        //OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::Close]End 2.\n"));
        SetConnectClient(NULL);
    }

    return true;
}

void CReactorClientInfo::SetConnectClient(CConnectClient* pConnectClient)
{
    m_pConnectClient = pConnectClient;
}

CConnectClient* CReactorClientInfo::GetConnectClient()
{
    return m_pConnectClient;
}

IClientMessage* CReactorClientInfo::GetClientMessage()
{
    //���������Ƿ��������������ж�
    if ((m_emConnectState == SERVER_CONNECT_FAIL) && NULL != m_pClientMessage)
    {
        m_emConnectState = SERVER_CONNECT_OK;
        //֪ͨ�ϲ�ĳһ�������Ѿ��ָ�
        m_pClientMessage->ReConnect(m_nServerID);
    }

    return m_pClientMessage;
}

ACE_INET_Addr CReactorClientInfo::GetServerAddr()
{
    return m_AddrServer;
}

EM_Server_Connect_State CReactorClientInfo::GetServerConnectState()
{
    return m_emConnectState;
}

void CReactorClientInfo::SetServerConnectState(EM_Server_Connect_State objState)
{
    m_emConnectState = objState;
}

void CReactorClientInfo::SetLocalAddr( const char* pIP, int nPort, uint8 u1IPType )
{
    int nRet = 0;

    if (u1IPType == TYPE_IPV4)
    {
        nRet = m_AddrLocal.set(nPort, pIP);
    }
    else
    {
        nRet = m_AddrLocal.set(nPort, pIP, 1, PF_INET6);
    }

    if (-1 == nRet)
    {
        OUR_DEBUG((LM_ERROR, "[CReactorClientInfo::SetLocalAddr]AddrLocal(%s:%d) error.\n", pIP, nPort));
        return;
    }
}

CClientReConnectManager::CClientReConnectManager(void)
{
    m_nTaskID                = -1;
    m_pReactor               = NULL;
    m_blReactorFinish        = false;
    m_u4ConnectServerTimeout = 0;
    m_u4MaxPoolCount         = 0;
}

CClientReConnectManager::~CClientReConnectManager(void)
{
    OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::~CClientReConnectManager].\n"));
    //Close();
}

bool CClientReConnectManager::Init(ACE_Reactor* pReactor)
{
    if (-1 == m_ReactorConnect.open(pReactor))
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Init]open error(%d).", ACE_OS::last_error()));
        return false;
    }

    //��¼����ص��������
    m_u4MaxPoolCount = App_MainConfig::instance()->GetServerConnectCount();

    //��ʼ��Hash����(TCP)
    m_objClientTCPList.Init((int)m_u4MaxPoolCount);

    //��ʼ��Hash����(UDP)
    m_objClientUDPList.Init((int)m_u4MaxPoolCount);

    m_u4ConnectServerTimeout = App_MainConfig::instance()->GetConnectServerTimeout() * 1000; //ת��Ϊ΢��

    if (m_u4ConnectServerTimeout == 0)
    {
        m_u4ConnectServerTimeout = RE_CONNECT_SERVER_TIMEOUT;
    }

    m_pReactor        = pReactor;
    m_blReactorFinish = true;
    return true;
}

bool CClientReConnectManager::Connect(int nServerID, const char* pIP, int nPort, uint8 u1IPType, IClientMessage* pClientMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    CReactorClientInfo* pClientInfo = NULL;

    //���ӳ�ʼ������
    if (false == ConnectTcpInit(nServerID, pIP, nPort, u1IPType, NULL, 0, u1IPType, pClientMessage, pClientInfo))
    {
        return false;
    }

    //��ʼ����
    if (false == pClientInfo->Run(m_blReactorFinish, SERVER_CONNECT_FIRST))
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect]Run Error.\n"));
        delete pClientInfo;
        pClientInfo = NULL;

        if (false == Close(nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CClientReConnectManager::Connect]Close Error"));
        }

        return false;
    }

    OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect]nServerID =(%d) connect is OK.\n", nServerID));
    return true;
}

bool CClientReConnectManager::Connect(int nServerID, const char* pIP, int nPort, uint8 u1IPType, const char* pLocalIP, int nLocalPort, uint8 u1LocalIPType, IClientMessage* pClientMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    CReactorClientInfo* pClientInfo = NULL;

    //���ӳ�ʼ������
    if (false == ConnectTcpInit(nServerID, pIP, nPort, u1IPType, pLocalIP, nLocalPort, u1LocalIPType, pClientMessage, pClientInfo))
    {
        return false;
    }

    //��ʼ����
    if (false == pClientInfo->Run(m_blReactorFinish, SERVER_CONNECT_FIRST))
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect]Run Error.\n"));
        delete pClientInfo;
        pClientInfo = NULL;

        if (false == Close(nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CClientReConnectManager::Connect]Close Error.\n"));
        }

        return false;
    }

    OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect]nServerID =(%d) connect is OK.\n", nServerID));
    return true;
}

bool CClientReConnectManager::ConnectUDP(int nServerID, const char* pIP, int nPort, uint8 u1IPType, EM_UDP_TYPE emType, IClientUDPMessage* pClientUDPMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    CReactorUDPClient* pReactorUDPClient = NULL;

    //��ʼ�����Ӷ���
    if (false == ConnectUdpInit(nServerID, pReactorUDPClient))
    {
        return false;
    }

    //��ʼ�����ӵ�ַ
    ACE_INET_Addr AddrLocal;
    int nErr = 0;

    if (emType != UDP_BROADCAST)
    {
        if (u1IPType == TYPE_IPV4)
        {
            nErr = AddrLocal.set(nPort, pIP);
        }
        else
        {
            nErr = AddrLocal.set(nPort, pIP, 1, PF_INET6);
        }
    }
    else
    {
        //�����UDP�㲥
        AddrLocal.set(nPort, (uint32)INADDR_ANY);
    }

    if (nErr != 0)
    {
        OUR_DEBUG((LM_INFO, "[CClientReConnectManager::ConnectUDP](%d)UDP set_address error[%d].\n", nServerID, errno));
        SAFE_DELETE(pReactorUDPClient);
        return false;
    }

    //��ʼ����
    if (0 != pReactorUDPClient->OpenAddress(AddrLocal, emType, App_ReactorManager::instance()->GetAce_Reactor(REACTOR_UDPDEFINE), pClientUDPMessage))
    {
        OUR_DEBUG((LM_INFO, "[CClientReConnectManager::ConnectUDP](%d)UDP OpenAddress error.\n", nServerID));
        SAFE_DELETE(pReactorUDPClient);
        return false;
    }

    return true;
}

bool CClientReConnectManager::SetHandler(int nServerID, CConnectClient* pConnectClient)
{
    if (NULL == pConnectClient)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::SetHandler]pProConnectClient is NULL.\n"));
        return false;
    }

    //������������
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //�����������Ѿ����ڣ�������ӵ��Ѿ����ڵĿͻ���Hash���������
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::SetHandler]nServerID =(%d) is not exist.\n", nServerID));
        return false;
    }

    pClientInfo->SetConnectClient(pConnectClient);
    return true;
}

bool CClientReConnectManager::Close(int nServerID)
{
    //�������Ϊ�������Ͽ�����ֻɾ��ProConnectClient��ָ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    //������������
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    //�ر����Ӷ���
    if (NULL != pClientInfo->GetConnectClient())
    {
        pClientInfo->GetConnectClient()->ClientClose();
    }

    //��Hash����ɾ����ǰ���ڵĶ���
    m_objClientTCPList.Del_Hash_Data(szServerID);

    return true;
}

bool CClientReConnectManager::CloseUDP(int nServerID)
{
    //�������Ϊ�������Ͽ�����ֻɾ��ProConnectClient��ָ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    //������������
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorUDPClient* pClientInfo = m_objClientUDPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::CloseUDP]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    pClientInfo->Close();
    SAFE_DELETE(pClientInfo);
    //��hash����ɾ����ǰ���ڵĶ���
    m_objClientUDPList.Del_Hash_Data(szServerID);
    return true;
}

bool CClientReConnectManager::ConnectErrorClose(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    //������������
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::ConnectErrorClose]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    //��Hash����ɾ����ǰ���ڵĶ���

    SAFE_DELETE(pClientInfo);
    m_objClientTCPList.Del_Hash_Data(szServerID);
    return true;
}

IClientMessage* CClientReConnectManager::GetClientMessage(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    //������������
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL != pClientInfo)
    {
        return pClientInfo->GetClientMessage();
    }

    return NULL;
}

bool CClientReConnectManager::SendData(int nServerID, char*& pData, int nSize, bool blIsDelete)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    //������������
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //�����������Ѿ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CProConnectManager::SendData]nServerID =(%d) is not exist.\n", nServerID));

        if (true == blIsDelete)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }

    ACE_Message_Block* pmblk = App_MessageBlockManager::instance()->Create(nSize);

    if (NULL == pmblk)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::SendData]nServerID =(%d) pmblk is NULL.\n", nServerID));

        if (true == blIsDelete)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }

    memcpy_safe((char* )pData, (uint32)nSize, pmblk->wr_ptr(), (uint32)nSize);
    pmblk->wr_ptr(nSize);

    if (true == blIsDelete)
    {
        SAFE_DELETE_ARRAY(pData);
    }

    //��������
    return pClientInfo->SendData(pmblk);
}

bool CClientReConnectManager::SendDataUDP(int nServerID, const char* pIP, int nPort, const char*& pMessage, uint32 u4Len, bool blIsDelete)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);
    CReactorUDPClient* pClientInfo = m_objClientUDPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //�����������Ѿ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CProConnectManager::Close]nServerID =(%d) is not exist.\n", nServerID));

        if (true == blIsDelete)
        {
            SAFE_DELETE_ARRAY(pMessage);
        }

        return false;
    }

    //��������
    bool blSendRet = pClientInfo->SendMessage(pMessage, u4Len, pIP, nPort);

    if (true == blIsDelete)
    {
        SAFE_DELETE_ARRAY(pMessage);
    }

    return blSendRet;
}

bool CClientReConnectManager::StartConnectTask(int nIntervalTime)
{
    CancelConnectTask();
    m_nTaskID = m_ActiveTimer.schedule(this, (void*)NULL, ACE_OS::gettimeofday() + ACE_Time_Value(nIntervalTime), ACE_Time_Value(nIntervalTime));

    if (m_nTaskID == -1)
    {
        OUR_DEBUG((LM_ERROR, "[CProConnectManager::StartConnectTask].StartConnectTask is fail, time is (%d).\n", nIntervalTime));
        return false;
    }

    m_ActiveTimer.activate();
    return true;
}

void CClientReConnectManager::CancelConnectTask()
{
    if (m_nTaskID != -1)
    {
        //ɱ��֮ǰ�Ķ�ʱ�������¿����µĶ�ʱ��
        m_ActiveTimer.cancel(m_nTaskID);
        m_nTaskID = -1;
    }
}

void CClientReConnectManager::Close()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]Begin.\n"));
    //����ж�ʱ������ɾ����ʱ��
    CancelConnectTask();

    //�ر������Ѵ��ڵ�����
    vector<CReactorClientInfo*> vecReactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecReactorClientInfo);

    for(int i = 0; i < (int)vecReactorClientInfo.size(); i++)
    {
        CReactorClientInfo* pClientInfo = vecReactorClientInfo[i];

        if(NULL != pClientInfo)
        {
            if (false == pClientInfo->Close())
            {
                OUR_DEBUG((LM_INFO, "[CClientReConnectManager::Close]Close error.\n"));
            }

            SAFE_DELETE(pClientInfo);
        }
    }

    m_objClientTCPList.Close();

    vector<CReactorUDPClient*> vecReactorUDPClient;
    m_objClientUDPList.Get_All_Used(vecReactorUDPClient);

    for(int i = 0; i < (int)vecReactorUDPClient.size(); i++)
    {
        CReactorUDPClient* pClientInfo = vecReactorUDPClient[i];

        if(NULL != pClientInfo)
        {
            pClientInfo->Close();
            SAFE_DELETE(pClientInfo);
        }
    }

    m_objClientUDPList.Close();
    m_u4MaxPoolCount = 0;
    m_ActiveTimer.deactivate();
    OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]End.\n"));
}

bool CClientReConnectManager::ConnectTcpInit(int nServerID, const char* pIP, int nPort, uint8 u1IPType, const char* pLocalIP, int nLocalPort, uint8 u1LocalIPType, IClientMessage* pClientMessage, CReactorClientInfo*& pClientInfo)
{
    //������������
    char szServerID[10] = { '\0' };
    sprintf_safe(szServerID, 10, "%d", nServerID);
    pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL != pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect](%d)pClientInfo is exist.\n", nServerID));
        return false;
    }

    //������Ѿ����ˣ�����������������
    if (m_objClientTCPList.Get_Used_Count() == m_objClientTCPList.Get_Count())
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) m_objClientTCPList is full.\n", nServerID));
        return false;
    }

    //��ʼ��������Ϣ
    pClientInfo = new CReactorClientInfo();

    if (NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect]pClientInfo is NULL.\n"));
        return false;
    }

    //�����Ч��pClientMessage
    App_ServerMessageTask::instance()->AddClientMessage(pClientMessage);

    if (false == pClientInfo->Init(nServerID, pIP, nPort, u1IPType, &m_ReactorConnect, pClientMessage, m_pReactor))
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect]pClientInfo Init Error.\n"));
        SAFE_DELETE(pClientInfo);

        if (false == Close(nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CClientReConnectManager::ConnectTcpInit]Close error.\n"));
        }

        return false;
    }

    //���ñ���IP�Ͷ˿�
    if (NULL != pLocalIP && nLocalPort > 0)
    {
        pClientInfo->SetLocalAddr(pLocalIP, nLocalPort, u1LocalIPType);
    }

    //��ӽ�hash
    if (-1 == m_objClientTCPList.Add_Hash_Data(szServerID, pClientInfo))
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) add m_objClientTCPList is fail.\n", nServerID));
        SAFE_DELETE(pClientInfo);
        return false;
    }

    return true;
}

bool CClientReConnectManager::ConnectUdpInit(int nServerID, CReactorUDPClient*& pReactorUDPClient)
{
    //������������
    char szServerID[10] = { '\0' };
    sprintf_safe(szServerID, 10, "%d", nServerID);
    pReactorUDPClient = m_objClientUDPList.Get_Hash_Box_Data(szServerID);

    if (NULL != pReactorUDPClient)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Connect](%d)pClientInfo is exist.\n", nServerID));
        return false;
    }

    //������Ѿ����ˣ�����������������
    if (m_objClientUDPList.Get_Used_Count() == m_objClientUDPList.Get_Count())
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) m_objClientTCPList is full.\n", nServerID));
        return false;
    }

    pReactorUDPClient = new CReactorUDPClient();

    if (NULL == pReactorUDPClient)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::ConnectUDP]nServerID =(%d) pProactorUDPClient is NULL.\n", nServerID));
        return false;
    }

    //�����Ѿ���������ӽ�hash
    if (-1 == m_objClientUDPList.Add_Hash_Data(szServerID, pReactorUDPClient))
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) add m_objClientTCPList is fail.\n", nServerID));
        SAFE_DELETE(pReactorUDPClient);
        return false;
    }

    return true;
}

int CClientReConnectManager::handle_timeout(const ACE_Time_Value& tv, const void* arg)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    if (arg != NULL)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::handle_timeout] arg is not NULL, tv = %d.\n", tv.sec()));
    }

    m_ThreadWritrLock.acquire();
    vector<CReactorClientInfo*> vecCReactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecCReactorClientInfo);
    m_ThreadWritrLock.release();

    for (int i = 0; i < (int)vecCReactorClientInfo.size(); i++)
    {
        //int nServerID = (int)b->first;
        CReactorClientInfo* pClientInfo = vecCReactorClientInfo[i];

        if(NULL != pClientInfo)
        {
            if (NULL == pClientInfo->GetConnectClient())
            {
                //������Ӳ����ڣ������½�������
                if (false == pClientInfo->Run(m_blReactorFinish, SERVER_CONNECT_RECONNECT))
                {
                    OUR_DEBUG((LM_INFO, "[CClientReConnectManager::handle_timeout]Run error.\n"));
                }
            }
            else
            {
                //��鵱ǰ���ӣ��Ƿ��ѹ��������
                ACE_Time_Value tvNow = ACE_OS::gettimeofday();

                //������첽ģʽ������Ҫ��鴦���߳��Ƿ񱻹���
                if(App_MainConfig::instance()->GetConnectServerRunType() == 1)
                {
                    App_ServerMessageTask::instance()->CheckServerMessageThread(tvNow);
                }
                else
                {
                    pClientInfo->GetConnectClient()->GetTimeout(tvNow);
                }
            }
        }
    }

    return 0;
}

void CClientReConnectManager::GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    VecClientConnectInfo.clear();
    vector<CReactorClientInfo*> vecReactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecReactorClientInfo);

    for (int i = 0; i < (int)vecReactorClientInfo.size(); i++)
    {
        CReactorClientInfo* pClientInfo =  vecReactorClientInfo[i];

        if (NULL != pClientInfo)
        {
            if (NULL != pClientInfo->GetConnectClient())
            {
                _ClientConnectInfo ClientConnectInfo = pClientInfo->GetConnectClient()->GetClientConnectInfo();
                ClientConnectInfo.m_addrRemote = pClientInfo->GetServerAddr();
                VecClientConnectInfo.push_back(ClientConnectInfo);
            }
            else
            {
                _ClientConnectInfo ClientConnectInfo;
                ClientConnectInfo.m_blValid    = false;
                ClientConnectInfo.m_addrRemote = pClientInfo->GetServerAddr();
                VecClientConnectInfo.push_back(ClientConnectInfo);
            }
        }
    }
}

void CClientReConnectManager::GetUDPConnectInfo(vecClientConnectInfo& VecClientConnectInfo)
{
    VecClientConnectInfo.clear();
    vector<CReactorUDPClient*> vecReactorUDPClient;
    m_objClientUDPList.Get_All_Used(vecReactorUDPClient);

    for (int i = 0; i < (int)vecReactorUDPClient.size(); i++)
    {
        CReactorUDPClient* pClientInfo = vecReactorUDPClient[i];

        if (NULL != pClientInfo)
        {
            _ClientConnectInfo ClientConnectInfo = pClientInfo->GetClientConnectInfo();
            VecClientConnectInfo.push_back(ClientConnectInfo);
        }
    }
}

bool CClientReConnectManager::CloseByClient(int nServerID)
{
    //�������ΪԶ�����ӶϿ�����ֻɾ��ProConnectClient��ָ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    pClientInfo->SetConnectClient(NULL);
    pClientInfo->SetServerConnectState(SERVER_CONNECT_FAIL);

    return true;
}

EM_Server_Connect_State CClientReConnectManager::GetConnectState( int nServerID )
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //�����������Ѿ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::GetConnectState]nServerID =(%d) is not exist.\n", nServerID));
        return SERVER_CONNECT_FAIL;
    }

    return pClientInfo->GetServerConnectState();
}

bool CClientReConnectManager::ReConnect(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    if (NULL == pClientInfo->GetConnectClient())
    {
        //������Ӳ����ڣ������½�������
        if (false == pClientInfo->Run(m_blReactorFinish, SERVER_CONNECT_RECONNECT))
        {
            OUR_DEBUG((LM_INFO, "[CClientReConnectManager::Close]Run error.\n"));
        }

        return true;
    }
    else
    {
        return true;
    }
}

ACE_INET_Addr CClientReConnectManager::GetServerAddr(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    ACE_INET_Addr remote_addr;
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //���������Ӳ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]nServerID =(%d) is not exist.\n", nServerID));
        return remote_addr;
    }
    else
    {
        remote_addr = pClientInfo->GetServerAddr();
        return remote_addr;
    }
}

bool CClientReConnectManager::SetServerConnectState(int nServerID, EM_Server_Connect_State objState)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //���������Ӳ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]nServerID =(%d) is not exist.\n", nServerID));
        return false;
    }
    else
    {
        pClientInfo->SetServerConnectState(objState);
        return true;
    }
}

bool CClientReConnectManager::GetServerIPInfo( int nServerID, _ClientIPInfo& objServerIPInfo )
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CReactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL == pClientInfo)
    {
        //���������Ӳ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientReConnectManager::Close]nServerID =(%d) is not exist.\n", nServerID));
        return false;
    }
    else
    {
        ACE_INET_Addr remote_addr = pClientInfo->GetServerAddr();
        sprintf_safe(objServerIPInfo.m_szClientIP, MAX_BUFF_50, remote_addr.get_host_addr());
        objServerIPInfo.m_nPort = remote_addr.get_port_number();
        return true;
    }
}

bool CClientReConnectManager::DeleteIClientMessage(IClientMessage* pClientMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    //���첽�ص���Ч�����д�pClientMessage����Ϊ��Ч
    App_ServerMessageTask::instance()->DelClientMessage(pClientMessage);

    //һһѰ����֮��Ӧ�������Լ������Ϣ��ɾ��֮
    vector<CReactorClientInfo*> vecReactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecReactorClientInfo);

    for(int i = 0; i < (int)vecReactorClientInfo.size(); i++)
    {
        CReactorClientInfo* pClientInfo = vecReactorClientInfo[i];

        if(NULL != pClientInfo && pClientInfo->GetClientMessage() == pClientMessage)
        {
            //�ر����ӣ���ɾ������
            //�ر����Ӷ���
            if (NULL != pClientInfo->GetConnectClient())
            {
                pClientInfo->GetConnectClient()->ClientClose();
            }

            char szServerID[10] = {'\0'};
            sprintf_safe(szServerID, 10, "%d", pClientInfo->GetServerID());

            SAFE_DELETE(pClientInfo);
            m_objClientTCPList.Del_Hash_Data(szServerID);
            return true;
        }
    }

    return true;
}
