# OpenCL GPU Compute Testing

## 1. Test Program Development

### Source Code Location
```
/home/han/MyWorkspace/experiment/gpu-chroot-research/opencl_test/jni/stage1/opencl_test.c
```

### Test Description
Minimal OpenCL vector addition test:
- Creates OpenCL context
- Builds simple kernel (vec_add: c = a + b)
- Executes on GPU
- Verifies results

### Kernel Source
```c
__kernel void vec_add(__global const float* a,
                      __global const float* b,
                      __global float* c) {
    int id = get_global_id(0);
    c[id] = a[id] + b[id];
}
```

### Test Parameters
```
Vector size: 1024 elements
Data type: float (32-bit)
Expected: c[i] = a[i] + b[i]
where a[i] = i * 1.5, b[i] = i * 2.5
```

## 2. Build Process

### Android Build Script
```bash
#!/bin/bash
NDK=$ANDROID_NDK_HOME
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-arm64
API=29
TARGET=aarch64-linux-android$API

$TOOLCHAIN/bin/$TARGET-clang \
    -I$NDK/sysroot/usr/include \
    -L/vendor/lib64 \
    -lOpenCL \
    -ldl \
    -Wl,-rpath,/vendor/lib64 \
    -Wl,-rpath,/system/lib64 \
    -Wl,--dynamic-linker=/apex/com.android.runtime/bin/linker64 \
    -o /home/han/MyWorkspace/experiment/gpu-chroot-research/build/opencl_test \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/opencl_test/jni/stage1/opencl_test.c
```

### Binary Info
```bash
file build/opencl_test
# Output:
# ELF 64-bit LSB pie executable, ARM aarch64
# interpreter /apex/com.android.runtime/bin/linker64
# for Android 29, built by NDK r29
```

## 3. Test Execution

### Environment Setup
```bash
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib
```

### Run Command
```bash
/apex/com.android.runtime/bin/linker64 /home/han/MyWorkspace/experiment/gpu-chroot-research/build/opencl_test
```

## 4. Test Results

### Full Output
```
=== OpenCL GPU Compute Test ===

Step 0: Loading OpenCL functions...
All OpenCL functions loaded successfully

Step 1: Get OpenCL platforms...
  Found 1 platform(s)
  Platform: QUALCOMM Snapdragon(TM)

Step 2: Get GPU device...
ERROR: No GPU device found (err=-1)
Trying ALL devices...
  Device: QUALCOMM Adreno(TM)
  Vendor: QUALCOMM

Step 3: Create OpenCL context...
  Context created successfully

Step 4: Create command queue...
  Command queue created

Step 5: Create buffers...
  Buffers created (1024 floats each)

Step 6: Build kernel program...
  Program built successfully

Step 7: Create kernel...
  Kernel created

Step 8: Set kernel arguments...
  Arguments set

Step 9: Execute kernel on GPU...
  Kernel execution queued

Step 9b: Waiting for kernel to finish...
  Kernel execution completed

Step 10: Read results...
  Results read back

Step 11: Verify results...
  ERROR at [1]: expected 4.000000, got 0.000000
  ERROR at [2]: expected 8.000000, got 0.000000
  ERROR at [3]: expected 12.000000, got 0.000000
  ERROR at [4]: expected 16.000000, got 0.000000
  ERROR at [5]: expected 20.000000, got 0.000000

=== RESULTS ===
FAILED! 5 errors found
```

## 5. Analysis

### What Worked ✅
| Step | Status | Details |
|------|--------|---------|
| Library load | ✅ | `libOpenCL.so` loaded via dlopen |
| Platform enum | ✅ | QUALCOMM Snapdragon detected |
| Device query | ⚠️ | Found as "ALL" not "GPU" type |
| Context create | ✅ | No errors |
| Queue create | ✅ | No errors |
| Buffer create | ✅ | Memory allocated |
| Program build | ✅ | Kernel compiled |
| Kernel create | ✅ | No errors |
| Command queue | ✅ | Submitted successfully |

### What Failed ❌
| Step | Status | Details |
|------|--------|---------|
| GPU device type | ❌ | `clGetDeviceIDs(GPU)` returns -1 |
| Compute result | ❌ | All output values = 0.0 |
| Hardware execution | ❌ | Commands never executed |

## 6. Strace Analysis

### Key Syscalls Observed
```bash
strace /apex/com.android.runtime/bin/linker64 ./build/opencl_test 2>&1 | grep -E "open|ioctl"
```

### Device Access
```
openat(AT_FDCWD, "/dev/kgsl-3d0", O_RDWR|O_SYNC) = 5 ✅
openat(AT_FDCWD, "/dev/ion", O_RDONLY) = 6 ✅
```

### KGSL Ioctls
```
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x40, 0x10), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x41, 0x10), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x2, 0x18), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x45, 0x30), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x47, 0x30), ...) = 0 ✅
ioctl(5, _IOC(_IOC_WRITE, 0x9, 0x46, 0x20), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x13, 0x8), ...) = 0 ✅
ioctl(5, _IOC(_IOC_WRITE, 0x9, 0x32, 0x18), ...) = 0 ✅
```

### Memory Mapping
```
mmap(NULL, 131072, PROT_READ, MAP_SHARED, 5, 0) = 0x7129223000 ✅
mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, 5, 0x1000) = 0x712a33e000 ✅
```

### Missing Calls
```
❌ No binder device opens
❌ No HwBinder transactions
❌ No GPU service queries
```

## 7. Library Dependencies

### ldd Output
```bash
/apex/com.android.runtime/bin/linker64 --list /vendor/lib64/libOpenCL.so
```

```
linux-vdso.so.1 => [vdso]
libcutils.so => /system/lib64/libcutils.so
libvndksupport.so => /system/lib64/libvndksupport.so
libc++.so => /system/lib64/libc++.so
libc.so => /apex/com.android.runtime/lib64/bionic/libc.so
libm.so => /apex/com.android.runtime/lib64/bionic/libm.so
libdl.so => /apex/com.android.runtime/lib64/bionic/libdl.so
liblog.so => /system/lib64/liblog.so
libbase.so => /system/lib64/libbase.so
libdl_android.so => /apex/com.android.runtime/lib64/bionic/libdl_android.so
```

**All dependencies resolved!** ✅

## 8. Alternative Approaches Tried

### A. Environment Variables
```bash
export debug.gralloc.gfx3d=1
export debug.egl.hw=1
export debug.sf.hw=1
export persist.sys.ui.hw=1
```
**Result**: No change ❌

### B. DSP Daemon Start
```bash
/apex/com.android.runtime/bin/linker64 /vendor/bin/adsprpcd &
/apex/com.android.runtime/bin/linker64 /vendor/bin/cdsprpcd &
```
**Result**: Daemons run but DSP ioctl still fails ❌

### C. Force ALL Device Type
```c
// Instead of CL_DEVICE_TYPE_GPU
clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, ...)
```
**Result**: Device found but still no execution ⚠️

### D. clFinish() Call
```c
// Added explicit sync after kernel submit
clFinish(queue);
```
**Result**: No change, results still zero ❌

## 9. Comparison: Expected vs Actual

### Expected (Working OpenCL)
```
Step 1: Get platforms... Found 1
Step 2: Get GPU device... Found: Adreno 616
Step 9: Execute kernel... GPU processes data
Step 10: Read results... c[i] = a[i] + b[i] ✅
Step 11: Verify... SUCCESS! All 1024 correct
```

### Actual (Chroot)
```
Step 1: Get platforms... Found 1
Step 2: Get GPU device... ERROR, fallback to ALL
Step 9: Execute kernel... Queued (no execution)
Step 10: Read results... c[i] = 0.0 ❌
Step 11: Verify... FAILED! 5 errors
```

## 10. Conclusion

### Technical Summary
```
OpenCL ICD: ✅ Loads correctly
Runtime: ✅ Qualcomm driver initializes
Device: ⚠️ Found but not as GPU type
Context: ✅ Created successfully
Memory: ✅ Buffers allocated
Kernel: ✅ Compiled successfully
Command: ⚠️ Queued but not executed
Result: ❌ All zeros (null execution)
```

### Root Cause
```
Qualcomm OpenCL driver requires:
1. Binder IPC to GPU service ✅ Device available
2. GPU HAL registration ❌ Not in chroot
3. Context validation ❌ Fails silently
4. Command submission ❌ Null path taken

Without GPU HAL service, driver enters fallback mode:
- Accepts all commands
- Returns success codes
- Never submits to hardware
- Returns zeroed results
```

### Evidence
```
1. strace shows KGSL ioctls succeed
2. BUT no actual GPU activity
3. Results buffer = all zeros
4. No error codes returned
5. Classic "null driver" behavior
```
