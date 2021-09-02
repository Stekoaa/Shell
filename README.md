# Shell
Simple implementation of a Linux shell written in C language. 
I made this project to learn internals of linux shells and improve my skills in C language.
## Usage
1. Clone this directory and `cd` into it.
2. Run the command `make`.
3. Run `./shell`.
4. Run any command in the shell.
5. In order to exit, run `exit`.

## Features
### Commands
1. `cd [DIR]`
    - Changes directory to directory specified
    - Throws an error when directory does not exist
2. `cp [FILE1] [FILE2]`
    - Copies FILE1 to FILE2, creates new file when FILE2 doesn't exist
    - Throws an error when FILE1 doesn't exist
3. `help`
     - Shows description of each command
4. `history` & `clearhis`
    - Lists history of used commands
    - `clearhis` clears history of used commands
5. `! [NUM]` & `!!`
    - Executes command number [NUM] in history file
    - `!!` executes last used command
6.  `myls [PATH]`
    - Implementaion of the ls command
    - If no arguments are specified, it lists files and directories in current directory
7. `mkf [FILE]` & `rmf [FILE]`
    - Creates specified FILE if it doesn't already exist
    - Removes specified FILE, throws an error when FILE doesn't exist
8. `rabbit [FILE]` 
    - Implementation of the cat command
    - Prints file on the standard output
9. `setenv [name] [value]` & `unsetenv [name]`
    - Creates an enviornmental variable `var` if it doesn't already exist and assigns it the value given
    - `unset [name]` destroys that environmental variable
10. `time`
    - Shows current time
### Other
1. `I/O redirections`
    - Handles `<`, `>`, `>>` operators wherever they are in the command
2. `Pipes` 
    -  Handles `|` operator
    -  Multiple piping is allowed

### Note 
This project is still in development. I plan to add new commands and features.
