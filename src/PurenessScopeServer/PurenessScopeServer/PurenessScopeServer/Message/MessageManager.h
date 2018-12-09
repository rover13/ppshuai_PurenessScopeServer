#ifndef _MESSAGEMANAGER_H
#define _MESSAGEMANAGER_H

#pragma once

#include "IMessageManager.h"
#include "Message.h"
#include "LoadModule.h"
#include "HashTable.h"

//�����޸�һ�£����һ�������Ӧһ��ģ���������Ƶġ�
//�����Ϊһ��������Զ�Ӧ���������Ĵ���ģ�飬�����ͱȽϺ��ˡ�
//��hash������Ի���ۻ����ߵ���������
//add by freeeyes

using namespace std;

//��������ߵĸ�ʽ
struct _ClientCommandInfo
{
    uint32          m_u4Count;                          //��ǰ������õĴ���
    uint32          m_u4TimeCost;                       //��ǰ������ʱ������
    uint32          m_u4CurrUsedCount;                  //��ǰ����ʹ�õ����ô���
    CClientCommand* m_pClientCommand;                   //��ǰ����ָ��
    uint16          m_u2CommandID;                      //��ǰ�����Ӧ��ID
    char            m_szModuleName[MAX_BUFF_100];       //����ģ������
    ACE_Date_Time   m_dtLoadTime;                       //��ǰ�������ʱ��
    _ClientIPInfo   m_objListenIPInfo;                  //��ǰ�����IP�Ͷ˿���ڣ�Ĭ�������е�ǰ�˿�

    _ClientCommandInfo()
    {
        m_pClientCommand  = NULL;
        m_u4Count         = 0;
        m_u4TimeCost      = 0;
        m_szModuleName[0] = '\0';
        m_u4CurrUsedCount = 0;
        m_u2CommandID     = 0;
    }
};

//ģ���_ClientCommandInfo֮��Ķ�Ӧ��ϵ
struct _ModuleClient
{
    vector<_ClientCommandInfo*> m_vecClientCommandInfo;    //һ��ģ�����ж�Ӧ�����б�
};

//һ����Ϣ���Զ�Ӧһ��CClientCommand*�����飬����Ϣ�����ʱ��ַ�����Щ������
class CClientCommandList
{
private:
    uint16 m_u2CommandID;
    typedef vector<_ClientCommandInfo*> vecClientCommandList;
    vecClientCommandList m_vecClientCommandList;

public:
    CClientCommandList()
    {
        m_u2CommandID = 0;
    }

    CClientCommandList(uint16 u2CommandID)
    {
        m_u2CommandID = u2CommandID;
    }

    ~CClientCommandList()
    {
        Close();
    }

    void SetCommandID(uint16 u2CommandID)
    {
        m_u2CommandID = u2CommandID;
    }

    uint16 GetCommandID()
    {
        return m_u2CommandID;
    }

    void Close()
    {
        for(int i = 0; i < (int)m_vecClientCommandList.size(); i++)
        {
            _ClientCommandInfo* pClientCommandInfo = m_vecClientCommandList[i];
            SAFE_DELETE(pClientCommandInfo);
        }

        m_vecClientCommandList.clear();
    }

    _ClientCommandInfo* AddClientCommand(CClientCommand* pClientCommand, const char* pMuduleName)
    {
        _ClientCommandInfo* pClientCommandInfo = new _ClientCommandInfo();

        if(NULL != pClientCommandInfo)
        {
            pClientCommandInfo->m_pClientCommand  = pClientCommand;
            m_vecClientCommandList.push_back(pClientCommandInfo);
            sprintf_safe(pClientCommandInfo->m_szModuleName, MAX_BUFF_100, "%s", pMuduleName);
            return pClientCommandInfo;
        }
        else
        {
            return NULL;
        }

    }

    _ClientCommandInfo* AddClientCommand(CClientCommand* pClientCommand, const char* pMuduleName, const _ClientIPInfo& objListenInfo)
    {
        _ClientCommandInfo* pClientCommandInfo = new _ClientCommandInfo();

        if(NULL != pClientCommandInfo)
        {
            pClientCommandInfo->m_pClientCommand  = pClientCommand;
            pClientCommandInfo->m_objListenIPInfo = objListenInfo;
            m_vecClientCommandList.push_back(pClientCommandInfo);
            sprintf_safe(pClientCommandInfo->m_szModuleName, MAX_BUFF_100, "%s", pMuduleName);
            return pClientCommandInfo;
        }
        else
        {
            return NULL;
        }

    }

    //�������Ϊtrue��֤�������Ϣ�Ѿ�û�ж�Ӧ���Ҫ��ΧHash�г�ȥ
    bool DelClientCommand(CClientCommand* pClientCommand)
    {
        for(vecClientCommandList::iterator b = m_vecClientCommandList.begin(); b!= m_vecClientCommandList.end(); ++b)
        {
            _ClientCommandInfo* pClientCommandInfo = (_ClientCommandInfo* )(*b);

            if(NULL != pClientCommandInfo && pClientCommand == pClientCommandInfo->m_pClientCommand)
            {
                SAFE_DELETE(pClientCommandInfo);
                m_vecClientCommandList.erase(b);
                break;
            }
        }

        if((int)m_vecClientCommandList.size() == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    //�õ�����
    int GetCount()
    {
        return (int)m_vecClientCommandList.size();
    }

    //�õ�ָ��λ�õ�ָ��
    _ClientCommandInfo* GetClientCommandIndex(int nIndex)
    {
        if(nIndex >= (int)m_vecClientCommandList.size())
        {
            return NULL;
        }
        else
        {
            return m_vecClientCommandList[nIndex];
        }
    }
};

class CMessageManager : public IMessageManager
{
public:
    CMessageManager(void);
    ~CMessageManager(void);

    void Init(uint16 u2MaxModuleCount, uint32 u4MaxCommandCount);

    void Close();

    bool AddClientCommand(uint16 u2CommandID, CClientCommand* pClientCommand, const char* pModuleName, _ClientIPInfo objListenInfo);   //ע������
    bool AddClientCommand(uint16 u2CommandID, CClientCommand* pClientCommand, const char* pModuleName);   //ע������
    bool DelClientCommand(uint16 u2CommandID, CClientCommand* pClientCommand);                            //ж������

    bool UnloadModuleCommand(const char* pModuleName, uint8 u1LoadState, uint32 u4ThreadCount);  //ж��ָ��ģ���¼���u1State= 1 ж�أ�2 ����

    int  GetCommandCount();                                            //�õ���ǰע������ĸ���
    CClientCommandList* GetClientCommandList(uint16 u2CommandID);      //�õ���ǰ�����ִ���б�

    CHashTable<_ModuleClient>* GetModuleClient();                      //��������ģ���ע��������Ϣ

    virtual uint32 GetWorkThreadCount();
    virtual uint32 GetWorkThreadByIndex(uint32 u4Index);

    uint16 GetMaxCommandCount();
    uint32 GetUpdateIndex();

    CHashTable<CClientCommandList>* GetHashCommandList();              //�õ���ǰHashCommandList�ĸ���

private:
    uint32                         m_u4UpdateIndex;                     //��ǰ����ID
    uint32                         m_u4MaxCommandCount;                 //���������е�����
    uint32                         m_u4CurrCommandCount;                //��ǰ��Ч������
    uint16                         m_u2MaxModuleCount;                  //ģ��������������
    CHashTable<CClientCommandList> m_objClientCommandList;              //����ֶ�Ӧ������
    CHashTable<_ModuleClient>      m_objModuleClientList;               //����ģ���Ӧ����Ϣ
    ACE_Recursive_Thread_Mutex     m_ThreadWriteLock;                   //������

};

typedef ACE_Singleton<CMessageManager, ACE_Null_Mutex> App_MessageManager;
#endif
