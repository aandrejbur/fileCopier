#include "filework.hpp"

WorkType FileWork::IdentifyWorkType()
{
    //1 lets see if in file name have 'delete_' prefix in it's name
    std::string name = inFile.filename().u8string();
    if(name.find("delete_") == 0) // delete must be at the file start
    {
        auto pos = strlen("delete_"); // lets simplify a bit, and just store position from which real file name starts
        // YYYYMMDDhhmmss
        std::string tsPart = name.substr(7,TSFormatSize);
        
        if(tsPart.length() == TSFormatSize && is_number(tsPart))
        {
            
            std::istringstream ss(tsPart);
            std::tm t = {0};
            ss >> std::get_time(&t, "%Y%m%d%H%M%S");
            if(!ss.fail()){
                deletionTime = mktime(&t);
                pos+=TSFormatSize+1;
                std::string temp = outFile.filename().u8string();
                outFile.replace_filename(temp.substr(pos));
                return WorkType::ON_TIME_DELETE;
            }
        }
        
        std::string temp = outFile.filename().u8string();
        outFile.replace_filename(temp.substr(pos));
        return WorkType::DELETE;
    }
        
    if(std::filesystem::exists(inFile))
    {
        if(std::filesystem::exists(outFile))
        {
            std::filesystem::directory_entry inDe(inFile);
            std::filesystem::directory_entry outDe(outFile);
            
            if(inDe.last_write_time() < outDe.last_write_time() && inDe.file_size() == outDe.file_size())
            {
                return WorkType::SKIP;
            }
        }
    }
    
    return WorkType::COPY;
}

FileWork::FileWork(std::filesystem::path src, std::filesystem::path target)
{
    inFile = src;
    outFile = target;
    work = IdentifyWorkType();
}

FileWork::FileWork(FileWork&& other)
{
    std::swap(inFile, other.inFile);    other.inFile.clear();
    std::swap(outFile, other.outFile);  other.outFile.clear();
    work = other.work;                  other.work = WorkType::SKIP;
    deletionTime = other.deletionTime;  other.deletionTime = 0;
}

FileWork& FileWork::operator=(FileWork&& other)
{
    if(this != &other)
    {
        std::swap(inFile, other.inFile);    other.inFile.clear();
        std::swap(outFile, other.outFile);  other.outFile.clear();
        work = other.work;                  other.work = WorkType::SKIP;
        deletionTime = other.deletionTime;  other.deletionTime = 0;
    }
    return *this;
}
