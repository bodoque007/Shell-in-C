#include <stdbool.h>

#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/wait.h>

const char* BUILTINS[] = {"echo", "exit", "type", "pwd"};
bool value_in_array(const char * val, const char *arr[], size_t size) {
  for(size_t i = 0; i < size; i++) {
    if(!strcmp(arr[i], val))
      return true;
  }
  return false;
}

const char * look_in_path(const char * path,
  const char * command) {
  char * pathCopy = strdup(path);
  if (!pathCopy) {
    perror("strdup");
    return NULL;
  }

  bool found = false;
  char * fullPath = malloc(1024);
  if (!fullPath) {
    perror("malloc");
    free(pathCopy);
    return NULL;
  }

  char * saveptr; // Use strtok_r instead of strtok, as strtok is a global tokenizer.
  char * dir = strtok_r(pathCopy, ":", & saveptr);

  while (dir != NULL) {
    snprintf(fullPath, 1024, "%s/%s", dir, command);
    if (access(fullPath, X_OK) == 0) {
      found = true;
      break;
    }
    dir = strtok_r(NULL, ":", & saveptr);
  }

  free(pathCopy);
  if (found) {
    return fullPath;
  }

  free(fullPath);
  return NULL;
}

void run_command(const char * path,
  const char * input) {
  char * cmdCopy = strdup(input);
  if (!cmdCopy) {
    perror("strdup");
    return;
  }

  // Tokenize input to get the command
  char * saveptr;
  char * prog = strtok_r(cmdCopy, " ", & saveptr);
  if (!prog) {
    fprintf(stderr, "Invalid command\n");
    free(cmdCopy);
    return;
  }

  // Look for the program in PATH
  const char * fullPath = look_in_path(path, prog);
  if (!fullPath) {
    printf("%s: command not found\n", prog);
    free(cmdCopy);
    return;
  }

  // Prepare arguments array
  char * args[256];
  int i = 1;
  args[0] = prog; // Include the program name as the first argument

  // Subsequent calls to strtok_r: tokenize to get the arguments
  char * arg;
  while ((arg = strtok_r(NULL, " ", & saveptr)) != NULL) {
    args[i] = arg;
    i++;
  }
  args[i] = NULL;

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    free(cmdCopy);
    free((void * ) fullPath);
    return;
  }

  if (pid == 0) {
    // Child process
    execv(fullPath, args);
    perror("execv"); // If execv fails, as execv only returns if the execution of the program passed as argument fails
    exit(EXIT_FAILURE);
  } else {
    // Parent process
    int status;
    waitpid(pid, & status, 0);
  }

  free(cmdCopy);
  free((void * ) fullPath);
}

void shell_loop() {
  char input[256];
  const char * keyword = "echo";
  const char * keyword2 = "type";
  size_t keyword_len = strlen(keyword);
  size_t keyword_len2 = strlen(keyword2);
  const char * path = getenv("PATH");

  while (true) {
    printf("$ ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) == NULL) {
      break; // Exit if fgets fails (e.g., EOF)
    }
    input[strcspn(input, "\n")] = '\0'; // Remove the newline character
    if (!strcmp(input, "exit 0")) {
      break;
    }
    if (!strncmp(input, keyword, keyword_len)) { // If command is "echo"
      if (strlen(input) > keyword_len) {
        char * to_echo = input + keyword_len + 1;
        printf("%s\n", to_echo);
      }
    } else if (!strncmp(input, keyword2, keyword_len2)) { // If command is "type"
      char * to_type = input + keyword_len2 + 1;
      while (*to_type == ' ') {
        to_type++;
      }
      if (value_in_array(to_type, BUILTINS, sizeof(BUILTINS) / sizeof(BUILTINS[0]))) {
        printf("%s is a shell builtin\n", to_type);
      } else { // Try looking for command in PATH env variable
        const char * full_path = look_in_path(path, to_type);
        if (full_path != NULL) {
          printf("%s is %s\n", to_type, full_path);
          free((void * ) full_path);
        } else {
          printf("%s: not found\n", to_type);
        }
      }
    } else if (!strcmp(input, "pwd")) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("%s\n", cwd);
      } else if (!strncmp(input, "cd ", 3)) {
          char * path_to_change = input + 3;
          while (*path_to_change == ' ') {
            path_to_change++;
          }
          if (strcmp(path_to_change, "~") == 0) {
            path_to_change = getenv("HOME");
          }
          if (chdir(path_to_change) < 0) {
            printf("cd: %s: No such file or directory\n", path_to_change);
          }
      } else { // Try running the command with its corresponding inputs
          run_command(path, input);
      }
  }
}

int main() {
  shell_loop();
  return 0;
}
