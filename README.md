# JOB-EXECUTOR
Job Executor creates Worker processes and together they answer to user queries. Specifically,
- Creates processes, using fork/exec syscalls
- Makes them communicate, using named-pipes
- Makes them co-operate using signals - low level IO
- Runs bash scripts to calculate log files' statistics

# Execution
- compilation: make minisearch
- execution: ./jobExecutor –d docfile –w numWorkers

where 
* docfile (or whatever name you decide to give your file) is a simple text file containing file paths to ASCII text files.\
Each line should contain exaclty one path.
* numWorkers is the number of workers this app will create.

# Implementation _1
At first, jobExecutor reads docfile, starts w Workers(processes) and shares the docfile folders between them. Supposedly, the directories are flat (no sub-directories, just text files). Afterwards, the app waits for the user input commands.

# User Commands
- /search q1 q2 q3 … qN –d deadline\
Where the user searches for the files containing at least one qi.\
jobExecutor sends every Worker the strings and waits for everyones results before answering the user.\
Every Worker searches every file they manage and finds every line containing at least one of the strings.\
For every file they find, the result returning to the jobExecutor is the file's path, the number of the qi(s) containing lines and the content of the specific line.\
If jobExecutor doesn't receive an answer in -deadline- seconds, then prints the user only the responses received before the timeout, with an appropriate message (Only x out of y Workers responded in time).
- /maxcount keyword\
Where the user searches for the file that contains the keyword more times than any other.\
jobExecutor sends the keyword to every Worker and asks them to return the file that contains the keyword most times\
After picking up every Worker's maxcount it prints the file with the max(maxcount).\
In case of equality, the smallest alphabetically full-path is printed.
- /mincount keyword\
Same as above, but this time we are interested in the file that contains the keyword less times (BUT AT LEAST ONCE) than any other file.\
If there's no file containing that word, an appropriate message is printed.
- /wc
Displays to the user the total number of characters(bytes), words and lines from every file the Workers proceed.\
jobExecutor asks every Worker for these statistics and prints them after calculating their sum.
- /exit
jobExecutor notifies the Workers to stop. Before termination, every Worker writes to the folder log/Worker_XXX (XXX: Worker's process id) records in the form below:\
Time of query arrival : Query type : string : pathname1 : pathname2 : pathname3 : … : pathnameN\
where pathname1...pathnameN are the paths of the files the string was found in the data set the worker manages.\
Also, the Worker cleans up any structs he has allocated and notifies the jobExecutor on the total amount of string found in the files.

# Implementation _2

This is the implemented schema:

![alt text](https://github.com/KGArgyropoulos/Job-Executor/blob/master/jobExecutor.png)

- jobExecutor.c: Driver file
    * After mapping the input paths, jobExecutor shares them to the workers, using pigeonhole principle for the split.Folders pass to the wordkers,through proConn -connections.c-,which return the id of the forked process. We'll talk more about this function later.Then, after the essential pipes between jobExecutor and workers have been made, the first one starts to read users commands. These commands and the queries,the users is inserting are stored in an array of string, of size 11 (max, up to 10 words+the command and deadline).Afterwards, execQueries -connections.c- is being called, which among others, finds and displays the results, as we 'll later see. Finally, signal SIGUSR2 in sent to the handler and the flag changes so that the workers can get terminated. FIFOs get unlinked and the structs used get free'd.
- connections.c: This file's functions are mainly responsible for the communication between jobExecutor and workers and the execution of the queries. Important functions implemented here
    * proConn: Defines two signals, SIGUSR1 and SIGUSR2, in the handler function and creates a process. Father process (jobExecutor) creates the named pipes, giving them names through function godFather, following the form: pipeX_Y.txt , where Y is the id of the process and X is the id of the FIFOs managing process (child reads/writes) and get the values WR,RD. At the same time, the child process is stopped, waiting for the joBExecutor to create the FIFOs and send to the worker signal SIGUSR (through handler) so that the child will continue execution. When that happens, jobExecutor starts writing to the appropriate FIFO the paths that the worker will proceed, waits till the worker finishes reading the folders and returns to process id to main function. Worker reads the files and stores them to an array, then counts the amount of files in each folder and creates a map and an inverted index(trie),to store the words of each file. (1 trie per worker). After index creation is done, worker stops and waits for jobExecutor's signal, to execute some query. jobExecutor, through execQueries function, writes the query that worker is in charge of and the worker calls caller function, that calls the appropriate function to execute the query. When the query is done, worker waits for the next one. If jobExecutor ends its execution, sends a SIGUSR2 signal through the handler that will unblock the worker. The worker will free any allocated structs and unlink reading FIFO.
    * execQueries: Uses a for-loop to start the workers, in order to execute a query and displays results. At first, execQueries checks that the process is still alive, or creates a new one by calling proConn as main did in first place. Then sends a signal to restart the appropriate process. Opens writing pipe and writes the user commands. When the other side is opened, execQueries blocks until results are read.At the same time, it prints them. If it's a search query, it checks the time spent and if its after deadline, returns to main the workers that were in time. 
    * digs: Counts the digits of a number.
    * newPathName: returns a string with the full name to every file.
    * str_split: Gets string with "\n" between the words and returns an array of strings.
    * alphaGreater: Returns a flag for the alphabetical greater path in case of equality at max/minCounter.
    * writeLog: Gets called just after writing  the results from the worker to fifo and creates (or appends to) the worker's log files.

- trie.c: Inverted index, already analyzed in Mini-Search-Engine project
    * See (https://github.com/KGArgyropoulos/Mini-Search-Engine/blob/master/README.md)
    * and (https://github.com/KGArgyropoulos/Mini-Search-Engine/blob/master/trie.png)
- searchMode.c: In this file, is the implemented query search.
    * When the word of a query is found, writeSearchFifo function is called,which writes the results to the FIFO, and writeLog, which writes to the log file necessary information.
- counts.c: In this file, maxmin_Count is implemented.
    * maxmin_Count function, finds the words in the trie and calls writeCountFifo, which calculates max and min counts. writeLog function, writes the result of each worker. wordCount recursive function keeps the line-word-byte sum of each worker's files and returns them through writeWcFifo to the jobExecutor. writeWcFifo has a wc struct and keeps the sum of its results.
- Bash Scripts: Statistics calculation, by parsing the log files
    * numOfKeywords.sh : Total number of keywords searched.\
    Form: Total number of keywords searched:[res]
    * MostLeastFF.sh : Keywords most and least frequently found.\
    Form1: Keyword most frequently found: keyword [totalNumFilesFound: XXX]: [word found the most times] (XXX: the number of files keyword was found)\
    Form2: Keyword least frequently found: keyword [totalNumFilesFound: XXX]: [word found the least times] (XXX: the number of files keyword was found)
