#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p){
	
	// input redirection
	if (p->in_file != NULL){
		int in_fd = open(p->in_file, O_RDONLY);
		if (in_fd < 0){
			perror("Failed to open input file");
			return;
		}
		dup2(in_fd, STDIN_FILENO);	// file descriptor no. of stdin redirect to in_fd
		close(in_fd);
	}

	// output redirection
	if (p->out_file != NULL){
		// O_TRUNC : ensure the file write from start
		// owner authorization, authorization of owner's group, others : 6->WR, 4->R 
		int out_fd = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (out_fd < 0){
			perror("Failed to open output file");
			return;
		}
		dup2(out_fd, STDOUT_FILENO);	// file descriptor no. of stdout redirect to out_fd
		close(out_fd);
	}

}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR){
		perror("signal");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	switch (pid){
		case -1:	// fork failed
			perror("fork");
			exit(EXIT_FAILURE);
		
		case 0:	// child process
		
			redirection(p);	// Redirection

			execvp(p->args[0], p->args);
			perror("execvp failed");
			exit(EXIT_FAILURE);
		
		default:	// parent process
			int status;
			waitpid(pid, &status, 0);	// parent block until child finished
			exit(EXIT_SUCCESS);
	}

  	return 1;
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node(struct cmd *cmd)
{
	return 1;
}
// ===============================================================


void shell()
{
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 | out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file)  dup2(in, 0);
				if (temp->out_file){
					dup2(out, 1);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
			}
		}
		// There are multiple commands ( | )
		else{
			
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
