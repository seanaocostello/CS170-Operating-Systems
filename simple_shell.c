
#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>




int parseInput(char * line, char ** arr){
	//char arr[255][32];
	int arg = 0;
	int pos = 0;
	int counter = 0;
	int end = -1;
	int nextLine = 0;
	while (line[counter] != 10){
		if (nextLine){
			arr[arg][pos] = '\0';
			//strlcpy(currentArg,arr[arg],(pos+1)*sizeof(char));
			arg++;
			pos = 0;
			nextLine = 0;
		}
		else if (line[counter] == '&'){
			if (pos){
				nextLine = 1;
			}
			else {
				arr[arg][pos] = line[counter];
				counter++;
				pos++;
				nextLine = 1;
				end = arg + 1;
			}
		}
		else if (arg == end){
			printf("ERROR: Parse error, invalid character \n");
			return 0;
		}
		else if ((line[counter] <= 'z' && line[counter] >= 'a') || (line[counter] <= '9' && line[counter] >= '0')){
			arr[arg][pos] = line[counter];
			pos++;
			counter++;
		}
		else if (line[counter] == '<'){
			if (arg == 0){
				nextLine = 1;
			}
			else if (arg == 1){
				arr[arg][0] = line[counter];
				pos++;
				counter++;
				nextLine = 1;
			}
			else {
				printf("ERROR: Parse error, invalid character \n");
				return 0;
			}
		}
		else if (line[counter] == '>'){
			if (end != -1){
				printf("ERROR: Parse error, invalid character \n");
				return 0;
			}
			if (pos){
				nextLine = 1;
			}
			else {
				arr[arg][0] = line[counter];
				pos++;
				counter++;
				nextLine = 1;
				end = arg + 2;
			}
		}
		else if (line[counter] == '|'){
			if (pos){
				nextLine = 1;
			}
			else {
				arr[arg][0] = line[counter];
				pos++;
				counter++;
				nextLine = 1;
			}
		}
		else if (line[counter] == ' '){
			if (pos){nextLine = 1;}
			counter++;
		}
		else{
			if (arg == end){
				printf("ERROR: Parse error, invalid character \n");
				return 0;
			}
			if (arg > 0){
				if (arr[arg-1][0] != '|'){
					arr[arg][pos] = line[counter];
					pos++;
					counter++;
				}
			}
		}
	}
	arr[arg][pos] = '\0';
	arr[arg+1] = NULL;
	return arg+1;
}


int execCommand(char ** arr, int background, int readfd, int writefd){
	int pid;
	pid = fork();
	if (pid == 0){
		dup2(readfd,0);
		dup2(writefd,1);
		if (execvp(arr[0],arr) == -1){
			printf("ERROR: command not found");
		}
		return -1;
	}
	else if (pid < 0){
		printf("ERROR: fork failed");
		return -1;
	}
	else {
		if (!background){
			wait(NULL);
		}
		return 1;
	}
}

int runArguments(char ** arr, int numArgs){
	int in = dup(0);
	int out = dup(1);
	int numPipes = 0;
	int background = 0;
	int inputRedirect = 0;
	int outputRedirect = 0;
	int input = 0;
	int output = 0;
	//int commandNum = 0;
	int begin = 1;
	int beginCounter = 0;
	int indices [numArgs];
	char * inputFile, * outputFile;
	for (int i = 0; i < numArgs; i++){
		if (strcmp(arr[i],"<") == 0){
			inputRedirect = 1;
			input = 1;
			inputFile = malloc(32*sizeof(char));
			strncpy(inputFile,arr[i+1], 32*sizeof(char));
			arr[i] = NULL;
			arr[i+1] = NULL;
			i++;
			begin = 1;
		}
		else if (strcmp(arr[i],">") == 0){
			outputRedirect = 1;
			output = 1;
			outputFile = malloc(32*sizeof(char));
			strncpy(outputFile,arr[i+1], 32*sizeof(char));
			arr[i] = NULL;
			arr[i+1] = NULL;
			i++;
			begin = 1;
		}
		else if (strcmp(arr[i],"|") == 0){
			numPipes++;
			arr[i] = NULL;
			begin = 1;
		}
		else if (strcmp(arr[i],"&") == 0){
			background = 1;
			arr[i] = NULL;
			begin = 1;
		}
		else {
			if (begin){
				indices[beginCounter] = i;
				beginCounter++;
				begin = 0;
			}
		}
	}
	/*if (!inputRedirect && !outputRedirect && !numPipes){
		execCommand(arr,background);
	}*/
	if (inputRedirect){
		inputRedirect = open(inputFile, O_RDONLY);
		dup2(inputRedirect,0);
	}
	if (outputRedirect){
		outputRedirect = open(outputFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
		dup2(outputRedirect,1);
	}
	if (!outputRedirect){outputRedirect = out;}
	if (!inputRedirect){inputRedirect = in;}
	if (numPipes){
		int pipefd[2];
		for (int i = 0; i < beginCounter; i++){
			if (pipe(pipefd) < 0){
				printf("ERROR: failed to make pipe");
				exit(1);
			}
			int pid = fork();
			if (pid == 0){
				close(1);
				if (i < numPipes){
					dup2(pipefd[1],1);
				}
				else{
					dup2(outputRedirect,1);
					close(pipefd[1]);
				}
				if (execvp(arr[indices[i]],arr+indices[i]) < 0){
					printf("ERROR: command not found");
					exit(1);
				}
			}
			else{
				close(0);

				if (i < numPipes){
					dup2(pipefd[0],0);
				}
				close(pipefd[0]);
				close(pipefd[1]);
			}
		}
		if (!background){
			for (int i = 0; i < beginCounter; i++){
				wait(NULL);
			}
		}
	}
	else {
		int pid = fork();
		if (pid == 0){
			if (execvp(arr[0],arr) == -1){
				printf("ERROR: command not found");
			}
			return -1;
		}
		else if (pid < 0){
			printf("ERROR: fork failed");
			return -1;
		}
		else {
			if (!background){
				wait(&pid);
			}
		}
	}
	dup2(in,0);
	dup2(out,1);
	if (input) {free(inputFile);}
	if (output) {free(outputFile);}
	return 1;
}


int main(int argc, char ** argv){
	char  line [512];
	char ** args;
	int argNum;
	while (1){
		args = malloc(255*sizeof(char*));
		for (int i = 0; i < 255; i++){
			args[i] = malloc(32*sizeof(char));
		}
		if (argc > 1){
			if (strcmp(argv[1],"-n") != 0){
				printf("shell: ");
			}
		}
		else {
			printf("shell: ");
		}
		if (fgets(line,512,stdin)){
			argNum = parseInput(line, args);
		}
		else {
			printf("\n");
			for (int i = 0; i < 255; i++){
				free(args[i]);
			}
			free(args);
			return 1;
		}
		runArguments(args, argNum);
	}
	return 0;
}