# JOB-EXECUTOR
Job Executor creates Worker processes and together they answer to user queries. Specifically,
- Creates processes, using fork/exec syscalls
- Makes them communicate, using named-pipes
- Makes them co-operate using signals
- Runs bash scripts to calculate log files' statistics

