#! /bin/bash
# check kconfigs obey constraints or not.
# it is called from Makefile, do not run it directly.
#
# usage:
# - check for only one arch:
#    ARCH=${arch} make dist-configs-check
#    available archs are: x86, arm64, loongarch, riscv
# - check for all arch:
#    make dist-configs-check

SCRIPT_DIR=$(realpath $(dirname $0))

final_exit_status=0

function check_arch() {
    local arch=$1

    local opt="--rules ${SCRIPT_DIR}/L0-MANDATORY/${arch}.config
              --level L0-MANDATORY
              --rules ${SCRIPT_DIR}/L1-RECOMMEND/${arch}.config
              --level L1-RECOMMEND "

    if [ -f ${SCRIPT_DIR}/EXTRA/${arch}.config ]; then
        opt="${opt} --rules ${SCRIPT_DIR}/EXTRA/${arch}.config
            --level L0-MANDATORY "
    fi

    if [ -f ${SCRIPT_DIR}/../../../arch/${arch}/configs/anolis_defconfig ]; then
        opt="${opt} ${SCRIPT_DIR}/../../../arch/${arch}/configs/anolis_defconfig"
    else
        opt="${opt} ${DIST_OUTPUT}/kernel-ANCK-generic-${arch}.config"
    fi

    echo "* Checking configs for arch: $arch"
    python3 ${SCRIPT_DIR}/anolis_kconfig_check.py check ${opt}

    local ret=$?
    if [ $final_exit_status -eq 0 ]; then
        final_exit_status=$ret
    fi
}

# arch sw_64 is not available now
arch_list=("x86" "arm64" "loongarch" "riscv")

if [ -n "${ARCH}" ]; then
    arch_list=(${ARCH})
fi

for arch in ${arch_list[@]}
do
    check_arch $arch
done

exit $final_exit_status
