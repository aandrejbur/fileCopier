I will continue updating this source code in my free tyme. 

My current todo list:
1) Allow multiple source (hot) directories to be watch (something like schema inside config file)
2) Change the process into demon way (on all platforms) allowing it to be runned in background for all the time
3) Change infrastructure a bit with implementation of patterns (now i think about factory, singleton and strategy) 
4) Extend config file to support flushlog, filename action masks (i.e. "delete_" "delete_YYYYMMDDHHMMSS" and create more available actions)
5) Create something like general log which will have messages for app status (begin, end) i.e.:
        "DATE_TIME Task started with COMMAND_ARGUMETS, configured as: CONFIGURATION."
        "DATE_TIME TASK finished ACTIONS_COUNTERS, elapsed time"
6) Refactor message formating
7) CHange threads creation from lambda to normal way with usage of shared pointers. 
8) And i will think for something more

A simple way to copy (back up) files from one folder into another

The programm is written in C++ with only standard methods in use, so it is crossplatform as much as C++17 (i would use C++20 for semaphores but this way it is even beter)

Programm tested on Mac OS, Linux (ubuntu) and windows 10. Although i only supply makefile for mac OS and linux

To build the programm simply use make command from the root folder of the package. Binary application will be stored in build/apps/ folder

for initial use of program nothing is needed, it will create both config and log file at the first start. Commands can be supplied with execution call (as comman dline input arguments) or while programm execution (terminal is not blcked, for user to pass commands)

Programm logic: 

This is a test program that copying last version of files from input (hot) directopry to output (archive) directory. 
File copying is implemented in multy threaded way:
    - 1 thread to parse directory and search for file, creates fileWork item and places it into appropriate queue (common file work or "on time" deletion queue)
    - configurable number of threads that get fileWork entry from queue and do copy or delete work
    - 1 thread for "on time" deletion of files, gets the vector of pairs for file from input directory and possible file from output directory according to time when they must be deleted and current time
    - 1 thread for logging - gets messages from log queue and put them into log file (with OS provided buffering), buffering can be disabled with -F command
    - Main thread which start all other threads and then waiting for user input (in simple way)
Synchronisation implemented with combination of mutexes, conditional variables and atomic variables

 Program architecture:

classes: 
    AppSynchro - class that accumulates synchronisation across all threads, qeeps shared data (queues, flags, config and synchronisation primitives) and provide access to queue elements.
    Config - class with configuration data.
    FileWork - a clss which represents single item for for shared queues, consists of two path (source and target file path), work type (copy, delete, delete on time or skip) and time to delete file

everything else plased into single methods:
etc.h - methods used accross all other class

in main.cpp there are also thread methods. 

Since AppSyncro class is shared accross the threads it is made uncopiyble and unmovable, because of it i used lambdas to create threads.  
All threads are stored in vector (to simplify code)

About deletion:
if file name prefixed with "delete_" prefix another check applied if there is an iso date time part next in ISO (YYYYMMDDhhmmss) format - i chose that format for simplicity, since it is irelevant for the task.
On time deletion implemented with map of vectors. where time to delete file is stored as key and files to delete at the same time are stored in vectors as pairs (in and out path)
On time deletion threads checks every half of second if there are files to delete, to simplify the process i used std::map since it has automatic sorting, and so we only need to compare first element of map with current time.

Log filtering: 
there are 2 commands for log representation:

-showlog - will read entire log file, line by line (yes i know that reading line by lyne is slow but since it is a log file it can grow wery big and as for reading by parts it is possible and not complicated but i'm leaving it for future)

-filter EXPRESSION - Reads entire log file line by line outputing only lines suited for filtering expression. filtering is possible with two search mechanisms:
    1) simple search using std::string::find() - this is left for simplicity
    2) regex search isung std::regex_search - this will cover all other search requirements

App commands: 
-in DIR_PATH -  to set or reset input directory
-out DIR_PATH - to set or reset backup directory
-log FILE_PATH - to set exact or relative location of log file, by default it uses 'log.log' near aplication binary
-ext EXTENSION -  extension which will be added for files in output directory
-exit or -stop as a sign to stop the application in proper way
-showlog - to display entire log file
-filter EXPRESSION  - show filtered log file based on EXPRESSION shows lines with exact matchof any symbol or as regex expression
-h or -help or --help - to display programm usage
-F to set log flush option (i.e. every message will be printed into log without caching)
--version or -v  - to print version of program, just a good way for support what is common
  
 stay idle waiting for other commands,
 
