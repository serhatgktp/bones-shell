#include "byos.h"
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
// Find out what other #include's you need! (E.g., see man pages.)

#define MAX_MSG_LEN 100 // Max length for success/failure messages
#define MAX_ARG_LEN 10000   // Max length for argument passed to echo

int interp(const struct cmd *c)
{
    // Consider three cases: FORX, ECHO, LIST
    // Implement recursively for LIST

    // Save existing output stream
    int init_out_stream = dup(1);

    // Set file descriptor if specified
    int fd = 1; // Set fd to STDOUT by default
    if(c->redir_stdout != NULL) // If redir_stdout != NULL, set fd to the file at the path redir_stdout. If the file doesn't exist, create it.
    {
        char *filename = c->redir_stdout;
        if ((fd = open(filename,O_CREAT | O_WRONLY | O_TRUNC,0666)) < 0)    // Truncates file upon opening
        {
            close(fd);
            close(init_out_stream);
            perror("open"); // If open fails, print an error message
            return 1;   // If open fails, return 1
        }
        dup2(fd, 1); // Set current output stream to fd
        close(fd);  // We may close fd now that it has been duplicated
    }

    if (c->type == LIST)    // Recursive case
    {
        int list_len = c->data.list.n;
        for(int i=0; i<list_len; i++)
        {
            // printf("Interp called recursively!\n");
            int a = interp(&(c->data.list.cmds[i]));
            if (a != 0) // Failure case
            {
                dup2(init_out_stream, 1);   // Restore initial output stream before returning
                close(init_out_stream);
                return a;
            }
        }
        dup2(init_out_stream, 1);   // Restore initial output stream before returning
        close(init_out_stream);
        return 0;   // Success case
    }

    else if(c->type == ECHO)
    {
        char *arg = c->data.echo.arg;

        if (write(1, arg, strlen(arg)) != strlen(arg)) {    // Write arg to output stream
            char error_msg[MAX_MSG_LEN];  // Error case for write
            sprintf(error_msg, "(echo) There was an error writing to fd: %d\n", fd);
            write(2, error_msg, strlen(error_msg)); // Write to stderr
            perror("write");

            dup2(init_out_stream, 1);   // Restore initial output stream before returning
            close(init_out_stream);
            return 1;   // Return 1 if writing fails
        }
    dup2(init_out_stream, 1);   // Restore initial output stream before returning
    close(init_out_stream);
    return 0;   // Success case

    }

    else if(c->type == FORX)
    {

        // Create child process and wait
        pid_t pid;
        int status;
        pid = fork();
        if (pid > 0)    // Parent case
        {
            if (waitpid(pid, &status, 0) == -1) // Wait for child
            {
                perror("\n\nwaitpid failed\n\n");   // Error case for waitpid()
            }

            // Case for exit with signal
            if (WIFSIGNALED(status)){
                int term_sig = WTERMSIG(status);
                char out_str[MAX_MSG_LEN];
                
                dup2(init_out_stream, 1);   // Restore initial output stream before returning
                close(init_out_stream);
                return (128 + term_sig);   // Return 128 + signal if the process was killed by signal
            }

            // Case for natural exit
            else if (WEXITSTATUS(status)){
                int es = WEXITSTATUS(status);
                char exit_msg[MAX_MSG_LEN];
                
                dup2(init_out_stream, 1);   // Restore initial output stream before returning
                close(init_out_stream);
                return es;  // Return exit status
            }
        }
        else if(pid == 0)   // Child case ||| For executing consider 'execvp(char *file, char *argv[]);' as it doesn't require a full path to the file
        {

            char *path = c->data.forx.pathname;
            char **args = c->data.forx.argv;
            
            // Print out command prior to executing
            char exec_msg_buffer[MAX_MSG_LEN] = "    [FORK] Executing: ";
            int i = 0;
            while (args[i] != NULL)
            {
                strcat(exec_msg_buffer, args[i]);
                strcat(exec_msg_buffer, " ");
                i++;
            }
            strcat(exec_msg_buffer, "\n");
            // printf("%s", exec_msg_buffer);

            int ret = execvp(path, args); // Execute the command

            if(ret == -1)   // exec failure case
            {
                perror("exec");
                exit(127);
            }

            dup2(init_out_stream, 1);   // Restore initial output stream before returning
            close(init_out_stream);
            return 1;   // Exit child
        }
    }
     
    return 0;   // Success (End Of Function)
}