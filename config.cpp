#include <fstream>
#include <mutex>
#include <sstream>
#include <queue>
#include <iostream>

#include "config.hpp"
#include <filesystem>

static const std::string confFile = "config.conf";   // the path to the config file

const std::string help = {
    "THe application recursevely copies content from inoput directory into output directory\n"
    "The application can also delete files that are marked with 'delete_' prefix\n"
    "Another option is to delete file in some exact time moment, it can be achieved with 'delete_YYYYMMDDhhmmss_' prefix\n"
    "Also program can print entire or filtered log file into console\n"
    "Below are basic commands for app usage:\n"
    "   '-in DIR_PATH' to set or reset input directory\n"
    "   '-out DIR_PATH' to set or reset backup directory\n"
    "   '-log FILE_PATH' to set exact or relative location of log file, by default it uses 'log.log' near aplication binary\n"
    "   '-ext EXTENSION' extension which will be added for files in output directory\n"
    "   '-exit' or '-stop' as a sign to stop the application in proper way\n"
    "   '-showlog' to display entire log file\n"
    "   '-filter EXPRESSION' show filtered log file based on EXPRESSION shows lines with exact matchof any symbol or as regex expression\n"
    "   '-h' or '-help' or '--help' to display programm usage\n"
    "   '-F' to set log flush option (i.e. every message will be printed into log without caching)\n"
    "   '--version' or '-v' prints version of program, just a good way for support what is common\n"
};

void Config::WriteConfigIntoFile()
{
    std::ofstream ConfigFileStream;
    ConfigFileStream.open(confFile, std::ios_base::out| std::ios_base::trunc);
    
    // i don't like a big parts of code to be copy pasted when there is no logic in it, so lets use lambda)
    auto PrintParam = [&ConfigFileStream](std::string name, std::string& value)
    {
        ConfigFileStream << name << "=" << value << std::endl;
    };
    
    PrintParam("logfile", logFile);
    PrintParam("indir", inDir);
    PrintParam("outdir", outDir);
    PrintParam("extension", extension);
    ConfigFileStream << "threads=" << threadsCnt << std::endl;

    ConfigFileStream.close();
}

void Config::SaveConfigLine(std::string& key, std::string& value )
{
    if(key == "logfile")
    {
        if(!value.empty())
            std::swap(logFile, value);
        else
            logFile = "log.log";
    }
    else if (key == "indir")
    {
        std::swap(inDir, value);
    }
    else if (key == "outdir")
    {
        std::swap(outDir, value);
    }
    else if (key == "extension")
    {
        std::swap(extension, value);
    }
    else if (key == "threads")
    {
        threadsCnt = std::stoi(value);
    }
}

void Config::ReadConfigFile()
{
    if(std::filesystem::exists(confFile))
    {
        // file exists lets just read it
        std::ifstream ConfigFileStream;
        ConfigFileStream.open(confFile);
        
        std::stringstream buffer;
        std::string line;
        buffer << ConfigFileStream.rdbuf();

        while( std::getline(buffer, line) )
        {
            
            std::istringstream is_line(line);
            std::string key;
            if( std::getline(is_line, key, '=') )
            {
                std::string value;
                if( is_line >> std::quoted( value) )
                {
                    SaveConfigLine(key, value);
                }
            }
        }
        ConfigFileStream.close();
    }
    else
    {
        // file does not exist, lets create it
        WriteConfigIntoFile();
    }
}

//Parse input parameters
void Config::ParseParams( std::queue<std::string> &params)
{
    if(params.size() == 0)
    {
        std::cout<<help<<std::endl;
    }
    
    std::string commandOption;
    std::string command;
    std::string unknownParams;
    
    auto GetCommandOption = [&params](std::string& opt){
        if(!params.empty())
        {
            std::string str =params.front();
            // yes there is no trim operation in c++17. so lets use it
            //str.erase(str.find_last_not_of(" \t")+1);
            
            std::swap(opt,str);
            //opt = params.front();
            params.pop();
        }
    };
    
    std::lock_guard<std::mutex> lock(mutex);
    
    while(!params.empty())
    {
        command = params.front();
        params.pop();
                   
        if(command == "--help" || command == "-help" || command == "-h")
        {
            std::cout<<help<<std::endl;
        }
        else if(command == "--version" || command == "-v")
        {
            std::cout<<"Program version is 1.0"<<std::endl;
        }
        else if(command == "-in")
        {
            GetCommandOption(inDir );
            restartFlag = restartDirFlag = true;
        }
        else if(command == "-out")
        {
            GetCommandOption(outDir );
            restartFlag = restartDirFlag = true;
        }
        else if(command == "-log")
        {
            std::string temp;
            GetCommandOption(temp );
            if(temp.empty())
                temp = "log.log";
            
            std::swap(temp, logFile);
            restartLogFlag = true;
        }
        else if(command == "-ext")
        {
            GetCommandOption(extension);
            restartFlag = true;
        }
        else if(command == "-showlog")
        {
            showLogRequested = true;
        }
        else if(command == "-filter")
        {
            showLogRequested = true;
            GetCommandOption(filter);
        }
        else if(command == "-F")
        {
            flushLog = true;
        }
        else if(command == "-exit" || command == "-stop")
        {
            exitFlag.store(true); // the command for exit, just store the flag, we need to stop all the threads
            return;
        }
        else
        {
            unknownParams += " " + command + "; ";
        }
    }
    
    if(!unknownParams.empty())
    {
        std::cout << "Unknown input parameters:" << unknownParams << std::endl;
    }
}
