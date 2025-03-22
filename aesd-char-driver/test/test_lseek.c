#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define DEVICE_PATH "/dev/testdev"  // Update with actual device name

int main() {
    int fd;
    off_t new_pos;

    // Open the device file
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device file");
        return EXIT_FAILURE;
    }

    printf("Opened device file successfully.\n");

    // Move file offset to 10 using SEEK_SET
    new_pos = lseek(fd, 10, SEEK_SET);
    if (new_pos == (off_t)-1) {
        perror("lseek SEEK_SET failed");
    } else {
        printf("New offset (SEEK_SET to 10): %lld\n", (long long)new_pos);
    }

    // Move file offset by 5 using SEEK_CUR
    new_pos = lseek(fd, 5, SEEK_CUR);
    if (new_pos == (off_t)-1) {
        perror("lseek SEEK_CUR failed");
    } else {
        printf("New offset (SEEK_CUR by 5): %lld\n", (long long)new_pos);
    }

    // Move file offset to -5 using SEEK_END (should fail if size is small)
    new_pos = lseek(fd, -5, SEEK_END);
    if (new_pos == (off_t)-1) {
        perror("lseek SEEK_END failed");
    } else {
        printf("New offset (SEEK_END -5): %lld\n", (long long)new_pos);
    }

    // Close device file
    close(fd);
    printf("Closed device file.\n");

    return EXIT_SUCCESS;
}
