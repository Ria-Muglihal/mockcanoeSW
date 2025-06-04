#pragma once

#include "BaseIPC.h"
#include <memory>

typedef enum {
    IPC, // Enum Value for TCP Class without Protobuf API's
	IPC_TCP,
	IPC_TCP_PROTOBUFFER,
	IPC_SHARED_MEM,
	IPC_UDP,
	IPC_UNKNOWN
} IPC_TYPE;

class IPCFactory
{
public:
	IPCFactory();
	virtual ~IPCFactory();
	static std::unique_ptr<IBaseIPC> createIPCObject(IPC_TYPE ipcType);
};

