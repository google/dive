#!/system/bin/sh

#export WRAP_GPU_ID=405.16
#export WRAP_GMEM_SIZE=0x40000

export WRAP_GPU_ID=630.1
export WRAP_GMEM_SIZE=0x100000

export LD_LIBRARY_PATH=/system/lib64/:/system/vendor/lib64/

for f in $*; do
	echo "Running: $f"
	n=`basename $f`
	n=${n%%.shader_test}
	TESTNUM=$n LD_PRELOAD=`pwd`/libwrapfake.so ./shader-runner $f
done

