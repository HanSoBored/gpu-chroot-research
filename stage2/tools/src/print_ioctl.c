#include <stdio.h>
#include <sys/ioctl.h>
int main() {
    printf("FASTRPC_IOCTL_INIT_ATTACH: 0x%08lX\n", (unsigned long)_IO('r', 2));
    printf("FASTRPC_IOCTL_INIT_CREATE: 0x%08lX\n", (unsigned long)_IOW('r', 3, unsigned int));
    return 0;
}
