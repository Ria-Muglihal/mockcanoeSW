#include "IPCFactory.h"
// #include "FMUSharedMemory.h"
#include "FMUIPC.h"
// #include "ProtoBufferTCP.h"
// #include "FlatBufferIPC.h"


IPCFactory::IPCFactory()
{
}


IPCFactory::~IPCFactory()
{
}

std::unique_ptr<IBaseIPC>IPCFactory::createIPCObject(IPC_TYPE ipcType)
{
	switch (ipcType)
	{
    case IPC:
        return std::unique_ptr<IBaseIPC>( new FMUTCP( ) );
        break;
	case IPC_TCP:
		// return std::unique_ptr<IBaseIPC>(new FlatBufferTCP());
		break;
	case IPC_TCP_PROTOBUFFER:
		// return std::unique_ptr<IBaseIPC>(new ProtoBufferTCP());
		break;
	case IPC_UDP: // fall-through
		// return std::unique_ptr<IBaseIPC>(new FlatBufferUDP());
		break;
	case IPC_SHARED_MEM:

		// Shared memory is not supported as shared memory stream need to be validated.
		//return std::unique_ptr<IBaseIPC>(new CFMUSharedMemory());
		return nullptr;
		break;
	default:
		break;
	}

	return nullptr;
}
