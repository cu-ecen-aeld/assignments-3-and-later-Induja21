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
CROSS_TOOLCHAIN_DIR_PATH=$(${CROSS_COMPILE}gcc --print-sysroot)
FINDERAPP_DIR=${PWD}

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}
if [ ! -d "$OUTDIR" ];then
    echo "Error: Provided directory could not be created hence exiting"
    exit 1
fi

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
    #Deep clean the kernel build tree removing the configuration files
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    #Configures for virtual arm device that would be simulated with qemu
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    #Builds kernel image to boot with qemu. j4 speeds up the kernel build
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    #Builds kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    #Builds device tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

    # Copy the results to the output directory
    echo "Copying build results to output directory"
    cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/
    echo "Kernel build is complete"
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir "$OUTDIR/rootfs"
cd "$OUTDIR/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
#Copies busy box to rootfs and also does required symlinks
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install


echo "Library dependencies"
INTERPRETER=$(${CROSS_COMPILE}readelf -a busybox | grep "program interpreter" | awk '{print $NF}'  | sed 's/\/lib\///g; s/]//g')
SHARED_LIBS=$(${CROSS_COMPILE}readelf -a busybox | grep "Shared library" | awk '{print $NF}' | sed 's/\[\|\]//g')

#TODO: Add library dependencies to rootfs
for LIB in ${SHARED_LIBS}; do
    LIB_PATH=$(find ${CROSS_TOOLCHAIN_DIR_PATH} -name ${LIB} 2>/dev/null)
    if [ -n "$LIB_PATH" ]; then
        echo "Copying shared lib path"
        sudo cp "$LIB_PATH" ${OUTDIR}/rootfs/lib64/
    fi

done

cp $(find ${CROSS_TOOLCHAIN_DIR_PATH} -name $(basename ${INTERPRETER})) ${OUTDIR}/rootfs/lib/


# TODO: Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
cd "${FINDERAPP_DIR}"
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDERAPP_DIR}/finder* ${OUTDIR}/rootfs/home/
cp ${FINDERAPP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDERAPP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp -rL ${FINDERAPP_DIR}/conf ${OUTDIR}/rootfs/home/


# TODO: Chown the root directory
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > initramfs.cpio
gzip -f initramfs.cpio
mv initramfs.cpio.gz ${OUTDIR}