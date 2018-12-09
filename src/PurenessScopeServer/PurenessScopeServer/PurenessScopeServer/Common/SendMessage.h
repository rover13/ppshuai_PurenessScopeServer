#ifndef _SENDMESSAGE_H
#define _SENDMESSAGE_H

#include "define.h"
#include "IBuffPacket.h"
#include "HashTable.h"

using namespace std;

//����һ���������������������첽���Ͷ���
struct _SendMessage
{
    uint32              m_u4ConnectID;    //Ҫ���͵�Զ��ID
    int32               m_nMessageID;     //������Ϣ��ID
    int32               m_nHashID;        //��ǰ�����HashID
    uint16              m_u2CommandID;    //Ҫ���͵�����ID������ͳ�ƹ���
    uint8               m_u1SendState;    //Ҫ���͵�״̬��0���������ͣ�1���Ȼ��治���ͣ�2���������ͺ�ر�
    uint8               m_nEvents;        //�������ͣ�0��ʹ��PacketParse��֯�������ݣ�1����ʹ��PacketParse��֯����
    uint8               m_u1Type;         //���ݰ������ͣ�0:���ݰ���1:�����ر���Ϊ
    bool                m_blDelete;       //������ɺ��Ƿ�ɾ����true��ɾ����false�ǲ�ɾ��
    IBuffPacket*        m_pBuffPacket;    //���ݰ�����
    ACE_Time_Value      m_tvSend;         //���ݰ����͵�ʱ���
    ACE_Message_Block*  m_pmbQueuePtr;    //��Ϣ����ָ���

    _SendMessage()
    {
        Clear();

        //����������Ϣ����ģ��ָ�����ݣ������Ͳ��ط�����new��delete����������
        //ָ���ϵҲ����������ֱ��ָ��������ʹ�õ�ʹ����ָ��
        m_pmbQueuePtr  = new ACE_Message_Block(sizeof(_SendMessage*));

        _SendMessage** ppMessage = (_SendMessage**)m_pmbQueuePtr->base();
        *ppMessage = this;
    }

    //�������캯��
    _SendMessage(const _SendMessage& ar)
    {
        this->m_u1SendState = ar.m_u1SendState;
        this->m_blDelete    = ar.m_blDelete;
        this->m_pBuffPacket = ar.m_pBuffPacket;
        this->m_u4ConnectID = ar.m_u4ConnectID;
        this->m_nEvents     = ar.m_nEvents;
        this->m_u2CommandID = ar.m_u2CommandID;
        this->m_nHashID     = ar.m_nHashID;
        this->m_nMessageID  = ar.m_nMessageID;
        this->m_u1Type      = ar.m_u1Type;
        this->m_tvSend      = ar.m_tvSend;

        //����������Ϣ����ģ��ָ�����ݣ������Ͳ��ط�����new��delete����������
        //ָ���ϵҲ����������ֱ��ָ��������ʹ�õ�ʹ����ָ��
        this->m_pmbQueuePtr  = new ACE_Message_Block(sizeof(_SendMessage*));

        _SendMessage** ppMessage = (_SendMessage**)m_pmbQueuePtr->base();
        *ppMessage = this;
    }

    _SendMessage& operator = (const _SendMessage& ar)
    {
        this->m_u1SendState = ar.m_u1SendState;
        this->m_blDelete    = ar.m_blDelete;
        this->m_pBuffPacket = ar.m_pBuffPacket;
        this->m_u4ConnectID = ar.m_u4ConnectID;
        this->m_nEvents     = ar.m_nEvents;
        this->m_u2CommandID = ar.m_u2CommandID;
        this->m_nHashID     = ar.m_nHashID;
        this->m_nMessageID  = ar.m_nMessageID;
        this->m_u1Type      = ar.m_u1Type;
        this->m_tvSend      = ar.m_tvSend;

        memcpy_safe((char* )ar.m_pmbQueuePtr->base(), (uint32)ar.m_pmbQueuePtr->length(), m_pmbQueuePtr->base(), (uint32)m_pmbQueuePtr->length());

        _SendMessage** ppMessage = (_SendMessage**)m_pmbQueuePtr->base();
        *ppMessage = this;
        return *this;
    }

    ~_SendMessage()
    {
        if(NULL != m_pmbQueuePtr)
        {
            m_pmbQueuePtr->release();
            m_pmbQueuePtr = NULL;
        }
    }

    void Clear()
    {
        m_u1SendState = 0;
        m_blDelete    = true;
        m_pBuffPacket = NULL;
        m_u4ConnectID = 0;
        m_nEvents     = 0;
        m_u2CommandID = 0;
        m_nMessageID  = 0;
        m_u1Type      = 0;
        m_nHashID     = 0;
        m_tvSend      = 0;
    }

    ACE_Message_Block* GetQueueMessage()
    {
        return m_pmbQueuePtr;
    }

    void SetHashID(int32 nHashID)
    {
        m_nHashID = nHashID;
    }

    int32 GetHashID()
    {
        return m_nHashID;
    }
};

class CSendMessagePool
{
public:
    CSendMessagePool(void);
    ~CSendMessagePool(void);

    void Init(int32 nObjcetCount = MAX_MSG_THREADQUEUE);
    void Close();

    _SendMessage* Create();
    bool Delete(_SendMessage* pObject);

    int32 GetUsedCount();
    int32 GetFreeCount();

private:
    CHashTable<_SendMessage>    m_objHashHandleList;
    ACE_Recursive_Thread_Mutex  m_ThreadWriteLock;                     //���ƶ��߳���
};
#endif

