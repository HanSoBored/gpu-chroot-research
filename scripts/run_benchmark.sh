#!/bin/bash
# Benchmark Runner: 8-Core CPU vs Adreno GPU
# Lokasi: /home/han/MyWorkspace/experiment/run_benchmark.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Konfigurasi Path
EXP_DIR="/home/han/MyWorkspace/experiment"
BENCH_BIN="$EXP_DIR/opencl_test/build/matmul_bench"
NDK="/home/han/Android/android-sdk/ndk/29.0.14206865"

# Konfigurasi Environment Android
export ANDROID_ROOT=/system
export ANDROID_DATA=/data
export ANDROID_RUNTIME_ROOT=/apex/com.android.runtime
export LD_LIBRARY_PATH=/vendor/lib64:/vendor/lib:/system/lib64:/system/lib
LINKER="/apex/com.android.runtime/bin/linker64"

echo -e "${BLUE}==================================================${NC}"
echo -e "${BLUE}   BENCHMARK: 8-CORE CPU vs ADRENO 616 GPU        ${NC}"
echo -e "${BLUE}==================================================${NC}"

# 1. Cek Prerequisites
if [ ! -f "$BENCH_BIN" ]; then
	echo -e "${YELLOW}Binary tidak ditemukan. Mencoba kompilasi...${NC}"
	TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-arm64"
	API=29
	TARGET=aarch64-linux-android$API

	$TOOLCHAIN/bin/$TARGET-clang \
		-I$NDK/sysroot/usr/include \
		-L/vendor/lib64 -lOpenCL -ldl -pthread \
		-Wl,-rpath,/vendor/lib64 -Wl,-rpath,/system/lib64 \
		-Wl,--dynamic-linker=$LINKER \
		-O3 -o "$BENCH_BIN" "$EXP_DIR/opencl_test/jni/matmul_bench.c"

	if [ $? -ne 0 ]; then
		echo -e "${RED}Kompilasi Gagal! Pastikan NDK terinstall.${NC}"
		exit 1
	fi
	echo -e "${GREEN}Kompilasi Berhasil.${NC}"
fi

# 2. Tentukan Ukuran Matriks (Default 1024)
SIZE=${1:-1024}
echo -e "Ukuran Matriks: ${YELLOW}${SIZE}x${SIZE}${NC} (Total: $(($SIZE * $SIZE)) elemen)"
echo ""

# 3. Jalankan Benchmark
echo -e "${GREEN}Memulai pengujian...${NC}"
echo "--------------------------------------------------"

$LINKER "$BENCH_BIN" "$SIZE"

#echo "--------------------------------------------------"
#echo -e "${BLUE}Benchmark Selesai.${NC}"
#echo -e "Tips: Gunakan './run_benchmark.sh 512' untuk test cepat"
#echo -e "      Gunakan './run_benchmark.sh 2048' untuk test sangat berat"
