#include <chrono>
#include"OsTargets.h"
#include <memory>
#ifdef  _WIN32
#include <thread>
#include "shellapi.h"
#elif __linux__
#include <pthread.h>
#endif
#include <atomic>
#include <vrtaClientSupport.h>
#include <stdio.h>
#ifdef _WIN32	
 #include <TlHelp32.h>	
#include <filesystem>
#include "Shlwapi.h"
#elif __linux__
	#include <cstdlib>
	#include <cstdarg>	
	#include <cstring>	
	#include <fstream>
    #include <sys/wait.h>
	#include <dirent.h>
	#include <sys/types.h> // for opendir(), readdir(), closedir()
	#include <sys/stat.h>  // for stat()		
	#include <limits.h>        // To use _POSIX_ARG_MAX
	#include <unistd.h>        // For Fork() call
	#include <sys/signal.h>	   // For SIGTERM call
	#include <sys/wait.h>	     // For waitpid () call
    #include <spawn.h>
    #include <stdlib.h>
	#define PROC_DIRECTORY		"/proc/"	
	#define OST_STRCMP_IC			strcasecmp
#include <unistd.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <limits.h>
#include <errno.h>
#endif	
#include "fmuTemplate.h"

class ProcessManager
{
public:
    static ProcessManager& getInstance( );
	
private:	
    static std::unique_ptr< ProcessManager> m_procMngr;
    friend std::unique_ptr< ProcessManager>::deleter_type;
	
	/** 
	* Constructor function
    */
    ProcessManager( );
	
	/** 
	* Destructor function
    */
    ~ProcessManager( );
	
public:

	/** To start the new process
    *         @param[in] strProcess :	Name of the process to be started 
    *		  @return		true      :	success
							false     :	failed
    */
    static bool StartProcess(const std::string& strProcess, std::string& strError, fmi2Component comp, int& iProcessID);
	
	/** To terminate the process
    *         @param[in] strProcess :	Name of the process to be terminated 
    */
	static void Terminateprocess(std::string& strProcess, fmi2Component comp);
    static void Terminateprocess(const int processID, fmi2Component comp);

	/** To check whether the given directory name is numeric
    *         @param[in] ccharptr_CharacterList :	Name of the directory 
    *		  @return		1      :	success
							0      :	failed
    */
	static int IsNumeric(const char* ccharptr_CharacterList);
	
	/** To get the pid of the given process
    *         @param[in] cchrptr_ProcessName :	Name of the process for which PID to be fetched 
    *		  @return		1      :	pid
							0      :	No process is running for the given name
		Note: Ignoring the case while comparing the process name
    */
	static int GetPIDbyName(const char* cchrptr_ProcessName);
	
	/** To check whether the given pid is present or not
    *         @param[in] pid :	Name of the directory 
    *		  @return		1      :	success
							0      :	failed
    */
	
private:


};