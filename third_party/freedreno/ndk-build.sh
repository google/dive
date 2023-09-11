export NDK_PROJECT_PATH=`pwd`
# GOOGLE: using the same NDK for libwrap and the layer
ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}
$ANDROID_NDK_ROOT/build/ndk-build NDK_APPLICATION_MK=./Application.mk "$@"
