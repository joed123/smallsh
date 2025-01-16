#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h> 
#include <sys/stat.h> 
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
volatile sig_atomic_t allowBackground = 1;
#define MAX_LENGTH 2048
#define MAX_ARGS 512
pid_t process_hold = 0;
char process_string [20];


void handle_SIGTSTP(int signo) {
    if (allowBackground) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n:";
        write(STDOUT_FILENO, message, 50);
        allowBackground = 0;
    } else {
        char* message = "Exiting foreground-only mode\n:";
        write(STDOUT_FILENO, message, 30);
        allowBackground = 1;
    }
}



void change_directory(char *path) {
    char cwd[PATH_MAX];

    if (path == NULL || path[0] == '\0') {
        path = getenv("HOME");
        if (path == NULL || path[0] == '\0') {
            printf("Home directory not successfully set.\n");
            return;
        }
    }
    if (chdir(path) != 0) {
        perror("chdir");
    } else {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd"); 
        }
    }
}
int check_if_typical(char **arguments, int arg_count){
    int loop1;
    for(loop1 = 0; loop1 < arg_count; loop1++){
        if((strcmp(arguments[loop1],"cd") == 0) || (strcmp(arguments[loop1],"exit") == 0) || (strcmp(arguments[loop1],"status") == 0)){
            return 0;
        }
        else{
            return 1;
        }
    }
}

char** clear_args(char **arguments, int *arg_count) {
    int i, j;
    for (i = 0; i < *arg_count; i++) {
        if (strcmp(arguments[i], "$") == 0 || strcmp(arguments[i], "$$") == 0) {
            for (j = i; j < *arg_count - 1; j++) {
                arguments[j] = arguments[j + 1];
            }
           i--; 
            (*arg_count)--; 
       }
   }

    for (i = 0; i < *arg_count; i++) {
        if (strcmp(arguments[i], "<") == 0 || strcmp(arguments[i], ">") == 0) {
            if (i + 1 < *arg_count) {
                for (j = i; j < *arg_count - 2; j++) {
                    arguments[j] = arguments[j + 2];
                }
                (*arg_count) -= 2;
                i--; 
            } else {
                (*arg_count)--; 
                break; 
            }
        }
    }

    arguments[*arg_count] = NULL; 
    return arguments;
}

int execute_test_command(char **arguments, int arg_count) {
    if (arg_count < 3 || strcmp(arguments[1], "-f") != 0) {
        fprintf(stderr, "Usage: test -f filename\n");
        return 1; 
    }

    char *filename = arguments[2];
    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        return S_ISREG(statbuf.st_mode) ? 0 : 1;
    } else {
        return 1;
    }
}


void execute_nonbuiltin(char *command, char *arguments[], char *input_file, char *output_file, int background, int arg_count) {
    int fd_in, fd_out;

    if(strcmp(command, "cat") == 0){
        strcpy(input_file, output_file);
        output_file[0] = '\0';
    }
    pid_t pid = fork();
    if(strcmp(command,"mkdir") == 0){
        process_hold = pid;
    }
      for (int loop1 = 0; loop1 < arg_count; loop1++) {
        char* position;
        while ((position = strstr(arguments[loop1], "$$")) != NULL) {

            pid_t pid = getpid();
            char buffer[1024];  
            int length_before = position - arguments[loop1];
            strncpy(buffer, arguments[loop1], length_before);
            buffer[length_before] = '\0'; 

            sprintf(buffer + length_before, "%d%s", (int)pid, position + 2);
            arguments[loop1] = strdup(buffer);

        }
    }
    if(strcmp(command,"mkdir") == 0){
       strcpy(process_string, arguments[1]);
    }
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        if (input_file[0] != '\0') {  
            fd_in = open(input_file[0] != '\0' ? input_file : "/dev/null", O_RDONLY);
            if (fd_in < 0) {
                perror("Failed to open input file");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Output redirection
        if (output_file[0] != '\0') {
            fd_out = open(output_file[0] != '\0' ? output_file : "/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("Failed to open output file");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if (execvp(command, arguments) == -1) {
            perror("execvp");
            exit(1);
        }
        pid_t pid = getpid();
        exit(0);
    } else { 
        if (!background) {
            int status;
            waitpid(pid, &status, 0); 
        } else {
            printf("Background process with PID: %d\n", pid);
        }
    }
}

void print_status(int status) {
    if (WIFEXITED(status)) {
        printf("exit value %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)) {
        printf("terminated by signal %d\n", WTERMSIG(status));
    }
}



void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {} // Discard characters until a newline or EOF
}

int main()
{
    int foreground = 0; //if foreground is true make it 1
    int background;
    int arg_count;
    int i = 0;
    char commandlinefull[MAX_LENGTH];
    char* arguments[MAX_ARGS];
    char* arguments_after[MAX_ARGS];
    char input_file[100];
    char output_file[100];
    int fakecount = 0;
    int loop1 = 0;
    int file_in1;
    int file_out1;
    signal(SIGINT, SIG_IGN);

    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    

    while(i == 0){
    background = 0;
    arg_count = 0;
    file_in1 = 0;
    file_out1 = 0;
    foreground = 0;
    printf(": ");
    fflush(stdout);
    fgets(commandlinefull, sizeof(commandlinefull), stdin); 

    char* token = strtok(commandlinefull, " \n"); 
    while(token != NULL) {

            arguments[arg_count++] = token; 
        token = strtok(NULL, " \n");
    }
    if (arg_count == 0 || strcmp(arguments[0], "#") == 0 || arguments[0][0] == '#') { 
            continue;
        }
    if (strcmp(arguments[0], "exit") == 0) {
        i = 1;
        return 0;
        }
        else{
    if (arg_count > 0 && strcmp(arguments[arg_count - 1], "&") == 0) {
                background = 1; 
                arguments[arg_count - 1] = NULL; 
                arg_count--;
    }
        for (fakecount = 0; fakecount < arg_count - 1; fakecount++){
    if (strcmp(arguments[fakecount], "<") == 0){
        strcpy(input_file, arguments[fakecount + 1]);
        printf("input file is: %s\n", input_file);
        file_in1 = 1;
        fakecount++; 
    } else if (strcmp(arguments[fakecount], ">") == 0) {
        strcpy(output_file, arguments[fakecount + 1]);
        printf("Output file is: %s\n", output_file);
        file_out1 = 1;
        fakecount++; 
    }
}
    if(file_in1 != 1){
        input_file[0] = '\0';
    }
    if(file_out1 != 1){
        output_file[0] = '\0';
    }

    for(loop1 = 0; loop1 < arg_count; loop1++){
        if(((strcmp(arguments[loop1],"$$") == 0) || (strcmp(arguments[loop1],"$")) == 0)){
        pid_t pid = getpid();
        printf("The process ID is %d\n", (int)pid);
        }
    }
    if(check_if_typical(arguments, arg_count) == 0){
    if (strcmp(arguments[0], "exit") == 0) {
        i = 1;
        return 0;
        }

    for(loop1 = 0; loop1 < arg_count; loop1++){
        if(strcmp(arguments[loop1],"cd") == 0){
            if (arg_count == 1){
                char tes_string[1];
                tes_string[0] = '\0';
                change_directory(tes_string);
                break;
            } else{
            change_directory(arguments[loop1 + 1]);
            break;
            }
        }
    }

    for(loop1 = 0; loop1 < arg_count; loop1++){
        if(strcmp(arguments[loop1],"status") == 0){
            print_status(foreground);
            break;
        }
    }
    }
    else{
    clear_args(arguments, &arg_count);
    if(arguments[0] != '\0'){ 
         if(strcmp(arguments[0], "test") == 0){
             int result = execute_test_command(arguments, arg_count);
             printf("result of test is:%d ", result);
         }
    execute_nonbuiltin(arguments[0], arguments, input_file, output_file, background, arg_count);
    } 
    else{
        printf("no arguments sorry");
    }
    }
        }
    arguments[arg_count] = NULL;
    }
  return 0;
}
