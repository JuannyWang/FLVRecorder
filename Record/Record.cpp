#include "StdAfx.h"
#include "RecordImp.h"


BOOL APIENTRY DllMain( HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved )
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}


IRecord *CreateRecord()
{
    CRecordImp *pRecord = new CRecordImp;
    return pRecord;
}

void DestoryRecord(IRecord *pRecord)
{
    if (pRecord)
    {
        delete pRecord;
        pRecord = NULL;
    }
}