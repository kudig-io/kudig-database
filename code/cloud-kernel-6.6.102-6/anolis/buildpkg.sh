set -xe

function do_rpmbuild() {
	if [ "$DIST_BUILD_MODE" == "official" ] || \
	   [ "$DIST_BUILD_MODE" == "nightly" ]  || \
	   [ "$DIST_BUILD_MODE" == "diy" ]; then
		CMD="-ba"
	else
		CMD="-bb"
	fi

	# Now we have:
	#  + variants: default, with-debug, with-gcov
	#  + extras: base, with-debuginfo, full
	#  + modes: official, nightly, dev
	#
	# DIST_BUILD_ARM64_64K control whether 64k kernels are built,
	# set DIST_BUILD_ARM64_64K=y to build 64k kernels.
	#
	# Matrix
	#

	# | official  | without sha id  | Yes          |
	# | nightly   | with git sha id | Yes          |
	# | devel     | with git sha id | No           |
	#
	# | Extra\Var | Default         |  With-debug  |  With-gcov   |
	# |-----------|-----------------|--------------|--------------|

	# | Base      |     +default    |   +default   |     gcov     |
	# |           |      -debug     |   +debug     |              |
	# |           |                   +headers                    |
	# |-----------|--------------------------------|--------------|
	# | debuginfo |            +debuginfo          |  +debuginfo  |
	# |-----------|--------------------------------|--------------|
	# | full      |      +tools +doc +perf         | +tools +doc  |
	#

	build_opts="--with headers --without bpftool"

	if [ "_${DIST_BUILD_VARIANT}" == "_with-debug" ]; then
		build_opts="$build_opts --with debug"
	elif [ "_${DIST_BUILD_VARIANT}" == "_with-gcov" ]; then
		build_opts="$build_opts --without debug --with gcov"
	else # assume default
		build_opts="$build_opts --without debug"
	fi

	if [ "_${DIST_BUILD_ARM64_64K}" == "_Y" ]; then
		build_opts="$build_opts --with arm64_64k"
	else # assume default
		build_opts="$build_opts --without arm64_64k"
	fi

	if [ "_${DIST_BUILD_EXTRA}" == "_debuginfo" ]; then
		build_opts="$build_opts --with debuginfo --without tools --without doc --without perf"
	elif [ "_${DIST_BUILD_EXTRA}" == "_base" ]; then
		build_opts="$build_opts --without debuginfo --without tools --without doc --without perf"
	else # assume full
		build_opts="$build_opts --with debuginfo --with tools --with doc --with perf"
	fi

	if [ "_${DIST_CROSS_COMPILE}" != "_" ]; then
	    build_opts="$build_opts --with cross"
	    TARGET=riscv64
	else
	    build_opts="$build_opts --without cross"
	    TARGET=$(uname -m)
	fi


    # launch a new shell to clear current environment variables passed by Makefile
	 rpmbuild \
		--define "%_smp_mflags -j$(nproc)" \
		--define "%packager <alicloud-linux@alibaba-inc.com>" \
		--define "%_topdir ${DIST_RPMBUILDDIR_OUTPUT}" \
		${build_opts} \
		${CMD} ${DIST_RPMBUILDDIR_OUTPUT}/SPECS/kernel.spec \
		--target ${TARGET} || exit 1
}

function output() {
	if [ -z "$DIST_OFFICIAL_BUILD" ]; then
		targetdir=${DIST_BUILD_NUMBER}
	else
		targetdir=${DIST_ANOLIS_VERSION}
	fi

	mkdir -p ${DIST_OUTPUT}/${targetdir}

	cp ${DIST_RPMBUILDDIR_OUTPUT}/RPMS/${TARGET}/*.rpm ${DIST_OUTPUT}/${targetdir}/

	# copy srpm packages if and only if they exist.
	if [ -f ${DIST_RPMBUILDDIR_OUTPUT}/SRPMS/*.rpm ]; then
		cp ${DIST_RPMBUILDDIR_OUTPUT}/SRPMS/*.rpm ${DIST_OUTPUT}/${targetdir}
	fi

	ls ${DIST_OUTPUT}/${targetdir}/*.rpm

	rpm_num=$(ls ${DIST_OUTPUT}/${targetdir}/*.rpm | wc -l)
	echo "${rpm_num} rpm(s) copied."
}

do_rpmbuild
output
