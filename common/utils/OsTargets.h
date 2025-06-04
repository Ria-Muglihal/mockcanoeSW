

/************************************** HEADER SECTION **************************************/

#ifdef _WIN32
	#define _WINSOCKAPI_        // to avoid re-definition errors while including ssh
	#include<Windows.h>
	#include <io.h>
	#include <tchar.h>
	#include <string>
	#include <direct.h>
#elif __linux__
	#include <sys/io.h>
	#include <unistd.h>
	#include <limits.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <string.h>
	#include <dirent.h>
	#include <signal.h>
	#include <stdio.h>
	#include <sys/types.h>
	#include<sys/stat.h>
#endif
/********************************************************************************************/

/************************************** DEFINES SECTION **************************************/

#ifdef _WIN32

	#define OST_PATH_SEP									'\\'
	#define OST_STRCMP_IC									_stricmp                        // compare strings ignoring case
	#define OST_CUR_PID										GetCurrentProcessId()
    #define checkStrdup(str)                               _strdup(str)
	#define	VS_PRINTF(format,argp)						   _vscprintf(format, argp)
	#define GetProcAddress								   GetProcAddress
	#define Sleep(x)									   Sleep(x)
	#define LoadLibraryA(dllPath)						   LoadLibraryA(dllPath)
#elif __linux__

	#define HANDLE											void*	
	#define DWORD 											unsigned long
	#define BOOL 												int
	#define TRUE 												1
	#define FALSE 												0	

	#define OST_PATH_SEP									'/'
	#define OST_STRCMP_IC									strcasecmp						// compare strings ignoring case
	#define Sleep(x)										usleep(x*1000)						// Input unit is in seconds
	#define OST_CUR_PID						 				getpid()						// get the current process id	
	#define	 checkStrdup(str)	                            strdup(str)
	#define	VS_PRINTF(format,argp)						    vsnprintf(NULL, 0, format, argp)
	#define GetProcAddress								    dlsym
	#define LoadLibraryA(dllPath)						   dlopen(dllPath,RTLD_LAZY)
#endif

/*********************************************************************************************/

