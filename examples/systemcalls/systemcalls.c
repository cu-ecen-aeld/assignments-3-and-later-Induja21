#include "systemcalls.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syslog.h>
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd)
{
    /*
     * system() call return 0 on success and -1 if error in creating a child process
     *  It can also return a positive value if exits with an error code
     */
    openlog("systemcalls.c", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);
    int status = system(cmd);
    if (status == -1)
    {
        syslog(LOG_ERR,"system() operation failed \n");
        closelog();
        return false;
    }
    //Check if command terminated normally and returned a 0 exit code
    if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    {
        syslog(LOG_DEBUG,"Child process exited normally \n");
        closelog();
        return true;
    }
    else
    {
        syslog(LOG_ERR,"Child process exited with error code %d \n",status);
        closelog();
        return false;
    }
}

/**
 * @param count -The numbers of variables passed to the function. The variables are command to execute.
 *   followed by arguments to pass to the command
 *   Since exec() does not perform path expansion, the command to execute needs
 *   to be an absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in execv()
 * @return true if the command @param ... with arguments @param arguments were executed successfully
 *   using the execv() call, false if an error occurred, either in invocation of the
 *   fork, waitpid, or execv() command, or if a non-zero return value was returned
 *   by the command issued in @param arguments with the specified arguments.
 */

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int i;
    int status;
    int pid;
    openlog("systemcalls.c", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


    /*
     * TODO:
     *   Execute a system command by calling fork, execv(),
     *   and wait instead of system (see LSP page 161).
     *   Use the command[0] as the full path to the command to execute
     *   (first argument to execv), and use the remaining arguments
     *   as second argument to the execv() command.
     *
     */

    pid = fork();
    if (pid == -1)
    {
        va_end(args);
        syslog(LOG_ERR,"Fork operation failed \n");
        closelog();
        return false;
    }
    else if (pid == 0)
    {
        if (execv(command[0], command) == -1)
        {
            syslog(LOG_ERR,"execv operation failed in child process \n");
            closelog();
            exit(EXIT_FAILURE);
        }
    }

    if (waitpid(pid, &status, 0) == -1)
    {
        va_end(args);
        syslog(LOG_ERR,"waitpid operation failed \n");
        closelog();
        return false;
    }

    va_end(args);
    // Check if the child process terminated normally and with a zero exit status
    if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    {
        syslog(LOG_DEBUG,"Child process exited normally \n");
        closelog();
        return true;
    }
    else
    {
        syslog(LOG_ERR,"Child process exited with error code %d \n",status);
        closelog();
        return false;
    }
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int status;
    int pid;
    int i;

    openlog("systemcalls.c", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    /*
     * TODO
     *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
     *   redirect standard out to a file specified by outputfile.
     *   The rest of the behaviour is same as do_exec()
     *
     */
    pid = fork();
    if (pid == -1)
    {
        close(fd);
        va_end(args);
        syslog(LOG_ERR,"Fork operation failed \n");
        closelog();
        return false;
    }
    else if (pid == 0)
    {
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            syslog(LOG_ERR,"Duplicating fd failed \n");
            close(fd);
            abort();
        }
        close(fd);
        if (execv(command[0], command) == -1)
        {
            syslog(LOG_ERR,"execv operation failed in child process \n");
            closelog();
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
    if (waitpid(pid, &status, 0) == -1)
    {
        va_end(args);
        syslog(LOG_ERR,"waitpid operation failed \n");
        closelog();
        return false;
    }

    va_end(args);
    // Check if the child process terminated normally and with a zero exit status
    if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    {
        syslog(LOG_DEBUG,"Child process exited normally \n");
        closelog();
        return true;
    }
    else
    {
        syslog(LOG_ERR,"Child process exited with error code %d \n",status);
        closelog();
        return false;
    }

    va_end(args);

    return true;
}
