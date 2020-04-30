### Simple Shell Interpreter

p1.c is a simple shell interpreter designed to perform basic cmdline tasks using basic system calls

#### List of Commands

* Any cmdline program ex. sleep, cat, ls, etc.
* cd : for changing directories
* bg : perform tasks in the background
  * usage given by : bg command arguments
  * ex. bg sleep 5
* bglist : display list of background activities with their PID
  * bglist will report back when a background process has finished execution
