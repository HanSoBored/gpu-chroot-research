# Dokumentasi Lengkap - Stage 2 Research

## 📁 Struktur File Stage 2

```
/home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/
├── 00-README-STAGE2.md            # Executive summary & overview
├── 01-GPU-FIX.md                  # Detail perbaikan GPU OpenCL
├── 02-IOCTL-SCAN-REPORT.md        # Laporan scanning IOCTL DSP
├── 03-BUILD-AND-TEST-RESULTS.md   # Hasil build & test lengkap
├── INDEX.md                       # File ini - index & guide
├── build_stage2_tools.sh          # Build script otomatis
├── tools/src/
│   ├── opencl_test.c              # OpenCL GPU compute test (FIXED)
│   ├── print_ioctl.c              # Print IOCTL constants
│   ├── scan_all_ioctls.c          # Brute-force IOCTL scanner
│   ├── scan_all_iowr_ioctls.c     # IOWR IOCTL scanner
│   ├── scan_ioctl.c               # Targeted 'r' type scanner
│   └── scan_R_ioctls.c            # Type 'R' multi-size scanner
└── build/
    ├── opencl_test                # GPU compute test binary
    ├── print_ioctl                # IOCTL printer binary
    ├── scan_all_ioctls            # All IOCTL scanner binary
    ├── scan_all_iowr_ioctls       # IOWR scanner binary
    ├── scan_ioctl                 # Targeted scanner binary
    └── scan_R_ioctls              # Type R scanner binary
```

---

## 📖 Reading Order

### Untuk Quick Start
1. `00-README-STAGE2.md` - Executive summary
2. `03-BUILD-AND-TEST-RESULTS.md` - Build & run instructions
3. `build_stage2_tools.sh` - Build semua tools

### Untuk Understanding GPU Fix
1. `00-README-STAGE2.md` - Overview
2. `01-GPU-FIX.md` - Detail fix constants
3. `03-BUILD-AND-TEST-RESULTS.md` - Test results

### Untuk Understanding DSP/IOCTL
1. `00-README-STAGE2.md` - Overview
2. `02-IOCTL-SCAN-REPORT.md` - Scanning methodology
3. `03-BUILD-AND-TEST-RESULTS.md` - IOCTL analysis

### Untuk Troubleshooting
1. `03-BUILD-AND-TEST-RESULTS.md` - Build instructions
2. Run tools dan compare output
3. `01-GPU-FIX.md` - Check constants jika GPU fail

---

## 📊 Summary Temuan Stage 2

### Yang Berhasil ✅
| Komponen | Status | Bukti |
|----------|--------|-------|
| GPU OpenCL Compute | ✅ SUCCESS | 1024/1024 akurasi |
| CL_MEM Constants Fix | ✅ IDENTIFIED | `(1 << 2)` dan `(1 << 5)` |
| IOCTL Detection DSP | ✅ WORKS | `0xC0045208` terdeteksi |
| Build System | ✅ COMPLETE | 6 tools built |

### Yang Gagal ❌
| Komponen | Status | Root Cause |
|----------|--------|------------|
| DSP FastRPC | ❌ EPERM | Binder auth required |
| ServiceManager | ❌ MISSING | Chroot limitation |

### Perbaikan dari Stage 1
| Aspek | Stage 1 | Stage 2 |
|-------|---------|---------|
| CL_MEM_READ_ONLY | `(1 << 30)` ❌ | `(1 << 2)` ✅ |
| CL_MEM_COPY_HOST_PTR | `(1 << 30)` ❌ | `(1 << 5)` ✅ |
| GPU Result | All zeros | 100% correct |
| DSP IOCTL | `0x00007202` ❌ | `0xC0045208` ✅ |
| DSP Error | EINVAL | EPERM |

---

## 🔧 Tools yang Tersedia

### GPU Tools

| Tool | Fungsi | Output Expected |
|------|--------|-----------------|
| `opencl_test` | OpenCL vector add | "SUCCESS! All 1024 computations correct!" |

### DSP/IOCTL Tools

| Tool | Fungsi | Output Expected |
|------|--------|-----------------|
| `print_ioctl` | Print IOCTL constants | `0x00007202`, `0x40047203` |
| `scan_all_ioctls` | Brute-force `_IO(0-255, 0-255)` | Terminal IOCTLs |
| `scan_all_iowr_ioctls` | Brute-force `_IOWR(0-255, 0-255)` | `0xC0045208` |
| `scan_ioctl` | Targeted scan type 'r' | (no output) |
| `scan_R_ioctls` | Targeted scan type 'R' | `0xC0045208` |

---

## 🎯 Kesimpulan Stage 2

### GPU Compute - BERHASIL! ✅

**Android Chroot BISA menggunakan GPU Adreno untuk compute workload!**

**Syarat**:
1. `/dev/kgsl-3d0` accessible (chmod 666)
2. `/dev/ion` accessible (chmod 666)
3. Qualcomm OpenCL libraries tersedia
4. Android runtime linker digunakan
5. **OpenCL constants STANDARD** (ini yang penting!)

**Yang TIDAK diperlukan**:
- Binder IPC
- GPU HAL service
- SurfaceFlinger
- Android Framework lengkap

### DSP Compute - TIDAK MUNGKIN ❌

**DSP FastRPC memerlukan Binder infrastructure**

**Alasan**:
1. FastRPC memerlukan sesi terotentikasi
2. Otentikasi via `adsprpcd` dan ServiceManager
3. ServiceManager tidak ada di chroot
4. Tanpa auth = EPERM

---

## 📝 Build Instructions

### Prerequisites
```bash
# NDK harus terinstall
export ANDROID_NDK_HOME=/home/han/Android/android-sdk/ndk/29.0.14206865
```

### Build Semua Tools
```bash
cd /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2
./build_stage2_tools.sh
```

### Output Build
```
=== Building Stage 2 Tools ===
Building opencl_test...
  -> /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/opencl_test
Building print_ioctl...
  -> /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/print_ioctl
Building scan_all_ioctls...
  -> /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_all_ioctls
Building scan_all_iowr_ioctls...
  -> /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_all_iowr_ioctls
Building scan_ioctl...
  -> /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_ioctl
Building scan_R_ioctls...
  -> /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_R_ioctls

=== Build Complete ===
```

---

## 🚀 Test Instructions

### Environment Setup
```bash
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib
```

### Run GPU Test
```bash
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/opencl_test
```

### Run IOCTL Scanners
```bash
# Print constants
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/print_ioctl

# Scan type 'R' (recommended)
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_R_ioctls

# Full scan (hati-hati, bisa lama)
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_all_iowr_ioctls
```

---

## 📋 Checklist Verifikasi

### Build Verification
- [ ] NDK terinstall dan `ANDROID_NDK_HOME` diset
- [ ] `build_stage2_tools.sh` executable
- [ ] Semua 6 binaries ada di `build/`
- [ ] Binary adalah ARM64 ELF executable

### GPU Test Verification
- [ ] Environment variables diset
- [ ] `libOpenCL.so` accessible
- [ ] `/dev/kgsl-3d0` accessible
- [ ] Output: "SUCCESS! All 1024 computations correct!"

### DSP Test Verification
- [ ] `/dev/adsprpc-smd` accessible
- [ ] `scan_R_ioctls` output: `Found (IOWR): 0xC0045208`
- [ ] errno=1 (EPERM) = expected behavior

---

## 🔗 Referensi ke Stage 1

### Dokumentasi Stage 1
```
../stage1/
├── 00-README-UTAMA.md           # Overview keseluruhan
├── 01-PERSIAPAN-ENVIRONMENT.md  # Setup chroot
├── 02-ANALISIS-HARDWARE.md      # Hardware discovery
├── 03-TEST-OPENCL-GPU.md        # OpenCL test (SEBELUM fix)
├── 04-TEST-DSP-HEXAGON.md       # DSP test
├── 05-ANALYSIS-ROOT-CAUSE.md    # Root cause analysis
├── 06-ALTERNATIF-SOLUSI.md      # Alternatif solusi
└── INDEX.md                     # Stage 1 index
```

### Perbandingan Stage 1 vs Stage 2

| Aspek | Stage 1 | Stage 2 |
|-------|---------|---------|
| GPU Compute | ❌ FAIL | ✅ SUCCESS |
| GPU Constants | ❌ Wrong | ✅ Correct |
| DSP IOCTL | ❌ Wrong number | ✅ Correct number |
| DSP Understanding | ⚠️ Limited | ✅ Complete |
| Documentation | ⚠️ Basic | ✅ Comprehensive |

---

## 📞 Kontak

**Research Timeline**: Maret 2026
**Device**: Qualcomm SDM712, Adreno 616
**Environment**: Ubuntu 26.04 chroot on AOSP Android

Untuk questions atau updates, lihat file individual untuk detail masing-masing topik.

---

**Last Updated**: Maret 2026
**Version**: 2.0
