// BuffPacket.h
// ����ʵ��ģ�����
// һ��������ɾ���Ϊ�࣬����ʵ��Ŀ�ꡣ
// add by freeeyes
// 2009-02-20

#include "LoadModule.h"

CLoadModule::CLoadModule(void)
{
    m_szModulePath[0] = '\0';
}

CLoadModule::~CLoadModule(void)
{
    OUR_DEBUG((LM_INFO, "[CLoadModule::~CLoadModule].\n"));
    //Close();
}

void CLoadModule::Init(uint16 u2MaxModuleCount)
{
    Close();

    //��ʼ��HashTable
    m_objHashModuleList.Init((int)u2MaxModuleCount);
}

void CLoadModule::Close()
{
    //�滷�رյ�ǰ��Ծģ��
    //OUR_DEBUG((LM_INFO, "[CLoadModule::Close]m_mapModuleInfo.GetSize=%d.\n", m_mapModuleInfo.GetSize()))

    vector<string> obj_vecModuleName;
    vector<_ModuleInfo*> vecModuleInfo;
    m_objHashModuleList.Get_All_Used(vecModuleInfo);

    for(int i = 0; i < (int)vecModuleInfo.size(); i++)
    {
        //OUR_DEBUG((LM_INFO, "[CLoadModule::Close]active name=%s.\n", m_mapModuleInfo.GetMapDataKey(i).c_str()));
        _ModuleInfo* pModuleInfo = vecModuleInfo[i];

        if(NULL != pModuleInfo)
        {
            obj_vecModuleName.push_back(pModuleInfo->GetName());
        }
    }

    for(int i = 0; i < (int)obj_vecModuleName.size(); i++)
    {
        //ж�ز�ɾ������new��module����
        //OUR_DEBUG((LM_INFO, "[CLoadModule::Close]name=%s.\n", obj_vecModuleName[i].c_str()));
        UnLoadModule(obj_vecModuleName[i].c_str());
    }

    //OUR_DEBUG((LM_INFO, "[CLoadModule::Close]FINISH!!!!.\n"));
    m_objHashModuleList.Close();
}

bool CLoadModule::LoadModule(const char* pModulePath, const char* pModuleName, const char* pModuleParam)
{
    string strModuleName = (string)pModuleName;

    _ModuleInfo* pModuleInfo = new _ModuleInfo();

    if(NULL == pModuleInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadMoudle] new _ModuleInfo is error!\n"));
        return false;
    }

    //��¼ģ�����
    pModuleInfo->strModuleParam = (string)pModuleParam;

    //��ʼע��ģ�麯��
    if(false == LoadModuleInfo(strModuleName, pModuleInfo, pModulePath))
    {
        SAFE_DELETE(pModuleInfo);
        return false;
    }

    //���Ҵ�ģ���Ƿ��Ѿ���ע�ᣬ�������Ϣ����Ϣ����
    _ModuleInfo* pOldModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleInfo->GetName());

    if(NULL != pOldModuleInfo)
    {
        //ж�ؾɵĲ��
        ACE_OS::dlclose(pOldModuleInfo->hModule);
        SAFE_DELETE(pOldModuleInfo);
        m_objHashModuleList.Del_Hash_Data(pModuleInfo->GetName());
    }

    //��ע��ɹ���ģ�飬���뵽Hash������
    if (-1 == m_objHashModuleList.Add_Hash_Data(pModuleInfo->GetName(), pModuleInfo))
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadMoudle] m_mapModuleInfo.AddMapData error!\n"));
        SAFE_DELETE(pModuleInfo);
        return false;
    }

    //��ʼ����ģ���ʼ������
    int nRet = pModuleInfo->LoadModuleData(App_ServerObject::instance());

    if(nRet != 0)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadMoudle] strModuleName = %s, Execute Function LoadModuleData is error!\n", strModuleName.c_str()));
        SAFE_DELETE(pModuleInfo);
        return false;
    }

    OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadMoudle] Begin Load ModuleName[%s] OK!\n", pModuleInfo->GetName()));
    return true;
}

bool CLoadModule::UnLoadModule(const char* szModuleName, bool blIsDelete)
{
    OUR_DEBUG((LM_ERROR, "[CLoadModule::UnLoadModule]szResourceName=%s.\n", szModuleName));
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(szModuleName);

    if(NULL == pModuleInfo)
    {
        return false;
    }
    else
    {
        //����ʹ˹��������ж���
        pModuleInfo->UnLoadModuleData();

        //�����ӳ�һ�£���Ϊ�п������ڴ���ǰ��Ϣ�����Ա����������ӳ�һ�£���ֹ�����ı��˴���ģʽ�����ַ�ʽ�Ǻܴ����ģ�������δ������֮��
        //ACE_Time_Value tvSleep(MAX_LOADMODEL_CLOSE, 0);
        //ACE_OS::sleep(tvSleep);

        //���ģ���������������
        int nRet = ACE_OS::dlclose(pModuleInfo->hModule);

        if(true == blIsDelete)
        {
            SAFE_DELETE(pModuleInfo);
            m_objHashModuleList.Del_Hash_Data(szModuleName);
        }

        OUR_DEBUG((LM_ERROR, "[CLoadModule::UnLoadModule] Close Module=%s, nRet=%d!\n", szModuleName, nRet));

        return true;
    }
}

bool CLoadModule::MoveUnloadList(const char* szModuleName, uint32 u4UpdateIndex, uint32 u4ThreadCount, uint8 u1UnLoadState, string strModulePath, string strModuleName, string strModuleParam)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_tmModule);
    OUR_DEBUG((LM_ERROR, "[CLoadModule::MoveUnloadList]szResourceName=%s.\n", szModuleName));
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(szModuleName);

    if (NULL == pModuleInfo)
    {
        return false;
    }
    else
    {
        //����ȴ�������߳��б�
        _WaitUnloadModule objWaitUnloadModule;
        sprintf_safe((char* )szModuleName, MAX_BUFF_100, objWaitUnloadModule.m_szModuleName, MAX_BUFF_100);
        objWaitUnloadModule.m_u4UpdateIndex        = u4UpdateIndex;
        objWaitUnloadModule.m_hModule              = pModuleInfo->hModule;
        objWaitUnloadModule.UnLoadModuleData       = pModuleInfo->UnLoadModuleData;
        objWaitUnloadModule.m_u4ThreadCurrEndCount = u4ThreadCount;
        objWaitUnloadModule.m_u1UnloadState        = u1UnLoadState;
        objWaitUnloadModule.m_strModuleName        = strModuleName;
        objWaitUnloadModule.m_strModulePath        = strModulePath;
        objWaitUnloadModule.m_strModuleParam       = strModuleParam;
        m_vecWaitUnloadModule.push_back(objWaitUnloadModule);

        //ɾ������m_objHashModuleList�Ĳ����Ϣ
        m_objHashModuleList.Del_Hash_Data(pModuleInfo->GetName());
        SAFE_DELETE(pModuleInfo);
        OUR_DEBUG((LM_ERROR, "[CLoadModule::MoveUnloadList]szResourceName=%s Move Finish.\n", szModuleName));
        return true;
    }
}

int CLoadModule::UnloadListUpdate(uint32 u4UpdateIndex)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_tmModule);
    int nRet = 0;
    vector<_WaitUnloadModule>::iterator itr = m_vecWaitUnloadModule.begin();

    while (itr != m_vecWaitUnloadModule.end())
    {
        if ((*itr).m_u4UpdateIndex == u4UpdateIndex)
        {
            if ((*itr).m_u4UpdateIndex > 0)
            {
                (*itr).m_u4UpdateIndex--;
            }

            if ((*itr).m_u4UpdateIndex == 0)
            {
                //�ص��߼������UnLoadModuleData����
                (*itr).UnLoadModuleData();

                //���ղ���˿���Դ
                OUR_DEBUG((LM_ERROR, "[CLoadModule::UnloadListUpdate]szResourceName=%s UnLoad.\n", (*itr).m_szModuleName));
                ACE_OS::dlclose((*itr).m_hModule);

                //�ж��Ƿ���Ҫ���ز��
                if(2 == (*itr).m_u1UnloadState)
                {
                    //���¼���
                    App_ModuleLoader::instance()->LoadModule((*itr).m_strModulePath.c_str(), (*itr).m_strModuleName.c_str(), (*itr).m_strModuleParam.c_str());
                    nRet = 1;
                }

                //����vector�е��������
                m_vecWaitUnloadModule.erase(itr);
                return nRet;
            }
        }

        ++itr;
    }

    return nRet;
}

int CLoadModule::GetCurrModuleCount()
{
    return m_objHashModuleList.Get_Used_Count();
}

int CLoadModule::GetModulePoolCount()
{
    return m_objHashModuleList.Get_Count();
}

_ModuleInfo* CLoadModule::GetModuleInfo(const char* pModuleName)
{
    return m_objHashModuleList.Get_Hash_Box_Data(pModuleName);
}

bool CLoadModule::LoadModuleInfo(string strModuleName, _ModuleInfo* pModuleInfo, const char* pModulePath)
{
    char szModuleFile[MAX_BUFF_200] = {'\0'};

    if(NULL == pModuleInfo)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, pModuleInfo is NULL!\n", strModuleName.c_str()));
        return false;
    }

    pModuleInfo->strModulePath = (string)pModulePath;

    sprintf_safe(szModuleFile, MAX_BUFF_200, "%s%s", pModulePath, strModuleName.c_str());

    m_tmModule.acquire();

    pModuleInfo->hModule = ACE_OS::dlopen((ACE_TCHAR*)szModuleFile, RTLD_NOW);

    if(NULL == pModuleInfo->hModule || !pModuleInfo->hModule)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, pModuleInfo->hModule is NULL(%s)!\n", strModuleName.c_str(), ACE_OS::dlerror()));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->LoadModuleData = (int(*)(CServerObject*))ACE_OS::dlsym(pModuleInfo->hModule, "LoadModuleData");

    if(NULL == pModuleInfo->LoadModuleData || !pModuleInfo->LoadModuleData)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function LoadMoudle is error!\n", strModuleName.c_str()));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->UnLoadModuleData = (int(*)())ACE_OS::dlsym(pModuleInfo->hModule, "UnLoadModuleData");

    if(NULL == pModuleInfo->UnLoadModuleData || !pModuleInfo->UnLoadModuleData)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function UnloadModule is error!\n", strModuleName.c_str()));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->GetDesc = (const char* (*)())ACE_OS::dlsym(pModuleInfo->hModule, "GetDesc");

    if(NULL == pModuleInfo->GetDesc || !pModuleInfo->GetDesc)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function GetDesc is error!\n", strModuleName.c_str()));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->GetName = (const char* (*)())ACE_OS::dlsym(pModuleInfo->hModule, "GetName");

    if(NULL == pModuleInfo->GetName || !pModuleInfo->GetName)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function GetName is error!\n", strModuleName.c_str()));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->GetModuleKey = (const char* (*)())ACE_OS::dlsym(pModuleInfo->hModule, "GetModuleKey");

    if(NULL == pModuleInfo->GetModuleKey || !pModuleInfo->GetModuleKey)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function GetModuleKey is error!\n", strModuleName.c_str()));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->DoModuleMessage = (int(*)(uint16, IBuffPacket*, IBuffPacket*))ACE_OS::dlsym(pModuleInfo->hModule, "DoModuleMessage");

    if(NULL == pModuleInfo->DoModuleMessage || !pModuleInfo->DoModuleMessage)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function DoModuleMessage is error(%d)!\n", strModuleName.c_str(), errno));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->GetModuleState = (bool(*)(uint32&))ACE_OS::dlsym(pModuleInfo->hModule, "GetModuleState");

    if(NULL == pModuleInfo->DoModuleMessage || !pModuleInfo->DoModuleMessage)
    {
        OUR_DEBUG((LM_ERROR, "[CLoadModule::LoadModuleInfo] strModuleName = %s, Function GetModuleState is error(%d)!\n", strModuleName.c_str(), errno));
        m_tmModule.release();
        return false;
    }

    pModuleInfo->strModuleName = strModuleName;
    m_tmModule.release();
    return true;
}

int CLoadModule::SendModuleMessage(const char* pModuleName, uint16 u2CommandID, IBuffPacket* pBuffPacket, IBuffPacket* pReturnBuffPacket)
{
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleName);

    if(NULL != pModuleInfo)
    {
        pModuleInfo->DoModuleMessage(u2CommandID, pBuffPacket, pReturnBuffPacket);
    }

    return 0;
}

bool CLoadModule::GetModuleExist(const char* pModuleName)
{
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleName);

    if(NULL != pModuleInfo)
    {
        return true;
    }
    else
    {
        return false;
    }
}

const char* CLoadModule::GetModuleParam(const char* pModuleName)
{
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleName);

    if(NULL != pModuleInfo)
    {
        return pModuleInfo->strModuleParam.c_str();
    }
    else
    {
        return NULL;
    }
}

const char* CLoadModule::GetModuleFileName(const char* pModuleName)
{
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleName);

    if(NULL != pModuleInfo)
    {
        return pModuleInfo->strModuleName.c_str();
    }
    else
    {
        return NULL;
    }
}

const char* CLoadModule::GetModuleFilePath(const char* pModuleName)
{
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleName);

    if(NULL != pModuleInfo)
    {
        return pModuleInfo->strModulePath.c_str();
    }
    else
    {
        return NULL;
    }
}

const char* CLoadModule::GetModuleFileDesc(const char* pModuleName)
{
    _ModuleInfo* pModuleInfo = m_objHashModuleList.Get_Hash_Box_Data(pModuleName);

    if(NULL != pModuleInfo)
    {
        return pModuleInfo->GetDesc();
    }
    else
    {
        return NULL;
    }
}

uint16 CLoadModule::GetModuleCount()
{
    return (uint16)m_objHashModuleList.Get_Used_Count();
}

void CLoadModule::GetAllModuleInfo(vector<_ModuleInfo*>& vecModeInfo)
{
    vecModeInfo.clear();
    m_objHashModuleList.Get_All_Used(vecModeInfo);
}

void CLoadModule::GetAllModuleName(vector<string> vecModuleName)
{
    vecModuleName.clear();
    vector<_ModuleInfo*> vecModeInfo;
    m_objHashModuleList.Get_All_Used(vecModeInfo);

    for(int i = 0; i < (int)vecModeInfo.size(); i++)
    {
        vecModuleName.push_back((string)vecModeInfo[i]->GetName());
    }
}

