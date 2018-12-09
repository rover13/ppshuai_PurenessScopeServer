#include "ParseCAPIFile.h"

char * ltrim(char * s) 
{
	while (s && *s && isspace(*s)) ++s;
	return s;
}

bool Parse_Function_Name(char* pLine, char* pFunctionName)
{
	int nLen   = (int)strlen(pLine);
	int nBegin = 0;
	int nEnd   = 0;

	for(int i = 0; i < nLen; i++)
	{
		if(pLine[i] == ' ' && nBegin == 0)
		{
			nBegin = i + 1;
		}
		
		if(pLine[i] == '(')
		{
			nEnd = i;
			break;
		}
	}

	if(nEnd > nBegin)
	{
		memcpy(pFunctionName, (char*)pLine + nBegin, nEnd - nBegin);
		pFunctionName[nEnd - nBegin] = '\0';
		sprintf_safe(pFunctionName, 200, "%s", ltrim(pFunctionName));
		return true;
	}
	else
	{
		return false;
	}
}

void Parse_File_Include(char* pData, int nFileSize, _File_Info& obj_File_Info)
{
	//�а���������.h
	char* pIncludeBegin = strstr(pData, CAPI_INCLUDE_BEGIN);
	char* pIncludeEnd = strstr(pData, CAPI_INCLUDE_END);

	if(pIncludeBegin != NULL && pIncludeEnd != NULL && (int)(pIncludeEnd - pIncludeBegin) > 0)
	{
		//�ж���Ĵ��룬������������
		int nPosBegin = (int)(pIncludeBegin + (int)strlen(CAPI_INCLUDE_BEGIN) - pData);
		int nPosEnd   = (int)(pIncludeEnd - pData);

		if(nPosEnd - nPosBegin > 0)
		{
			char* pTemp = new char[nPosEnd - nPosBegin + 1];
			memcpy(pTemp, (char* )pData + nPosBegin, nPosEnd - nPosBegin);
			pTemp[nPosEnd - nPosBegin] = '\0';
			obj_File_Info.m_strExtHead = (string)pTemp;
			delete[] pTemp;
		}
		
	}
}

void Parse_File_Function_Info(char* pData, int nFileSize, _File_Info& obj_File_Info)
{
	char szLine[2000]  = {'\0'};
	int nLineBegin     = 0;
	int nLineEnd       = 0;
	bool blIsLineBehin = true;
	bool blIsContent   = false;

	char szTag[100] = {'\0'};
	memcpy(szTag, CAPI_INCLUDE_END, strlen(CAPI_INCLUDE_END) - 2);
	szTag[strlen(CAPI_INCLUDE_END) - 2] = '\0';

	char* pIncludeEnd = strstr(pData, szTag);
	int nPos = (int)(pIncludeEnd - pData) + (int)strlen(CAPI_INCLUDE_END);

	if(nPos < 0)
	{
		return;
	}

	nLineBegin = nPos;
	string strNotes = "";
	for(int i = nPos; i < nFileSize; i++)
	{
		if(pData[i] == '\n')
		{
			nLineEnd = i;
			if(nLineEnd > nLineBegin)
			{
				//�ҳ�һ���ı�
				memcpy(szLine, &pData[nLineBegin], nLineEnd - nLineBegin + 1);
				szLine[nLineEnd - nLineBegin + 1] = '\0';

				//�����ж��Ƿ��Ǻ����廹�Ǻ�����
				if(szLine[0] == '{')
				{
					blIsContent = true;
					strNotes    = "";
					nLineBegin  = i + 1;
					continue;
				}
				else if(szLine[0] == '}')
				{
					blIsContent = false;
					strNotes    = "";
					nLineBegin  = i + 1;
					continue;
				}

				if(false == blIsContent)
				{
					//����Ǻ���ͷע�ͣ����¼
					if(szLine[0] == '/' && szLine[1] == '/')
					{
						strNotes += (string)szLine;
					}

					//����ı���һ���ֽڲ���#,\n,\r,\�е��κ�һ��������Ϊ
					if(szLine[0] != '#' && szLine[0] != '\r' &&
						szLine[0] != '\n' && szLine[0] != '/')
					{
						//���Ϊ����ͷ
						_FunctionCode obj_FunctionCode;
						obj_FunctionCode.m_strFuncCode = (string)szLine;
						obj_FunctionCode.m_strNotes    = strNotes;
						Parse_Function_Name(szLine, obj_FunctionCode.m_szFuncName);
						obj_File_Info.m_vecFunctionCode.push_back(obj_FunctionCode);
						strNotes = "";
					}
				}
				else
				{
					//����Ǻ�����
					int nIndex = obj_File_Info.m_vecFunctionCode.size() - 1;
					_FunctionCode& obj_FunctionCode = obj_File_Info.m_vecFunctionCode[nIndex];
					obj_FunctionCode.m_strCode += (string)szLine;
				}

				nLineBegin = i + 1;
			}
			else
			{
				nLineBegin++;
			}
		}
		
	}
}

bool Parse_CAPI_File(const char* pFileName, _File_Info& obj_File_Info)
{
	char szBakFileName[200] = {'\0'};
	char* pFileBuff = NULL;

	if(NULL == pFileName)
	{
		return false;
	}
	sprintf_safe(szBakFileName, 200, "%s.bak", pFileName);

	FILE* pFile = fopen(pFileName, "rb+");
	if(NULL == pFile)
	{
		return false;
	}

	//�õ��ļ�����
	fseek(pFile, 0, SEEK_END);
	int nFileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	//ȡ���ļ����ݣ������ļ�
	pFileBuff = new char[nFileSize + 1];
	fread(pFileBuff, nFileSize, sizeof(char), pFile);
	pFileBuff[nFileSize] = '\0';
	fclose(pFile);
	FILE* pBakFile = fopen(szBakFileName, "wb");
	if(NULL == pBakFile)
	{
		return false;
	}
	fwrite(pFileBuff, nFileSize, sizeof(char), pBakFile);
	fclose(pBakFile);

	//��ʼ��������
	Parse_File_Include(pFileBuff, nFileSize, obj_File_Info);

	//�������еĺ�����
	Parse_File_Function_Info(pFileBuff, nFileSize, obj_File_Info);

	delete[] pFileBuff;
	return true;
}

bool Search_CAPI_Code(const char* pFuncName, _File_Info& obj_File_Info, _FunctionCode*& pFunctionCode)
{
	char szFunctionName[100] = {'\0'};
	sprintf_safe(szFunctionName, 100, "Exec_%s", pFuncName);

	for(int i = 0; i < (int)obj_File_Info.m_vecFunctionCode.size(); i++)
	{
		_FunctionCode& obj_FunctionCode = obj_File_Info.m_vecFunctionCode[i];
		if(strcmp(obj_FunctionCode.m_szFuncName, szFunctionName) == 0)
		{
			//����ҵ���
			pFunctionCode = &obj_FunctionCode;
			obj_FunctionCode.m_blIsUsed = true;
			return true;
		}
	}

	return false;
}
