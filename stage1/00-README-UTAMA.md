# GPU Compute Research di Android Chroot Environment

## Ringkasan Eksekutif

Penelitian ini bertujuan untuk memanfaatkan **GPU Adreno 616** pada Qualcomm SDM712 untuk compute workload (bukan graphics) di dalam chroot environment Ubuntu pada Android.

### Device Target
- **SoC**: Qualcomm SDM712 (Snapdragon 712)
- **GPU**: Adreno 616v1 @ 180MHz
- **DSP**: Hexagon 685
- **Android Host**: AOSP-based
- **Chroot**: Ubuntu 26.04 (resolute)

### Kesimpulan Utama

| Komponen | Status | Catatan |
|----------|--------|---------|
| Device Nodes | ✅ ACCESSIBLE | `/dev/kgsl-3d0`, `/dev/ion`, `/dev/binder`, `/dev/adsprpc-smd` |
| Library Loading | ✅ WORKS | Qualcomm drivers load via APEX linker |
| OpenCL Enum | ✅ WORKS | Platform & device terdeteksi |
| GPU Compute | ❌ FAILS | Commands queued tapi hasil = 0 |
| DSP FastRPC | ❌ FAILS | Ioctl EINVAL |

**Root Cause**: Driver Adreno membutuhkan Android framework services (Binder IPC, SurfaceFlinger, GPU HAL) untuk submit command ke hardware.

---

## Struktur Dokumentasi

```
gpu-chroot-research/
├── 00-README-UTAMA.md          # File ini - overview
├── 01-PERSIAPAN-ENVIRONMENT.md # Setup chroot & permissions
├── 02-ANALISIS-HARDWARE.md     # Device discovery & capabilities
├── 03-TEST-OPENCL-GPU.md       # OpenCL GPU compute testing
├── 04-TEST-DSP-HEXAGON.md      # Hexagon DSP testing
├── 05-ANALYSIS-ROOT-CAUSE.md   # Deep dive failure analysis
├── 06-ALTERNATIF-SOLUSI.md     # Workarounds & alternatives
└── INDEX.md                    # Index & reading guide
```

---

## Timeline Penelitian

1. **Initial Discovery** - Cek GPU availability di chroot
2. **Permission Setup** - Mount & chmod device nodes
3. **Library Analysis** - Check Qualcomm driver availability
4. **APEX Discovery** - Find Android runtime linker
5. **OpenCL Testing** - Build & run GPU compute tests
6. **DSP Testing** - Test Hexagon compute
7. **Root Cause Analysis** - Trace why GPU commands fail

---

## File-file Penting

### Device Nodes (di chroot)
```
/dev/kgsl-3d0        crw-rw-rw-  # GPU device
/dev/ion             crw-rw-rw-  # Memory allocator
/dev/binder          crw-rw-rw-  # Android IPC
/dev/adsprpc-smd     crw-rw-rw-  # FastRPC to DSP
/dev/dri/renderD128  crw-rw-rw-  # DRM render node
```

### Libraries (Android)
```
/vendor/lib64/libOpenCL.so           # Qualcomm OpenCL
/vendor/lib64/libEGL_adreno.so       # Adreno EGL
/vendor/lib64/libGLESv2_adreno.so    # Adreno GLES
/vendor/lib64/hw/vulkan.adreno.so    # Adreno Vulkan
/vendor/lib64/libsnpeml.so           # SNPE ML library
/vendor/lib64/libhexagon_nn_stub.so  # Hexagon NN stub
```

### APEX Runtime
```
/apex/com.android.runtime/bin/linker64    # Android linker
/apex/com.android.runtime/lib64/bionic/   # Bionic libc
```

---

## Quick Start

### Test OpenCL GPU
```bash
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib

/apex/com.android.runtime/bin/linker64 /home/han/MyWorkspace/experiment/gpu-chroot-research/build/opencl_test
```

### Test DSP
```bash
# Start DSP daemons first
/apex/com.android.runtime/bin/linker64 /vendor/bin/adsprpcd &
/apex/com.android.runtime/bin/linker64 /vendor/bin/cdsprpcd &

# Run DSP test
/apex/com.android.runtime/bin/linker64 /home/han/MyWorkspace/experiment/gpu-chroot-research/build/dsp_test
```

---

## Kontak & Credits

Research conducted on Android Chroot Environment with:
- Ubuntu 26.04 chroot
- AOSP Android host
- Qualcomm SDM712 device
