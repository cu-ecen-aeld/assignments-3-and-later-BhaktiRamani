

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stddef.h>


#define IOCTL_CMD_1 0x1234  // Replace with your actual ioctl command
#define IOCTL_CMD_2 0x5678  // Example of another ioctl command

int main() {
    int fd = open("/dev/aesdchar", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    printf("Sending IOCTL command 0x1234...\n");
    if (ioctl(fd, IOCTL_CMD_1) == -1) {
        perror("IOCTL_CMD_1 failed");
    } else {
        printf("IOCTL_CMD_1 successful\n");
    }

    printf("Sending IOCTL command 0x5678...\n");
    if (ioctl(fd, IOCTL_CMD_2) == -1) {
        perror("IOCTL_CMD_2 failed");
    } else {
        printf("IOCTL_CMD_2 successful\n");
    }

    close(fd);
    return EXIT_SUCCESS;
}
