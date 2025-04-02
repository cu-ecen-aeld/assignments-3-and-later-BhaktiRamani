// #include <stdio.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <errno.h>
// #include <stdlib.h>

// #define DEVICE_PATH "/dev/testdev"  // Update with actual device name

// int main() {
//     int fd;
//     off_t new_pos;

//     // Open the device file
//     fd = open(DEVICE_PATH, O_RDWR);
//     if (fd < 0) {
//         perror("Failed to open device file");
//         return EXIT_FAILURE;
//     }

//     printf("Opened device file successfully.\n");

//     // Move file offset to 10 using SEEK_SET
//     new_pos = lseek(fd, 10, SEEK_SET);
//     if (new_pos == (off_t)-1) {
//         perror("lseek SEEK_SET failed");
//     } else {
//         printf("New offset (SEEK_SET to 10): %lld\n", (long long)new_pos);
//     }

//     // Move file offset by 5 using SEEK_CUR
//     new_pos = lseek(fd, 5, SEEK_CUR);
//     if (new_pos == (off_t)-1) {
//         perror("lseek SEEK_CUR failed");
//     } else {
//         printf("New offset (SEEK_CUR by 5): %lld\n", (long long)new_pos);
//     }

//     // Move file offset to -5 using SEEK_END (should fail if size is small)
//     new_pos = lseek(fd, -5, SEEK_END);
//     if (new_pos == (off_t)-1) {
//         perror("lseek SEEK_END failed");
//     } else {
//         printf("New offset (SEEK_END -5): %lld\n", (long long)new_pos);
//     }

//     // Close device file
//     close(fd);
//     printf("Closed device file.\n");

//     return EXIT_SUCCESS;
// }
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define AESDCHAR_IOCSEEKTO _IOW('a', 1, struct aesd_seekto)

struct aesd_seekto {
    uint32_t write_cmd;
    uint32_t write_cmd_offset;
};

int main() {
    int fd = open("/dev/aesdchar", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct aesd_seekto seekto;
    seekto.write_cmd = 1;      // Second write entry (0-based index)
    seekto.write_cmd_offset = 3; // Offset within that entry

    if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    char buf[100] = {0};
    read(fd, buf, sizeof(buf) - 1);
    printf("Read after seek: %s\n", buf);

    close(fd);
    return 0;
}
