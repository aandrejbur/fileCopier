#ifndef appsynchro_hpp
#define appsynchro_hpp

#include <stdio.h>
#include <map>
#include "etc.hpp"
#include "filework.hpp"
#include "config.hpp"

class AppSynchro{

public:
    // lets shorten some text with typedef
    typedef std::pair<std::string,std::string> FilesPair;
    typedef std::vector<FilesPair> FilesPairsVec;
    typedef std::map<std::time_t, FilesPairsVec> OnTimeDeleteMap;
    typedef std::queue<FileWork> FilesWorksQueue;
    typedef std::queue<std::string> StringsQueue;
    
    // general mutaxes for adding and editing of queues, and directories update
    std::mutex log_mutex, work_mutex, delete_mutex, dir_mutex;
    
    
    // will serve as semaphores, since we do need to set threads on normal wait, not on spin lock
    std::condition_variable log_cv, work_cv, delete_cv, dir_cv;
        
    // mutex and cv for sleeping threads, in this way we will be able to wakeup sleeping threads to finish execution
    std::condition_variable g_sleep_cv;
    std::mutex g_sleep_mutex;
    
    OnTimeDeleteMap onTimeDeleteMap;        // a map of files vectors that needs to be deleted on time
    FilesWorksQueue worksQueue;             // a queue of files work (copy or delete)
    StringsQueue    logQueue;               // a queue of messages that needs to be printed in log file
    
    Config config;
    
     AppSynchro(){}
    ~AppSynchro(){}
    
    void StoreFileWork(FileWork& workItem);
    void GetFileWork(FileWork& workItem);
    void StoreLogMessage(std::string msg);
    void GetLogMessage(std::string& msg);
    void StoreOnTimeDelete(FileWork& workItem);
    void GetOnTimeDeleteVector(FilesPairsVec& vec);
    void Reset();
    void WaitDirrectories();
    void StopAllThreads();
    void NotifyDirSet();
    void SetDirSet(bool val);
    
private:
    // this class may not be copied or moved, since it is a synchronisation class
    AppSynchro(const AppSynchro&) = delete;
    AppSynchro(AppSynchro&&) = delete;
    AppSynchro operator=(const AppSynchro&) = delete;
    AppSynchro& operator=(AppSynchro&&) = delete;
};


#endif /* appsynchro_hpp */
