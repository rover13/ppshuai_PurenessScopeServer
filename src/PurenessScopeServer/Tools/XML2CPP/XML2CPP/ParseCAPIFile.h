#ifndef _PARSE_C_API_FILE_H
#define _PARSE_C_API_FILE_H

#include "Common.h"
#include <string>
using namespace std ;

#define CAPI_INCLUDE_BEGIN "//add your Include file under at here, don't delete this!\n"
#define CAPI_INCLUDE_END "//add your Include file end, don't delete this!\n\n"

//��¼���õĺ��������
struct _FunctionCode
{
	char   m_szFuncName[200];  //������
	string m_strFuncCode;      //������������
	string m_strCode;          //��������
	string m_strNotes;         //������������ע��
	bool   m_blIsUsed;

	_FunctionCode()
	{
		m_szFuncName[0] = '\0';
		m_blIsUsed      = false;
	}
};
typedef vector<_FunctionCode> vecFunctionCode;

//�ļ������Ϣ
struct _File_Info
{
	string m_strExtHead;                  //��¼������ͷ�ļ� 
	vecFunctionCode m_vecFunctionCode;    //��¼������Ϣ
};

//������������
bool Parse_Function_Name(char* pLine, char* pFunctionName);

//����������ͷ�ļ�
void Parse_File_Include(char* pData, int nFileSize, _File_Info& obj_File_Info);

//�������������к������ͺ�����
void Parse_File_Function_Info(char* pData, int nFileSize, _File_Info& obj_File_Info);

//�����ļ�����������ͷ�ļ�
bool Parse_CAPI_File(const char* pFileName, _File_Info& obj_File_Info);

//����ָ���ĺ�����Ӧ����Ϣ
bool Search_CAPI_Code(const char* pFuncName, _File_Info& obj_File_Info, _FunctionCode*& pFunctionCode);

#endif