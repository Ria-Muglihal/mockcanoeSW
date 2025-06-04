#ifdef  __linux__
#include<sys/types.h>
#include <dirent.h>
#endif
#include <vector>
//#include "fmuTemplate.h"
#ifdef  _WIN32
#include <Windows.h>
#include <string>
#elif __linux__
  #include <string.h>
#endif

#include <memory>
#include <vector>
class FileManager
{
    private:
      static std::unique_ptr< FileManager> m_FileMngr;
      friend std::unique_ptr< FileManager>::deleter_type;
     public:
        static FileManager& getInstance();
        static std::vector<std::string> filteredFileList(const char* strPath, const char* strExt);
};