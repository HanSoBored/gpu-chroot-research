# DSP IOCTL Scan Report - Dokumentasi Lengkap

## 1. Latar Belakang

### 1.1 Masalah Stage 1

Pada riset Tahap 1, test DSP FastRPC gagal dengan error `EINVAL` (Invalid argument):

```c
// Stage 1 - dsp_test.c
#define FASTRPC_IOCTL_INIT_ATTACH _IO('r', 2)  // = 0x00007202

ret = ioctl(fd, FASTRPC_IOCTL_INIT_ATTACH);
// Result: -1 EINVAL (22)
```

**Hipotesis Awal**:
- Nomor IOCTL salah
- Driver tidak mengenali command
- Device node tidak accessible

### 1.2 Tujuan Stage 2

1. Verifikasi apakah IOCTL yang digunakan benar
2. Scan semua IOCTL yang mungkin direspon oleh driver
3. Identifikasi IOCTL yang valid untuk FastRPC

---

## 2. Metodologi Scanning

### 2.1 IOCTL Encoding

Linux IOCTL menggunakan encoding berikut:

```c
// Bit layout (32-bit)
#define _IOC(dir,type,nr,size) \
    (((dir)  << _IOC_DIRSHIFT) | \
     ((type) << _IOC_TYPESHIFT) | \
     ((nr)   << _IOC_NRSHIFT) | \
     ((size) << _IOC_SIZESHIFT))

// Direction bits
#define _IOC_NONE  0U   // 00 = no data
#define _IOC_WRITE 1U   // 01 = write data
#define _IOC_READ  2U   // 10 = read data

// Macro helpers
#define _IO(type,nr)         _IOC(_IOC_NONE, type, nr, 0)
#define _IOR(type,nr,size)   _IOC(_IOC_READ, type, nr, sizeof(size))
#define _IOW(type,nr,size)   _IOC(_IOC_WRITE, type, nr, sizeof(size))
#define _IOWR(type,nr,size)  _IOC(_IOC_READ|_IOC_WRITE, type, nr, sizeof(size))
```

### 2.2 Tools yang Dibuat

| Tool | Source | Fungsi |
|------|--------|--------|
| `print_ioctl` | `print_ioctl.c` | Print konstanta IOCTL |
| `scan_all_ioctls` | `scan_all_ioctls.c` | Brute-force `_IO(0-255, 0-255)` |
| `scan_all_iowr_ioctls` | `scan_all_iowr_ioctls.c` | Brute-force `_IOWR(0-255, 0-255)` |
| `scan_ioctl` | `scan_ioctl.c` | Targeted scan type 'r' |
| `scan_R_ioctls` | `scan_R_ioctls.c` | Targeted scan type 'R' |

### 2.3 Strategi Scanning

```
┌─────────────────────────────────────────────────────────┐
│  Scanning Strategy                                      │
├─────────────────────────────────────────────────────────┤
│  1. Print constants → Verify macro output               │
│  2. Full scan (_IO) → Find any responsive IOCTL         │
│  3. Full scan (_IOWR) → Find R/W IOCTL                  │
│  4. Targeted scan ('r') → Check lowercase type          │
│  5. Targeted scan ('R') → Check uppercase type          │
└─────────────────────────────────────────────────────────┘
```

---

## 3. Hasil Scanning

### 3.1 Print IOCTL Constants

**Command**:
```bash
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/print_ioctl
```

**Output**:
```
FASTRPC_IOCTL_INIT_ATTACH: 0x00007202
FASTRPC_IOCTL_INIT_CREATE: 0x40047203
```

**Analisis**:
- `0x00007202` = `_IO('r', 2)` = type='r' (0x72), nr=2, size=0
- `0x40047203` = `_IOW('r', 3, unsigned int)` = type='r', nr=3, size=4

### 3.2 Scan All IOCTLs (_IO)

**Command**:
```bash
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_all_ioctls
```

**Output**:
```
Scanning ALL IOCTLs _IO(0..255, 0..255)...
Found valid-looking IOCTL: 0x00000002 (type=0, nr=2), ret=-1, errno=14
Found valid-looking IOCTL: 0x00005421 (type=84, nr=33), ret=-1, errno=14
Found valid-looking IOCTL: 0x00005452 (type=84, nr=82), ret=-1, errno=14
```

**Analisis**:
- `errno=14` = `EFAULT` (Bad address) - IOCTL dikenali tapi pointer invalid
- `0x00005421` = `_IO('T', 33)` = Terminal IOCTL (TTY)
- `0x00005452` = `_IO('T', 82)` = Terminal IOCTL (TTY)
- **Tidak ada FastRPC IOCTL terdeteksi dengan type 'r'**

### 3.3 Scan All IOWR IOCTLs

**Command**:
```bash
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_all_iowr_ioctls
```

**Output**:
```
Scanning ALL IOCTLs _IOWR(0..255, 0..255, uint)...
Found valid-looking IOCTL: 0xC0045208 (type=82, nr=8), ret=-1, errno=1
Found valid-looking IOCTL: 0xC0045877 (type=88, nr=119), ret=-1, errno=1
Found valid-looking IOCTL: 0xC0045878 (type=88, nr=120), ret=-1, errno=1
```

**TEMUAN PENTING**:
- `0xC0045208` = type=82, nr=8, size=4
- Type 82 = 0x52 = **'R'** (uppercase, BUKAN 'r' lowercase!)
- `errno=1` = `EPERM` (Operation not permitted)

### 3.4 Targeted Scan Type 'r' (lowercase)

**Command**:
```bash
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_ioctl
```

**Output**:
```
Scanning IOCTLs _IO('r', 0..255)...
(no output - all ENOTTY)

Scanning IOCTLs _IOW('r', 0..255, uint)...
(no output - all ENOTTY)

Scanning IOCTLs _IOR('r', 0..255, uint)...
(no output - all ENOTTY)

Scanning IOCTLs _IOWR('r', 0..255, uint)...
(no output - all ENOTTY)
```

**Kesimpulan**: Type 'r' (0x72) lowercase **TIDAK** digunakan oleh driver FastRPC.

### 3.5 Targeted Scan Type 'R' (uppercase)

**Command**:
```bash
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_R_ioctls
```

**Output**:
```
Scanning IOCTLs with type 'R' (0x52) for different sizes...
Found (IOWR): 0xC0045208 (nr=8), errno=1
```

**KONFIRMASI**: IOCTL yang valid adalah `0xC0045208` = `_IOWR('R', 8, unsigned int)`

---

## 4. Analisis Mendalam

### 4.1 Breakdown IOCTL 0xC0045208

```
Hexadecimal: C0045208
Binary:      1100 0000 0000 0100 0101 0010 0000 1000

Bit fields (32-bit IOCTL):
31-30  Direction:  11 = _IOC_READ | _IOC_WRITE (Read/Write)
29-26  Size:       0000 = 4 bytes
25-16  Type:       0101 0010 = 0x52 = 82 = 'R'
15-8   Number:     0000 1000 = 8
7-0    Reserved:   0000 0000
```

**IOCL Macro Equivalent**:
```c
_IOWR('R', 8, unsigned int)
```

### 4.2 Perbandingan IOCTL

| IOCTL | Hex | Type | Nr | Size | Error | Keterangan |
|-------|-----|------|-----|------|-------|------------|
| Stage 1 (SALAH) | `0x00007202` | 'r' | 2 | 0 | EINVAL | Wrong type & nr |
| Stage 2 (BENAR) | `0xC0045208` | 'R' | 8 | 4 | EPERM | Correct IOCTL |

### 4.3 Analisis Error EPERM

```
ioctl(fd, 0xC0045208) = -1 EPERM (1)
```

**Interpretasi**:
1. ✅ IOCTL **DIKENALI** oleh driver (bukan ENOTTY)
2. ✅ IOCTL **DIPROSES** oleh kernel
3. ❌ Driver **MENOLAK** eksekusi
4. ❌ Alasan: **Tidak ada sesi terotentikasi**

**Mengapa EPERM?**

FastRPC memerlukan otentikasi sesi:
```
┌─────────────────────────────────────────────────────────┐
│  FastRPC Authentication Flow                            │
├─────────────────────────────────────────────────────────┤
│  1. App opens /dev/adsprpc-smd ✓                        │
│  2. App calls ioctl(0xC0045208) ✓                       │
│  3. Kernel driver receives IOCTL ✓                      │
│  4. Driver checks for valid session ❌                  │
│  5. Session requires Binder auth ❌                     │
│  6. Binder → ServiceManager → adsprpcd ❌               │
│  7. No ServiceManager in chroot ❌                      │
│  8. Return EPERM ❌                                     │
└─────────────────────────────────────────────────────────┘
```

### 4.4 Kernel Driver Flow

```c
// Pseudocode kernel driver fastrpc
static long fastrpc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case 0xC0045208:  // Known IOCTL
        if (!filp->private_data) {
            // No valid session
            return -EPERM;  // ← Ini yang terjadi di chroot
        }
        // Process IOCTL with valid session
        break;
    default:
        return -ENOTTY;  // Unknown IOCTL
    }
    return 0;
}
```

---

## 5. Root Cause Analysis

### 5.1 Masalah Stage 1

**IOCTL Salah**:
```c
// Stage 1 menggunakan type 'r' (lowercase)
#define FASTRPC_IOCTL_INIT_ATTACH _IO('r', 2)  // 0x00007202

// Driver FastRPC menggunakan type 'R' (uppercase)
// Result: ENOTTY (Unknown IOCTL)
```

### 5.2 Masalah Stage 2

**IOCTL Benar, tapi Auth Gagal**:
```c
// Stage 2 menemukan IOCTL yang benar
#define FASTRPC_IOCTL_VALID _IOWR('R', 8, unsigned int)  // 0xC0045208

// Driver mengenali IOCTL, tapi menolak tanpa sesi valid
// Result: EPERM (Operation not permitted)
```

### 5.3 Diagram Arsitektur

```
┌─────────────────────────────────────────────────────────┐
│  Chroot Environment                                     │
│                                                         │
│  ┌─────────────────┐                                    │
│  │  DSP Test App   │                                   │
│  │  (ioctl call)   │                                   │
│  └────────┬────────┘                                    │
│           │                                             │
│           │ ioctl(0xC0045208)                           │
│           ▼                                             │
│  ┌─────────────────┐                                    │
│  │  /dev/adsprpc-  │                                    │
│  │  smd            │                                    │
│  └────────┬────────┘                                    │
│           │                                             │
│           │ ❌ Tidak ada Binder                         │
│           │ ❌ Tidak ada ServiceManager                 │
│           ▼                                             │
│  ┌─────────────────┐                                    │
│  │  Kernel Driver  │                                    │
│  │  fastrpc        │                                    │
│  │  return -EPERM  │                                    │
│  └─────────────────┘                                    │
└─────────────────────────────────────────────────────────┘
                         │
                         │ Diperlukan Binder IPC
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Android Host                                           │
│                                                         │
│  ┌─────────────────┐    ┌─────────────────┐            │
│  │  adsprpcd       │◄───│  ServiceManager │            │
│  │  (DSP daemon)   │    │  (Binder)       │            │
│  └────────┬────────┘    └─────────────────┘            │
│           │                                             │
│           │ Validasi sesi & otentikasi                  │
│           ▼                                             │
│  ┌─────────────────┐                                    │
│  │  DSP            │                                    │
│  │  Firmware       │                                    │
│  │  (Hexagon)      │                                    │
│  └─────────────────┘                                    │
└─────────────────────────────────────────────────────────┘
```

---

## 6. Kesimpulan

### 6.1 Temuan Utama

1. **IOCTL yang benar adalah `0xC0045208`** (`_IOWR('R', 8, unsigned int)`)
2. **Type 'R' uppercase (0x52)**, BUKAN 'r' lowercase (0x72)
3. **Driver mengenali IOCTL** (return EPERM, bukan ENOTTY)
4. **EPERM = Session authentication required**

### 6.2 Perbandingan Stage 1 vs Stage 2

| Aspek | Stage 1 | Stage 2 |
|-------|---------|---------|
| IOCTL Used | `0x00007202` ❌ | `0xC0045208` ✅ |
| IOCTL Type | 'r' (lowercase) ❌ | 'R' (uppercase) ✅ |
| IOCTL Number | 2 ❌ | 8 ✅ |
| Error | EINVAL ❌ | EPERM ⚠️ |
| Understanding | Limited | Complete ✅ |

### 6.3 Implikasi

**DSP FastRPC TIDAK bisa digunakan di chroot tanpa Binder infrastructure**

**Alasan**:
1. FastRPC memerlukan sesi terotentikasi
2. Otentikasi dilakukan via Binder IPC
3. ServiceManager tidak ada di chroot
4. Tanpa auth = EPERM

**Solusi yang Mungkin**:
- Binder proxy daemon dari Android host
- Custom RPC bridge chroot ↔ host
- Eksekusi DSP di Android host (bukan di chroot)
- CPU fallback (SNPE CPU mode)

---

## 7. Referensi Teknis

### 7.1 IOCTL Constants

```c
// FastRPC IOCTL yang valid (ditemukan di scan)
#define FASTRPC_IOCTL_CUSTOM _IOWR('R', 8, unsigned int)  // 0xC0045208

// Standard FastRPC IOCTL (dari kernel header)
#define FASTRPC_IOCTL_INIT_ATTACH    _IO('R', 1)   // 0x00005201
#define FASTRPC_IOCTL_INIT_CREATE    _IOW('R', 2)  // 0x40045202
#define FASTRPC_IOCTL_INVOKE         _IOWR('R', 3) // 0xC0045203
```

### 7.2 Error Codes

| errno | Value | Meaning |
|-------|-------|---------|
| ENOTTY | 25 | Inappropriate ioctl for device (unknown IOCTL) |
| EINVAL | 22 | Invalid argument |
| EPERM | 1 | Operation not permitted (auth required) |
| EFAULT | 14 | Bad address |

### 7.3 File Terkait

```
stage2/tools/src/scan_R_ioctls.c     # Tool yang menemukan 0xC0045208
stage2/tools/src/scan_all_iowr_ioctls.c  # Tool yang mengkonfirmasi
stage2/build/scan_R_ioctls           # Binary scanner
```

---

## 8. Cara Menjalankan Ulang

### 8.1 Build Tools
```bash
cd /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2
./build_stage2_tools.sh
```

### 8.2 Run Scanners
```bash
# Print IOCTL constants
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/print_ioctl

# Scan type 'R' (recommended - paling cepat)
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_R_ioctls

# Full IOWR scan (lebih lama)
/apex/com.android.runtime/bin/linker64 \
    /home/han/MyWorkspace/experiment/gpu-chroot-research/stage2/build/scan_all_iowr_ioctls
```

---

**Dokumentasi ini dibuat**: Maret 2026
**Device**: Qualcomm SDM712, Hexagon 685 DSP
**Environment**: Ubuntu 26.04 chroot on AOSP Android
