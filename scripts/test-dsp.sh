#!/bin/bash
# Hexagon DSP Test Runner
# Tests FastRPC communication with DSP

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "Hexagon DSP Test"
echo "========================================"
echo ""

# Setup environment
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib

TEST_BINARY="/home/han/MyWorkspace/experiment/opencl_test/build/dsp_test"
ADSPRPCD="/vendor/bin/adsprpcd"
CDSPRPCD="/vendor/bin/cdsprpcd"

# Check if binary exists
if [ ! -f "$TEST_BINARY" ]; then
    echo -e "${RED}Error: Test binary not found${NC}"
    echo "Please build first:"
    echo "  cd /home/han/MyWorkspace/experiment/opencl_test && bash build.sh"
    exit 1
fi

echo "Checking prerequisites..."
echo ""

if [ ! -e /dev/adsprpc-smd ]; then
    echo -e "${RED}✗ /dev/adsprpc-smd not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} /dev/adsprpc-smd exists"

if [ ! -f /vendor/lib64/libadsprpc.so ]; then
    echo -e "${RED}✗ libadsprpc.so not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} libadsprpc.so exists"

echo ""

# Check if daemon is running
if pgrep -f "adsprpcd" > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} adsprpcd daemon is running"
else
    echo -e "${YELLOW}⚠${NC} adsprpcd daemon not running"
    echo "Starting daemon (background)..."
    
    if [ -f "$ADSPRPCD" ]; then
        /apex/com.android.runtime/bin/linker64 "$ADSPRPCD" &
        sleep 2
        
        if pgrep -f "adsprpcd" > /dev/null 2>&1; then
            echo -e "${GREEN}✓${NC} adsprpcd started"
        else
            echo -e "${YELLOW}⚠${NC} Failed to start adsprpcd"
        fi
    else
        echo -e "${YELLOW}⚠${NC} adsprpcd binary not found"
    fi
fi

echo ""
echo "Running DSP test..."
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
    echo "DSP communication is working!"
else
    echo -e "${RED}✗ TEST FAILED${NC}"
    echo ""
    echo "This is EXPECTED behavior."
    echo "FastRPC requires Binder-based service validation"
    echo "which is not available in chroot environment."
    echo ""
    echo "The adsprpc-smd device is accessible, but the"
    echo "kernel daemon rejects attachment requests without"
    echo "proper Android framework services."
    echo ""
    echo "See: /home/han/gpu-chroot-research/04-TEST-DSP-HEXAGON.md"
fi

exit $EXIT_CODE
