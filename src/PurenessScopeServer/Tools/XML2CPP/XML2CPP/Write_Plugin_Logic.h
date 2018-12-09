#ifndef _WRITE_PLUGIN_LOGIC_H
#define _WRITE_PLUGIN_LOGIC_H

#include "XmlOpeation.h"
#include "ParseCAPIFile.h"

void Gen_2_Cpp_Logic_H(_Project_Info& objProjectInfo, vecClassInfo& objvecClassInfo)
{
    char szTemp[200]     = {'\0'};
    char szPathFile[200] = {'\0'};

    sprintf_safe(szPathFile, 200, "%s/%s_Logic.h",
                 objProjectInfo.m_szProjectName,
                 objProjectInfo.m_szProjectName);

    //�����Ƿ��Ѵ��ڴ��ļ�����ʼ�ֽ����е����ݣ����ϲ������ļ���ȥ��
    _File_Info obj_H_File_Info;
    Parse_CAPI_File(szPathFile, obj_H_File_Info);

    //�������������ļ���
    FILE* pFile = fopen(szPathFile, "w");

    if(NULL == pFile)
    {
        return;
    }

    //������ת���ɴ�д
    char szHText[100] = {'\0'};
    To_Upper_String(objProjectInfo.m_szProjectName, szHText);
    sprintf_safe(szTemp, 200, "#ifndef _%s_LOGIC_H\n", szHText);
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
    sprintf_safe(szTemp, 200, "#define _%s_LOGIC_H\n\n", szHText);
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

    sprintf_safe(szTemp, 200, "#include \"Protocol.h\"\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
    sprintf_safe(szTemp, 200, "#include \"IObject.h\"\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
    sprintf_safe(szTemp, 200, "#include \"Trace.h\"\n\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

    //��Ӻ����������ڴ���������
    sprintf_safe(szTemp, 200, "#define __ENTER_LOGIC() {try{\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
    sprintf_safe(szTemp, 200, "#define __LEAVE_LOGIC_WITHRETURN(ret) }catch(char* msg){AssertSpecial(false,msg); return ret; }}\n\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

    //����Զ�����Ϣͷ
    sprintf_safe(szTemp, 200, "//add your Include file under at here, don't delete this!\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

    //���ԭ�������а���ͷ���ã����ƹ���
    if(obj_H_File_Info.m_strExtHead.length() > 0)
    {
        fwrite(obj_H_File_Info.m_strExtHead.c_str(), obj_H_File_Info.m_strExtHead.length(), sizeof(char), pFile);
    }

    sprintf_safe(szTemp, 200, "//add your Include file end, don't delete this!\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

    for(int i = 0; i < (int)objProjectInfo.m_objCommandList.size(); i++)
    {
        if(strlen(objProjectInfo.m_objCommandList[i].m_szCommandInID) > 0)
        {
            char* pHeadObjectName   = NULL;
            char* pBodyObjectName   = NULL;
            char* pReturnObjectName = NULL;

            for(int j = 0; j < (int)objProjectInfo.m_objCommandList[i].m_vecObjectInfo.size(); j++)
            {
                if(objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_emPacketType == PACKET_TYPE_HEAD)
                {
                    pHeadObjectName = objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_szClassName;
                }
                else if(objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_emPacketType == PACKET_TYPE_BODY)
                {
                    pBodyObjectName = objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_szClassName;
                }
                else if(objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_emPacketType == PACKET_TYPE_RETURN)
                {
                    pReturnObjectName = objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_szClassName;
                }
            }

            bool bl_Is_Data = false;
            //���߲������У�д�뺯��
            sprintf_safe(szTemp, 200, "bool Logic_%s(CServerObject* pServerObject, _MessageBase* pMessageBase",
                         objProjectInfo.m_objCommandList[i].m_szCommandFuncName);
            fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

            if(NULL != pHeadObjectName)
            {
                sprintf_safe(szTemp, 200, ", %s& obj%s",
                             pHeadObjectName, pHeadObjectName);
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                bl_Is_Data = true;
            }

            if(NULL != pBodyObjectName)
            {
                if(true == bl_Is_Data)
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pBodyObjectName, pBodyObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                }
                else
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pBodyObjectName, pBodyObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                    bl_Is_Data = true;
                }
            }

            if(NULL != pReturnObjectName)
            {
                if(true == bl_Is_Data)
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pReturnObjectName, pReturnObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                }
                else
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pReturnObjectName, pReturnObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                    bl_Is_Data = true;
                }
            }

            sprintf_safe(szTemp, 200, ", const char* pTraceID);\n\n");
            fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
        }
    }

    sprintf_safe(szTemp, 200, "#endif\n\n");
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
    fclose(pFile);
}


void Gen_2_Cpp_Logic_Cpp(_Project_Info& objProjectInfo, vecClassInfo& objvecClassInfo)
{
    char szTemp[200]     = {'\0'};
    char szPathFile[200] = {'\0'};

    sprintf_safe(szPathFile, 200, "%s/%s_Logic.cpp",
                 objProjectInfo.m_szProjectName,
                 objProjectInfo.m_szProjectName);

    //�����Ƿ��Ѵ��ڴ��ļ�����ʼ�ֽ����е����ݣ����ϲ������ļ���ȥ��
    _File_Info obj_CPP_File_Info;
    Parse_CAPI_File(szPathFile, obj_CPP_File_Info);

    //�������������ļ���
    FILE* pFile = fopen(szPathFile, "w");

    if(NULL == pFile)
    {
        return;
    }

    sprintf_safe(szTemp, 200, "#include \"%s_Logic.h\"\n\n",
                 objProjectInfo.m_szProjectName);
    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

    for(int i = 0; i < (int)objProjectInfo.m_objCommandList.size(); i++)
    {
        if(strlen(objProjectInfo.m_objCommandList[i].m_szCommandInID) > 0)
        {
            char* pHeadObjectName   = NULL;
            char* pBodyObjectName   = NULL;
            char* pReturnObjectName = NULL;

            for(int j = 0; j < (int)objProjectInfo.m_objCommandList[i].m_vecObjectInfo.size(); j++)
            {
                if(objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_emPacketType == PACKET_TYPE_HEAD)
                {
                    pHeadObjectName = objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_szClassName;
                }
                else if(objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_emPacketType == PACKET_TYPE_BODY)
                {
                    pBodyObjectName = objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_szClassName;
                }
                else if(objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_emPacketType == PACKET_TYPE_RETURN)
                {
                    pReturnObjectName = objProjectInfo.m_objCommandList[i].m_vecObjectInfo[j].m_szClassName;
                }
            }

            //���߲������У�д�뺯��
            bool bl_Is_Data = false;
            sprintf_safe(szTemp, 200, "bool Logic_%s(CServerObject* pServerObject, _MessageBase* pMessageBase",
                         objProjectInfo.m_objCommandList[i].m_szCommandFuncName);
            fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

            if(NULL != pHeadObjectName)
            {
                sprintf_safe(szTemp, 200, ", %s& obj%s",
                             pHeadObjectName, pHeadObjectName);
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                bl_Is_Data = true;
            }

            if(NULL != pBodyObjectName)
            {
                if(true == bl_Is_Data)
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pBodyObjectName, pBodyObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                }
                else
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pBodyObjectName, pBodyObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                    bl_Is_Data = true;
                }
            }

            if(NULL != pReturnObjectName)
            {
                if(true == bl_Is_Data)
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pReturnObjectName, pReturnObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                }
                else
                {
                    sprintf_safe(szTemp, 200, ", %s& obj%s",
                                 pReturnObjectName, pReturnObjectName);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                    bl_Is_Data = true;
                }
            }

            sprintf_safe(szTemp, 200, ", const char* pTraceID)\n");
            fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
            sprintf_safe(szTemp, 200, "{\n");
            fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

            char szFunctionName[200] = {'\0'};
            sprintf_safe(szFunctionName, 200, "\togic_%s\n", objProjectInfo.m_objCommandList[i].m_szCommandFuncName);
            _FunctionCode* pFunctionCode = NULL;
            Search_CAPI_Code(szFunctionName, obj_CPP_File_Info, pFunctionCode);

            if(NULL == pFunctionCode)
            {
                //�½�����
                sprintf_safe(szTemp, 200, "\t__ENTER_LOGIC()\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\tif(NULL == pServerObject)\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t{\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t\tOUR_DEBUG((LM_INFO, \"Logic_%s]pServerObject is NULL.\\n\"));\n",
                             objProjectInfo.m_objCommandList[i].m_szCommandFuncName);
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t\treturn false;\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t}\n\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

                //�鿴�Ƿ�������Ⱦɫ
                sprintf_safe(szTemp, 200, "\t//check your TraceID.\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\tDO_TRACE(\". /\", pTraceID);\n\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

                sprintf_safe(szTemp, 200, "\tif(0 == pMessageBase->m_u4ConnectID)\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t{\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t\tOUR_DEBUG((LM_INFO, \"Logic_%s]u4ConnectID is 0.\\n\"));\n",
                             objProjectInfo.m_objCommandList[i].m_szCommandFuncName);
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t\treturn false;\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t}\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

                sprintf_safe(szTemp, 200, "\t//add your code at here.\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);

                if(strlen(objProjectInfo.m_objCommandList[i].m_szCommandInID) > 0)
                {
                    sprintf_safe(szTemp, 200, "\t//CommandiID in %s.\n", objProjectInfo.m_objCommandList[i].m_szCommandInID);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                }

                if(strlen(objProjectInfo.m_objCommandList[i].m_szCommandOutID) > 0)
                {
                    sprintf_safe(szTemp, 200, "\t//CommandiID Out %s.\n", objProjectInfo.m_objCommandList[i].m_szCommandOutID);
                    fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                }

                sprintf_safe(szTemp, 200, "\treturn true;\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
                sprintf_safe(szTemp, 200, "\t__LEAVE_LOGIC_WITHRETURN(false)\n");
                fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
            }
            else
            {
                //��ԭ�д�����ֲ����
                fwrite(pFunctionCode->m_strCode.c_str(), pFunctionCode->m_strCode.length(), sizeof(char), pFile);
            }

            sprintf_safe(szTemp, 200, "}\n\n");
            fwrite(szTemp, strlen(szTemp), sizeof(char), pFile);
        }
    }

    fclose(pFile);
}

#endif
