#ifndef _PACKETPARSEBASE_H
#define _PACKETPARSEBASE_H

//������ǰѲ������仯�ĺ������������ֻ�ÿ�����ȥʵ�ֱ�Ҫ��5���ӿ�
//ʣ�µģ������߲���Ҫ��ϵ�����ǿ���Լ�ʵ�ֵ����顣
//���԰������������࣬����������ࡣ
//���ÿһ��εĽ���������������ȥ����������롣
//add by freeeyes

#include "BuffPacket.h"
#include "IMessageBlockManager.h"

class CPacketParseBase
{
public:
    CPacketParseBase(void);

    virtual ~CPacketParseBase(void);

    void Clear();

    void Close();

    const char* GetPacketVersion();
    uint32 GetPacketHeadLen();
    uint32 GetPacketBodyLen();

    uint16 GetPacketCommandID();
    bool GetIsHandleHead();
    uint32 GetPacketHeadSrcLen();
    uint32 GetPacketBodySrcLen();

    void SetSort(uint8 u1Sort);

    void Check_Recv_Unit16(uint16& u2Data);
    void Check_Recv_Unit32(uint32& u4Data);
    void Check_Recv_Unit64(uint64& u2Data);

    void Check_Send_Unit16(uint16& u2Data);
    void Check_Send_Unit32(uint32& u4Data);
    void Check_Send_Unit64(uint64& u8Data);

    ACE_Message_Block* GetMessageHead();
    ACE_Message_Block* GetMessageBody();

    virtual void Init() = 0;
    IPacketHeadInfo* GetPacketHeadInfo();
    void SetPacketHeadInfo(IPacketHeadInfo* pPacketHeadInfo);

    void SetPacket_Head_Curr_Length(uint32 u4CurrLength);
    void SetPacket_Body_Curr_Length(uint32 u4CurrLength);
    void SetPacket_Head_Src_Length(uint32 u4SrcLength);
    void SetPacket_Body_Src_Length(uint32 u4SrcLength);
    void SetPacket_CommandID(uint16 u2PacketCommandID);
    void SetPacket_IsHandleHead(bool blState);
    void SetPacket_Head_Message(ACE_Message_Block* pmbHead);
    void SetPacket_Body_Message(ACE_Message_Block* pmbHead);

private:
    uint32 m_u4PacketHead;               //��ͷ�ĳ���
    uint32 m_u4PacketBody;               //����ĳ���
    uint32 m_u4HeadSrcSize;              //��ͷ��ԭʼ����
    uint32 m_u4BodySrcSize;              //�����ԭʼ����
    uint16 m_u2PacketCommandID;          //������
    bool   m_blIsHandleHead;
    char   m_szPacketVersion[MAX_BUFF_20];   //���������汾
    uint8  m_u1Sort;                         //�ֽ������0Ϊ�����ֽ���1Ϊ�����ֽ���

    IPacketHeadInfo*    m_pPacketHeadInfo;  //���ݰ�ͷ��Ϣ

    ACE_Message_Block* m_pmbHead;   //��ͷ����
    ACE_Message_Block* m_pmbBody;   //���岿��
};

#endif
