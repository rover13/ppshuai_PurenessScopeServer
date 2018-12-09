#include "ProControlListen.h"

CProControlListen::CProControlListen()
{
}

CProControlListen::~CProControlListen()
{
}

bool CProControlListen::AddListen( const char* pListenIP, uint32 u4Port, uint8 u1IPType, int nPacketParseID)
{
    bool blState = App_ProConnectAcceptManager::instance()->CheckIPInfo(pListenIP, u4Port);

    if(true == blState)
    {
        //��ǰ�����Ѿ����ڣ��������ظ�����
        OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen](%s:%d) is exist.\n", pListenIP, u4Port));
        return false;
    }

    //����һ���µ�accept����
    ProConnectAcceptor* pProConnectAcceptor = App_ProConnectAcceptManager::instance()->GetNewConnectAcceptor();

    if(NULL == pProConnectAcceptor)
    {
        OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen](%s:%d) new ConnectAcceptor error.\n", pListenIP, u4Port));
        return false;
    }

    ACE_INET_Addr listenAddr;
    //�ж�IPv4����IPv6
    int nErr = 0;

    if(u1IPType == TYPE_IPV4)
    {
        nErr = listenAddr.set(u4Port, pListenIP);
    }
    else
    {
        nErr = listenAddr.set(u4Port, pListenIP, 1, PF_INET6);
    }

    if(nErr != 0)
    {
        OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen](%s:%d)set_address error[%d].\n", pListenIP, u4Port, errno));
        return false;
    }

    //�����µļ���
    //���ü���IP��Ϣ
    pProConnectAcceptor->SetListenInfo(pListenIP, u4Port);
    pProConnectAcceptor->SetPacketParseInfoID(nPacketParseID);

    ACE_Proactor* pProactor = App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE);

    if(NULL == pProactor)
    {
        OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen]App_ProactorManager::instance()->GetAce_Proactor(REACTOR_CLIENTDEFINE) is NULL.\n"));
        return false;
    }

    int nRet = pProConnectAcceptor->open(listenAddr, 0, 1, App_MainConfig::instance()->GetBacklog(), 1, pProactor);

    if(-1 == nRet)
    {
        OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen] Listen from [%s:%d] error(%d).\n",listenAddr.get_host_addr(), listenAddr.get_port_number(), errno));
        return false;
    }

    OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen](%s:%d)Add Listen success.\n", pListenIP, u4Port));

    return true;
}

bool CProControlListen::DelListen(const char* pListenIP, uint32 u4Port)
{
    bool blState = App_ProConnectAcceptManager::instance()->CheckIPInfo(pListenIP, u4Port);

    if(false == blState)
    {
        //��ǰ�����Ѿ����ڣ��������ظ�����
        OUR_DEBUG((LM_INFO, "[CProControlListen::AddListen](%s:%d) is exist.\n", pListenIP, u4Port));
        return false;
    }

    return App_ProConnectAcceptManager::instance()->Close(pListenIP, u4Port);
}

PControlInfo CProControlListen::CreateListenSnapshot(int & nControlInfoNum)
{
	int nCount = 0;
	PControlInfo p_controlInfo = NULL;

	nControlInfoNum = 0;

	if (0 == App_ProConnectAcceptManager::instance()->GetCount())
	{
		//�����δ��������Ҫ�������ļ��л�ȡ
		nCount = App_MainConfig::instance()->GetServerPortCount();
		p_controlInfo = new ControlInfo[nCount];
		if (p_controlInfo)
		{
			for (int i = 0; i < nCount; i++)
			{
				_ServerInfo * pServerInfo = App_MainConfig::instance()->GetServerPort(i);
				if (pServerInfo != NULL)
				{
					sprintf_safe(p_controlInfo[nControlInfoNum].m_szListenIP,
						MAX_BUFF_50, "%s", pServerInfo->m_szServerIP);
					p_controlInfo[nControlInfoNum].m_u4Port = pServerInfo->m_nPort;
					nControlInfoNum++;
				}
			}
		}
	}
	else
	{
		nCount = App_ProConnectAcceptManager::instance()->GetCount();
		p_controlInfo = new ControlInfo[nCount];
		for (int i = 0; i < nCount; i++)
		{
			ProConnectAcceptor* pConnectAcceptor = App_ProConnectAcceptManager::instance()->GetConnectAcceptor(i);

			if (pConnectAcceptor != NULL)
			{
				sprintf_safe(p_controlInfo[nControlInfoNum].m_szListenIP,
					MAX_BUFF_50, "%s", pConnectAcceptor->GetListenIP());
				p_controlInfo[nControlInfoNum].m_u4Port = pConnectAcceptor->GetListenPort();
				nControlInfoNum++;
			}
		}
	}
	return p_controlInfo;
}

void CProControlListen::ReleaseListenSnapshot(PPControlInfo ppControlInfo)
{
	if (ppControlInfo && (*ppControlInfo))
	{
		delete[](*ppControlInfo);
		(*ppControlInfo) = NULL;
	}
}
uint32 CProControlListen::GetServerID()
{
    return App_MainConfig::instance()->GetServerID();
}
