#include <iostream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <queue>
#include <regex>

#include "etc.hpp"
#include "filework.hpp"
#include "config.hpp"
#include "appsynchro.hpp"

std::atomic_bool waikup{0};

void ShowFIlteredLog(const std::string logFile, const std::string filter, std::mutex& mutex)
{
    std::ifstream logFileStream(logFile);
    std::string str;
    std::regex regex;
    if(logFile.empty())
    {
        // impossible but still possible
        std::cout << "Error: no log file specified!" << std::endl;
        return;
    }
    
    if(!logFileStream.is_open())
    {
        std::cout << "Error: Could not open '"<< logFile <<"'" << std::endl;
        return;
    }
    
    // read log file line by line, yes it is slow, but simple and works just fine
    // other option is to read exact number of data from file.
    while (std::getline(logFileStream, str))
    {
        if(!filter.empty())
        {
            if( str.find(filter)!=std::string::npos){
                // if we just found exact match from of filter message do not invoke regex
                std::cout << str << std::endl;
                continue;
            }
            bool match = false;
            try {
                std::regex regex(filter);
                match = std::regex_search(str,regex);
            }
            catch (const std::regex_error& e) {
                std::cout << "Filtering value error: " << e.what() << ". Input: '" << filter <<"'" <<std::endl;
                
                break;
            }
            if(!match)
                continue;;
        }
        std::cout << str << std::endl;
    }
    
    logFileStream.close();
    
}

void LoggingProcessor(AppSynchro& app)
{
    std::ofstream logStream;
    
    logStream.open(app.config.logFile, std::fstream::out | std::fstream::app);
       
    while (1)
    {
        if(app.config.exitFlag)
        {
            //even when all threads are stopped, we still need to finish logging
            std::lock_guard<std::mutex> lock(app.log_mutex);
            if(app.logQueue.size()==0)
            {
                break;
            }else{
                app.config.flushLog = true; // but atleast set flush log
            }
        }
        
        if(app.config.restartLogFlag)
        {
            std::lock_guard<std::mutex> lock(app.log_mutex);
            logStream.flush();
            logStream.close();
            logStream.open(app.config.logFile, std::fstream::out | std::fstream::app);
            
        }
    
        std::string temp;
        app.GetLogMessage(temp);
        
        if(temp.empty())
            continue;
        else
        {
            std::lock_guard<std::mutex> lock(app.log_mutex);
            logStream << temp;
            if(app.config.flushLog)
                logStream.flush();
        }
    }
    
    logStream.flush();
    logStream.close();
}

void DeleteOnTimeProcessor(AppSynchro& app)
{
    // run forever until we are set to end processing
    std::string logMessage;
    AppSynchro::FilesPairsVec tempVec;

    while (!app.config.exitFlag)
    {
        app.Reset();
        
        app.GetOnTimeDeleteVector(tempVec);
        
        if(app.config.exitFlag)
            return;
        
        for(auto& filesPair: tempVec)
        {
            logMessage = RemoveFiles(filesPair.first,filesPair.second);
            app.StoreLogMessage(logMessage);
        }
        
        // this thread must work always, since it checks current time, and then tries to delete files, but still it can sleep for a half of second
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
};

void ProcessDirectory(AppSynchro& app, std::filesystem::path& src, std::filesystem::path& target, std::string& newExtension)
{
    namespace fs = std::filesystem;
    
    for(auto& p :std::filesystem::directory_iterator(src))
    {
        app.Reset();
        
        if(app.config.exitFlag || app.config.restartDirFlag)
            break;
            
        if(p.is_directory())
        {
            std::filesystem::path newTarget = target;
            newTarget += GetFileSeparator();
            newTarget += p.path().filename();
                    
            if(!fs::exists(newTarget))
            {
                try {
                    fs::create_directory(newTarget);
                }
                catch (fs::filesystem_error& e)
                {
                    std::string msg("Error: ");
                    app.StoreLogMessage(msg + "Unable to create drectory: "+e.what()+"\n");
                    break;
                }
                
            }
            std::filesystem::path newsrc = p.path();
            ProcessDirectory(app, newsrc, newTarget, newExtension);
        }else{
            std::string newTarget = target; newTarget += GetFileSeparator(); newTarget += p.path().filename(); newTarget += newExtension;
                
            FileWork newWork( p.path(), newTarget);
                
            if(newWork.work == WorkType::ON_TIME_DELETE)
            {
                app.StoreOnTimeDelete(newWork);
            }
            else if(newWork.work == WorkType::COPY || newWork.work == WorkType::DELETE)
            {
                app.StoreFileWork(newWork);
            }
        }
    }
}

void DirectoryProcessor(AppSynchro& app)
{
    std::filesystem::path src, target;
    std::string extension;
    bool firstTime = true;
    auto ReloadConf = [&app, &src, &target, &extension, &firstTime]()
    {
        std::lock_guard<std::mutex> lock(app.config.mutex);
        src = app.config.inDir;
        target = app.config.outDir;
        extension = app.config.extension;
        firstTime=true;
    };
    
    auto wait = [&]()
    {   // sleep for a minute and then start again
        std::unique_lock<std::mutex> lock(app.g_sleep_mutex);
        app.g_sleep_cv.wait_for(lock, std::chrono::minutes(1), [](){return waikup==true;});
        waikup = false;
    };
    
    ReloadConf();
    
    while(!app.config.exitFlag)
    {
        // always check if we need to reset
        app.Reset();
        
        // wait for directories if they are empty in config file
        app.WaitDirrectories();
                
        // in, out directories or extension is changed, we need to reload config
        if(app.config.restartDirFlag)
        {
            ReloadConf();
            app.config.restartDirFlag = 0;
        }
        
        if(src.empty() || target.empty())
        {
            wait();
            continue;
        }
    
        if(!std::filesystem::exists(target))
        {// if target dirrectory does not exists lets try to create it, recursevrly
            
            try {
                std::filesystem::create_directories(target);
            }
            catch (std::filesystem::filesystem_error &e)
            {
                if(!firstTime)
                {
                    std::string msg(e.what());
                    msg += " ";
                    msg += target;
                    app.StoreLogMessage(msg);
                }
            }
        }
        
        if(!std::filesystem::exists(src) )
        {
            if(!firstTime)
            {
                std::string msg = src;
                msg+=" - Input directory doesn't exist";
                app.StoreLogMessage("ERROR: " + msg);
                firstTime = false;
            }
        }
        else
        {
            if(std::filesystem::exists(target))
                ProcessDirectory(app,src,target,extension);
            else
                continue;
        }
        
        wait();
    }
}

void FileWorksProcessor(AppSynchro& app)
{
    namespace fs = std::filesystem;
    while (!app.config.exitFlag)
    {
        app.Reset();
        // a loop until we actually will be commanded to stop execution
        FileWork workItem;
        app.GetFileWork(workItem);
        if(workItem.work == WorkType::SKIP)
        {
            continue; 
        }
        else
        {
            std::string logMessage;
            if(workItem.work == WorkType::DELETE)
            {
                logMessage = RemoveFiles(workItem.inFile, workItem.outFile);
            }
            else
            {
                logMessage = CopyFile(workItem.inFile,workItem.outFile);
            }
            
            app.StoreLogMessage(logMessage);
        }
    }
}


// Newver post it into production!, it is only for educational purpuses!!!!
int main(int argc, const char * argv[])
{
    AppSynchro app;
    
    app.config.ReadConfigFile();
    
    std::queue<std::string> inParams;
    MakeParamsQueue(inParams, argc, argv);
    
    app.config.ParseParams(inParams);
    app.config.flushLog = true;
    app.Reset();
        
    std::vector<std::thread> threads;
    threads.push_back(std::thread ([&app](){DirectoryProcessor(app);}));
    threads.push_back(std::thread ([&app](){LoggingProcessor(app);}));
    threads.push_back(std::thread ([&app](){DeleteOnTimeProcessor(app);}));
    
    std::vector<std::thread> fileWorkThread;
    
    if(app.config.threadsCnt == 0)
        app.config.threadsCnt = 1;
    
    for(int i = 0; i <app.config.threadsCnt; i++)
        fileWorkThread.push_back(std::thread ([&app](){FileWorksProcessor(app);}));
    
    app.NotifyDirSet();
    
    while (app.config.exitFlag.load()!=true)
    {
        std::cout << "Input some command ";
        std::string input;
        
        std::cin.ignore( std::cin.rdbuf()->in_avail() );
        std::getline(std::cin,input);
        std::cin.clear();
        
        MakeParamsQueue(inParams,input);
                
        app.config.ParseParams(inParams);
        app.config.WriteConfigIntoFile();
        
        // notyfy dirextory reset
        if(app.config.restartDirFlag)
            app.NotifyDirSet();
        
        // adjust number of file working threads
        if(app.config.showLogRequested)
        {
            ShowFIlteredLog(app.config.logFile, app.config.filter, app.log_mutex);
            app.config.showLogRequested = false;app. config.filter.clear();
        }
        
        // we got some input waikaup sleeping directory processor thread
        waikup=true;
        app.g_sleep_cv.notify_one();
    }
    
    app.config.exitFlag = true;
    waikup=true;
    app.StopAllThreads();
    
    for(auto &p : threads)
    {
        p.join();
    }
       
    return 0;
}
