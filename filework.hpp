#ifndef filework_hpp
#define filework_hpp

#include <filesystem>
#include <string>
#include <sstream>
#include <cstring>
#include "etc.hpp"

enum class WorkType{SKIP = 0, COPY = 1, DELETE = 2, ON_TIME_DELETE = 3};

class FileWork
{
public:
    std::filesystem::path inFile;
    std::filesystem::path outFile;
    WorkType work{0};
    time_t deletionTime{0};
    
    FileWork(){}
    FileWork(std::filesystem::path src, std::filesystem::path target);
    WorkType IdentifyWorkType();
    
    FileWork(FileWork&& other);
    FileWork& operator=(FileWork&& other);

private:
    // items are not copyable, i dont need it, and no one should
    FileWork(const FileWork&) = delete;
    FileWork& operator=(const FileWork&) = delete;
};

#endif /* filework_hpp */
