#pragma once
#ifdef  _WIN32
#include <string>
#elif __linux__
#include <string.h>
#include<iostream>
#endif
#include <cstdint>
struct AddressData {

	AddressData()
	{
		bIsGet = false;
		memset(&name, 0, sizeof(name));

		offset = 0;
		size = 0;
		address = 0;
		memset(&value, 0, sizeof(value));
		id = 0;
	}

	bool bIsGet;
	char name[200];
	uint32_t  offset;
	uint32_t  size;
	long address;
	unsigned char value[7000];
	int id;
};

typedef AddressData AddressDataType;


const static int IPC_PKT_DATA_SIZE = 8192;
typedef enum {
	IPC_RETURN_SUCCESS,
	IPC_RETURN_ERROR,
	IPC_RETURN_WARNING,
	IPC_RETURN_RESET
} IPC_RETURN_TYPE;

typedef enum {
    IPC_ACK_OK = 1,
    IPC_ACK_ERROR,
	IPC_ACK_RESET
} IPC_ACK_TYPE;

class IBaseIPC
{
public:
	virtual IPC_RETURN_TYPE InitCommunication(const std::string& strPortORFileName ,const std::string& strIP_Address) = 0;
	virtual IPC_RETURN_TYPE OpenCommunication() = 0;
	virtual IPC_RETURN_TYPE WriteData(void* pMemData, int iDataSize) = 0;
    virtual IPC_RETURN_TYPE ReadData( void* pMemData, int iDataSize,int* readDataSize ) = 0;
    virtual IPC_RETURN_TYPE ReadStatus(int* recv_value ) = 0;
	virtual IPC_RETURN_TYPE waitForServerToClose() = 0;
	virtual std::string getErrorDescription() = 0;
	virtual ~IBaseIPC() {}
	virtual void CloseCommunication() = 0;
};

