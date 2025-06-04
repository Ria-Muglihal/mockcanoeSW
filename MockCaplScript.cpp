#include <iostream>
#include <thread>
#include <chrono>
#include <dlfcn.h>
#include "MockCaplSystem.h"
#include "MockMessages.h"

//
extern bool blockSendingTick;
extern bool blockSendingData;


//
bool reset = false;
bool something = false;

// Constants
constexpr int SIM_NORESET = 0;
constexpr int master = 1;

// Globals
char ipaddress[16] = "127.0.0.1";
int port = 2019;
double stepsize = 5.0;
int sim_reset = SIM_NORESET;
int gHandle = 0;
void *glibHandle = nullptr;

// class instance
MockCapl mockCapl;

typedef void (*RegisterCDLLFunc)(VIACapl *);
typedef void (*InitializeFunc)(uint32 handle, char *ipaddress, uint32 port, uint32 ismaster);
typedef void (*SetParamFunc)(uint32 handle, uint32 parameter);
typedef void (*transactionofTxRxDataFunc)(uint32 handle);
typedef void (*SetCanFrameFunc)( uint32 handle,unsigned long channel, 
                                unsigned long direction,unsigned long canid,
                                long long timestamp,unsigned long type,
                                unsigned long dlc, unsigned long rtr,
                                unsigned long fdf, unsigned long brs,
                                unsigned long esi, unsigned char payload[]);

void onPreStart()
{
    glibHandle = dlopen("/home/mgl1kor/git_repositories/pjvecu/onesilcontroller/mockCanoeSW/libs/libcaplserver.so", RTLD_LAZY);
    if (!glibHandle)
    {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
    }
    
    RegisterCDLLFunc registerCAPLDLL = (RegisterCDLLFunc)dlsym(glibHandle, "VIARegisterCDLL");
    if (!registerCAPLDLL)
    {
        std::cerr << "Failed to find VIARegisterCDLL in DLL." << std::endl;
    }
    
    std::cout << "--------------------- CAPL-DLL Registration -----------------------" << std::endl;
    std::cout << "Start procedure:" << std::endl;
    
    registerCAPLDLL(&mockCapl);
}

void setParameter()
{
    SetParamFunc setParamCAPLDLL = (SetParamFunc)dlsym(glibHandle, "_Z8SetParamjj");
    setParamCAPLDLL(0xBEEF, sim_reset); // needs 2019 port
}

void onstart()
{    
    InitializeFunc initCAPLDLL = (InitializeFunc)dlsym(glibHandle, "_Z7appInitjPcjj");
    initCAPLDLL(0xBEEF, ipaddress, port, master); //2019 3-way handshake established
    
    std::cout << "Capl DLL Initialization: Done!" << std::endl;
    
    setParameter();    
}

void onAnyCanMessage(CanMessage& msg)
{
    
    SetCanFrameFunc setCanFrame = (SetCanFrameFunc)dlsym(glibHandle,"_Z11setCanFramejmmmxmmmmmmPh");
    
    setCanFrame(0xBEEF,
                msg.channel,
                msg.direction,
                msg.id,
                msg.timestamp_ns,
                msg.type,
                msg.dlc,
                msg.rtr,
                msg.fdf,
                msg.brs,
                msg.esi,
                msg.data);
}

void setReset()
{
    while(!reset)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
    }

    sim_reset = 4;
    setParameter();
    std::cout << "reset complete" << std::endl;
    
    something = true;
}

void transactionofTxRxData()
{
    transactionofTxRxDataFunc transactionofTxRxData = (transactionofTxRxDataFunc)dlsym(glibHandle, "_Z21transactionofTxRxDataj");
    static int counter = 0;
    
    while(true)
    {
        for (int i = 0; i < 1; ++i) 
        {
            CanMessage msg{};
            msg.id = 0x100 + i;
            msg.channel = 5;
            msg.direction = 0;
            msg.timestamp_ns = 1000000 * i;
            msg.type = 1;
            msg.dlc = 8;
            msg.rtr = 0;
            msg.fdf = 0;
            msg.brs = 0;
            msg.esi = 0;
    
            // Example payload: fill with 0x10 + i
            for (int j = 0; j < msg.dlc; ++j) 
            {
                msg.data[j] = 0x10 + i + j;
            }
    
            onAnyCanMessage(msg);   //setcanframe one message at a time in caplserver for 10times.
        }
        
        while (blockSendingData == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        transactionofTxRxData(0xBEEF); 
        
        blockSendingData = true;
        blockSendingTick = false;  
        std::cout << "data " << ++counter;
    }   
}