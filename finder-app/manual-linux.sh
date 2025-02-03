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
#CROSS_COMPILE=${CROSS_COMPILE}-
CROSS_COMPILE=/home/bhakti/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
echo "Checkpoint 1"
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
    # Cleaning any .config files
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE}- mrproper
    
    # Specifying defconfig
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE}- defconfig
    
    # QEMU kernel build vmlinux
    make -j4 ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE}- all
    
    # Adding kernel modules and devicetree
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE}- modules
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE}- dtbs
    echo "Checkpoint 2" 
fi

echo "Kernel Build complete"

# Copying results to the output directory
echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
# Creating root file system in output directory and making basic directories
mkdir "$OUTDIR/rootfs"
cd "$OUTDIR/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/bin usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone https://github.com/mirror/busybox.git
#git clone git@github.com:mirror/busybox.git

	    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "Busy box configuration"
    make distclean
    make defconfig
   
else
    echo "Busy box already configured"
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd "${OUTDIR}"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ./rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ./rootfs/bin/busybox | grep "Shared library"

cp /home/bhakti/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp /home/bhakti/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp /home/bhakti/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/
cp /home/bhakti/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/

#TODO: Add library dependencies to rootfs

# TODO: Add library dependencies to rootfs
# ldd busy box prints shared library and program interpreter both
#cd "$OUTDIR/rootfs/lib64"
# Copying shared library to lib64 - because we are using Arch64
#${CROSS_COMPILE}ldd busybox | grep "=>" | awk '{print $3}' | xargs -I {} cp {} $OUTDIR/rootfs/lib64/
# Copying/Adding necessary program interpreter to lib
#INTERPRETER=$(${CROSS_COMPILE}ldd busybox | grep "ld-linux" | awk '{print $1}')
#cp $INTERPRETER ${OUTDIR}/rootfs/lib/

echo "Library Dependencies added"
#echo $(ls ${OUTDIR}/rootfs/lib64)
#echo $(ls ${OUTDIR}/rootfs/lib)


# TODO: Make device nodes
# creating a device/special file - giving permision as mode - permissions - path for newly created file - character file - major no(memory driver) - minor no(specific driver)
#cd "${OUTDIR}/rootfs"

sudo rm -f /dev/null
#sudo mknod -m 666 /dev/null c 1 3

sudo rm -f /dev/console
#sudo mknod -m 600 /dev/console c 5 1

sudo rm -f /dev/tty
#sudo mknod -m 600 /dev/tty c 5 0
#chown root:tty /dev/{console,ptmx,tty}

mount -n -t tmpfs none /dev
mknod -m 622 /dev/console c 5 1
mknod -m 666 /dev/null c 1 3
mknod -m 666 /dev/zero c 1 5
mknod -m 666 /dev/tty c 5 0 
chown root:tty /dev/{console,tty}

echo "Noed Added"

# TODO: Clean and build the writer utility
# Go to finder app directory
cd /home/bhakti/Work/assignments-3-and-later-BhaktiRamani/finder-app
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# copy command - source destination
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/writer.c ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp -rL ${FINDER_APP_DIR}/conf ${OUTDIR}/rootfs/home/

#matches=$(ls | grep -E 'finder|writer|autorun-qemu.sh|conf')
# Check if any matches were found
#if [ -n "$matches" ]; then
#	for file in $matches; do
#		cp "$file" ${OUTDIR}/rootfs/home
#	done
#else
#	echo "No finder related files"
#fi	

# on the target rootfs
cd "${OUTDIR}/rootfs"

# TODO: Chown the root directory
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find .| cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio
#mv initramfs.cpio.gz ${OUTDIR}
