
#include "FileManager.h"
#include <string>

std::unique_ptr< FileManager> FileManager::m_FileMngr(nullptr);

FileManager& FileManager::getInstance()
{
  if (m_FileMngr.get() == nullptr)
    m_FileMngr.reset(new FileManager());

  return *m_FileMngr.get();
}
// -------------------------------------------------------------------
// Get the all the adx file names present in the fmu/resources folder
// @input: adxPath - Path to resource folder
// -------------------------------------------------------------------
std::vector<std::string> FileManager::filteredFileList(const char* strPath, const char* strExt)
{
  
#ifdef _WIN32
    std::vector<std::string> fileNames;
    LPWIN32_FIND_DATAA fd = new WIN32_FIND_DATAA;

    std::string fileExtension = "";
    fileExtension = fileExtension.append(strPath).append(strExt);

    HANDLE hFind = ::FindFirstFileA(LPCSTR(fileExtension.c_str()), fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Read all files having extension .adx from the %temp%\\fmu\\resources folder
            if (!(fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string filePath = strPath;
                filePath = filePath.append(fd->cFileName);
                fileNames.push_back(filePath);
            }
        } while (::FindNextFileA(hFind, fd));

        ::FindClose(hFind);
    }
    delete fd;
    return fileNames;
#elif __linux__
    std::vector<std::string> fileNames;
    DIR* dirFile = opendir(strPath);
    struct dirent* hFile;
    while ((hFile = readdir(dirFile)) != NULL)
    {
        std::string fn = hFile->d_name;
        std::string  Ext = strExt;
        std::string::size_type i = Ext.find("*.");
        if (i != std::string::npos)
            Ext.erase(i, i+2);
        if (fn.substr(fn.find_last_of(".") + 1) ==Ext)
        {
                std::string filePath = strPath;
                filePath = filePath.append(hFile->d_name);
                fileNames.push_back(filePath);
         
        }
    }
    closedir(dirFile);
    return fileNames;
 #endif
}