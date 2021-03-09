#include "etc.hpp"

// check if string consists only from dijits
bool is_number(const std::string &s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// Get separator based on OS
std::string GetFileSeparator()
{
    #ifdef _WIN32
        return = "\\";
    #else
        return "/";
    #endif
}

// parse CLI parameters into queue of string
void MakeParamsQueue( std::queue<std::string> &params, const int& argc, const char * argv[])
{
    for (int i = 1; i < argc; i++)
    {
        params.push(argv[i]);
    }
}

// parse CLI parameters into queue of string
void MakeParamsQueue( std::queue<std::string> &params,const std::string& command)
{
    std::istringstream iss(command);
    std::string part;
    
    while(iss >> std::quoted(part))
        params.push(part);
}

// Generate current time stamp as string
std::string getTimeStr()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y.%m.%d %H:%M:%S");
        
    return oss.str();
}

// remove files
std::string RemoveFiles(std::string file1, std::string file2)
{
    std::string msg;
    
    msg = RemoveFile(file1) + RemoveFile(file2);
    
    if(!msg.empty())
       return getTimeStr() + msg + "\n";
    else
       return"";
}

// remove single file, returns mesage with info
std::string RemoveFile(std::string file)
{
    std::string msg;
    namespace fs = std::filesystem;
    
    try {
        if(fs::exists(file))
        {
            if(fs::remove(file))
            {
                msg.append(" File '" + file + "' Deleted.");
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        msg.append("Error occured while deleting file: '" + file + "': " + e.what());
    }
    return msg;
}

std::string CopyFile(std::string in, std::string out)
{
    std::string msg;
    std::string opState = "Copied";
    namespace fs = std::filesystem;
    
    try {
        if(fs::exists(in))
        {
            if(fs::exists(out))
            {   // both files exists, so it must be update
                opState = "Updated";
                fs::directory_entry inDe(in);
                fs::directory_entry outDe(out);
                
                if(inDe.last_write_time() != outDe.last_write_time() || inDe.file_size() != outDe.file_size())
                {
                    if(fs::copy_file(in, out, fs::copy_options::update_existing))
                    {
                        msg.append(" File '" + in + "' updated as '" + out + "'");
                    }
                }
            }
            else
            {
                if(fs::copy_file(in, out, fs::copy_options::update_existing))
                {
                    msg.append(" File '" + in + "' copied into '" + out + "'");
                }
            }
        }
        
    } catch (const std::filesystem::filesystem_error& e) {
        msg.append(" Error occured while copying: '" + in + " into '"+ out +"': " + e.what());
    }
    
    if(!msg.empty())
       return getTimeStr() + msg + "\n";
    else
       return"";
}
