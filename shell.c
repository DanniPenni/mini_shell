#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

void sigint_handler(int sig) {}

char ** get_tokenized_str(int *num_tokens) {
	  char **tokenized;
	  char *input_buffer = NULL;
	  size_t buffer_size; 
	  size_t chars_read = getline(&input_buffer, &buffer_size, stdin);
	  if (chars_read == -1 || input_buffer[0] == '\n') {
		  *num_tokens = -1;
		  return NULL; 
	  } 

	  input_buffer[chars_read - 1] = '\0';

	  tokenized = malloc(sizeof(char *));
	  (*num_tokens) = 1;
	  char *token = strtok(input_buffer, " ");

	  while (token != NULL) {
		  tokenized[(*num_tokens) - 1] = token;
		  (*num_tokens) += 1;
		  tokenized = realloc(tokenized, sizeof(char *) * (*num_tokens));
		  token = strtok(NULL, " ");
	  }

	  tokenized[(*num_tokens) - 1] = NULL; // Ensuring arg-array is NULL terminated.
	  *(num_tokens) -= 1;

	  return tokenized;
}

void cd_handler(char * pathname) {
	char * path;
	if (pathname == NULL) {
		path = getenv("HOME");
	} else {
		path = pathname;
	}

	if (chdir(path) == -1) {
		if (errno == ENOENT) {
			fprintf(stderr, "cd: %s: No such file or directory\n", path);
		} else if (errno == EACCES) {
			fprintf(stderr, "cd: %s: Permission denied\n", path);
		} else {
			fprintf(stderr, "chdir() failed on errno: %d\n", errno);
		}
	}

}

char * check_redirect(int num_tokens, char ** tokenized) {
	char * filename = NULL;
	for (int i = 0; i < num_tokens; ++i) {
		if (strcmp(tokenized[i], ">") == 0) {
			filename = tokenized[i + 1];
			break;
		}
	}
	return filename;
}

int main(int argc, char *argv[]) {
	struct sigaction act;
	act.sa_handler = &sigint_handler;
	if (sigaction(SIGINT, &act, NULL) == -1) {
		fprintf(stderr, "sigaction() failed on errno: %d", errno);
	}

	for(;;) {
		time_t time_secs = time(NULL);
		if (time_secs == -1) {
			fprintf(stderr, "time() failed on errno: %d", errno);
			exit(EXIT_FAILURE);
		}

		struct tm curr_time = *(localtime(&time_secs));
		printf("[%02d/%02d %02d:%02d] # ", curr_time.tm_mday, curr_time.tm_mon, curr_time.tm_hour, curr_time.tm_min); // Printing bash prompt.
		
		int num_tokens;
		char **tokenized_str = get_tokenized_str(&num_tokens);	
		
		if (num_tokens == -1) {
			putc('\n', stdout);
			if (errno == EINTR) {
				// Getline was interrupted
				clearerr(stdin); // Clear error in stdin
				errno = 0;
				continue;	
			} else if (errno == 0) {		
				exit(EXIT_SUCCESS); // Successful exit.
			}
		}

		if (strcmp(tokenized_str[0],"cd") == 0) {
			cd_handler(tokenized_str[1]);
			continue;
		}

		char * redirect_file = check_redirect(num_tokens, tokenized_str);
		int redirect_fd;
		int save_stdout = dup(fileno(stdout));

		if (redirect_file != NULL) {
			redirect_fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			dup2(redirect_fd, fileno(stdout));
			tokenized_str[num_tokens - 2] = NULL;
		}

		// Passing information into execvp
		pid_t pid = fork();
		if (pid == -1) {
			fprintf(stderr, "fork() failed on errno: %d\n", errno);
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			// Child Process - Execvp 
			if (execvp(tokenized_str[0], tokenized_str) == -1) {
				fprintf(stderr, "execvp() failed on errno: %d\n", errno);
				exit(EXIT_FAILURE);
			}
		} else {
			// Parent Process - Wait 
			pid = wait(NULL);
			if (pid == -1) {
				fprintf(stderr, "wait() failed on errno: %d\n", errno);
				exit(EXIT_FAILURE);
			} else if (redirect_file != NULL) {
				fflush(stdout);
				close(redirect_fd);
				dup2(save_stdout, fileno(stdout));
				close(save_stdout);
			}
		}
	}
}
