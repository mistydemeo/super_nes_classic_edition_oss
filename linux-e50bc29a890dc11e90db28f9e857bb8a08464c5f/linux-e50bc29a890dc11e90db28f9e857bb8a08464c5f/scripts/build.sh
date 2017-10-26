#!/bin/bash
set -e

#Setup common variables
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-
export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump
export LOCALVERSION=""
export MKBOOTIMG=${LICHEE_TOOLS_DIR}/pack/pctools/linux/android/mkbootimg

KERNEL_VERSION=`make -s kernelversion -C ./`
LICHEE_KDIR=`pwd`
LICHEE_MOD_DIR==${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
export LICHEE_KDIR

update_kern_ver()
{
	if [ -r include/generated/utsrelease.h ]; then
		KERNEL_VERSION=`cat include/generated/utsrelease.h |awk -F\" '{print $2}'`
	fi
	LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
}

show_help()
{
	printf "
Build script for Lichee platform

Invalid Options:

	help         - show this help
	kernel       - build kernel
	modules      - build kernel module in modules dir
	clean        - clean kernel and modules

"
}

NAND_ROOT=${LICHEE_KDIR}/modules/nand

build_nand_lib()
{
	echo "build nand library ${NAND_ROOT}/${LICHEE_CHIP}/lib"
	if [ -d ${NAND_ROOT}/${LICHEE_CHIP}/lib ]; then
		echo "build nand library now"
		make -C modules/nand/${LICHEE_CHIP}/lib clean 2> /dev/null
		make -C modules/nand/${LICHEE_CHIP}/lib lib install
	else
		echo "build nand with existing library"
	fi
}

build_gpu_sun8iw3()
{
	export LANG=en_US.UTF-8
	unset LANGUAGE
	make -j16 -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
	install
}

build_gpu_sun9iw1()
{
	if [ "x${LICHEE_PLATFORM}" = "xandroid" ] ; then
		unset OUT
		unset TOP
		make -j16 -C modules/rogue_km/build/linux/sunxi_android \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
			RGX_BVNC=1.75.2.30 \
			KERNELDIR=${LICHEE_KDIR}
		for file in $(find  modules/rogue_km -name "*.ko"); do
			cp $file ${LICHEE_MOD_DIR}
		done
	fi
}

build_gpu()
{
	chip_sw=`echo ${LICHEE_CHIP} | awk '{print substr($0,1,length($0)-2)}'`

	echo build gpu module for ${chip_sw} ${LICHEE_PLATFORM}

	if [ "${chip_sw}" = "sun9iw1" ]; then
		build_gpu_sun9iw1
	elif [ "${chip_sw}" = "sun8iw3" ] || [ "${chip_sw}" = "sun8iw5" ] ; then
		build_gpu_sun8iw3
	fi
}

clean_gpu_sun9iw1()
{
    unset OUT
    unset TOP
    make -C modules/rogue_km/build/linux/sunxi_android \
        LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
        RGX_BVNC=1.75.2.30 \
        KERNELDIR=${LICHEE_KDIR} clean
}

clean_gpu()
{
	chip_sw=`echo $LICHEE_CHIP | awk '{print substr($0,1,length($0)-2)}'`
	echo
    echo clean gpu module ${chip_sw} $LICHEE_PLATFORM
	if [ "${chip_sw}" = "sun9iw1" ]; then
		clean_gpu_sun9iw1
	fi
}

copy_nand_mod()
{

    cd $LICHEE_KDIR
    if [ -x "./scripts/build_rootfs.sh" ]; then
    		chmod a+x scripts/build_rootfs.sh
    fi
    
    if [ -x "./scripts/build_rootfs.sh" ]; then
        ./scripts/build_rootfs.sh e rootfs.cpio.gz >/dev/null
    else
        echo "No such file: build_rootfs.sh"
        exit 1
    fi

    if [ ! -d "./skel/lib/modules/$KERNEL_VERSION" ]; then
        mkdir -p ./skel/lib/modules/$KERNEL_VERSION
    fi

    cp $LICHEE_MOD_DIR/nand.ko ./skel/lib/modules/$KERNEL_VERSION

    if [ $? -ne 0 ]; then
        echo "copy nand module error: $?"
        exit 1
    fi
    if [ -f "./rootfs.cpio.gz" ]; then
        rm rootfs.cpio.gz
    fi
    ./scripts/build_rootfs.sh c rootfs.cpio.gz >/dev/null
#    rm -rf skel

}

build_kernel()
{
	echo "Building kernel"

	cd ${LICHEE_KDIR}

    if [ ! -f .config ] ; then
        printf "\n\033[0;31;1mUsing default config ...\033[0m\n\n"
        cp arch/arm/configs/${LICHEE_KERN_DEFCONF} .config
    fi

	make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} uImage modules

	update_kern_ver

	rm -rf output
	mkdir -p ${LICHEE_MOD_DIR}

	#The Image is origin binary from vmlinux.
	cp -vf arch/arm/boot/Image output/bImage
	cp -vf arch/arm/boot/[zu]Image output/

	cp .config output/

	tar -jcf output/vmlinux.tar.bz2 vmlinux

        if [ ! -f ./drivers/arisc/binary/arisc ]; then
                echo "arisc" > ./drivers/arisc/binary/arisc
        fi
        cp ./drivers/arisc/binary/arisc output/

	for file in $(find drivers sound crypto block fs security net -name "*.ko"); do
		cp $file ${LICHEE_MOD_DIR}
	done
	cp -f Module.symvers ${LICHEE_MOD_DIR}

	echo -e "\n\033[0;31;1m${LICHEE_CHIP} compile successful\033[0m\n\n"
}

build_modules()
{
	echo "Building modules"

	if [ ! -f include/generated/utsrelease.h ]; then
		printf "Please build kernel first!\n"
		exit 1
	fi

	if [ ! ${LICHEE_PLATFORM} = "tina" ]; then
		update_kern_ver

		build_nand_lib
		make -C modules/nand LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
			CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install
		make -C modules/aw_schw LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
			install

		build_gpu
	fi
}

regen_rootfs_cpio()
{
	echo "regenerate rootfs cpio"

	cd ${LICHEE_KDIR}/output
	if [ -x "../scripts/build_rootfs.sh" ]; then
		../scripts/build_rootfs.sh e ../rootfs.cpio.gz > /dev/null
	else
		echo "No such file: scripts/build_rootfs.sh"
		exit 1
	fi

	mkdir -p ./skel/lib/modules/${KERNEL_VERSION}

	if [ -e ${LICHEE_MOD_DIR}/nand.ko ]; then
		cp ${LICHEE_MOD_DIR}/nand.ko ./skel/lib/modules/${KERNEL_VERSION}
		if [ $? -ne 0 ]; then
			echo "copy nand module error: $?"
			exit 1
		fi
	fi

	ko_file=`find ./skel/lib/modules/$KERNEL_VERSION/ -name *.ko`
	if [ ! -z "$ko_file" ]; then
	        ${STRIP} -d ./skel/lib/modules/$KERNEL_VERSION/*.ko
	fi

	rm -f rootfs.cpio.gz
	../scripts/build_rootfs.sh c rootfs.cpio.gz > /dev/null
	rm -rf skel

	cd - > /dev/null
}

build_ramfs()
{
	# update rootfs.cpio.gz with new module files
	regen_rootfs_cpio

	# If uboot use *boota* to boot kernel, we should use bImage
	if [ "x${LICHEE_CHIP}" = "xsun9iw1p1" ];then
		${MKBOOTIMG} --kernel output/bImage \
		--ramdisk output/rootfs.cpio.gz \
		--board 'sun9i' \
		--base 0x20000000 \
		-o output/boot.img
	else
		${MKBOOTIMG} --kernel output/bImage \
		--ramdisk output/rootfs.cpio.gz \
		--board 'sun8i' \
		--base 0x40000000 \
		-o output/boot.img
	fi
	
	# If uboot use *bootm* to boot kernel, we should use uImage.
	echo build_ramfs
    	echo "Copy boot.img to output directory ..."
    	cp output/boot.img ${LICHEE_PLAT_OUT}
	cp output/vmlinux.tar.bz2 ${LICHEE_PLAT_OUT}

        if [ ! -f output/arisc ]; then
        	echo "arisc" > output/arisc
        fi
        cp output/arisc    ${LICHEE_PLAT_OUT}
}

gen_output()
{
    if [ "x${LICHEE_PLATFORM}" = "xandroid" ] ; then
        echo "Copy modules to target ..."
        rm -rf ${LICHEE_PLAT_OUT}/lib
        cp -rf ${LICHEE_KDIR}/output/* ${LICHEE_PLAT_OUT}
        return
    fi

    if [ "x${LICHEE_PLATFORM}" = "xtina" ] ; then
        echo "Copy modules to target ..."
        rm -rf ${LICHEE_PLAT_OUT}/lib
        cp -rf ${LICHEE_KDIR}/output/* ${LICHEE_PLAT_OUT}
        return
    fi

    if [ -d ${LICHEE_BR_OUT}/target ] ; then
        echo "Copy modules to target ..."
        local module_dir="${LICHEE_BR_OUT}/target/lib/modules"
        rm -rf ${module_dir}
        mkdir -p ${module_dir}
        cp -rf ${LICHEE_MOD_DIR} ${module_dir}
    fi
}

clean_kernel()
{
	echo "Cleaning kernel"
	make ARCH=arm clean
	rm -rf output/*

	(
	export LANG=en_US.UTF-8
	unset LANGUAGE
	make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
	make -C modules/aw_schw LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
	)
}

clean_modules()
{
	echo "Cleaning modules"
	clean_gpu
}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

#LICHEE_ROOT=`(cd ${LICHEE_KDIR}/..; pwd)`
#export PATH=${LICHEE_ROOT}/buildroot/output/external-toolchain/bin:${LICHEE_ROOT}/tools/pack/pctools/linux/android:$PATH
#if [ x$2 = x ];then
#	echo Error! you show pass chip name as param 2
#	exit 1
#else
#	chip_name=$2
#	platform_name=${chip_name:0:5}
#fi

case "$1" in
kernel)
	build_kernel
	;;
modules)
	build_modules
	;;
clean)
	clean_kernel
	clean_modules
	;;
*)
	build_kernel
	build_modules
	build_ramfs
	gen_output
	;;
esac

