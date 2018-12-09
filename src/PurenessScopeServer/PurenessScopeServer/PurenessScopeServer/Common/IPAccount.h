#ifndef _IPACCOUNT_H
#define _IPACCOUNT_H

//��ӶԿͻ������ӵ�ͳ����Ϣ
//add by freeeyes
//2016-11-25
//�������Hash����ķ�ʽ���
//Ĭ���ǵ�ǰ������ӵ�2��


#include <string>
#include "ace/Date_Time.h"
#include "define.h"
#include "HashTable.h"
#include "ace/Recursive_Thread_Mutex.h"

//IP����ͳ��ģ��
struct _IPAccount
{
    int32          m_nCount;             //��ǰ���Ӵ���
    int32          m_nAllCount;          //ָ��IP���Ӵ����ܺ�
    int32          m_nMinute;            //��ǰ������
    string         m_strIP;              //��ǰ���ӵ�ַ
    ACE_Date_Time  m_dtLastTime;         //�������ʱ��

    _IPAccount()
    {
        m_nCount     = 0;
        m_nAllCount  = 0;
        m_dtLastTime.update();
        m_nMinute    = (int32)m_dtLastTime.minute();
    }

    void Add(ACE_Date_Time dtNowTime)
    {
        m_dtLastTime = dtNowTime;

        if(m_dtLastTime.minute() != m_nMinute)
        {
            m_nMinute  = (int32)m_dtLastTime.minute();
            m_nCount   = 1;
            m_nAllCount++;
        }
        else
        {
            m_nCount++;
            m_nAllCount++;
        }
    }

    //falseΪ�����ѹ���
    bool Check(ACE_Date_Time dtNowTime)
    {
        //���3������û�и��£�������֮
        uint16 u2NowTime = (uint32)dtNowTime.minute();

        if(u2NowTime - m_nMinute  < 0)
        {
            u2NowTime += 60;
        }

        if(u2NowTime  - m_nMinute >= 3)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
};

typedef vector<_IPAccount> vecIPAccount;

class CIPAccount
{
public:
    CIPAccount()
    {
        m_u4MaxConnectCount = 100;  //Ĭ��ÿ�����100��
        m_u2CurrTime        = 0;
    }

    ~CIPAccount()
    {
        OUR_DEBUG((LM_INFO, "[CIPAccount::~CIPAccount].\n"));
        Close();
        OUR_DEBUG((LM_INFO, "[CIPAccount::~CIPAccount]End.\n"));
    }

    void Close()
    {
        for(int32 i = 0; i < m_objIPList.Get_Count(); i++)
        {
            _IPAccount* pIPAccount = m_objIPList.Pop();
            SAFE_DELETE(pIPAccount);
        }

        m_objIPList.Close();
    }

    void Init(uint32 u4IPCount)
    {
        m_u4MaxConnectCount = u4IPCount;

        //��ʼ��HashTable
        m_objIPList.Init((int32)u4IPCount);

        ACE_Date_Time  dtNowTime;
        m_u2CurrTime = (uint16)dtNowTime.minute();
    }

    bool AddIP(string strIP)
    {
        ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadLock);

        //��鵱ǰʱ�䣬10��������һ�ֵ�ǰHash����
        ACE_Date_Time  dtNowTime;
        uint16 u2NowTime = (uint16)dtNowTime.minute();

        if((int32)(u2NowTime - m_u2CurrTime)  < 0)
        {
            u2NowTime += 60;
        }

        if(u2NowTime - m_u2CurrTime >= 10)
        {
            //����Hash����
            vector<_IPAccount* > vecIPAccount;
            m_objIPList.Get_All_Used(vecIPAccount);

            for(int32 i = 0; i < (int32)vecIPAccount.size(); i++)
            {
                _IPAccount* pIPAccount =vecIPAccount[i];

                if(NULL != pIPAccount)
                {
                    if(false == pIPAccount->Check(dtNowTime))
                    {
                        if (-1 == m_objIPList.Del_Hash_Data(pIPAccount->m_strIP.c_str()))
                        {
                            OUR_DEBUG((LM_INFO, "[CIPAccount::AddIP]Del_Hash_Data(%s) is error.\n", pIPAccount->m_strIP.c_str()));
                        }

                        SAFE_DELETE(pIPAccount);
                    }
                }
            }
        }

        bool blRet = false;

        //������Ҫ����Ҫ�ж��������Ҫ�������IPͳ��
        if(m_u4MaxConnectCount > 0)
        {
            _IPAccount* pIPAccount = m_objIPList.Get_Hash_Box_Data(strIP.c_str());

            if(NULL == pIPAccount)
            {
                //û���ҵ������
                pIPAccount = new _IPAccount();

                if(NULL == pIPAccount)
                {
                    blRet = true;
                }
                else
                {
                    pIPAccount->m_strIP     = strIP;
                    pIPAccount->Add(dtNowTime);

                    //�鿴�����Ƿ�����
                    if(m_objIPList.Get_Count() == m_objIPList.Get_Used_Count())
                    {
                        //��ʱ������
                        SAFE_DELETE(pIPAccount);
                        return true;
                    }
                    else
                    {
                        if (-1 == m_objIPList.Add_Hash_Data(strIP.c_str(), pIPAccount))
                        {
                            OUR_DEBUG((LM_INFO, "[CIPAccount::AddIP]Add_Hash_Data(%s) is error.\n", strIP.c_str()));
                        }
                    }
                }
            }
            else
            {
                pIPAccount->Add(dtNowTime);

                if((uint32)pIPAccount->m_nCount >= m_u4MaxConnectCount)
                {
                    blRet = false;
                }
            }

            blRet = true;
        }
        else
        {
            blRet = true;
        }

        return blRet;
    }

    int32 GetCount()
    {
        return m_objIPList.Get_Used_Count();
    }

    void GetInfo(vecIPAccount& VecIPAccount)
    {
        ACE_Guard<ACE_Recursive_Thread_Mutex> WGuard(m_ThreadLock);
        vector<_IPAccount* > vecIPAccount;
        m_objIPList.Get_All_Used(vecIPAccount);

        for(int32 i = 0; i < (int32)vecIPAccount.size(); i++)
        {
            _IPAccount* pIPAccount = vecIPAccount[i];

            if(NULL != pIPAccount)
            {
                VecIPAccount.push_back(*pIPAccount);
            }
        }
    }

private:
    uint32                           m_u4MaxConnectCount;                  //ÿ������������������ǰ����m_nNeedCheck = 0;�Ż���Ч
    uint16                           m_u2CurrTime;                         //��ǰʱ��
    CHashTable<_IPAccount>           m_objIPList;                          //IPͳ����Ϣ
    ACE_Recursive_Thread_Mutex       m_ThreadLock;                         //���߳���
};

typedef ACE_Singleton<CIPAccount, ACE_Recursive_Thread_Mutex> App_IPAccount;

//��λʱ��������ͳ��
class CConnectAccount
{
public:
    CConnectAccount()
    {
        m_u4ConnectMin     = 0;
        m_u4ConnectMax     = 0;
        m_u4DisConnectMin  = 0;
        m_u4DisConnectMax  = 0;
        m_u4CurrConnect    = 0;
        m_u4CurrDisConnect = 0;
        m_u1Minute         = 0;
    }

    ~CConnectAccount()
    {

    }

    uint32 Get4ConnectMin()
    {
        return m_u4ConnectMin;
    }

    uint32 GetConnectMax()
    {
        return m_u4ConnectMax;
    }

    uint32 GetDisConnectMin()
    {
        return m_u4DisConnectMin;
    }

    uint32 GetDisConnectMax()
    {
        return m_u4DisConnectMax;
    }

    uint32 GetCurrConnect()
    {
        return m_u4CurrConnect;
    }

    uint32 GetCurrDisConnect()
    {
        return m_u4CurrDisConnect;
    }

    void Init(uint32 u4ConnectMin, uint32 u4ConnectMax, uint32 u4DisConnectMin, uint32 u4DisConnectMax)
    {
        m_u4ConnectMin     = u4ConnectMin;
        m_u4ConnectMax     = u4ConnectMax;
        m_u4DisConnectMin  = u4DisConnectMin;
        m_u4DisConnectMax  = u4DisConnectMax;
        m_u4CurrConnect    = 0;
        m_u4CurrDisConnect = 0;

        ACE_Date_Time  dtLastTime;
        m_u1Minute = (uint8)dtLastTime.minute();
    }

    bool AddConnect()
    {
        if(m_u4ConnectMax > 0)
        {
            ACE_Date_Time  dtLastTime;

            if(m_u1Minute != (uint8)dtLastTime.minute())
            {
                //�µ�һ����
                m_u4CurrConnect = 1;
            }
            else
            {
                m_u4CurrConnect++;
            }

            return true;
        }
        else
        {
            return true;
        }
    }

    bool AddDisConnect()
    {
        if(m_u4ConnectMax > 0)
        {
            ACE_Date_Time  dtLastTime;

            if(m_u1Minute != (uint8)dtLastTime.minute())
            {
                //�µ�һ����
                m_u4CurrDisConnect = 1;
            }
            else
            {
                m_u4CurrDisConnect++;
            }

            return true;
        }
        else
        {
            return true;
        }
    }

    int32 CheckConnectCount()
    {
        if(m_u4ConnectMax > 0)
        {
            if(m_u4CurrConnect > m_u4ConnectMax)
            {
                return 1;   //1Ϊ��Խmax����
            }
        }

        if(m_u4ConnectMin > 0)
        {
            if(m_u4CurrConnect < m_u4ConnectMin)
            {
                return 2;    //2Ϊ����min����
            }
        }
        else
        {
            return 0;
        }

        return 0;
    }

    int32 CheckDisConnectCount()
    {
        if(m_u4DisConnectMax > 0)
        {
            if(m_u4CurrDisConnect > m_u4DisConnectMax)
            {
                return 1;    //1Ϊ��Խmax����
            }
        }

        if(m_u4ConnectMin > 0)
        {
            if(m_u4CurrDisConnect < m_u4DisConnectMin)
            {
                return 2;    //2Ϊ����min����
            }
        }
        else
        {
            return 0;
        }

        return 0;
    }

private:
    uint32 m_u4CurrConnect;
    uint32 m_u4CurrDisConnect;

    uint32 m_u4ConnectMin;
    uint32 m_u4ConnectMax;
    uint32 m_u4DisConnectMin;
    uint32 m_u4DisConnectMax;
    uint8  m_u1Minute;            //��ǰ������
};

typedef ACE_Singleton<CConnectAccount, ACE_Recursive_Thread_Mutex> App_ConnectAccount;
#endif
