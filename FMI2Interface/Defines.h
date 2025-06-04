#pragma once
// define class name and unique id
//#define MODEL_IDENTIFIER "VECU_CanDio_30"
#define MODEL_GUID "{E3C58495-9EC1-4954-8568-15c15471f40}"


// define model size
#define NUMBER_OF_STATES 2
#define NUMBER_OF_EVENT_INDICATORS 1

#define LOCAL_IP_ADDRESS "127.0.0.1"
#define VRTA_SERVER_PORT 17185
#define VINC_SENDER_PORT "5000"
#define VINC_RECEIVER_PORT 7000

#if defined(WIN32) || defined(_WIN32) 
#define PATH_SEPARATOR "\\" 
#else 
#define PATH_SEPARATOR "/" 
#endif

#ifdef __linux__
#ifdef __x86_64
#define BINARIES_DIR   "binaries/linux64/"
#define ADX_DIR        "linux64"
#else
#define BINARIES_DIR   "binaries/linux32/"
#define ADX_DIR        "linux32"
#endif
#else
#ifdef _WIN64
#define BINARIES_DIR   "binaries\\win64\\"
#define ADX_DIR        "win64"
#else
#define BINARIES_DIR   "binaries\\win32\\"
#define ADX_DIR        "win32"
#endif
#endif

#define RESOURCES_DIR   "resources"
