# Analisis Hardware - Qualcomm SDM712

## 1. SoC Overview

**Qualcomm Snapdragon 712 (SDM712)**
```
CPU: 8x Kryo 360 (4x Gold @ 2.3GHz, 4x Silver @ 1.7GHz)
GPU: Adreno 616v1 @ 180MHz
DSP: Hexagon 685
ISP: Spectra 250
Process: 10nm
```

## 2. GPU - Adreno 616

### Device Information
```bash
cat /sys/class/kgsl/kgsl-3d0/gpu_model
# Output: Adreno616v1

cat /sys/class/kgsl/kgsl-3d0/gpuclk
# Output: 180000000 (180 MHz)

cat /sys/class/kgsl/kgsl-3d0/min_pwrlevel
# Output: 5

cat /sys/class/kgsl/kgsl-3d0/max_pwrlevel
# Output: 0
```

### Device Node
```
/dev/kgsl-3d0
Major: 236
Minor: 0
Type: Character device
Permissions: crw-rw-rw-
```

### KGSL Ioctl Interface
```
KGSL = Kernel Graphics Support Linux
Ioctl magic: 0x9
Key ioctls observed:
- 0x9, 0x40: GPU command submit
- 0x9, 0x41: GPU memory mapping
- 0x9, 0x2:  GPU query
- 0x9, 0x45: GPU context create
- 0x9, 0x47: GPU context query
- 0x9, 0x46: GPU sync
- 0x9, 0x13: GPU power control
- 0x9, 0x32: GPU memory alloc
```

## 3. Memory - ION Allocator

### Device Node
```
/dev/ion
Major: 10
Minor: 62
Type: Character device
```

### Function
- Android-specific memory allocator
- Used for GPU buffer sharing
- Supports DMA-BUF export
- Required for zero-copy between CPU/GPU/DSP

## 4. IPC - Binder

### Device Node
```
/dev/binder
Major: 10
Minor: 54
Type: Character device
```

### Function
- Android IPC mechanism
- Used by GPU driver for:
  - Service discovery
  - Context management
  - Command submission coordination

### Observation
Binder device accessible, BUT:
- No ServiceManager available in chroot
- GPU driver can't find GPU service endpoints
- Results in silent command failure

## 5. DSP - Hexagon 685

### Device Nodes
```
/dev/adsprpc-smd        Major: 225, Minor: 0  (Audio DSP)
/dev/adsprpc-smd-secure Major: 225, Minor: 1  (Secure DSP)
```

### DSP Architecture
```
Hexagon 685 DSP
├── ADSP (Audio DSP)    - adsprpc-smd
├── CDSP (Compute DSP)  - cdsprpc-smd (not present as device)
└── MDSP (Modem DSP)    - mdsprpc (not accessible)
```

### FastRPC Interface
```
FastRPC = Fast Remote Procedure Call
Mechanism: Shared memory + RPC
Required for:
- SNPE DSP inference
- Audio processing
- Sensor fusion
- Computer vision
```

### Libraries Available
```
/vendor/lib64/libadsprpc.so           # ADSP RPC client
/vendor/lib64/libcdsprpc.so           # CDSP RPC client
/vendor/lib64/libhexagon_nn_stub.so   # Hexagon NN stub
/vendor/lib64/libsnpeml.so            # SNPE ML library
/vendor/lib64/libsnpe_dsp_domains_v2.so
```

### DSP Files in /vendor
```
/vendor/dsp/adsp/
├── qtc800s_dsp.bin                    # DSP firmware
└── map_*.txt                          # Symbol maps

/vendor/lib/rfsa/adsp/
├── libdspCV_skel.so                   # DSP CV skeleton
├── libfastcvdsp_skel.so               # FastCV skeleton
├── libfastcvadsp.so                   # FastCV library
└── libsnpe_dsp_v65_domains_v2_skel.so # SNPE skeleton
```

## 6. Hardware Capability Summary

| Component | Hardware | Driver Available | Accessible | Functional |
|-----------|----------|------------------|------------|------------|
| GPU (Adreno) | ✅ Adreno 616 | ✅ libOpenCL.so | ✅ /dev/kgsl-3d0 | ❌ No command exec |
| Memory (ION) | ✅ ION | ✅ libion.so | ✅ /dev/ion | ✅ Memory alloc |
| Binder IPC | ✅ Binder | ✅ libbinder.so | ✅ /dev/binder | ⚠️ No services |
| DSP (Hexagon) | ✅ Hexagon 685 | ✅ libadsprpc.so | ✅ /dev/adsprpc-smd | ❌ Ioctl fails |

## 7. Kernel Drivers

### KGSL Driver
```
Driver: msm_kgsl
Location: Kernel module (built-in)
Interface: /dev/kgsl-3d0
Protocol: Ioctl-based command submission
```

### Observed Behavior
```
1. Driver opens successfully
2. Memory mapping works (mmap on /dev/kgsl-3d0)
3. Some ioctls succeed (query, info)
4. Command submit ioctls return success
5. BUT hardware never executes (results = 0)
```

### Root Cause
```
KGSL driver checks for:
1. Valid GPU context (created via Binder)
2. GPU service registration (via ServiceManager)
3. Proper security tokens (from SurfaceFlinger)

Without these, driver enters "null submission" mode:
- Commands accepted
- No hardware execution
- Silent failure
```

## 8. Comparison: Android vs Chroot

| Feature | Android | Chroot |
|---------|---------|--------|
| Device access | ✅ Full | ✅ Full |
| Library loading | ✅ Native | ✅ Via APEX linker |
| Binder IPC | ✅ Full stack | ⚠️ Device only |
| ServiceManager | ✅ Running | ❌ Not available |
| SurfaceFlinger | ✅ Running | ❌ Not available |
| GPU HAL | ✅ Running | ❌ Not available |
| Command submission | ✅ Works | ❌ Null mode |

## 9. Key Finding: APEX Runtime

### Discovery
```bash
# Android runtime tersedia di chroot!
ls -la /apex/com.android.runtime/bin/linker64
# Output: -rwxr-xr-x 2185856 bytes
```

### Significance
- Allows loading Android binaries in chroot
- Provides Bionic libc compatibility
- Enables Qualcomm driver execution
- BUT doesn't provide framework services

### Usage
```bash
/apex/com.android.runtime/bin/linker64 /path/to/android/binary
```

## 10. Hardware Test Results

### GPU Device Test
```bash
# Open /dev/kgsl-3d0
fd = open("/dev/kgsl-3d0", O_RDWR)
# Result: fd = 5 ✅

# Query GPU info
ioctl(fd, KGSL_IOCTL_QUERY, &args)
# Result: Success ✅

# Submit command
ioctl(fd, KGSL_IOCTL_SUBMIT, &cmd)
# Result: Success (but no execution) ⚠️
```

### DSP Device Test
```bash
# Open /dev/adsprpc-smd
fd = open("/dev/adsprpc-smd", O_RDWR)
# Result: fd = 3 ✅

# Attach to DSP
ioctl(fd, FASTRPC_IOCTL_ATTACH)
# Result: EINVAL ❌

# Root cause: Missing DSP service handshake
```
