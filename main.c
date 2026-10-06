#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

enum BUILT_IN_FUNCS map_str_to_func(const char* cmd) {
	if(strcmp(cmd, "cd") == 0)
		return CD;
	if(strcmp(cmd, "kill") == 0)
		return KILL;
	return UNKNOWN;
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

void output_CWD() {
	char *cwd = getcwd(NULL, 0);
	printf("%s$ ", cwd);
	free(cwd);
}

void resolve_user_input(char* user_input) {
	switch(map_str_to_func(user_input)) {
		default:
			printf("<%s>: Command not found.\n", user_input);
			break;
	}	
}
void run() {

}
int main() {
	output_CWD();
	run();
	printf("\n");
	return 0;
}
