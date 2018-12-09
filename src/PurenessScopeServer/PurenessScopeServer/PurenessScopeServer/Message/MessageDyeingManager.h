#ifndef MESSAGE_DYEING_MANAGER_H
#define MESSAGE_DYEING_MANAGER_H

#include "IMessage.h"
#include "Trace.h"

//�������ݰ�Ⱦɫ��ع���
//���ǵ������Ĵ��ۣ�Ŀǰ���Ǿ���������С�������Ӧ���ǿ��Խ��ܵġ�
//add by freeeyes

#define  MAX_DYEING_COMMAND_COUNT 10  //���Ⱦɫ������

//IP��أ�Ŀǰ����ͬһʱ��ֻ�ܼ��һ��
struct _Dyeing_IP
{
    uint16  m_u2MaxCount;        //��ǰ��Ҫ��ص�������
    uint16  m_u2CurrCount;       //��ǰ���Ѵ���ĸ���
    bool m_blIsValid;            //����Ƿ���Ч
    char m_szIP[MAX_BUFF_20];    //�û�IP

    _Dyeing_IP()
    {
        m_u2MaxCount  = 0;
        m_u2CurrCount = 0;
        m_blIsValid   = false;
        m_szIP[0]     = '\0';
    }

    void Clear()
    {
        m_u2MaxCount = 0;
        m_u2CurrCount = 0;
        m_blIsValid = false;
        m_szIP[0] = '\0';
    }
};

//����������
struct _Dyeing_Command
{
    uint16  m_u2CommandID;       //��ǰCommandID
    uint16  m_u2MaxCount;        //��ǰ��Ҫ��ص�������
    uint16  m_u2CurrCount;       //��ǰ���Ѵ���ĸ���
    bool    m_blIsValid;         //����Ƿ���Ч

    _Dyeing_Command()
    {
        m_u2CommandID   = 0;
        m_u2MaxCount    = 0;
        m_u2CurrCount   = 0;
        m_blIsValid     = false;
    }

    void Clear()
    {
        m_u2CommandID = 0;
        m_u2MaxCount = 0;
        m_u2CurrCount = 0;
        m_blIsValid = false;
    }
};

typedef vector<_Dyeing_Command> vec_Dyeing_Command_list;  //CommandIDȾɫ�б�

class CMessageDyeingManager
{
public:
    CMessageDyeingManager();
    ~CMessageDyeingManager();

    void AddDyringIP(const char* pClientIP, uint16 u2MaxCount);                    //���һ��IPȾɫ����
    bool AddDyeingCommand(uint16 u2CommandID, uint16 u2MaxCount);                  //���һ������Ⱦɫ����

    void GetDyeingCommand(vec_Dyeing_Command_list& objList);                       //��õ�ǰ����Ⱦɫ״̬

    string GetTraceID(const char* pClientIP, short sClintPort, uint16 u2CommandID);  //�����Ƿ�����µ�traceID

private:
    _Dyeing_Command m_objCommandList[MAX_DYEING_COMMAND_COUNT];   //��ǰ��������Ⱦɫ��CommandID�б�
    _Dyeing_IP      m_objDyeingIP;                                //��ǰ����Ⱦɫ��IP
    uint16          m_u2CurrCommandCount;                         //��ǰ����ִ�е�ȾɫCommandID����
};

#endif
