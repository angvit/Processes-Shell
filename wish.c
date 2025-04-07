#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


#define BUFF_SIZE 1000
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1


typedef struct
{
    char **args;
    pthread_t thread_id;
} Command;


// the initial shell path should contain one directory "/bin"
char *shell_paths[BUFF_SIZE] = {"/bin", "/usr/bin", NULL};
int num_paths = 1;


void error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}


char *cell_check_executable(char **args)
{
    // edge case check for empty commands
    if (args[0] == NULL) return NULL;
    char full_path[BUFF_SIZE];
    for (int i=0; shell_paths[i]; i++)
    {
        // building a full file path by combining the current path and command 
        snprintf(full_path, BUFF_SIZE, "%s/%s", shell_paths[i], args[0]);
        // system call for checking if file exists in directory and is executable
        if (access(full_path, X_OK) == 0) return strdup(full_path);
    }
    return NULL;
};


int cell_built_ins(char **args)
{
    int argc = 0;
    while (args[argc] != NULL) argc++;

    if (strcmp(args[0], "exit") == 0) 
    {
        if (argc != 1) // throw an error and 
        {
            error();
            return 1;
        }
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(args[0], "cd") == 0) 
    {
        // int argc = 0;
        // while (args[argc] != NULL) argc++;

        if (argc != 2) error();
        else if (chdir(args[1]) != 0) error();
        return 1;
    }
    else if (strcmp(args[0], "path") == 0)
    {
        // clearing old path
        for (int i=0; i<num_paths; i++)
            shell_paths[i] = NULL;
        
        num_paths = 0;

        // overwriting old path with newly specified path 
        for (int i=1; args[i]; i++)
            shell_paths[num_paths++] = strdup(args[i]);
        
        return 1;
    }
    return 0;
}


// using for parsing from stdin in interactive mode, and from files in batch mode
char *cell_read_line(FILE *input_stream)
{
    char *buf = NULL;
    size_t bufsize;

    if (getline(&buf, &bufsize, input_stream) == -1)
    {
        buf = NULL;

        if (feof(input_stream))
        {
            exit(EXIT_SUCCESS);
        }
        else 
        {
            error();
            exit(EXIT_FAILURE);
        }   
    }
    return buf;
}


char **cell_split_commands(char *line)
{
    char **commands;
    char *command;
    int position = 0;
    
    commands = malloc(sizeof(char *) * 128);

    while ((command = strsep(&line, "&")) != NULL)
    {
        // strsep replaces delimiter 
        if (*command == '\0') continue;
        commands[position++] = command;
    }
    commands[position] = NULL;
    return commands;
}


char **cell_split_line(char *line)
{
    char **tokens;
    char *token;
    size_t buf_size = BUFF_SIZE;
    int position = 0;

    tokens = malloc(buf_size * sizeof(char *));

    // all the chars that are used by isspace() function
    char *del = "\n\t\v\f\r ";

    while ((token = strsep(&line, del)) != NULL)
    {
        if (*token == '\0') continue;
        tokens[position++] = token;
    }
    
    tokens[position] = NULL;
    return tokens;
}


int cell_search_redirect(char **args)
{
    int redirect_idx = -1;
    for (int i=0; args[i]; i++)
        if (strcmp(args[i], ">") == 0)
            {
                redirect_idx = i;
                break;
            }
    return redirect_idx;
}


char *cell_validate_redirect(char **args, int redirection_idx)
{
    if (args[redirection_idx + 1] == NULL || args[redirection_idx + 2] != NULL) 
    {
        return NULL;
    }
    // cutting off args at the redirection symbol, prepping array for execv()
    args[redirection_idx] = NULL; 
    char *output_file = args[redirection_idx + 1];
    return output_file;
}


char *normalize_redirect(char *line)
{
    char buffer[BUFF_SIZE];
    int j = 0;

    for (int i=0; line[i]; i++)
    {
        if (line[i] == '>')
        {
            buffer[j++] = ' ';
            buffer[j++] = '>';
            buffer[j++] = ' ';
        } 
        else
        {
            buffer[j++] = line[i];
        }
    }
    buffer[j] = '\0';
    strcpy(line, buffer);
};


// pthread_create requires the thread function to take a void * argument
void *cell_handle_command(void *command)
{
    // casting the input command safely
    Command *cmd = (Command *)command;
    char **args = cmd->args;
    char *output_file = NULL;

    if (args[0] == NULL)
    {
        // free(args);
        return NULL;
    }

    if (cell_built_ins(args)) return NULL;

    int redirect_idx = cell_search_redirect(args);

    if (redirect_idx != -1)
    {
        output_file = cell_validate_redirect(args, redirect_idx);
        if (output_file == NULL) 
        {
            error();
            return NULL;
        };
    }
    
    char *executable_file = cell_check_executable(args);

    if (executable_file == NULL)
    {
        error();
        return NULL;
    } 

    int fc = fork();

    if (fc == 0) // in the child process
    {
        if (output_file != NULL)
        {
            // int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            // file gets created with no permissions, need to add 0644 to read
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
                error();
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execv(executable_file, args);
        error();
        exit(EXIT_FAILURE);

    } else if (fc < 0) // if fork fails 
    {
        error();
    } else // in the parent process
    {
        wait(NULL);
    }
    free(executable_file);
    return NULL;
}


int main(int argc, char* argv[])
{
    FILE *input_stream;

    if (argc == 1) // interactive mode
    { 
        input_stream = stdin; 
    } 
    else if (argc == 2) // batch mode
    {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL)
        {
            error();
            exit(EXIT_FAILURE);
        }
    }
    else // too many arguments
    {
        error();
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        if (input_stream == stdin)
        {
            printf("wish> ");
        }

        char *line = cell_read_line(input_stream); // get full input line
        if (line == NULL) break;

        normalize_redirect(line); // built to prevent 

        char **commands = cell_split_commands(line); // split on "&"
        Command *cmd_list[BUFF_SIZE] = {0};
        int cmd_count = 0;
        
        // creating Command objects for each command setting their associated thread
        for (int i=0; commands[i]; i++)
        {
            Command *cmd = malloc(sizeof(Command));
            cmd->args = cell_split_line(commands[i]);

            // upon creation of a thread, pass each to cell_handle_command
            pthread_create(&cmd->thread_id, NULL, cell_handle_command, (void *)cmd);
            cmd_list[cmd_count++] = cmd; // append each new Command object to array
        }

        // joining all threads
        for (int i=0; cmd_list[i]; i++)
        {
            pthread_join(cmd_list[i]->thread_id, NULL);
            free(cmd_list[i]->args); // after thread returns, clear command arguments
            free(cmd_list[i]); // free Command object
        }
        free(line);
    }

    return 0;
}