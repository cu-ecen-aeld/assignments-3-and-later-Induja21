/******************************************************
# This program  
# 1) Creates a stream socket
# 2) Listens for a connection
# 3) Stores the data received from clients in /var/tmp/aesdsocketdata file
# 4) Returns the same data back to the client
# Author: Induja Narayanan <Induja.Narayanan@colorado.edu>
******************************************************/
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>  
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#pragma GCC diagnostic warning "-Wunused-variable"

#define SOCKETDATA_FILE "/var/tmp/aesdsocketdata"
#define CLIENT_BUFFER_LEN 1024
FILE *tmp_file = NULL;
bool exit_main_loop = false;

bool create_daemon()
{
    pid_t pid;
    pid = fork();
    bool status = false;
    int dev_null_fd;

    if (pid < 0)
    {
        syslog(LOG_ERR, "Fork failed");
        return status;
    }

    if(pid > 0)
    {
        //Parent process hence exit
        exit(EXIT_SUCCESS);
    }

    //create new group and session
    if (setsid() < 0) {
        syslog(LOG_ERR, "Create new session  failed");
        return status;
    }

    //Change the working directory to "/"
    if(chdir("/")==-1)
    {
        syslog(LOG_ERR, "Changing working directory failed");
        return status;

    }
    //Since no files were open in parent, no fds are closed here
    // Redirect STDIN , STDOUT and STDERR to /dev/null
    dev_null_fd = open("/dev/null", O_RDWR);
    if (dev_null_fd == -1) {
        perror("Failed to open /dev/null");
        return status;
    }

    // Redirect stdin (fd 0) to /dev/null
    if (dup2(dev_null_fd, STDIN_FILENO) == -1) {
        perror("Failed to redirect stdin");
        close(dev_null_fd);
        return status;
    }

    // Redirect stdout (fd 1) to /dev/null
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        close(dev_null_fd);
        return status;
    }

    // Redirect stderr (fd 2) to /dev/null
    if (dup2(dev_null_fd, STDERR_FILENO) == -1) {
        perror("Failed to redirect stderr");
        close(dev_null_fd);
        return status;
    }

    // Close the original /dev/null file descriptor
    close(dev_null_fd);
    return true;
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        syslog(LOG_INFO,"Caught SIGINT (Ctrl+C), exiting gracefully\n");
    } else if (signal == SIGTERM) {
         syslog(LOG_INFO,"Caught SIGTERM, exiting gracefully\n");
    }
    // Perform any clean-up tasks here, if necessary
    exit_main_loop = true;
}

void initialize_sigaction()
{
    struct sigaction sighandle;
    //Initialize sigaction
    sighandle.sa_handler = signal_handler;
    sigemptyset(&sighandle.sa_mask);  // Initialize the signal set to empty
    sighandle.sa_flags = 0;            // No special flags

    // Catch SIGINT
    if (sigaction(SIGINT, &sighandle, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
    }

    // Catch SIGTERM
    if (sigaction(SIGTERM, &sighandle, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
     }

}



int receive_and_store_socket_data(int client_fd, int file_fd) {
    char *client_buffer = NULL;
    size_t total_received = 0;
    size_t current_size = CLIENT_BUFFER_LEN;
    size_t multiplication_factor = 1;

    // Dynamically allocate initial buffer
    client_buffer = (char *)calloc(current_size, sizeof(char));
    if (client_buffer == NULL) {
        syslog(LOG_ERR, "Client buffer allocation failed, returning with error");
        return -1;
    }

    while (true) {
        // Receive data from client
        ssize_t received_no_of_bytes = recv(client_fd, client_buffer + total_received, current_size - total_received - 1, 0);
        if (received_no_of_bytes <= 0) {
            break; // Connection closed or error
        }
        total_received += received_no_of_bytes;
        client_buffer[total_received] = '\0'; // Null-terminate the string

        // Check for newline
        if (strchr(client_buffer, '\n') != NULL) {
            break; // Newline found, exit the loop
        }

        // If we reach this point, we need to resize the buffer
        multiplication_factor <<= 1;
        size_t new_size = multiplication_factor * CLIENT_BUFFER_LEN;
        char *new_buffer = (char *)realloc(client_buffer, new_size);
        if (new_buffer == NULL) {
            syslog(LOG_ERR, "Reallocation of client buffer failed, returning with error");
            free(client_buffer);
            return -1;
        }
        client_buffer = new_buffer;
        current_size = new_size;
    }

    // Now we have the complete data, store it in the file
    syslog(LOG_INFO, "Writing received data to the sockedata file");
    if(write(file_fd, client_buffer, total_received)!=-1)
    {
      syslog(LOG_INFO, "Syncing data to the disk");
      fdatasync(file_fd);
    }
    else
    {
	syslog(LOG_ERR, "Writing received data to the socketdata file failed");
	free(client_buffer);
	return -1;
    }

    free(client_buffer);
    return 0; // Return success
}


int return_socketdata_to_client(int client_fd,int file_fd)
{
    char *send_buffer;
    size_t bytes_read;
    lseek(file_fd, 0, SEEK_SET);
    send_buffer = (char *)malloc(CLIENT_BUFFER_LEN);
    if(send_buffer == NULL)
    {
        syslog(LOG_INFO, "Client buffer was not allocated hence returning with error");
        return -1;
    }
    
    // Read and send data
    while ((bytes_read = read(file_fd, send_buffer, sizeof(send_buffer) - 1)) > 0) {
        send_buffer[bytes_read] = '\0';
        // Send to client
        if (send(client_fd, send_buffer, bytes_read, 0) == -1) {
            syslog(LOG_ERR, "Send to client failed: %s", strerror(errno));
            break;
        }
    }
    free(send_buffer);
    return 0;
}

int main(int argc, char **argv)
{
    struct addrinfo inputs, *server_info;
    int socket_fd,client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    int file_fd,status;
    int  yes=1;
    char client_ip[INET_ADDRSTRLEN];
    bool daemon_mode = false;

    //Check if the application to be run in daemon mode
    if((argc >=2)&&(strcmp(argv[1],"-d")==0))
    {
        daemon_mode = true;
    }


    // Open a system logger connection for aesdsocket utility
    openlog("aesdsocket", LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

    /*Line  was partly referred from https://beej.us/guide/bgnet/html/#socket */
    memset(&inputs,0,sizeof(inputs));
    inputs.ai_family = AF_INET;     // don't care IPv4 or IPv6
    inputs.ai_socktype = SOCK_STREAM; // TCP stream sockets
    inputs.ai_flags = AI_PASSIVE;     // fill in my IP for me

    //Get address info
    if((status = getaddrinfo(NULL,"9000",&inputs,&server_info)) !=0)
    {
         syslog(LOG_ERR, "Error occured while getting the address info: %s \n", gai_strerror(status));
         //Freeing of  the linked-list is not done as no memory is allocated
         closelog(); //Close syslog
         exit(1);
    }


    // Open a stream socket
    socket_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_fd == -1) {
        syslog(LOG_ERR, "Error occurred while creating a socket: %s\n", strerror(errno));
        freeaddrinfo(server_info); // free the linked-list
        closelog(); //Close syslog
        exit(1);
    }
    // Set socket options

    if (setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) 
    {
        //Set Socket operation has failed, log the details,
        syslog(LOG_ERR, "Error occured while setting a socket option: %s \n", strerror(errno));
        freeaddrinfo(server_info); // free the linked-list
        closelog(); //Close syslog
        exit(1);
    } 

    if(bind(socket_fd,server_info->ai_addr,server_info->ai_addrlen)==-1)
    {
       //Bind operation has failed, log the details,
        syslog(LOG_ERR, "Error occured while binding a socket: %s \n", strerror(errno));
        freeaddrinfo(server_info); // free the linked-list
        closelog(); //Close syslog
        exit(1); 
    }

    //Check if daemon to be created
    if(daemon_mode)
    {
        if(!create_daemon())
        {
            syslog(LOG_ERR, "Daemon creation failed, hence exiting");
            freeaddrinfo(server_info); // free the linked-list
            closelog(); //Close syslog
            exit(1); 
        }
    }

     if(listen(socket_fd,20)==-1)
     {
        //listen operation has failed, log the details,
        syslog(LOG_ERR, "Error occured during listen operation: %s \n", strerror(errno));
        freeaddrinfo(server_info); // free the linked-list
        closelog(); //Close syslog
        exit(1); 
     }


    file_fd = open(SOCKETDATA_FILE, O_RDWR |O_CREAT | O_TRUNC,0666);
    if (file_fd == -1) 
    {
        syslog(LOG_ERR, "Open/create of /var/tmp/aesdsocketdata failed");
        freeaddrinfo(server_info); // free the linked-list
        closelog(); //Close syslog
        exit(1);
    }
    initialize_sigaction();
    client_addr_size = sizeof(client_addr); // Initialize client address size
     while(!exit_main_loop)
     {
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        if(client_fd==-1)
        {
            //accept operation has failed, log the details and continue for next connection
            syslog(LOG_ERR, "Error occured during accept operation: %s \n", strerror(errno)); 
            continue;
        }

        //Convert binary IP address from binary to human readable format
        inet_ntop(AF_INET, &(((struct sockaddr_in *)&client_addr)->sin_addr), client_ip, sizeof(client_ip));
        // Log the client ip
        syslog(LOG_INFO, "Accepted connection from %s",client_ip);


        //Receive packets from the client and store in SOCKETDATA_FILE
        if (receive_and_store_socket_data(client_fd,file_fd) == 0) 
        {
            //Send back the stored data of file back to the client
           return_socketdata_to_client(client_fd,file_fd);
        }
       // receive_and_store_socket_data(client_fd,file_fd);
        if(close(client_fd)==0)
        {
            syslog(LOG_INFO, "Closed connection from %s",client_ip);
        }
        else
        {
            syslog(LOG_ERR, "Closing of connection from %s failed",client_ip);
        }
        

     }
    close(file_fd);
    freeaddrinfo(server_info); // free the linked-list
    closelog(); //Close syslog
}
