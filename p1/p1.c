
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>


//Linked List struct and functionality
struct bg_tasks {
	pid_t pid;
	char cmd[1024];
	struct bg_task *next;
};

typedef struct bg_tasks *node;

node createNode(void);
node addNode(node head, pid_t pid, char *cmd);
node deleteNode(node head, pid_t pid);
node findPid(node head, pid_t pid);

//Link List node modification functions

//Link List node creation
//See Reference #2
node createNode(void) {
	node temp;
	temp = (node)malloc(sizeof(struct bg_tasks));
	temp->next = NULL;
	return temp;
}

//Add node to the list with given root node
//See Reference #2
node addNode(node root, pid_t pid, char *cmd) {
	node curr;
	node temp;
	curr = createNode();
	curr->pid = pid;
	strcpy(curr->cmd, cmd);
	//If list is empty new node becomes the root
	if(root == NULL) {
		root = curr;
	}
	else {
		temp = root;
		//Cycle through list
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = curr;
	}
	return root;
}

//Remove node with given pid from list with given root 
node deleteNode(node root, pid_t pid) {
	node curr;
	node prev;
	curr = root;
	//If pid value is the head of the list
	if(curr->pid == pid) {
		//If list only has 1 item
		if(curr->next == NULL) {
			free(root);
			return NULL;
		}
		//Else more than 1 item
		else {
			root = curr->next;
			free(curr);
			return root;
		}
	}
	//If pid value is not the head of the list
	while(curr->next != NULL) {
		prev = curr;
		curr = curr->next;
		if(curr->pid == pid) {
			//Double check to make sure nothing funny is going on
			if(curr != NULL) {
				prev->next = curr->next;
				free(curr);
				return root;
			}
		}
	}
	return root;
}

//Find node with given pid from list with given root
node findPid(node root, pid_t pid) {
	node curr;
	curr = root;
	while(curr != NULL) {
		if(curr->pid == pid) {
			return curr;
		}
		curr = curr->next;
	}
	return NULL;

}

//BG Task Globals
int totalBg;
node root;

//Prototypes
void cmdLoop(void);
char **readLine(void);
char **cmdTokenize(char *str);
int readCmds(char **args);
void changedirs(char *str);
void executeCmds(char **options);
void checkBG(void);
int executeBG(char **options);

//Main function initializes and sends to the cmd prompt loop
int main(void) {
	totalBg = 0;
	cmdLoop();
}

//Main loop for the cmdline
void cmdLoop(void) {
	
	char **args;
	char cwd[1024];
	char host[1024];
	char *user;
	int status;
	int counter = 0;
	gethostname(host, 1024);
	user = getlogin();
	if(host == NULL | user == NULL) {
		printf("Failed to get user and host");
		exit(1);
	}
	int check = 1;
	do {
		if(getcwd(cwd,sizeof(cwd))) {
			printf("SSI:  %s@%s:  %s > ", user, host,  cwd);
		}
		else {
			perror("Failed to get cwd");
		}

		//read line and tokenize
		args = readLine();
		//Parse cmds and return 0 if quit cmd was given
		check = readCmds(args);
		//Check if any process has terminated in bg
		if(totalBg > 0) {
			checkBG();		
		}
		//cleanup
		free(args);
	} while(check);
}

//Checks bg tasks to see if any have terminated
void checkBG(void) {
	int counter = 0;
	int status;
	pid_t result;
	node temp = root;	
	while(totalBg > counter) {
		result = waitpid(0, &status, WNOHANG);
		//All children still running
		if(result == 0) {
			;
		}
		else if(result < 0) {
			printf("Error in checkBG");
			exit(1);
		}
		//Child process terminated
		else {
			//Find process with terminated pid
			temp = findPid(root, result);
			printf("%d: %s has terminated\n", result, temp->cmd);	
			//Remove process from list
			root = deleteNode(root, result);
			totalBg -= 1;
		}
		counter++;
	}
}

//Bglist command implementation
void printBG(void) {
	int counter = 0;
	node temp = root;
	while(totalBg > counter) {
		//Add printing path and args of process
		printf("%d: %s\n", temp->pid, temp->cmd);
		temp = temp->next;
		counter++;
	}
	printf("Total Background Tasks: %d\n", totalBg);
}

//Reads the user input to shell
char **readLine(void) {
	char **result;
	char *input;
	int buffersize = 1024;
	int counter = 0;
	int check = 1;
	int current;
	//readline
	//See reference #1
	input = malloc(buffersize * sizeof(char));
	if(!input) {
		printf("Error allocating line");
		exit(1);
	}
	//Get input line
	while(check) {
		current = getchar();
		//'\n' symbolizes end of cmd so add null terminator
		if(current == 10) {
			current  = '\0';
			check = 0;
		}
		input[counter] = current;
		counter++;	
		//check if buffer is full
		//See reference #1
		if(counter >= buffersize) {
			buffersize += 1024;
			input = realloc(input, buffersize);
			if(!input) {
				printf("Error realloc line");
				exit(1);
			}
		}
	}	
	//send line to tokenizer
	result = cmdTokenize(input);
	return result;	
}

//Tokenize input to split into commands
char **cmdTokenize(char *str) {
	int buffersize = 1024;
	int counter = 0;
	char *current;
	//See reference #1
	char **cmds = malloc(buffersize * sizeof(char*));
	if(!cmds) {
		printf("Error alloc in Tokenize\n");
		exit(1);
	}
	//Tokenize input
	current = strtok(str, " ");
	while(current) {
		cmds[counter] = current;
		counter++;
		//check buffer is not full
		//See reference #1
		if(counter >= buffersize) {
			buffersize += 1024;
			cmds = realloc(cmds, buffersize * sizeof(char*));
		}
		//error handle
		if(!cmds) {
			printf("Error realloc in Tokenize\n");
			exit(1);
		}
		current = strtok(NULL, " ");
	}	
	cmds[counter] = NULL;
	return cmds;
}

//Parses cmds
int readCmds(char **args) {
	pid_t pid;
	int bg;
	char *checker = "~";
	//is the command empty? cd? quit? bg? bglist?
	//4 is used to prevent incorrect commands starting with cd
	if(args[0]  == '\0') {
		return 1;
	}
	if(!strncmp(args[0], "cd", 4)) {
		//If nothing after cd - change cmd to ~
		if(!args[1]) {
			changedirs(checker);
		}
		else {
			changedirs(args[1]);
		}
		return 1;	
	}
	if(!strncmp(args[0], "quit", 4)) {
		return 0;
	}
	if(!strncmp(args[0], "bg", 4)) {
		bg = executeBG(args);
		return bg;
	}
	if(!strncmp(args[0], "bglist", 8)) {
		printBG();
		return 1;
	}
	//else program command so
	//fork() and send to execute
	pid = fork();
	if(pid < 0) {
		printf("Error Forking\n");
		exit(1);
	}
	else if (pid == 0) {
		executeCmds(args);
	}
	else {
		waitpid(pid, NULL, 0);
		return 1;
	}
}

//cd implementation
void changedirs(char *str) {
	//If ~ go to home directory
	if(!strncmp(str, "~", 1)) {
		chdir(getenv("HOME"));
	}
	chdir(str);	
}

//Execute shell cmds
void executeCmds(char **options) {
	execvp(options[0], options);
	printf("Error in execution\n");	
}

//bg execution implementation
int executeBG(char **options) {
	char **temp = options+1;
	//Strings for creating path of the process
	char *temp3;
	char buf[1024];
	char result[1024] = "";
	int counter = 1;
	//Create new process and attempt exec
	pid_t pid = fork();
	int status;
	if(pid < 0) {
		printf("Error in forking\n");
	}
	else if(pid == 0) {
		execvp(temp[0], temp);
		printf("Error in execution\n");
		exit(1);
	}
	else {
		//Remove . if cmd is run on cwd
		//and then get cwd and add it to result
		//What about long relative path cmds? ex: Desktop/inf 
		if(temp[0][0] == '.') {
			if(getcwd(buf, sizeof(buf)) == NULL) {
				printf("Error getting cwd\n");
			}
			else {
				temp3 = temp[0]+1; 
				strcat(result, buf);
			}
		}
		else {
			temp3 = temp[0];
		}
		//Concatenate cmd and arguments onto the path string
		strcat(result, temp3);
		while(temp[counter] != NULL) {
			strcat(result, " ");
			strcat(result, temp[counter]);
			counter++;
		}
		root = addNode(root, pid, result);	
		totalBg += 1;
		return pid;
	}
}

