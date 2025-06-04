
/*

#include "FMUSharedMemory.h"

const DWORD CFMUSharedMemory::m_dwWaitTime = INFINITE;
CFMUSharedMemory::CFMUSharedMemory() : m_pShMemClient (nullptr)
, m_pShMemServer(nullptr)
, m_hShMemClient(nullptr)
, m_hShMemServer(nullptr)
, m_hClientAvailable(nullptr)
, m_hClientRead(nullptr)
, m_hServerRead(nullptr)
, m_hClientWrite(nullptr)
, m_hServerWrite(nullptr)
, m_hShmServerMutex(nullptr)
, m_hShmClientMutex(nullptr)
, m_strClientName(L"FMUSHAREDMEMCLIENT")
, m_strServerName(L"FMUSHAREDMEMSERVER")
{

}

CFMUSharedMemory::~CFMUSharedMemory()
{
ReleaseMutex(m_hShmClientMutex);
UnmapViewOfFile(m_pShMemClient);
CloseHandle(m_hShMemClient);
UnmapViewOfFile(m_pShMemServer);
CloseHandle(m_hShMemServer);
CloseHandle(m_hClientAvailable);

CloseHandle(m_hClientWrite);
CloseHandle(m_hServerRead);
CloseHandle(m_hClientRead);
CloseHandle(m_hServerWrite);

ReleaseMutex(m_hShmServerMutex);

CloseHandle(m_hShmServerMutex);
CloseHandle(m_hShmClientMutex);
}

std::string CFMUSharedMemory::getErrorDescription()
{
return m_errorDescription;
}

IPC_RETURN_TYPE CFMUSharedMemory::InitCommunication(const std::string& strPortORFileName, const std::string& strIP_Address)
{
m_strSHMFieNameSuffix.assign(strPortORFileName.begin(), strPortORFileName.end());
m_strClientName.append(L"_").append(m_strSHMFieNameSuffix);
m_strServerName.append(L"_").append(m_strSHMFieNameSuffix);

// Create the mutex
wchar_t sClientMutex[MAX_PATH];
wcscpy(sClientMutex, m_strClientName.c_str());
wcscat(sClientMutex, L"ClientMutex");
m_hShmClientMutex = CreateMutex(nullptr, FALSE, sClientMutex);

if (m_hShmClientMutex)
{
DWORD dwResult = WaitForSingleObject(m_hShmClientMutex, 0);
if (dwResult != WAIT_OBJECT_0)
{
// Error waiting return false;
CloseHandle(m_hShmClientMutex);
return IPC_RETURN_ERROR;
}
}
else
{
// Some error
return IPC_RETURN_ERROR;
}

wchar_t sServerMemFile[MAX_PATH]; // Server mapping file we open this for write mode.
wcscpy(sServerMemFile, m_strServerName.c_str());
if ((m_hShMemServer = CreateFileMapping(INVALID_HANDLE_VALUE,
nullptr,
PAGE_READWRITE,
0,
sizeof(int) + IPC_PKT_DATA_SIZE,
sServerMemFile)) != nullptr)
{
if ((m_pShMemServer = (BYTE*)MapViewOfFile(m_hShMemServer,
FILE_MAP_WRITE,
0,
0,
IPC_PKT_DATA_SIZE + sizeof(int))) != nullptr)
{
wchar_t sServerWrite[MAX_PATH];
wchar_t sClientRead[MAX_PATH];

wcscpy(sServerWrite, m_strServerName.c_str());
wcscat(sServerWrite, L"ServerWrite");
wcscpy(sClientRead, m_strClientName.c_str());
wcscat(sClientRead, L"ClientRead");

m_hClientRead = CreateEventW(nullptr, FALSE, TRUE, sClientRead);
m_hServerWrite = CreateEventW(nullptr, FALSE, FALSE, sServerWrite);


// Notify the client saying the server is started.
wchar_t sClientAvl[MAX_PATH];
wcscpy(sClientAvl, m_strClientName.c_str());
wcscat(sClientAvl, L"Avl");
m_hClientAvailable = CreateEventW(nullptr, FALSE, FALSE, sClientAvl);
SetEvent(m_hClientAvailable);
return IPC_RETURN_SUCCESS;
}
else
{
// Failed to map the file
}
CloseHandle(m_hShMemServer);
}
else
{
// Failed to create the file mapping
}

CloseHandle(m_hShmClientMutex);
return IPC_RETURN_ERROR;
}

IPC_RETURN_TYPE CFMUSharedMemory::OpenCommunication()
{
// Create the mutex
wchar_t sServerMutex[MAX_PATH];
wcscpy(sServerMutex, m_strServerName.c_str());
wcscat(sServerMutex, L"ServerMutex");
m_hShmServerMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, sServerMutex);
if (m_hShmServerMutex)
{

DWORD dwResult = WaitForSingleObject(m_hShmServerMutex, 0);
if (dwResult != WAIT_OBJECT_0)
{
// Error waiting return false;
CloseHandle(m_hShmServerMutex);
return false;
}
  }
  else
  {
    // Some error
    return IPC_RETURN_ERROR;
  }

  wchar_t sServerRead[MAX_PATH];
  wchar_t sClientWrite[MAX_PATH];

  wcscpy(sServerRead, m_strServerName.c_str());
  wcscat(sServerRead, L"ServerRead");
  wcscpy(sClientWrite, m_strClientName.c_str());
  wcscat(sClientWrite, L"ClientWrite");

  m_hServerRead = OpenEventW(EVENT_ALL_ACCESS, FALSE, sServerRead);
  m_hClientWrite = OpenEventW(EVENT_ALL_ACCESS, FALSE, sClientWrite);

  if (m_hServerRead == nullptr || m_hClientWrite == nullptr)
  {
    CloseHandle(m_hShmServerMutex);
    return IPC_RETURN_ERROR;
  }

  wchar_t sClientMemFile[MAX_PATH]; // Server mapping file we open this for read only mode.
  wcscpy(sClientMemFile, m_strClientName.c_str());

  if ((m_hShMemClient = OpenFileMapping(FILE_MAP_READ,
    FALSE,
    sClientMemFile)) != nullptr)
  {
    if ((m_pShMemClient = (BYTE*)MapViewOfFile(m_hShMemClient,
      FILE_MAP_READ,
      0,
      0,
      IPC_PKT_DATA_SIZE + sizeof(int))) != nullptr)
    {
      // Is client avaialble
      wchar_t sServerAvl[MAX_PATH];
      wcscpy(sServerAvl, m_strServerName.c_str());
      wcscat(sServerAvl, L"Avl");
      HANDLE hServerAvailable = OpenEventW(EVENT_ALL_ACCESS, FALSE, sServerAvl);
      if (WaitForSingleObject(hServerAvailable, 0) == WAIT_OBJECT_0)
      {
        CloseHandle(hServerAvailable);
        return IPC_RETURN_SUCCESS;
      }
      CloseHandle(hServerAvailable);
      UnmapViewOfFile(m_pShMemClient);
    }
    else
    {
      // Failed to map the file
    }

    CloseHandle(m_hShMemClient);
  }
  else
  {
    // Failed to open the file mapping
  }

  CloseHandle(m_hShmServerMutex);
  return IPC_RETURN_ERROR;
}
IPC_RETURN_TYPE CFMUSharedMemory::WriteData(void* pMemData, int iDataSize)
{
  // Check whether the client has read the data.
  HANDLE  hHandles[2];
  hHandles[0] = m_hServerRead;
  hHandles[1] = m_hShmServerMutex;
  DWORD   dwResult = WaitForMultipleObjects(2, hHandles, FALSE, m_dwWaitTime);
  switch (dwResult)
  {
  case WAIT_OBJECT_0:
    if (iDataSize > IPC_PKT_DATA_SIZE)
      return IPC_RETURN_ERROR;
    // Data was read from the shared memory pool, write new data
    // and notify any listener that new data was written.
    memcpy(m_pShMemServer, pMemData, iDataSize);
    SetEvent(m_hServerWrite);
    return IPC_RETURN_SUCCESS;
  case WAIT_OBJECT_0 + 1:
    // The client has closd
    // Release mutex as we have locked it
    ReleaseMutex(m_hShmServerMutex);
    return IPC_RETURN_ERROR;
  case WAIT_ABANDONED_0 + 1:
    // Something happened to the client.
    ReleaseMutex(m_hShmServerMutex);
    return IPC_RETURN_ERROR;
  case WAIT_FAILED:
    if (!m_hServerRead)
      return IPC_RETURN_ERROR;
    // Cause for strange failure
    return IPC_RETURN_ERROR;
  case WAIT_TIMEOUT:
    // The client has not read the data we sent
    return IPC_RETURN_ERROR;
  }
  return IPC_RETURN_ERROR;
}

IPC_RETURN_TYPE CFMUSharedMemory::ReadData(void* pMemData, int iDataSize)
{
  // Check whether the client has read the data.
  HANDLE  hHandles[2];
  hHandles[0] = m_hClientWrite;
  hHandles[1] = m_hShmServerMutex;
  DWORD   dwResult = WaitForMultipleObjects(2, hHandles, FALSE, m_dwWaitTime);

  switch (dwResult)
  {
  case WAIT_OBJECT_0:
    if (iDataSize > IPC_PKT_DATA_SIZE)
      return IPC_RETURN_ERROR;
    // Data was read from the shared memory pool, write new data
    // and notify any listener that new data was written.
    memcpy(pMemData, m_pShMemClient, iDataSize);
    SetEvent(m_hClientRead);
    return IPC_RETURN_SUCCESS;
  case WAIT_OBJECT_0 + 1:
    // The client has closd
    // Release mutex as we have locked it
    ReleaseMutex(m_hShmServerMutex);
    return IPC_RETURN_ERROR;
  case WAIT_ABANDONED_0 + 1:
    // Something happened to the client.
    ReleaseMutex(m_hShmServerMutex);
    return IPC_RETURN_ERROR;
  case WAIT_FAILED:
    if (!m_hClientWrite)
      return IPC_RETURN_ERROR;

    // Cause for strange failure
    return IPC_RETURN_ERROR;
  case WAIT_TIMEOUT:
    // The Server has not read the data we sent
    return IPC_RETURN_ERROR;
  }
  return IPC_RETURN_ERROR;
}
*/