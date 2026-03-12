#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define FASTRPC_IOCTL_INIT_ATTACH_NEW _IOWR('R', 8, unsigned int)

int main() {
    printf("=== Hexagon DSP Test (Stage 2 - New IOCTL) ===\n\n");
    
    printf("Step 1: Opening /dev/adsprpc-smd...\n");
    int fd = open("/dev/adsprpc-smd", O_RDWR);
    if (fd < 0) {
        perror("  Failed to open adsprpc-smd");
        return 1;
    }
    printf("  Device opened successfully (fd=%d)\n", fd);

    printf("\nStep 2: Attaching to DSP using IOCTL 0x%08lX...\n", (unsigned long)FASTRPC_IOCTL_INIT_ATTACH_NEW);
    unsigned int arg = 0;
    int ret = ioctl(fd, FASTRPC_IOCTL_INIT_ATTACH_NEW, &arg);
    if (ret < 0) {
        printf("ERROR: Failed to attach to DSP: %s (errno=%d)\n", strerror(errno), errno);
        printf("Check if adsprpcd is running on Android host/chroot.\n");
        close(fd);
        return 1;
    }
    printf("  DSP attached successfully!\n");

    close(fd);
    return 0;
}
