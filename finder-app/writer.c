#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define TOTAL_NO_OF_ARGUMENTS 3

typedef enum fileOperationStatus
{
    FILE_WRITE_SUCCESSFUL,
    FILE_OPEN_FAILED,
    FILE_WRITE_FAILED
} fileOps;

void printUsage(const char *executableName)
{
    syslog(LOG_ERR, "Usage: %s <filePath> <textString>\n", executableName);
    syslog(LOG_ERR, " <filePath>   :  Full path of the file where the data is to be written \n");
    syslog(LOG_ERR, " <textString> :  Text to be written to the file\n");
}

fileOps createAndWriteContentsToTheFile(const char *filePath, const char *writeStr)
{
    int fd;
    fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd != -1)
    {
        syslog(LOG_DEBUG, "Writing \'%s\' to \'%s\'", writeStr, filePath);
        if (write(fd, writeStr, strlen(writeStr)) == -1)
        {
            syslog(LOG_ERR, "Error occured while writing to the file %s: %s \n", filePath, strerror(errno));
            close(fd);
            return FILE_WRITE_FAILED;
        }

        fdatasync(fd);
        close(fd);
    }
    else
    {
        syslog(LOG_ERR, "Creation/Open of file %s failed : %s \n", filePath, strerror(errno));
        return FILE_OPEN_FAILED;
    }
    return FILE_WRITE_SUCCESSFUL;
}

int main(int argc, char *argv[])
{
    fileOps fileOperationReturnStatus = FILE_OPEN_FAILED;
    // Open a system logger connection for writer utility
    openlog("writer", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

    // Check if the number of arguments are proper else throw an error and exit
    if (argc != TOTAL_NO_OF_ARGUMENTS)
    {
        syslog(LOG_ERR, "Improper usage of writer utility and hence exiting\n");
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
    }

    fileOperationReturnStatus = createAndWriteContentsToTheFile(argv[1], argv[2]);
    if(fileOperationReturnStatus==FILE_WRITE_SUCCESSFUL)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
    
}