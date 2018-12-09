#ifndef _BUFFPACKETMAGER_H
#define _BUFFPACKETMAGER_H

//����������CreateBuffPacket��ǳ�����������Ҫ����������
//ʹ��BuffPacket�أ��ڷ�����������ʱ��һ���Դ����ö��Buffpacket�������ڴ�ص��뷨��
//�����лfreebird�Ĳ��Կͻ��ˣ�������Ϊ�����Ҳ��ܷ�����������⡣��Ҫ�ù�ע�����ʧ�������Ǳ���ġ�
//2010-06-03 freeeyes

#include "HashTable.h"
#include "IPacketManager.h"
#include "BuffPacket.h"

using namespace std;

class CBuffPacketManager : public IPacketManager
{
public:
    CBuffPacketManager(void);
    ~CBuffPacketManager(void);

    void Init(uint32 u4PacketCount, uint32 u4MaxBuffSize, bool blByteOrder);
    void Close();

    uint32 GetBuffPacketUsedCount();
    uint32 GetBuffPacketFreeCount();

    IBuffPacket* Create();
    bool Delete(IBuffPacket* pBuffPacket);

private:
    CHashTable<CBuffPacket>    m_objHashBuffPacketList;               //�洢����BuffPacketָ���hash�б�
    bool                       m_blSortType;                          //�������trueΪ����falseΪ������
    ACE_Recursive_Thread_Mutex m_ThreadWriteLock;
};

typedef ACE_Singleton<CBuffPacketManager, ACE_Null_Mutex> App_BuffPacketManager;
#endif