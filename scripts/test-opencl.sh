#!/bin/bash
# OpenCL GPU Test Runner
# Tests OpenCL compute on Adreno GPU

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "OpenCL GPU Compute Test"
echo "========================================"
echo ""

# Setup environment
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib

TEST_BINARY="/home/han/MyWorkspace/experiment/opencl_test/build/opencl_test"

# Check if binary exists
if [ ! -f "$TEST_BINARY" ]; then
    echo -e "${RED}Error: Test binary not found${NC}"
    echo "Please build first:"
    echo "  cd /home/han/MyWorkspace/experiment/opencl_test && bash build.sh"
    exit 1
fi

# Check prerequisites
echo "Checking prerequisites..."
echo ""

if [ ! -e /dev/kgsl-3d0 ]; then
    echo -e "${RED}✗ /dev/kgsl-3d0 not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} /dev/kgsl-3d0 exists"

if [ ! -e /dev/ion ]; then
    echo -e "${RED}✗ /dev/ion not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} /dev/ion exists"

if [ ! -f /vendor/lib64/libOpenCL.so ]; then
    echo -e "${RED}✗ libOpenCL.so not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} libOpenCL.so exists"

if [ ! -x /apex/com.android.runtime/bin/linker64 ]; then
    echo -e "${RED}✗ linker64 not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} linker64 exists"

echo ""
echo "Running OpenCL test..."
echo "----------------------------------------"
echo ""

# Run the test
/apex/com.android.runtime/bin/linker64 "$TEST_BINARY"
EXIT_CODE=$?

echo ""
echo "----------------------------------------"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ TEST PASSED${NC}"
    echo "GPU compute is working!"
else
    echo -e "${RED}✗ TEST FAILED${NC}"
    echo ""
    echo "This is EXPECTED behavior."
    echo "GPU commands are queued but not executed"
    echo "due to missing Android framework services."
    echo ""
    echo "See: /home/han/gpu-chroot-research/06-ALTERNATIF-SOLUSI.md"
    echo "for alternative approaches that DO work."
fi

exit $EXIT_CODE
