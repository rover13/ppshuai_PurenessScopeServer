#ifndef _FILETESTMANAGER_H_
#define _FILETESTMANAGER_H_

#include <vector>
#include <map>
using namespace std;

#ifndef WIN32
#include "ConnectHandler.h"
#else
//�����windows
#include "ProConnectHandle.h"
#endif

#include "XmlOpeation.h"

#include "ace/FILE_Addr.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "FileTest.h"

class CFileTestManager : public ACE_Task<ACE_MT_SYNCH>, public IFileTestManager
{
public:
    CFileTestManager(void);
    virtual ~CFileTestManager(void);
public:
    //�ļ����Է���
    FileTestResultInfoSt FileTestStart(const char* szXmlFileTestName);      //��ʼ�ļ�����
    int FileTestEnd();                                                      //�����ļ�����
    void HandlerServerResponse(uint32 u4ConnectID);                         //��ǰ���ӷ������ݰ��Ļص�����
private:
    bool LoadXmlCfg(const char* szXmlFileTestName, FileTestResultInfoSt& objFileTestResult);        //��ȡ���������ļ�
    int  ReadTestFile(const char* pFileName, int nType, FileTestDataInfoSt& objFileTestDataInfo);   //����Ϣ���ļ��������ݽṹ

    virtual int handle_timeout(const ACE_Time_Value& tv, const void* arg);   //��ʱ�����
private:
    ACE_Recursive_Thread_Mutex  m_ThreadWriteLock;
    ACE_Time_Value m_atvLastCheck;
    //�ļ����Ա���
    bool m_bFileTesting;          //�Ƿ����ڽ����ļ�����
    int32 m_n4TimerID;            //��ʱ��ID

    CXmlOpeation m_MainConfig;
    string m_strProFilePath;
    int    m_n4TimeInterval;      //��ʱ���¼����
    int    m_n4ConnectCount;      //ģ��������
    int    m_n4ResponseCount;     //������Ӧ������
    int    m_n4ExpectTime;        //��������ʱ�ܵĺ�ʱ(��λ����)
    uint32 m_u4ParseID;           //������ID
    int    m_n4ContentType;       //Э����������,1�Ƕ�����Э��,0���ı�Э��

    typedef vector<FileTestDataInfoSt> vecFileTestDataInfoSt;
    vecFileTestDataInfoSt m_vecFileTestDataInfoSt;

    typedef struct RESPONSERECORD
    {
        uint64 m_u8StartTime;
        uint8 m_u1ResponseCount;

        RESPONSERECORD()
        {
            Init();
        }

        RESPONSERECORD(const RESPONSERECORD& ar)
        {
            this->m_u8StartTime = ar.m_u8StartTime;
            this->m_u1ResponseCount = ar.m_u1ResponseCount;
        }

        void Init()
        {
            m_u8StartTime      = 0;
            m_u1ResponseCount  = 0;
        }

        RESPONSERECORD& operator= (const RESPONSERECORD& ar)
        {
            this->m_u8StartTime     = ar.m_u8StartTime;
            this->m_u1ResponseCount = ar.m_u1ResponseCount;
            return *this;
        }
    } ResponseRecordSt;

    typedef map<uint32,ResponseRecordSt> mapResponseRecordSt;
    mapResponseRecordSt m_mapResponseRecordSt;
};

typedef ACE_Singleton<CFileTestManager, ACE_Null_Mutex> App_FileTestManager;
#endif //_FILETESTMANAGER_H_