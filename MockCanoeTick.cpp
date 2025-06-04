// // MOCK canoe software

#include <iostream>
#include <thread>
#include <memory>
#include <chrono>
#include <atomic>
#include "FMI2Interface/FMUIPC.h"
#include "FMI2Interface/IPCFactory.h"

std::atomic<bool> runloop {true};  // Shared stop signal

std::unique_ptr<IBaseIPC> m_SILConIPCObject; // Do step
extern bool blockSendingTick;
extern bool blockSendingData;
extern bool reset;
extern bool something;

typedef enum
{
    fmi2OK,
    fmi2Warning,
    fmi2Discard,
    fmi2Error,
    fmi2Fatal,
    fmi2Pending
} fmi2Status;

bool initSocketComm()
{
     // init
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
    int counter  = 0;
    
    while(true)
    {   
        while (!runloop) {
            if(!reset) {
                std::cout << "Loop exit signal received.\n";
                reset = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
            if(something)
            {
                runloop = true;
            }
        }
         
        while (blockSendingTick == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        m_SILConIPCObject->WriteData(&stepsize, sizeof(uint64_t));
        int recv_value = 0;
        m_SILConIPCObject->ReadStatus(&recv_value);

        
        if(IPC_ACK_RESET == recv_value)
		{
			std::cout << "Received: Reset signal from silcontroller" << std::endl;

			m_SILConIPCObject->CloseCommunication();
			m_SILConIPCObject->OpenCommunication();
            //Send Init Data to Server
            
            std::string strError;
            std::string resourcepath("");
            
			status = fmi2OK;
		}
        
        if (IPC_ACK_OK != recv_value)
        {
            status = fmi2Error;
            return false;
        }
        
        blockSendingTick = true;
        blockSendingData = false;
        
        std::cout << "tick " << ++counter;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
        
        if (counter > 1000) {
            std::cout << "Auto-breaking loop\n";
            runloop = false;  // Break from within
        }
    }
}

bool doStep(std::chrono::microseconds cycle_time)
{
    auto count = cycle_time.count();
    return (executeStep(static_cast<int>(count)));
}

void fmi2DoStep(double communicationStepSize)
{
    try
    {
        int valueMicroSec = int(communicationStepSize * 1000 * 1000);
        doStep(std::chrono::microseconds(valueMicroSec));
    }
    catch (const std::exception &e) 
    {
        std::cerr << "Exception in dostep: " << e.what() << std::endl;
    } 
    catch (...) 
    {
        std::cerr << "Unknown exception in dostep!" << std::endl;
    }
}
