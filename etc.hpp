#ifndef etc_hpp
#define etc_hpp

#include <string>
#include <queue>
#include <filesystem>
#include <sstream>

#define TSFormatSize 14 // size of "YYYYMMDDhhmmss" format

bool is_number(const std::string &s); // check if string only consists of integer

std::string GetFileSeparator(); // get address separator based on current OS

// Methods to parce both input parameters and input command line
void MakeParamsQueue( std::queue<std::string> &params, const std::string& command);
void MakeParamsQueue( std::queue<std::string> &params, const int& argc, const char * argv[]);

typedef std::deque<std::string> LogQueue;

std::string getTimeStr();

std::string RemoveFiles(std::string file1, std::string file2);
std::string RemoveFile(std::string file);

std::string CopyFile(std::string in, std::string out);

#endif /* etc_hpp */
