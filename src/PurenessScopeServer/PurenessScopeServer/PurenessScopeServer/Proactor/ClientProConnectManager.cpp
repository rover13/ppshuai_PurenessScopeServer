#include "ClientProConnectManager.h"

CProactorClientInfo::CProactorClientInfo()
{
    m_pProConnectClient = NULL;
    m_pProAsynchConnect = NULL;
    m_pClientMessage    = NULL;
    m_szServerIP[0]     = '\0';
    m_nPort             = 0;
    m_nServerID         = 0;
    m_emConnectState    = SERVER_CONNECT_READY;
    //m_AddrLocal         = (ACE_INET_Addr &)ACE_Addr::sap_any;
}

CProactorClientInfo::~CProactorClientInfo()
{
}

bool CProactorClientInfo::Init(const char* pIP, int nPort, uint8 u1IPType, int nServerID, CProAsynchConnect* pProAsynchConnect, IClientMessage* pClientMessage)
{
    OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Init]SetAddrServer(%s:%d) Begin.\n", pIP, nPort));

    int nRet = 0;

    if(u1IPType == TYPE_IPV4)
    {
        nRet = m_AddrServer.set(nPort, pIP);
    }
    else
    {
        nRet = m_AddrServer.set(nPort, pIP, 1, PF_INET6);
    }

    if(-1 == nRet)
    {
        OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Init]nServerID = %d, adrClient(%s:%d) error.\n", nServerID, pIP, nPort));
        return false;
    }

    m_pProAsynchConnect = pProAsynchConnect;
    m_pClientMessage    = pClientMessage;
    m_nServerID         = nServerID;

    sprintf_safe(m_szServerIP, MAX_BUFF_20, "%s", pIP);
    m_nPort = nPort;

    return true;
}

bool CProactorClientInfo::Run(bool blIsReadly, EM_Server_Connect_State emState)
{
    if(NULL != m_pProConnectClient)
    {
        OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Run]Connect is exist.\n"));
        return false;
    }

    if(NULL == m_pProAsynchConnect)
    {
        OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Run]m_pAsynchConnect is NULL.\n"));
        return false;
    }

    /*
    //����Զ�˷�����
    if(m_pProAsynchConnect->GetConnectState() == true)
    {
    OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Run]m_pProAsynchConnect is run.\n"));
    return false;
    }
    */

    if(true == blIsReadly && SERVER_CONNECT_FIRST != m_emConnectState && SERVER_CONNECT_RECONNECT != m_emConnectState)
    {
        m_pProAsynchConnect->SetConnectState(true);
        //OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Run]Connect IP=%s,Port=%d.\n", m_AddrServer.get_host_addr(), m_AddrServer.get_port_number()));

        //����һ�����ݲ���������Զ��
        _ProConnectState_Info* pProConnectInfo = new _ProConnectState_Info();
        pProConnectInfo->m_nServerID = m_nServerID;

        m_emConnectState = emState;

        if(m_pProAsynchConnect->connect(m_AddrServer, (const ACE_INET_Addr&)m_AddrLocal, 1, (const void*)pProConnectInfo) == -1)
        {
            OUR_DEBUG((LM_ERROR, "[CProactorClientInfo::Run]m_pAsynchConnect open error(%d).\n", ACE_OS::last_error()));
            return false;
        }
    }

    return true;
}

bool CProactorClientInfo::SendData(ACE_Message_Block* pmblk)
{
    if(NULL == m_pProConnectClient)
    {
        //����������ڽ��������У��ȴ�5���룬���
        if(SERVER_CONNECT_FIRST == m_emConnectState || SERVER_CONNECT_RECONNECT == m_emConnectState)
        {
            return false;
        }

        if(NULL == m_pProConnectClient)
        {
            if(SERVER_CONNECT_FIRST != m_emConnectState && SERVER_CONNECT_RECONNECT != m_emConnectState)
            {
                //������Ӳ����ڣ��������ӡ�
                if (false == Run(true, SERVER_CONNECT_RECONNECT))
                {
                    OUR_DEBUG((LM_INFO, "[CProactorClientInfo::SendData]Run error.\n"));
                }
            }

            if(NULL != pmblk)
            {
                App_MessageBlockManager::instance()->Close(pmblk);
            }

            //�����Ϣ�д���ӿڣ��򷵻�ʧ�ܽӿ�
            if(NULL != m_pClientMessage)
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
        return m_pProConnectClient->SendData(pSend);
    }
    else
    {
        //��������
        return m_pProConnectClient->SendData(pmblk);
    }
}

int CProactorClientInfo::GetServerID()
{
    if(NULL == m_pProConnectClient)
    {
        return 0;
    }
    else
    {
        return m_pProConnectClient->GetServerID();
    }
}

bool CProactorClientInfo::Close()
{
    if(NULL != m_pProConnectClient)
    {
        SetProConnectClient(NULL);
    }

    return true;
}

void CProactorClientInfo::SetProConnectClient(CProConnectClient* pProConnectClient)
{
    m_pProConnectClient = pProConnectClient;
}

CProConnectClient* CProactorClientInfo::GetProConnectClient()
{
    return m_pProConnectClient;
}

IClientMessage* CProactorClientInfo::GetClientMessage()
{
    //���������Ƿ��������������ж��Լ��Ƿ��ǵ�һ�����ӵĻص�
    if((m_emConnectState == SERVER_CONNECT_RECONNECT || m_emConnectState == SERVER_CONNECT_FIRST) && NULL != m_pClientMessage)
    {
        m_emConnectState = SERVER_CONNECT_OK;
        //֪ͨ�ϲ�ĳһ�������Ѿ��ָ������ѽ���
        m_pClientMessage->ReConnect(m_nServerID);
    }

    return m_pClientMessage;
}

ACE_INET_Addr CProactorClientInfo::GetServerAddr()
{
    return m_AddrServer;
}

EM_Server_Connect_State CProactorClientInfo::GetServerConnectState()
{
    return m_emConnectState;
}

void CProactorClientInfo::SetServerConnectState( EM_Server_Connect_State objState )
{
    m_emConnectState = objState;
}

void CProactorClientInfo::SetLocalAddr( const char* pIP, int nPort, uint8 u1IPType )
{
    if(u1IPType == TYPE_IPV4)
    {
        m_AddrLocal.set(nPort, pIP);
    }
    else
    {
        m_AddrLocal.set(nPort, pIP, 1, PF_INET6);
    }
}

CClientProConnectManager::CClientProConnectManager(void)
{
    m_nTaskID                = -1;
    m_blProactorFinish       = false;
    m_u4MaxPoolCount         = 0;
    m_u4ConnectServerTimeout = 0;
}

CClientProConnectManager::~CClientProConnectManager(void)
{
    Close();
}

bool CClientProConnectManager::Init(ACE_Proactor* pProactor)
{
    if(pProactor == NULL)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Init]pProactor is NULL.\n"));
        return false;
    }

    m_u4ConnectServerTimeout = App_MainConfig::instance()->GetConnectServerTimeout() * 1000; //ת��Ϊ΢��

    if(m_u4ConnectServerTimeout == 0)
    {
        m_u4ConnectServerTimeout = PRO_CONNECT_SERVER_TIMEOUT;
    }

    //��¼����ص��������
    m_u4MaxPoolCount = App_MainConfig::instance()->GetServerConnectCount();

    //��ʼ��Hash����(TCP)
    m_objClientTCPList.Init((int)m_u4MaxPoolCount);

    //��ʼ��Hash����(UDP)
    m_objClientUDPList.Init((int)m_u4MaxPoolCount);

    if(-1 == m_ProAsynchConnect.open(false, pProactor, true))
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Init]m_ProAsynchConnect open error(%d).\n", ACE_OS::last_error()));
        return false;
    }
    else
    {
        //���Proactor�Ѿ����ӳɹ�
        m_blProactorFinish = true;

        //�����������ɹ�����ʱ��������ʱ��
        m_ActiveTimer.activate();

        return true;
    }
}

bool CClientProConnectManager::Connect(int nServerID, const char* pIP, int nPort, uint8 u1IPType, IClientMessage* pClientMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    CProactorClientInfo* pClientInfo = NULL;

    //���ӳ�ʼ������
    if (false == ConnectTcpInit(nServerID, pIP, nPort, u1IPType, NULL, 0, u1IPType, pClientMessage, pClientInfo))
    {
        return false;
    }

    //��һ�ο�ʼ����
    if(false == pClientInfo->Run(m_blProactorFinish, SERVER_CONNECT_FIRST))
    {
        SAFE_DELETE(pClientInfo);
        return false;
    }

    //�����Ч��pClientMessage
    App_ServerMessageTask::instance()->AddClientMessage(pClientMessage);

    OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) connect is OK.\n", nServerID));

    return true;
}

bool CClientProConnectManager::Connect(int nServerID, const char* pIP, int nPort, uint8 u1IPType, const char* pLocalIP, int nLocalPort, uint8 u1LocalIPType, IClientMessage* pClientMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    CProactorClientInfo* pClientInfo = NULL;

    //���ӳ�ʼ������
    if (false == ConnectTcpInit(nServerID, pIP, nPort, u1IPType, pLocalIP, nLocalPort, u1LocalIPType, pClientMessage, pClientInfo))
    {
        return false;
    }

    //��һ�ο�ʼ����
    if(false == pClientInfo->Run(m_blProactorFinish, SERVER_CONNECT_FIRST))
    {
        SAFE_DELETE(pClientInfo);
        return false;
    }

    //�����Ч��pClientMessage
    if (App_MainConfig::instance()->GetConnectServerRunType() == 1)
    {
        App_ServerMessageTask::instance()->AddClientMessage(pClientMessage);
    }

    OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) connect is OK.\n", nServerID));

    return true;
}

bool CClientProConnectManager::ConnectUDP(int nServerID, const char* pIP, int nPort, uint8 u1IPType, EM_UDP_TYPE emType, IClientUDPMessage* pClientUDPMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    CProactorUDPClient* pProactorUDPClient = NULL;

    //���ӳ�ʼ��
    if (false == ConnectUdpInit(nServerID, pProactorUDPClient))
    {
        return false;
    }

    //����UDP���ӵ�ַ
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
        OUR_DEBUG((LM_INFO, "[CClientProConnectManager::ConnectUDP](%d)UDP set_address error[%d].\n", nServerID, errno));
        SAFE_DELETE(pProactorUDPClient);
        return false;
    }

    //����UDP
    if(0 != pProactorUDPClient->OpenAddress(AddrLocal, emType, App_ProactorManager::instance()->GetAce_Proactor(REACTOR_UDPDEFINE), pClientUDPMessage))
    {
        OUR_DEBUG((LM_INFO, "[CClientProConnectManager::ConnectUDP](%d)UDP OpenAddress error.\n", nServerID));
        SAFE_DELETE(pProactorUDPClient);
        return false;
    }

    return true;
}

bool CClientProConnectManager::SetHandler(int nServerID, CProConnectClient* pProConnectClient)
{
    if(NULL == pProConnectClient)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::SetHandler]pProConnectClient is NULL.\n"));
        return false;
    }

    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL != pClientInfo)
    {
        pClientInfo->SetProConnectClient(pProConnectClient);
    }
    else
    {
        //���������Ӳ�����
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::SetHandler]nServerID =(%d) is not exist.\n", nServerID));
        return false;
    }

    return true;
}

bool CClientProConnectManager::Close(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    //�������Ϊ�������Ͽ�����ֻɾ��ProConnectClient��ָ��
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Close]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    EM_s2s ems2s = S2S_INNEED_CALLBACK;

    //�ر����Ӷ���
    if(NULL != pClientInfo->GetProConnectClient())
    {
        //OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Close]nServerID =(%d) Begin.\n", nServerID));
        pClientInfo->GetProConnectClient()->ClientClose(ems2s);
        //OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Close]nServerID =(%d) End.\n", nServerID));
    }

    if(S2S_NEED_CALLBACK == ems2s)
    {
        SAFE_DELETE(pClientInfo);
    }
    else
    {
        if (false == pClientInfo->Close())
        {
            OUR_DEBUG((LM_INFO, "[CClientProConnectManager::Close]pClientInfo->Close fail.\n"));
        }

        SAFE_DELETE(pClientInfo);
    }

    //��Hash������ɾ��
    m_objClientTCPList.Del_Hash_Data(szServerID);

    return true;
}

bool CClientProConnectManager::CloseUDP(int nServerID)
{
    //�������Ϊ�������Ͽ�����ֻɾ��ProConnectClient��ָ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorUDPClient* pClientInfo = m_objClientUDPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::CloseUDP]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    pClientInfo->Close();
    SAFE_DELETE(pClientInfo);
    //��Hash����ɾ����ǰ���ڵĶ���
    m_objClientUDPList.Del_Hash_Data(szServerID);
    return true;
}

bool CClientProConnectManager::ConnectErrorClose(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::ConnectErrorClose]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    //��Hash����ɾ����ǰ���ڵĶ���
    SAFE_DELETE(pClientInfo);
    m_objClientTCPList.Del_Hash_Data(szServerID);

    return true;
}

IClientMessage* CClientProConnectManager::GetClientMessage(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL != pClientInfo)
    {
        return pClientInfo->GetClientMessage();
    }

    return NULL;
}

bool CClientProConnectManager::SendData(int nServerID, char*& pData, int nSize, bool blIsDelete)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        //���������Ӳ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::SendData]nServerID =(%d) is not exist.\n", nServerID));

        if(true == blIsDelete)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }

    ACE_Message_Block* pmblk = App_MessageBlockManager::instance()->Create(nSize);

    if(NULL == pmblk)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::SendData]nServerID =(%d) pmblk is NULL.\n", nServerID));

        if(true == blIsDelete)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }

    memcpy_safe((char* )pData, (uint32)nSize, (char* )pmblk->wr_ptr(), (uint32)nSize);
    pmblk->wr_ptr(nSize);

    if(true == blIsDelete && NULL != pData)
    {
        SAFE_DELETE_ARRAY(pData);
    }

    //��������
    return pClientInfo->SendData(pmblk);
}

bool CClientProConnectManager::SendDataUDP(int nServerID,const char* pIP, int nPort, const char*& pMessage, uint32 u4Len, bool blIsDelete)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorUDPClient* pClientInfo = m_objClientUDPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        //���������Ӳ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::SendDataUDP]nServerID =(%d) is not exist.\n", nServerID));

        if(true == blIsDelete)
        {
            SAFE_DELETE_ARRAY(pMessage);
        }

        return false;
    }

    //��������
    bool blSendRet = pClientInfo->SendMessage(pMessage, u4Len, pIP, nPort);

    if(true == blIsDelete)
    {
        SAFE_DELETE_ARRAY(pMessage);
    }

    return blSendRet;
}

bool CClientProConnectManager::StartConnectTask(int nIntervalTime)
{
    CancelConnectTask();
    m_nTaskID = m_ActiveTimer.schedule(this, (void* )NULL, ACE_OS::gettimeofday() + ACE_Time_Value(nIntervalTime), ACE_Time_Value(nIntervalTime));

    if(m_nTaskID == -1)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::StartConnectTask].StartConnectTask is fail, time is (%d).\n", nIntervalTime));
        return false;
    }

    m_ActiveTimer.activate();
    return true;
}

void CClientProConnectManager::CancelConnectTask()
{
    if(m_nTaskID != -1)
    {
        //ɱ��֮ǰ�Ķ�ʱ�������¿����µĶ�ʱ��
        m_ActiveTimer.cancel(m_nTaskID);
        m_nTaskID = -1;
    }
}

void CClientProConnectManager::Close()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    //����ж�ʱ�����رն�ʱ��
    CancelConnectTask();

    //�ر������Ѵ��ڵ�����
    vector<CProactorClientInfo*> vecProactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecProactorClientInfo);

    for(int i = 0; i < (int)vecProactorClientInfo.size(); i++)
    {
        CProactorClientInfo* pClientInfo = vecProactorClientInfo[i];

        if(NULL != pClientInfo)
        {
            if (false == pClientInfo->Close())
            {
                OUR_DEBUG((LM_INFO, "[CClientProConnectManager::Close]pClientInfo->Close is fail.\n"));
            }

            SAFE_DELETE(pClientInfo);
        }
    }

    m_objClientTCPList.Close();

    vector<CProactorUDPClient*> vecProactorUDPClient;
    m_objClientUDPList.Get_All_Used(vecProactorUDPClient);

    for(int i = 0; i < (int)vecProactorUDPClient.size(); i++)
    {
        CProactorUDPClient* pClientInfo = vecProactorUDPClient[i];

        if(NULL != pClientInfo)
        {
            pClientInfo->Close();
            SAFE_DELETE(pClientInfo);
        }
    }

    m_objClientUDPList.Close();
    m_u4MaxPoolCount = 0;
    m_ActiveTimer.deactivate();

}

int CClientProConnectManager::handle_timeout(const ACE_Time_Value& tv, const void* arg)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    //OUR_DEBUG((LM_DEBUG, "[CClientProConnectManager::handle_timeout]Begin.\n"));
    if(m_ProAsynchConnect.GetConnectState() == true)
    {
        return 0;
    }

    m_ThreadWritrLock.acquire();
    vector<CProactorClientInfo*> vecProactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecProactorClientInfo);
    m_ThreadWritrLock.release();

    for(int i = 0; i < (int)vecProactorClientInfo.size(); i++)
    {
        CProactorClientInfo* pClientInfo = vecProactorClientInfo[i];

        if(NULL != pClientInfo)
        {
            if(NULL == pClientInfo->GetProConnectClient())
            {
                //������Ӳ����ڣ������½�������
                if (false == pClientInfo->Run(m_blProactorFinish, SERVER_CONNECT_RECONNECT))
                {
                    OUR_DEBUG((LM_DEBUG, "[CClientProConnectManager::handle_timeout]Run is fail.\n"));
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
                    pClientInfo->GetProConnectClient()->GetTimeout(tvNow);
                }
            }
        }
    }

    //OUR_DEBUG((LM_DEBUG, "[CClientProConnectManager::handle_timeout]End.\n"));
    return 0;
}

bool CClientProConnectManager::ConnectTcpInit(int nServerID, const char* pIP, int nPort, uint8 u1IPType, const char* pLocalIP, int nLocalPort, uint8 u1LocalIPType, IClientMessage* pClientMessage, CProactorClientInfo*& pClientInfo)
{
    char szServerID[10] = { '\0' };
    sprintf_safe(szServerID, 10, "%d", nServerID);
    pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if (NULL != pClientInfo)
    {
        //�����������Ѿ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) is exist.\n", nServerID));
        return false;
    }

    //������Ѿ����ˣ�����������������
    if (m_objClientTCPList.Get_Used_Count() == m_objClientTCPList.Get_Count())
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) m_objClientTCPList is full.\n", nServerID));
        return false;
    }

    //��ʼ��������Ϣ
    pClientInfo = new CProactorClientInfo();

    if (false == pClientInfo->Init(pIP, nPort, u1IPType, nServerID, &m_ProAsynchConnect, pClientMessage))
    {
        SAFE_DELETE(pClientInfo);
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

bool CClientProConnectManager::ConnectUdpInit(int nServerID, CProactorUDPClient*& pProactorUDPClient)
{
    char szServerID[10] = { '\0' };
    sprintf_safe(szServerID, 10, "%d", nServerID);
    pProactorUDPClient = m_objClientUDPList.Get_Hash_Box_Data(szServerID);

    if (NULL != pProactorUDPClient)
    {
        //�����������Ѿ����ڣ��򲻴����µ�����
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::ConnectUDP]nServerID =(%d) is exist.\n", nServerID));
        return false;
    }

    //������Ѿ����ˣ�����������������
    if (m_objClientUDPList.Get_Used_Count() == m_objClientUDPList.Get_Count())
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::ConnectUDP]nServerID =(%d) m_objClientTCPList is full.\n", nServerID));
        return false;
    }

    pProactorUDPClient = new CProactorUDPClient();

    if (NULL == pProactorUDPClient)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::ConnectUDP]nServerID =(%d) pProactorUDPClient is NULL.\n", nServerID));
        return false;
    }

    //�����Ѿ���������ӽ�hash
    if (-1 == m_objClientUDPList.Add_Hash_Data(szServerID, pProactorUDPClient))
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::Connect]nServerID =(%d) add m_objClientTCPList is fail.\n", nServerID));
        SAFE_DELETE(pProactorUDPClient);
        return false;
    }

    return true;
}

void CClientProConnectManager::GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    VecClientConnectInfo.clear();
    vector<CProactorClientInfo*> vecProactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecProactorClientInfo);

    for(int i = 0; i < (int)vecProactorClientInfo.size(); i++)
    {
        CProactorClientInfo* pClientInfo = vecProactorClientInfo[i];

        if(NULL != pClientInfo)
        {
            if(NULL != pClientInfo->GetProConnectClient())
            {
                //�����Ѿ�����
                _ClientConnectInfo ClientConnectInfo = pClientInfo->GetProConnectClient()->GetClientConnectInfo();
                ClientConnectInfo.m_addrRemote = pClientInfo->GetServerAddr();
                VecClientConnectInfo.push_back(ClientConnectInfo);
            }
            else
            {
                //����δ����
                _ClientConnectInfo ClientConnectInfo;
                ClientConnectInfo.m_addrRemote = pClientInfo->GetServerAddr();
                ClientConnectInfo.m_blValid    = false;
                VecClientConnectInfo.push_back(ClientConnectInfo);
            }
        }
    }
}

void CClientProConnectManager::GetUDPConnectInfo(vecClientConnectInfo& VecClientConnectInfo)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    vector<CProactorUDPClient*> vecProactorUDPClient;
    m_objClientUDPList.Get_All_Used(vecProactorUDPClient);

    for(int i = 0; i < (int)vecProactorUDPClient.size(); i++)
    {
        CProactorUDPClient* pClientInfo = vecProactorUDPClient[i];

        if(NULL != pClientInfo)
        {
            _ClientConnectInfo ClientConnectInfo = pClientInfo->GetClientConnectInfo();
            VecClientConnectInfo.push_back(ClientConnectInfo);
        }
    }

}

bool CClientProConnectManager::CloseByClient(int nServerID)
{
    //�������ΪԶ�̿ͻ��˶Ͽ�����ֻɾ��ProConnectClient��ָ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CClientProConnectManager::CloseByClient]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    pClientInfo->SetProConnectClient(NULL);
    pClientInfo->SetServerConnectState(SERVER_CONNECT_FAIL);

    return true;
}

EM_Server_Connect_State CClientProConnectManager::GetConnectState(int nServerID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        //���������Ӳ����ڣ��򲻴����µ�����
        return SERVER_CONNECT_FAIL;
    }
    else
    {
        return pClientInfo->GetServerConnectState();
    }
}

bool CClientProConnectManager::ReConnect(int nServerID)
{
    //��鵱ǰ�����Ƿ��ǻ�Ծ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL == pClientInfo)
    {
        //OUR_DEBUG((LM_ERROR, "[GetConnectState::Close]nServerID =(%d) pClientInfo is NULL.\n", nServerID));
        return false;
    }

    if(NULL == pClientInfo->GetProConnectClient())
    {
        //������Ӳ����ڣ������½�������
        if (false == pClientInfo->Run(m_blProactorFinish, SERVER_CONNECT_RECONNECT))
        {
            OUR_DEBUG((LM_INFO, "[CClientProConnectManager::ReConnect]Run is fail.\n"));
            return false;
        }

        return true;
    }
    else
    {
        return true;
    }
}

ACE_INET_Addr CClientProConnectManager::GetServerAddr(int nServerID)
{
    ACE_INET_Addr remote_addr;
    //��鵱ǰ�����Ƿ��ǻ�Ծ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL != pClientInfo)
    {
        remote_addr = pClientInfo->GetServerAddr();
        return remote_addr;
    }
    else
    {
        return remote_addr;
    }
}

bool CClientProConnectManager::SetServerConnectState(int nServerID, EM_Server_Connect_State objState)
{
    //��鵱ǰ�����Ƿ��ǻ�Ծ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL != pClientInfo)
    {
        pClientInfo->SetServerConnectState(objState);
        return true;
    }
    else
    {
        return false;
    }
}

bool CClientProConnectManager::GetServerIPInfo(int nServerID, _ClientIPInfo& objServerIPInfo)
{
    //��鵱ǰ�����Ƿ��ǻ�Ծ��
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);
    char szServerID[10] = {'\0'};
    sprintf_safe(szServerID, 10, "%d", nServerID);

    CProactorClientInfo* pClientInfo = m_objClientTCPList.Get_Hash_Box_Data(szServerID);

    if(NULL != pClientInfo)
    {
        ACE_INET_Addr remote_addr = pClientInfo->GetServerAddr();
        sprintf_safe(objServerIPInfo.m_szClientIP, MAX_BUFF_50, remote_addr.get_host_addr());
        objServerIPInfo.m_nPort = remote_addr.get_port_number();
        return true;
    }
    else
    {
        return  false;
    }
}

bool CClientProConnectManager::DeleteIClientMessage(IClientMessage* pClientMessage)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_ThreadWritrLock);

    //���첽�ص���Ч�����д�pClientMessage����Ϊ��Ч
    App_ServerMessageTask::instance()->DelClientMessage(pClientMessage);

    //һһѰ����֮��Ӧ�������Լ������Ϣ��ɾ��֮
    vector<CProactorClientInfo*> vecProactorClientInfo;
    m_objClientTCPList.Get_All_Used(vecProactorClientInfo);

    for(int i = 0; i < (int)vecProactorClientInfo.size(); i++)
    {
        CProactorClientInfo* pClientInfo = vecProactorClientInfo[i];

        if(NULL != pClientInfo && pClientInfo->GetClientMessage() == pClientMessage)
        {
            //�ر����ӣ���ɾ������
            //�ر����Ӷ���
            if (NULL != pClientInfo->GetClientMessage())
            {
                EM_s2s ems2s = S2S_INNEED_CALLBACK;
                pClientInfo->GetProConnectClient()->ClientClose(ems2s);
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
