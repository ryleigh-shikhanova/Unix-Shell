#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum BUILT_IN_FUNCS {
	QUIT,	
};

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

void run() {
	enum BUILT_IN_FUNCS selection;
	switch(selection) {
	default:
		break;
	}
}
int main() {
	run();
	return 0;
}
