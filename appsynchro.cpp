#include "appsynchro.hpp"
#include <thread>
//global counters
std::atomic_int g_logcnt{0}, g_workcnt{0}, g_deleteCnt{0};
std::atomic_bool g_dirSet{0};

void AppSynchro::StoreFileWork(FileWork& workItem)
{
    std::unique_lock<std::mutex> lock(work_mutex);
    // lets just sleep for a half of second if there is too much files in work queue
    if(worksQueue.size()>1000)
    {
        
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        lock.lock();
    }
    worksQueue.push(std::move(workItem));
    g_workcnt++;
    work_cv.notify_one();
}

void AppSynchro::GetFileWork(FileWork& workItem)
{
    std::unique_lock<std::mutex> lock(work_mutex);
    work_cv.wait(lock, [](){return g_workcnt>0;});
    
    if(config.exitFlag)
        return;
    
    if(worksQueue.size() == 0)
        return;
    
    workItem = std::move(worksQueue.front());
    g_workcnt--;
    worksQueue.pop();
}

void AppSynchro::StoreLogMessage(std::string msg)
{
    if(msg.empty())
        return;
    
    std::unique_lock<std::mutex> lock(log_mutex);
    logQueue.push(msg);
    g_logcnt++;
    log_cv.notify_one();
}

void AppSynchro::GetLogMessage(std::string& msg)
{
    std::unique_lock<std::mutex> lock(log_mutex);
    log_cv.wait(lock, [](){return g_logcnt>0;});
    
    if(config.exitFlag)
        return;
    
    if(logQueue.size() == 0)
        return;
    msg = std::move(logQueue.front());
    g_logcnt--;
    logQueue.pop();
}

void AppSynchro::StoreOnTimeDelete(FileWork& workItem)
{
    std::unique_lock<std::mutex> lock(delete_mutex);
    onTimeDeleteMap[workItem.deletionTime].push_back(std::make_pair(workItem.inFile, workItem.outFile));
    g_deleteCnt++;
    delete_cv.notify_one();
}

void AppSynchro::GetOnTimeDeleteVector(FilesPairsVec& vec)
{
    vec.clear();
    
    std::unique_lock<std::mutex> lock(delete_mutex);
    delete_cv.wait(lock,[](){return g_deleteCnt>0;});
    
    if(config.exitFlag)
        return;
    
    std::time_t now = std::time(nullptr);
        
    OnTimeDeleteMap::iterator itr = onTimeDeleteMap.begin();
                    
    if( itr!= onTimeDeleteMap.end() && itr->first <= now)
    {
        g_deleteCnt -= (int)itr->second.size();
        std::swap(vec,itr->second);
        onTimeDeleteMap.erase(itr);
    }
    return;
}

void AppSynchro::Reset()
{
    bool reset = true;
    config.restartFlag.compare_exchange_strong(reset, false);

    if(reset)
    {
        config.restartFlag = 0;
        {
            std::lock_guard lock(work_mutex);
            FilesWorksQueue temp;
            std::swap(temp, worksQueue); 
            g_workcnt = 0;
        }
        
        {
            std::lock_guard lock(delete_mutex);
            OnTimeDeleteMap temp;
            std::swap(temp, onTimeDeleteMap);
            g_deleteCnt = 0;
        }
        config.restartDirFlag = true;
        config.restartLogFlag = true;
    }
}

void AppSynchro::WaitDirrectories()
{
    if(!g_dirSet)
    {
        std::unique_lock<std::mutex> lock(dir_mutex);
        dir_cv.wait(lock,[](){return g_dirSet == true;});
        
        if(config.exitFlag)
            return;
        
        config.restartDirFlag = true;
    }
}


void AppSynchro::StopAllThreads()
{
    // instead of complex synchronisation, lets just set exit flag and notify all waiting threads
    config.exitFlag.store(true);
    
    g_logcnt++;
    g_workcnt++;
    g_deleteCnt++;
    g_dirSet=true;
    
    dir_cv.notify_one();
    work_cv.notify_all();
    delete_cv.notify_one();
    log_cv.notify_one();
    
    g_sleep_cv.notify_all();
}

void AppSynchro::NotifyDirSet()
{
    g_dirSet = true;
    dir_cv.notify_one();
}

void AppSynchro::SetDirSet(bool val)
{
    g_dirSet = val;
}
