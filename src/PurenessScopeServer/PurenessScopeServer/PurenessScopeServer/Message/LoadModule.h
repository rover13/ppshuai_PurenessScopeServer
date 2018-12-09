#ifndef _LOADMODULE_H
#define _LOADMODULE_H

#include "ace/Date_Time.h"
#include "ace/Thread_Mutex.h"
#include "ace/Singleton.h"
#include "ace/OS_NS_dlfcn.h"
#include "IObject.h"
#include "HashTable.h"

#include <string>
#include <vector>

using namespace std;

struct _ModuleInfo
{
    string           strModuleName;         //ģ���ļ�����
    string           strModulePath;         //ģ��·��
    string           strModuleParam;        //ģ����������
    ACE_Date_Time    dtCreateTime;          //ģ�鴴��ʱ��
    ACE_SHLIB_HANDLE hModule;
    int (*LoadModuleData)(CServerObject* pServerObject);
    int (*UnLoadModuleData)(void);
    const char* (*GetDesc)(void);
    const char* (*GetName)(void);
    const char* (*GetModuleKey)(void);
    int (*DoModuleMessage)(uint16 u2CommandID, IBuffPacket* pBuffPacket, IBuffPacket* pReturnBuffPacket);
    bool (*GetModuleState)(uint32& u4AErrorID);

    _ModuleInfo()
    {
        hModule           = NULL;
        LoadModuleData    = NULL;
        UnLoadModuleData  = NULL;
        GetDesc           = NULL;
        GetName           = NULL;
        GetModuleKey      = NULL;
        DoModuleMessage   = NULL;
        GetModuleState    = NULL;
    }
};

struct _WaitUnloadModule
{
    uint32           m_u4UpdateIndex;                //�����߳�����
    uint32           m_u4ThreadCurrEndCount;         //��ǰ�Ѿ������Ĺ����̸߳���
    uint8            m_u1UnloadState;                //����״̬��1Ϊж�أ�2Ϊ����
    char             m_szModuleName[MAX_BUFF_100];   //�������
    ACE_SHLIB_HANDLE m_hModule;                      //�����ָ��
    int (*UnLoadModuleData)(void);                   //ж�ز���ĺ���ָ��
    string           m_strModuleName;                //ģ���ļ�����
    string           m_strModulePath;                //ģ��·��
    string           m_strModuleParam;               //ģ����������

    _WaitUnloadModule()
    {
        m_hModule              = 0;
        m_u1UnloadState        = 0;
        m_u4ThreadCurrEndCount = 0;
        m_u4UpdateIndex        = 0;
        m_szModuleName[0]      = '\0';
        UnLoadModuleData       = NULL;
    }
};

class CLoadModule : public IModuleInfo
{
public:
    CLoadModule(void);
    virtual ~CLoadModule(void);

    void Init(uint16 u2MaxModuleCount);

    void Close();

    bool LoadModule(const char* pModulePath, const char* pModuleName, const char* pModuleParam);
    bool UnLoadModule(const char* szModuleName, bool blIsDelete = true);
    bool MoveUnloadList(const char* szModuleName, uint32 u4UpdateIndex, uint32 u4ThreadCount, uint8 u1UnLoadState,
                        string strModulePath, string strModuleName, string strModuleParam);                           //��Ҫж�صĲ�����뻺���б�
    int  UnloadListUpdate(uint32 u4UpdateIndex);                                                                      //�����̻߳ص��ӿڣ������й����̻߳ص��������ͷŲ���˿ڣ����ﷵ��0��ʲô������������1������Ҫ���¼��ظ���

    int  SendModuleMessage(const char* pModuleName, uint16 u2CommandID, IBuffPacket* pBuffPacket, IBuffPacket* pReturnBuffPacket);

    int  GetCurrModuleCount();
    int  GetModulePoolCount();
    _ModuleInfo* GetModuleInfo(const char* pModuleName);

    //����ӿ��ṩ��ع���
    bool GetModuleExist(const char* pModuleName);
    const char* GetModuleParam(const char* pModuleName);
    const char* GetModuleFileName(const char* pModuleName);
    const char* GetModuleFilePath(const char* pModuleName);
    const char* GetModuleFileDesc(const char* pModuleName);
    uint16 GetModuleCount();
    void GetAllModuleInfo(vector<_ModuleInfo*>& vecModeInfo);
    void GetAllModuleName(vector<string> vecModuleName);

private:
    bool LoadModuleInfo(string strModuleName, _ModuleInfo* pModuleInfo, const char* pModulePath);    //��ʼ����ģ��Ľӿں�����

private:
    CHashTable<_ModuleInfo>            m_objHashModuleList;
    char                               m_szModulePath[MAX_BUFF_200];
    vector<_WaitUnloadModule>          m_vecWaitUnloadModule;
    ACE_Recursive_Thread_Mutex         m_tmModule;
};

typedef ACE_Singleton<CLoadModule, ACE_Null_Mutex> App_ModuleLoader;
#endif
