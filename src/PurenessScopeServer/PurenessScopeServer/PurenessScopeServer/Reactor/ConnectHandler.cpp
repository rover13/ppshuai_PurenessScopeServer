#include "ConnectHandler.h"

CConnectHandler::CConnectHandler(void)
{
    m_szError[0]          = '\0';
    m_u4ConnectID         = 0;
    m_u2SendCount         = 0;
    m_u4AllRecvCount      = 0;
    m_u4AllSendCount      = 0;
    m_u4AllRecvSize       = 0;
    m_u4AllSendSize       = 0;
    m_u4SendThresHold     = MAX_MSG_SNEDTHRESHOLD;
    m_u2SendQueueMax      = MAX_MSG_SENDPACKET;
    m_u1ConnectState      = CONNECT_INIT;
    m_u1SendBuffState     = CONNECT_SENDNON;
    m_pCurrMessage        = NULL;
    m_pBlockMessage       = NULL;
    m_pPacketParse        = NULL;
    m_u4CurrSize          = 0;
    m_u4HandlerID         = 0;
    m_u2MaxConnectTime    = 0;
    m_u1IsActive          = 0;
    m_u4MaxPacketSize     = MAX_MSG_PACKETLENGTH;
    m_blBlockState        = false;
    m_nBlockCount         = 0;
    m_nBlockSize          = MAX_BLOCK_SIZE;
    m_nBlockMaxCount      = MAX_BLOCK_COUNT;
    m_u8RecvQueueTimeCost = 0;
    m_u4RecvQueueCount    = 0;
    m_u8SendQueueTimeCost = 0;
    m_u4ReadSendSize      = 0;
    m_u4SuccessSendSize   = 0;
    m_nHashID             = 0;
    m_u8SendQueueTimeout  = MAX_QUEUE_TIMEOUT * 1000 * 1000;  //Ŀǰ��Ϊ��¼��������
    m_u8RecvQueueTimeout  = MAX_QUEUE_TIMEOUT * 1000 * 1000;  //Ŀǰ��Ϊ��¼��������
    m_u2TcpNodelay        = TCP_NODELAY_ON;
    m_emStatus            = CLIENT_CLOSE_NOTHING;
    m_u4SendMaxBuffSize   = 5*MAX_BUFF_1024;
    m_szLocalIP[0]        = '\0';
    m_szConnectName[0]    = '\0';
    m_blIsLog             = false;
    m_u4PacketParseInfoID = 0;
    m_u4PacketDebugSize   = 0;
    m_pPacketDebugData    = NULL;
    m_emIOType            = NET_INPUT;
    m_pFileTest           = NULL;
    m_u4SendCheckTime     = 0;
    m_u4LocalPort         = 0;
}

CConnectHandler::~CConnectHandler(void)
{
    this->closing_ = true;
    //OUR_DEBUG((LM_INFO, "[CConnectHandler::~CConnectHandler].\n"));
    //OUR_DEBUG((LM_INFO, "[CConnectHandler::~CConnectHandler]End.\n"));
    SAFE_DELETE(m_pPacketDebugData);
    m_pPacketDebugData  = NULL;
    m_u4PacketDebugSize = 0;
}

const char* CConnectHandler::GetError()
{
    return m_szError;
}

void CConnectHandler::Close()
{
    //�������ӶϿ���Ϣ
    App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->DisConnect(GetConnectID());

    //��֯����
    _MakePacket objMakePacket;

    objMakePacket.m_u4ConnectID       = GetConnectID();
    objMakePacket.m_pPacketParse      = NULL;
    objMakePacket.m_AddrRemote        = m_addrRemote;

    if (ACE_OS::strcmp("INADDR_ANY", m_szLocalIP) == 0)
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort);
    }
    else
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort, m_szLocalIP);
    }

    if (CONNECT_SERVER_CLOSE == m_u1ConnectState)
    {
        //�����������Ͽ�
        objMakePacket.m_u1Option = PACKET_SDISCONNECT;
    }
    else
    {
        //�ͻ������ӶϿ�
        objMakePacket.m_u1Option = PACKET_CDISCONNECT;
    }

    //���Ϳͻ������ӶϿ���Ϣ��
    ACE_Time_Value tvNow = ACE_OS::gettimeofday();

    if(false == App_MakePacket::instance()->PutMessageBlock(&objMakePacket, tvNow))
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::Close] ConnectID = %d, PACKET_CONNECT is error.\n", GetConnectID()));
    }

    //msg_queue()->deactivate();
    shutdown();
    AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %dws, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %dws.",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, (uint32)m_u8RecvQueueTimeCost, m_u4RecvQueueCount, (uint32)m_u8SendQueueTimeCost);

    //�������PacketParse����
    ClearPacketParse();

    OUR_DEBUG((LM_ERROR, "[CConnectHandler::Close](0x%08x)Close(ConnectID=%d) OK.\n", this, GetConnectID()));

    //ɾ�����Ӷ���
    App_ConnectManager::instance()->CloseConnectByClient(GetConnectID());

    m_u4ConnectID = 0;

    //�ع��ù���ָ��
    App_ConnectHandlerPool::instance()->Delete(this);
}

void CConnectHandler::Init(uint16 u2HandlerID)
{
    m_u4HandlerID      = u2HandlerID;
    m_u2MaxConnectTime = App_MainConfig::instance()->GetMaxConnectTime();
    m_u4SendThresHold  = App_MainConfig::instance()->GetSendTimeout();
    m_u2SendQueueMax   = App_MainConfig::instance()->GetSendQueueMax();
    m_u4MaxPacketSize  = App_MainConfig::instance()->GetRecvBuffSize();
    m_u2TcpNodelay     = App_MainConfig::instance()->GetTcpNodelay();

    m_u8SendQueueTimeout = (uint64)(App_MainConfig::instance()->GetSendQueueTimeout()) * 1000 * 1000;

    if(m_u8SendQueueTimeout == 0)
    {
        m_u8SendQueueTimeout = MAX_QUEUE_TIMEOUT * 1000 * 1000;
    }

    m_u8RecvQueueTimeout = (uint64)(App_MainConfig::instance()->GetRecvQueueTimeout()) * 1000 * 1000;

    if(m_u8RecvQueueTimeout == 0)
    {
        m_u8RecvQueueTimeout = MAX_QUEUE_TIMEOUT * 1000 * 1000;
    }

    m_u4SendMaxBuffSize  = App_MainConfig::instance()->GetBlockSize();
    //m_pBlockMessage      = new ACE_Message_Block(m_u4SendMaxBuffSize);
    m_pBlockMessage      = NULL;
    m_emStatus           = CLIENT_CLOSE_NOTHING;

    m_pPacketDebugData   = new char[App_MainConfig::instance()->GetDebugSize()];
    m_u4PacketDebugSize  = App_MainConfig::instance()->GetDebugSize() / 5;
}

void CConnectHandler::SetPacketParseInfoID(uint32 u4PacketParseInfoID)
{
    m_u4PacketParseInfoID = u4PacketParseInfoID;
}

uint32 CConnectHandler::GetPacketParseInfoID()
{
    return m_u4PacketParseInfoID;
}

uint32 CConnectHandler::GetHandlerID()
{
    return m_u4HandlerID;
}

void CConnectHandler::SetConnectID(uint32 u4ConnectID)
{
    m_u4ConnectID = u4ConnectID;
}

uint32 CConnectHandler::GetConnectID()
{
    return m_u4ConnectID;
}

int CConnectHandler::open(void*)
{
    m_blBlockState        = false;
    m_nBlockCount         = 0;
    m_u8SendQueueTimeCost = 0;
    m_blIsLog             = false;
    m_szConnectName[0]    = '\0';
    m_u1IsActive          = 1;

    //���û�����
    //m_pBlockMessage->reset();

    if (NULL == App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID))
    {
        //��������������ڣ���ֱ�ӶϿ�����
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open](%s)can't find PacketParseInfo.\n", m_addrRemote.get_host_addr()));
        return -1;
    }

    //���Զ�����ӵ�ַ�Ͷ˿�
    if(this->peer().get_remote_addr(m_addrRemote) == -1)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]this->peer().get_remote_addr error.\n"));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::open]this->peer().get_remote_addr error.");
        return -1;
    }

    if(App_ForbiddenIP::instance()->CheckIP(m_addrRemote.get_host_addr()) == false)
    {
        //�ڽ�ֹ�б��У����������
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]IP Forbidden(%s).\n", m_addrRemote.get_host_addr()));
        return -1;
    }

    //��鵥λʱ�����Ӵ����Ƿ�ﵽ����
    if(false == App_IPAccount::instance()->AddIP((string)m_addrRemote.get_host_addr()))
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]IP connect frequently.\n", m_addrRemote.get_host_addr()));
        App_ForbiddenIP::instance()->AddTempIP(m_addrRemote.get_host_addr(), App_MainConfig::instance()->GetIPAlert()->m_u4IPTimeout);

        //���͸澯�ʼ�
        AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECT,
                                               App_MainConfig::instance()->GetIPAlert()->m_u4MailID,
                                               (char* )"Alert IP",
                                               "[CConnectHandler::open] IP is more than IP Max,");

        return -1;
    }

    //��ʼ�������
    m_TimeConnectInfo.Init(App_MainConfig::instance()->GetClientDataAlert()->m_u4RecvPacketCount,
                           App_MainConfig::instance()->GetClientDataAlert()->m_u4RecvDataMax,
                           App_MainConfig::instance()->GetClientDataAlert()->m_u4SendPacketCount,
                           App_MainConfig::instance()->GetClientDataAlert()->m_u4SendDataMax);

    int nRet = 0;

    //��������Ϊ������ģʽ
    if (this->peer().enable(ACE_NONBLOCK) == -1)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]this->peer().enable  = ACE_NONBLOCK error.\n"));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::open]this->peer().enable  = ACE_NONBLOCK error.");
        return -1;
    }

    //����Ĭ�ϱ���
    SetConnectName(m_addrRemote.get_host_addr());
    //OUR_DEBUG((LM_INFO, "[CConnectHandler::open]Connection from [%s:%d]\n",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number()));

    //��ʼ����ǰ���ӵ�ĳЩ����
    m_atvConnect          = ACE_OS::gettimeofday();
    m_atvInput            = ACE_OS::gettimeofday();
    m_atvOutput           = ACE_OS::gettimeofday();
    m_atvSendAlive        = ACE_OS::gettimeofday();

    m_u4AllRecvCount      = 0;
    m_u4AllSendCount      = 0;
    m_u4AllRecvSize       = 0;
    m_u4AllSendSize       = 0;
    m_u8RecvQueueTimeCost = 0;
    m_u4RecvQueueCount    = 0;
    m_u8SendQueueTimeCost = 0;
    m_u4CurrSize          = 0;

    m_u4ReadSendSize      = 0;
    m_u4SuccessSendSize   = 0;
    m_emStatus            = CLIENT_CLOSE_NOTHING;

    //���ý��ջ���صĴ�С
    int nTecvBuffSize = MAX_MSG_SOCKETBUFF;
    //ACE_OS::setsockopt(this->get_handle(), SOL_SOCKET, SO_RCVBUF, (char* )&nTecvBuffSize, sizeof(nTecvBuffSize));
    ACE_OS::setsockopt(this->get_handle(), SOL_SOCKET, SO_SNDBUF, (char* )&nTecvBuffSize, sizeof(nTecvBuffSize));

    if(m_u2TcpNodelay == TCP_NODELAY_OFF)
    {
        //��������˽���Nagle�㷨��������Ҫ���á�
        int nOpt=1;
        ACE_OS::setsockopt(this->get_handle(), IPPROTO_TCP, TCP_NODELAY, (char* )&nOpt, sizeof(int));
    }

    //int nOverTime = MAX_MSG_SENDTIMEOUT;
    //ACE_OS::setsockopt(this->get_handle(), SOL_SOCKET, SO_SNDTIMEO, (char* )&nOverTime, sizeof(nOverTime));

    m_pPacketParse = App_PacketParsePool::instance()->Create();

    if(NULL == m_pPacketParse)
    {
        OUR_DEBUG((LM_DEBUG,"[CConnectHandler::open]Open(%d) m_pPacketParse new error.\n", GetConnectID()));
        return -1;
    }

    //����ͷ�Ĵ�С��Ӧ��mb
    if(App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u1PacketParseType == PACKET_WITHHEAD)
    {
        m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u4OrgLength);
    }
    else
    {
        m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_MainConfig::instance()->GetServerRecvBuff());
    }

    if(m_pCurrMessage == NULL)
    {
        //AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %d, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %d.",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, m_u8RecvQueueTimeCost, m_u4RecvQueueCount, m_u8SendQueueTimeCost);
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]pmb new is NULL.\n"));
        return -1;
    }

    //��������ӷ������ӿ�
    if(false == App_ConnectManager::instance()->AddConnect(this))
    {
        OUR_DEBUG((LM_ERROR, "%s.\n", App_ConnectManager::instance()->GetError()));
        sprintf_safe(m_szError, MAX_BUFF_500, "%s", App_ConnectManager::instance()->GetError());
        return -1;
    }

    AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Connection from [%s:%d].",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number());

    //����PacketParse����Ӧ����
    App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Connect(GetConnectID(), GetClientIPInfo(), GetLocalIPInfo());

    //��֯����
    _MakePacket objMakePacket;

    objMakePacket.m_u4ConnectID       = GetConnectID();
    objMakePacket.m_pPacketParse      = NULL;
    objMakePacket.m_u1Option          = PACKET_CONNECT;
    objMakePacket.m_AddrRemote        = m_addrRemote;

    if (ACE_OS::strcmp("INADDR_ANY", m_szLocalIP) == 0)
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort);
    }
    else
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort, m_szLocalIP);
    }

    //�������ӽ�����Ϣ��
    ACE_Time_Value tvNow = ACE_OS::gettimeofday();

    if(false == App_MakePacket::instance()->PutMessageBlock(&objMakePacket, tvNow))
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open] ConnectID=%d, PACKET_CONNECT is error.\n", GetConnectID()));
    }

    OUR_DEBUG((LM_DEBUG,"[CConnectHandler::open]Open(%d) Connection from [%s:%d](0x%08x).\n", GetConnectID(), m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), this));

    m_u1ConnectState = CONNECT_OPEN;

    nRet = this->reactor()->register_handler(this, ACE_Event_Handler::READ_MASK|ACE_Event_Handler::WRITE_MASK);
    //OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]ConnectID=%d, nRet=%d.\n", GetConnectID(), nRet));

    int nWakeupRet = reactor()->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);

    if (-1 == nWakeupRet)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]ConnectID=%d, nWakeupRet=%d, errno=%d.\n", GetConnectID(), nWakeupRet, errno));
    }

    m_emIOType = NET_INPUT;
    return nRet;
}

//��������
int CConnectHandler::handle_input(ACE_HANDLE fd)
{
    m_atvInput = ACE_OS::gettimeofday();

    if(fd == ACE_INVALID_HANDLE)
    {
        m_u4CurrSize = 0;
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::handle_input]fd == ACE_INVALID_HANDLE.\n"));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::handle_input]fd == ACE_INVALID_HANDLE.");

        return -1;
    }

    //�ж����ݰ��ṹ�Ƿ�ΪNULL
    if(m_pPacketParse == NULL)
    {
        m_u4CurrSize = 0;
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::handle_input]ConnectID=%d, m_pPacketParse == NULL.\n", GetConnectID()));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::handle_input]m_pPacketParse == NULL.");

        return -1;
    }

    //��л���ʯ�Ľ���
    //���￼�Ǵ����etģʽ��֧��
    if(App_MainConfig::instance()->GetNetworkMode() != (uint8)NETWORKMODE_RE_EPOLL_ET)
    {
        return RecvData();
    }
    else
    {
        return RecvData_et();
    }
}

int CConnectHandler::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
    if (fd == ACE_INVALID_HANDLE)
    {
        m_u4CurrSize = 0;
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::handle_output]fd == ACE_INVALID_HANDLE.\n"));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::handle_output]fd == ACE_INVALID_HANDLE.");

        return -1;
    }

    ACE_Message_Block* pmbSendData = NULL;
    ACE_Time_Value nowait(ACE_OS::gettimeofday());

    while (-1 != this->getq(pmbSendData, &nowait))
    {
        if (NULL == pmbSendData)
        {
            OUR_DEBUG((LM_INFO, "[CConnectHandler::handle_output]ConnectID=%d pmbSendData is NULL.\n", GetConnectID()));
            break;
        }

        //����ǶϿ�ָ���ִ�����ӶϿ�����
        if (pmbSendData->msg_type() == ACE_Message_Block::MB_STOP)
        {
            App_MessageBlockManager::instance()->Close(pmbSendData);
            return -1;
        }

        uint32 u4SendSuc = (uint32)pmbSendData->length();
        bool blRet = PutSendPacket(pmbSendData);

        if (true == blRet)
        {
            if (m_u4ReadSendSize >= m_u4SuccessSendSize + u4SendSuc)
            {
                //OUR_DEBUG((LM_INFO, "[CConnectHandler::handle_output]ConnectID=%d, m_u4SuccessSendSize=%d, u4SendSuc=%d.\n", GetConnectID(), m_u4SuccessSendSize, u4SendSuc));
                //��¼�ɹ������ֽ�
                m_u4SuccessSendSize += u4SendSuc;
            }
        }
        else
        {
            OUR_DEBUG((LM_INFO, "[CConnectHandler::handle_output]ConnectID=%d write is close.\n", GetConnectID()));
            return -1;
        }

        //OUR_DEBUG((LM_INFO, "[CConnectHandler::handle_output]ConnectID=%d send finish.\n", GetConnectID()));
    }

    int nWakeupRet =  reactor()->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);

    if (-1 == nWakeupRet)
    {
        OUR_DEBUG((LM_INFO, "[CConnectHandler::handle_output]ConnectID=%d,nWakeupRet=%d, errno=%d.\n", GetConnectID(), nWakeupRet, errno));
    }

    //OUR_DEBUG((LM_INFO, "[CConnectHandler::handle_output]ConnectID=%d,cancel_wakeup.\n", GetConnectID()));
    return 0;
}

//����������ݴ���
int CConnectHandler::RecvData()
{
    return Dispose_Recv_Data();
}

//etģʽ��������
int CConnectHandler::RecvData_et()
{
    int nRet = 0;

    while(true)
    {
        //�жϻ����Ƿ�ΪNULL
        nRet = Dispose_Recv_Data();

        if (0 != nRet)
        {
            break;
        }
    }

    return nRet;
}

int CConnectHandler::Dispose_Recv_Data()
{
    ACE_Time_Value nowait(0, MAX_QUEUE_TIMEOUT);

    //�жϻ����Ƿ�ΪNULL
    if (m_pCurrMessage == NULL)
    {
        m_u4CurrSize = 0;
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::RecvData]m_pCurrMessage == NULL.\n"));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::RecvData]m_pCurrMessage == NULL.");

        return -1;
    }

    //����Ӧ�ý��յ����ݳ���
    int nCurrCount = 0;

    if (App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u1PacketParseType == PACKET_WITHHEAD)
    {
        if (m_pPacketParse->GetIsHandleHead())
        {
            nCurrCount = (uint32)App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u4OrgLength - (uint32)m_pCurrMessage->length();
        }
        else
        {
            nCurrCount = (uint32)m_pPacketParse->GetPacketBodySrcLen() - (uint32)m_pCurrMessage->length();
        }
    }
    else
    {
        nCurrCount = (uint32)App_MainConfig::instance()->GetServerRecvBuff() - (uint32)m_pCurrMessage->length();
    }

    //������Ҫ��m_u4CurrSize���м�顣
    if (nCurrCount < 0)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::RecvData][%d] nCurrCount < 0 m_u4CurrSize = %d.\n", GetConnectID(), m_u4CurrSize));
        m_u4CurrSize = 0;

        return -1;
    }

    int nDataLen = (int)this->peer().recv(m_pCurrMessage->wr_ptr(), nCurrCount, MSG_NOSIGNAL, &nowait);

    if (nDataLen <= 0)
    {
        m_u4CurrSize = 0;
        uint32 u4Error = (uint32)errno;

        //�����-1 ��Ϊ11�Ĵ��󣬺���֮
        if (nDataLen == -1 && u4Error == EAGAIN)
        {
            return 0;
        }

        OUR_DEBUG((LM_ERROR, "[CConnectHandler::RecvData] ConnectID=%d, recv data is error nDataLen = [%d] errno = [%d].\n", GetConnectID(), nDataLen, u4Error));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::RecvData] ConnectID = %d, recv data is error[%d].\n", GetConnectID(), nDataLen);

        return -1;
    }

    //�����DEBUG״̬����¼��ǰ���ܰ��Ķ���������
    if (App_MainConfig::instance()->GetDebug() == DEBUG_ON || m_blIsLog == true)
    {
        char szLog[10] = { '\0' };
        uint32 u4DebugSize = 0;
        bool blblMore = false;

        if ((uint32)nDataLen >= m_u4PacketDebugSize)
        {
            u4DebugSize = m_u4PacketDebugSize - 1;
            blblMore = true;
        }
        else
        {
            u4DebugSize = (uint32)nDataLen;
        }

        char* pData = m_pCurrMessage->wr_ptr();

        for (uint32 i = 0; i < u4DebugSize; i++)
        {
            sprintf_safe(szLog, 10, "0x%02X ", (unsigned char)pData[i]);
            sprintf_safe(m_pPacketDebugData + 5 * i, MAX_BUFF_1024 - 5 * i, "%s", szLog);
        }

        m_pPacketDebugData[5 * u4DebugSize] = '\0';

        if (blblMore == true)
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_DEBUG_CLIENTRECV, "[(%s)%s:%d]%s.(���ݰ�����ֻ��¼ǰ200�ֽ�)", m_szConnectName, m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_pPacketDebugData);
        }
        else
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_DEBUG_CLIENTRECV, "[(%s)%s:%d]%s.", m_szConnectName, m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_pPacketDebugData);
        }
    }

    m_u4CurrSize += nDataLen;

    m_pCurrMessage->wr_ptr(nDataLen);

    if (App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u1PacketParseType == PACKET_WITHHEAD)
    {
        //���û�ж��꣬�̶�
        if (nCurrCount - nDataLen > 0)
        {
            //û���꣬������
            return 0;
        }

        if (m_pCurrMessage->length() == App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u4OrgLength && m_pPacketParse->GetIsHandleHead())
        {
            _Head_Info objHeadInfo;
            bool blStateHead = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Parse_Packet_Head_Info(GetConnectID(), m_pCurrMessage, App_MessageBlockManager::instance(), &objHeadInfo);

            if (false == blStateHead)
            {
                m_u4CurrSize = 0;
                OUR_DEBUG((LM_ERROR, "[CConnectHandler::RecvData]SetPacketHead is false.\n"));

                return -1;
            }
            else
            {
                if (NULL == objHeadInfo.m_pmbHead)
                {
                    OUR_DEBUG((LM_ERROR, "[CConnectHandler::RecvData]ConnectID=%d, objHeadInfo.m_pmbHead is NULL.\n", GetConnectID()));
                }

                m_pPacketParse->SetPacket_IsHandleHead(false);
                m_pPacketParse->SetPacket_Head_Message(objHeadInfo.m_pmbHead);
                m_pPacketParse->SetPacket_Head_Curr_Length(objHeadInfo.m_u4HeadCurrLen);
                m_pPacketParse->SetPacket_Body_Src_Length(objHeadInfo.m_u4BodySrcLen);
                m_pPacketParse->SetPacket_CommandID(objHeadInfo.m_u2PacketCommandID);
            }

            uint32 u4PacketBodyLen = m_pPacketParse->GetPacketBodySrcLen();
            m_u4CurrSize = 0;

            //�������ֻ�����ͷ������
            //�������ֻ�а�ͷ������Ҫ���壬�����������һЩ����������ֻ�����ͷ���ӵ�DoMessage()
            if (u4PacketBodyLen == 0)
            {
                //ֻ�����ݰ�ͷ
                if (false == CheckMessage())
                {
                    return -1;
                }

                m_u4CurrSize = 0;

                //�����µİ�
                m_pPacketParse = App_PacketParsePool::instance()->Create();

                if (NULL == m_pPacketParse)
                {
                    OUR_DEBUG((LM_DEBUG, "[%t|CConnectHandle::RecvData] Open(%d) m_pPacketParse new error.\n", GetConnectID()));
                    return -1;
                }

                //����ͷ�Ĵ�С��Ӧ��mb
                m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u4OrgLength);

                if (m_pCurrMessage == NULL)
                {
                    AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %dws, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %dws.", m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, (uint32)m_u8RecvQueueTimeCost, m_u4RecvQueueCount, (uint32)m_u8SendQueueTimeCost);
                    OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvData] pmb new is NULL.\n"));
                    return -1;
                }
            }
            else
            {
                //����������������ȣ�Ϊ�Ƿ�����
                if (u4PacketBodyLen >= m_u4MaxPacketSize)
                {
                    m_u4CurrSize = 0;
                    OUR_DEBUG((LM_ERROR, "[CConnectHandler::RecvData]u4PacketHeadLen(%d) more than %d.\n", u4PacketBodyLen, m_u4MaxPacketSize));

                    return -1;
                }
                else
                {
                    //OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvClinetPacket] m_pPacketParse->GetPacketBodyLen())=%d.\n", m_pPacketParse->GetPacketBodyLen()));
                    //����ͷ�Ĵ�С��Ӧ��mb
                    m_pCurrMessage = App_MessageBlockManager::instance()->Create(u4PacketBodyLen);

                    if (m_pCurrMessage == NULL)
                    {
                        m_u4CurrSize = 0;
                        //AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %d, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %d.",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, m_u8RecvQueueTimeCost, m_u4RecvQueueCount, m_u8SendQueueTimeCost);
                        OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvData] pmb new is NULL.\n"));

                        return -1;
                    }
                }
            }

        }
        else
        {
            //��������������ɣ���ʼ�����������ݰ�
            _Body_Info obj_Body_Info;
            bool blStateBody = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Parse_Packet_Body_Info(GetConnectID(), m_pCurrMessage, App_MessageBlockManager::instance(), &obj_Body_Info);

            if (false == blStateBody)
            {
                //������ݰ����Ǵ���ģ���Ͽ�����
                m_u4CurrSize = 0;
                OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvData]SetPacketBody is false.\n"));

                return -1;
            }
            else
            {
                m_pPacketParse->SetPacket_Body_Message(obj_Body_Info.m_pmbBody);
                m_pPacketParse->SetPacket_Body_Curr_Length(obj_Body_Info.m_u4BodyCurrLen);

                if (obj_Body_Info.m_u2PacketCommandID > 0)
                {
                    m_pPacketParse->SetPacket_CommandID(obj_Body_Info.m_u2PacketCommandID);
                }
            }

            if (false == CheckMessage())
            {
                return -1;
            }

            m_u4CurrSize = 0;

            //�����µİ�
            m_pPacketParse = App_PacketParsePool::instance()->Create();

            if (NULL == m_pPacketParse)
            {
                OUR_DEBUG((LM_DEBUG, "[%t|CConnectHandle::RecvData] Open(%d) m_pPacketParse new error.\n", GetConnectID()));
                return -1;
            }

            //����ͷ�Ĵ�С��Ӧ��mb
            m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u4OrgLength);

            if (m_pCurrMessage == NULL)
            {
                AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %dws, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %dws.", m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, (uint32)m_u8RecvQueueTimeCost, m_u4RecvQueueCount, (uint32)m_u8SendQueueTimeCost);
                OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvData] pmb new is NULL.\n"));
                return -1;
            }
        }
    }
    else
    {
        //����ģʽ����
        while (true)
        {
            _Packet_Info obj_Packet_Info;
            uint8 n1Ret = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Parse_Packet_Stream(GetConnectID(), m_pCurrMessage, dynamic_cast<IMessageBlockManager*>(App_MessageBlockManager::instance()), &obj_Packet_Info);

            if (PACKET_GET_ENOUGTH == n1Ret)
            {
                m_pPacketParse->SetPacket_Head_Message(obj_Packet_Info.m_pmbHead);
                m_pPacketParse->SetPacket_Body_Message(obj_Packet_Info.m_pmbBody);
                m_pPacketParse->SetPacket_CommandID(obj_Packet_Info.m_u2PacketCommandID);
                m_pPacketParse->SetPacket_Head_Src_Length(obj_Packet_Info.m_u4HeadSrcLen);
                m_pPacketParse->SetPacket_Head_Curr_Length(obj_Packet_Info.m_u4HeadCurrLen);
                m_pPacketParse->SetPacket_Head_Src_Length(obj_Packet_Info.m_u4BodySrcLen);
                m_pPacketParse->SetPacket_Body_Curr_Length(obj_Packet_Info.m_u4BodyCurrLen);

                if (false == CheckMessage())
                {
                    return -1;
                }

                m_u4CurrSize = 0;

                //�����µİ�
                m_pPacketParse = App_PacketParsePool::instance()->Create();

                if (NULL == m_pPacketParse)
                {
                    OUR_DEBUG((LM_DEBUG, "[%t|CConnectHandle::RecvData] Open(%d) m_pPacketParse new error.\n", GetConnectID()));
                    return -1;
                }

                //�����Ƿ���������
                if (m_pCurrMessage->length() == 0)
                {
                    break;
                }
                else
                {
                    //�������ݣ���������
                    continue;
                }
            }
            else if (PACKET_GET_NO_ENOUGTH == n1Ret)
            {
                break;
            }
            else
            {
                m_pPacketParse->Clear();

                AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %dws, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %dws.", m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, (uint32)m_u8RecvQueueTimeCost, m_u4RecvQueueCount, (uint32)m_u8SendQueueTimeCost);
                OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvData] pmb new is NULL.\n"));
                return -1;
            }
        }

        App_MessageBlockManager::instance()->Close(m_pCurrMessage);
        m_u4CurrSize = 0;

        //����ͷ�Ĵ�С��Ӧ��mb
        m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_MainConfig::instance()->GetServerRecvBuff());

        if (m_pCurrMessage == NULL)
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %dws, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %dws.", m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, (uint32)m_u8RecvQueueTimeCost, m_u4RecvQueueCount, (uint32)m_u8SendQueueTimeCost);
            OUR_DEBUG((LM_ERROR, "[CConnectHandle::RecvData] pmb new is NULL.\n"));
            return -1;
        }
    }

    return 0;
}

//�ر�����
int CConnectHandler::handle_close(ACE_HANDLE h, ACE_Reactor_Mask mask)
{
    if(h == ACE_INVALID_HANDLE)
    {
        OUR_DEBUG((LM_DEBUG,"[CConnectHandler::handle_close] h is NULL mask=%d.\n", (int)mask));
    }

    //���÷�����Ϣ���в����ٷ����κ���Ϣ
    if(m_u1ConnectState != CONNECT_SERVER_CLOSE)
    {
        m_u1ConnectState = CONNECT_CLIENT_CLOSE;
    }

    //��ȡ���Ͷ����ڲ����������ݣ����Ϊ��Ч�������ڴ�
    ACE_Message_Block* pmbSendData = NULL;
    ACE_Time_Value nowait(ACE_OS::gettimeofday());

    while (-1 != this->getq(pmbSendData, &nowait))
    {
        App_MessageBlockManager::instance()->Close(pmbSendData);
    }

    Close();

    return 0;
}

uint32 CConnectHandler::file_open(IFileTestManager* pFileTest)
{
    m_blBlockState = false;
    m_nBlockCount = 0;
    m_u8SendQueueTimeCost = 0;
    m_blIsLog = false;
    m_szConnectName[0] = '\0';
    m_u1IsActive = 1;

    //���û�����
    //m_pBlockMessage->reset();

    if (NULL == App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID))
    {
        //��������������ڣ���ֱ�ӶϿ�����
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open](%s)can't find PacketParseInfo.\n", m_addrRemote.get_host_addr()));
        return -1;
    }

    //��ʼ�������
    m_TimeConnectInfo.Init(App_MainConfig::instance()->GetClientDataAlert()->m_u4RecvPacketCount,
                           App_MainConfig::instance()->GetClientDataAlert()->m_u4RecvDataMax,
                           App_MainConfig::instance()->GetClientDataAlert()->m_u4SendPacketCount,
                           App_MainConfig::instance()->GetClientDataAlert()->m_u4SendDataMax);

    /*
    int nRet = ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::open();
    if(nRet != 0)
    {
    OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::open() error [%d].\n", nRet));
    sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::open]ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::open() error [%d].", nRet);
    return -1;
    }
    */

    //��������Ϊ������ģʽ
    if (this->peer().enable(ACE_NONBLOCK) == -1)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]this->peer().enable  = ACE_NONBLOCK error.\n"));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::open]this->peer().enable  = ACE_NONBLOCK error.");
        return 1;
    }

    //����Ĭ�ϱ���
    SetConnectName(m_addrRemote.get_host_addr());
    //OUR_DEBUG((LM_INFO, "[CConnectHandler::open]Connection from [%s:%d]\n",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number()));

    //��ʼ����ǰ���ӵ�ĳЩ����
    m_atvConnect = ACE_OS::gettimeofday();
    m_atvInput = ACE_OS::gettimeofday();
    m_atvOutput = ACE_OS::gettimeofday();
    m_atvSendAlive = ACE_OS::gettimeofday();

    m_u4AllRecvCount = 0;
    m_u4AllSendCount = 0;
    m_u4AllRecvSize = 0;
    m_u4AllSendSize = 0;
    m_u8RecvQueueTimeCost = 0;
    m_u4RecvQueueCount = 0;
    m_u8SendQueueTimeCost = 0;
    m_u4CurrSize = 0;

    m_u4ReadSendSize = 0;
    m_u4SuccessSendSize = 0;
    m_emStatus = CLIENT_CLOSE_NOTHING;

    //���ý��ջ���صĴ�С
    int nTecvBuffSize = MAX_MSG_SOCKETBUFF;
    //ACE_OS::setsockopt(this->get_handle(), SOL_SOCKET, SO_RCVBUF, (char* )&nTecvBuffSize, sizeof(nTecvBuffSize));
    ACE_OS::setsockopt(this->get_handle(), SOL_SOCKET, SO_SNDBUF, (char*)&nTecvBuffSize, sizeof(nTecvBuffSize));

    if (m_u2TcpNodelay == TCP_NODELAY_OFF)
    {
        //��������˽���Nagle�㷨��������Ҫ���á�
        int nOpt = 1;
        ACE_OS::setsockopt(this->get_handle(), IPPROTO_TCP, TCP_NODELAY, (char*)&nOpt, sizeof(int));
    }

    //int nOverTime = MAX_MSG_SENDTIMEOUT;
    //ACE_OS::setsockopt(this->get_handle(), SOL_SOCKET, SO_SNDTIMEO, (char* )&nOverTime, sizeof(nOverTime));

    m_pPacketParse = App_PacketParsePool::instance()->Create();

    if (NULL == m_pPacketParse)
    {
        OUR_DEBUG((LM_DEBUG, "[CConnectHandler::open]Open(%d) m_pPacketParse new error.\n", GetConnectID()));
        return 0;
    }

    //����ͷ�Ĵ�С��Ӧ��mb
    if (App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u1PacketParseType == PACKET_WITHHEAD)
    {
        m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->m_u4OrgLength);
    }
    else
    {
        m_pCurrMessage = App_MessageBlockManager::instance()->Create(App_MainConfig::instance()->GetServerRecvBuff());
    }

    if (m_pCurrMessage == NULL)
    {
        //AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Close Connection from [%s:%d] RecvSize = %d, RecvCount = %d, SendSize = %d, SendCount = %d, m_u8RecvQueueTimeCost = %d, m_u4RecvQueueCount = %d, m_u8SendQueueTimeCost = %d.",m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4AllRecvSize, m_u4AllRecvCount, m_u4AllSendSize, m_u4AllSendCount, m_u8RecvQueueTimeCost, m_u4RecvQueueCount, m_u8SendQueueTimeCost);
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open]pmb new is NULL.\n"));
        return 0;
    }

    //��������ӷ������ӿ�
    if (false == App_ConnectManager::instance()->AddConnect(this))
    {
        OUR_DEBUG((LM_ERROR, "%s.\n", App_ConnectManager::instance()->GetError()));
        sprintf_safe(m_szError, MAX_BUFF_500, "%s", App_ConnectManager::instance()->GetError());
        return 0;
    }

    AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "Connection from [%s:%d].", m_addrRemote.get_host_addr(), m_addrRemote.get_port_number());

    //����PacketParse����Ӧ����
    App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Connect(GetConnectID(), GetClientIPInfo(), GetLocalIPInfo());

    //��֯����
    _MakePacket objMakePacket;

    objMakePacket.m_u4ConnectID  = GetConnectID();
    objMakePacket.m_pPacketParse = NULL;
    objMakePacket.m_u1Option     = PACKET_CONNECT;
    objMakePacket.m_AddrRemote   = m_addrRemote;

    if (ACE_OS::strcmp("INADDR_ANY", m_szLocalIP) == 0)
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort);
    }
    else
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort, m_szLocalIP);
    }

    //�������ӽ�����Ϣ��
    ACE_Time_Value tvNow = ACE_OS::gettimeofday();

    if (false == App_MakePacket::instance()->PutMessageBlock(&objMakePacket, tvNow))
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::open] ConnectID=%d, PACKET_CONNECT is error.\n", GetConnectID()));
    }

    OUR_DEBUG((LM_DEBUG, "[CConnectHandler::open]Open(%d) Connection from [%s:%d](0x%08x).\n", GetConnectID(), m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), this));

    m_u1ConnectState = CONNECT_OPEN;
    m_emIOType       = FILE_INPUT;
    m_pFileTest      = pFileTest;

    return GetConnectID();
}

int CConnectHandler::handle_write_file_stream(const char* pData, uint32 u4Size, uint8 u1ParseID)
{
    m_pPacketParse = App_PacketParsePool::instance()->Create();

    if (NULL == m_pPacketParse)
    {
        OUR_DEBUG((LM_DEBUG, "[CProConnectHandle::handle_write_file_stream](%d) m_pPacketParse new error.\n", GetConnectID()));
        return -1;
    }

    if (App_PacketParseLoader::instance()->GetPacketParseInfo(u1ParseID)->m_u1PacketParseType == PACKET_WITHHEAD)
    {
        uint32 u4PacketHead = App_PacketParseLoader::instance()->GetPacketParseInfo(u1ParseID)->m_u4OrgLength;
        ACE_Message_Block* pMbHead = App_MessageBlockManager::instance()->Create(u4PacketHead);

        if (NULL == pMbHead)
        {
            OUR_DEBUG((LM_DEBUG, "[CProConnectHandle::handle_write_file_stream](%d) pMbHead is NULL.\n", GetConnectID()));
            return -1;
        }

        memcpy_safe((char*)pData, u4PacketHead, pMbHead->wr_ptr(), u4PacketHead);
        pMbHead->wr_ptr(u4PacketHead);

        //������Ϣͷ
        _Head_Info objHeadInfo;
        bool blStateHead = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Parse_Packet_Head_Info(GetConnectID(), pMbHead, App_MessageBlockManager::instance(), &objHeadInfo);

        if (false == blStateHead)
        {
            OUR_DEBUG((LM_DEBUG, "[CProConnectHandle::handle_write_file_stream](%d) Parse_Packet_Head_Info is illegal.\n", GetConnectID()));
            ClearPacketParse();
            return -1;
        }

        m_pPacketParse->SetPacket_IsHandleHead(false);
        m_pPacketParse->SetPacket_Head_Message(objHeadInfo.m_pmbHead);
        m_pPacketParse->SetPacket_Head_Curr_Length(objHeadInfo.m_u4HeadCurrLen);
        m_pPacketParse->SetPacket_Body_Src_Length(objHeadInfo.m_u4BodySrcLen);
        m_pPacketParse->SetPacket_CommandID(objHeadInfo.m_u2PacketCommandID);

        //������Ϣ��
        ACE_Message_Block* pMbBody = App_MessageBlockManager::instance()->Create(u4Size - u4PacketHead);

        if (NULL == pMbBody)
        {
            OUR_DEBUG((LM_DEBUG, "[CProConnectHandle::handle_write_file_stream](%d) pMbHead is NULL.\n", GetConnectID()));
            return -1;
        }

        memcpy_safe((char*)&pData[u4PacketHead], u4Size - u4PacketHead, pMbBody->wr_ptr(), u4Size - u4PacketHead);
        pMbBody->wr_ptr(u4Size - u4PacketHead);

        //�������ݰ���
        _Body_Info obj_Body_Info;
        bool blStateBody = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Parse_Packet_Body_Info(GetConnectID(), pMbBody, App_MessageBlockManager::instance(), &obj_Body_Info);

        if (false == blStateBody)
        {
            //������ݰ���Ƿ����Ͽ�����
            OUR_DEBUG((LM_ERROR, "[CProConnectHandle::handle_write_file_stream]Parse_Packet_Body_Info is illegal.\n"));

            //����PacketParse
            ClearPacketParse();
            return -1;
        }
        else
        {
            m_pPacketParse->SetPacket_Body_Message(obj_Body_Info.m_pmbBody);
            m_pPacketParse->SetPacket_Body_Curr_Length(obj_Body_Info.m_u4BodyCurrLen);

            if (obj_Body_Info.m_u2PacketCommandID > 0)
            {
                m_pPacketParse->SetPacket_CommandID(obj_Body_Info.m_u2PacketCommandID);
            }
        }

        if (false == CheckMessage())
        {
            OUR_DEBUG((LM_ERROR, "[CProConnectHandle::handle_write_file_stream]CheckMessage is false.\n"));
            return -1;
        }
    }
    else
    {
        //��ģʽ�����ļ�����
        ACE_Message_Block* pMbStream = App_MessageBlockManager::instance()->Create(u4Size);

        if (NULL == pMbStream)
        {
            OUR_DEBUG((LM_DEBUG, "[CProConnectHandle::handle_write_file_stream](%d) pMbHead is NULL.\n", GetConnectID()));
            return -1;
        }

        memcpy_safe((char*)pData, u4Size, pMbStream->wr_ptr(), u4Size);

        _Packet_Info obj_Packet_Info;
        uint8 n1Ret = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Parse_Packet_Stream(GetConnectID(), pMbStream, dynamic_cast<IMessageBlockManager*>(App_MessageBlockManager::instance()), &obj_Packet_Info);

        if (PACKET_GET_ENOUGTH == n1Ret)
        {
            m_pPacketParse->SetPacket_Head_Message(obj_Packet_Info.m_pmbHead);
            m_pPacketParse->SetPacket_Body_Message(obj_Packet_Info.m_pmbBody);
            m_pPacketParse->SetPacket_CommandID(obj_Packet_Info.m_u2PacketCommandID);
            m_pPacketParse->SetPacket_Head_Src_Length(obj_Packet_Info.m_u4HeadSrcLen);
            m_pPacketParse->SetPacket_Head_Curr_Length(obj_Packet_Info.m_u4HeadCurrLen);
            m_pPacketParse->SetPacket_Head_Src_Length(obj_Packet_Info.m_u4BodySrcLen);
            m_pPacketParse->SetPacket_Body_Curr_Length(obj_Packet_Info.m_u4BodyCurrLen);

            //�Ѿ��������������ݰ����Ӹ������߳�ȥ����
            if (false == CheckMessage())
            {
                OUR_DEBUG((LM_ERROR, "[CProConnectHandle::handle_write_file_stream]CheckMessage is false.\n"));
                return -1;
            }

        }
        else if (PACKET_GET_NO_ENOUGTH == n1Ret)
        {
            OUR_DEBUG((LM_ERROR, "[CProConnectHandle::handle_write_file_stream]Parse_Packet_Stream is PACKET_GET_NO_ENOUGTH.\n"));
            //���յ����ݲ�����
            ClearPacketParse();
            return -1;
        }
        else
        {
            OUR_DEBUG((LM_ERROR, "[CProConnectHandle::handle_write_file_stream]Parse_Packet_Stream is PACKET_GET_ERROR.\n"));
            ClearPacketParse();
            return -1;
        }
    }

    return 0;
}

bool CConnectHandler::CheckAlive(ACE_Time_Value& tvNow)
{
    //ACE_Time_Value tvNow = ACE_OS::gettimeofday();
    ACE_Time_Value tvIntval(tvNow - m_atvInput);

    if(tvIntval.sec() > m_u2MaxConnectTime)
    {
        //������������ʱ�䣬��������ر�����
        OUR_DEBUG ((LM_ERROR, "[CConnectHandle::CheckAlive] Connectid=%d Server Close!\n", GetConnectID()));
        return false;
    }
    else
    {
        return true;
    }
}

void CConnectHandler::SetRecvQueueTimeCost(uint32 u4TimeCost)
{
    //���������ֵ�����¼����־��ȥ
    if((uint32)(m_u8RecvQueueTimeout * 1000) <= u4TimeCost)
    {
        AppLogManager::instance()->WriteLog(LOG_SYSTEM_RECVQUEUEERROR, "[TCP]IP=%s,Prot=%d, m_u8RecvQueueTimeout=[%d], Timeout=[%d].", GetClientIPInfo().m_szClientIP, GetClientIPInfo().m_nPort, (uint32)m_u8RecvQueueTimeout, u4TimeCost);
    }

    m_u4RecvQueueCount++;
}

void CConnectHandler::SetSendQueueTimeCost(uint32 u4TimeCost)
{
    //���������ֵ�����¼����־��ȥ
    if((uint32)(m_u8SendQueueTimeout) <= u4TimeCost)
    {
        ACE_Time_Value tvNow = ACE_OS::gettimeofday();
        AppLogManager::instance()->WriteLog(LOG_SYSTEM_SENDQUEUEERROR, "[TCP]IP=%s,Prot=%d,m_u8SendQueueTimeout = [%d], Timeout=[%d].", GetClientIPInfo().m_szClientIP, GetClientIPInfo().m_nPort, (uint32)m_u8SendQueueTimeout, u4TimeCost);

        //��֯����
        _MakePacket objMakePacket;

        objMakePacket.m_u4ConnectID       = GetConnectID();
        objMakePacket.m_pPacketParse      = NULL;
        objMakePacket.m_u1Option          = PACKET_SEND_TIMEOUT;
        objMakePacket.m_AddrRemote        = m_addrRemote;

        if (ACE_OS::strcmp("INADDR_ANY", m_szLocalIP) == 0)
        {
            objMakePacket.m_AddrListen.set(m_u4LocalPort);
        }
        else
        {
            objMakePacket.m_AddrListen.set(m_u4LocalPort, m_szLocalIP);
        }

        //���߲�����ӷ��ͳ�ʱ��ֵ����
        if(false == App_MakePacket::instance()->PutMessageBlock(&objMakePacket, tvNow))
        {
            OUR_DEBUG((LM_ERROR, "[CProConnectHandle::SetSendQueueTimeCost] ConnectID = %d, PACKET_CONNECT is error.\n", GetConnectID()));
        }
    }
}

uint8 CConnectHandler::GetConnectState()
{
    return m_u1ConnectState;
}

uint8 CConnectHandler::GetSendBuffState()
{
    return m_u1SendBuffState;
}

bool CConnectHandler::SendMessage(uint16 u2CommandID, IBuffPacket* pBuffPacket, uint8 u1State, uint8 u1SendType, uint32& u4PacketSize, bool blDelete, int nMessageID)
{
    //�����ǰ�����ѱ�����̹߳رգ������ﲻ������ֱ���˳�
    if (m_u1IsActive == 0)
    {
        ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
        memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char*)pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
        pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
        ACE_Time_Value tvNow = ACE_OS::gettimeofday();
        App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

        if (blDelete == true)
        {
            App_BuffPacketManager::instance()->Delete(pBuffPacket);
        }

        return false;
    }

    if (NET_INPUT == m_emIOType)
    {
        if (CONNECT_SERVER_CLOSE == m_u1ConnectState || CONNECT_CLIENT_CLOSE == m_u1ConnectState)
        {
            //�ڶ������Ѿ����ڹر�����ָ�֮�����������д����ȫ�����跢�͡�
            ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
            memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char*)pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
            pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
            ACE_Time_Value tvNow = ACE_OS::gettimeofday();
            App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

            if (blDelete == true)
            {
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            return false;
        }

        //�������ֱ�ӷ������ݣ���ƴ�����ݰ�
        if (u1State == PACKET_SEND_CACHE)
        {
            //���ж�Ҫ���͵����ݳ��ȣ������Ƿ���Է��뻺�壬�����Ƿ��Ѿ�������
            uint32 u4SendPacketSize = 0;

            if (u1SendType == SENDMESSAGE_NOMAL)
            {
                u4SendPacketSize = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Make_Send_Packet_Length(GetConnectID(), pBuffPacket->GetPacketLen(), u2CommandID);
            }
            else
            {
                u4SendPacketSize = (uint32)pBuffPacket->GetPacketLen();
            }

            u4PacketSize = u4SendPacketSize;

            if (u4SendPacketSize + (uint32)m_pBlockMessage->length() >= m_u4SendMaxBuffSize)
            {
                OUR_DEBUG((LM_DEBUG, "[CConnectHandler::SendMessage] Connectid=[%d] m_pBlockMessage is not enougth.\n", GetConnectID()));
                ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
                memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char*)pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
                pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
                ACE_Time_Value tvNow = ACE_OS::gettimeofday();
                App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

                if (blDelete == true)
                {
                    App_BuffPacketManager::instance()->Delete(pBuffPacket);
                }

                return false;
            }
            else
            {
                //��ӽ�������
                //ACE_Message_Block* pMbBufferData = NULL;

                //SENDMESSAGE_NOMAL����Ҫ��ͷ��ʱ�򣬷��򣬲����ֱ�ӷ���
                if (u1SendType == SENDMESSAGE_NOMAL)
                {
                    //������ɷ������ݰ�
                    App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Make_Send_Packet(GetConnectID(), pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), m_pBlockMessage, u2CommandID);
                }
                else
                {
                    //�������SENDMESSAGE_NOMAL����ֱ�����
                    memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), m_pBlockMessage->wr_ptr(), pBuffPacket->GetPacketLen());
                    m_pBlockMessage->wr_ptr(pBuffPacket->GetPacketLen());
                }
            }

            if (blDelete == true)
            {
                //ɾ���������ݰ�
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            //������ɣ��������˳�
            return true;
        }
        else
        {
            //OUR_DEBUG((LM_DEBUG,"[CConnectHandler::SendMessage]Connectid=%d,333.\n", GetConnectID()));
            //���ж��Ƿ�Ҫ��װ��ͷ�������Ҫ������װ��m_pBlockMessage��
            uint32 u4SendPacketSize = 0;

            if (u1SendType == SENDMESSAGE_NOMAL)
            {
                u4SendPacketSize = App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Make_Send_Packet_Length(GetConnectID(), pBuffPacket->GetPacketLen(), u2CommandID);

                if (u4SendPacketSize >= m_u4SendMaxBuffSize)
                {
                    OUR_DEBUG((LM_DEBUG, "[CConnectHandler::SendMessage](%d) u4SendPacketSize is more than(%d)(%d).\n", GetConnectID(), u4SendPacketSize, m_u4SendMaxBuffSize));
                    ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
                    memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char*)pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
                    pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
                    ACE_Time_Value tvNow = ACE_OS::gettimeofday();
                    App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

                    if (blDelete == true)
                    {
                        //ɾ���������ݰ�
                        App_BuffPacketManager::instance()->Delete(pBuffPacket);
                    }

                    return false;
                }

                //OUR_DEBUG((LM_DEBUG,"[CConnectHandler::SendMessage] Connectid=[%d] aaa m_pBlockMessage=0x%08x.\n", GetConnectID(), m_pBlockMessage));
                App_PacketParseLoader::instance()->GetPacketParseInfo(m_u4PacketParseInfoID)->Make_Send_Packet(GetConnectID(), pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), m_pBlockMessage, u2CommandID);
                //����MakePacket�Ѿ��������ݳ��ȣ����������ﲻ��׷��
            }
            else
            {
                u4SendPacketSize = (uint32)pBuffPacket->GetPacketLen();

                if (u4SendPacketSize >= m_u4SendMaxBuffSize)
                {
                    OUR_DEBUG((LM_DEBUG, "[CConnectHandler::SendMessage](%d) u4SendPacketSize is more than(%d)(%d).\n", GetConnectID(), u4SendPacketSize, m_u4SendMaxBuffSize));
                    ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
                    memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char*)pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
                    pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
                    ACE_Time_Value tvNow = ACE_OS::gettimeofday();
                    App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

                    if (blDelete == true)
                    {
                        //ɾ���������ݰ�
                        App_BuffPacketManager::instance()->Delete(pBuffPacket);
                    }

                    return false;
                }

                //OUR_DEBUG((LM_DEBUG,"[CConnectHandler::SendMessage] Connectid=[%d] aaa m_pBlockMessage=0x%08x.\n", GetConnectID(), m_pBlockMessage));
                memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), m_pBlockMessage->wr_ptr(), pBuffPacket->GetPacketLen());
                m_pBlockMessage->wr_ptr(pBuffPacket->GetPacketLen());
            }

            //���֮ǰ�л������ݣ���ͻ�������һ����
            u4PacketSize = (uint32)m_pBlockMessage->length();

            ACE_Message_Block* pMbData = NULL;

            //����϶������0
            if (m_pBlockMessage->length() > 0)
            {
                //��Ϊ���첽���ͣ����͵�����ָ�벻���������ͷţ�������Ҫ�����ﴴ��һ���µķ������ݿ飬�����ݿ���
                pMbData = App_MessageBlockManager::instance()->Create((uint32)m_pBlockMessage->length());
            }

            if (NULL == pMbData)
            {
                OUR_DEBUG((LM_DEBUG, "[CConnectHandler::SendMessage] Connectid=[%d] pMbData is NULL.\n", GetConnectID()));
                ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
                memcpy_safe((char*)pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char*)pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
                pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
                ACE_Time_Value tvNow = ACE_OS::gettimeofday();
                App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

                if (blDelete == true)
                {
                    //ɾ���������ݰ�
                    App_BuffPacketManager::instance()->Delete(pBuffPacket);
                }

                return false;
            }

            //OUR_DEBUG((LM_DEBUG,"[CConnectHandler::SendMessage] Connectid=[%d] m_pBlockMessage=0x%08x.\n", GetConnectID(), m_pBlockMessage));
            memcpy_safe(m_pBlockMessage->rd_ptr(), (uint32)m_pBlockMessage->length(), pMbData->wr_ptr(), (uint32)m_pBlockMessage->length());
            pMbData->wr_ptr(m_pBlockMessage->length());
            //������ɣ�����ջ������ݣ�ʹ�����
            m_pBlockMessage->reset();

            if (blDelete == true)
            {
                //ɾ���������ݰ�
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            //�����Ҫ������ɺ�ɾ���������ñ��λ
            if (PACKET_SEND_FIN_CLOSE == u1State)
            {
                m_emStatus = CLIENT_CLOSE_SENDOK;
            }

            //�ж��Ƿ񳬹���ֵ
            if (false == CheckSendMask((uint32)pMbData->length()))
            {
                App_MessageBlockManager::instance()->Close(pMbData);
                return false;
            }

            //����ϢID����MessageBlock
            ACE_Message_Block::ACE_Message_Type objType = ACE_Message_Block::MB_USER + nMessageID;
            pMbData->msg_type(objType);

            //����Ϣ������У���output�ڷ�Ӧ���̷߳��͡�
            ACE_Time_Value xtime = ACE_OS::gettimeofday();

            //���������������ٷŽ�ȥ��,�Ͳ��Ž�ȥ��
            if (msg_queue()->is_full() == true)
            {
                OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendMessage] Connectid=%d,putq is full(%d).\n", GetConnectID(), msg_queue()->message_count()));
                App_MessageBlockManager::instance()->Close(pMbData);
                return false;
            }

            if (this->putq(pMbData, &xtime) == -1)
            {
                OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendMessage] Connectid=%d,putq(%d) output errno = [%d].\n", GetConnectID(), msg_queue()->message_count(), errno));
                App_MessageBlockManager::instance()->Close(pMbData);
            }
            else
            {
                int nWakeupRet = reactor()->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);

                if (-1 == nWakeupRet)
                {
                    OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendMessage] Connectid=%d, nWakeupRet(%d) output errno = [%d].\n", GetConnectID(), nWakeupRet, errno));
                }
            }

            return true;
        }
    }
    else
    {
        //�ļ���ڣ�ֱ��д����־
        char szLog[10] = { '\0' };
        uint32 u4DebugSize = 0;
        bool blblMore = false;

        if (pBuffPacket->GetPacketLen() >= m_u4PacketDebugSize)
        {
            u4DebugSize = m_u4PacketDebugSize - 1;
            blblMore = true;
        }
        else
        {
            u4DebugSize = (int)pBuffPacket->GetPacketLen();
        }

        char* pData = (char*)pBuffPacket->GetData();

        for (uint32 i = 0; i < u4DebugSize; i++)
        {
            sprintf_safe(szLog, 10, "0x%02X ", (unsigned char)pData[i]);
            sprintf_safe(m_pPacketDebugData + 5 * i, MAX_BUFF_1024 - 5 * i, "0x%02X ", (unsigned char)pData[i]);
        }

        m_pPacketDebugData[5 * u4DebugSize] = '\0';

        if (blblMore == true)
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_DEBUG_CLIENTSEND, "[(%s)%s:%d]%s.(���ݰ�����)", m_szConnectName, m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_pPacketDebugData);
        }
        else
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_DEBUG_CLIENTSEND, "[(%s)%s:%d]%s.", m_szConnectName, m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_pPacketDebugData);
        }

        //�ص������ļ�����ӿ�
        if (NULL != m_pFileTest)
        {
            m_pFileTest->HandlerServerResponse(GetConnectID());
        }

        if (blDelete == true)
        {
            App_BuffPacketManager::instance()->Delete(pBuffPacket);
        }

        return true;
    }
}

bool CConnectHandler::SendCloseMessage()
{
    if (NET_INPUT == m_emIOType)
    {
        //��������Ϣ�Ͽ�����outputȥִ�У������Ͳ���Ҫͬ�������ˡ�
        ACE_Message_Block* pMbData = App_MessageBlockManager::instance()->Create(sizeof(int));

        if (NULL == pMbData)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendCloseMessage] Connectid=%d, pMbData is NULL.\n", GetConnectID()));
            return false;
        }

        ACE_Message_Block::ACE_Message_Type objType = ACE_Message_Block::MB_STOP;
        pMbData->msg_type(objType);

        //����Ϣ������У���output�ڷ�Ӧ���̷߳��͡�
        ACE_Time_Value xtime = ACE_OS::gettimeofday();

        //���������������ٷŽ�ȥ��,�Ͳ��Ž�ȥ��
        if (msg_queue()->is_full() == true)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendCloseMessage] Connectid=%d,putq is full(%d).\n", GetConnectID(), msg_queue()->message_count()));
            App_MessageBlockManager::instance()->Close(pMbData);
            return false;
        }

        if (this->putq(pMbData, &xtime) == -1)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendCloseMessage] Connectid=%d,putq(%d) output errno = [%d].\n", GetConnectID(), msg_queue()->message_count(), errno));
            App_MessageBlockManager::instance()->Close(pMbData);
        }
        else
        {
            m_u1ConnectState = CONNECT_SERVER_CLOSE;
            int nWakeupRet = reactor()->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);

            if (-1 == nWakeupRet)
            {
                OUR_DEBUG((LM_ERROR, "[CConnectHandler::SendCloseMessage] Connectid=%d, nWakeupRet(%d) output errno = [%d].\n", GetConnectID(), nWakeupRet, errno));
            }
        }
    }
    else
    {
        //�رջ��յ�ǰ����
        Close();
    }

    return true;
}

bool CConnectHandler::PutSendPacket(ACE_Message_Block* pMbData)
{
    if(NULL == pMbData)
    {
        return false;
    }

    //�����DEBUG״̬����¼��ǰ���Ͱ��Ķ���������
    if(App_MainConfig::instance()->GetDebug() == DEBUG_ON || m_blIsLog == true)
    {
        char szLog[10]  = {'\0'};
        int  nDebugSize = 0;
        bool blblMore   = false;

        if((uint32)pMbData->length() >= m_u4PacketDebugSize)
        {
            nDebugSize = m_u4PacketDebugSize - 1;
            blblMore   = true;
        }
        else
        {
            nDebugSize = (int)pMbData->length();
        }

        char* pData = pMbData->rd_ptr();

        for(int i = 0; i < nDebugSize; i++)
        {
            sprintf_safe(szLog, 10, "0x%02X ", (unsigned char)pData[i]);
            sprintf_safe(m_pPacketDebugData + 5*i, MAX_BUFF_1024 - 5*i, "%s", szLog);
        }

        m_pPacketDebugData[5 * nDebugSize] = '\0';

        if(blblMore == true)
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_DEBUG_CLIENTSEND, "[(%s)%s:%d]%s.(���ݰ�����ֻ��¼ǰ200�ֽ�)", m_szConnectName, m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_pPacketDebugData);
        }
        else
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_DEBUG_CLIENTSEND, "[(%s)%s:%d]%s.", m_szConnectName, m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_pPacketDebugData);
        }
    }

    //ͳ�Ʒ�������
    ACE_Date_Time dtNow;

    if(false == m_TimeConnectInfo.SendCheck((uint8)dtNow.minute(), 1, (uint32)pMbData->length()))
    {
        //�������޶��ķ�ֵ����Ҫ�ر����ӣ�����¼��־
        AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECTABNORMAL,
                                               App_MainConfig::instance()->GetClientDataAlert()->m_u4MailID,
                                               (char* )"Alert",
                                               "[TCP]IP=%s,Prot=%d,SendPacketCount=%d, SendSize=%d.",
                                               m_addrRemote.get_host_addr(),
                                               m_addrRemote.get_port_number(),
                                               m_TimeConnectInfo.m_u4SendPacketCount,
                                               m_TimeConnectInfo.m_u4SendSize);

        //���÷��ʱ��
        App_ForbiddenIP::instance()->AddTempIP(m_addrRemote.get_host_addr(), App_MainConfig::instance()->GetIPAlert()->m_u4IPTimeout);
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::PutSendPacket] ConnectID = %d, Send Data is more than limit.\n", GetConnectID()));

        ACE_Time_Value tvNow = ACE_OS::gettimeofday();
        App_MakePacket::instance()->PutSendErrorMessage(GetConnectID(), pMbData, tvNow);
        //App_MessageBlockManager::instance()->Close(pMbData);

        return false;
    }

    //���ͳ�ʱʱ������
    ACE_Time_Value  nowait(0, m_u4SendThresHold*MAX_BUFF_1000);

    if(NULL == pMbData)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::PutSendPacket] ConnectID = %d, get_handle() == ACE_INVALID_HANDLE.\n", GetConnectID()));
        return false;
    }

    if(get_handle() == ACE_INVALID_HANDLE)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::PutSendPacket] ConnectID = %d, get_handle() == ACE_INVALID_HANDLE.\n", GetConnectID()));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectHandler::PutSendPacket] ConnectID = %d, get_handle() == ACE_INVALID_HANDLE.\n", GetConnectID());
        ACE_Time_Value tvNow = ACE_OS::gettimeofday();
        App_MakePacket::instance()->PutSendErrorMessage(GetConnectID(), pMbData, tvNow);
        App_MessageBlockManager::instance()->Close(pMbData);
        return false;
    }

    //��������
    int nSendPacketLen = (int)pMbData->length();
    int nIsSendSize    = 0;

    //ѭ�����ͣ�ֱ�����ݷ�����ɡ�
    while(true)
    {
        if(nSendPacketLen <= 0)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectHandler::PutSendPacket] ConnectID = %d, nCurrSendSize error is %d.\n", GetConnectID(), nSendPacketLen));
            App_MessageBlockManager::instance()->Close(pMbData);
            return false;
        }

        int nDataLen = (int)this->peer().send(pMbData->rd_ptr(), nSendPacketLen - nIsSendSize, &nowait);

        if(nDataLen <= 0)
        {
            int nErrno = errno;
            OUR_DEBUG((LM_ERROR, "[CConnectHandler::PutSendPacket] ConnectID = %d, error = %d.\n", GetConnectID(), nErrno));

            AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "WriteError [%s:%d] nErrno = %d  result.bytes_transferred() = %d, ",
                                                m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), nErrno,
                                                nIsSendSize);

            //������Ϣ�ص�
            pMbData->rd_ptr((size_t)0);
            ACE_Time_Value tvNow = ACE_OS::gettimeofday();
            App_MakePacket::instance()->PutSendErrorMessage(GetConnectID(), pMbData, tvNow);

            OUR_DEBUG((LM_ERROR, "[CConnectHandler::PutSendPacket] ConnectID=%d send cancel.\n", GetConnectID()));
            //�رյ�ǰ����
            //App_ConnectManager::instance()->CloseUnLock(GetConnectID());

            return false;
        }
        else if(nDataLen >= nSendPacketLen - nIsSendSize)   //�����ݰ�ȫ��������ϣ���ա�
        {
            //OUR_DEBUG((LM_ERROR, "[CConnectHandler::handle_output] ConnectID = %d, send (%d) OK.\n", GetConnectID(), msg_queue()->is_empty()));
            m_u4AllSendCount    += 1;
            m_u4AllSendSize     += (uint32)pMbData->length();
            m_atvOutput         = ACE_OS::gettimeofday();

            int nMessageID = pMbData->msg_type() - ACE_Message_Block::MB_USER;

            if(nMessageID > 0)
            {
                //��Ҫ�ص����ͳɹ���ִ
                _MakePacket objMakePacket;

                CPacketParse objPacketParse;
                ACE_Message_Block* pSendOKData = App_MessageBlockManager::instance()->Create(sizeof(int));
                memcpy_safe((char* )&nMessageID, sizeof(int), pSendOKData->wr_ptr(), sizeof(int));
                pSendOKData->wr_ptr(sizeof(int));
                objPacketParse.SetPacket_Head_Message(pSendOKData);
                objPacketParse.SetPacket_Head_Curr_Length((uint32)pSendOKData->length());

                objMakePacket.m_u4ConnectID       = GetConnectID();
                objMakePacket.m_pPacketParse      = &objPacketParse;
                objMakePacket.m_u1Option          = PACKET_SEND_OK;
                objMakePacket.m_AddrRemote        = m_addrRemote;

                if (ACE_OS::strcmp("INADDR_ANY", m_szLocalIP) == 0)
                {
                    objMakePacket.m_AddrListen.set(m_u4LocalPort);
                }
                else
                {
                    objMakePacket.m_AddrListen.set(m_u4LocalPort, m_szLocalIP);
                }

                //���Ϳͻ������ӶϿ���Ϣ��
                ACE_Time_Value tvNow = ACE_OS::gettimeofday();

                if(false == App_MakePacket::instance()->PutMessageBlock(&objMakePacket, tvNow))
                {
                    OUR_DEBUG((LM_ERROR, "[CConnectHandle::Close] ConnectID = %d, PACKET_CONNECT is error.\n", GetConnectID()));
                }

                //��ԭ��Ϣ����
                pMbData->msg_type(ACE_Message_Block::MB_DATA);
            }

            App_MessageBlockManager::instance()->Close(pMbData);

            return true;
        }
        else
        {
            pMbData->rd_ptr(nDataLen);
            nIsSendSize      += nDataLen;
            m_atvOutput      = ACE_OS::gettimeofday();
            continue;
        }
    }

    return true;
}

bool CConnectHandler::CheckMessage()
{
    if (m_pPacketParse->GetMessageHead() == NULL)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectHandler::CheckMessage](%d)head is NULL.\n", GetConnectID()));
        return false;
    }

    if(m_pPacketParse->GetMessageBody() == NULL)
    {
        m_u4AllRecvSize += (uint32)m_pPacketParse->GetMessageHead()->length();
    }
    else
    {
        m_u4AllRecvSize += (uint32)m_pPacketParse->GetMessageHead()->length() + (uint32)m_pPacketParse->GetMessageBody()->length();
    }

    //OUR_DEBUG((LM_ERROR, "[CConnectHandler::CheckMessage]head length=%d.\n", m_pPacketParse->GetMessageHead()->length()));
    //OUR_DEBUG((LM_ERROR, "[CConnectHandler::CheckMessage]body length=%d.\n", m_pPacketParse->GetMessageBody()->length()));

    m_u4AllRecvCount++;

    ACE_Time_Value tvCheck = ACE_OS::gettimeofday();
    ACE_Date_Time dtNow(tvCheck);

    if(false == m_TimeConnectInfo.RecvCheck((uint8)dtNow.minute(), 1, m_u4AllRecvSize))
    {
        //�������޶��ķ�ֵ����Ҫ�ر����ӣ�����¼��־
        AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECTABNORMAL,
                                               App_MainConfig::instance()->GetClientDataAlert()->m_u4MailID,
                                               (char* )"Alert",
                                               "[TCP]IP=%s,Prot=%d,PacketCount=%d, RecvSize=%d.",
                                               m_addrRemote.get_host_addr(),
                                               m_addrRemote.get_port_number(),
                                               m_TimeConnectInfo.m_u4RecvPacketCount,
                                               m_TimeConnectInfo.m_u4RecvSize);

        App_PacketParsePool::instance()->Delete(m_pPacketParse, true);
        m_pPacketParse = NULL;

        //���÷��ʱ��
        App_ForbiddenIP::instance()->AddTempIP(m_addrRemote.get_host_addr(), App_MainConfig::instance()->GetIPAlert()->m_u4IPTimeout);
        OUR_DEBUG((LM_ERROR, "[CConnectHandle::CheckMessage] ConnectID = %d, PutMessageBlock is check invalid.\n", GetConnectID()));
        return false;
    }

    //��֯����
    _MakePacket objMakePacket;

    objMakePacket.m_u4ConnectID       = GetConnectID();
    objMakePacket.m_pPacketParse      = m_pPacketParse;
    objMakePacket.m_AddrRemote        = m_addrRemote;
    objMakePacket.m_u4PacketParseID   = GetPacketParseInfoID();

    if(ACE_OS::strcmp("INADDR_ANY", m_szLocalIP) == 0)
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort);
    }
    else
    {
        objMakePacket.m_AddrListen.set(m_u4LocalPort, m_szLocalIP);
    }

    objMakePacket.m_u1Option = PACKET_PARSE;

    //������Buff������Ϣ����
    if(false == App_MakePacket::instance()->PutMessageBlock(&objMakePacket, tvCheck))
    {
        m_pPacketParse = NULL;

        OUR_DEBUG((LM_ERROR, "[CConnectHandle::CheckMessage] ConnectID = %d, PutMessageBlock is error.\n", GetConnectID()));
    }

    //����ʱ������
    App_ConnectManager::instance()->SetConnectTimeWheel(this);

    App_PacketParsePool::instance()->Delete(m_pPacketParse);

    return true;
}

_ClientConnectInfo CConnectHandler::GetClientInfo()
{
    _ClientConnectInfo ClientConnectInfo;

    ClientConnectInfo.m_blValid             = true;
    ClientConnectInfo.m_u4ConnectID         = GetConnectID();
    ClientConnectInfo.m_addrRemote          = m_addrRemote;
    ClientConnectInfo.m_u4RecvCount         = m_u4AllRecvCount;
    ClientConnectInfo.m_u4SendCount         = m_u4AllSendCount;
    ClientConnectInfo.m_u4AllRecvSize       = m_u4AllSendSize;
    ClientConnectInfo.m_u4AllSendSize       = m_u4AllSendSize;
    ClientConnectInfo.m_u4BeginTime         = (uint32)m_atvConnect.sec();
    ClientConnectInfo.m_u4AliveTime         = (uint32)(ACE_OS::gettimeofday().sec() -  m_atvConnect.sec());
    ClientConnectInfo.m_u4RecvQueueCount    = m_u4RecvQueueCount;
    ClientConnectInfo.m_u8RecvQueueTimeCost = m_u8RecvQueueTimeCost;
    ClientConnectInfo.m_u8SendQueueTimeCost = m_u8SendQueueTimeCost;

    return ClientConnectInfo;
}

_ClientIPInfo  CConnectHandler::GetClientIPInfo()
{
    _ClientIPInfo ClientIPInfo;
    sprintf_safe(ClientIPInfo.m_szClientIP, MAX_BUFF_50, "%s", m_addrRemote.get_host_addr());
    ClientIPInfo.m_nPort = (int)m_addrRemote.get_port_number();
    return ClientIPInfo;
}

_ClientIPInfo  CConnectHandler::GetLocalIPInfo()
{
    _ClientIPInfo ClientIPInfo;
    sprintf_safe(ClientIPInfo.m_szClientIP, MAX_BUFF_50, "%s", m_szLocalIP);
    ClientIPInfo.m_nPort = (int)m_u4LocalPort;
    return ClientIPInfo;
}

bool CConnectHandler::CheckSendMask(uint32 u4PacketLen)
{
    //OUR_DEBUG((LM_ERROR, "[CConnectHandler::CheckSendMask]ConnectID=%d, m_u4ReadSendSize=%d, u4PacketLen=%d.\n", GetConnectID(), m_u4ReadSendSize, u4PacketLen));
    m_u4ReadSendSize += u4PacketLen;

    //OUR_DEBUG ((LM_ERROR, "[CConnectHandler::CheckSendMask]GetSendDataMask = %d, m_u4ReadSendSize=%d, m_u4SuccessSendSize=%d.\n", App_MainConfig::instance()->GetSendDataMask(), m_u4ReadSendSize, m_u4SuccessSendSize));
    if(m_u4ReadSendSize - m_u4SuccessSendSize >= App_MainConfig::instance()->GetSendDataMask())
    {
        OUR_DEBUG ((LM_ERROR, "[CConnectHandler::CheckSendMask]ConnectID = %d, SingleConnectMaxSendBuffer is more than DataMask(%d)(%d:%d)!\n", GetConnectID(), App_MainConfig::instance()->GetSendDataMask(), m_u4ReadSendSize, m_u4SuccessSendSize));
        AppLogManager::instance()->WriteLog(LOG_SYSTEM_SENDQUEUEERROR, "]Connection from [%s:%d], SingleConnectMaxSendBuffer is more than(%d)!.", m_addrRemote.get_host_addr(), m_addrRemote.get_port_number(), m_u4ReadSendSize - m_u4SuccessSendSize);
        return false;
    }
    else
    {
        return true;
    }
}

void CConnectHandler::ClearPacketParse()
{
    if(NULL != m_pPacketParse)
    {
        if (m_pPacketParse->GetMessageHead() != NULL)
        {
            App_MessageBlockManager::instance()->Close(m_pPacketParse->GetMessageHead());
        }

        if (m_pPacketParse->GetMessageBody() != NULL)
        {
            App_MessageBlockManager::instance()->Close(m_pPacketParse->GetMessageBody());
        }

        if(m_pCurrMessage != NULL && m_pPacketParse->GetMessageBody() != m_pCurrMessage && m_pPacketParse->GetMessageHead() != m_pCurrMessage)
        {
            App_MessageBlockManager::instance()->Close(m_pCurrMessage);
        }

        m_pCurrMessage = NULL;

        App_PacketParsePool::instance()->Delete(m_pPacketParse);
        m_pPacketParse = NULL;
    }
}

void CConnectHandler::SetConnectName(const char* pName)
{
    sprintf_safe(m_szConnectName, MAX_BUFF_100, "%s", pName);
}

void CConnectHandler::SetIsLog(bool blIsLog)
{
    m_blIsLog = blIsLog;
}

char* CConnectHandler::GetConnectName()
{
    return m_szConnectName;
}

bool CConnectHandler::GetIsLog()
{
    return m_blIsLog;
}

void CConnectHandler::SetHashID(int nHashID)
{
    m_nHashID = nHashID;
}

int CConnectHandler::GetHashID()
{
    return m_nHashID;
}

void CConnectHandler::SetLocalIPInfo(const char* pLocalIP, uint32 u4LocalPort)
{
    sprintf_safe(m_szLocalIP, MAX_BUFF_50, "%s", pLocalIP);
    m_u4LocalPort = u4LocalPort;
}

void CConnectHandler::SetSendCacheManager(CSendCacheManager* pSendCacheManager)
{
    if(NULL != pSendCacheManager)
    {
        m_pBlockMessage = pSendCacheManager->GetCacheData(GetConnectID());
    }
}

//***************************************************************************
CConnectManager::CConnectManager(void):m_mutex(), m_cond(m_mutex)
{
    m_u4TimeCheckID      = 0;
    m_szError[0]         = '\0';

    m_pTCTimeSendCheck   = NULL;
    m_tvCheckConnect     = ACE_OS::gettimeofday();
    m_blRun              = false;

    m_u4TimeConnect      = 0;
    m_u4TimeDisConnect   = 0;
    m_u4SendQueuePutTime = 0;

    //��ʼ�����Ͷ����
    m_SendMessagePool.Init();
}

CConnectManager::~CConnectManager(void)
{
    OUR_DEBUG((LM_INFO, "[CConnectManager::~CConnectManager].\n"));
    //m_blRun = false;
    //CloseAll();
}

void CConnectManager::CloseAll()
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGrard(m_ThreadWriteLock);
    if(m_blRun)
    {
        if (false == this->CloseMsgQueue())
        {
            OUR_DEBUG((LM_INFO, "[CConnectManager::CloseAll]CloseMsgQueue error.\n"));
        }
    }
    else
    {
        msg_queue()->deactivate();
    }

    if (false == KillTimer())
    {
        OUR_DEBUG((LM_INFO, "[CConnectManager::CloseAll]KillTimer error.\n"));
    }

    vector<CConnectHandler*> vecCloseConnectHandler;
    m_objHashConnectList.Get_All_Used(vecCloseConnectHandler);

    //��ʼ�ر���������
    for(int i = 0; i < (int)vecCloseConnectHandler.size(); i++)
    {
        CConnectHandler* pConnectHandler = vecCloseConnectHandler[i];
        pConnectHandler->Close();
    }

    //ɾ��hash��ռ�
    m_objHashConnectList.Close();

    //ɾ���������
    m_SendCacheManager.Close();
}

bool CConnectManager::Close(uint32 u4ConnectID)
{
    //OUR_DEBUG((LM_ERROR, "[CConnectManager::Close]ConnectID=%d Begin.\n", u4ConnectID));
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    //OUR_DEBUG((LM_ERROR, "[CConnectManager::Close]ConnectID=%d Begin 1.\n", u4ConnectID));
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);

    if (false == DelConnectTimeWheel(m_objHashConnectList.Get_Hash_Box_Data(szConnectID)))
    {
        OUR_DEBUG((LM_INFO, "[CConnectManager::Close]DelConnectTimeWheel error.\n"));
    }

    int nPos = m_objHashConnectList.Del_Hash_Data(szConnectID);

    if(0 < nPos)
    {
        //���շ��ͻ���
        m_SendCacheManager.FreeCacheData(u4ConnectID);
        m_u4TimeDisConnect++;

        //��������ͳ�ƹ���
        App_ConnectAccount::instance()->AddDisConnect();

        //OUR_DEBUG((LM_ERROR, "[CConnectManager::Close]ConnectID=%d End.\n", u4ConnectID));
        return true;
    }
    else
    {
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::Close] ConnectID[%d] is not find.", u4ConnectID);
        return true;
    }
}

bool CConnectManager::CloseUnLock(uint32 u4ConnectID)
{
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);

    //���ӹرգ����ʱ������
    if (false == DelConnectTimeWheel(m_objHashConnectList.Get_Hash_Box_Data(szConnectID)))
    {
        OUR_DEBUG((LM_INFO, "[CConnectManager::CloseUnLock]DelConnectTimeWheel error.\n"));
    }

    int nPos = m_objHashConnectList.Del_Hash_Data(szConnectID);

    if(0 < nPos)
    {
        //���շ��ͻ���
        m_SendCacheManager.FreeCacheData(u4ConnectID);
        m_u4TimeDisConnect++;

        //��������ͳ�ƹ���
        App_ConnectAccount::instance()->AddDisConnect();

        OUR_DEBUG((LM_ERROR, "[CConnectManager::CloseUnLock]ConnectID=%d End.\n", u4ConnectID));
        return true;
    }
    else
    {
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::Close] ConnectID[%d] is not find.", u4ConnectID);
        return true;
    }
}

bool CConnectManager::CloseConnect(uint32 u4ConnectID)
{
    m_ThreadWriteLock.acquire();
    char szConnectID[10] = { '\0' };
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);
    m_ThreadWriteLock.release();

    if(NULL != pConnectHandler)
    {
        //�ŵ���Ӧ���߳���ȥ��
        if (false == pConnectHandler->SendCloseMessage())
        {
            OUR_DEBUG((LM_INFO, "[CConnectManager::CloseConnect]SendCloseMessage error.\n"));
        }

        return true;
    }
    else
    {
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::CloseConnect] ConnectID[%d] is not find.", u4ConnectID);
        return true;
    }
}

bool CConnectManager::CloseConnect_By_Queue(uint32 u4ConnectID)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    //���뷢�Ͷ���
    _SendMessage* pSendMessage = m_SendMessagePool.Create();

    if (NULL == pSendMessage)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::CloseConnect_By_Queue]pSendMessage is NULL.\n"));
        return false;
    }

    ACE_Message_Block* mb = pSendMessage->GetQueueMessage();

    if (NULL != mb)
    {
        //��װ�ر�����ָ��
        pSendMessage->m_u4ConnectID = u4ConnectID;
        pSendMessage->m_pBuffPacket = NULL;
        pSendMessage->m_nEvents = 0;
        pSendMessage->m_u2CommandID = 0;
        pSendMessage->m_u1SendState = 0;
        pSendMessage->m_blDelete = false;
        pSendMessage->m_nMessageID = 0;
        pSendMessage->m_u1Type = 1;
        pSendMessage->m_tvSend = ACE_OS::gettimeofday();

        //�ж϶����Ƿ����Ѿ����
        int nQueueCount = (int)msg_queue()->message_count();

        if (nQueueCount >= (int)MAX_MSG_THREADQUEUE)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectManager::CloseConnect_By_Queue] Queue is Full nQueueCount = [%d].\n", nQueueCount));
            m_SendMessagePool.Delete(pSendMessage);
            return false;
        }

        ACE_Time_Value xtime = ACE_OS::gettimeofday() + ACE_Time_Value(0, m_u4SendQueuePutTime);

        if (this->putq(mb, &xtime) == -1)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectManager::CloseConnect_By_Queue] Queue putq  error nQueueCount = [%d] errno = [%d].\n", nQueueCount, errno));
            m_SendMessagePool.Delete(pSendMessage);
            return false;
        }
    }
    else
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::CloseConnect_By_Queue] mb new error.\n"));
        m_SendMessagePool.Delete(pSendMessage);
        return false;
    }

    return true;
}

bool CConnectManager::AddConnect(uint32 u4ConnectID, CConnectHandler* pConnectHandler)
{
    //OUR_DEBUG((LM_ERROR, "[CConnectManager::AddConnect]ConnectID=%d Begin.\n", u4ConnectID));
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    //OUR_DEBUG((LM_ERROR, "[CConnectManager::AddConnect]ConnectID=%d Begin 1.\n", u4ConnectID));

    if(pConnectHandler == NULL)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::AddConnect]ConnectID=%d, pConnectHandler is NULL.\n", u4ConnectID));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::AddConnect] pConnectHandler is NULL.");
        return false;
    }

    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pCurrConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if(NULL != pCurrConnectHandler)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::AddConnect]ConnectID=%d is find.\n", u4ConnectID));
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::AddConnect] ConnectID[%d] is exist.", u4ConnectID);
        return false;
    }

    pConnectHandler->SetConnectID(u4ConnectID);
    pConnectHandler->SetSendCacheManager(&m_SendCacheManager);
    //����Hash����
    m_objHashConnectList.Add_Hash_Data(szConnectID, pConnectHandler);
    m_u4TimeConnect++;

    //OUR_DEBUG((LM_ERROR, "[CConnectManager::AddConnect]ConnectID=%d.\n", u4ConnectID));

    //��������ͳ�ƹ���
    App_ConnectAccount::instance()->AddConnect();

    //����ʱ������
    if (false == SetConnectTimeWheel(pConnectHandler))
    {
        OUR_DEBUG((LM_INFO, "[CConnectManager::AddConnect]SetConnectTimeWheel error.\n"));
    }

    //OUR_DEBUG((LM_ERROR, "[CConnectManager::AddConnect]ConnectID=%d End.\n", u4ConnectID));
    return true;
}

bool CConnectManager::SetConnectTimeWheel(CConnectHandler* pConnectHandler)
{
    bool bAddResult = m_TimeWheelLink.Add_TimeWheel_Object(pConnectHandler);

    if(!bAddResult)
    {
        OUR_DEBUG((LM_ERROR,"[CConnectManager::SetConnectTimeWheel]Fail to set pConnectHandler(0x%08x).\n", pConnectHandler));
    }

    return bAddResult;
}

bool CConnectManager::DelConnectTimeWheel(CConnectHandler* pConnectHandler)
{
    m_TimeWheelLink.Del_TimeWheel_Object(pConnectHandler);
    return true;
}

bool CConnectManager::SendMessage(uint32 u4ConnectID, IBuffPacket* pBuffPacket, uint16 u2CommandID, uint8 u1SendState, uint8 u1SendType, ACE_Time_Value& tvSendBegin, bool blDelete, int nMessageID)
{
    //��Ϊ�Ƕ��е��ã��������ﲻ��Ҫ�����ˡ�
    if(NULL == pBuffPacket)
    {
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::SendMessage] ConnectID[%d] pBuffPacket is NULL.", u4ConnectID);
        return false;
    }

    m_ThreadWriteLock.acquire();
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);
    m_ThreadWriteLock.release();

    if(NULL != pConnectHandler)
    {
        uint32 u4PacketSize = 0;

        //OUR_DEBUG((LM_ERROR, "[CConnectManager::SendMessage]ConnectID=%d Begin 1 pConnectHandler.\n", u4ConnectID));
        if (false == pConnectHandler->SendMessage(u2CommandID, pBuffPacket, u1SendState, u1SendType, u4PacketSize, blDelete, nMessageID))
        {
            OUR_DEBUG((LM_ERROR, "[CConnectManager::SendMessage](%d) BSendMessage error.\n", u4ConnectID));
        }

        //OUR_DEBUG((LM_ERROR, "[CConnectManager::SendMessage]ConnectID=%d End 1 pConnectHandler.\n", u4ConnectID));
        //��¼��Ϣ��������ʱ��
        ACE_Time_Value tvInterval = ACE_OS::gettimeofday() - tvSendBegin;
        uint32 u4SendCost = (uint32)(tvInterval.msec());
        pConnectHandler->SetSendQueueTimeCost(u4SendCost);
        m_CommandAccount.SaveCommandData(u2CommandID, (uint64)u4SendCost, PACKET_TCP, u4PacketSize, u4PacketSize, COMMAND_TYPE_OUT);
        return true;
    }
    else
    {
        sprintf_safe(m_szError, MAX_BUFF_500, "[CConnectManager::SendMessage] ConnectID[%d] is not find.", u4ConnectID);
        //������Ӳ������ˣ������ﷵ��ʧ�ܣ��ص���ҵ���߼�ȥ����
        ACE_Message_Block* pSendMessage = App_MessageBlockManager::instance()->Create(pBuffPacket->GetPacketLen());
        memcpy_safe((char* )pBuffPacket->GetData(), pBuffPacket->GetPacketLen(), (char* )pSendMessage->wr_ptr(), pBuffPacket->GetPacketLen());
        pSendMessage->wr_ptr(pBuffPacket->GetPacketLen());
        ACE_Time_Value tvNow = ACE_OS::gettimeofday();
        App_MakePacket::instance()->PutSendErrorMessage(0, pSendMessage, tvNow);

        if(true == blDelete)
        {
            App_BuffPacketManager::instance()->Delete(pBuffPacket);
        }

        return true;
    }

    //OUR_DEBUG((LM_ERROR, "[CConnectManager::SendMessage]ConnectID=%d End.\n", u4ConnectID));
    return true;
}

bool CConnectManager::PostMessage(uint32 u4ConnectID, IBuffPacket* pBuffPacket, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    //OUR_DEBUG((LM_INFO, "[CConnectManager::PostMessage]Begin.\n"));
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGrard(m_ThreadWriteLock);
    //OUR_DEBUG((LM_INFO, "[CConnectManager::PostMessage]Begin 1.\n"));

    if(NULL == pBuffPacket)
    {
        OUR_DEBUG((LM_ERROR,"[CConnectManager::PutMessage] pBuffPacket is NULL.\n"));
        return false;
    }

    //���뷢�Ͷ���
    _SendMessage* pSendMessage = m_SendMessagePool.Create();

    if (NULL == pSendMessage)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::PutMessage](%d)pSendMessage is NULL(free=%d).\n", u4ConnectID, m_SendMessagePool.GetFreeCount()));
        return false;
    }

    ACE_Message_Block* mb = pSendMessage->GetQueueMessage();

    if(NULL != mb)
    {
        pSendMessage->m_u4ConnectID = u4ConnectID;
        pSendMessage->m_pBuffPacket = pBuffPacket;
        pSendMessage->m_nEvents     = u1SendType;
        pSendMessage->m_u2CommandID = u2CommandID;
        pSendMessage->m_blDelete    = blDelete;
        pSendMessage->m_u1SendState = u1SendState;
        pSendMessage->m_nMessageID  = nServerID;
        pSendMessage->m_u1Type      = 0;
        pSendMessage->m_tvSend      = ACE_OS::gettimeofday();

        //�ж϶����Ƿ����Ѿ����
        int nQueueCount = (int)msg_queue()->message_count();

        if(nQueueCount >= (int)MAX_MSG_THREADQUEUE)
        {
            OUR_DEBUG((LM_ERROR,"[CConnectManager::PutMessage] Queue is Full nQueueCount = [%d].\n", nQueueCount));

            if(blDelete == true)
            {
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            m_SendMessagePool.Delete(pSendMessage);
            return false;
        }

        ACE_Time_Value xtime = ACE_OS::gettimeofday() + ACE_Time_Value(0, m_u4SendQueuePutTime);

        if(this->putq(mb, &xtime) == -1)
        {
            OUR_DEBUG((LM_ERROR,"[CConnectManager::PutMessage] Queue putq  error nQueueCount = [%d] errno = [%d].\n", nQueueCount, errno));

            if(blDelete == true)
            {
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            m_SendMessagePool.Delete(pSendMessage);
            return false;
        }
    }
    else
    {
        OUR_DEBUG((LM_ERROR,"[CMessageService::PutMessage] mb new error.\n"));

        if(blDelete == true)
        {
            App_BuffPacketManager::instance()->Delete(pBuffPacket);
        }

        m_SendMessagePool.Delete(pSendMessage);
        return false;
    }

    //OUR_DEBUG((LM_INFO, "[CConnectManager::PostMessage]End.\n"));
    return true;
}

const char* CConnectManager::GetError()
{
    return m_szError;
}

bool CConnectManager::StartTimer()
{
    //���������߳�
    if(0 != open())
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::StartTimer]Open() is error.\n"));
        return false;
    }

    //���ⶨʱ���ظ�����
    if (false == KillTimer())
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::StartTimer]KillTimer error.\n"));
    }

    OUR_DEBUG((LM_ERROR, "[CConnectManager::StartTimer]begin....\n"));
    //�õ��ڶ���Reactor
    ACE_Reactor* pReactor = App_ReactorManager::instance()->GetAce_Reactor(REACTOR_POSTDEFINE);

    if(NULL == pReactor)
    {
        OUR_DEBUG((LM_ERROR, "CConnectManager::StartTimer() -->GetAce_Reactor(REACTOR_POSTDEFINE) is NULL.\n"));
        return false;
    }

    m_pTCTimeSendCheck = new _TimerCheckID();

    if(NULL == m_pTCTimeSendCheck)
    {
        OUR_DEBUG((LM_ERROR, "CConnectManager::StartTimer() m_pTCTimeSendCheck is NULL.\n"));
        return false;
    }

    m_pTCTimeSendCheck->m_u2TimerCheckID = PARM_CONNECTHANDLE_CHECK;
    long lTimeCheckID = pReactor->schedule_timer(this, (const void*)m_pTCTimeSendCheck, ACE_Time_Value(App_MainConfig::instance()->GetCheckAliveTime(), 0), ACE_Time_Value(App_MainConfig::instance()->GetCheckAliveTime(), 0));

    if(-1 == lTimeCheckID)
    {
        OUR_DEBUG((LM_ERROR, "CConnectManager::StartTimer()--> Start thread m_u4TimeCheckID error.\n"));
        return false;
    }
    else
    {
        m_u4TimeCheckID = (uint32)lTimeCheckID;
        OUR_DEBUG((LM_ERROR, "CConnectManager::StartTimer()--> Start thread time OK.\n"));
        return true;
    }
}

bool CConnectManager::KillTimer()
{
    if(m_u4TimeCheckID > 0)
    {
        App_ReactorManager::instance()->GetAce_Reactor(REACTOR_POSTDEFINE)->cancel_timer(m_u4TimeCheckID);
        m_u4TimeCheckID = 0;
    }

    SAFE_DELETE(m_pTCTimeSendCheck);
    return true;
}

int CConnectManager::handle_timeout(const ACE_Time_Value& tv, const void* arg)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    ACE_Time_Value tvNow = ACE_OS::gettimeofday();
    vector<CConnectHandler*> vecDelConnectHandler;

    if(arg == NULL)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::handle_timeout]arg is not NULL, tv = %d.\n", tv.sec()));
    }

    _TimerCheckID* pTimerCheckID = (_TimerCheckID*)arg;

    if(NULL == pTimerCheckID)
    {
        return 0;
    }

    //��ʱ��ⷢ�ͣ����ｫ��ʱ��¼������Ϣ�������У�����һ����ʱ��
    if(pTimerCheckID->m_u2TimerCheckID == PARM_CONNECTHANDLE_CHECK)
    {
        m_TimeWheelLink.Tick();

        //�ж��Ƿ�Ӧ�ü�¼������־
        ACE_Time_Value tvNow = ACE_OS::gettimeofday();
        ACE_Time_Value tvInterval(tvNow - m_tvCheckConnect);

        if(tvInterval.sec() >= MAX_MSG_HANDLETIME)
        {
            AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONNECT, "[CConnectManager]CurrConnectCount = %d,TimeInterval=%d, TimeConnect=%d, TimeDisConnect=%d.",
                                                GetCount(), MAX_MSG_HANDLETIME, m_u4TimeConnect, m_u4TimeDisConnect);

            //���õ�λʱ���������ͶϿ�������
            m_u4TimeConnect    = 0;
            m_u4TimeDisConnect = 0;
            m_tvCheckConnect   = tvNow;
        }

        //������������Ƿ�Խ��ط�ֵ
        if(App_MainConfig::instance()->GetConnectAlert()->m_u4ConnectAlert > 0)
        {
            if(GetCount() > (int)App_MainConfig::instance()->GetConnectAlert()->m_u4ConnectAlert)
            {
                AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECT,
                                                       App_MainConfig::instance()->GetConnectAlert()->m_u4MailID,
                                                       (char* )"Alert",
                                                       "[CProConnectManager]active ConnectCount is more than limit(%d > %d).",
                                                       GetCount(),
                                                       App_MainConfig::instance()->GetConnectAlert()->m_u4ConnectAlert);
            }
        }

        //��ⵥλʱ���������Ƿ�Խ��ֵ
        int nCheckRet = App_ConnectAccount::instance()->CheckConnectCount();

        if(nCheckRet == 1)
        {
            AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECT,
                                                   App_MainConfig::instance()->GetConnectAlert()->m_u4MailID,
                                                   (char* )"Alert",
                                                   "[CProConnectManager]CheckConnectCount is more than limit(%d > %d).",
                                                   App_ConnectAccount::instance()->GetCurrConnect(),
                                                   App_ConnectAccount::instance()->GetConnectMax());
        }
        else if(nCheckRet == 2)
        {
            AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECT,
                                                   App_MainConfig::instance()->GetConnectAlert()->m_u4MailID,
                                                   (char* )"Alert",
                                                   "[CProConnectManager]CheckConnectCount is little than limit(%d < %d).",
                                                   App_ConnectAccount::instance()->GetCurrConnect(),
                                                   App_ConnectAccount::instance()->Get4ConnectMin());
        }

        //��ⵥλʱ�����ӶϿ����Ƿ�Խ��ֵ
        nCheckRet = App_ConnectAccount::instance()->CheckDisConnectCount();

        if(nCheckRet == 1)
        {
            AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECT,
                                                   App_MainConfig::instance()->GetConnectAlert()->m_u4MailID,
                                                   (char* )"Alert",
                                                   "[CProConnectManager]CheckDisConnectCount is more than limit(%d > %d).",
                                                   App_ConnectAccount::instance()->GetCurrConnect(),
                                                   App_ConnectAccount::instance()->GetDisConnectMax());
        }
        else if(nCheckRet == 2)
        {
            AppLogManager::instance()->WriteToMail(LOG_SYSTEM_CONNECT,
                                                   App_MainConfig::instance()->GetConnectAlert()->m_u4MailID,
                                                   (char* )"Alert",
                                                   "[CProConnectManager]CheckDisConnectCount is little than limit(%d < %d).",
                                                   App_ConnectAccount::instance()->GetCurrConnect(),
                                                   App_ConnectAccount::instance()->GetDisConnectMin());
        }

    }

    return 0;
}

void CConnectManager::TimeWheel_Timeout_Callback(void* pArgsContext, vector<CConnectHandler*> vecConnectHandle)
{
    OUR_DEBUG((LM_INFO, "[CConnectManager::TimeWheel_Timeout_Callback]Timeout Count(%d).\n", vecConnectHandle.size()));

    for (int i = 0; i < (int)vecConnectHandle.size(); i++)
    {
        //�Ͽ���ʱ������
        CConnectManager* pManager = reinterpret_cast<CConnectManager*>(pArgsContext);
        OUR_DEBUG((LM_INFO, "[CConnectManager::TimeWheel_Timeout_Callback]ConnectID(%d).\n", vecConnectHandle[i]->GetConnectID()));

        if (NULL != pManager)
        {
            if (false == pManager->CloseConnect_By_Queue(vecConnectHandle[i]->GetConnectID()))
            {
                OUR_DEBUG((LM_INFO, "[CConnectManager::TimeWheel_Timeout_Callback]CloseConnect_By_Queue error.\n"));
            }
        }
    }
}

int CConnectManager::GetCount()
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    return m_objHashConnectList.Get_Used_Count();
}

int CConnectManager::open(void* args)
{
    if(args != NULL)
    {
        OUR_DEBUG((LM_INFO,"[CConnectManager::open]args is not NULL.\n"));
    }

    m_blRun = true;
    msg_queue()->high_water_mark(MAX_MSG_MASK);
    msg_queue()->low_water_mark(MAX_MSG_MASK);

    OUR_DEBUG((LM_INFO,"[CConnectManager::open] m_u4HighMask = [%d] m_u4LowMask = [%d]\n", MAX_MSG_MASK, MAX_MSG_MASK));

    if(activate(THREAD_PARAM, MAX_MSG_THREADCOUNT) == -1)
    {
        OUR_DEBUG((LM_ERROR, "[CConnectManager::open] activate error ThreadCount = [%d].", MAX_MSG_THREADCOUNT));
        m_blRun = false;
        return -1;
    }

    m_u4SendQueuePutTime = App_MainConfig::instance()->GetSendQueuePutTime() * 1000;

    resume();

    return 0;
}

int CConnectManager::svc (void)
{
    ACE_Time_Value xtime;

    while(true)
    {
        ACE_Message_Block* mb = NULL;
        ACE_OS::last_error(0);

        if(getq(mb, 0) == -1)
        {
            OUR_DEBUG((LM_ERROR,"[CConnectManager::svc] get error errno = [%d].\n", ACE_OS::last_error()));
            m_blRun = false;
            break;
        }
        else
        {
            if (mb == NULL)
            {
                continue;
            }

            if ((0 == mb->size ()) && (mb->msg_type () == ACE_Message_Block::MB_STOP))
            {
                m_mutex.acquire();
                mb->release ();
                this->msg_queue ()->deactivate ();
                m_cond.signal();
                m_mutex.release();
                break;
            }

            _SendMessage* msg = *((_SendMessage**)mb->base());

            if (!msg)
            {
                continue;
            }

            if (0 == msg->m_u1Type)
            {
                //����������
                if (false == SendMessage(msg->m_u4ConnectID, msg->m_pBuffPacket, msg->m_u2CommandID, msg->m_u1SendState, msg->m_nEvents, msg->m_tvSend, msg->m_blDelete, msg->m_nMessageID))
                {
                    OUR_DEBUG((LM_INFO, "[CConnectManager::svc]SendMessage error.\n"));
                }
            }
            else
            {
                //�������ӷ����������ر�
                if (false == CloseConnect(msg->m_u4ConnectID))
                {
                    OUR_DEBUG((LM_INFO, "[CConnectManager::svc]CloseConnect error.\n"));
                }
            }

            m_SendMessagePool.Delete(msg);
        }
    }

    OUR_DEBUG((LM_INFO,"[CConnectManager::svc] svc finish!\n"));
    return 0;
}

int CConnectManager::close(u_long)
{
    //m_blRun = false;
    OUR_DEBUG((LM_INFO,"[CConnectManager::close] close().\n"));
    return 0;
}

void CConnectManager::SetRecvQueueTimeCost(uint32 u4ConnectID, uint32 u4TimeCost)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if(NULL != pConnectHandler)
    {
        pConnectHandler->SetRecvQueueTimeCost(u4TimeCost);
    }
}

void CConnectManager::GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    vector<CConnectHandler*> vecConnectHandler;
    m_objHashConnectList.Get_All_Used(vecConnectHandler);

    for(int i = 0; i < (int)vecConnectHandler.size(); i++)
    {
        CConnectHandler* pConnectHandler = vecConnectHandler[i];

        if(pConnectHandler != NULL)
        {
            VecClientConnectInfo.push_back(pConnectHandler->GetClientInfo());
        }
    }
}

_ClientIPInfo CConnectManager::GetClientIPInfo(uint32 u4ConnectID)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if(NULL != pConnectHandler)
    {
        return pConnectHandler->GetClientIPInfo();
    }
    else
    {
        _ClientIPInfo ClientIPInfo;
        return ClientIPInfo;
    }
}

_ClientIPInfo CConnectManager::GetLocalIPInfo(uint32 u4ConnectID)
{
    //ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if(NULL != pConnectHandler)
    {
        return pConnectHandler->GetLocalIPInfo();
    }
    else
    {
        _ClientIPInfo ClientIPInfo;
        return ClientIPInfo;
    }
}

bool CConnectManager::PostMessageAll(IBuffPacket* pBuffPacket, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    m_ThreadWriteLock.acquire();
    vector<CConnectHandler*> objvecConnectManager;
    m_objHashConnectList.Get_All_Used(objvecConnectManager);
    m_ThreadWriteLock.release();

    uint32 u4ConnectID = 0;

    for(uint32 i = 0; i < (uint32)objvecConnectManager.size(); i++)
    {
        IBuffPacket* pCurrBuffPacket = App_BuffPacketManager::instance()->Create();

        if(NULL == pCurrBuffPacket)
        {
            OUR_DEBUG((LM_INFO, "[CConnectManager::PostMessage]pCurrBuffPacket is NULL.\n"));

            if(blDelete == true)
            {
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            return false;
        }

        pCurrBuffPacket->WriteStream(pBuffPacket->GetData(), pBuffPacket->GetPacketLen());

        //���뷢�Ͷ���
        _SendMessage* pSendMessage = m_SendMessagePool.Create();

        if(NULL == pSendMessage)
        {
            OUR_DEBUG((LM_ERROR,"[CConnectManager::PutMessage] new _SendMessage is error.\n"));

            if(blDelete == true)
            {
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            return false;
        }

        ACE_Message_Block* mb = pSendMessage->GetQueueMessage();

        if(NULL != mb)
        {
            pSendMessage->m_u4ConnectID = u4ConnectID;
            pSendMessage->m_pBuffPacket = pCurrBuffPacket;
            pSendMessage->m_nEvents     = u1SendType;
            pSendMessage->m_u2CommandID = u2CommandID;
            pSendMessage->m_blDelete    = blDelete;
            pSendMessage->m_u1SendState = u1SendState;
            pSendMessage->m_nMessageID  = nServerID;
            pSendMessage->m_u1Type      = 0;
            pSendMessage->m_tvSend      = ACE_OS::gettimeofday();

            //�ж϶����Ƿ����Ѿ����
            int nQueueCount = (int)msg_queue()->message_count();

            if(nQueueCount >= (int)MAX_MSG_THREADQUEUE)
            {
                OUR_DEBUG((LM_ERROR,"[CConnectManager::PutMessage] Queue is Full nQueueCount = [%d].\n", nQueueCount));

                if(blDelete == true)
                {
                    App_BuffPacketManager::instance()->Delete(pBuffPacket);
                }

                m_SendMessagePool.Delete(pSendMessage);
                return false;
            }

            ACE_Time_Value xtime = ACE_OS::gettimeofday() + ACE_Time_Value(0, MAX_MSG_PUTTIMEOUT);

            if(this->putq(mb, &xtime) == -1)
            {
                OUR_DEBUG((LM_ERROR,"[CConnectManager::PutMessage] Queue putq  error nQueueCount = [%d] errno = [%d].\n", nQueueCount, errno));

                if(blDelete == true)
                {
                    App_BuffPacketManager::instance()->Delete(pBuffPacket);
                }

                m_SendMessagePool.Delete(pSendMessage);
                return false;
            }
        }
        else
        {
            OUR_DEBUG((LM_ERROR,"[CMessageService::PutMessage] mb new error.\n"));

            if(blDelete == true)
            {
                App_BuffPacketManager::instance()->Delete(pBuffPacket);
            }

            m_SendMessagePool.Delete(pSendMessage);
            return false;
        }
    }

    return true;
}

bool CConnectManager::SetConnectName(uint32 u4ConnectID, const char* pName)
{
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if(NULL != pConnectHandler)
    {
        pConnectHandler->SetConnectName(pName);
        return true;
    }
    else
    {
        return false;
    }
}

bool CConnectManager::SetIsLog(uint32 u4ConnectID, bool blIsLog)
{
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if(NULL != pConnectHandler)
    {
        pConnectHandler->SetIsLog(blIsLog);
        return true;
    }
    else
    {
        return false;
    }
}

void CConnectManager::GetClientNameInfo(const char* pName, vecClientNameInfo& objClientNameInfo)
{
    vector<CConnectHandler*> vecConnectHandler;
    m_objHashConnectList.Get_All_Used(vecConnectHandler);

    for(int i = 0; i < (int)vecConnectHandler.size(); i++)
    {
        CConnectHandler* pConnectHandler = vecConnectHandler[i];

        if(NULL != pConnectHandler && ACE_OS::strcmp(pConnectHandler->GetConnectName(), pName) == 0)
        {
            _ClientNameInfo ClientNameInfo;
            ClientNameInfo.m_nConnectID = (int)pConnectHandler->GetConnectID();
            sprintf_safe(ClientNameInfo.m_szName, MAX_BUFF_100, "%s", pConnectHandler->GetConnectName());
            sprintf_safe(ClientNameInfo.m_szClientIP, MAX_BUFF_50, "%s", pConnectHandler->GetClientIPInfo().m_szClientIP);
            ClientNameInfo.m_nPort =  pConnectHandler->GetClientIPInfo().m_nPort;

            if(pConnectHandler->GetIsLog() == true)
            {
                ClientNameInfo.m_nLog = 1;
            }
            else
            {
                ClientNameInfo.m_nLog = 0;
            }

            objClientNameInfo.push_back(ClientNameInfo);
        }
    }
}

_CommandData* CConnectManager::GetCommandData( uint16 u2CommandID )
{
    return m_CommandAccount.GetCommandData(u2CommandID);
}

void CConnectManager::Init( uint16 u2Index )
{
    //�����̳߳�ʼ��ͳ��ģ�������
    char szName[MAX_BUFF_50] = {'\0'};
    sprintf_safe(szName, MAX_BUFF_50, "�����߳�(%d)", u2Index);
    m_CommandAccount.InitName(szName, App_MainConfig::instance()->GetMaxCommandCount());

    //��ʼ��ͳ��ģ�鹦��
    m_CommandAccount.Init(App_MainConfig::instance()->GetCommandAccount(),
                          App_MainConfig::instance()->GetCommandFlow(),
                          App_MainConfig::instance()->GetPacketTimeOut());

    //��ʼ�����ͻ���
    m_SendCacheManager.Init(App_MainConfig::instance()->GetBlockCount(), App_MainConfig::instance()->GetBlockSize());

    //��ʼ��Hash��
    uint16 u2PoolSize = App_MainConfig::instance()->GetMaxHandlerCount();
    m_objHashConnectList.Init((int)u2PoolSize);

    //��ʼ��ʱ������
    m_TimeWheelLink.Init(App_MainConfig::instance()->GetMaxConnectTime(), App_MainConfig::instance()->GetCheckAliveTime(), (int)u2PoolSize, CConnectManager::TimeWheel_Timeout_Callback, (void*)this);
}

uint32 CConnectManager::GetCommandFlowAccount()
{
    return m_CommandAccount.GetFlowOut();
}

EM_Client_Connect_status CConnectManager::GetConnectState(uint32 u4ConnectID)
{
    char szConnectID[10] = {'\0'};
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);

    if(NULL == m_objHashConnectList.Get_Hash_Box_Data(szConnectID))
    {
        return CLIENT_CONNECT_NO_EXIST;
    }
    else
    {
        return CLIENT_CONNECT_EXIST;
    }
}

int CConnectManager::handle_write_file_stream(uint32 u4ConnectID, const char* pData, uint32 u4Size, uint8 u1ParseID)
{
    char szConnectID[10] = { '\0' };
    sprintf_safe(szConnectID, 10, "%d", u4ConnectID);
    CConnectHandler* pConnectHandler = m_objHashConnectList.Get_Hash_Box_Data(szConnectID);

    if (NULL == pConnectHandler)
    {
        return -1;
    }
    else
    {
        return pConnectHandler->handle_write_file_stream(pData, u4Size, u1ParseID);
    }
}

int CConnectManager::CloseMsgQueue()
{
    // We can choose to process the message or to differ it into the message
    // queue, and process them into the svc() method. Chose the last option.
    int retval;

    ACE_Message_Block* mblk = 0;
    ACE_NEW_RETURN(mblk,ACE_Message_Block (0, ACE_Message_Block::MB_STOP),-1);

    // If queue is full, flush it before block in while
    if (msg_queue ()->is_full())
    {
        if ((retval=msg_queue ()->flush()) == -1)
        {
            OUR_DEBUG((LM_ERROR, "[CConnectManager::CloseMsgQueue]put error flushing queue\n"));
            return -1;
        }
    }

    m_mutex.acquire();

    while ((retval = putq (mblk)) == -1)
    {
        if (msg_queue ()->state () != ACE_Message_Queue_Base::PULSED)
        {
            OUR_DEBUG((LM_ERROR,ACE_TEXT("[CConnectManager::CloseMsgQueue]put Queue not activated.\n")));
            break;
        }
    }

    m_cond.wait();
    m_mutex.release();
    return retval;
}

//*********************************************************************************

CConnectHandlerPool::CConnectHandlerPool(void)
{
    //ConnectID��������1��ʼ
    m_u4CurrMaxCount = 1;
}

CConnectHandlerPool::~CConnectHandlerPool(void)
{
    OUR_DEBUG((LM_INFO, "[CConnectHandlerPool::~CConnectHandlerPool].\n"));
    Close();
    OUR_DEBUG((LM_INFO, "[CConnectHandlerPool::~CConnectHandlerPool]End.\n"));
}

void CConnectHandlerPool::Init(int nObjcetCount)
{
    Close();

    //��ʼ��HashTable
    m_objHashHandleList.Init((int)nObjcetCount);

    for(int i = 0; i < nObjcetCount; i++)
    {
        CConnectHandler* pHandler = new CConnectHandler();

        if(NULL != pHandler)
        {
            //��ID��Handlerָ��Ĺ�ϵ����hashTable
            char szHandlerID[10] = {'\0'};
            sprintf_safe(szHandlerID, 10, "%d", m_u4CurrMaxCount);
            int nHashPos = m_objHashHandleList.Add_Hash_Data(szHandlerID, pHandler);

            if(-1 != nHashPos)
            {
                pHandler->Init(i);
                pHandler->SetHashID(i);
            }

            m_u4CurrMaxCount++;
        }
    }
}

void CConnectHandlerPool::Close()
{
    //���������Ѵ��ڵ�ָ��
    vector<CConnectHandler*> vecConnectHandler;
    m_objHashHandleList.Get_All_Used(vecConnectHandler);

    for(int i = 0; i < (int)vecConnectHandler.size(); i++)
    {
        CConnectHandler* pHandler = vecConnectHandler[i];
        SAFE_DELETE(pHandler);
    }

    m_u4CurrMaxCount  = 1;
}

int CConnectHandlerPool::GetUsedCount()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    return m_objHashHandleList.Get_Count() - m_objHashHandleList.Get_Used_Count();
}

int CConnectHandlerPool::GetFreeCount()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    return m_objHashHandleList.Get_Used_Count();
}

CConnectHandler* CConnectHandlerPool::Create()
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    //��Hash���е���һ����ʹ�õ�����
    CConnectHandler* pHandler = m_objHashHandleList.Pop();

    //û�ҵ������
    return pHandler;
}

bool CConnectHandlerPool::Delete(CConnectHandler* pObject)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);

    if(NULL == pObject)
    {
        return false;
    }

    char szHandlerID[10] = {'\0'};
    sprintf_safe(szHandlerID, 10, "%d", pObject->GetHandlerID());
    bool blState = m_objHashHandleList.Push(szHandlerID, pObject);

    if(false == blState)
    {
        OUR_DEBUG((LM_INFO, "[CProConnectHandlerPool::Delete]szHandlerID=%s(0x%08x).\n", szHandlerID, pObject));
    }
    else
    {
        //OUR_DEBUG((LM_INFO, "[CProConnectHandlerPool::Delete]szHandlerID=%s(0x%08x) nPos=%d.\n", szHandlerID, pObject, nPos));
    }

    return true;
}

//==============================================================
CConnectManagerGroup::CConnectManagerGroup()
{
    m_objConnnectManagerList = NULL;
    m_u4CurrMaxCount         = 0;
    m_u2ThreadQueueCount     = SENDQUEUECOUNT;
}

CConnectManagerGroup::~CConnectManagerGroup()
{
    OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::~CConnectManagerGroup].\n"));

    Close();
}

void CConnectManagerGroup::Close()
{
    if(NULL != m_objConnnectManagerList)
    {
        for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
        {
            CConnectManager* pConnectManager = m_objConnnectManagerList[i];
            SAFE_DELETE(pConnectManager);
        }
    }

    SAFE_DELETE_ARRAY(m_objConnnectManagerList);
    m_u2ThreadQueueCount = 0;
}

void CConnectManagerGroup::Init(uint16 u2SendQueueCount)
{
    Close();

    m_objConnnectManagerList = new CConnectManager*[u2SendQueueCount];
    memset(m_objConnnectManagerList, 0, sizeof(CConnectManager*)*u2SendQueueCount);

    for(int i = 0; i < (int)u2SendQueueCount; i++)
    {
        CConnectManager* pConnectManager = new CConnectManager();

        if(NULL != pConnectManager)
        {
            //��ʼ��ͳ����
            pConnectManager->Init((uint16)i);
            //��������
            m_objConnnectManagerList[i] = pConnectManager;
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::Init]Creat %d SendQueue OK.\n", i));
        }
    }

    m_u2ThreadQueueCount = u2SendQueueCount;
}

uint32 CConnectManagerGroup::GetGroupIndex()
{
    //�������ӻ�����У��������������㷨��
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadWriteLock);
    m_u4CurrMaxCount++;
    return m_u4CurrMaxCount;
}

bool CConnectManagerGroup::AddConnect(CConnectHandler* pConnectHandler)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGrard(m_ThreadWriteLock);

    uint32 u4ConnectID = GetGroupIndex();

    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::AddConnect]No find send Queue object.\n"));
        return false;
    }

    //OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::Init]u4ConnectID=%d, u2ThreadIndex=%d.\n", u4ConnectID, u2ThreadIndex));

    return pConnectManager->AddConnect(u4ConnectID, pConnectHandler);
}

bool CConnectManagerGroup::SetConnectTimeWheel(CConnectHandler* pConnectHandler)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGrard(m_ThreadWriteLock);

    uint32 u4ConnectID = pConnectHandler->GetConnectID();

    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if (NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::AddConnect]No find send Queue object.\n"));
        return false;
    }

    //OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::Init]u4ConnectID=%d, u2ThreadIndex=%d.\n", u4ConnectID, u2ThreadIndex));

    return pConnectManager->SetConnectTimeWheel(pConnectHandler);
}

bool CConnectManagerGroup::DelConnectTimeWheel(CConnectHandler* pConnectHandler)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> WGrard(m_ThreadWriteLock);

    uint32 u4ConnectID = pConnectHandler->GetConnectID();

    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if (NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::AddConnect]No find send Queue object.\n"));
        return false;
    }

    //OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::Init]u4ConnectID=%d, u2ThreadIndex=%d.\n", u4ConnectID, u2ThreadIndex));

    return pConnectManager->DelConnectTimeWheel(pConnectHandler);
}

bool CConnectManagerGroup::PostMessage(uint32 u4ConnectID, IBuffPacket*& pBuffPacket, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager =  m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]No find send Queue object.\n"));
        return false;
    }

    //OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]u4ConnectID=%d, u2ThreadIndex=%d.\n", u4ConnectID, u2ThreadIndex));

    return pConnectManager->PostMessage(u4ConnectID, pBuffPacket, u1SendType, u2CommandID, u1SendState, blDelete, nServerID);
}

bool CConnectManagerGroup::PostMessage( uint32 u4ConnectID, const char*& pData, uint32 nDataLen, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager =  m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]No find send Queue object.\n"));

        if(blDelete == true)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }

    //OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]u4ConnectID=%d, u2ThreadIndex=%d.\n", u4ConnectID, u2ThreadIndex));
    IBuffPacket* pBuffPacket = App_BuffPacketManager::instance()->Create();

    if(NULL != pBuffPacket)
    {
        bool bWriteResult = pBuffPacket->WriteStream(pData, nDataLen);

        if(blDelete == true)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        if(bWriteResult)
        {
            return pConnectManager->PostMessage(u4ConnectID, pBuffPacket, u1SendType, u2CommandID, u1SendState, true, nServerID);
        }
        else
        {
            return false;
        }
    }
    else
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]pBuffPacket is NULL.\n"));

        if(blDelete == true)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }
}

bool CConnectManagerGroup::PostMessage( vector<uint32> vecConnectID, IBuffPacket*& pBuffPacket, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    uint32 u4ConnectID = 0;

    for(uint32 i = 0; i < (uint32)vecConnectID.size(); i++)
    {
        //�ж����е���һ���߳�������ȥ
        u4ConnectID = vecConnectID[i];
        uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

        CConnectManager* pConnectManager =  m_objConnnectManagerList[u2ThreadIndex];

        if(NULL == pConnectManager)
        {
            //OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]No find send Queue object.\n"));
            continue;
        }

        //Ϊÿһ��Connect���÷��Ͷ������ݰ�
        IBuffPacket* pCurrBuffPacket = App_BuffPacketManager::instance()->Create();

        if(NULL == pCurrBuffPacket)
        {
            continue;
        }

        pCurrBuffPacket->WriteStream(pBuffPacket->GetData(), pBuffPacket->GetWriteLen());

        if (false == pConnectManager->PostMessage(u4ConnectID, pCurrBuffPacket, u1SendType, u2CommandID, u1SendState, true, nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage](%d)PostMessage error.\n", u4ConnectID));
        }
    }

    if(true == blDelete)
    {
        App_BuffPacketManager::instance()->Delete(pBuffPacket);
    }

    return true;
}

bool CConnectManagerGroup::PostMessage( vector<uint32> vecConnectID, const char*& pData, uint32 nDataLen, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    uint32 u4ConnectID = 0;

    for(uint32 i = 0; i < (uint32)vecConnectID.size(); i++)
    {
        //�ж����е���һ���߳�������ȥ
        u4ConnectID = vecConnectID[i];
        uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

        CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

        if(NULL == pConnectManager)
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]No find send Queue object.\n"));
            continue;
        }

        //Ϊÿһ��Connect���÷��Ͷ������ݰ�
        IBuffPacket* pBuffPacket = App_BuffPacketManager::instance()->Create();

        if(NULL == pBuffPacket)
        {
            continue;
        }

        pBuffPacket->WriteStream(pData, nDataLen);

        if (false == pConnectManager->PostMessage(u4ConnectID, pBuffPacket, u1SendType, u2CommandID, u1SendState, true, nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage](%d)PostMessage error.\n", u4ConnectID));
        }
    }

    if(true == blDelete)
    {
        SAFE_DELETE_ARRAY(pData);
    }

    return true;
}

bool CConnectManagerGroup::CloseConnect(uint32 u4ConnectID)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::CloseConnect]No find send Queue object.\n"));
        return false;
    }

    //ͨ����Ϣ����ȥʵ�ֹر�
    return pConnectManager->CloseConnect_By_Queue(u4ConnectID);
}

bool CConnectManagerGroup::CloseConnectByClient(uint32 u4ConnectID)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::CloseConnectByClient]No find send Queue object.\n"));
        return false;
    }

    return pConnectManager->Close(u4ConnectID);
}

_ClientIPInfo CConnectManagerGroup::GetClientIPInfo(uint32 u4ConnectID)
{
    _ClientIPInfo objClientIPInfo;
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::GetClientIPInfo]No find send Queue object.\n"));
        return objClientIPInfo;
    }

    return pConnectManager->GetClientIPInfo(u4ConnectID);
}

_ClientIPInfo CConnectManagerGroup::GetLocalIPInfo(uint32 u4ConnectID)
{
    _ClientIPInfo objClientIPInfo;
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::GetLocalIPInfo]No find send Queue object.\n"));
        return objClientIPInfo;
    }

    return pConnectManager->GetLocalIPInfo(u4ConnectID);
}


void CConnectManagerGroup::GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo)
{
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            pConnectManager->GetConnectInfo(VecClientConnectInfo);
        }
    }
}

int CConnectManagerGroup::GetCount()
{
    uint32 u4Count = 0;

    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            u4Count += pConnectManager->GetCount();
        }
    }

    return u4Count;
}

void CConnectManagerGroup::CloseAll()
{
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            pConnectManager->CloseAll();
        }
    }
}

bool CConnectManagerGroup::StartTimer()
{
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            if (false == pConnectManager->StartTimer())
            {
                OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::StartTimer]StartTimer error.\n"));
            }
        }
    }

    return true;
}

bool CConnectManagerGroup::Close(uint32 u4ConnectID)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::GetClientIPInfo]No find send Queue object.\n"));
        return false;
    }

    return pConnectManager->Close(u4ConnectID);
}

bool CConnectManagerGroup::CloseUnLock(uint32 u4ConnectID)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::GetClientIPInfo]No find send Queue object.\n"));
        return false;
    }

    return pConnectManager->CloseUnLock(u4ConnectID);
}

const char* CConnectManagerGroup::GetError()
{
    return (char* )"";
}

void CConnectManagerGroup::SetRecvQueueTimeCost(uint32 u4ConnectID, uint32 u4TimeCost)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::GetClientIPInfo]No find send Queue object.\n"));
        return;
    }

    pConnectManager->SetRecvQueueTimeCost(u4ConnectID, u4TimeCost);
}

bool CConnectManagerGroup::PostMessageAll( IBuffPacket*& pBuffPacket, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    //ȫ��Ⱥ��
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL == pConnectManager)
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]No find send Queue object.\n"));
            continue;
        }

        if (false == pConnectManager->PostMessageAll(pBuffPacket, u1SendType, u2CommandID, u1SendState, false, nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessageAll](u2CommandID=0x%04x)PostMessageAll error.\n", u2CommandID));
        }
    }

    //�����˾�ɾ��
    if(true == blDelete)
    {
        App_BuffPacketManager::instance()->Delete(pBuffPacket);
    }

    return true;
}

bool CConnectManagerGroup::PostMessageAll( const char*& pData, uint32 nDataLen, uint8 u1SendType, uint16 u2CommandID, uint8 u1SendState, bool blDelete, int nServerID)
{
    IBuffPacket* pBuffPacket = App_BuffPacketManager::instance()->Create();

    if(NULL == pBuffPacket)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessageAll]pBuffPacket is NULL.\n"));

        if(blDelete == true)
        {
            SAFE_DELETE_ARRAY(pData);
        }

        return false;
    }
    else
    {
        pBuffPacket->WriteStream(pData, nDataLen);
    }

    //ȫ��Ⱥ��
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL == pConnectManager)
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessage]No find send Queue object.\n"));
            continue;
        }

        if (false == pConnectManager->PostMessageAll(pBuffPacket, u1SendType, u2CommandID, u1SendState, false, nServerID))
        {
            OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::PostMessageAll](u2CommandID=0x%04x)PostMessageAll error.\n", u2CommandID));
        }
    }

    App_BuffPacketManager::instance()->Delete(pBuffPacket);

    //�����˾�ɾ��
    if(true == blDelete)
    {
        SAFE_DELETE_ARRAY(pData);
    }

    return true;
}

bool CConnectManagerGroup::SetConnectName(uint32 u4ConnectID, const char* pName)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::SetConnectName]No find send Queue object.\n"));
        return false;
    }

    return pConnectManager->SetConnectName(u4ConnectID, pName);
}

bool CConnectManagerGroup::SetIsLog(uint32 u4ConnectID, bool blIsLog)
{
    //�ж����е���һ���߳�������ȥ
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::SetIsLog]No find send Queue object.\n"));
        return false;
    }

    return pConnectManager->SetIsLog(u4ConnectID, blIsLog);
}

void CConnectManagerGroup::GetClientNameInfo(const char* pName, vecClientNameInfo& objClientNameInfo)
{
    objClientNameInfo.clear();

    //ȫ������
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            pConnectManager->GetClientNameInfo(pName, objClientNameInfo);
        }
    }
}

void CConnectManagerGroup::GetCommandData( uint16 u2CommandID, _CommandData& objCommandData )
{
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            _CommandData* pCommandData = pConnectManager->GetCommandData(u2CommandID);

            if(pCommandData != NULL)
            {
                objCommandData += (*pCommandData);
            }
        }
    }
}

void CConnectManagerGroup::GetCommandFlowAccount(_CommandFlowAccount& objCommandFlowAccount)
{
    for(uint16 i = 0; i < m_u2ThreadQueueCount; i++)
    {
        CConnectManager* pConnectManager = m_objConnnectManagerList[i];

        if(NULL != pConnectManager)
        {
            uint32 u4FlowOut = pConnectManager->GetCommandFlowAccount();
            objCommandFlowAccount.m_u4FlowOut += u4FlowOut;
        }
    }
}

EM_Client_Connect_status CConnectManagerGroup::GetConnectState(uint32 u4ConnectID)
{
    //�ж����е���һ���߳�������
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if(NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::GetConnectState]No find send Queue object.\n"));
        return CLIENT_CONNECT_NO_EXIST;
    }

    return pConnectManager->GetConnectState(u4ConnectID);
}

int CConnectManagerGroup::handle_write_file_stream(uint32 u4ConnectID, const char* pData, uint32 u4Size, uint8 u1ParseID)
{
    //�ж����е���һ���߳�������
    uint16 u2ThreadIndex = u4ConnectID % m_u2ThreadQueueCount;

    CConnectManager* pConnectManager = m_objConnnectManagerList[u2ThreadIndex];

    if (NULL == pConnectManager)
    {
        OUR_DEBUG((LM_INFO, "[CConnectManagerGroup::handle_write_file_stream]No find send Queue object.\n"));
        return CLIENT_CONNECT_NO_EXIST;
    }

    return pConnectManager->handle_write_file_stream(u4ConnectID, pData, u4Size, u1ParseID);
}
