#!/bin/bash
# GPU Chroot Diagnostic Script
# Checks all GPU/DSP related components

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "GPU Chroot Environment Diagnosis"
echo "========================================"
echo ""

# Function to check device
check_device() {
    local device=$1
    local expected_perm=$2
    
    if [ -e "$device" ]; then
        local perm=$(ls -la "$device" | awk '{print $1}')
        if [[ "$perm" == *"rw-rw-rw"* ]]; then
            echo -e "${GREEN}✓${NC} $device: $perm (OK)"
            return 0
        else
            echo -e "${YELLOW}⚠${NC} $device: $perm (not rw-rw-rw)"
            return 1
        fi
    else
        echo -e "${RED}✗${NC} $device: NOT FOUND"
        return 1
    fi
}

# Function to check library
check_library() {
    local lib=$1
    if [ -f "$lib" ]; then
        echo -e "${GREEN}✓${NC} $lib: $(ls -lh $lib | awk '{print $5}')"
        return 0
    else
        echo -e "${RED}✗${NC} $lib: NOT FOUND"
        return 1
    fi
}

echo "1. DEVICE NODES"
echo "----------------------------------------"
check_device "/dev/kgsl-3d0"
check_device "/dev/ion"
check_device "/dev/binder"
check_device "/dev/adsprpc-smd"
check_device "/dev/adsprpc-smd-secure"
check_device "/dev/dri/renderD128"
echo ""

echo "2. GPU INFO"
echo "----------------------------------------"
if [ -f /sys/class/kgsl/kgsl-3d0/gpu_model ]; then
    echo -e "${GREEN}✓${NC} GPU Model: $(cat /sys/class/kgsl/kgsl-3d0/gpu_model)"
else
    echo -e "${RED}✗${NC} GPU Model: NOT FOUND"
fi

if [ -f /sys/class/kgsl/kgsl-3d0/gpuclk ]; then
    echo -e "${GREEN}✓${NC} GPU Clock: $(cat /sys/class/kgsl/kgsl-3d0/gpuclk) Hz"
else
    echo -e "${YELLOW}⚠${NC} GPU Clock: NOT AVAILABLE"
fi
echo ""

echo "3. ANDROID LIBRARIES"
echo "----------------------------------------"
check_library "/vendor/lib64/libOpenCL.so"
check_library "/vendor/lib64/libEGL_adreno.so"
check_library "/vendor/lib64/libGLESv2_adreno.so"
check_library "/vendor/lib64/hw/vulkan.adreno.so"
check_library "/vendor/lib64/libsnpeml.so"
check_library "/vendor/lib64/libhexagon_nn_stub.so"
echo ""

echo "4. APEX RUNTIME"
echo "----------------------------------------"
if [ -x /apex/com.android.runtime/bin/linker64 ]; then
    echo -e "${GREEN}✓${NC} Android linker64: EXISTS"
else
    echo -e "${RED}✗${NC} Android linker64: NOT FOUND"
fi

if [ -d /apex/com.android.runtime/lib64/bionic ]; then
    echo -e "${GREEN}✓${NC} Bionic libraries: EXISTS"
else
    echo -e "${RED}✗${NC} Bionic libraries: NOT FOUND"
fi
echo ""

echo "5. OPENCL CONFIG"
echo "----------------------------------------"
if [ -d /etc/OpenCL/vendors ]; then
    echo -e "${GREEN}✓${NC} OpenCL vendors dir: EXISTS"
    echo "  ICD files:"
    for icd in /etc/OpenCL/vendors/*.icd; do
        if [ -f "$icd" ]; then
            echo "    - $(basename $icd): $(cat $icd)"
        fi
    done
else
    echo -e "${RED}✗${NC} OpenCL vendors dir: NOT FOUND"
fi
echo ""

echo "6. VULKAN CONFIG"
echo "----------------------------------------"
if [ -d /usr/share/vulkan/icd.d ]; then
    echo -e "${GREEN}✓${NC} Vulkan ICD dir: EXISTS"
    echo "  ICD files:"
    for icd in /usr/share/vulkan/icd.d/*.json; do
        if [ -f "$icd" ]; then
            echo "    - $(basename $icd)"
        fi
    done
else
    echo -e "${RED}✗${NC} Vulkan ICD dir: NOT FOUND"
fi
echo ""

echo "7. ENVIRONMENT"
echo "----------------------------------------"
echo "ANDROID_ROOT=$ANDROID_ROOT"
echo "ANDROID_DATA=$ANDROID_DATA"
echo "ANDROID_RUNTIME_ROOT=$ANDROID_RUNTIME_ROOT"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo ""

echo "8. QUICK TESTS"
echo "----------------------------------------"

# Test clinfo
if command -v clinfo &> /dev/null; then
    platforms=$(clinfo 2>/dev/null | grep "Platform Name" | wc -l)
    if [ "$platforms" -gt 0 ]; then
        echo -e "${GREEN}✓${NC} clinfo: $platforms platform(s) found"
    else
        echo -e "${YELLOW}⚠${NC} clinfo: No platforms (might need ICD config)"
    fi
else
    echo -e "${YELLOW}⚠${NC} clinfo: NOT INSTALLED"
fi

# Test vulkaninfo
if command -v vulkaninfo &> /dev/null; then
    if vulkaninfo --summary 2>/dev/null | grep -q "GPU"; then
        echo -e "${GREEN}✓${NC} vulkaninfo: GPU found"
    else
        echo -e "${YELLOW}⚠${NC} vulkaninfo: No GPU (using llvmpipe?)"
    fi
else
    echo -e "${YELLOW}⚠${NC} vulkaninfo: NOT INSTALLED"
fi

echo ""
echo "========================================"
echo "Diagnosis Complete"
echo "========================================"
echo ""
echo "Summary:"
echo "- Green (✓) = Working"
echo "- Yellow (⚠) = Warning/Optional"
echo "- Red (✗) = Missing/Broken"
echo ""
echo "For GPU compute to work, you need:"
echo "  ✓ All device nodes with rw-rw-rw permissions"
echo "  ✓ Qualcomm libraries in /vendor/lib64/"
echo "  ✓ APEX runtime available"
echo "  ✓ Proper ICD configuration"
echo ""
echo "NOTE: Even with all above, GPU compute may"
echo "      still fail due to missing Android"
echo "      framework services (Binder, HAL, etc)"
