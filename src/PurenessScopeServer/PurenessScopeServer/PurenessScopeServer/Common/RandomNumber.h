#ifndef _RANDOMNUMBER_H
#define _RANDOMNUMBER_H

//����������ĺ���
//����ָ����Χ���������������

#include <stdlib.h>

#ifdef WIN32
#include "windows.h"
#include "wincrypt.h"
#else
#include <unistd.h>
#include <fcntl.h>
#endif

class CRandomNumber
{
public:
    CRandomNumber(void);
    ~CRandomNumber(void);

    void SetRange(int nMinNumber, int nMaxNumber);
    int GetRandom();

private:
    int GetRandomSeed();

private:
    int m_nMinNumber;
    int m_nMaxNumber;
};
#endif
