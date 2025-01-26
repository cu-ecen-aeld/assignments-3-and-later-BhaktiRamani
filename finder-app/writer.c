#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

int read_write(const char *file_path, const char *file_content, int num_of_input);

int main(int argc, char *argv[])
{
	printf("File Read Write program\n");
	
	const char *file_path = argv[1];
	const char *file_content = argv[2];
	int num_of_input = argc;
	

	read_write(file_path, file_content, num_of_input);
	

}

int read_write(const char *file_path, const char *file_content, int num_of_input)
{
	//Opening syslog functionality
	openlog("writer", LOG_PID, LOG_USER);
	
	if( num_of_input != 3)
	{
		printf("Wrong Input\n");
		syslog(LOG_ERR, "Wrong Input\n");
		return 1;		
	}
	
	// Abstracting the file name from a path
	const char *file_name;
	file_name = strrchr(file_path, '/');
	if(file_name)
	{
		file_name++;
	}
	
	// Opening the file with read write and execute permission
	int fd;
	
	fd = open(file_path, O_RDWR | O_CREAT | O_APPEND | S_IRWXG);
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

