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
    
    printf("Scanning IOCTLs with type 'R' (0x52) for different sizes...\n");
    for (int i = 0; i < 256; i++) {
        // Try various sizes and directions
        unsigned long cmd_io = _IO('R', i);
        unsigned long cmd_iow = _IOW('R', i, unsigned int);
        unsigned long cmd_ior = _IOR('R', i, unsigned int);
        unsigned long cmd_iowr = _IOWR('R', i, unsigned int);
        
        if (ioctl(fd, cmd_io) != -1 || errno != ENOTTY) printf("Found (IO): 0x%08lX (nr=%d), errno=%d\n", cmd_io, i, errno);
        if (ioctl(fd, cmd_iow) != -1 || errno != ENOTTY) printf("Found (IOW): 0x%08lX (nr=%d), errno=%d\n", cmd_iow, i, errno);
        if (ioctl(fd, cmd_ior) != -1 || errno != ENOTTY) printf("Found (IOR): 0x%08lX (nr=%d), errno=%d\n", cmd_ior, i, errno);
        if (ioctl(fd, cmd_iowr) != -1 || errno != ENOTTY) printf("Found (IOWR): 0x%08lX (nr=%d), errno=%d\n", cmd_iowr, i, errno);
    }
    
    close(fd);
    return 0;
}
