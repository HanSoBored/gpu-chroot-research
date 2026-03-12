#!/bin/bash
# run_gpu_test.sh - Script for running GPU compute test in Android Chroot

# 1. Environment variables
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib

# 2. Build fixed binary if needed
NDK=$ANDROID_NDK_HOME
if [ -d "$NDK" ]; then
    echo "NDK found at $NDK, rebuilding binary..."
    TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-arm64
    API=29
    TARGET=aarch64-linux-android$API

    $TOOLCHAIN/bin/$TARGET-clang \
        -I$NDK/sysroot/usr/include \
        -L/vendor/lib64 \
        -lOpenCL \
        -ldl \
        -Wl,-rpath,/vendor/lib64 \
        -Wl,-rpath,/system/lib64 \
        -Wl,--dynamic-linker=/apex/com.android.runtime/bin/linker64 \
        -o gpu-chroot-research/build/opencl_test \
        gpu-chroot-research/opencl_test/jni/stage2/opencl_test.c
fi

# 3. Run the test
echo "Running GPU compute test..."
/apex/com.android.runtime/bin/linker64 /home/han/MyWorkspace/experiment/gpu-chroot-research/gpu-chroot-research/build/opencl_test
