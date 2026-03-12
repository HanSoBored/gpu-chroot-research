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
    
    printf("Scanning ALL IOCTLs _IO(0..255, 0..255)...\n");
    for (int t = 0; t < 256; t++) {
        for (int i = 0; i < 256; i++) {
            unsigned long cmd = _IO(t, i);
            int ret = ioctl(fd, cmd);
            if (errno != ENOTTY) {
                printf("Found valid-looking IOCTL: 0x%08lX (type=%d, nr=%d), ret=%d, errno=%d\n", cmd, t, i, ret, errno);
            }
        }
    }
    
    close(fd);
    return 0;
}
