# Persiapan Environment - Chroot Setup

## 1. Initial State

### Chroot Environment
```
Path: /data/local/rootfs/ubuntu-resolute-26.04
User: han (uid=1000, gid=100)
Host: Android (AOSP-based)
```

### Initial Device Check
```bash
# Di dalam chroot
ls -la /dev/kgsl-3d0
# Output: crw-rw-rw-+ 1 han 1000 236, 0 /dev/kgsl-3d0 ✅

cat /sys/class/kgsl/kgsl-3d0/gpu_model
# Output: Adreno616v1 ✅
```

## 2. Device Permission Issues Found

### Initial State (dari Android host)
```
/dev/kgsl-3d0        crw-rw-rw-  system system   ✅ OK
/dev/ion             crw-rw-r--+ system system   ❌ Read-only for others
/dev/binder          crw-rw-rw-  root root       ✅ OK
/dev/adsprpc-smd     crw-rw-r--  system system   ❌ Read-only for others
```

### Fix Applied (via android-bridge)
```bash
# chmod device nodes di chroot
chmod 666 /data/local/rootfs/ubuntu-resolute-26.04/dev/ion
chmod 666 /data/local/rootfs/ubuntu-resolute-26.04/dev/adsprpc-smd
chmod 666 /data/local/rootfs/ubuntu-resolute-26.04/dev/adsprpc-smd-secure
```

### Final State (di chroot)
```
/dev/kgsl-3d0        crw-rw-rw-  system system   ✅
/dev/ion             crw-rw-rw-  system system   ✅
/dev/binder          crw-rw-rw-  root root       ✅
/dev/adsprpc-smd     crw-rw-rw-  system system   ✅
/dev/adsprpc-smd-secure crw-rw-rw- system system ✅
/dev/dri/renderD128  crw-rw-rw-  root graphics   ✅
```

## 3. Library Path Configuration

### Vulkan ICD (created)
```bash
# File: /usr/share/vulkan/icd.d/adreno_icd.json
{"ICD":{"api_version":"1.3.200","library_path":"/vendor/lib64/hw/vulkan.adreno.so"},"file_format_version":"1.0.1"}
```

### OpenCL ICD (created)
```bash
# File: /etc/OpenCL/vendors/adreno.icd
/vendor/lib64/libOpenCL.so
```

## 4. APEX Mount Discovery

### APEX sudah ter-mount di chroot
```bash
ls -la /apex/
# Output: 69 APEX modules including:
# - /apex/com.android.runtime (ART)
# - /apex/com.android.conscrypt
# - dll
```

### Critical APEX paths
```
/apex/com.android.runtime/bin/linker64      # Android linker 64-bit
/apex/com.android.runtime/lib64/bionic/     # Bionic libraries
  ├── libc.so
  ├── libdl.so
  ├── libdl_android.so
  └── libm.so
```

## 5. Environment Variables Required

```bash
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib
```

## 6. Tools Installed

```bash
# OpenCL tools
apt install clinfo clpeak mesa-utils

# Vulkan tools
apt install vulkan-tools

# Build tools (NDK already available)
$ANDROID_NDK_HOME = /home/han/Android/android-sdk/ndk/29.0.14206865
```

## 7. Verification Commands

```bash
# Check all devices
ls -la /dev/kgsl-3d0 /dev/ion /dev/binder /dev/adsprpc-smd

# Check libraries
ldconfig -p | grep -E "(OpenCL|EGL|GLES)"

# Check APEX
ls -la /apex/com.android.runtime/bin/linker64

# Check GPU model
cat /sys/class/kgsl/kgsl-3d0/gpu_model
```

## 8. Known Issues

### Permission Denied
- `/vendor/bin/` tidak accessible dari chroot (permission denied) (Use Android-Bridge)
- Workaround: Copy binaries atau jalankan dari Android host

### Shell Limitations
- android-bridge device_exec tidak support pipe (`|`) (FIXED)
- android-bridge device_exec tidak support redirect kompleks (FIXED)
- Workaround: Gunakan command sederhana atau jalankan manual di chroot (FIXED)
