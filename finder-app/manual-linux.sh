#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
TC_DIR=/home/ubuntu/workspace/coursera/toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu
BUILD_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}"   )" && pwd   )"

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j2 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin usr/lib64
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

# TODO: Make and install busybox
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

ML_ROOT_DIR=${OUTDIR}/rootfs
echo "Library dependencies"
cd ${ML_ROOT_DIR}
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
cp ${TC_DIR}/libc/lib/ld-linux-aarch64.so.1 ${ML_ROOT_DIR}/lib/
cp ${TC_DIR}/libc/lib64/libc.so.6 ${ML_ROOT_DIR}/lib64/
cp ${TC_DIR}/libc/lib64/libresolv.so.2 ${ML_ROOT_DIR}/lib64/
cp ${TC_DIR}/libc/lib64/libm.so.6 ${ML_ROOT_DIR}/lib64/

# TODO: Make device nodes
cd ${ML_ROOT_DIR}
if [ ! -e "${ML_ROOT_DIR}/dev/null" ]; then
    sudo mknod -m 666 ${ML_ROOT_DIR}/dev/null c 1 3
fi
if [ ! -e "${ML_ROOT_DIR}/dev/console" ]; then
    sudo mknod -m 666 ${ML_ROOT_DIR}/dev/console c 5 1
fi

# TODO: Clean and build the writer utility
echo "Clean and build the writer utility in ${BUILD_DIR}"
cd ${BUILD_DIR}
make clean
make CROSS=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copy the writer utility from ${BUILD_DIR} to ${ML_ROOT_DIR}/home/"
cd ${BUILD_DIR}
cp writer finder.sh finder-test.sh autorun-qemu.sh ${ML_ROOT_DIR}/home/
mkdir ${ML_ROOT_DIR}/home/conf
cp conf/* ${ML_ROOT_DIR}/home/conf/
#sed -i 's|\.\./conf|conf|' ${ML_ROOT_DIR}/home/finder-test.sh

# TODO: Chown the root directory

# TODO: Create initramfs.cpio.gz
cd ${ML_ROOT_DIR}
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
