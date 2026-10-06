#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/syscall.h>

#define CHAR_BUFF_SIZE 256
#define GETDENTS_BUF_SIZE 1024

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

enum BUILT_IN_FUNCS {
	QUIT,	
	CD,
	KILL,
	LIST,
	PID,
	PRINTENV,
	PWD,
	WHICH,
	UNKNOWN
};

struct INSTANCE {
	uint id;

	char* user;
	char* path;
	char* cwd;
	char* home;
};

enum BUILT_IN_FUNCS map_str_to_func(const char* cmd) {
	if(strcmp(cmd, "quit") == 0)
		return QUIT;
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
	unsigned int 	fd, nread;
	char 			buf[GETDENTS_BUF_SIZE];
	struct 			linux_dirent *dir;
	unsigned int 	bpos;
	char 			d_type;

	fd = open(path, O_RDONLY | O_DIRECTORY);
	if (fd == -1)
		handle_error("open");
	
	for (;;) {
		nread = syscall(SYS_getdents, fd, buf, GETDENTS_BUF_SIZE);
		if (nread == -1)
			handle_error("getdents");

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
			printf("----%s\n", (d_type == DT_DIR) ? "<DIR>" : "<FILE>");
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

void output_CWD(struct INSTANCE* instance) {
	printf("%s$ ", instance->cwd);
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
		case LIST:
			list_dir(instance->cwd, false);
			break;
		default:
			printf("<%s>: Command not found.\n", user_input);
			break;
	}	
}

void run() {

}

void init_shell(struct INSTANCE* instance) {
	instance->user = getenv("USER");
	instance->path = getenv("PATH");
	instance->home = getenv("HOME");
	instance->cwd = getenv("HOME");
	chdir(instance->cwd);
}

void output_envr_vars(struct INSTANCE* instance) {
	printf("User: %s\n", instance->user);
	printf("Path: %s\n", instance->path);
	printf("CWD: %s\n", instance->cwd);
	printf("Home: %s\n", instance->home);
}

int main() {
	struct INSTANCE instance;
	init_shell(&instance);
	output_CWD(&instance);
	if(valid_directory(instance.cwd)) {
		list_dir(instance.cwd, false);
	}
	return 0;
}
