include Makefile.variables

all: help examples

dist-check:
	@if [ "${DIST_BUILD_MODE}" == "official" ]; then \
		if [ "$(shell git describe --tags --exact-match HEAD 2>/dev/null)" != "${DIST_ANOLIS_VERSION}" ]; then \
			echo "Error: For official build, the tag ${DIST_ANOLIS_VERSION} should point to HEAD"; \
			exit 1; \
		fi \
	fi
	@if [ "${DIST_BUILD_MODE}" == "diy" ] && [ -z "${DIST_DIY}" ]; then \
		echo "Error: For diy build, the variable DIST_DIY should not be empty"; \
		exit 1; \
	fi

dist-genlog:
	bash genlog.sh

dist-genspec: dist-check
	bash genspec.sh

dist-genrpmtree: dist-check
	bash genrpmtree.sh

dist-rpms: dist-genrpmtree dist-check
	bash buildpkg.sh

DIST_CONFIG_TARGETS := dist-defconfig dist-debug-defconfig dist-configs dist-configs-update dist-configs-move dist-configs-import dist-configs-export dist-configs-modify dist-configs-check dist-configs-help

$(DIST_CONFIG_TARGETS):
	make -C configs/ $@

clean:
	rm -rf $(DIST_OUTPUT)
	rm -rf configs/configs.*.old/

dist-version:
	@echo $(DIST_ANOLIS_VERSION)

examples:
	@echo ''
	@echo 'Build Examples:'
	@echo '- *RECOMMEND* devel build with basic rpm packages'
	@echo '    DIST_BUILD_MODE=devel DIST_BUILD_EXTRA=base make dist-rpms'
	@echo '- *RECOMMEND* devel build with full rpm packages'
	@echo '    DIST_BUILD_MODE=devel make dist-rpms'
	@echo '- *RECOMMEND* nightly build with full rpm packages'
	@echo '    DIST_BUILD_MODE=nightly make dist-rpms'
	@echo '- devel build with basic arm64 64k rpm packages'
	@echo '    DIST_BUILD_MODE=devel DIST_BUILD_EXTRA=base DIST_BUILD_ARM64_64K=Y make dist-rpms'
	@echo '- diy build with basic rpm packages'
	@echo '    DIST_BUILD_MODE=diy BUILD_DIY="your_diy_name" DIST_BUILD_EXTRA=base make dist-rpms'
	@echo '- cross compile for riscv'
	@echo '    DIST_BUILD_MODE=nightly DIST_BUILD_EXTRA=base DIST_CROSS_COMPILE=riscv64-linux-gnu- make dist-rpms'

	@echo ''
	@echo 'Kernel Version Examples:'
	@echo '- show the kernel version of devel mode'
	@echo '    DIST_BUILD_MODE=devel make dist-version'
	@echo '- show the kernel version of diy mode'
	@echo '    DIST_BUILD_MODE=diy BUILD_DIY="your_diy_name" make dist-version'
	@echo ''
	@echo 'Other Examples:'
	@echo '- only generate rpm tree in devel mode, but do not build rpm packages'
	@echo '    DIST_BUILD_MODE=devel make dist-genrpmtree'
	@echo '- cleanup'
	@echo '    make clean'

help:
	@echo 'For anolis release'
	@echo ''
	@echo 'RUN `make examples` for some examples'
	@echo '--------------------------------'
	@echo 'generic commands:'
	@echo '    dist-genspec     - generate kernel spec file through kernel.spec.template and changelog files'
	@echo '    dist-genlog      - generate changelogs'
	@echo '    dist-genrpmtree  - generate rpm tree'
	@echo '    dist-rpms        - build kernel rpm package, it will auto generated in $(DIST_SHORT_OUTPUT)'
	@echo '    dist-version     - show dist version'
	@echo '    clean            - cleanup output dir'
	@echo '    examples          - show some examples'
	@echo ''
	@echo '-------------------------------'
	@echo 'the environment variables that could override:'
	@echo '    DIST                 - the distribution suffix, eg: .an7, .an8, .an23'
	@echo '    DIST_OUTPUT          - the output directory, default: $(DIST_SHORT_OUTPUT)'
	@echo '    DIST_BUILD_MODE      - the build mode. optional: official/nightly/devel/diy'
	@echo '                           !!! NOTE: BE CAUTIOUS ABOUT USING official BUILD !!!'
	@echo '                             - official build. kernel version: $(DIST_KERNELVERSION)-$(DIST_OFFICIAL_PKGRELEASEVERION), with srpm'
	@echo '                             - nightly build. kernel version: $(DIST_KERNELVERSION)-$(DIST_UNOFFICIAL_PKGRELEASEVERION), with srpm'
	@echo '                             - devel build. kernel version: $(DIST_KERNELVERSION)-$(DIST_UNOFFICIAL_PKGRELEASEVERION), without srpm'
	@echo '                             - diy build. kernel version: $(DIST_KERNELVERSION)-$(DIST_DIY_PKGRELEASEVERION), with srpm'
	@echo '    DIST_BUILD_NUMBER    - the build number for unofficial build, eg: 1/2'
	@echo '    DIST_DIY             - the kernel version for diy build'
	@echo '    DIST_CROSS_COMPILE   - the cross-compile tool chain, just for riscv, eg: riscv64-unknown-linux-gnu-'
	@echo '    DIST_BUILD_VARIANT & DIST_BUILD_EXTRA & DIST_BUILD_ARM64_64K - see comments in buildpkg.sh'
	@make -C configs/ dist-configs-help

export
