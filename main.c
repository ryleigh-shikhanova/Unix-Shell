#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>

#define CHAR_BUFF_SIZE 256
#define GETDENTS_BUF_SIZE 1024

enum BUILT_IN_FUNCS {
	QUIT,	
	CD,
	KILL,
	LIST,
	PID,
	PRINTENV,
	SETENV,
	PWD,
	WHICH,
	UNKNOWN
};

struct INSTANCE {
	pid_t id;

	char* user;
	char* path; 		// He wants this to be a linked list...? Which seems redundant since PATH is already a colon-separated list of directories, but I can do that if he wants.
	char* cwd;
	char* home;
};

enum BUILT_IN_FUNCS map_str_to_func(const char* cmd) {
	if  (strcmp(cmd, "quit") == 0  ||
		strcmp(cmd, "exit") == 0 ||
		strcmp(cmd, "logout") == 0)	
	{
		return QUIT;
	}
	if(strcmp(cmd, "cd") == 0)
		return CD;
	if(strcmp(cmd, "kill") == 0)
		return KILL;
	if(strcmp(cmd, "list") == 0)
		return LIST;
	if(strcmp(cmd, "pid") == 0)
		return PID;
	if(strcmp(cmd, "printenv") == 0)
		return PRINTENV;
	if(strcmp(cmd, "setenv") == 0)
		return SETENV;
	if(strcmp(cmd, "pwd") == 0)
		return PWD;
	if(strcmp(cmd, "which") == 0)
		return WHICH;
	return UNKNOWN;
}

struct linux_dirent {
	long 			d_ino;
	off_t 			d_off;
	unsigned short  d_reclen;
	char 			d_name[];
};

// This is based on the example provided in the man page for getdents(2)
void list_dir(const char* path, bool show_hidden) {
	int				fd;
	long			nread;
	char 			buf[GETDENTS_BUF_SIZE];
	struct 			linux_dirent *dir;
	unsigned int 	bpos;
	char 			d_type;

	fd = open(path, O_RDONLY | O_DIRECTORY);
	if (fd == -1)
		perror("open");
	
	for (;;) {
		nread = syscall(SYS_getdents, fd, buf, GETDENTS_BUF_SIZE);
		if (nread == -1)
			perror("getdents");

		if (nread == 0)
			break;

		for (bpos = 0; bpos < nread;) {
			dir = (struct linux_dirent *) (buf + bpos);
			if(dir->d_name[0] == '.' && !show_hidden) {
				bpos += dir->d_reclen;
				continue;
			}
			d_type = *(buf + bpos + dir->d_reclen - 1);
			printf("%s:", dir->d_name);
			printf("%s\n", (d_type == DT_DIR) ? "<DIR>" : "<FILE>");
			bpos += dir->d_reclen;
		}
	}
	close(fd);
}

// This function checks if the given path is a valid directory
bool valid_directory(const char* path) {
	int fd = open(path, O_RDONLY | O_DIRECTORY);
	if (fd != -1) {
		close(fd);
	}
	return (fd != -1) ? true : false;
}

void output_pwd(struct INSTANCE* instance) {
	printf("%s$ ", instance->cwd);
}

void print_prompt(struct INSTANCE* instance) {
	printf("%s@%s > ", instance->user, instance->cwd);
}
// This function checks if the given command exists in the directories specified in the PATH environment variable
// If the command is found & show_all is false, it will execute the first instance of the command found in the PATH and return
// If the command is found & show_all is true, it will output all instances of the command found in the PATH
void which(const char* cmd, struct INSTANCE* instance, bool show_all) {
	char* path_env = instance->path;
	char* env_copy = strdup(path_env); 
	char* token = strtok(env_copy, ":");	
	char  command_path[CHAR_BUFF_SIZE];
	bool  found = false;

	while(token != NULL) {
		snprintf(command_path, sizeof(command_path), "%s/%s", token, cmd);
		if (access(command_path, X_OK) == 0) {
			printf("%s\n", command_path);
			found = true;
		}
		token = strtok(NULL, ":");
	}
	if (!found) {
		printf("<%s>: Command not found.\n", cmd);
	}
}

void cd(const char* path, struct INSTANCE* instance) {
	if(path == NULL || strcmp(path, "~") == 0) {
		chdir(instance->home);
	}else{
		valid_directory(path) ? chdir(path) : printf("<%s>: No such directory.\n", path);	
	}
	instance->cwd = getcwd(NULL, 0);
}

void pid(struct INSTANCE* instance) {
	printf("PID: %d\n", instance->id);
}

void kill_proc(const char* pid, int sig) {
	if (kill(atoi(pid), sig) == -1) {
		perror(errno == ESRCH ? "No such process" : "Could not kill process");
	}
}

void set_env(const char* var, const char* value, int overwrite) {
	return;
}

void init_shell(struct INSTANCE* instance) {
	instance->id = getpid();
	instance->user = getenv("USER");
	instance->path = getenv("PATH");
	instance->home = getenv("HOME");
	instance->cwd = getcwd(NULL, 0);
	cd(instance->cwd, instance);
}

void output_envr_vars(struct INSTANCE* instance) {
	printf("User: %s\n", instance->user);
	printf("PID: %d\n", instance->id);
	printf("Path: %s\n", instance->path);
	printf("CWD: %s\n", instance->cwd);
	printf("Home: %s\n", instance->home);
}

//reads input and ensures that fgets is cleared if a \0 is lost
bool read_ln(char *buff, size_t size) {
  if (fgets(buff, size, stdin) == NULL) {
    return false;
  }

  size_t len = strlen(buff);

  if (len > 0 && buff[len - 1] == '\n') {
    buff[len - 1] = '\0';
  }
  else {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF){}
  }
  return true;
}

bool get_string_input(char *input, size_t size) {
  return read_ln(input, size);
}

void resolve_user_input(char* user_input, struct INSTANCE* instance) {
	switch(map_str_to_func(user_input)) {
		case QUIT:
			exit(0);
			break;
		case CD:
			cd(NULL, instance);
			break;
		case KILL:
			kill_proc("12345", SIGKILL);
			break;
		case LIST:
			list_dir(instance->cwd, false);
			break;
		case PID:
			pid(instance);
			break;
		case PWD:
			output_pwd(instance);
			break;
		case PRINTENV:
			output_envr_vars(instance);
			break;
		case SETENV:
			set_env(user_input, NULL, 1);
			break;
		case WHICH:
			which(user_input, instance, false);
			break;
		default:
			printf("<%s>: Command not found.\n", user_input);
			break;
	}	
}

void run() {

}

int main() {
	struct INSTANCE instance;
	init_shell(&instance);
	print_prompt(&instance);
	cd(NULL, &instance);
	cd("./Coding", &instance);
	if(valid_directory(instance.cwd)) {
		list_dir(instance.cwd, false);
	}
	return 0;
}
