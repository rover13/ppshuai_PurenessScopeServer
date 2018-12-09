#ifdef WIN32
#include "WindowsProcess.h"

ACE_NT_SERVICE_DEFINE (PssServer, CProgramService, ACE_TEXT("PssServer Service"));

Process::Process (void)
    : m_nOptinstall (0),
      m_nOptremove (0),
      m_nOptstart (0),
      m_nOptkill (0),
      m_nOpttype (0),
      m_nOptdebug (0),
      m_nOptstartup (0),
      m_pFrameLoggingStrategy(0)
{
    m_szProgName[0]         = '\0';

}

Process::~Process (void)
{
    stopprocesslog();
}

void
Process::print_usage_and_die(void)
{
    ACE_DEBUG ((LM_INFO,
                "֧�ֲ�������: \n  -i: ��װwindows����\n"
                "  -r: ɾ��windows����\n"
                "  -s: ����windows����\n"
                "  -k: ɱ��windows����\n",
                0));
}

bool Process::parse_args(int argc, ACE_TCHAR* argv[])
{
    bool blFind = false;
    ACE_Get_Opt get_opt(argc, argv, ACE_TEXT ("i:rskt:d"));
    int c;

    while ((c = get_opt()) != -1)
        switch (c)
        {
        case 'i':
            m_nOptinstall = 1;
            m_nOptstartup = ACE_OS::atoi (get_opt.opt_arg());

            if (m_nOptstartup <= 0)
            {
                print_usage_and_die ();
            }

            blFind = true;
            break;

        case 'r':
            m_nOptremove = 1;
            blFind = true;
            break;

        case 's':
            m_nOptstart = 1;
            blFind = true;
            break;

        case 'k':
            m_nOptkill = 1;
            blFind = true;
            break;

        default:

            if (ACE_OS::strcmp (get_opt.argv ()[get_opt.opt_ind () - 1], ACE_TEXT ("-i")) == 0)
            {
                m_nOptinstall = 1;
                m_nOptstartup = DEFAULT_SERVICE_INIT_STARTUP;
                blFind = true;
            }

            break;
        }

    return blFind;
}

int Process::run(int argc, ACE_TCHAR* argv[])
{
    App_Service::instance()->name(ACE_TEXT (App_MainConfig::instance()->GetDisplayServiceName()), ACE_TEXT (App_MainConfig::instance()->GetWindowsServiceName()));

    bool blStata = parse_args(argc, argv);

    if(blStata == true)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT ("parse_args is true.\n")));
    }
    else
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT ("parse_args is false.\n")));
    }

    if(blStata == true)
    {
        if (m_nOptinstall && !m_nOptremove)
        {
            if (-1 == App_Service::instance ()->insert(m_nOptstartup))
            {
                ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT ("insert")));
                return -1;
            }

            return 0;
        }

        if (m_nOptremove && !m_nOptinstall)
        {
            if (-1 == App_Service::instance ()->remove())
            {
                ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT ("remove")));
                return -1;
            }

            return 0;
        }

        if (m_nOptstart && m_nOptkill)
        {
            print_usage_and_die ();
        }

        if (m_nOptstart)
        {
            if (-1 == App_Service::instance ()->start_svc())
            {
                ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT ("start")));
                return -1;
            }

            return 0;
        }

        if (m_nOptkill)
        {
            if (-1 == App_Service::instance()->stop_svc())
            {
                ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT ("stop")));
                return -1;
            }

            return 0;
        }

        if (m_nOpttype)
        {
            if (-1 == App_Service::instance ()->startup(m_nOptstartup))
            {
                ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT ("set startup")));
                return -1;
            }

            return 0;
        }
    }
    else
    {
        //��ʼ��������
        ACE_NT_SERVICE_RUN (PssServer, App_Service::instance(), ret);

        if (ret == 0)
        {
            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT ("%p\n"),
                        ACE_TEXT ("Couldn't start service")));
        }
        else
        {
            ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("%T (%t): Service stopped.\n")));
        }
    }

    return 0;
}

void Process::startprocesslog()
{
    stopprocesslog();

    m_pFrameLoggingStrategy = new Frame_Logging_Strategy();

    //�Ƿ��ACE_DEBUG�ļ��洢
    Logging_Config_Param objParam;

    sprintf_safe(objParam.m_strLogFile, 256, "%s", (string(App_MainConfig::instance()->GetDisplayServiceName()) + string(".log")).c_str());
    objParam.m_iChkInterval    = App_MainConfig::instance()->GetChkInterval();
    objParam.m_iLogFileMaxCnt  = App_MainConfig::instance()->GetLogFileMaxCnt();
    objParam.m_iLogFileMaxSize = App_MainConfig::instance()->GetLogFileMaxSize();
    sprintf_safe(objParam.m_strLogLevel, 128, "%s", App_MainConfig::instance()->GetDebugLevel());

    m_pFrameLoggingStrategy->InitLogStrategy(objParam);
}

void Process::stopprocesslog()
{
    if(NULL != m_pFrameLoggingStrategy)
    {
        m_pFrameLoggingStrategy->EndLogStrategy();
        SAFE_DELETE(m_pFrameLoggingStrategy);
    }
}

#endif

