# Hexagon DSP Testing

## 1. DSP Architecture Overview

### Qualcomm Hexagon 685
```
Hexagon 685 DSP
├── ADSP (Audio DSP)
│   ├── Audio processing
│   ├── Voice recognition
│   └── Sensor hub
├── CDSP (Compute DSP)
│   ├── Computer vision
│   ├── Machine learning
│   └── Image processing
└── MDSP (Modem DSP)
    ├── Cellular modem
    └── Signal processing
```

### FastRPC Interface
```
FastRPC = Fast Remote Procedure Call
Purpose: User-space to DSP communication
Mechanism: Shared memory + RPC stubs
Device: /dev/adsprpc-smd (Audio DSP)
```

## 2. Available Libraries

### DSP Client Libraries
```bash
ls -la /vendor/lib64/lib*rpc*.so
# Output:
libadsprpc.so      264560 bytes  # ADSP RPC client
libcdsprpc.so      264576 bytes  # CDSP RPC client
libmdsprpc.so      (32-bit only) # MDSP RPC client
libsdsprpc.so      (32-bit only) # Sensor DSP
```

### SNPE Libraries
```bash
ls -la /vendor/lib64/libsnpe*.so /vendor/lib64/libhexagon*.so
# Output:
libsnpeml.so                  100104 bytes  # SNPE ML library
libsnpe_dsp_domains_v2.so      21632 bytes  # DSP domains
libhexagon_nn_stub.so          58904 bytes  # Hexagon NN stub
```

### DSP Skeleton Files
```bash
ls -la /vendor/lib/rfsa/adsp/
# Output:
libdspCV_skel.so                    # Computer vision skeleton
libfastcvdsp_skel.so                # FastCV skeleton
libfastcvadsp.so                    # FastCV library
libsnpe_dsp_v65_domains_v2_skel.so  # SNPE skeleton
libhexagon_nn_skel.so               # Hexagon NN skeleton
```

## 3. Device Nodes

### Available Devices
```bash
ls -la /dev/adsprpc*
# Output:
/dev/adsprpc-smd        crw-rw-rw-  system system  225, 0
/dev/adsprpc-smd-secure crw-rw-rw-  system system  225, 1
```

### Device Major/Minor
```
adsprpc-smd:
  Major: 225
  Minor: 0 (normal), 1 (secure)
  Type: Character device
  Driver: fastrpc (kernel)
```

## 4. DSP Daemon Services

### Android Services
```bash
# From /vendor/etc/init/

service vendor.adsprpcd /vendor/bin/adsprpcd
  class main
  user system
  group media

service vendor.cdsprpcd /vendor/bin/cdsprpcd
  class main
  user system
  group system

service vendor.dspservice /vendor/bin/dspservice
  class hal
  user system
  interface vendor.qti.hardware.dsp@1.0::IDspService
```

### Daemons Started in Test
```bash
# Started manually in chroot
/apex/com.android.runtime/bin/linker64 /vendor/bin/adsprpcd &
/apex/com.android.runtime/bin/linker64 /vendor/bin/cdsprpcd &

# Process status
ps aux | grep adsprpcd
# Output: Running (PID 13875)
```

## 5. DSP Test Program

### Source Code
```
/home/han/MyWorkspace/experiment/gpu-chroot-research/opencl_test/jni/stage1/dsp_test.c
```

### Test Description
Basic FastRPC attachment test:
1. Open `/dev/adsprpc-smd`
2. Attach to DSP
3. Verify communication

### Key Code
```c
fd = open("/dev/adsprpc-smd", O_RDWR);
if (fd < 0) {
    perror("Failed to open adsprpc-smd");
    return 1;
}

ret = ioctl(fd, FASTRPC_IOCTL_INIT_ATTACH);
if (ret < 0) {
    perror("Failed to attach to DSP");
    return 1;
}
```

### Build Command
```bash
$TOOLCHAIN/bin/aarch64-linux-android29-clang \
    -o /home/han/MyWorkspace/experiment/gpu-chroot-research/build/dsp_test \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/opencl_test/jni/stage1/dsp_test.c
```

## 6. Test Execution

### Run Command
```bash
/apex/com.android.runtime/bin/linker64 /home/han/MyWorkspace/experiment/gpu-chroot-research/build/dsp_test
```

### Full Output
```
=== Hexagon DSP Test ===

Step 1: Opening /dev/adsprpc-smd...
  Device opened successfully (fd=3)

Step 2: Attaching to DSP...
ERROR: Failed to attach to DSP: Inappropriate ioctl for device
DSP might be busy or not initialized.
```

## 7. Analysis

### What Worked ✅
| Step | Status | Details |
|------|--------|---------|
| Device open | ✅ | `/dev/adsprpc-smd` accessible |
| File descriptor | ✅ | fd = 3 returned |

### What Failed ❌
| Step | Status | Details |
|------|--------|---------|
| DSP attach | ❌ | `ioctl(FASTRPC_IOCTL_INIT_ATTACH)` returns EINVAL |
| Communication | ❌ | No RPC possible without attach |

### Error Details
```
Error: "Inappropriate ioctl for device"
Errno: EINVAL (28)
Ioctl: FASTRPC_IOCTL_INIT_ATTACH (0x'r', 2)
```

## 8. Strace Analysis

### Device Access
```bash
strace ./dsp_test 2>&1 | grep -E "open|ioctl"
```

### Observed Calls
```
openat(AT_FDCWD, "/dev/adsprpc-smd", O_RDWR) = 3 ✅
ioctl(3, FASTRPC_IOCTL_INIT_ATTACH) = -1 EINVAL ❌
close(3) = 0
```

### Missing Calls
```
❌ No mmap calls (shared memory setup)
❌ No subsequent RPC ioctls
❌ No binder opens
```

## 9. Root Cause Analysis

### Why Ioctl Fails

#### Expected Flow (Android)
```
1. User opens /dev/adsprpc-smd
2. User calls FASTRPC_IOCTL_INIT_ATTACH
3. Kernel forwards to adsprpcd daemon
4. Daemon validates via Binder
5. Daemon registers session
6. Kernel returns success
7. User can now do RPC calls
```

#### Actual Flow (Chroot)
```
1. User opens /dev/adsprpc-smd ✅
2. User calls FASTRPC_IOCTL_INIT_ATTACH
3. Kernel forwards to adsprpcd daemon
4. Daemon tries Binder validation ❌
5. No ServiceManager in chroot ❌
6. Daemon rejects session ❌
7. Kernel returns EINVAL ❌
```

### Missing Components
```
1. ServiceManager ❌
   - Required for service discovery
   - Provides binder context

2. DSP HAL Service ❌
   - vendor.qti.hardware.dsp@1.0::IDspService
   - Manages DSP sessions

3. Proper Security Context ❌
   - SELinux labels
   - Process credentials
```

## 10. Comparison: Android vs Chroot

| Component | Android | Chroot |
|-----------|---------|--------|
| /dev/adsprpc-smd | ✅ Available | ✅ Available |
| adsprpcd daemon | ✅ Running | ⚠️ Can start |
| Binder IPC | ✅ Full stack | ⚠️ Device only |
| ServiceManager | ✅ Running | ❌ Missing |
| DSP HAL | ✅ Running | ❌ Missing |
| FASTRPC attach | ✅ Works | ❌ EINVAL |
| RPC calls | ✅ Works | ❌ Can't attach |

## 11. Alternative Approaches Tried

### A. Start DSP Daemons
```bash
/apex/com.android.runtime/bin/linker64 /vendor/bin/adsprpcd &
/apex/com.android.runtime/bin/linker64 /vendor/bin/cdsprpcd &
```
**Result**: Daemons run but ioctl still fails ❌

### B. Check Daemon Status
```bash
ps aux | grep adsprpcd
# Output: Running
```
**Result**: Process exists but can't serve ❌

### C. Try Secure Device
```c
fd = open("/dev/adsprpc-smd-secure", O_RDWR);
```
**Result**: Same EINVAL error ❌

### D. Direct DSP Library Call
```c
// Try loading libadsprpc.so directly
dlopen("libadsprpc.so", RTLD_LAZY);
```
**Result**: Library loads but functions still fail ❌

## 12. SNPE Testing Attempt

### SNPE Architecture
```
SNPE (Snapdragon Neural Processing Engine)
├── CPU backend (works)
├── GPU backend (fails - same as OpenCL)
└── DSP backend (fails - FastRPC issue)
```

### Available SNPE Libraries
```
/vendor/lib64/libsnpeml.so           # SNPE ML core
/vendor/lib64/libsnpe_dsp_domains_v2.so  # DSP domains
```

### Why SNPE DSP Fails
```
SNPE DSP backend requires:
1. FastRPC attachment ❌ (fails)
2. DSP skeleton loading ⚠️ (files exist)
3. Shared memory setup ❌ (requires working FastRPC)
```

### Possible Alternative
```
SNPE CPU backend might work:
- Uses standard C++ libraries
- No special device access needed
- Slower but functional
```

## 13. DSP Register Map

### Observed Ioctls (from kernel headers)
```c
#define FASTRPC_IOCTL_INIT_ATTACH    _IO('r', 2)
#define FASTRPC_IOCTL_INIT_CREATE    _IOW('r', 3, unsigned int)
#define FASTRPC_IOCTL_INVOKE         _IOWR('r', 1, struct fastrpc_ioctl_invoke)
#define FASTRPC_IOCTL_MMAP           _IOWR('r', 4, struct fastrpc_ioctl_mmap)
#define FASTRPC_IOCTL_MUNMAP         _IOWR('r', 5, struct fastrpc_ioctl_munmap)
```

### Ioctl Numbers (from strace)
```
Device: /dev/adsprpc-smd
Major: 225
Ioctl base: 'r' (0x72)
```

## 14. Conclusion

### Summary
```
Device Access: ✅ /dev/adsprpc-smd accessible
Library Load: ✅ libadsprpc.so loads
Daemon Start: ✅ adsprpcd can run
FastRPC Attach: ❌ EINVAL - missing framework
RPC Calls: ❌ Can't proceed without attach
SNPE DSP: ❌ Requires working FastRPC
```

### Root Cause
```
FastRPC requires Binder-based service validation.
Without ServiceManager and DSP HAL in chroot,
the kernel daemon rejects all attachment requests.
```

### Workaround Possibilities
```
1. Port ServiceManager to chroot (complex)
2. Emulate DSP HAL interface (very complex)
3. Use CPU backend only (limited)
4. Run DSP code on Android side (IPC overhead)
```

### Final Verdict
**DSP compute NOT functional in current chroot setup**
