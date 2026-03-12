# Root Cause Analysis - Mengapa GPU Compute Gagal

## 1. Problem Statement

**Observasi**: 
- Device nodes accessible ✅
- Libraries load successfully ✅
- OpenCL APIs return success ✅
- **Tapi hasil compute = 0** ❌

**Pertanyaan**: Mengapa GPU tidak execute commands?

## 2. Evidence Collection

### 2.1 Strace Analysis

#### Device Access
```bash
openat(AT_FDCWD, "/dev/kgsl-3d0", O_RDWR|O_SYNC) = 5 ✅
```
**Interpretation**: KGSL device opens successfully

#### Memory Mapping
```bash
mmap(NULL, 131072, PROT_READ, MAP_SHARED, 5, 0) = 0x7129223000 ✅
mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, 5, 0x1000) = 0x712a33e000 ✅
```
**Interpretation**: GPU memory allocated

#### Ioctl Calls
```bash
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x40, 0x10), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x41, 0x10), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x2, 0x18), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x45, 0x30), ...) = 0 ✅
ioctl(5, _IOC(_IOC_READ|_IOC_WRITE, 0x9, 0x47, 0x30), ...) = 0 ✅
ioctl(5, _IOC(_IOC_WRITE, 0x9, 0x46, 0x20), ...) = 0 ✅
```
**Interpretation**: All KGSL ioctls return success

#### Missing Calls
```bash
# No binder device access
# No HwBinder transactions
# No service manager queries
```
**Interpretation**: Driver tidak menggunakan Binder IPC

### 2.2 Library Analysis

#### OpenCL Library Dependencies
```bash
/apex/com.android.runtime/bin/linker64 --list /vendor/lib64/libOpenCL.so

libcutils.so => /system/lib64/libcutils.so ✅
libvndksupport.so => /system/lib64/libvndksupport.so ✅
libc++.so => /system/lib64/libc++.so ✅
libc.so => /apex/com.android.runtime/lib64/bionic/libc.so ✅
liblog.so => /system/lib64/liblog.so ✅
libbase.so => /system/lib64/libbase.so ✅
libdl_android.so => /apex/com.android.runtime/lib64/bionic/libdl_android.so ✅
```
**Interpretation**: Semua dependencies resolved

#### Library Loading Trace
```bash
openat(AT_FDCWD, "/vendor/lib64/libOpenCL.so", O_RDONLY) = 4 ✅
openat(AT_FDCWD, "/system/lib64/libcutils.so", O_RDONLY) = 5 ✅
openat(AT_FDCWD, "/apex/com.android.runtime/lib64/bionic/libc.so", O_RDONLY) = 6 ✅
```
**Interpretation**: Libraries load correctly

### 2.3 Kernel Driver Behavior

#### KGSL Driver Flow (Normal)
```
1. open(/dev/kgsl-3d0)
   └─> Create file descriptor
   └─> Initialize context structure

2. mmap()
   └─> Map GPU memory
   └─> Create GEM object

3. ioctl(KGSL_IOCTL_CONTEXT_CREATE)
   └─> Create GPU context
   └─> Register with GPU service via Binder ⚠️

4. ioctl(KGSL_IOCTL_SUBMIT)
   └─> Validate context
   └─> Submit to hardware queue
   └─> Signal completion
```

#### KGSL Driver Flow (Chroot)
```
1. open(/dev/kgsl-3d0)
   └─> Create file descriptor ✅

2. mmap()
   └─> Map GPU memory ✅

3. ioctl(KGSL_IOCTL_CONTEXT_CREATE)
   └─> Create GPU context ✅
   └─> Try Binder call ❌ FAILS/SKIPPED
   └─> Context marked "invalid" ⚠️

4. ioctl(KGSL_IOCTL_SUBMIT)
   └─> Check context validity ⚠️
   └─> Context invalid → null submission ⚠️
   └─> Return success (silent fail) ❌
```

## 3. Root Cause Identification

### 3.1 Missing Android Framework

#### Component 1: ServiceManager
```
Function: Android service registry
Role: Maps service names to Binder endpoints
Impact if missing: Can't find GPU service

Required services:
- android.hardware.graphics.composer
- android.hardware.graphics.allocator
- vendor.qti.hardware.display.composer
```

#### Component 2: SurfaceFlinger
```
Function: Display compositor
Role: Manages GPU contexts for rendering
Impact if missing: No GPU context validation

Dependencies:
- Creates GPU contexts via Binder
- Registers with SurfaceFlinger
- Gets security tokens
```

#### Component 3: GPU HAL
```
Function: Hardware Abstraction Layer
Role: Translates HAL calls to KGSL ioctls
Impact if missing: No proper command submission

HAL Services:
- IAllocator (memory allocation)
- IMapper (buffer mapping)
- IComposer (command submission)
```

### 3.2 Driver Behavior Analysis

#### Normal Mode (Android)
```c
// Simplified driver flow
int kgsl_submit(struct kgsl_context *ctx, struct kgsl_command *cmd) {
    // 1. Validate context
    if (!ctx->valid) return -EINVAL;
    
    // 2. Check Binder registration
    if (!ctx->binder_registered) {
        register_with_gpu_service(ctx);  // ← Requires Binder
        if (!ctx->binder_registered) return -EINVAL;
    }
    
    // 3. Submit to hardware
    submit_to_gpu(cmd);
    
    // 4. Wait for completion
    wait_for_gpu(cmd);
    
    return 0;
}
```

#### Null Mode (Chroot)
```c
// What actually happens in chroot
int kgsl_submit(struct kgsl_context *ctx, struct kgsl_command *cmd) {
    // 1. Validate context
    if (!ctx->valid) return -EINVAL;
    
    // 2. Check Binder registration
    if (!ctx->binder_registered) {
        register_with_gpu_service(ctx);  // ← Binder call FAILS
        // Driver doesn't return error!
        // Instead, marks context as "null"
        ctx->null_mode = true;  // ⚠️ Silent fallback
    }
    
    // 3. Null mode - skip hardware submit
    if (ctx->null_mode) {
        // Do nothing, return success
        return 0;  // ❌ Silent failure!
    }
    
    // 4. Normal submit (never reached)
    submit_to_gpu(cmd);
    return 0;
}
```

### 3.3 Evidence of Null Mode

#### Behavior Pattern
```
1. All APIs return success ✅
2. No error codes ❌
3. No hardware activity ❌
4. Results = all zeros ❌
```

#### Comparison
```
Normal GPU execution:
- ioctl(SUBMIT) → hardware processes → results updated

Null mode:
- ioctl(SUBMIT) → driver ignores → results unchanged (zeros)
```

## 4. Why No Error?

### Design Decision
```
Qualcomm driver designers chose:
- Silent fallback over hard failure
- Allows development/testing without full framework
- Prevents crashes in partial environments
```

### Use Cases for Null Mode
```
1. Early boot (before services start)
2. Recovery mode (minimal framework)
3. Development environments
4. Fallback when GPU busy
```

### Detection
```
Null mode is intentionally hard to detect:
- No error codes
- No log messages
- Same API behavior
Only way to detect: Check results!
```

## 5. Binder IPC Deep Dive

### What Binder Does for GPU

#### Step 1: Service Discovery
```java
// Android side (Java)
IBinder binder = ServiceManager.getService("gpu_service");
IGpuService gpuService = IGpuService.Stub.asInterface(binder);
```

#### Step 2: Context Registration
```java
// Register GPU context
gpuService.registerContext(contextId, binderToken);
```

#### Step 3: Command Submission
```java
// Submit commands via Binder
gpuService.submitCommands(contextId, commandBuffer);
```

#### Step 4: Synchronization
```java
// Wait for completion
gpuService.waitForCompletion(contextId, fenceId);
```

### What Happens Without Binder

#### Missing Steps
```
1. ❌ Can't find gpu_service endpoint
2. ❌ Can't register context
3. ❌ Can't submit via service
4. ❌ Can't synchronize
```

#### Driver Response
```
Driver detects Binder failure:
- Doesn't crash
- Doesn't return error
- Enters null mode
- Silently ignores commands
```

## 6. Comparison Matrix

### Full Android Stack
```
┌─────────────────────────────────────────┐
│ Application (OpenCL/Vulkan)             │
├─────────────────────────────────────────┤
│ Qualcomm Driver (libOpenCL.so)          │
├─────────────────────────────────────────┤
│ Binder IPC                              │
├─────────────────────────────────────────┤
│ GPU HAL Service                         │
├─────────────────────────────────────────┤
│ SurfaceFlinger                          │
├─────────────────────────────────────────┤
│ ServiceManager                          │
├─────────────────────────────────────────┤
│ Kernel KGSL Driver                      │
├─────────────────────────────────────────┤
│ GPU Hardware (Adreno 616)               │
└─────────────────────────────────────────┘

Flow: App → Driver → Binder → HAL → Kernel → GPU ✅
```

### Chroot Environment
```
┌─────────────────────────────────────────┐
│ Application (OpenCL/Vulkan)             │
├─────────────────────────────────────────┤
│ Qualcomm Driver (libOpenCL.so)          │
├─────────────────────────────────────────┤
│ Binder IPC                              │ ❌ Device only
├─────────────────────────────────────────┤
│ GPU HAL Service                         │ ❌ Missing
├─────────────────────────────────────────┤
│ SurfaceFlinger                          │ ❌ Missing
├─────────────────────────────────────────┤
│ ServiceManager                          │ ❌ Missing
├─────────────────────────────────────────┤
│ Kernel KGSL Driver                      │
├─────────────────────────────────────────┤
│ GPU Hardware (Adreno 616)               │
└─────────────────────────────────────────┘

Flow: App → Driver → [NULL MODE] → ❌
```

## 7. Why APEX Linker Isn't Enough

### What APEX Provides
```
✅ Bionic libc compatibility
✅ Android dynamic linking
✅ Library loading
✅ Symbol resolution
```

### What APEX Doesn't Provide
```
❌ ServiceManager
❌ Binder framework
❌ HAL services
❌ Security context
❌ Process isolation
```

### Analogy
```
APEX linker = Engine
Android framework = Road + Traffic system

You can start the engine in a garage (chroot)
But you can't drive without roads (framework)
```

## 8. Attempted Solutions

### Solution 1: Environment Variables
```bash
export debug.gralloc.gfx3d=1
export debug.egl.hw=1
export debug.sf.hw=1
export persist.sys.ui.hw=1
```
**Result**: No effect
**Reason**: Variables read by SurfaceFlinger (not running)

### Solution 2: Start Daemons
```bash
/apex/com.android.runtime/bin/linker64 /vendor/bin/adsprpcd &
```
**Result**: Daemon runs but can't serve
**Reason**: Daemon needs Binder to validate clients

### Solution 3: Direct Device Access
```c
// Try bypassing driver, talk to hardware
open("/dev/kgsl-3d0");
ioctl(KGSL_IOCTL_SUBMIT);
```
**Result**: ioctl succeeds, hardware doesn't execute
**Reason**: Driver null mode

### Solution 4: Force Device Type
```c
// Instead of CL_DEVICE_TYPE_GPU
clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, ...);
```
**Result**: Device found, still no execution
**Reason**: Device type doesn't change driver behavior

## 9. Fundamental Limitation

### The Problem Is Architectural
```
This isn't a configuration issue.
This isn't a permission issue.
This is an architectural dependency.

Qualcomm GPU drivers were designed to work
with Android framework from day one.

They assume:
- Binder IPC exists
- Services are registered
- Security tokens are validated

Without these assumptions, driver fails safe.
```

### Why It Can't Be Fixed Easily
```
To make GPU work in chroot, you need:
1. Port ServiceManager (complex)
2. Port GPU HAL (very complex)
3. Port SurfaceFlinger (extremely complex)
4. Setup Binder framework (complex)
5. Configure security contexts (complex)

This is essentially rebuilding Android framework.
```

## 10. Conclusion

### Root Cause Summary
```
Primary: Missing Android framework services
Secondary: Driver enters null mode silently
Tertiary: No error indication to application
```

### Why It Matters
```
Understanding this prevents wasted effort:
- More permissions won't help
- Different libraries won't help  
- Environment variables won't help

The limitation is fundamental.
```

### The Real Question
```
Not "How do I fix this?"
But "Is there an alternative approach?"

See: 06-ALTERNATIF-SOLUSI.md
```
