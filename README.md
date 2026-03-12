# GPU Compute Research

An individual research project and open knowledge base focused on enabling and analyzing **GPU (Adreno 616)** and **DSP (Hexagon 685)** compute capabilities within an Android Chroot environment.

## 📱 Research Target
- **SoC**: Qualcomm Snapdragon 712 (SDM712)
- **GPU**: Adreno 616v1
- **DSP**: Hexagon 685

## 🧪 Tested On
- **Device**: Realme 5 Pro (RMX1971)
- **OS Environment**: Android Host (AOSP-based) with Ubuntu 26.04 (Resolute) Chroot

## 🎯 Project Goals
This research explores the boundaries of hardware acceleration in isolated environments, specifically:
- Mapping the **KGSL (Kernel Graphics Support Linux)** ioctl interface.
- Analyzing the dependency of Qualcomm drivers on the **Android Framework** (Binder, HAL, SurfaceFlinger).
- Testing **OpenCL** and **FastRPC** (DSP) compute throughput.
- Documenting the "Null Mode" silent failure in Adreno drivers.

## 🧬 Project Structure
The research is organized into progressive stages:
- **Stage 1**: Environment setup, hardware analysis, and initial failure discovery.
- **Stage 2**: GPU compute success and exhaustive IOCTL scanning.
- **Stage 3**: Deep dive GPU & DSP compute with universal IOCTL scanning.
- **Stage 4**: Advanced DSP interrogation & GPU verification.

## 📅 TODO / Future Stages
- **Stage 5**: Final checks, performance benchmarks, and mmap optimization.

## 🚀 Key Findings
- **Driver Independence**: While libraries (`libOpenCL.so`) load via APEX linker, hardware execution requires a valid Binder context from the Android side.
- **Null Mode**: The Adreno driver enters a "silent success" mode where commands are accepted but never executed by the hardware if framework services are missing.
- **FastRPC Limitation**: DSP attachment fails with `EINVAL` due to missing service discovery via ServiceManager.

## 🛠️ Tools & Scripts
Included in this repository are several diagnostic and testing tools:
- `scripts/diagnose.sh`: Comprehensive system check for GPU/DSP nodes and libraries.
- `scripts/run_benchmark.sh`: Matrix multiplication benchmark for CPU vs GPU analysis.
- `stage1/tools/src/opencl_test.c`: Low-level OpenCL test using direct `dlopen`.

## 📖 How to Use
1. **Setup Chroot**: Follow the instructions in `stage1/01-PERSIAPAN-ENVIRONMENT.md`.
2. **Run Diagnostics**: Execute `./scripts/diagnose.sh` to check your hardware availability.
3. **Explore Stages**: Each directory contains detailed research notes and corresponding source code.

## 🤝 Open Knowledge
This project is an **individual research effort** dedicated to the community. All findings, failures, and workarounds are documented to help other developers and researchers working on mobile hardware acceleration.

## 📜 License
This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---
**Researcher**: [HanSoBored](https://github.com/HanSoBored)
**Date**: March 2026
