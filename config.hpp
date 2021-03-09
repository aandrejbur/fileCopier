#ifndef config_hpp
#define config_hpp

#include <stdio.h>
#include <mutex>
#include <atomic>
#include <queue>
#include <string>


// applicatin config class, main configuration to use accross everything
class Config {
public:
    std::mutex mutex;                       // config mutex, shared across all threads
    std::atomic_bool exitFlag{0};           // this will work as a sign to all threads to stop their execution
    std::atomic_bool restartFlag{0};        // this will work as a sign to restart execution: clear files queue
    std::atomic_bool restartLogFlag{0};     // this will work as a sign that we will need to change log file
    std::atomic_bool restartDirFlag{0};     // this will work as a sign that we will need to reset directory search
    
    short threadsCnt{0};        // number of threads involved in file copying
    bool  showLogRequested{0};  // flag that wee need to show log this time
    std::atomic_bool  flushLog{0};  // flag that every single message is written into log in time

    std::string logFile{"log.log"};        // path for the log file
    std::string inDir;          // input (hot) directory path
    std::string outDir;         // output (backuo) directory
    std::string extension;      // output files extension
    std::string filter;         // filtered expressssion
    
    Config(){}
    ~Config(){}
    
    void WriteConfigIntoFile();
    void SaveConfigLine(std::string& key, std::string& value);
    void ReadConfigFile();
    void ParseParams( std::queue<std::string> &params);
private:
    // config is not copyable or movable. it is the only one for the app
    Config(const Config& other) = delete;
    Config(Config&& other) = delete;
    Config operator=(const Config& other) = delete;
    Config& operator=(const Config&& other) = delete;
    
};

#endif /* config_hpp */
