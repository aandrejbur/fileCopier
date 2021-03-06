A simple cross platform way to copy (back up) files from one folder into another

The program is written in C++ with only standard methods in use, so it is cross platform as much as C++17 (i would use C++20 for semaphores but this way it is even better)

Programm tested on Mac OS, Linux (ubuntu) and windows 10. Although i only supply makefile for mac OS and linux

To build the program simply use make command from the root folder of the package. Binary application will be stored in build/apps/ folder

for initial use of program nothing is needed, it will create both config and log file at the first start. Commands can be supplied with execution call (as command line input arguments) or while program execution (terminal is not blocked, for user to pass commands)

Programm logic: 

This is a test program that copying last version of files from input (hot) directory to output (archive) directory. 
File copying is implemented in multi threaded way:
    - 1 thread to parse directory and search for file, creates fileWork item and places it into appropriate queue (common file work or "on time" deletion queue)
    - configurable number of threads that get fileWork entry from queue and do copy or delete work
    - 1 thread for "on time" deletion of files, gets the vector of pairs for file from input directory and possible file from output directory according to time when they must be deleted and current time
    - 1 thread for logging - gets messages from log queue and put them into log file (with OS provided buffering), buffering can be disabled with -F command
    - Main thread which start all other threads and then waiting for user input (in simple way)
Synchronisation implemented with combination of mutexes, conditional variables and atomic variables

 Program architecture:

classes: 
    AppSynchro - class that accumulates synchronisation across all threads, stores shared data (queues, flags, config and synchronisation primitives) and provide access to queue elements.
    Config - class with configuration data.
    FileWork - a class which represents single item for for shared queues, consists of two path (source and target file path), work type (copy, delete, delete on time or skip) and time to delete file

everything else placed into single methods:
etc.h - methods used across all other class

in main.cpp there are also thread methods. 

Since AppSyncro class is shared across the threads it is made uncopyable and unmovable, because of it i used lambdas to create threads.  
All threads are stored in vector (to simplify code)

About deletion:
if file name prefixed with "delete_" prefix another check applied if there is an iso date time part next in ISO (YYYYMMDDhhmmss) format - i chose that format for simplicity, since it is irrelevant for the task.
On time deletion implemented with map of vectors. where time to delete file is stored as key and files to delete at the same time are stored in vectors as pairs (in and out path)
On time deletion threads checks every half of second if there are files to delete, to simplify the process i used std::map since it has automatic sorting, and so we only need to compare first element of map with current time.

Log filtering: 
there are 2 commands for log representation:

-showlog - will read entire log file, line by line (yes i know that reading line by lyne is slow but since it is a log file it can grow very big and as for reading by parts it is possible and not complicated but i'm leaving it for future)

-filter EXPRESSION - Reads entire log file line by line printing only lines suited for filtering expression. filtering is possible with two search mechanisms:
    1) simple search using std::string::find() - this is left for simplicity
    2) regex search using std::regex_search - this will cover all other search requirements

App commands: 
-in DIR_PATH -  to set or reset input directory
-out DIR_PATH - to set or reset backup directory
-log FILE_PATH - to set exact or relative location of log file, by default it uses 'log.log' near application binary
-ext EXTENSION -  extension which will be added for files in output directory
-exit or -stop as a sign to stop the application in proper way
-showlog - to display entire log file
-filter EXPRESSION  - show filtered log file based on EXPRESSION shows lines with exact matchof any symbol or as regex expression
-h or -help or --help - to display program usage
-F to set log flush option (i.e. every message will be printed into log without caching)
--version or -v  - to print version of program, just a good way for support what is common

I will continue updating this source code in my free time. 

My current todo list:

1) Add way to not only copy files, but also move files from one directory to another (based on config)
2) Add possibility to restore missing files from backup directory (based on config)
3) Add support for symbolic links, and resolving possible cycle scenario
4) Add extension based file skipping
5) Allow multiple source (hot) directories to be watch (something like schema inside config file)
6) Change the process into demon way (on all platforms) allowing it to be runned in background for all the time
7) Change infrastructure a bit with implementation of patterns (now i think about factory, singleton and strategy) 
8) Extend config file to support flushlog, filename action masks (i.e. "delete_" "delete_YYYYMMDDHHMMSS" and create more available actions)
9) Create something like general log which will have messages for app status (begin, end) i.e.:
        "DATE_TIME Task started with COMMAND_ARGUMETS, configured as: CONFIGURATION."
        "DATE_TIME TASK finished ACTIONS_COUNTERS, elapsed time"
6) Refactor message formating
7) Change threads creation from lambda to normal way with usage of shared pointers. 
8) And i will think for something more
9) Create cross platform build file (i think i will create cmake, but for that i will need to fix issues with my Mac os configuration)
10) Add some compression and remote storage of compressed versions - as configurable option
11) add Some indexing way
12) Investigate the way to not copy entire file but just append what missing (git way) - this will help to reduce disk usage

And all of the above wasn't implemented in the first time since I just didn't have enough time. And it wasn't a purpose to build a production grade application in one day) 
