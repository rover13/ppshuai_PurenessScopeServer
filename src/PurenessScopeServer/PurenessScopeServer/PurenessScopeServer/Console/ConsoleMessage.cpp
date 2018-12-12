#include "ConsoleMessage.h"

CConsoleMessage::CConsoleMessage()
{
    m_pvecConsoleKey = NULL;
    m_objConsolePromissions.Init(CONSOLECONFIG);
}

CConsoleMessage::~CConsoleMessage()
{
}

int CConsoleMessage::Dispose(ACE_Message_Block* pmb, IBuffPacket* pBuffPacket, uint8& u1OutputType)
{
    //��������
    if(NULL == pmb)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::Dispose]pmb is NULL.\n"));
        return CONSOLE_MESSAGE_FAIL;
    }

    char* pCommand = new char[(uint32)pmb->length() + 1];

    if(NULL == pCommand)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::Dispose]pCommand is NULL.\n"));
        return CONSOLE_MESSAGE_FAIL;
    }

    pCommand[(uint32)pmb->length() - 1] = '\0';

    memcpy_safe((char* )pmb->rd_ptr(), (uint32)pmb->length(), pCommand, (uint32)pmb->length());

    //ȥ������β������ֹ��
    if (pCommand[pmb->length() - 1] == '&')
    {
        pCommand[pmb->length() - 1] = '\0';
    }
    else
    {
        pCommand[pmb->length() - 3] = '\0';
    }

    //��������������и����
    if(CONSOLE_MESSAGE_SUCCESS != ParseCommand(pCommand, pBuffPacket, u1OutputType))
    {
        SAFE_DELETE_ARRAY(pCommand);
        return CONSOLE_MESSAGE_FAIL;
    }
    else
    {
        SAFE_DELETE_ARRAY(pCommand);
        return CONSOLE_MESSAGE_SUCCESS;
    }
}

int CConsoleMessage::ParsePlugInCommand(const char* pCommand, IBuffPacket* pBuffPacket)
{
    uint8 u1OutputType = 0;

    //ƴ�Ӳ����������ָ��
    char szPluginCommand[MAX_BUFF_200] = { '\0' };
    sprintf_safe(szPluginCommand, MAX_BUFF_200, "b plugin %s", pCommand);
    return ParseCommand_Plugin(szPluginCommand, pBuffPacket, u1OutputType);
}

bool CConsoleMessage::GetCommandInfo(const char* pCommand, _CommandInfo& CommandInfo)
{
    int nLen = (int)ACE_OS::strlen(pCommand);
    char szKey[MAX_BUFF_100] = {'\0'};

    AppLogManager::instance()->WriteLog(LOG_SYSTEM_CONSOLEDATA, "<Command>%s.", pCommand);

    if(nLen > MAX_BUFF_100*2 + 1)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::GetCommandInfo]pCommand is too long.\n"));
        return false;
    }

    //������ģʽ
    char szOutputType[MAX_BUFF_100] = { '\0' };
    char* pKeyBegin = ACE_OS::strstr((char*)pCommand, COMMAND_SPLIT_STRING);

    if (NULL == pKeyBegin)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::GetCommandInfo]OutputType is no find.\n"));
        return false;
    }

    memcpy_safe((char*)pCommand, (int)(pKeyBegin - pCommand), szOutputType, (int)(pKeyBegin - pCommand));
    szOutputType[(int)(pKeyBegin - pCommand)] = '\0';

    if (ACE_OS::strcmp(szOutputType, "b") == 0)
    {
        CommandInfo.m_u1OutputType = 0;
    }
    else
    {
        CommandInfo.m_u1OutputType = 1;
    }

    //���keyֵ
    char* pCommandBegin = ACE_OS::strstr((char*)pKeyBegin + ACE_OS::strlen(COMMAND_SPLIT_STRING), COMMAND_SPLIT_STRING);

    if (NULL == pCommandBegin)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::GetCommandInfo]CommandBegin is no find.\n"));
        return false;
    }

    memcpy_safe((char* )pKeyBegin + ACE_OS::strlen(COMMAND_SPLIT_STRING), (int)(pCommandBegin - pKeyBegin - ACE_OS::strlen(COMMAND_SPLIT_STRING)), szKey, (int)(pCommandBegin - pKeyBegin - ACE_OS::strlen(COMMAND_SPLIT_STRING)));
    szKey[(int)(pCommandBegin - pKeyBegin - ACE_OS::strlen(COMMAND_SPLIT_STRING))] = '\0';

    if (true == memcpy_safe(szKey, (uint32)ACE_OS::strlen(szKey), CommandInfo.m_szUser, MAX_BUFF_50))
    {
        CommandInfo.m_szUser[ACE_OS::strlen(szKey)] = '\0';
    }

    if(false == CheckConsoleKey(szKey))
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::GetCommandInfo]szKey is invalid.\n"));
        return false;
    }

    //�������ͷ
    char* pParamBegin = ACE_OS::strstr((char*)pCommandBegin + ACE_OS::strlen(COMMAND_SPLIT_STRING), COMMAND_SPLIT_STRING);

    if (NULL == pParamBegin)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::GetCommandInfo]ParamBegin is no find.\n"));
        return false;
    }

    memcpy_safe(pCommandBegin + ACE_OS::strlen(COMMAND_SPLIT_STRING), (uint32)(pParamBegin - pCommandBegin - ACE_OS::strlen(COMMAND_SPLIT_STRING)), (char*)CommandInfo.m_szCommandTitle, (uint32)(pParamBegin - pCommandBegin - (int)ACE_OS::strlen(COMMAND_SPLIT_STRING)));
    CommandInfo.m_szCommandTitle[(int)(pParamBegin - pCommandBegin - ACE_OS::strlen(COMMAND_SPLIT_STRING))] = '\0';

    //�����չ����
    memcpy_safe(pParamBegin + ACE_OS::strlen(COMMAND_SPLIT_STRING), (uint32)(nLen - (pParamBegin - pCommand - ACE_OS::strlen(COMMAND_SPLIT_STRING)) + 1), (char*)CommandInfo.m_szCommandExp, (uint32)(nLen - (pParamBegin - pCommand - (int)ACE_OS::strlen(COMMAND_SPLIT_STRING)) + 1));
    CommandInfo.m_szCommandExp[(nLen - (pParamBegin - pCommand - ACE_OS::strlen(COMMAND_SPLIT_STRING)) + 1)] = '\0';

    return true;
}

int CConsoleMessage::ParseCommand_Plugin(const char* pCommand, IBuffPacket* pBuffPacket, uint8& u1OutputType)
{
    _CommandInfo CommandInfo;

    IBuffPacket* pCurrBuffPacket = App_BuffPacketManager::instance()->Create();

    if (NULL == pCurrBuffPacket)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::ParseCommand]pCurrBuffPacket is NULL.\n"));
        return CONSOLE_MESSAGE_FAIL;
    }

    if (false == GetCommandInfo(pCommand, CommandInfo))
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::ParseCommand]pCommand format is error.\n"));
        return CONSOLE_MESSAGE_FAIL;
    }

    u1OutputType = CommandInfo.m_u1OutputType;

    //ִ������
    return DoCommand(CommandInfo, pCurrBuffPacket, pBuffPacket);
}

int CConsoleMessage::ParseCommand(const char* pCommand, IBuffPacket* pBuffPacket, uint8& u1OutputType)
{
    _CommandInfo CommandInfo;

    IBuffPacket* pCurrBuffPacket = App_BuffPacketManager::instance()->Create();

    if(NULL == pCurrBuffPacket)
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::ParseCommand]pCurrBuffPacket is NULL.\n"));
        return CONSOLE_MESSAGE_FAIL;
    }

    if(false == GetCommandInfo(pCommand, CommandInfo))
    {
        OUR_DEBUG((LM_ERROR, "[CConsoleMessage::ParseCommand]pCommand format is error.\n"));
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
        return CONSOLE_MESSAGE_FAIL;
    }

    //�жϵ�ǰ�����Ƿ����ִ��
    int nPromission = m_objConsolePromissions.Check_Promission(CommandInfo.m_szCommandTitle, CommandInfo.m_szUser);

    if (0 != nPromission)
    {
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
        return CONSOLE_MESSAGE_FAIL;
    }

    u1OutputType = CommandInfo.m_u1OutputType;

    //ִ������
    return DoCommand(CommandInfo, pCurrBuffPacket, pBuffPacket);
}

int CConsoleMessage::DoCommand(_CommandInfo& CommandInfo, IBuffPacket* pCurrBuffPacket, IBuffPacket* pReturnBuffPacket)
{
    uint16 u2ReturnCommandID = CONSOLE_COMMAND_UNKNOW;

    if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_LOADMOUDLE) == 0)
    {
        //�������ģ������֧�����أ�
        DoMessage_LoadModule(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_UNLOADMOUDLE) == 0)
    {
        //����ж��ģ�������
        DoMessage_UnLoadModule(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_RELOADMOUDLE) == 0)
    {
        //����ж��ģ�������
        DoMessage_ReLoadModule(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_CLIENTCOUNT) == 0)
    {
        //�����õ�ǰ������������
        DoMessage_ClientMessageCount(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_SHOWMOUDLE) == 0)
    {
        //������ʾ���е�ǰ�Ѿ����ص�ģ�����ƺ��ļ���
        DoMessage_ShowModule(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_COMMANDINFO) == 0)
    {
        DoMessage_CommandInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_THREADINFO) == 0)
    {
        DoMessage_WorkThreadState(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_CLIENTINFO) == 0)
    {
        DoMessage_ClientInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_FORBIDDENIP) == 0)
    {
        DoMessage_ForbiddenIP(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_FORBIDDENIPSHOW) == 0)
    {
        DoMessage_ShowForbiddenList(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_LIFTED) == 0)
    {
        DoMessage_LifedIP(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_COLSECLIENT) == 0)
    {
        DoMessage_CloseClient(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_UDPCONNECTINFO) == 0)
    {
        DoMessage_UDPClientInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_SERVERCONNECT_TCP) == 0)
    {
        DoMessage_ServerConnectTCP(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_SERVERCONNECT_UDP) == 0)
    {
        DoMessage_ServerConnectUDP(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_PROCESSINFO) == 0)
    {
        DoMessage_ShowProcessInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_CLIENTHISTORY) == 0)
    {
        DoMessage_ShowClientHisTory(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_ALLCOMMANDINFO) == 0)
    {
        DoMessage_ShowAllCommandInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SERVERINFO) == 0)
    {
        DoMessage_ShowServerInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SERVERRECONNECT) == 0)
    {
        DoMessage_ReConnectServer(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_COMMANDTIMEOUT) == 0)
    {
        DoMessage_CommandTimeout(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAHE_COMMANDTIMEOUTCLR) == 0)
    {
        DoMessage_CommandTimeoutclr(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_COMMANDDATALOG) == 0)
    {
        DoMessage_CommandDataLog(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETDEBUG) == 0)
    {
        DoMessage_SetDebug(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SHOWDEBUG) == 0)
    {
        DoMessage_ShowDebug(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETTRACKIP) == 0)
    {
        DoMessage_SetTrackIP(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETTRACECOMMAND) == 0)
    {
        DoMessage_SetTraceCommand(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_GETTRACKIPINFO) == 0)
    {
        DoMessage_GetTrackCommand(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_GETCONNECTIPINFO) == 0)
    {
        DoMessage_GetConnectIPInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_GETLOGINFO) == 0)
    {
        DoMessage_GetLogLevelInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETLOGLEVEL) == 0)
    {
        DoMessage_SetLogLevelInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_GETWTAI) == 0)
    {
        DoMessage_GetThreadAI(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_GETWTTIMEOUT) == 0)
    {
        DoMessage_GetWorkThreadTO(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETWTAI) == 0)
    {
        DoMessage_SetWorkThreadAI(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_GETNICKNAMEINFO) == 0)
    {
        DoMessage_GetNickNameInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETCONNECTLOG) == 0)
    {
        DoMessage_SetConnectLog(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_SETMAXCONNECTCOUNT) == 0)
    {
        DoMessage_SetMaxConnectCount(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_ADD_LISTEN) == 0)
    {
        DoMessage_AddListen(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSAGE_DEL_LISTEN) == 0)
    {
        DoMessage_DelListen(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSATE_SHOW_LISTEN) == 0)
    {
        DoMessage_ShowListen(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSATE_MONITOR_INFO) == 0)
    {
        DoMessage_MonitorInfo(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSATE_FILE_TEST_START) == 0)
    {
        DoMessage_TestFileStart(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSATE_FILE_TEST_STOP) == 0)
    {
        DoMessage_TestFileStop(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandTitle, CONSOLEMESSATE_SERVER_CLOSE) == 0)
    {
        //����ָ��رշ�������Ϣ������Ҫ������һ���ڴ档
        DoMessage_ServerClose(CommandInfo, pCurrBuffPacket, u2ReturnCommandID);
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
        return CONSOLE_MESSAGE_CLOSE;
    }
    else
    {
        u2ReturnCommandID = CONSOLE_COMMAND_UNKNOW;
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
        return CONSOLE_MESSAGE_FAIL;
    }

    //ƴ�ӷ������ݰ�����
    uint32 u4PacketSize = pCurrBuffPacket->GetPacketLen();

    if (u4PacketSize == 0 || CONSOLE_COMMAND_UNKNOW == u2ReturnCommandID)
    {
        u2ReturnCommandID = CONSOLE_COMMAND_UNKNOW;
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
        return CONSOLE_MESSAGE_FAIL;
    }

    if (CommandInfo.m_u1OutputType == 0)
    {
        (*pReturnBuffPacket) << u2ReturnCommandID;
        pReturnBuffPacket->WriteStream(pCurrBuffPacket->GetData(), pCurrBuffPacket->GetPacketLen());
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
    }
    else
    {
        pReturnBuffPacket->WriteStream(pCurrBuffPacket->GetData(), pCurrBuffPacket->GetPacketLen());
        App_BuffPacketManager::instance()->Delete(pCurrBuffPacket);
    }

    return CONSOLE_MESSAGE_SUCCESS;
}

bool CConsoleMessage::GetFileInfo(const char* pFile, _FileInfo& FileInfo)
{
    //���ն��Ų��
    int nBegin    = 0;
    int nEnd      = 0;
    int nPosIndex = 0;

    int nLen = (int)ACE_OS::strlen(pFile);

    for(int i = 0; i < nLen; i++)
    {
        if(pFile[i] == ',')
        {
            if(nPosIndex == 0)
            {
                //ģ��·��
                nEnd = i;
                memcpy_safe((char* )&pFile[nBegin], nEnd - nBegin, FileInfo.m_szFilePath, MAX_BUFF_100);
                FileInfo.m_szFilePath[nEnd - nBegin] = '\0';
                nBegin = i + 1;
                nPosIndex++;
            }
            else if(nPosIndex == 1)
            {
                //ģ���ļ���
                nEnd = i;
                memcpy_safe((char* )&pFile[nBegin], nEnd - nBegin, FileInfo.m_szFileName, MAX_BUFF_100);
                FileInfo.m_szFileName[nEnd - nBegin] = '\0';
                nBegin = i + 1;
                //ģ�����
                nEnd = nLen;
                memcpy_safe((char* )&pFile[nBegin], nEnd - nBegin, FileInfo.m_szFileParam, MAX_BUFF_200);
                FileInfo.m_szFileParam[nEnd - nBegin] = '\0';
                break;
            }
        }
    }

    return true;
}

bool CConsoleMessage::GetForbiddenIP(const char* pCommand, _ForbiddenIP& ForbiddenIP)
{
    char szTempData[MAX_BUFF_100] = {'\0'};

    //���IP��ַ
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-c ");
    char* pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe(pPosBegin + 3, (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    sprintf_safe(ForbiddenIP.m_szClientIP, MAX_IP_SIZE, szTempData);

    pPosBegin = (char* )ACE_OS::strstr(pCommand, "-t ");
    pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe(pPosBegin + 3, (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    ForbiddenIP.m_u1Type = (uint8)ACE_OS::atoi(szTempData);

    pPosBegin = (char* )ACE_OS::strstr(pCommand, "-s ");
    pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe(pPosBegin + 3, (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    ForbiddenIP.m_u4Second = (uint32)ACE_OS::atoi(szTempData);

    return true;
}

bool CConsoleMessage::GetTrackIP(const char* pCommand, _ForbiddenIP& ForbiddenIP)
{
    char szTempData[MAX_BUFF_100] = {'\0'};

    //���IP��ַ
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-c ");
    char* pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe(pPosBegin + 3, (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    sprintf_safe(ForbiddenIP.m_szClientIP, MAX_IP_SIZE, szTempData);

    return true;
}

bool CConsoleMessage::GetLogLevel(const char* pCommand, int& nLogLevel)
{
    //�����־�ȼ�
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-l ");

    if(NULL != pPosBegin)
    {
        char szTempData[MAX_BUFF_100] = {'\0'};
        int nLen = (int)ACE_OS::strlen(pCommand) - (int)(pPosBegin - pCommand) - 3;
        memcpy_safe(pPosBegin + 3, (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
        nLogLevel = ACE_OS::atoi(szTempData);
    }

    return true;
}

bool CConsoleMessage::GetAIInfo(const char* pCommand, int& nAI, int& nDispose, int& nCheck, int& nStop)
{
    int nIndex               = 0;
    int nBegin               = 0;
    int nEnd                 = 0;
    char szTemp[MAX_BUFF_20] = {'\0'};

    nBegin = 3;
    ACE_OS::memset(szTemp, 0, MAX_BUFF_20);

    for(int i = 3; i < (int)ACE_OS::strlen(pCommand); i++)
    {
        if(pCommand[i] == ',')
        {
            nEnd = i;
            memcpy_safe((char* )&pCommand[nBegin], (uint32)(nEnd - nBegin), szTemp, (uint32)MAX_BUFF_20);

            if(nIndex == 0)
            {
                nAI = ACE_OS::atoi(szTemp);
            }
            else if(nIndex == 1)
            {
                nDispose = ACE_OS::atoi(szTemp);
            }
            else if(nIndex == 2)
            {
                nCheck = ACE_OS::atoi(szTemp);
            }

            ACE_OS::memset(szTemp, 0, MAX_BUFF_20);
            nBegin = i + 1;
            nIndex++;
        }
    }

    //���һ������
    memcpy_safe((char* )&pCommand[nBegin], (uint32)(ACE_OS::strlen(pCommand) - nBegin), szTemp, (uint32)MAX_BUFF_20);
    nStop = ACE_OS::atoi(szTemp);

    return true;
}

bool CConsoleMessage::GetNickName(const char* pCommand, char* pName)
{
    //��ñ���
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-n ");

    if(NULL != pPosBegin)
    {
        int nLen = (int)ACE_OS::strlen(pCommand) - (int)(pPosBegin - pCommand) - 3;
        memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, pName, (uint32)MAX_BUFF_100);
        pName[nLen] = '\0';
    }

    return true;
}

bool CConsoleMessage::GetConnectID(const char* pCommand, uint32& u4ConnectID, bool& blFlag)
{
    char szTempData[MAX_BUFF_100] = {'\0'};
    int  nFlag                    = 0;

    //���ConnectID
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-n ");
    char* pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    u4ConnectID = (uint32)ACE_OS::atoi(szTempData);

    pPosBegin = (char* )ACE_OS::strstr(pCommand, "-f ");
    pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    nFlag = (int)ACE_OS::atoi(szTempData);

    if(nFlag == 0)
    {
        blFlag = false;
    }
    else
    {
        blFlag = true;
    }

    return true;
}

bool CConsoleMessage::GetMaxConnectCount(const char* pCommand, uint16& u2MaxConnectCount)
{
    char szTempData[MAX_BUFF_100] = {'\0'};

    //���ConnectID
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-n ");
    char* pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    u2MaxConnectCount = (uint16)ACE_OS::atoi(szTempData);

    return true;
}

bool CConsoleMessage::GetConnectServerID(const char* pCommand, int& nServerID)
{
    char szTempData[MAX_BUFF_100] = {'\0'};

    int nCommandLen = (int)ACE_OS::strlen(pCommand);
    //���IP��ַ
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-s ");
    int nLen = (int)(nCommandLen - (pPosBegin - pCommand) - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);

    nServerID = ACE_OS::atoi(szTempData);

    return true;
}

bool CConsoleMessage::GetListenInfo(const char* pCommand, _ListenInfo& objListenInfo)
{
    char szTempData[MAX_BUFF_100] = {'\0'};

    //���IP��ַ
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-i ");
    char* pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    sprintf_safe(objListenInfo.m_szListenIP, MAX_BUFF_20, szTempData);

    //���Port
    pPosBegin = (char* )ACE_OS::strstr(pCommand, "-p ");
    pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    objListenInfo.m_u4Port = ACE_OS::atoi(szTempData);

    //���IP����
    pPosBegin = (char* )ACE_OS::strstr(pCommand, "-t ");
    pPosEnd   = (char* )ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    objListenInfo.m_u1IPType = ACE_OS::atoi(szTempData);

    //���PacketParseID
    pPosBegin = (char*)ACE_OS::strstr(pCommand, "-n ");
    pPosEnd = (char*)ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if (nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char*)(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    objListenInfo.m_u4PacketParseID = ACE_OS::atoi(szTempData);
    return true;
}

bool CConsoleMessage::GetTestFileName(const char* pCommand, char* pFileName)
{
    int nCommandSize = (int)ACE_OS::strlen(pCommand);
    char* pPosBegin = (char*)ACE_OS::strstr(pCommand, "-f ");
    uint16 u2Len = (uint16)(nCommandSize - (int(pPosBegin - pCommand) + 3));

    if (u2Len < MAX_BUFF_200 && u2Len > 0)
    {
        memcpy_safe((char*)(pPosBegin + 3), (uint32)u2Len, pFileName, (uint32)MAX_BUFF_200);
        pFileName[u2Len] = '\0';
        return true;
    }
    else
    {
        OUR_DEBUG((LM_INFO, "[CConsoleMessage::GetTestFileName]nCommandSize=%d is more than MAX_BUFF_200 or is zero.\n", u2Len));
        return false;
    }
}

bool CConsoleMessage::GetDyeingIP(const char* pCommand, _DyeIPInfo& objDyeIPInfo)
{
    char szTempData[MAX_BUFF_100] = { '\0' };

    //���IP��ַ
    char* pPosBegin = (char*)ACE_OS::strstr(pCommand, "-i ");
    char* pPosEnd = (char*)ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if (nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char*)(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    sprintf_safe(objDyeIPInfo.m_szClientIP, MAX_BUFF_20, szTempData);

    //��õ�ǰ����
    pPosBegin = (char*)ACE_OS::strstr(pCommand, "-c ");
    pPosEnd = (char*)ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if (nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char*)(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    objDyeIPInfo.m_u2MaxCount = ACE_OS::atoi(szTempData);

    return true;
}

bool CConsoleMessage::GetDyeingCommand(const char* pCommand, _DyeCommandInfo& objDyeCommandInfo)
{
    char szCommandID[MAX_BUFF_20] = { '\0' };
    char szTempData[MAX_BUFF_100] = { '\0' };

    //���CommandID
    char* pPosBegin = (char*)ACE_OS::strstr(pCommand, "-i ");
    char* pPosEnd = (char*)ACE_OS::strstr(pPosBegin + 3, " ");
    int nLen = (int)(pPosEnd - pPosBegin - 3);

    if (nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char*)(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';

    if (szTempData[0] != '0' && szTempData[1] != 'x')
    {
        return false;
    }

    if (false == memcpy_safe(&szTempData[2], (uint32)(ACE_OS::strlen(szTempData) - 2), szCommandID, MAX_BUFF_20))
    {
        return false;
    }

    objDyeCommandInfo.m_u2CommandID = (uint16)ACE_OS::strtol(szCommandID, NULL, 16);

    //��õ�ǰ����
    pPosBegin = (char*)ACE_OS::strstr(pCommand, "-c ");
    pPosEnd = (char*)ACE_OS::strstr(pPosBegin + 3, " ");
    nLen = (int)(pPosEnd - pPosBegin - 3);

    if (nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char*)(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);
    szTempData[nLen] = '\0';
    objDyeCommandInfo.m_u2MaxCount = ACE_OS::atoi(szTempData);

    return true;
}

void CConsoleMessage::DoMessage_LoadModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    _FileInfo FileInfo;

    if(false == GetFileInfo(CommandInfo.m_szCommandExp, FileInfo))
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)1;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "Command data is Error.\n");
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    //�����ļ�MessageManager
    //��Ϣ���еĹ����߳�ͬ������һ�¸��Ե���Ϣ�б���
    if (true == App_ModuleLoader::instance()->LoadModule(FileInfo.m_szFilePath, FileInfo.m_szFileName, FileInfo.m_szFileParam) &&
        true == App_MessageServiceGroup::instance()->PutUpdateCommandMessage(0))
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)0;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "LoadModule(%s) is fail.\n", FileInfo.m_szFileName);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }
    else
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)1;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "LoadModule(%s) is OK.\n", FileInfo.m_szFileName);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_LOADMOUDLE;
}

void CConsoleMessage::DoMessage_UnLoadModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(true == App_MessageManager::instance()->UnloadModuleCommand(CommandInfo.m_szCommandExp, (uint8)1, App_MessageServiceGroup::instance()->GetWorkThreadCount()) &&
       true == App_MessageServiceGroup::instance()->PutUpdateCommandMessage(App_MessageManager::instance()->GetUpdateIndex()))
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)0;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "UnloadModule(%s) is ok.\n", CommandInfo.m_szCommandExp);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }
    else
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)1;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "UnloadModule(%s) is fail.\n", CommandInfo.m_szCommandExp);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_UNLOADMOUDLE;
}

void CConsoleMessage::DoMessage_ReLoadModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(true == App_MessageManager::instance()->UnloadModuleCommand(CommandInfo.m_szCommandExp, (uint8)2, App_MessageServiceGroup::instance()->GetWorkThreadCount()) &&
       true == App_MessageServiceGroup::instance()->PutUpdateCommandMessage(App_MessageManager::instance()->GetUpdateIndex()))
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)0;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "UnloadModule(%s) is ok.\n", CommandInfo.m_szCommandExp);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }
    else
    {
        if(NULL != pBuffPacket)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)1;
            }
            else
            {
                char szTemp[MAX_BUFF_200] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_200, "UnloadModule(%s) is false.\n", CommandInfo.m_szCommandExp);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_RELOADMOUDLE;
}

void CConsoleMessage::DoMessage_ClientMessageCount(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-c") == 0)
    {
        //-c ֻ���ص�ǰ�����������
#if WIN32
        int nActiveClient = App_ProConnectManager::instance()->GetCount();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)nActiveClient;
        }
        else
        {
            char szTemp[MAX_BUFF_200] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_200, "ActiveClient(%d).\n", nActiveClient);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

#else
        int nActiveClient = App_ConnectManager::instance()->GetCount();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)nActiveClient;
        }
        else
        {
            char szTemp[MAX_BUFF_200] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_200, "ActiveClient(%d).\n", nActiveClient);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

#endif
    }
    else if (ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-cp") == 0)
    {
        //-cp ���ص�ǰ�����������ͳ���ʣ��ɷ�����
#if WIN32
        int nActiveClient = App_ProConnectManager::instance()->GetCount();
        int nPoolClient = App_ProConnectHandlerPool::instance()->GetFreeCount();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)nActiveClient;
            (*pBuffPacket) << (uint32)nPoolClient;
            (*pBuffPacket) << App_MainConfig::instance()->GetMaxHandlerCount();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ActiveClient(%d).\nPoolClient(%d).\nMaxHandlerCount(%d).\n", nActiveClient, nPoolClient, App_MainConfig::instance()->GetMaxHandlerCount());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

#else
        int nActiveClient = App_ConnectManager::instance()->GetCount();
        int nPoolClient = App_ConnectHandlerPool::instance()->GetFreeCount();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)nActiveClient;
            (*pBuffPacket) << (uint32)nPoolClient;
            (*pBuffPacket) << App_MainConfig::instance()->GetMaxHandlerCount();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ActiveClient(%d).\nPoolClient(%d).\nMaxHandlerCount(%d).\n", nActiveClient, nPoolClient, App_MainConfig::instance()->GetMaxHandlerCount());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_CLIENTCOUNT;
}

void CConsoleMessage::DoMessage_ShowModule(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        if(App_ModuleLoader::instance()->GetCurrModuleCount() != 0)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint32)App_ModuleLoader::instance()->GetCurrModuleCount();
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleCount(%d)\n", App_ModuleLoader::instance()->GetCurrModuleCount());
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
        else
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint32)0;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleCount(%d)\n", App_ModuleLoader::instance()->GetCurrModuleCount());
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

        vector<_ModuleInfo*> vecModeInfo;
        App_ModuleLoader::instance()->GetAllModuleInfo(vecModeInfo);

        for(int i = 0; i < (int)vecModeInfo.size(); i++)
        {
            _ModuleInfo* pModuleInfo = vecModeInfo[i];

            if(NULL != pModuleInfo)
            {
                if (CommandInfo.m_u1OutputType == 0)
                {
                    VCHARS_STR strSName;
                    strSName.u1Len = (uint8)ACE_OS::strlen(pModuleInfo->GetName());
                    strSName.text = (char*)pModuleInfo->GetName();
                    (*pBuffPacket) << strSName;
                    VCHARS_STR strSModileFile;
                    strSModileFile.u1Len = (uint8)ACE_OS::strlen(pModuleInfo->strModuleName.c_str());
                    strSModileFile.text = (char*)pModuleInfo->strModuleName.c_str();
                    (*pBuffPacket) << strSModileFile;
                    VCHARS_STR strSModilePath;
                    strSModilePath.u1Len = (uint8)ACE_OS::strlen(pModuleInfo->strModulePath.c_str());
                    strSModilePath.text = (char*)pModuleInfo->strModulePath.c_str();
                    (*pBuffPacket) << strSModilePath;
                    VCHARS_STR strSModileParam;
                    strSModileParam.u1Len = (uint8)ACE_OS::strlen(pModuleInfo->strModuleParam.c_str());
                    strSModileParam.text = (char*)pModuleInfo->strModuleParam.c_str();
                    (*pBuffPacket) << strSModileParam;
                    VCHARS_STR strSModileDesc;
                    strSModileDesc.u1Len = (uint8)ACE_OS::strlen(pModuleInfo->GetDesc());
                    strSModileDesc.text = (char*)pModuleInfo->GetDesc();
                    (*pBuffPacket) << strSModileDesc;

                    char szTime[MAX_BUFF_100] = { '\0' };
                    sprintf_safe(szTime, MAX_BUFF_100, "%04d-%02d-%02d %02d:%02d:%02d", pModuleInfo->dtCreateTime.year(), pModuleInfo->dtCreateTime.month(), pModuleInfo->dtCreateTime.day(), pModuleInfo->dtCreateTime.hour(), pModuleInfo->dtCreateTime.minute(), pModuleInfo->dtCreateTime.second());
                    strSName.text = (char*)szTime;
                    strSName.u1Len = (uint8)ACE_OS::strlen(szTime);
                    (*pBuffPacket) << strSName;

                    //д��Module��ǰ״̬
                    uint32 u4ErrorID = 0;
                    uint8  u1MouduleState = 0;

                    if (true == pModuleInfo->GetModuleState(u4ErrorID))
                    {
                        u1MouduleState = 0;
                    }
                    else
                    {
                        u1MouduleState = 1;
                    }

                    (*pBuffPacket) << u1MouduleState;
                    (*pBuffPacket) << u4ErrorID;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleName(%s)\n", pModuleInfo->GetName());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleFileName(%s)\n", pModuleInfo->strModuleName.c_str());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleFilePath(%s)\n", pModuleInfo->strModulePath.c_str());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleParam(%s)\n", pModuleInfo->strModuleParam.c_str());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleDesc(%s)\n", pModuleInfo->GetDesc());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "CreeateTime(%04d-%02d-%02d %02d:%02d:%02d)\n", pModuleInfo->dtCreateTime.year(), pModuleInfo->dtCreateTime.month(), pModuleInfo->dtCreateTime.day(), pModuleInfo->dtCreateTime.hour(), pModuleInfo->dtCreateTime.minute(), pModuleInfo->dtCreateTime.second());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));

                    uint32 u4ErrorID = 0;

                    if (true == pModuleInfo->GetModuleState(u4ErrorID))
                    {
                        sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleState Run OK\n");
                        pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    }
                    else
                    {
                        sprintf_safe(szTemp, MAX_BUFF_1024, " ModuleState Run Fail\n");
                        pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    }

                    sprintf_safe(szTemp, MAX_BUFF_1024, " ModuleStateErrorID(%d)\n", u4ErrorID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

        u2ReturnCommandID = CONSOLE_COMMAND_SHOWMOUDLE;
    }
}

void CConsoleMessage::DoMessage_CommandInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    uint16 u2CommandID = (uint16)ACE_OS::strtol(CommandInfo.m_szCommandExp, NULL, 16);

    if(u2CommandID != 0)
    {
        _CommandData objCommandData;
        _CommandData objCommandDataIn;
        _CommandData objCommandDataOut;

        //�Ȳ�ѯ��������
        App_MessageServiceGroup::instance()->GetCommandData(u2CommandID, objCommandDataIn);

        if(objCommandDataIn.m_u2CommandID == u2CommandID)
        {
            objCommandData += objCommandDataIn;
        }

#if WIN32
        //��ѯ��������
        App_ProConnectManager::instance()->GetCommandData(u2CommandID, objCommandDataOut);

        if(objCommandDataOut.m_u2CommandID == u2CommandID)
        {

            objCommandData += objCommandDataOut;
        }

#else
        //��ѯ��������
        App_ConnectManager::instance()->GetCommandData(u2CommandID, objCommandDataOut);

        if(objCommandDataOut.m_u2CommandID == u2CommandID)
        {

            objCommandData += objCommandDataOut;
        }

#endif

        if(objCommandData.m_u2CommandID == u2CommandID)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint32)1;
                (*pBuffPacket) << (uint16)1;
                (*pBuffPacket) << u2CommandID;
                (*pBuffPacket) << objCommandData.m_u4CommandCount;
                (*pBuffPacket) << (uint32)objCommandData.m_u8CommandCost;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID is Find.\n");
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID(0x%08x)\n", u2CommandID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandCount(%d)\n", objCommandData.m_u4CommandCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandCost(%d)\n", objCommandData.m_u8CommandCost);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
        else
        {
            //û���ҵ�
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint32)0;
                (*pBuffPacket) << (uint16)0;
                (*pBuffPacket) << 0;
                (*pBuffPacket) << 0;
                (*pBuffPacket) << (uint32)0;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID is no Find.\n");
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }

            u2ReturnCommandID = CONSOLE_COMMAND_COMMANDINFO;
        }
    }
}

void CConsoleMessage::DoMessage_WorkThreadState(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-s") == 0)
    {
        //��õ�ǰ�����߳�״̬
        CThreadInfo* pThreadInfo = App_MessageServiceGroup::instance()->GetThreadInfo();

        if(NULL != pThreadInfo)
        {
            int nThreadCount = pThreadInfo->GetThreadCount();
            (*pBuffPacket) << (uint8)nThreadCount;

            for(int i = 0; i < nThreadCount; i++)
            {
                _ThreadInfo* pCurrThreadInfo = pThreadInfo->GetThreadInfo(i);

                if (CommandInfo.m_u1OutputType == 0)
                {
                    (*pBuffPacket) << (uint8)i;
                    (*pBuffPacket) << (uint32)pCurrThreadInfo->m_tvUpdateTime.sec();
                    (*pBuffPacket) << (uint32)pCurrThreadInfo->m_tvCreateTime.sec();
                    (*pBuffPacket) << (uint8)pCurrThreadInfo->m_u4State;
                    (*pBuffPacket) << (uint32)pCurrThreadInfo->m_u4RecvPacketCount;
                    (*pBuffPacket) << (uint16)pCurrThreadInfo->m_u2CommandID;
                    (*pBuffPacket) << (uint16)pCurrThreadInfo->m_u2PacketTime;
                    (*pBuffPacket) << (uint32)pCurrThreadInfo->m_u4CurrPacketCount;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadID(%d)\n", i);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadUpdateTime(%d)\n", pCurrThreadInfo->m_tvUpdateTime.sec());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadCreateTime(%d)\n", pCurrThreadInfo->m_tvCreateTime.sec());
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadState(%d)\n", pCurrThreadInfo->m_u4State);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadRecvPacketCount(%d)\n", pCurrThreadInfo->m_u4RecvPacketCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadCommandID(%d)\n", pCurrThreadInfo->m_u2CommandID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadPacketTime(%d)\n", pCurrThreadInfo->m_u2PacketTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ThreadCurrPacketCount(%d)\n", pCurrThreadInfo->m_u4CurrPacketCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }

            u2ReturnCommandID = CONSOLE_COMMAND_THREADINFO;
        }
    }
}

void CConsoleMessage::DoMessage_ClientInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        char szIP[MAX_BUFF_100] = {'\0'};
#ifdef WIN32
        vecClientConnectInfo VecClientConnectInfo;
        App_ProConnectManager::instance()->GetConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "Client IP Count(%d)\n", VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            if(true == ClientConnectInfo.m_blValid)
            {
                sprintf_safe(szIP, MAX_BUFF_100, "%s:%d", ClientConnectInfo.m_addrRemote.get_host_addr(), ClientConnectInfo.m_addrRemote.get_port_number());

                VCHARS_STR strSName;
                strSName.text  = szIP;
                strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                if (CommandInfo.m_u1OutputType == 0)
                {
                    (*pBuffPacket) << strSName;
                    (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                    (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client IP(%s)\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client ConnectID(%d)\n", ClientConnectInfo.m_u4ConnectID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client SendCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client RecvQueueTimeCost(%d)\n", (uint32)ClientConnectInfo.m_u8RecvQueueTimeCost);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client SendQueueTimeCost(%d)\n", (uint32)ClientConnectInfo.m_u8SendQueueTimeCost);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

#else
        vecClientConnectInfo VecClientConnectInfo;
        App_ConnectManager::instance()->GetConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "Client IP Count(%d)\n", VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            if(true == ClientConnectInfo.m_blValid)
            {
                sprintf_safe(szIP, MAX_BUFF_100, "%s:%d", ClientConnectInfo.m_addrRemote.get_host_addr(), ClientConnectInfo.m_addrRemote.get_port_number());

                VCHARS_STR strSName;
                strSName.text  = szIP;
                strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                if (CommandInfo.m_u1OutputType == 0)
                {
                    (*pBuffPacket) << strSName;
                    (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                    (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client IP(%s)\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client ConnectID(%d)\n", ClientConnectInfo.m_u4ConnectID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client SendCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client RecvQueueTimeCost(%d)\n", (uint32)ClientConnectInfo.m_u8RecvQueueTimeCost);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Client SendQueueTimeCost(%d)\n", (uint32)ClientConnectInfo.m_u8SendQueueTimeCost);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_CLIENTINFO;
}

void CConsoleMessage::DoMessage_CloseClient(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    uint32 u4ConnectID = (uint32)ACE_OS::atoi(CommandInfo.m_szCommandExp);
#ifdef WIN32
    App_ProConnectManager::instance()->CloseConnect(u4ConnectID);

    if (CommandInfo.m_u1OutputType == 0)
    {
        (*pBuffPacket) << (uint8)0;
    }
    else
    {
        char szTemp[MAX_BUFF_1024] = { '\0' };
        sprintf_safe(szTemp, MAX_BUFF_1024, "Client Close is OK\n");
        pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
    }

#else
    App_ConnectManager::instance()->CloseConnect(u4ConnectID);

    if (CommandInfo.m_u1OutputType == 0)
    {
        (*pBuffPacket) << (uint8)0;
    }
    else
    {
        char szTemp[MAX_BUFF_1024] = { '\0' };
        sprintf_safe(szTemp, MAX_BUFF_1024, "Client Close is OK\n");
        pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
    }

#endif

    u2ReturnCommandID = CONSOLE_COMMAND_COLSECLIENT;
}

void CConsoleMessage::DoMessage_ForbiddenIP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    _ForbiddenIP ForbiddenIP;

    if(GetForbiddenIP(CommandInfo.m_szCommandExp, ForbiddenIP) == true)
    {
        if(ForbiddenIP.m_u1Type == 0)
        {
            //���÷��IP
            App_ForbiddenIP::instance()->AddForeverIP(ForbiddenIP.m_szClientIP);
        }
        else
        {
            //���ʱ��IP
            App_ForbiddenIP::instance()->AddTempIP(ForbiddenIP.m_szClientIP, ForbiddenIP.m_u4Second);
        }

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)0;   //��ӳɹ�
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden is OK\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }
    else
    {
        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)1;   //���ʧ��
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden is fail\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_FORBIDDENIP;
}

void CConsoleMessage::DoMessage_ShowForbiddenList(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        VecForbiddenIP* pForeverForbiddenIP = App_ForbiddenIP::instance()->ShowForeverIP();
        VecForbiddenIP* pTempForbiddenIP = App_ForbiddenIP::instance()->ShowTemoIP();

        if(pForeverForbiddenIP == NULL || pTempForbiddenIP == NULL)
        {
            return;
        }

        uint32 u4Count = (uint32)pForeverForbiddenIP->size() + (uint32)pTempForbiddenIP->size();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << u4Count;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden Count(%d)\n", u4Count);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)pForeverForbiddenIP->size(); i++)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                VCHARS_STR strSName;
                strSName.text = pForeverForbiddenIP->at(i).m_szClientIP;
                strSName.u1Len = (uint8)ACE_OS::strlen(pForeverForbiddenIP->at(i).m_szClientIP);

                (*pBuffPacket) << strSName;
                (*pBuffPacket) << pForeverForbiddenIP->at(i).m_u1Type;
                (*pBuffPacket) << (uint32)pForeverForbiddenIP->at(i).m_tvBegin.sec();
                (*pBuffPacket) << pForeverForbiddenIP->at(i).m_u4Second;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden(%s)\n", pForeverForbiddenIP->at(i).m_szClientIP);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden Type(%d)\n", pForeverForbiddenIP->at(i).m_u1Type);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden BeginTime(%d)\n", pForeverForbiddenIP->at(i).m_tvBegin.sec());
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden IntervalTime(%d)\n", pForeverForbiddenIP->at(i).m_u4Second);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

        for(int i = 0; i < (int)pTempForbiddenIP->size(); i++)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                VCHARS_STR strSName;
                strSName.text = pTempForbiddenIP->at(i).m_szClientIP;
                strSName.u1Len = (uint8)ACE_OS::strlen(pTempForbiddenIP->at(i).m_szClientIP);

                (*pBuffPacket) << strSName;
                (*pBuffPacket) << pTempForbiddenIP->at(i).m_u1Type;
                (*pBuffPacket) << (uint32)pTempForbiddenIP->at(i).m_tvBegin.sec();
                (*pBuffPacket) << pTempForbiddenIP->at(i).m_u4Second;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden(%s)\n", pForeverForbiddenIP->at(i).m_szClientIP);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden Type(%d)\n", pForeverForbiddenIP->at(i).m_u1Type);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden BeginTime(%d)\n", pForeverForbiddenIP->at(i).m_tvBegin.sec());
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden IntervalTime(%d)\n", pForeverForbiddenIP->at(i).m_u4Second);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_FORBIDDENIPSHOW;
}

void CConsoleMessage::DoMessage_LifedIP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    App_ForbiddenIP::instance()->DelForeverIP(CommandInfo.m_szCommandExp);
    App_ForbiddenIP::instance()->DelTempIP(CommandInfo.m_szCommandExp);

    if (CommandInfo.m_u1OutputType == 0)
    {
        (*pBuffPacket) << (uint8)0;
    }
    else
    {
        char szTemp[MAX_BUFF_1024] = { '\0' };
        sprintf_safe(szTemp, MAX_BUFF_1024, "IP Forbidden cancel OK\n");
        pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
    }

    u2ReturnCommandID = CONSOLE_COMMAND_LIFTED;
}

void CConsoleMessage::DoMessage_UDPClientInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        char szIP[MAX_BUFF_100] = {'\0'};
#ifdef WIN32
        vecClientConnectInfo VecClientConnectInfo;
        App_ProUDPManager::instance()->GetClientConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "UDPClient Count(%d)\n", (uint32)VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            if(true == ClientConnectInfo.m_blValid)
            {
                sprintf_safe(szIP, MAX_BUFF_100, "0.0.0.0:0");

                if (CommandInfo.m_u1OutputType == 0)
                {
                    VCHARS_STR strSName;
                    strSName.text = szIP;
                    strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                    (*pBuffPacket) << strSName;
                    (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                    (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP IP(%s)\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP Command(%d)\n", ClientConnectInfo.m_u4ConnectID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP SendCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

#else
        vecClientConnectInfo VecClientConnectInfo;
        App_ReUDPManager::instance()->GetClientConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "UDPClient Count(%d)\n", (uint32)VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            if(true == ClientConnectInfo.m_blValid)
            {
                sprintf_safe(szIP, MAX_BUFF_100, "0.0.0.0:0");

                if (CommandInfo.m_u1OutputType == 0)
                {
                    VCHARS_STR strSName;
                    strSName.text = szIP;
                    strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                    (*pBuffPacket) << strSName;
                    (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                    (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP IP(%s)\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP Command(%d)\n", ClientConnectInfo.m_u4ConnectID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP SendCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_UDPCONNECTINFO;
}

void CConsoleMessage::DoMessage_ServerConnectTCP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        char szIP[MAX_BUFF_100] = {'\0'};
#ifdef WIN32
        vecClientConnectInfo VecClientConnectInfo;
        App_ClientProConnectManager::instance()->GetConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ServerConnect Count(%d)\n", (uint32)VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            sprintf_safe(szIP, MAX_BUFF_100, "%s:%d", ClientConnectInfo.m_addrRemote.get_host_addr(), ClientConnectInfo.m_addrRemote.get_port_number());

            VCHARS_STR strSName;
            strSName.text  = szIP;
            strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

            if (CommandInfo.m_u1OutputType == 0)
            {
                if(ClientConnectInfo.m_blValid == true)
                {
                    (*pBuffPacket) << (uint8)0;   //�����Ѵ���
                }
                else
                {
                    (*pBuffPacket) << (uint8)1;   //���Ӳ�����
                }

                (*pBuffPacket) << strSName;
                (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };

                if (ClientConnectInfo.m_blValid == true)
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Connect Active.\n");
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
                else
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Connect Disactive.\n");
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }

                sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectIP(%s)\n", szIP);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectID(%d)\n", ClientConnectInfo.m_u4ConnectID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "SendCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

#else
        vecClientConnectInfo VecClientConnectInfo;
        App_ClientReConnectManager::instance()->GetConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ServerConnect Count(%d)\n", (uint32)VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            sprintf_safe(szIP, MAX_BUFF_100, "%s:%d", ClientConnectInfo.m_addrRemote.get_host_addr(), ClientConnectInfo.m_addrRemote.get_port_number());

            VCHARS_STR strSName;
            strSName.text  = szIP;
            strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

            if(ClientConnectInfo.m_blValid == true)
            {
                (*pBuffPacket) << (uint8)0;   //�����Ѵ���
            }
            else
            {
                (*pBuffPacket) << (uint8)1;   //���Ӳ�����
            }

            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << strSName;
                (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };

                if (ClientConnectInfo.m_blValid == true)
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Connect Active.\n");
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
                else
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "Connect Disactive.\n");
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }

                sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectIP(%s)\n", szIP);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectID(%d)\n", ClientConnectInfo.m_u4ConnectID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "SendCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SERVERCONNECT_TCP;
}

void CConsoleMessage::DoMessage_ServerConnectUDP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        char szIP[MAX_BUFF_100] = {'\0'};
#ifdef WIN32
        vecClientConnectInfo VecClientConnectInfo;
        App_ClientProConnectManager::instance()->GetUDPConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "UDP Server Count(%d)\n", (uint32)VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            if(true == ClientConnectInfo.m_blValid)
            {
                sprintf_safe(szIP, MAX_BUFF_100, "0.0.0.0:0");

                if (CommandInfo.m_u1OutputType == 0)
                {
                    VCHARS_STR strSName;
                    strSName.text = szIP;
                    strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                    (*pBuffPacket) << strSName;
                    (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                    (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP IP(%s)\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer ConnectID(%d)\n", ClientConnectInfo.m_u4ConnectID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer RecvCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

#else
        vecClientConnectInfo VecClientConnectInfo;
        App_ClientReConnectManager::instance()->GetUDPConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "UDP Server Count(%d)\n", (uint32)VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            if(true == ClientConnectInfo.m_blValid)
            {
                sprintf_safe(szIP, MAX_BUFF_100, "0.0.0.0:0");

                if (CommandInfo.m_u1OutputType == 0)
                {
                    VCHARS_STR strSName;
                    strSName.text = szIP;
                    strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                    (*pBuffPacket) << strSName;
                    (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                    (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                    (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                    (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                    (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
                }
                else
                {
                    char szTemp[MAX_BUFF_1024] = { '\0' };
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDP IP(%s)\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer ConnectID(%d)\n", ClientConnectInfo.m_u4ConnectID);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer RecvCount(%d)\n", ClientConnectInfo.m_u4RecvCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer RecvCount(%d)\n", ClientConnectInfo.m_u4SendCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer AllRecvSize(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer AllSendSize(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer BeginTime(%d)\n", ClientConnectInfo.m_u4BeginTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer AliveTime(%d)\n", ClientConnectInfo.m_u4AliveTime);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    sprintf_safe(szTemp, MAX_BUFF_1024, "UDPServer RecvQueueCount(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
            }
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SERVERCONNECT_UDP;
}

void CConsoleMessage::DoMessage_ShowProcessInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        _CommandFlowAccount objCommandFlowIn;
        _CommandFlowAccount objCommandFlowOut;

        //�õ���ڵ���������ͳ��
        App_MessageServiceGroup::instance()->GetFlowInfo(objCommandFlowIn);

#ifdef WIN32  //�����windows
        //�õ����г�������ͳ��
        App_ProConnectManager::instance()->GetCommandFlowAccount(objCommandFlowOut);

        double d8CurrCpu = App_ProComputerUsageManager::instance()->GetProcessCPU_Idle();
        uint64 u4CurrMemory = App_ProComputerUsageManager::instance()->GetProcessMemorySize();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (double)d8CurrCpu;
            (*pBuffPacket) << (uint64)u4CurrMemory;
            (*pBuffPacket) << (uint8)objCommandFlowIn.m_u1FLow;
            (*pBuffPacket) << (uint32)objCommandFlowIn.m_u4FlowIn;
            (*pBuffPacket) << (uint32)objCommandFlowOut.m_u4FlowOut;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "CPUUsedRote(%g%%)\n", d8CurrCpu);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "MemorySize(%lld Bytes)\n", u4CurrMemory);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "FLowSize(%d)\n", objCommandFlowIn.m_u1FLow);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "FlowIn(%d)\n", objCommandFlowIn.m_u4FlowIn);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "FlowOut(%d)\n", objCommandFlowIn.m_u4FlowOut);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }


#else   //�����linux
        App_ConnectManager::instance()->GetCommandFlowAccount(objCommandFlowOut);

		double d8CurrCpu = GetProcessCPU_Idle_Linux();
        uint64 u4CurrMemory = GetProcessMemorySize_Linux();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (double)d8CurrCpu;
            (*pBuffPacket) << (uint64)u4CurrMemory;
            (*pBuffPacket) << (uint8)objCommandFlowIn.m_u1FLow;
            (*pBuffPacket) << (uint32)objCommandFlowIn.m_u4FlowIn;
            (*pBuffPacket) << (uint32)objCommandFlowOut.m_u4FlowOut;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "CPUUsedRote(%g%%)\n", d8CurrCpu);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "MemorySize(%lld Bytes)\n", u4CurrMemory);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "FLowSize(%d)\n", objCommandFlowIn.m_u1FLow);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "FlowIn(%d)\n", objCommandFlowIn.m_u4FlowIn);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "FlowOut(%d)\n", objCommandFlowIn.m_u4FlowOut);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_PROCESSINFO;
}

void CConsoleMessage::DoMessage_ShowClientHisTory(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        char szTime[MAX_BUFF_100] = {'\0'};

        vecIPAccount VecIPAccount;
        App_IPAccount::instance()->GetInfo(VecIPAccount);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecIPAccount.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ClientHisToryCount(%d)\n", VecIPAccount.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecIPAccount.size(); i++)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                VCHARS_STR strSName;
                strSName.text = (char*)VecIPAccount[i].m_strIP.c_str();
                strSName.u1Len = (uint8)VecIPAccount[i].m_strIP.length();

                (*pBuffPacket) << strSName;
                (*pBuffPacket) << (uint32)VecIPAccount[i].m_nCount;
                (*pBuffPacket) << (uint32)VecIPAccount[i].m_nAllCount;

                sprintf_safe(szTime, MAX_BUFF_100, "%04d-%02d-%02d %02d:%02d:%02d", VecIPAccount[i].m_dtLastTime.year(), VecIPAccount[i].m_dtLastTime.month(), VecIPAccount[i].m_dtLastTime.day(), VecIPAccount[i].m_dtLastTime.hour(), VecIPAccount[i].m_dtLastTime.minute(), VecIPAccount[i].m_dtLastTime.second());
                strSName.text = (char*)szTime;
                strSName.u1Len = (uint8)ACE_OS::strlen(szTime);

                (*pBuffPacket) << strSName;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP(%s)\n", VecIPAccount[i].m_strIP.c_str());
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "Count(%d)\n", VecIPAccount[i].m_nCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllCount(%d)\n", VecIPAccount[i].m_nAllCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllCount(%d)\n", VecIPAccount[i].m_nAllCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTime, MAX_BUFF_100, "%04d-%02d-%02d %02d:%02d:%02d", VecIPAccount[i].m_dtLastTime.year(), VecIPAccount[i].m_dtLastTime.month(), VecIPAccount[i].m_dtLastTime.day(), VecIPAccount[i].m_dtLastTime.hour(), VecIPAccount[i].m_dtLastTime.minute(), VecIPAccount[i].m_dtLastTime.second());
                sprintf_safe(szTemp, MAX_BUFF_1024, "DateTime(%s)\n", VecIPAccount[i].m_nAllCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_CLIENTHISTORY;
}

void CConsoleMessage::DoMessage_ShowAllCommandInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        CHashTable<_ModuleClient>* pHashModuleClient = App_MessageManager::instance()->GetModuleClient();

        if(pHashModuleClient != NULL)
        {
            //ͳ�Ƹ���
            uint32 u4Count = 0;
            vector<_ModuleClient*> vecModuleClient;
            pHashModuleClient->Get_All_Used(vecModuleClient);
            u4Count = (uint32)vecModuleClient.size();

            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << u4Count;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandInfoCount(%d)\n", u4Count);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }

            for(int i = 0; i < (int)vecModuleClient.size(); i++)
            {
                _ModuleClient* pModuleClient = vecModuleClient[i];

                if(NULL != pModuleClient)
                {
                    if (CommandInfo.m_u1OutputType == 0)
                    {
                        (*pBuffPacket) << (uint32)pModuleClient->m_vecClientCommandInfo.size();
                    }
                    else
                    {
                        char szTemp[MAX_BUFF_1024] = { '\0' };
                        sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleClientCount(%d)\n", pModuleClient->m_vecClientCommandInfo.size());
                        pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                    }

                    for(int j = 0; j < (int)pModuleClient->m_vecClientCommandInfo.size(); j++)
                    {
                        _ClientCommandInfo* pClientCommandInfo = pModuleClient->m_vecClientCommandInfo[j];

                        if(NULL != pClientCommandInfo)
                        {
                            if (CommandInfo.m_u1OutputType == 0)
                            {
                                VCHARS_STR strSName;
                                strSName.text = pClientCommandInfo->m_szModuleName;
                                strSName.u1Len = (uint8)ACE_OS::strlen(pClientCommandInfo->m_szModuleName);
                                (*pBuffPacket) << strSName;
                                (*pBuffPacket) << pClientCommandInfo->m_u2CommandID;
                                (*pBuffPacket) << pClientCommandInfo->m_u4Count;
                                (*pBuffPacket) << pClientCommandInfo->m_u4TimeCost;
                            }
                            else
                            {
                                char szTemp[MAX_BUFF_1024] = { '\0' };
                                sprintf_safe(szTemp, MAX_BUFF_1024, "ModuleName(%s)\n", pClientCommandInfo->m_szModuleName);
                                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID(%d)\n", pClientCommandInfo->m_u2CommandID);
                                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                                sprintf_safe(szTemp, MAX_BUFF_1024, "Count(%d)\n", pClientCommandInfo->m_u4Count);
                                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                                sprintf_safe(szTemp, MAX_BUFF_1024, "TimeCost(%d)\n", pClientCommandInfo->m_u4TimeCost);
                                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                            }
                        }
                    }
                }
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_ALLCOMMANDINFO;
}

void CConsoleMessage::DoMessage_ShowServerInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        if (CommandInfo.m_u1OutputType == 0)
        {
            VCHARS_STR strSTemp;

            //���ط�����ID
            uint16 u2SerevrID = (uint32)App_MainConfig::instance()->GetServerID();
            (*pBuffPacket) << u2SerevrID;

            //���ط���������
            strSTemp.text = (char*)App_MainConfig::instance()->GetServerName();
            strSTemp.u1Len = (uint8)ACE_OS::strlen(App_MainConfig::instance()->GetServerName());
            (*pBuffPacket) << strSTemp;

            //���ط������汾
            strSTemp.text = (char*)App_MainConfig::instance()->GetServerVersion();
            strSTemp.u1Len = (uint8)ACE_OS::strlen(App_MainConfig::instance()->GetServerVersion());
            (*pBuffPacket) << strSTemp;

            //���ؼ���ģ�����
            (*pBuffPacket) << (uint16)App_ModuleLoader::instance()->GetCurrModuleCount();

            //���ع����̸߳���
            (*pBuffPacket) << (uint16)App_MessageServiceGroup::instance()->GetThreadInfo()->GetThreadCount();

            //���ص�ǰЭ����İ汾��
            strSTemp.text = (char*)App_MainConfig::instance()->GetServerVersion();
            strSTemp.u1Len = (uint8)ACE_OS::strlen(App_MainConfig::instance()->GetPacketVersion());
            (*pBuffPacket) << strSTemp;

            //���ص�ǰ�������Ǵ�˻���С��
            if (App_MainConfig::instance()->GetCharOrder() == SYSTEM_LITTLE_ORDER)
            {
                (*pBuffPacket) << (uint8)0;     //С��
            }
            else
            {
                (*pBuffPacket) << (uint8)1;     //���
            }

            //���ص�ǰ������������
            if (App_MainConfig::instance()->GetByteOrder() == false)
            {
                (*pBuffPacket) << (uint8)0;   //��������
            }
            else
            {
                (*pBuffPacket) << (uint8)1;   //��������
            }
        }
        else
        {
            //�ı����
            char szCharOrder[MAX_BUFF_20] = { '\0' };

            if (App_MainConfig::instance()->GetByteOrder() == false)
            {
                sprintf_safe(szCharOrder, MAX_BUFF_20, "HostOrder");   //��������
            }
            else
            {
                sprintf_safe(szCharOrder, MAX_BUFF_20, "NetOrder");    //��������
            }


            char szConsoleOutput[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szConsoleOutput, MAX_BUFF_1024, "ServerID:%d\nServerName:%s\nServerVersion:%s\nServerModuleCount=%d\nWorkthreadCount:%d\nServerProtocol:%s\nCharOrder:%s\n",
                         App_MainConfig::instance()->GetServerID(),
                         App_MainConfig::instance()->GetServerName(),
                         App_MainConfig::instance()->GetServerVersion(),
                         App_ModuleLoader::instance()->GetCurrModuleCount(),
                         App_MessageServiceGroup::instance()->GetThreadInfo()->GetThreadCount(),
                         App_MainConfig::instance()->GetServerVersion(),
                         szCharOrder);

            pBuffPacket->WriteStream(szConsoleOutput, (uint32)ACE_OS::strlen(szConsoleOutput));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SERVERINFO;
}

void CConsoleMessage::DoMessage_ReConnectServer(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    int nSerevrID = 0;

    if(GetConnectServerID(CommandInfo.m_szCommandExp, nSerevrID) == true)
    {
        char szIP[MAX_BUFF_100] = {'\0'};
#ifdef WIN32  //�����windows
        App_ClientProConnectManager::instance()->ReConnect(nSerevrID);

        //��õ�ǰ����״̬
        vecClientConnectInfo VecClientConnectInfo;
        App_ClientProConnectManager::instance()->GetConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint32)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectServerCount(%d)\n", VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            sprintf_safe(szIP, MAX_BUFF_100, "%s:%d", ClientConnectInfo.m_addrRemote.get_host_addr(), ClientConnectInfo.m_addrRemote.get_port_number());

            if (CommandInfo.m_u1OutputType == 0)
            {
                VCHARS_STR strSName;
                strSName.text = szIP;
                strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                if (ClientConnectInfo.m_blValid == true)
                {
                    (*pBuffPacket) << (uint8)0;   //�����Ѵ���
                }
                else
                {
                    (*pBuffPacket) << (uint8)1;   //���Ӳ�����
                }

                (*pBuffPacket) << strSName;
                (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP(%s)\n", szIP);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));

                if (ClientConnectInfo.m_blValid == true)
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectType:Connected\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
                else
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectType:DisConnected\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }

                sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectID:(%d)\n", ClientConnectInfo.m_u4ConnectID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvCount:(%d)\n", ClientConnectInfo.m_u4RecvCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "SendCount:(%d)\n", ClientConnectInfo.m_u4SendCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllRecvSize:(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllSendSize:(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "BeginTime:(%d)\n", ClientConnectInfo.m_u4BeginTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AliveTime:(%d)\n", ClientConnectInfo.m_u4AliveTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvQueueCount:(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

#else   //�����linux
        App_ClientReConnectManager::instance()->ReConnect(nSerevrID);

        //��õ�ǰ����״̬
        vecClientConnectInfo VecClientConnectInfo;
        App_ClientReConnectManager::instance()->GetConnectInfo(VecClientConnectInfo);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)VecClientConnectInfo.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectServerCount(%d)\n", VecClientConnectInfo.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(int i = 0; i < (int)VecClientConnectInfo.size(); i++)
        {
            _ClientConnectInfo ClientConnectInfo = VecClientConnectInfo[i];

            sprintf_safe(szIP, MAX_BUFF_100, "%s:%d", ClientConnectInfo.m_addrRemote.get_host_addr(), ClientConnectInfo.m_addrRemote.get_port_number());

            if (CommandInfo.m_u1OutputType == 0)
            {
                VCHARS_STR strSName;
                strSName.text = szIP;
                strSName.u1Len = (uint8)ACE_OS::strlen(szIP);

                if (ClientConnectInfo.m_blValid == true)
                {
                    (*pBuffPacket) << (uint8)0;   //�����Ѵ���
                }
                else
                {
                    (*pBuffPacket) << (uint8)1;   //���Ӳ�����
                }

                (*pBuffPacket) << strSName;
                (*pBuffPacket) << ClientConnectInfo.m_u4ConnectID;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4SendCount;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllRecvSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4AllSendSize;
                (*pBuffPacket) << ClientConnectInfo.m_u4BeginTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4AliveTime;
                (*pBuffPacket) << ClientConnectInfo.m_u4RecvQueueCount;
                (*pBuffPacket) << ClientConnectInfo.m_u8RecvQueueTimeCost;
                (*pBuffPacket) << ClientConnectInfo.m_u8SendQueueTimeCost;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "IP(%s)\n", szIP);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));

                if (ClientConnectInfo.m_blValid == true)
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectType:Connected\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }
                else
                {
                    sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectType:DisConnected\n", szIP);
                    pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                }

                sprintf_safe(szTemp, MAX_BUFF_1024, "ConnectID:(%d)\n", ClientConnectInfo.m_u4ConnectID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvCount:(%d)\n", ClientConnectInfo.m_u4RecvCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "SendCount:(%d)\n", ClientConnectInfo.m_u4SendCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllRecvSize:(%d)\n", ClientConnectInfo.m_u4AllRecvSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AllSendSize:(%d)\n", ClientConnectInfo.m_u4AllSendSize);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "BeginTime:(%d)\n", ClientConnectInfo.m_u4BeginTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "AliveTime:(%d)\n", ClientConnectInfo.m_u4AliveTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "RecvQueueCount:(%d)\n", ClientConnectInfo.m_u4RecvQueueCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SERVERRECONNECT;
}

void CConsoleMessage::DoMessage_CommandTimeout(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        vecCommandTimeOut CommandTimeOutList;
        App_MessageServiceGroup::instance()->GetCommandTimeOut(CommandTimeOutList);
        uint32 u4Count = (uint32)CommandTimeOutList.size();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << u4Count;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "CommandTimeoutCount(%d)\n", u4Count);
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for(uint32 i = 0; i < u4Count; i++)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << CommandTimeOutList[i].m_u2CommandID;
                (*pBuffPacket) << (uint32)CommandTimeOutList[i].m_tvTime.sec();
                (*pBuffPacket) << (uint32)CommandTimeOutList[i].m_u4TimeOutTime;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID(%d)\n", CommandTimeOutList[i].m_u2CommandID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "Time(%d)\n", CommandTimeOutList[i].m_tvTime.sec());
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "TimeOutTime(%d)\n", CommandTimeOutList[i].m_u4TimeOutTime);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_COMMANDTIMEOUT;
}

void CConsoleMessage::DoMessage_CommandTimeoutclr(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        App_MessageServiceGroup::instance()->ClearCommandTimeOut();

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)0;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(OK)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_COMMANDTIMEOUTCLR;
}

void CConsoleMessage::DoMessage_CommandDataLog(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        //�洢���н���ͳ����־
        App_MessageServiceGroup::instance()->SaveCommandDataLog();

        if (CommandInfo.m_u1OutputType == 0)
        {
            //�洢���з���ͳ����־
            (*pBuffPacket) << (uint8)0;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(OK)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_COMMANDDATALOG;
}

void CConsoleMessage::DoMessage_SetDebug(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    uint8 u1Debug = 0;

    if(GetDebug(CommandInfo.m_szCommandExp, u1Debug) == true)
    {
        if(u1Debug == DEBUG_OFF)
        {
            App_MainConfig::instance()->SetDebug(DEBUG_OFF);
        }
        else
        {
            App_MainConfig::instance()->SetDebug(DEBUG_ON);
        }

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)0;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(OK)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SETDEBUG;
}

void CConsoleMessage::DoMessage_ShowDebug(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)App_MainConfig::instance()->GetDebug();
            (*pBuffPacket) << (uint8)0;
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "DebugState(%d)\n", App_MainConfig::instance()->GetDebug());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(OK)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SHOWDEBUG;
}

bool CConsoleMessage::SetConsoleKey(vecConsoleKey* pvecConsoleKey)
{
    m_pvecConsoleKey = pvecConsoleKey;
    return true;
}

bool CConsoleMessage::CheckConsoleKey( const char* pKey )
{
    if(NULL == m_pvecConsoleKey)
    {
        return false;
    }

    for(int i = 0; i < (int)m_pvecConsoleKey->size(); i++)
    {
        if(ACE_OS::strcmp((*m_pvecConsoleKey)[i].m_szKey, pKey) == 0)
        {
            //keyֵ������
            return true;
        }
    }

    return false;
}

bool CConsoleMessage::GetDebug(const char* pCommand, uint8& u1Debug)
{
    char szTempData[MAX_BUFF_100] = {'\0'};

    int nCommandLen = (int)ACE_OS::strlen(pCommand);
    //���IP��ַ
    char* pPosBegin = (char* )ACE_OS::strstr(pCommand, "-s ");
    int nLen = (int)(nCommandLen - (pPosBegin - pCommand) - 3);

    if(nLen >= MAX_BUFF_100 || nLen < 0)
    {
        return false;
    }

    memcpy_safe((char* )(pPosBegin + 3), (uint32)nLen, szTempData, (uint32)MAX_BUFF_100);

    u1Debug = (uint8)ACE_OS::atoi(szTempData);

    return true;
}

void CConsoleMessage::DoMessage_SetTrackIP(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    _DyeIPInfo objDyeIPInfo;

    if(true == GetDyeingIP(CommandInfo.m_szCommandExp, objDyeIPInfo))
    {
        if (CommandInfo.m_u1OutputType == 0)
        {
            //����IPȾɫ
            App_MessageServiceGroup::instance()->AddDyringIP(objDyeIPInfo.m_szClientIP, objDyeIPInfo.m_u2MaxCount);
            (*pBuffPacket) << (uint8)0;   //׷�ٳɹ�
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(OK)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }
    else
    {
        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)1;   //׷��ʧ��
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(FAIL)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SETTRACKIP;
}

void CConsoleMessage::DoMessage_SetTraceCommand(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    _DyeCommandInfo objDyeCommandInfo;

    if(true == GetDyeingCommand(CommandInfo.m_szCommandExp, objDyeCommandInfo))
    {
        //���׷��(�˹����۲��ṩ)
        if (true == App_MessageServiceGroup::instance()->AddDyeingCommand(objDyeCommandInfo.m_u2CommandID, objDyeCommandInfo.m_u2MaxCount))
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)0;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "State(OK)\n");
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
        else
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint8)1;   //����ʧ��
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "State(FAIL)\n");
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }

    }
    else
    {
        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)1;   //����ʧ��
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "State(FAIL)\n");
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SETTRACECOMMAND;
}

void CConsoleMessage::DoMessage_GetTrackCommand(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    vec_Dyeing_Command_list objList;

    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        //��¼�ܸ���(�˹����۲��ṩ)
        App_MessageServiceGroup::instance()->GetDyeingCommand(objList);

        if (CommandInfo.m_u1OutputType == 0)
        {
            (*pBuffPacket) << (uint8)objList.size();
        }
        else
        {
            char szTemp[MAX_BUFF_1024] = { '\0' };
            sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID Count(%d)\n", objList.size());
            pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
        }

        for (int i = 0; i < (int)objList.size(); i++)
        {
            if (CommandInfo.m_u1OutputType == 0)
            {
                (*pBuffPacket) << (uint16)objList[i].m_u2CommandID;
                (*pBuffPacket) << (uint16)objList[i].m_u2CurrCount;
                (*pBuffPacket) << (uint16)objList[i].m_u2MaxCount;
            }
            else
            {
                char szTemp[MAX_BUFF_1024] = { '\0' };
                sprintf_safe(szTemp, MAX_BUFF_1024, "CommandID ID(%d)\n", objList[i].m_u2CommandID);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "CurrCount ID(%d)\n", objList[i].m_u2CurrCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
                sprintf_safe(szTemp, MAX_BUFF_1024, "MaxCount ID(%d)\n", objList[i].m_u2MaxCount);
                pBuffPacket->WriteStream(szTemp, (uint32)ACE_OS::strlen(szTemp));
            }
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_GETTRACKCOMMAND;
}

void CConsoleMessage::DoMessage_GetConnectIPInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    int nConnectID = 0;

    if(GetConnectServerID(CommandInfo.m_szCommandExp, nConnectID) == true)
    {
#ifdef WIN32  //�����windows
        _ClientIPInfo objClientIPInfo = App_ProConnectManager::instance()->GetClientIPInfo((uint32)nConnectID);
#else
        _ClientIPInfo objClientIPInfo = App_ConnectManager::instance()->GetClientIPInfo((uint32)nConnectID);
#endif


        if(ACE_OS::strlen(objClientIPInfo.m_szClientIP) == 0)
        {
            //û���ҵ���Ӧ��IP��Ϣ
            (*pBuffPacket) << (uint16)1;
        }
        else
        {
            //�ҵ��˶�Ӧ��IP��Ϣ
            (*pBuffPacket) << (uint16)0;

            VCHARS_STR strSName;
            strSName.text  = objClientIPInfo.m_szClientIP;
            strSName.u1Len = (uint8)ACE_OS::strlen(objClientIPInfo.m_szClientIP);

            (*pBuffPacket) << strSName;                         //IP
            (*pBuffPacket) << (uint32)objClientIPInfo.m_nPort;  //�˿�
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_GETCONNECTIPINFO;
}

void CConsoleMessage::DoMessage_GetLogLevelInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        (*pBuffPacket) << AppLogManager::instance()->GetLogCount();
        (*pBuffPacket) << AppLogManager::instance()->GetCurrLevel();

        for(uint16 i = 0; i < (uint16)AppLogManager::instance()->GetLogCount(); i++)
        {
            uint16 u2LogID = AppLogManager::instance()->GetLogID(i);

            (*pBuffPacket) << u2LogID;

            char* pServerName = AppLogManager::instance()->GetLogInfoByServerName(u2LogID);

            if(NULL == pServerName)
            {
                //�������������Ϊ����ֱ�ӷ���
                return;
            }

            VCHARS_STR strSName;
            strSName.text  = pServerName;
            strSName.u1Len = (uint8)ACE_OS::strlen(pServerName);

            (*pBuffPacket) << strSName;

            char* pLogName = AppLogManager::instance()->GetLogInfoByLogName(u2LogID);

            if(NULL == pLogName)
            {
                //�������������Ϊ����ֱ�ӷ���
                return;
            }

            strSName.text  = pLogName;
            strSName.u1Len = (uint8)ACE_OS::strlen(pLogName);

            (*pBuffPacket) << strSName;

            uint8 u1LogType = (uint8)AppLogManager::instance()->GetLogInfoByLogDisplay(u2LogID);

            (*pBuffPacket) << u1LogType;
            (*pBuffPacket) << AppLogManager::instance()->GetLogInfoByLogLevel(i);
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_GETLOGINFO;
}

void CConsoleMessage::DoMessage_SetLogLevelInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    int nLogLevel = 1;

    if(GetLogLevel(CommandInfo.m_szCommandExp, nLogLevel) == true)
    {
        AppLogManager::instance()->ResetLogData(nLogLevel);

        (*pBuffPacket) << (uint32)0;
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SETLOGLEVEL;
}

void CConsoleMessage::DoMessage_GetThreadAI(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        vecWorkThreadAIInfo objvecWorkThreadAIInfo;
        App_MessageServiceGroup::instance()->GetWorkThreadAIInfo(objvecWorkThreadAIInfo);

        uint16 u2ThreadCount = (uint16)objvecWorkThreadAIInfo.size();
        (*pBuffPacket) << (uint16)u2ThreadCount;

        for(uint16 i = 0; i < u2ThreadCount; i++)
        {
            (*pBuffPacket) << objvecWorkThreadAIInfo[i].m_u4ThreadID;
            (*pBuffPacket) << objvecWorkThreadAIInfo[i].m_u1WTAI;
            (*pBuffPacket) << objvecWorkThreadAIInfo[i].m_u4DisposeTime;
            (*pBuffPacket) << objvecWorkThreadAIInfo[i].m_u4WTCheckTime;
            (*pBuffPacket) << objvecWorkThreadAIInfo[i].m_u4WTTimeoutCount;
            (*pBuffPacket) << objvecWorkThreadAIInfo[i].m_u4WTStopTime;
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_GETWTAI;
}

void CConsoleMessage::DoMessage_GetWorkThreadTO(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        vecCommandTimeout objTimeout;
        vecCommandTimeout objTimeoutF;
        App_MessageServiceGroup::instance()->GetAITO(objTimeout);

        uint16 u2ThreadCount = (uint16)objTimeout.size();
        (*pBuffPacket) << (uint16)u2ThreadCount;

        for(uint16 i = 0; i < u2ThreadCount; i++)
        {
            (*pBuffPacket) << objTimeout[i].m_u4ThreadID;
            (*pBuffPacket) << objTimeout[i].m_u2CommandID;
            (*pBuffPacket) << objTimeout[i].m_u4Second;
            (*pBuffPacket) << objTimeout[i].m_u4Timeout;
        }

        App_MessageServiceGroup::instance()->GetAITF(objTimeoutF);

        u2ThreadCount = (uint16)objTimeoutF.size();
        (*pBuffPacket) << (uint16)u2ThreadCount;

        for(uint16 i = 0; i < u2ThreadCount; i++)
        {
            (*pBuffPacket) << objTimeoutF[i].m_u4ThreadID;
            (*pBuffPacket) << objTimeoutF[i].m_u2CommandID;
            (*pBuffPacket) << objTimeoutF[i].m_u4Second;
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_GETWTTIMEOUT;
}

void CConsoleMessage::DoMessage_SetWorkThreadAI(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    int nAI      = 0;
    int nDispose = 0;
    int nCheck   = 0;
    int nStop    = 0;

    if(GetAIInfo(CommandInfo.m_szCommandExp, nAI, nDispose, nCheck, nStop) == true)
    {
        //�淶������
        if(nDispose > 0 && nCheck > 0 && nStop > 0)
        {
            if(nAI > 0)
            {
                nAI = 1;
            }
            else
            {
                nAI = 0;
            }

            App_MessageServiceGroup::instance()->SetAI((uint8)nAI, (uint32)nDispose, (uint32)nCheck, (uint32)nStop);
        }
    }

    (*pBuffPacket) << (uint32)0;

    u2ReturnCommandID = CONSOLE_COMMAND_SETWTAI;
}

void CConsoleMessage::DoMessage_GetNickNameInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    char szNickName[MAX_BUFF_100] = {'\0'};
    vecClientNameInfo objClientNameInfo;

    if(GetNickName(CommandInfo.m_szCommandExp, szNickName) == true)
    {
#ifdef WIN32
        App_ProConnectManager::instance()->GetClientNameInfo(szNickName, objClientNameInfo);
#else
        App_ConnectManager::instance()->GetClientNameInfo(szNickName, objClientNameInfo);
#endif

        //������Ϣ�б�
        (*pBuffPacket) << (uint32)objClientNameInfo.size();

        for(uint32 i = 0; i < objClientNameInfo.size(); i++)
        {
            VCHARS_STR strIP;
            strIP.text  = objClientNameInfo[i].m_szClientIP;
            strIP.u1Len = (uint8)ACE_OS::strlen(objClientNameInfo[i].m_szClientIP);

            VCHARS_STR strName;
            strName.text  = objClientNameInfo[i].m_szName;
            strName.u1Len = (uint8)ACE_OS::strlen(objClientNameInfo[i].m_szName);

            (*pBuffPacket) << (uint32)objClientNameInfo[i].m_nConnectID;
            (*pBuffPacket) << strIP;
            (*pBuffPacket) << (uint32)objClientNameInfo[i].m_nPort;
            (*pBuffPacket) << strName;
            (*pBuffPacket) << (uint8)objClientNameInfo[i].m_nLog;
        }
    }

    u2ReturnCommandID = CONSOLE_COMMAND_GETNICKNAMEINFO;
}

void CConsoleMessage::DoMessage_SetConnectLog(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    uint32 u4ConnectID = 0;
    bool blIsLog       = false;

    if(GetConnectID(CommandInfo.m_szCommandExp, u4ConnectID, blIsLog) == true)
    {
#ifdef WIN32
        App_ProConnectManager::instance()->SetIsLog(u4ConnectID, blIsLog);
#else
        App_ConnectManager::instance()->SetIsLog(u4ConnectID, blIsLog);
#endif
    }

    (*pBuffPacket) << (uint32)0;

    u2ReturnCommandID = CONSOLE_COMMAND_SETCONNECTLOG;
}

void CConsoleMessage::DoMessage_SetMaxConnectCount(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    uint16 u2MaxConnectHandler = 0;

    if(GetMaxConnectCount(CommandInfo.m_szCommandExp, u2MaxConnectHandler) == true)
    {
        if(u2MaxConnectHandler > 0)
        {
            App_MainConfig::instance()->SetMaxHandlerCount(u2MaxConnectHandler);
        }
    }

    (*pBuffPacket) << (uint32)0;

    u2ReturnCommandID = CONSOLE_COMMAND_SETMAXCONNECTCOUNT;
}

void CConsoleMessage::DoMessage_AddListen(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    _ListenInfo objListenInfo;

    if(GetListenInfo(CommandInfo.m_szCommandExp, objListenInfo) == true)
    {
#ifdef WIN32
        bool blState = App_ProControlListen::instance()->AddListen(objListenInfo.m_szListenIP,
                       objListenInfo.m_u4Port,
                       objListenInfo.m_u1IPType,
                       objListenInfo.m_u4PacketParseID);

        if(true == blState)
        {
            (*pBuffPacket) << (uint32)0;
        }
        else
        {
            (*pBuffPacket) << (uint32)1;
        }

#else
        bool blState = App_ControlListen::instance()->AddListen(objListenInfo.m_szListenIP,
                       objListenInfo.m_u4Port,
                       objListenInfo.m_u1IPType,
                       objListenInfo.m_u4PacketParseID);

        if(true == blState)
        {
            (*pBuffPacket) << (uint32)0;
        }
        else
        {
            (*pBuffPacket) << (uint32)1;
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_ADD_LISTEN;
}

void CConsoleMessage::DoMessage_DelListen(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    _ListenInfo objListenInfo;

    if(GetListenInfo(CommandInfo.m_szCommandExp, objListenInfo) == true)
    {
#ifdef WIN32
        bool blState = App_ProControlListen::instance()->DelListen(objListenInfo.m_szListenIP,
                       objListenInfo.m_u4Port);

        if(true == blState)
        {
            (*pBuffPacket) << (uint32)0;
        }
        else
        {
            (*pBuffPacket) << (uint32)1;
        }

#else
        bool blState = App_ControlListen::instance()->DelListen(objListenInfo.m_szListenIP,
                       objListenInfo.m_u4Port);

        if(true == blState)
        {
            (*pBuffPacket) << (uint32)0;
        }
        else
        {
            (*pBuffPacket) << (uint32)1;
        }

#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_DEL_LISTEN;
}

void CConsoleMessage::DoMessage_ShowListen(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
#ifdef WIN32
		int nControlInfoNum = 0;
		PControlInfo pControlInfo = App_ProControlListen::instance()->CreateListenSnapshot(nControlInfoNum);
		if (pControlInfo)
		{
			if (nControlInfoNum > 0)
			{
				(*pBuffPacket) << (uint32)nControlInfoNum;
				for (int i = 0; i < nControlInfoNum; i++)
				{
					VCHARS_STR strIP;
					strIP.text = pControlInfo[i].m_szListenIP;
					strIP.u1Len = (uint8)ACE_OS::strlen(pControlInfo[i].m_szListenIP);

					(*pBuffPacket) << strIP;
					(*pBuffPacket) << pControlInfo[i].m_u4Port;
				}
			}
			App_ProControlListen::instance()->ReleaseListenSnapshot(&pControlInfo);
		}
#else
		int nControlInfoNum = 0;
		PControlInfo pControlInfo = App_ProControlListen::instance()->CreateListenSnapshot(nControlInfoNum);
		if (pControlInfo)
		{
			if (nControlInfoNum > 0)
			{
				(*pBuffPacket) << (uint32)nControlInfoNum;
				for (int i = 0; i < nControlInfoNum; i++)
				{
					VCHARS_STR strIP;
					strIP.text = pControlInfo[i].m_szListenIP;
					strIP.u1Len = (uint8)ACE_OS::strlen(pControlInfo[i].m_szListenIP);

					(*pBuffPacket) << strIP;
					(*pBuffPacket) << pControlInfo[i].m_u4Port;
				}
			}
			App_ProControlListen::instance()->ReleaseListenSnapshot(&pControlInfo);
		}
#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_SHOW_LISTEN;
}

void CConsoleMessage::DoMessage_MonitorInfo(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        _CommandFlowAccount objCommandFlowIn;
        _CommandFlowAccount objCommandFlowOut;

        //�õ���ڵ���������ͳ��
        App_MessageServiceGroup::instance()->GetFlowInfo(objCommandFlowIn);

#if WIN32
        //�õ����г�������ͳ��
        App_ProConnectManager::instance()->GetCommandFlowAccount(objCommandFlowOut);

        int nActiveClient = App_ProConnectManager::instance()->GetCount();
        int nPoolClient   = App_ProConnectHandlerPool::instance()->GetFreeCount();
        (*pBuffPacket) << (uint32)nActiveClient;
        (*pBuffPacket) << (uint32)nPoolClient;
        (*pBuffPacket) << (uint16)App_MainConfig::instance()->GetMaxHandlerCount();
        (*pBuffPacket) << (uint16)App_ConnectAccount::instance()->GetCurrConnect();
        (*pBuffPacket) << objCommandFlowIn.m_u4FlowIn;
        (*pBuffPacket) << objCommandFlowOut.m_u4FlowOut;
#else
        //�õ����г�������ͳ��
        App_ConnectManager::instance()->GetCommandFlowAccount(objCommandFlowOut);

        int nActiveClient = App_ConnectManager::instance()->GetCount();
        int nPoolClient   = App_ConnectHandlerPool::instance()->GetFreeCount();
        (*pBuffPacket) << (uint32)nActiveClient;
        (*pBuffPacket) << (uint32)nPoolClient;
        (*pBuffPacket) << (uint16)App_MainConfig::instance()->GetMaxHandlerCount();
        (*pBuffPacket) << (uint16)App_ConnectAccount::instance()->GetCurrConnect();
        (*pBuffPacket) << objCommandFlowIn.m_u4FlowIn;
        (*pBuffPacket) << objCommandFlowOut.m_u4FlowOut;
#endif
    }

    u2ReturnCommandID = CONSOLE_COMMAND_MONITOR_INFO;
}

void CConsoleMessage::DoMessage_ServerClose(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if(NULL == pBuffPacket)
    {
        return;
    }

    if(ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0 && App_MainConfig::instance()->GetServerClose() == 0)
    {
        u2ReturnCommandID = CONSOLE_COMMAND_CLOSE_SERVER;
        App_ServerObject::instance()->GetServerManager()->Close();
    }
}

void CConsoleMessage::DoMessage_TestFileStart(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    char szFileName[MAX_BUFF_200] = { '\0' };

    if (GetTestFileName(CommandInfo.m_szCommandExp, szFileName) == true)
    {
        OUR_DEBUG((LM_INFO, "[CConsoleMessage::Do_Message_TestFileStart]file=%s.\n", szFileName));
        u2ReturnCommandID = CONSOLE_COMMAND_FILE_TEST_START;

        FileTestResultInfoSt objFileResult;
        /*
        //���Դ���
        objFileResult.n4Result = 0;
        objFileResult.n4TimeInterval = 2000;
        objFileResult.n4ProNum = 10;
        objFileResult.vecProFileDesc.push_back((string)"Test freeeyes");
        objFileResult.vecProFileDesc.push_back((string)"Test liuchao");
        */
        objFileResult = App_FileTestManager::instance()->FileTestStart(szFileName);
        (*pBuffPacket) << objFileResult.n4Result;
        (*pBuffPacket) << objFileResult.n4TimeInterval;
        (*pBuffPacket) << objFileResult.n4ProNum;
        (*pBuffPacket) << (uint16)objFileResult.vecProFileDesc.size();

        for (uint16 i = 0; i < (uint16)objFileResult.vecProFileDesc.size(); i++)
        {
            (*pBuffPacket) << objFileResult.vecProFileDesc[i];
        }
    }
    else
    {
        u2ReturnCommandID = CONSOLE_COMMAND_FILE_TEST_START;
        (*pBuffPacket) << (int)-1;
    }
}

void CConsoleMessage::DoMessage_TestFileStop(_CommandInfo& CommandInfo, IBuffPacket* pBuffPacket, uint16& u2ReturnCommandID)
{
    if (ACE_OS::strcmp(CommandInfo.m_szCommandExp, "-a") == 0)
    {
        u2ReturnCommandID = CONSOLE_COMMAND_FILE_TEST_STOP;
        int nRet = App_FileTestManager::instance()->FileTestEnd();
        (*pBuffPacket) << (uint32)nRet;
    }
}

