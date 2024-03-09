/**
 * @file main.c
 * @brief This is the main file for the shell. It contains the main function and the loop that runs the shell. MAINNNNNN
 * @version 0.1
 * @date 2023-06-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

// #define MAX_PATH_LENGTH 1024
#define MAX_ALIASES 50
#define MAX_HISTORY_SIZE 100
char commandHistory[MAX_HISTORY_SIZE][MAX_STRING_LENGTH];
int historyIndex = 0;

void sliceString(const char *input, int start, int end, char *result)
{
    strncpy(result, input + start, end - start);
    result[end - start] = '\0';
}

char *getInput(FILE *input_source)
{
    char *command = malloc(MAX_STRING_LENGTH);

    if (fgets(command, MAX_STRING_LENGTH, input_source) == NULL)
    {
        free(command);
        return NULL;
    }

    return command;
}

typedef struct
{
    char *name;
    char *value;
} Alias;

Alias aliases[MAX_ALIASES];
int numAliases = 0;

void createAlias(char *name, char *value)
{
    if (numAliases >= MAX_ALIASES)
    {
        printf("Maximum number of aliases reached\n");
        return;
    }

    char *val = strdup(value);
    char slice[MAX_STRING_LENGTH];
    sliceString(val, 1, strlen(val) - 1, slice);

    aliases[numAliases].name = strdup(name);
    aliases[numAliases].value = strdup(slice);
    numAliases++;
}

// function that destroys a specific alias
void destroyAlias(char *name)
{
    for (int i = 0; i < numAliases; i++)
    {
        if (aliases[i].name != NULL && strcmp(name, aliases[i].name) == 0)
        {
            free(aliases[i].name);
            free(aliases[i].value);

            // Remove the entry from the aliases array
            for (int j = i; j < numAliases - 1; j++)
            {
                aliases[j] = aliases[j + 1];
            }

            numAliases--;
            return;
        }
    }
    printf("Alias '%s' not found\n", name);
}

int isAlias(char *command)
{
    if (command == NULL)
    {
        return -1; // or handle the case appropriately
    }

    int len = strcspn(command, "\n");
    command[len] = '\0';
    for (int i = 0; i < numAliases; i++)
    {
        if (aliases[i].name != NULL && strcmp(command, aliases[i].name) == 0)
        {
            return i;
        }
    }
    return -1;
}

int executeCommand(char *command)
{
    strncpy(commandHistory[historyIndex], command, sizeof(commandHistory[historyIndex]) - 1);
    historyIndex++;

    int aliasIndex = isAlias(command);
    if (aliasIndex != -1)
    {
        size_t commandLength = strlen(aliases[aliasIndex].value) + 1;
        command = malloc(commandLength);
        strcpy(command, aliases[aliasIndex].value);
    }

    if (strncmp(command, "history ", 8) == 0)
    {
        char *arg = command + 8;
        int index = atoi(arg) - 1;

        if (index >= 0 && index < historyIndex)
        {
            strcpy(command, commandHistory[index]);
        }
        else
        {
            printf("Invalid history index\n");
            return 0;
        }
    }

    if (strcmp(command, "exit") == 0)
    {
        return 1;
    }
    else if (strcmp(command, "pwd") == 0)
    {
        char path[MAX_STRING_LENGTH];
        if (getcwd(path, sizeof(path)) != NULL)
        {
            printf("%s\n", path);
        }
        else
        {
            printf("pwd error\n");
        }
    }
    else if (strncmp(command, "cd ", 3) == 0)
    {
        char *directory = command + 3;
        directory[strcspn(directory, "\n")] = '\0';

        int result = chdir(directory);
        if (result != 0)
        {
            printf("Failed to change directory\n");
        }
    }
    else if (strcmp(command, "cd") == 0)
    {
        char *directory = getenv("HOME");
        int result = chdir(directory);

        if (result != 0)
        {
            printf("Failed to change directory\n");
        }
    }
    else if (strncmp(command, "echo ", 5) == 0)
    {
        char *message = command + 5;
        message[strcspn(message, "\n")] = '\0';

        char *quotePos = strchr(message, '\"');
        while (quotePos != NULL)
        {
            memmove(quotePos, quotePos + 1, strlen(quotePos));
            quotePos = strchr(message, '\"');
        }

        printf("%s\n", message);
    }

    else if (strcmp(command, "ls") == 0)
    {
        DIR *dir;
        struct dirent *entry;

        dir = opendir(".");
        if (dir == NULL)
        {
            printf("Failed to open directory\n");
            return 0;
        }

        while ((entry = readdir(dir)) != NULL)
        {
            // Exclude entries for current directory and parent directory
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                printf("%s\n", entry->d_name);
            }
        }

        closedir(dir);
    }

    else if (strncmp(command, "alias ", 6) == 0)
    {
        char *alias = command + 6;
        char *name = strtok_r(alias, " ", &alias);
        char *value = strtok_r(NULL, "", &alias);

        if (name != NULL && value != NULL)
        {
            value[strcspn(value, "\n")] = '\0';
            createAlias(name, value);
        }
        else if (name != NULL)
        {
            char *alias = command + 6;
            alias[strcspn(alias, "\n")] = '\0';
            int aliasIndex = isAlias(alias);
            if (aliasIndex != -1)
            {
                printf("%s='%s'\n", aliases[aliasIndex].name, aliases[aliasIndex].value);
            }
            else
            {
                printf("alias not found\n");
            }
        }
        else
        {
            printf("Invalid alias format\n");
        }
    }

    // else if only alias is passed as command return all current alias saved
    else if (strcmp(command, "alias") == 0)
    {
        for (int i = 0; i < numAliases; i++)
        {
            printf("%s='%s'\n", aliases[i].name, aliases[i].value);
        }
    }

    else if (strncmp(command, "unalias ", 8) == 0)
    {
        char *alias = command + 8;
        alias[strcspn(alias, "\n")] = '\0';
        destroyAlias(alias);
    }

    else if (strcmp(command, "history") == 0)
    {
        for (int i = 0; i < historyIndex; i++)
        {
            printf("%d %s", i + 1, commandHistory[i]);
        }
    }

    else
    {

        char *args[MAX_STRING_LENGTH];
        char *token = strtok(command, " ");
        int i = 0;
        while (token != NULL)
        {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // remove quotations marks from all args before sending to execvp
        for (int j = 0; j < i; j++)
        {
            char *quotePos = strchr(args[j], '\'');
            while (quotePos != NULL)
            {
                memmove(quotePos, quotePos + 1, strlen(quotePos));
                quotePos = strchr(args[j], '\'');
            }

            quotePos = strchr(args[j], '\"');
            while (quotePos != NULL)
            {
                memmove(quotePos, quotePos + 1, strlen(quotePos));
                quotePos = strchr(args[j], '\"');
            }
        }

        int pid = fork();
        if (pid == 0)
        {
            execvp(args[0], args);
            printf("Command not found\n");
            exit(0);
        }
        else
        {
            wait(NULL);
        }
    }

    return 0;
}

void runScript(const char *scriptFile)
{
    FILE *file = fopen(scriptFile, "r");
    if (file == NULL)
    {
        printf("Failed to open script file\n");
        return;
    }

    char *command;
    while ((command = getInput(file)) != NULL)
    {
        // if command is empty line skip and continue to next command
        if (strcmp(command, "\n") == 0)
        {
            free(command);
            continue;
        }

        if (executeCommand(command) == 1)
        {
            free(command);
            break;
        }
        free(command);
    }

    fclose(file);
}

int main(int argc, char *argv[])
{
    char *command;

    if (argc > 1)
    {
        runScript(argv[1]);
        return 0;
    }

    while (1)
    {
        // Read input using readline() with current dir showing up before command input
        char cwd[MAX_STRING_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("\033[0;32m");
            printf("%s", cwd);
            printf("\033[0m");
        }
        else
        {
            printf("pwd error\n");
        }
        command = readline("$ ");

        // Add input to command history
        if (command && *command)
        {
            add_history(command);
        }

        // Process the input...
        int exitCode = executeCommand(command);

        // Free the input memory
        free(command);

        // Check if the command is "exit"
        if (exitCode == 1)
        {
            break;
        }
    }

    return 0;
}