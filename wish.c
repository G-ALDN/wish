#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>



#define BIG_NUMBER 100000

//In Built Commands
#define QUIT_KEYWORD "exit"
#define CHANGE_DIR "cd"
#define PATH "path"




/*
Adan Ali
ICS 462 Operating Systems
Programming Assignment 2 - WISH
07/15/2024
*/

void wish();


char *batchFile = NULL;
int main(int argc, char *argv[])
{
	setenv("PATH", "/bin", 1);
	//Argc Check
	if(argc > 2){
		char error_message[30] = "An error has occurred\n";
    		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	if(argc == 2){
		batchFile = argv[1];
	}
	wish();
	return 0;
}

//Runs Wish Command Line Interface
void wish(){
	FILE *fp = NULL;
	if(batchFile){
		fp = fopen(batchFile, "r");
	}
	
	//Keep track of how many times the loop runs
	int spinCount = 0;
	while(true){
		//allocate space for input line
		char *line = malloc(BIG_NUMBER * sizeof(char)); //freed
		size_t lineSize = (BIG_NUMBER * sizeof(char));
		
		//Batch Read
		if(fp){
			line = fgets(line, (int) lineSize, fp);
			if(!line){
				if(spinCount == 0){
					char error_message[30] = "An error has occurred\n";
		  		write(STDERR_FILENO, error_message, strlen(error_message));
					exit(1);
				}
				else{
					exit(0);
				}
			}
		}
		//Interactive Read
		else{
			printf("wish> ");
			getline(&line, &lineSize, stdin);
		}
		
		//Checks if line has anything other than whitespace characters
		int n;
		bool hasChar = false; 
		for(n = 0; n < sizeof(line); n++){
			if(line[n] == '\0'){
				break;
			}
			
			if(line[n] == ' ' || line[n] == '\n' || line[n] == '\t'){
				continue;
			}
			hasChar = true;
		}
		
		//Loops again if no characters are found
		if(!hasChar){
			continue;
		}
		
		
		//Loops through line and finds all parallel commands that will be run
		char *newline = line; 
		char *token_a; 
		char *phrases[BIG_NUMBER] = {NULL}; 

		int phrasecount = 0;
		while(true){
			token_a = strsep(&newline, "&");
			if(token_a[0] == '\0'){
				continue;
			}
			if(!newline && token_a){
				phrases[phrasecount] = token_a;
				phrasecount++;
				break;
			}
			else if(token_a && newline){
				phrases[phrasecount] = token_a;
				phrasecount++;			
			}
			else{
				break;
			}
		}
		
		//Loops through every command found above
		int i;
		for(i = 0; i < phrasecount; i++){
			bool redirection = false;
			char *phrase = phrases[i]; 
			char *token; 
			char *keywords[BIG_NUMBER] = {NULL}; 
			if(strstr(phrase, ">")){
				redirection = true;
			}
			
			//Parses whitespace out of commands breaks them into a command and its arguments
			//Also checks for redirects using redcount
			int count = 0;
			int redcount = 0;
			while(true){
				token = strsep(&phrase, " \t\n");
				if(strstr(token, ">")){
					redcount++;
					continue;
				}
				if(!phrase && token){
					if(strlen(token) == 0){
						break;
					}
					keywords[count] = token;
					count++;
					break;
				}
				else if(token && phrase){
					if(token[0] == '\0'){
						continue;
					}
					keywords[count] = token;
					count++;			
				}
				else{
					break;
				}
			}
			
			//Number of Redirect Symbols Check
			if(redcount > 1){
				char error_message[30] = "An error has occurred\n";
    		write(STDERR_FILENO, error_message, strlen(error_message));
    		break;
			}
			
			char *command = keywords[0];
			char *filename = NULL;
			if(redirection){
				filename = keywords[count-1];
				keywords[count-1] = NULL;
				count--;
			}

			
			//Quit if exit is typed and matches length
			if(strstr(command, QUIT_KEYWORD) && strlen(command) == 4 &&  count == 1){
				exit(0);
			}
			
			//CHANGE DIRECTORY COMMAND
			if(strstr(command, CHANGE_DIR) && strlen(command) == 2){
				if(count == 2){
					int success = chdir(keywords[1]);
					if(success < 0){
						char error_message[30] = "An error has occurred\n";
    				write(STDERR_FILENO, error_message, strlen(error_message));
					}
				}
				else{
					char error_message[30] = "An error has occurred\n";
  				write(STDERR_FILENO, error_message, strlen(error_message));
				}
				
			}
			//PATH COMMAND
			else if(strstr(command, PATH) && strlen(command) == 4){
				//Sets path to empty if there are no arguments otherwise overrides current path with new one
				if(count == 1){
					int success = setenv("PATH", "", 1);
					if(success <  0){
						char error_message[30] = "An error has occurred\n";
						write(STDERR_FILENO, error_message, strlen(error_message));
					}
				}
				else{
					char result[BIG_NUMBER] = {0};
					int p;
					for(p = 1; p < count; p++){
						strcat(result, keywords[p]);
						if(p < count -1){
							strcat(result, ":");
						}
					}
					int success = setenv("PATH", result, 1);
					if(success <  0){
						char error_message[30] = "An error has occurred\n";
						write(STDERR_FILENO, error_message, strlen(error_message));
					}
				}
			}
			//ALL OTHER COMMANDS
			//Checks path to see if valid command then it forks and exec the command
			else{
				char result[BIG_NUMBER] = {0};
				bool is_script = false;
				if(strstr(command, ".sh")){
					is_script = true;
				}
				if(!is_script){
					snprintf(result, sizeof(result), "/bin/%s", command);
				}
				if(access(result, X_OK) == 0 || is_script){
					pid_t pid = fork();
					if(pid == -1){
						char error_message[30] = "An error has occurred\n";
						write(STDERR_FILENO, error_message, strlen(error_message));
					}
					else if(pid > 0){
						//parent process
						int status;
						waitpid(pid, &status, 0);
						if(WIFEXITED(status) && WEXITSTATUS(status) > 0){
							int c = fgetc(stderr);
							if(c == '\0'){
								char error_message[30] = "An error has occurred\n";
								write(STDERR_FILENO, error_message, strlen(error_message));
							}
							ungetc(c, stderr);
						}
					}
					else{
						//child process
						//If filename isn't null, redirect stdout to file
						if(filename){
							int file = open(filename, O_WRONLY | O_CREAT, 0777);
							if(file == -1){
								exit(1);
							}
							dup2(file, STDOUT_FILENO);
							close(file);
						}
						
						if(is_script){ 
							int rmfile = open("/dev/null", O_WRONLY, 0777);
							int sherr_cpy = dup(STDERR_FILENO);
							dup2(rmfile, STDERR_FILENO);
							close(rmfile);
							char *script_args[] = {"/bin/sh", "-c", command, NULL};
							execvp("/bin/sh", script_args);
							dup2(sherr_cpy, STDERR_FILENO);
							close(sherr_cpy);
							exit(1);
						}
						execvp(command, keywords);
						exit(1);
					}
				}
				else{
					char error_message[30] = "An error has occurred\n";
  				write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}
		}
		free(line);
		spinCount++;
	}
	exit(0);
}

