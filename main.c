#include <stdbool.h>

#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/wait.h>

#include "builtins.h"

#define MAX_BUILTINS 32

const char * BUILTINS[] = {
    "echo",
    "pwd",
    "cd",
    "type"
};

bool value_in_array(const char * val,
    const char * arr[], size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (!strcmp(arr[i], val))
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
    char * fullPath = (char * ) malloc(1024);
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
        // Child process, we use execv for the sake of learning and not execvp, which looks for the program in PATH
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

void handle_echo(char * input) {
    int keyword_len = 4;
    if (strlen(input) > keyword_len) {
        input += keyword_len + 1; // Skip the "echo" part
    }
    printf("%s\n", input);
}

void handle_pwd(char * input) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
}

void handle_cd(char * input) {
    char * dir = input + 3; // Skip the "cd " part
    while ( * dir == ' ') {
        dir++;
    }
    if (chdir(dir) != 0) {
        perror("chdir");
    }
}

void handle_type(char * input) {
    char * to_type = input + 4 + 1;
    while ( * to_type == ' ') {
        to_type++;
    }
    if (value_in_array(to_type, BUILTINS, sizeof(BUILTINS) / sizeof(BUILTINS[0]))) {
        printf("%s is a shell builtin\n", to_type);
    } else { // Try looking for command in PATH env variable
        const char * full_path = look_in_path(getenv("PATH"), to_type);
        if (full_path != NULL) {
            printf("%s is %s\n", to_type, full_path);
            free((void * ) full_path);
        } else {
            printf("%s: not found\n", to_type);
        }
    }
}

void shell_loop() {
    const char * path = getenv("PATH");
    BuiltinTable builtins = {
        0
    };

    // Register builtins
    builtin_register( & builtins, "echo", handle_echo);
    builtin_register( & builtins, "pwd", handle_pwd);
    builtin_register( & builtins, "cd", handle_cd);
    builtin_register( & builtins, "type", handle_type);

    while (true) {
        printf("$ ");
        fflush(stdout);

        char * input = NULL;
        size_t len = 0;

        if (getline( & input, & len, stdin) == -1) break;

        input[strcspn(input, "\n")] = '\0'; // Remove newline

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        // Handle exit immediately
        if (!strcmp(input, "exit")) {
            free(input);
            break;
        }

        // Extract command name (first token)
        char * cmdCopy = strdup(input);
        char * saveptr;
        char * cmd = strtok_r(cmdCopy, " ", & saveptr);

        if (!cmd) {
            free(cmdCopy);
            free(input);
            continue;
        }

        // Lookup builtin
        BuiltinCommand * builtin = builtin_lookup( & builtins, cmd);
        if (builtin) {
            builtin -> handler(input);
        } else {
            run_command(path, input);
        }

        free(cmdCopy);
        free(input);
    }
}

int main() {
    shell_loop();
    return 0;
}