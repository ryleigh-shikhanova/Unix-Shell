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
#include <sys/wait.h>

#include "get_path.h"

#define CHAR_BUFF_SIZE 256
#define GETDENTS_BUF_SIZE 1024
#define MAX_CMD_ARGS 16

extern char **environ;

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

    char *user;
    path_t *path;
    char cwd[CHAR_BUFF_SIZE];
    char *home;
};

struct linux_dirent {
    long            d_ino;
    off_t           d_off;
    unsigned short  d_reclen;
    char            d_name[];
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

void init_shell(struct INSTANCE *instance) {
    instance->id = getpid();
    instance->user = getenv("USER");
    instance->path = getPath();
    instance->home = getenv("HOME");

    if (getcwd(instance->cwd, sizeof(instance->cwd)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

void free_path_list(path_t* pathlist) {
    path_t* current = pathlist;

    while (current != NULL) {
        path_t* next = current->next;
        free(current->element);
        free(current);
        current = next;
    }
}

void print_prompt(struct INSTANCE* instance) {
    printf("%s@%s > ", instance->user, instance->cwd);
}

void output_pwd(struct INSTANCE *instance) {
    printf("%s\n", instance->cwd);
}

void output_envr_vars(struct INSTANCE* instance) {
    path_t* curr = instance->path;

    printf("User: %s\n", instance->user);
    printf("PID: %d\n", instance->id);
    printf("PATH: \n");
    while(curr != NULL) {
        printf(":%s \n", curr->element);
        curr = curr->next;
    }
    printf("CWD: %s\n", instance->cwd);
    printf("Home: %s\n", instance->home);

}

void list_dir(const char *path, bool show_hidden) {
    int fd;
    long nread;
    char buf[GETDENTS_BUF_SIZE];
    struct linux_dirent *dir;
    long bpos;
    char d_type;

    fd = open(path, O_RDONLY | O_DIRECTORY);

    if (fd == -1) {
        perror("open");
        return;
    }

    for (;;) {
        nread = syscall(SYS_getdents, fd, buf, GETDENTS_BUF_SIZE);

        if (nread == -1) {
            perror("getdents");
            close(fd);
            return;
        }

        if (nread == 0)
            break;

        for (bpos = 0; bpos < nread;) {
            dir = (struct linux_dirent *)(buf + bpos);

            if (dir->d_reclen == 0) {
                fprintf(stderr, "getdents: invalid directory entry\n");
                close(fd);
                return;
            }

            if (dir->d_name[0] == '.' && !show_hidden) {
                bpos += dir->d_reclen;
                continue;
            }

            d_type = *(buf + bpos + dir->d_reclen - 1);

            printf("%s ", dir->d_name);
            printf(
                "%s",
                (d_type == DT_DIR) ? "<DIR>" : "<FILE>"
            );
			printf("\n");

            bpos += dir->d_reclen;
        }
    }
    close(fd);
}

void cd(const char *path, struct INSTANCE *instance) {
    const char *target;

    if (path == NULL || strcmp(path, "~") == 0) {
        target = instance->home;
    } else {
        target = path;
    }

    if (chdir(target) == -1) {
        perror("cd");
        return;
    }

    if (getcwd(instance->cwd, sizeof(instance->cwd)) == NULL) {
        perror("getcwd");
    }
}

void pid(struct INSTANCE* instance) {
    printf("PID: %d\n", instance->id);
}

bool parse_pid(const char *str, pid_t *pid) {
    char *endptr;
    long value;

    errno = 0;
    value = strtol(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || value <= 0) {
        return false;
    }

    *pid = (pid_t)value;
    return true;
}

void kill_proc(const char *pid_str, int sig) {
    pid_t pid;

    if (!parse_pid(pid_str, &pid)) {
        fprintf(stderr, "Invalid PID: %s\n", pid_str);
        return;
    }

    if (kill(pid, sig) == -1) {
        if (errno == ESRCH) {
            fprintf(stderr, "No such process\n");
        } else {
            perror("kill");
        }
    }
}

bool set_env(const char *name, const char *value) {
    if (setenv(name, value, 1) == -1) {
        perror("setenv");
        return false;
    }

    return true;
}

// This function checks if the given command exists in the directories specified in the PATH environment variable
// If the command is found & show_all is false, it will execute the first instance of the command found in the PATH and return
// If the command is found & show_all is true, it will output all instances of the command found in the PATH
void which(const char* cmd, struct INSTANCE* instance, bool show_all) {
    path_t* curr = instance->path;
    bool found = false;
    char command_path[CHAR_BUFF_SIZE];

    while(curr != NULL) {
        int w = snprintf(command_path, sizeof(command_path), "%s/%s", curr->element, cmd);

        if (w >= 0 && (size_t)w < sizeof(command_path) && access(command_path, X_OK) == 0) {
            printf("%s\n", command_path);
            found = true;
            if(!show_all) {
                return;
            }
        }

        curr = curr->next;
    }

    if(!found) {
        printf("<%s>: Command not found.\n", cmd);
    }
}

bool find_cmd(const char* cmd, struct INSTANCE* instance, char* res, size_t res_size) {
    path_t* curr = instance->path;

    while(curr != NULL) {
        int w = snprintf(res, res_size, "%s/%s", curr->element, cmd);

        if (w >= 0 && (size_t)w < res_size && access(res, X_OK) == 0) {
            return true;
        }

        curr = curr->next;
    }

    return false;
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

// returns false is get_string_input fails
bool get_user_input(char* input, size_t size) {
    if (!get_string_input(input, size)) {
        printf("Error reading input.\n");
		return false;
    }
	return true;
}

void execute_cmd(const char* path, char **argv, struct INSTANCE *instance) {
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork");
		return;
	}

	if (pid == 0) {
		execve(path, argv, environ);

		perror("execve");
		_exit(EXIT_FAILURE);
	}

	// parent
	if (waitpid(pid, NULL, 0) == -1) {
		perror("waitpid");
	}
}

void handle_exit(char **argv, struct INSTANCE *instance) {
	if(argv[1] != NULL) {
		printf("Usage: exit\n");
		return;
	}
	free_path_list(instance->path);
	exit(0);
}

void handle_cd(char **argv, struct INSTANCE *instance) {
    if(argv[1] == NULL) {
        cd(NULL, instance);
        return;
    }

    if(argv[2] != NULL) {
        printf("Usage: cd <dir>\n");
        return;
    }

    cd(argv[1], instance);
}

void handle_kill(char **argv) {
	if (argv[3] != NULL) {
        printf("Usage: kill [-signal] <pid>\n");
		return;
	}
    if (argv[1] == NULL) {
        printf("Usage: kill [-signal] <pid>\n");
        return;
    }

    if (argv[2] == NULL) {
        kill_proc(argv[1], SIGTERM);
        return;
    }

    char *endptr;
    long sig;

    errno = 0;
    sig = strtol(argv[1] + 1, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || sig <= 0) {
        fprintf(stderr, "Invalid signal: %s\n", argv[1]);
        return;
    } else {
        kill_proc(argv[2], (int)sig);
    }
    printf("Usage: kill [-signal] <pid>\n");
}

void handle_list(char **argv, struct INSTANCE *instance) {
    if (argv[1] == NULL) {
        list_dir(instance->cwd, false);
        return;
    }

    for (int i = 1; argv[i] != NULL; i++) {
        printf("\n%s:\n", argv[i]);
        list_dir(argv[i], false);
    }
}

void handle_pid(char **argv, struct INSTANCE *instance) {
    if(argv[1] != NULL) {
        printf("Usage: pid\n");
        return;
    }

    pid(instance);
}

void handle_printenv(char **argv) {
    if (argv[1] != NULL) {
        fprintf(stderr, "Usage: printenv\n");
        return;
    }

    for (char **env = environ; *env != NULL; ++env) {
        printf("%s\n", *env);
    }
}

void handle_setenv(char **argv) {
    if (argv[1] == NULL || argv[3] != NULL) {
        fprintf(stderr, "Usage: setenv <var> [value]\n");
        return;
    }

    if (argv[2] == NULL) {
        set_env(argv[1], "");
        return;
    }

    set_env(argv[1], argv[2]);
}

void handle_pwd(char **argv, struct INSTANCE *instance) {
    if(argv[1] != NULL) {
        printf("Usage: pwd\n");
        return;
    } 

    output_pwd(instance);
}

void handle_which(char **argv, struct INSTANCE *instance) {
    if (argv[1] == NULL) {
        printf("Usage: which [-a] <cmd>\n");
        return;
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argv[2] == NULL || argv[3] != NULL) {
            printf("Usage: which [-a] <cmd>\n");
            return;
        }

        which(argv[2], instance, true);
        return;
    }

    if (argv[2] != NULL) {
        printf("Usage: which [-a] <cmd>\n");
        return;        
    }

    which(argv[1], instance, false);
}

void handle_unknown(char **argv, struct INSTANCE *instance) {
    char res[CHAR_BUFF_SIZE];

    if(argv[0][0] == '/') {
        if(access(argv[0], X_OK) == 0) {
            execute_cmd(argv[0], argv, instance);
        } else {
            printf("<%s>: Command not found.\n", argv[0]);
        }

		return;
    } //abs path

    if (strncmp(argv[0], "./", 2) == 0 || strncmp(argv[0], "../", 3) == 0) {
		if(access(argv[0], X_OK) == 0) {
			execute_cmd(argv[0], argv, instance);
		} else {
			printf("<%s>: Command not found.\n", argv[0]);
		}

		return;
    } //rel path

    if (find_cmd(argv[0], instance, res, sizeof(res))) {
		execute_cmd(res, argv, instance);
	} else {
		printf("<%s>: Command not found.\n", argv[0]);
	}
}

void resolve_user_input(char *user_input, struct INSTANCE *instance) {
    char *argv[MAX_CMD_ARGS] = {NULL};
    size_t argc = 0;

    char *token = strtok(user_input, " ");

    while (token != NULL && argc < MAX_CMD_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) {
        return;
	}

    switch (map_str_to_func(argv[0])) {
        case QUIT:
            handle_exit(argv, instance);
            break;

        case CD:
            handle_cd(argv, instance);
            break;

        case KILL:
            handle_kill(argv);
            break;

        case LIST:
            handle_list(argv, instance);
            break;

        case PID:
            handle_pid(argv, instance);
            break;

        case PWD:
            handle_pwd(argv, instance);
            break;

        case PRINTENV:
            handle_printenv(argv);
            break;

        case SETENV:
            handle_setenv(argv);
            break;

        case WHICH:
            handle_which(argv, instance);
            break;

        case UNKNOWN: {
            handle_unknown(argv, instance);
            break;
        }
    }
}

void run(struct INSTANCE* instance) {
    char user_input[CHAR_BUFF_SIZE];

    while (1) {
        print_prompt(instance);
        if (!get_user_input(user_input, sizeof(user_input))) {
			printf("\n");
			break;
		}

		resolve_user_input(user_input, instance);
    }
}

int main() {
    struct INSTANCE instance;
    init_shell(&instance);
    run(&instance);
    return 0;
}