#ifndef _LOADPACKETPARSE_H
#define _LOADPACKETPARSE_H

#include "ace/Date_Time.h"
#include "ace/Thread_Mutex.h"
#include "ace/Singleton.h"
#include "ace/OS_NS_dlfcn.h"

#include "define.h"
#include "HashTable.h"
#include "IMessageBlockManager.h"

struct _Packet_Parse_Info
{
    uint32           m_u4PacketParseID;       //��ǰpacketParseID
    uint8            m_u1PacketParseType;     //��ͷ���������ģʽ
    uint32           m_u4OrgLength;           //���ݰ�ͷ����
    ACE_Date_Time    m_dtCreateTime;          //ģ�鴴��ʱ��
    ACE_SHLIB_HANDLE m_hModule;
    bool (*Parse_Packet_Head_Info)(uint32 u4ConnectID, ACE_Message_Block* pmbHead, IMessageBlockManager* pMessageBlockManager, _Head_Info* pHeadInfo);
    bool (*Parse_Packet_Body_Info)(uint32 u4ConnectID, ACE_Message_Block* pmbbody, IMessageBlockManager* pMessageBlockManager, _Body_Info* pBodyInfo);
    uint8 (*Parse_Packet_Stream)(uint32 u4ConnectID, ACE_Message_Block* pCurrMessage, IMessageBlockManager* pMessageBlockManager, _Packet_Info* pPacketInfo);
    bool (*Make_Send_Packet)(uint32 u4ConnectID, const char* pData, uint32 u4Len, ACE_Message_Block* pMbData, uint16 u2CommandID);
    uint32 (*Make_Send_Packet_Length)(uint32 u4ConnectID, uint32 u4DataLen, uint16 u2CommandID);
    bool (*Connect)(uint32 u4ConnectID, _ClientIPInfo objClientIPInfo, _ClientIPInfo objLocalIPInfo);
    void (*DisConnect)(uint32 u4ConnectID);
    void(*Close)();

    _Packet_Parse_Info()
    {
        m_u4PacketParseID       = 0;
        m_u1PacketParseType     = PACKET_WITHHEAD;
        m_u4OrgLength           = 0;
        m_hModule               = NULL;
        Parse_Packet_Head_Info  = NULL;
        Parse_Packet_Body_Info  = NULL;
        Parse_Packet_Stream     = NULL;
        Make_Send_Packet        = NULL;
        Make_Send_Packet_Length = NULL;
        Connect                 = NULL;
        DisConnect              = NULL;
        Close                   = NULL;
    }
};

class CLoadPacketParse
{
public:
    CLoadPacketParse();
    ~CLoadPacketParse();

    void Init(int nCount);

    bool LoadPacketInfo(uint32 u4PacketParseID, uint8 u1Type, uint32 u4HeadLen, const char* pPacketParsePath, const char* szPacketParseName);

    void Close();

    _Packet_Parse_Info* GetPacketParseInfo(uint32 u4PacketParseID);

private:
    //_Packet_Parse_Info m_Packet_Parse_Info;
    CHashTable<_Packet_Parse_Info>        m_objPacketParseList;                  //Hash�ڴ��
};

typedef ACE_Singleton<CLoadPacketParse, ACE_Null_Mutex> App_PacketParseLoader;

#endif
