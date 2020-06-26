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

