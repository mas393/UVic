/*
 *  CSC360 Assignment 1
 *  Matthew Stephenson
 *  The purpose of this assignment is to create a shell interpreter that 
 *  is a simplified version of the Linux Bash Shell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define ARG_MAX 2048 //defines the maximum string length accepted by ssi prompt
//Other defines used, but specified in limits.h include:
// HOST_NAME_MAX
// PATH_MAX

typedef struct bg_process {
    struct bg_process *next;
    pid_t pid;
    char *path; 
    char **args;
} bg_process;

typedef struct bg_process_list {
    bg_process *head;
} bg_process_list;

bg_process_list* init_bg_process_list();
bg_process* init_bg_process(const pid_t pid, const char *path, char **args);
void insert_bg_process(bg_process_list *bg_list, const pid_t pid, const char *path, char **args);
void remove_bg_process(bg_process_list *bg_list, bg_process *rem);
void del_bg_process(bg_process *rem);
void print_bg_process(const bg_process *process);
void check_bg_process_list(bg_process_list *bg_list);
void print_bg_list(const bg_process_list *bg_list);
void print_path(const char *user, const char *host, const char *path);
void get_input(char *input, int inputsize);
char** parse_input(char *args_string);
void kill_process(bg_process_list *bg_list, char **args);
void change_directory(char *path, char **args);
int execute(char **args, char *path, bg_process_list *bg_list);

int main(int argc, char** argv) {
    //getting shell details
    char *username= getlogin();
    char *hostname = (char*)malloc(HOST_NAME_MAX);
    if (gethostname(hostname, HOST_NAME_MAX) == -1) {
	fprintf(stderr, "ssi: main: error getting hostname\n");
	exit(1);
    }
    char *pathname = (char*)malloc(PATH_MAX);
    getcwd(pathname, PATH_MAX);
    if (pathname == NULL) {
	fprintf(stderr, "ssi: main: error getting path\n");
	exit(1);
    }

    print_path(username, hostname, pathname);

    bg_process_list *bg_list = init_bg_process_list();    

    for(;;) {
	char *user_args = (char*)malloc(ARG_MAX);
	get_input(user_args, ARG_MAX);

	char **args = parse_input(user_args);

	//checking if any background child processes have terminated
	if (bg_list -> head != NULL) check_bg_process_list(bg_list);
	
	if (execute(args, pathname, bg_list)) break;
	print_path(username, hostname, pathname);

	for (int i = 0; args[i] != NULL; ++i) free(args[i]);
	free(args);
	free(user_args);    
    }
    
    free(hostname);
    free(pathname);
    return 0;
}


/*
 * init_bg_process_list
 *
 * Initializes a bg_process_list struct.
 */
bg_process_list* init_bg_process_list() {
    bg_process_list *temp = malloc(sizeof(bg_process_list));
    temp -> head = NULL;
    return temp;
}

/*
 * init_bg_process
 *
 * Initializes a bg_process struct with the pid, path, and args values provided as input.
 */
bg_process* init_bg_process(const pid_t pid, const char *path, char **args) {
    bg_process *temp = (bg_process *)malloc(sizeof(bg_process));
    temp -> next = NULL;
    temp -> pid = pid;
    temp -> path = (char*)malloc(sizeof(char *) * strlen(path));
    temp -> args = (char**)malloc(sizeof(char **) * ARG_MAX);    
    memcpy(temp -> path, path, strlen(path));
    
    int i;
    for (i = 0; args[i] != NULL; ++i) {
	temp -> args[i] = (char*)malloc(strlen(args[i]));
	strcpy(temp -> args[i], args[i]);
    }
    temp -> args[i] = NULL;
    temp -> args = realloc(temp -> args, sizeof(char*) * i + 1);

    return temp;
}

/*
 * insert_bg_process
 *
 * Initializes a bg_process with the provided pid, path, and args values by calling init_bg_process
 * and inserts the bg_process struct at the end of the bg_list linked list provided as input.
 */
void insert_bg_process(bg_process_list *bg_list, const pid_t pid, const char *path, char **args) {
    bg_process *new = init_bg_process(pid, path, args);
    if (bg_list -> head == NULL) bg_list -> head = new;
    else {
	bg_process *temp = bg_list -> head;
	while(temp -> next != NULL) temp = temp -> next;
	temp -> next = new;
    }
}

/*
 * remove_bg_proces
 * 
 * Updates the prev and next pointers in bg_list to reflect the removal of 
 * the bg_process rem.
 */
void remove_bg_process(bg_process_list *bg_list, bg_process *rem) {
    if (bg_list -> head == rem) bg_list -> head = rem -> next;
    else {
	bg_process *temp = bg_list -> head;
	while(temp -> next != rem) temp = temp -> next;
	temp -> next = rem -> next;
    }
}

/*
 * del_bg_process
 *
 * Frees memory used by the input bg_process rem.
 */
void del_bg_process(bg_process *rem) {
    free(rem -> path);
    for (int i = 0; rem -> args[i] != NULL; ++i) free(rem -> args[i]);
    free(rem -> args);
    free(rem);
}

/*
 * print_bg_process
 *
 * Prints a bg_process in the format specified in assignment description.
 */
void print_bg_process(const bg_process *process) {
    printf("%d: ", process -> pid);
    printf("%s/", process -> path);
    for (int i = 0; process -> args[i] != NULL; ++i) printf("%s ", process -> args[i]);
}

/*
 * print_path
 *
 * Prints the ssi prompt in the format specified in assignment description.
 */
void print_path(const char *user, const char *host, const char *path) {
    printf("%s@%s: %s > ", user, host, path);
}

/*
 * get_input
 *
 * Retrieves user input from ssi prompt.
 */
void get_input(char *input, const int inputsize) {
    fgets(input, inputsize, stdin);
    input[strlen(input) - 1] = '\0';
}

/*
 * parse_input
 *
 * convert an arg string into an array of args that is formed using the space delimeter.
 */
char** parse_input(char *args_string) {
    char **args = (char**)malloc(sizeof(char*) * ARG_MAX);

    char *arg = strtok(args_string, " ");
    int i;
    for(i=0; arg != NULL; ++i) {
	args[i] = (char*)malloc(strlen(arg));
	strcpy(args[i], arg);
	
	arg = strtok(NULL, " ");	
    }
    args[i] = NULL;
    args = realloc(args, sizeof(char*) * i + 1);
    
    return args;
}

/*
 * change_directory
 *
 * Calls the chdir() function with the path specified in **args.
 * Special path args accepted are:
 * cd .. , which changes the directory to the directory one level higher than the current direcotyr
 * cd ~ or cd (no arguments), which changes the directory to the home directory of the user, as specified by getenv("HOME")
 * cd path , which changes the directory to that specified by path.
 * Path can be relative to the current directory, or absolute.
 */
void change_directory(char *path, char **args) {
    char *new_path;    
    if (args[2] != NULL){
	printf("ssi: cd: error, too many arguments\n");
	return; 
    }
    else if (args[1] == NULL) new_path = "~"; //copying behaviour of typical shell, where the commands "cd ~" and "cd" are equivalent
    else new_path = args[1];
    
    if (!strcmp(new_path, "..")) {
	for (int i = strlen(path) - 2; i >= 0; --i) {
	    if (i == 0) {
		path[1] = '\0';
		break;
	    }
	    else if (path[i] == '/') {
		path[i] = '\0';
		break;
	    }
	}
    }	
    else if (!strcmp(new_path, "~")) {
	strcpy(path, getenv("HOME"));
    }
    else {
        if (path[strlen(path)-1] == '/') path[strlen(path) - 1] = '\0';
	strcpy(path, new_path);
    }
    
    if (chdir(path) == -1) fprintf(stderr, "ssi: cd: %s: error changing into specified directory\n", path);
    
    getcwd(path, PATH_MAX); //modify path input variable to reflect new working directory
}

/*
 * check_bg_process_list
 *
 * Traverses the bg_list linked list, checking if any background processes have terminated.
 * If a terminated process is found, the user is notified and bg_list is updated.
 */
void check_bg_process_list(bg_process_list *bg_list){
    int delete_flag = 0;
    bg_process *delete_temp;

    pid_t pid = waitpid(0, NULL, WNOHANG);
    while((pid != 0) && (bg_list -> head != NULL)){
	if (pid == -1) {
	    printf("error with waitpid, error: %s\n", strerror(errno));
	    break;
	}
	else {
	    bg_process *temp = bg_list -> head;
	    while (temp -> pid != pid) {
		temp = temp -> next;
		if (temp == NULL) break;
	    }

	    if (temp != NULL) {
		print_bg_process(temp); 
		printf("has terminated.\n");
	    
		remove_bg_process(bg_list, temp);
		del_bg_process(temp);
	    }
	}
	pid = waitpid(0, NULL, WNOHANG);			 
    }
}

/*
 * print_bg_list
 *
 * Prints all background process in bg_list in the format specified in the assignment description.
 */
void print_bg_list(const bg_process_list *bg_list){
    int i = 0;
    for (bg_process *temp = bg_list -> head; temp != NULL; temp = temp -> next){
	print_bg_process(temp); printf("\n");
	i++;
    }
    printf("Total Background jobs: %d\n", i);	
}

/*
 * kill_process
 *
 * Attempts to terminate child processes whos pid are specified in input args.
 * Like the typical kill shell command, nothing is output to the user on success.
 */
void kill_process(bg_process_list *bg_list, char **args) {
    for (int i = 1; args[i] != NULL; ++i) {	
	pid_t pid = atoi(args[i]);
    
	bg_process *temp = bg_list -> head;
	while ((temp != NULL) && (temp -> pid != pid)) {
	    temp = temp -> next;
	}
    
	if (temp != NULL) {
	    if (kill(pid, SIGINT) == -1) printf("ssi: kill_process: error killing pid %d\n", pid);
	    remove_bg_process(bg_list, temp);
	    del_bg_process(temp);
	}
    }    
}

/*
 * execute
 *
 * Handles the ssi command specified in the input string.
 * Prior to executing the command, execute loops over background processes, checking
 * if any have terminated and notifying the user if any processes have and updating
 * the bg_list input parameter.
 * Special commands include exit, which terminates the program,
 * cd, which calls the change_directory funtion to modify the input variable path
 * bglist, which lists the ongoing background processes, and
 * bg cmd, which will execute the command specified by cmd in the background.
 * Otherwise, execute will attempt to exec the command specified in the input.
 */
int execute(char **args, char *path, bg_process_list *bg_list) {
    if (args[0] == NULL) return 0;
    
    if (!strcmp(args[0], "exit")) return 1;      
    else if (!strcmp(args[0], "cd")) {
	//change_directory modifies the path variable in place after changing directory
	change_directory(path, args);	
    }    
    else if (!strcmp(args[0], "bglist")) {
	print_bg_list(bg_list);
    }
    else if (!strcmp(args[0], "kill")) {
	kill_process(bg_list, args);
    }
    else {
	int bg = 0;
	pid_t pid = fork();
	
	if (!strcmp(args[0], "bg")) {	    
	    bg = 1;
	    ++args; //args[0] is now args[1] to avoid the leading "bg"
	    //	    input_cpy += 3;
	}
	
	if (pid < 0) {
	    fprintf(stderr, "ssi: execute: error fork failed\n");
	    exit(1);
	}	    
	else if (pid == 0) { //child
	    if(execvp(args[0], args) == -1) fprintf(stderr, "ssi: execute: error execvp failed\n");
	    exit(1);
	}
	else { //parent
	    if (bg) insert_bg_process(bg_list, pid, path, args);
	    else wait(NULL); //if not bg wait for child process termination
	}	
    }

    return 0;
}

