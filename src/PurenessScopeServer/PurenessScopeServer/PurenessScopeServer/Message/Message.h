#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "ace/Singleton.h"
#include "ace/Thread_Mutex.h"
#include "HashTable.h"

#include "MessageBlockManager.h"
#include "BuffPacket.h"
#include "IMessage.h"

using namespace std;

class CMessage : public IMessage
{
public:
    CMessage(void);
    ~CMessage(void);

    void Close();
    void Clear();

    void SetHashID(int nHasnID);
    int  GetHashID();

    void SetMessageBase(_MessageBase* pMessageBase);

    ACE_Message_Block* GetMessageHead();
    ACE_Message_Block* GetMessageBody();

    _MessageBase* GetMessageBase();

    bool GetPacketHead(_PacketInfo& PacketInfo);
    bool GetPacketBody(_PacketInfo& PacketInfo);
    bool SetPacketHead(ACE_Message_Block* pmbHead);
    bool SetPacketBody(ACE_Message_Block* pmbBody);

    const char* GetError();

    ACE_Message_Block*  GetQueueMessage();

private:
    int           m_nHashID;
    char          m_szError[MAX_BUFF_500];
    _MessageBase* m_pMessageBase;

    ACE_Message_Block* m_pmbHead;             //��ͷ����
    ACE_Message_Block* m_pmbBody;             //���岿��

    ACE_Message_Block*  m_pmbQueuePtr;        //��Ϣ����ָ���
};


//Message�����
class CMessagePool
{
public:
    CMessagePool();
    ~CMessagePool();

    void Init(uint32 u4PacketCount);
    void Close();

    CMessage* Create();
    bool Delete(CMessage* pMakePacket);

    int GetUsedCount();
    int GetFreeCount();

private:
    CHashTable<CMessage>        m_objHashMessageList;                  //Message�����
    ACE_Recursive_Thread_Mutex  m_ThreadWriteLock;                     //���ƶ��߳���
};

#endif
