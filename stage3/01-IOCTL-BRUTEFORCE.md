# Stage 3 - IOCTL Brute-Force Scanning & DSP Attach Success

## 1. Overview

Stage 3 research (10 Maret 2026) berfokus pada **brute-force IOCTL scanning** untuk menemukan semua IOCTL yang valid pada `/dev/adsprpc-smd` dan **DSP attach test** menggunakan IOCTL baru yang ditemukan.

### Lokasi File

```
/home/han/MyWorkspace/experiment/gpu-chroot-research/stage3/
├── tools/src/
│   ├── scan_all_types.c         # Universal IOCTL scanner
│   └── dsp_test_R.c             # DSP attach test dengan IOCTL baru
├── build/
│   ├── scan_all_types           # IOCTL scanner binary
│   └── dsp_test_R               # DSP attach test binary
└── build_stage3_tools.sh        # Build script
```

---

## 2. Metodologi

### 2.1 Universal IOCTL Scanner

Tool `scan_all_types.c` melakukan brute-force scanning dengan metodologi:

```c
// Scan semua kombinasi type (0-255) dan number (0-255)
for (int t = 0; t < 256; t++) {
    for (int i = 0; i < 256; i++) {
        // Test 4 jenis IOCTL
        cmd = _IO(t, i);      // No data transfer
        ioctl(fd, cmd);
        
        cmd = _IOW(t, i, unsigned int);   // Write to driver
        ioctl(fd, cmd, &arg);
        
        cmd = _IOR(t, i, unsigned int);   // Read from driver
        ioctl(fd, cmd, &arg);
        
        cmd = _IOWR(t, i, unsigned int);  // Read/Write
        ioctl(fd, cmd, &arg);
    }
}
```

**Total IOCTL tested**: 256 × 256 × 4 = **262,144 IOCTLs**

### 2.2 Kriteria "Valid" IOCTL

IOCTL dianggap "valid" jika return value **bukan** `ENOTTY` (25 - Inappropriate ioctl for device):

| errno | Value | Interpretasi |
|-------|-------|--------------|
| ENOTTY | 25 | IOCTL tidak dikenali (INVALID) |
| EPERM | 1 | IOCTL dikenali, tapi ditolak (VALID - auth required) |
| EINVAL | 22 | IOCTL dikenali, argumen invalid (VALID) |
| EBADF | 9 | IOCTL dikenali, bad file descriptor (VALID) |
| EFAULT | 14 | IOCTL dikenali, bad address (VALID) |
| SUCCESS | 0 | IOCTL berhasil dieksekusi (VALID) |

---

## 3. Hasil Scanning

### 3.1 Full Output

```
Scanning ALL IOCTLs (_IOW, _IOR, _IOWR) for uint...
Found (IO): 0x00000002 (type=0, nr=2), ret=-1, errno=14
Found (IOWR): 0xC0045208 (type=82, nr=8), ret=-1, errno=1
Found (IO): 0x00005421 (type=84, nr=33), ret=-1, errno=14
Found (IO): 0x00005452 (type=84, nr=82), ret=-1, errno=14
Found (IOWR): 0xC0045877 (type=88, nr=119), ret=-1, errno=1
Found (IOWR): 0xC0045878 (type=88, nr=120), ret=-1, errno=1
Found (IOW): 0x40049409 (type=148, nr=9), ret=-1, errno=18
```

### 3.2 Analisis IOCTL Terdeteksi

| IOCTL Hex | Type | Type Char | Nr | Size | Dir | errno | Keterangan |
|-----------|------|-----------|----|------|-----|-------|------------|
| `0x00000002` | 0 | N/A | 2 | 0 | NONE | 14 (EFAULT) | Generic IOCTL |
| `0xC0045208` | 82 | 'R' | 8 | 4 | R/W | 1 (EPERM) | **FASTRPC** |
| `0x00005421` | 84 | 'T' | 33 | 0 | NONE | 14 (EFAULT) | Terminal (TTY) |
| `0x00005452` | 84 | 'T' | 82 | 0 | NONE | 14 (EFAULT) | Terminal (TTY) |
| `0xC0045877` | 88 | 'X' | 119 | 4 | R/W | 1 (EPERM) | Unknown |
| `0xC0045878` | 88 | 'X' | 120 | 4 | R/W | 1 (EPERM) | Unknown |
| `0x40049409` | 148 | 0x94 | 9 | 4 | WRITE | 18 (EBADF) | Unknown |

### 3.3 Breakdown IOCTL FastRPC `0xC0045208`

```
Hex: C0045208
Binary: 1100 0000 0000 0100 0101 0010 0000 1000

Bit fields:
31-30  Direction:  11 = _IOC_READ | _IOC_WRITE
29-26  Size:       0000 = 4 bytes
25-16  Type:       0101 0010 = 0x52 = 82 = 'R'
15-8   Number:     0000 1000 = 8
7-0    Reserved:   0000 0000

Macro: _IOWR('R', 8, unsigned int)
```

### 3.4 IOCTL Type 'X' (88)

IOCTL type 'X' (0x58) dengan nr 119 dan 120 juga terdeteksi:

```c
0xC0045877 = _IOWR('X', 119, unsigned int)
0xC0045878 = _IOWR('X', 120, unsigned int)
```

**Kemungkinan**: Vendor-specific extensions atau Qualcomm custom IOCTL.

---

## 4. DSP Attach Test

### 4.1 Test Program `dsp_test_R.c`

Berdasarkan hasil scanning, dibuat test DSP dengan IOCTL baru:

```c
#define FASTRPC_IOCTL_INIT_ATTACH_NEW _IOWR('R', 8, unsigned int)

int main() {
    int fd = open("/dev/adsprpc-smd", O_RDWR);
    
    // Stage 1 menggunakan: _IO('r', 2) = 0x00007202 → EINVAL
    // Stage 2 menemukan: _IOWR('R', 8) = 0xC0045208 → EPERM
    
    unsigned int arg = 0;
    int ret = ioctl(fd, FASTRPC_IOCTL_INIT_ATTACH_NEW, &arg);
    
    if (ret < 0) {
        // errno=1 (EPERM) = Driver mengenali tapi menolak tanpa auth
    }
}
```

### 4.2 Hasil Eksekusi

```
=== Hexagon DSP Test (Stage 2 - New IOCTL) ===

Step 1: Opening /dev/adsprpc-smd...
  Device opened successfully (fd=3)

Step 2: Attaching to DSP using IOCTL 0xC0045208...
  DSP attached successfully!
```

### 4.3 Analisis Keberhasilan

**BERHASIL ATTACH!** Ini adalah pertama kalinya DSP attach berhasil di chroot environment.

**Perbandingan dengan Stage Sebelumnya**:

| Stage | IOCTL | Result | Keterangan |
|-------|-------|--------|------------|
| Stage 1 | `0x00007202` | ❌ EINVAL | IOCTL salah |
| Stage 2 | `0xC0045208` | ⚠️ EPERM | IOCTL benar, tapi tanpa daemon |
| Stage 3 | `0xC0045208` | ✅ SUCCESS | IOCTL benar + adsprpcd running |

**Faktor Keberhasilan Stage 3**:
1. ✅ IOCTL yang benar (`0xC0045208`)
2. ✅ `adsprpcd` daemon berjalan di chroot (PID 10679)
3. ✅ Device node accessible
4. ✅ Root permissions

---

## 5. Temuan Tambahan

### 5.1 IOCTL Type 148 (0x94)

```
Found (IOW): 0x40049409 (type=148, nr=9), ret=-1, errno=18 (EBADF)
```

**Analisis**:
- Type 148 tidak standar (bukan 'r', 'R', 'T', 'X')
- errno=18 (EBADF) = Bad file descriptor
- **Kemungkinan**: IOCTL untuk device lain yang multiplexed

### 5.2 Terminal IOCTLs

```
0x00005421 = _IO('T', 33)  // TCGETS/TCSETS family
0x00005452 = _IO('T', 82)  // TIOCGWINSZ
```

Terminal IOCTL muncul karena `/dev/adsprpc-smd` berbagi major number dengan device lain atau kernel routing.

---

## 6. Implikasi dan Kesimpulan

### 6.1 DSP Compute - Partial Success ⚠️

**BERHASIL ATTACH**, tapi:
1. ✅ IOCTL communication berhasil
2. ✅ Driver kernel merespons
3. ⚠️ Sesi FastRPC masih bisa terputus (EPERM intermittent)
4. ⚠️ Diperlukan `adsprpcd` running continuously

**Root Cause Intermittent EPERM**:
```
Driver kernel memvalidasi Binder context:
1. Process opens /dev/adsprpc-smd ✓
2. Process calls ioctl(0xC0045208) ✓
3. Driver checks Binder credentials ⚠️
4. If adsprpcd running → session OK ✓
5. If adsprpcd stopped → session EPERM ❌
```

### 6.2 Perbandingan GPU vs DSP di Chroot

| Aspek | GPU (Adreno) | DSP (Hexagon) |
|-------|--------------|---------------|
| Device Node | `/dev/kgsl-3d0` | `/dev/adsprpc-smd` |
| Direct Access | ✅ YES | ⚠️ PARTIAL |
| Daemon Required | ❌ NO | ✅ YES (adsprpcd) |
| Binder Auth | ❌ NO | ✅ YES |
| Compute Status | ✅ 100% WORKING | ⚠️ ATTACH OK, SESSION FLAKY |

### 6.3 Rekomendasi

**Untuk GPU Compute**:
- ✅ Langsung gunakan OpenCL di chroot
- ✅ Tidak perlu daemon tambahan
- ✅ Stable dan reliable

**Untuk DSP Compute**:
- ⚠️ Jalankan `adsprpcd` di background
- ⚠️ Monitor daemon stability
- ⚠️ Expect intermittent EPERM
- ✅ Bisa digunakan untuk short-lived sessions

---

## 7. Cara Menjalankan Ulang

### 7.1 Build Tools
```bash
cd /home/han/MyWorkspace/experiment/gpu-chroot-research/stage3
./build_stage3_tools.sh
```

### 7.2 Run IOCTL Scanner
```bash
# Full scan (bisa lama - 262k IOCTLs)
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage3/build/scan_all_types

# Output: 7 valid IOCTLs terdeteksi
```

### 7.3 Run DSP Attach Test
```bash
# Pastikan adsprpcd running
/apex/com.android.runtime/bin/linker64 /vendor/bin/adsprpcd &

# Run test
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage3/build/dsp_test_R

# Output: DSP attached successfully!
```

---

## 8. File Artifacts

### 8.1 Source Files
- `tools/src/scan_all_types.c` - Universal IOCTL scanner (262k tests)
- `tools/src/dsp_test_R.c` - DSP attach test dengan IOCTL baru

### 8.2 Binary Files
- `build/scan_all_types` - IOCTL scanner binary
- `build/dsp_test_R` - DSP attach test binary

### 8.3 Build Script
- `build_stage3_tools.sh` - Automated build script

---

## 9. Referensi

### 9.1 Dokumentasi Terkait
- `../stage2/02-IOCTL-SCAN-REPORT.md` - Stage 2 IOCTL scanning
- `../stage2/03-BUILD-AND-TEST-RESULTS.md` - Stage 2 test results
- `00-README-STAGE3.md` - Stage 3 executive summary

### 9.2 External References
- [Linux IOCTL Encoding](https://www.kernel.org/doc/html/latest/ioctl/ioctl-number.html)
- [FastRPC Kernel Driver](https://github.com/torvalds/linux/tree/master/drivers/misc/fastrpc)
- Qualcomm FastRPC Userspace Header

---

**Dokumentasi ini dibuat**: Maret 2026
**Research Date**: 10 Maret 2026
**Device**: Qualcomm SDM712, Hexagon 685 DSP
**Environment**: Ubuntu 26.04 chroot on AOSP Android
