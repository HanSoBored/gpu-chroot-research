#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

int main() {
    int fd = open("/dev/adsprpc-smd", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    printf("Scanning IOCTLs _IO('r', 0..255)...\n");
    for (int i = 0; i < 256; i++) {
        unsigned long cmd = _IO('r', i);
        int ret = ioctl(fd, cmd);
        if (errno != ENOTTY) {
            printf("Found valid-looking IOCTL: 0x%08lX (nr=%d), ret=%d, errno=%d\n", cmd, i, ret, errno);
        }
    }
    
    printf("\nScanning IOCTLs _IOW('r', 0..255, uint)...\n");
    for (int i = 0; i < 256; i++) {
        unsigned long cmd = _IOW('r', i, unsigned int);
        int ret = ioctl(fd, cmd);
        if (errno != ENOTTY) {
            printf("Found valid-looking IOCTL (IOW): 0x%08lX (nr=%d), ret=%d, errno=%d\n", cmd, i, ret, errno);
        }
    }

    printf("\nScanning IOCTLs _IOR('r', 0..255, uint)...\n");
    for (int i = 0; i < 256; i++) {
        unsigned long cmd = _IOR('r', i, unsigned int);
        int ret = ioctl(fd, cmd);
        if (errno != ENOTTY) {
            printf("Found valid-looking IOCTL (IOR): 0x%08lX (nr=%d), ret=%d, errno=%d\n", cmd, i, ret, errno);
        }
    }

    printf("\nScanning IOCTLs _IOWR('r', 0..255, uint)...\n");
    for (int i = 0; i < 256; i++) {
        unsigned long cmd = _IOWR('r', i, unsigned int);
        int ret = ioctl(fd, cmd);
        if (errno != ENOTTY) {
            printf("Found valid-looking IOCTL (IOWR): 0x%08lX (nr=%d), ret=%d, errno=%d\n", cmd, i, ret, errno);
        }
    }

    close(fd);
    return 0;
}
