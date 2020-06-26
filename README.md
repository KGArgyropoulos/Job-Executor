# JOB-EXECUTOR
Job Executor creates Worker processes and together they answer to user queries. Specifically,
- Creates processes, using fork/exec syscalls
- Makes them communicate, using named-pipes
- Makes them co-operate using signals
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

- jobExecutor.c
    * bla bla
- connections.c
    * bla bla
    * bla

