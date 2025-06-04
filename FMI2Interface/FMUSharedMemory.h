/*
#pragma once

#include "BaseIPC.h"
#include <winsock2.h>

class CFMUSharedMemory : IBaseIPC
{
public:
	CFMUSharedMemory();
	virtual ~CFMUSharedMemory();

	virtual IPC_RETURN_TYPE InitCommunication(const std::string& strPortORFileName, const std::string& strIP_Address) override;
	virtual IPC_RETURN_TYPE OpenCommunication() override;
	virtual IPC_RETURN_TYPE WriteData(void* pMemData, int iDataSize) override;
	virtual IPC_RETURN_TYPE ReadData(void* pMemData, int iDataSize) override;
	virtual std::string getErrorDescription() override;


private:
	BYTE *m_pShMemClient;
	BYTE *m_pShMemServer;

	HANDLE m_hShMemClient;
	HANDLE m_hShMemServer;
	HANDLE m_hClientAvailable;
	HANDLE m_hClientRead;   // Has client read?
	HANDLE m_hServerRead;
	HANDLE m_hClientWrite;
	HANDLE m_hServerWrite;  // Server Write the data
	HANDLE m_hShmServerMutex;
	HANDLE m_hShmClientMutex;

	std::wstring m_strClientName;
	std::wstring m_strServerName;
	std::string m_errorDescription;
	std::wstring m_strSHMFieNameSuffix;
	static const DWORD m_dwWaitTime;
};
*/
