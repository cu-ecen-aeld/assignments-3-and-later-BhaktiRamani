// ========================================================================
// Author: Bhakti Ramani
// Date: 2025-01-26
// University: University of Colorado Boulder
// Subject: ECEN 5713 Advanced Embedded Linux Development
//
// File Description:
// Works same as writer.sh file, makes a file in given path and content using
// syscalls and throws messages using syslog
// to run a file normally, type ./writer (PATH) (CONTENT_OF_FILE)
// to check error or messages on system, type sudo cat /var/log/syslog | grep "mesg" 
//
// File Name: writer.c
// ========================================================================

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

int read_write(const char *file_path, const char *file_content);

int main(int argc, char *argv[])
{
	printf("File Read Write program\n");
	
	const char *file_path = argv[1];
	const char *file_content = argv[2];
	int num_of_input = argc;
	
	if( num_of_input != 3)
	{
		printf("Wrong Input\n");
		syslog(LOG_ERR, "Wrong Input\n");
		return 1;		
	}
	
	read_write(file_path, file_content);
	

}

int read_write(const char *file_path, const char *file_content)
{
	//Opening syslog functionality
	openlog("writer", LOG_PID, LOG_USER);
	

	// Abstracting the file name from a path
	const char *file_name;
	file_name = strrchr(file_path, '/');
	if(file_name)
	{
		file_name++;
	}
	
	// Opening the file with read write and execute permission
	int fd;
	
	fd = open(file_path, O_RDWR | O_CREAT | O_APPEND , S_IRWXG);
	if(fd == -1)
	{
		printf("File open error\n");
		syslog(LOG_ERR, "File open error\n");
		return 0;
	}
	
	// Writing contents into a file
	ssize_t wr;
	
	wr = write(fd, file_content, strlen(file_content));
	if(wr == -1)
	{
		printf("Write operation error\n");
		syslog(LOG_ERR, "Write operation error\n");
		return 0;
	}
	
	printf("Write sucssesful\n");
	
	syslog(LOG_DEBUG, "Writing %s to a %s file\n", file_content, file_name);
	
	//Closing syslog
	closelog();
	return 0;
	
	

	
}

