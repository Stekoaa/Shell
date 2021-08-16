#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BUFFER_SIZE 1024
#define TOKEN_SIZE 64
#define TOKEN_DELIM " \t\n\r\a"
#define ENTER 10
#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_BLUE  "\033[1;96m"
#define FONT_RESET "\033[0m\n"
#define FONT_RESET_W "\033[0m"

char HISTORY_PATH[PATH_MAX];
char *LAST_COMMAND;


int shell_cd(char **args);
int shell_cp(char **args);
int shell_rabbit(char **args);
int shell_exit(char **args);
int shell_help(char **args);
int shell_history(char **args);
int shell_history_clear(char **args);
int shell_history_execute(char **args);
int shell_history_execute_last(char **args);
int shell_ls(char **args);
int shell_mkf(char **args);
int shell_rmf(char **args);
int shell_time(char **args);


int shell_execute(char **args);
int shell_start(char *line);


char *builtin[] ={
        "cd",
        "cp",
        "exit",
        "help",
        "history",
        "clearhis",
        "!",
        "!!",
        "myls",
        "mkf",
        "rabbit",
        "rmf",
        "time"
};

char *builtin_description[] = {
        " - changes directory to the directory specified, throws an error if the directory does not exist",
        " [source] [destination] = copies contents of source file to destination file",
        " - exits the shell with return status as success",
        " - here we are",
        " - shows history of commands",
        " - clears history of commands",
        " [number] - executes ",
        " - executes last command",
        " [optionally path] - recreation of ls command",
        " [name] - makes new file",
        " [name] - prints file on standard output",
        " [name] - removes given file",
        " - shows current time",
};

int (*builtin_func[]) (char **) = {
        &shell_cd,
        &shell_cp,
        &shell_exit,
        &shell_help,
        &shell_history,
        &shell_history_clear,
        &shell_history_execute,
        &shell_history_execute_last,
        &shell_ls,
        &shell_mkf,
        &shell_rabbit,
        &shell_rmf,
        &shell_time
};

int number_of_builtin(){
    return sizeof(builtin) / sizeof(char*);
}

void redirect_out(char *file_name){
    int out = open(file_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);

    if (out == -1){
        perror("File opening error!");
        return;
    }

    if(dup2(out, STDOUT_FILENO) == -1){
        perror("dup2 error!");
    }

    if(close(out) == -1){
        perror("File closing error");
    }
}

void redirect_in(char *file_name){
    int in = open(file_name, O_RDONLY);

    if (in == -1){
        perror("File opening error!");
        return;
    }

    if(dup2(in, STDIN_FILENO) == -1){
        perror("dup2 error!");
    }

    if(close(in) == -1){
        perror("File closing error!");
    }
}

void redirect_out_append(char *file_name){
    int out = open(file_name, O_CREAT | O_APPEND | O_WRONLY, 0666);

    if (out == -1){
        perror("File opening error!");
        return;
    }

    if(dup2(out, STDOUT_FILENO) == -1){
        perror("dup2 error!");
    }

    if(close(out) == -1){
        perror("File closing error!");
    }
}

void create_pipe(char **args){
    int fd[2];

    if (pipe(fd) == -1){
        perror("Pipe creating error!");
        return;
    }

    if(dup2(fd[1], 1) == -1){
        perror("dup2 error!");
        return;
    }

    if(close(fd[1]) == -1){
        perror("Output side of pipe closing error!");
        return;
    }

    shell_execute(args);

    if(dup2(fd[0], 0) == -1){
        perror("dup2 error!");
        return;
    }

    if(close(fd[0]) == -1){
        perror("Input side of pipe closing error!");
    }
}


int shell_cd(char **args){
    if(args[1] == NULL){
        fprintf(stdout,"Expected argument\n");
        return 1;
    }

    if(chdir(args[1]) == -1){
        perror("Changing directory error!");
    }

    return 1;
}

int shell_cp(char **args){
    if(args[1] == NULL){
        fprintf(stdout,"Expected source file\n");
        return 1;
    }

    if(args[2] == NULL){
        fprintf(stdout,"Expected target file\n");
        return 1;
    }

    FILE *source = fopen(args[1], "r");

    if(source == NULL){
        perror("Opening source file error!");
        return 1;
    }

    FILE *target = fopen(args[2], "w");

    if(target == NULL){
        perror("Opening target file error!");
        fclose(source);
        return 1;
    }

    int a;

    while((a = fgetc(source)) != EOF){
        fputc(a, target);
    }

    fclose(source);
    fclose(target);

    return 1;
}


int shell_exit(char **args){
    return 0;
}


int shell_help(char **args){
    fprintf(stdout,"Jakub Steczkiewicz's shell\n");
    fprintf(stdout,"Type program names and arguments, and hit enter\n");
    fprintf(stdout,"Buitin commands:\n ");

    int i;

    for (i = 0; i < number_of_builtin(); i++){
        fprintf(stdout,"%s%s\n", builtin[i], builtin_description[i]);
    }

    fprintf(stdout,"Use man command for information on other programs\n");
    return 1;
}


int shell_history(char **args){
    FILE* f_ptr = fopen(HISTORY_PATH, "r");
    char current_line[PATH_MAX];

    if(f_ptr == NULL){
        perror("Opening history file error!");
        return 1;
    }

    int counter = 1;

    while(fgets(current_line, sizeof(current_line), f_ptr) != NULL){
        fprintf(stdout,"%d.%s", counter, current_line);
        counter++;
    }

    fclose(f_ptr);
    return 1;
}


int shell_history_clear(char **args){
    FILE *f_ptr = fopen(HISTORY_PATH, "w");

    if(f_ptr == NULL){
        perror("Opening history file error!");
        return 1;
    }

    fclose(f_ptr);
    return 1;
}

int shell_history_execute(char **args){

    if(args[1] == NULL){
        fprintf(stdout,"Expected command number");
        return 1;
    }

    int command_number = atoi(args[1]);

    FILE *f_ptr = fopen(HISTORY_PATH, "r");

    if(f_ptr == NULL){
        perror("Opening history file error!");
        return 1;
    }

    int counter = 1;
    int status = 1;
    char *line = malloc(BUFFER_SIZE * sizeof(char));


    while(fgets(line, BUFFER_SIZE, f_ptr) != NULL && counter != command_number){
        counter++;
    }

    if (command_number != counter){
        fprintf(stdout, "There's no command with given number");
    }

    else{
        line[strlen(line) - 1] = '\0';
        status = shell_start(line);
    }


    free(line);
    fclose(f_ptr);
    return status;
}

int shell_history_execute_last(char **args){

    if(LAST_COMMAND == NULL || strcmp(LAST_COMMAND, "!!") == 0){
        return 1;
    }

    int status  = shell_start(LAST_COMMAND);
    return status;
}



int shell_ls(char **args){

    DIR *my_dir;
    struct dirent *my_file;
    struct stat my_stat;

    char dir_path[PATH_MAX + 1];

    if(args[1] == NULL || strcmp(args[1], ">") == 0 || strrchr(args[1],'.') != NULL || strcmp(args[1], "|" ) == 0){
        args[1] = getcwd(dir_path, sizeof(dir_path));
    }

    if(args[1] == NULL){
        perror("getcwd() error!");
        return 1;
    }

    my_dir = opendir(args[1]);

    if(my_dir == NULL){
        perror("Opening directory error!");
        return 1;
    }

    char buf[BUFFER_SIZE];

    while((my_file = readdir(my_dir)) != NULL) {
        sprintf(buf, "%s/%s", args[1], my_file->d_name);
        if (stat(buf, &my_stat) == -1){
            perror("stat() error!");
            return 1;
        }

        if (S_ISREG(my_stat.st_mode)) {
            fprintf(stdout,"%s\nRegular file\n", my_file->d_name);
        }

        if (S_ISDIR(my_stat.st_mode)) {
            fprintf(stdout,"%s%s%sDirectory\n", COLOR_BLUE, my_file->d_name, FONT_RESET);
        }

        fprintf(stdout,"Owner permissions: ");

        if (my_stat.st_mode & S_IRUSR) {
            fprintf(stdout,"read ");
        }

        if (my_stat.st_mode & S_IWUSR) {
            fprintf(stdout,"write ");
        }

        if (my_stat.st_mode & S_IXUSR) {
            fprintf(stdout,"execute");
        }

        fprintf(stdout,"\n");

        fprintf(stdout,"Group permissions: ");
        if (my_stat.st_mode & S_IRGRP) {
            fprintf(stdout,"read ");
        }

        if (my_stat.st_mode & S_IWGRP) {
            fprintf(stdout,"write ");
        }

        if (my_stat.st_mode & S_IXGRP) {
            fprintf(stdout,"execute");
        }

        fprintf(stdout,"\n");

        fprintf(stdout, "Others permissions: ");
        if (my_stat.st_mode & S_IROTH) {
            fprintf(stdout,"read ");
        }

        if (my_stat.st_mode & S_IWOTH) {
            fprintf(stdout,"write ");
        }

        if (my_stat.st_mode & S_IXOTH) {
            fprintf(stdout,"execute");
        }
        fprintf(stdout,"\n\n");
    }

    if(closedir(my_dir) == -1){
        perror("Closing directory stream error!");
    }

    return 1;
}


int shell_mkf(char **args){

    if(args[1] == NULL){
        fprintf(stdout,"Expected argument\n");
        return 1;
    }

    FILE  *fptr = NULL;
    fptr = fopen(args[1], "a");

    if(fptr == NULL){
        perror("File creating error");
    }

    else{
        puts("File successfully created");
        fclose(fptr);
    }

    return 1;
}

int shell_rabbit(char **args){

    if(args[1] == NULL){
        fprintf(stdout, "Expected file name");
        return 1;
    }

    FILE  *fptr = fopen(args[1], "r");

    if(fptr == NULL){
        perror("File opening error!");
        return 1;
    }

    int a;

    while((a = fgetc(fptr)) != EOF){
        fputc(a, stdout);
    }

    fclose(fptr);

    return 1;
}


int shell_rmf(char **args){

    if(args[1] == NULL){
        fprintf(stdout,"Expected argument\n");
        return 1;
    }

    if(remove(args[1]) != 0){
        perror("Deleting file error!");
    }

    else{
        puts("File successfully deleted");
    }

    return 1;
}

int shell_time(char **args){
    time_t curr_time = time(NULL);

    if(curr_time != (time_t)(-1)) {
        fprintf(stdout, "Current time: %s", ctime(&curr_time));
    }

    return 1;
}


void print_prompt(char *user_name){
    char cwd[PATH_MAX];

    fprintf(stdout,"%s@%s%s:", COLOR_GREEN, user_name, FONT_RESET_W);
    if (getcwd(cwd, sizeof(cwd)) != NULL){
        fprintf(stdout, "%s%s%s> ", COLOR_BLUE, cwd, FONT_RESET_W);
    }

    else{
        perror("getcwd() error!");
        exit(EXIT_FAILURE);
    }
}


char *read_line(){
    int buf_size = BUFFER_SIZE;
    char *buf = (char*)malloc(sizeof(char) * buf_size);
    int position = 0;
    char c;

    if(!buf) {
        perror("Allocation error!");
        exit(EXIT_FAILURE);
    }

    while(1){
        c = getchar();

        if (c == EOF || c == '\n'){
            buf[position] = '\0';
            return buf;
        }

        else{
            buf[position] = c;
        }

        position++;

        if(position >= buf_size){
            buf_size += buf_size;
            buf = (char*)realloc(buf, sizeof(char) * buf_size);

            if(buf == NULL){
                perror("Reallocation error!");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char *add_spaces(char *line){

    int i;
    int j = 0;

    char *tokenized = (char*)malloc((TOKEN_SIZE * 2) * sizeof(char));

    if(tokenized == NULL){
        perror("Allocation error!");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < strlen(line); i++) {
        if (line[i] != '>' && line[i] != '<' && line[i] != '|') {
            tokenized[j++] = line[i];
        } else {
            tokenized[j++] = ' ';
            tokenized[j++] = line[i];
            tokenized[j++] = ' ';
        }
    }
    tokenized[j] = '\0';

    return tokenized;
}

int shell_launch(char **args){

    pid_t pid_number;
    int status;

    pid_number = fork();

    if(pid_number == 0){
        if(execvp(args[0], args) == -1){
            perror("Exec error");
            exit(EXIT_FAILURE);
        }
    }

    else if(pid_number < 0){
        perror("Fork error!");
    }

    else{
        do {
            waitpid(pid_number, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    redirect_in("/dev/tty");
    redirect_out("/dev/tty");

    return 1;
}

int shell_execute(char **args){

    if(args[0] == NULL){
        return 1;
    }

    int num_builtin = number_of_builtin();

    for(int i = 0; i < num_builtin; i++){
        if(strcmp(args[0], builtin[i]) == 0){
            return(*builtin_func[i])(args);
        }
    }

    return shell_launch(args);
}


int shell_start(char* line){

    char *line_spaces;
    char *tokens[BUFFER_SIZE];
    int status = 1;

    line_spaces = add_spaces(line);

    char *token = strtok(line_spaces, " ");
    int position = 0;

    while(token){
        if(strcmp(token, "<") == 0 ) {
            tokens[position] = strtok(NULL, TOKEN_DELIM);
            redirect_in(tokens[position++]);
        }

        else if (strcmp(token, ">") == 0 ){
            tokens[position] = strtok(NULL, TOKEN_DELIM);
            redirect_out(tokens[position++]);
        }

        else if(strcmp(token, ">>") == 0){
            tokens[position] = strtok(NULL, TOKEN_DELIM);
            redirect_out_append(tokens[position++]);
        }

        else if(strcmp(token, "|") == 0){
            tokens[position] = NULL;
            create_pipe(tokens);
            position = 0;
        }

        else{
            tokens[position] = token;
            position++;
        }
        token = strtok(NULL, " ");
    }

    tokens[position] = NULL;

    status = shell_execute(tokens);

    redirect_in("/dev/tty");
    redirect_out("/dev/tty");

    free(line_spaces);
    return status;
}


void shell_loop(){
    int status;
    char *line ;
    char *username = getenv("USER");

    getcwd(HISTORY_PATH, sizeof(HISTORY_PATH));
    strcat(HISTORY_PATH, "/history.txt");

    FILE* f_ptr = fopen(HISTORY_PATH, "a");

    if(f_ptr == NULL){
        perror("Failed to open history file");
    }

    do{
        print_prompt(username);
        line = read_line();

        fputs(line, f_ptr);
        fputc(ENTER, f_ptr);
        fflush(f_ptr);

        status = shell_start(line);
        strcpy(LAST_COMMAND, line);
        free(line);
    }while(status);

    fclose(f_ptr);
}

int main() {
    LAST_COMMAND = (char*)malloc(sizeof(char) * BUFFER_SIZE);

    if(LAST_COMMAND == NULL){
        perror("Allocation error!");
        exit(EXIT_FAILURE);
    }

    shell_loop();
    free(LAST_COMMAND);
    return EXIT_SUCCESS;
}
