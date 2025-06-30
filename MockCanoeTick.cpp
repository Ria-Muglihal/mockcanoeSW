#include <iostream>
#include <thread>
#include <memory>
#include <chrono>
#include <atomic>
#include "FMI2Interface/FMUIPC.h"
#include "FMI2Interface/IPCFactory.h"

std::atomic<bool> runloop {true};
std::unique_ptr<IBaseIPC> m_SILConIPCObject; 

extern bool blockSendingTick;
extern bool blockSendingData;
extern bool reset;
extern bool something;
extern void setReset();

int totalTicks  = 0;
#define  PACKET_SERVER_READY 1001

typedef enum
{
    fmi2OK,
    fmi2Warning,
    fmi2Discard,
    fmi2Error,
    fmi2Fatal,
    fmi2Pending
} fmi2Status;

bool init_SocketConn_FmuTick()
{
    bool error;
    error = (m_SILConIPCObject = IPCFactory::createIPCObject(IPC_TYPE::IPC)) != nullptr ? true : false;
 
    if (error && m_SILConIPCObject->InitCommunication("8000", "127.0.0.1") == IPC_RETURN_SUCCESS)
    {
        if (IPC_RETURN_SUCCESS == m_SILConIPCObject->OpenCommunication())
        {
            return true;
        }
    }
    return false;
}

bool executeStep(int count)
{
    fmi2Status status = fmi2Error;
    uint64_t stepsize = (uint64_t)count;
    
    m_SILConIPCObject->WriteData(&stepsize, sizeof(uint64_t));
    int recv_value = 0;
    m_SILConIPCObject->ReadStatus(&recv_value);
    
    if (IPC_ACK_OK == recv_value)
    {
        status = fmi2OK;
    }
    else if(IPC_ACK_RESET == recv_value)
    {
        std::cout << "Received: Reset signal from silcontroller" << std::endl;
        
        int send_value = IPC_ACK_OK;
        m_SILConIPCObject->WriteData(&send_value, sizeof(uint64_t));

        m_SILConIPCObject->CloseCommunication();
        m_SILConIPCObject->waitForServerToClose(); 

        // Reconnect + wait for ready packet
        IPC_RETURN_TYPE connected = IPC_RETURN_ERROR;
        
        while (true)
        {
            connected = m_SILConIPCObject->OpenCommunication();
            if (connected == IPC_RETURN_SUCCESS)
            {
                // Wait for server to send PACKET_SERVER_READY (e.g., int value)
                int ready = 0;
                int rc = m_SILConIPCObject->ReadStatus(&ready);
                if (rc == IPC_RETURN_SUCCESS && ready == PACKET_SERVER_READY)
                {
                    std::cout << "[fmi2DoStep] Server is ready\n";
                    break;
                }
                else
                {
                    std::cerr << "[fmi2DoStep] Server not ready yet, retrying...\n";
                    m_SILConIPCObject->CloseCommunication(); 
                }
            }

            sleep(2); 
        }
        
        status = fmi2OK;
        recv_value = IPC_ACK_OK;
    }
    else
    {
        status = fmi2Error;
        return false;
    }
    
    if (totalTicks == 1000)
    {
        setReset();
    }
    
    std::cout << "total ticks: " << totalTicks << std::endl;
    
    totalTicks = totalTicks + 1;
   
    return true;
    
}

bool doStep(std::chrono::microseconds cycle_time)
{
    auto count = cycle_time.count();
    return (executeStep(static_cast<int>(count)));
}

void fmi2DoStep(double communicationStepSize)
{
    int valueMicroSec = int(communicationStepSize * 1000 * 1000);
    doStep(std::chrono::microseconds(valueMicroSec));
}
