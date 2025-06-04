#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <poll.h>
#include <unistd.h>

extern bool fmi2DoStep(double communicationStepSize);
extern void onPreStart();
extern void onstart();
extern void setParameter();
extern bool fmi2DoStep(double communicationStepSize);
extern void transactionofTxRxData();
extern bool initSocketComm();
extern void setReset();

//
extern bool sim_reset;
extern bool reset;

//
std::mutex io_mutex;
bool blockSendingTick = false;
bool blockSendingData = true;

void userCommandHandler() 
{
    std::string input;
    while (true) 
    {
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "\nEnter command (r = reset, q = quit): ";
        }
        std::getline(std::cin, input);

        if (input == "q") {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Exiting program...\n";
            exit(0);
        } 
        else {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Invalid input.\n";
        }
    }
}

int main()
{
    onPreStart();
    
    if( initSocketComm() )
    {
        std::thread t1(fmi2DoStep, 0.005);
        t1.detach();
    }
    
    onstart();
    
    std::thread t2(setReset);
    t2.detach();
        
    std::thread t3(transactionofTxRxData);
    t3.detach();
    
    for(int i = 0; i<5000; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
    }
           
    std::thread inputThread(userCommandHandler);
    inputThread.join();  // block main until user exits
    
    return 0;
}