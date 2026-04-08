# containerd v1.3 Release Notes

Source: [v1.3.10](https://github.com/containerd/containerd/releases/tag/v1.3.10)

Welcome to the v1.3.10 release of containerd!

The tenth patch release for `containerd` 1.3 contains a fix for CVE-2021-21334
along with various other minor issues. This is the final release for
`containerd` 1.3.

See [GHSA-6g2q-w5j3-fwh4](https://github.com/containerd/containerd/security/advisories/GHSA-6g2q-w5j3-fwh4)
for more details related to CVE-2021-21334.

### Notable Updates
* **Fix container create in CRI to prevent possible environment variable leak between containers** [#1629](https://github.com/containerd/cri/pull/1629)
* **Add bounds on max `oom_score_adj` value for shim's AdjustOOMScore** [#4875](https://github.com/containerd/containerd/pull/4875)
* **Update task manager to use fresh context when calling shim shutdown** [#4930](https://github.com/containerd/containerd/pull/4930)
* **Fix incorrect usage calculation** [#5126](https://github.com/containerd/containerd/pull/5126)

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Derek McGowan
* Phil Estes
* Sebastiaan van Stijn
* Mike Brown
* Akihiro Suda
* Kir Kolyshkin
* Wei Fu
* Shengjing Zhu
* Li Yuxuan
* Michael Crosby
* Phil Estes
* Sam Whited
* Tom Faulhaber
* Brian Goff
* Derek McGowan
* IceberGu
* Ivan Markin
* Maksym Pavlenko
* Michael Crosby
* Samuel Karp
* Simon Kaegi
* Tibor Vass
* Wilbert van de Ridder
* Xiaodong Ye

### Changes
<details><summary>16 commits</summary>
<p>

* [`1c5970efb`](https://github.com/containerd/containerd/commit/1c5970efbdd8bc864a34baa60c0b382434d4d7c2) Merge pull request from GHSA-6g2q-w5j3-fwh4
* [`9d46f241e`](https://github.com/containerd/containerd/commit/9d46f241e851fb3eb808d965874bbf9f7c531ff7) Prepare release notes for 1.3.10
* [`0eb8cbd29`](https://github.com/containerd/containerd/commit/0eb8cbd2924e06f1e6fc82c868895b2591c36ce4) Merge pull request  [#5126](https://github.com/containerd/containerd/pull/5126) from dmcgowan/backport-1.3-continuity-usage-calculation
* [`8f71d98c6`](https://github.com/containerd/containerd/commit/8f71d98c6296e499827a0dc3fe390448f32f501d) Update continuity to fix usage calculation
* [`dc49905ce`](https://github.com/containerd/containerd/commit/dc49905ce767287a9b169b962cbeab79b8095e26) Merge pull request  [#5121](https://github.com/containerd/containerd/pull/5121) from fuweid/update-cri-plugin
* [`2d9c8aa4b`](https://github.com/containerd/containerd/commit/2d9c8aa4b3f4313982c5c999af57212a1c5d144b) vendor: update CRI plugin with commit ca9c55
* [`3405c1d61`](https://github.com/containerd/containerd/commit/3405c1d6179e81defa278b27f956ce323825512c) Merge pull request  [#4992](https://github.com/containerd/containerd/pull/4992) from Iceber/fix-runc-v2-service-1.3
* [`fb872ce79`](https://github.com/containerd/containerd/commit/fb872ce79bc7340874735420e553e09db9435f1c) runtime: fix shutdown runc v2 service
* [`070cc0129`](https://github.com/containerd/containerd/commit/070cc0129ac01d7fa82b3c03642a24e8ddf085c9) Merge pull request  [#4930](https://github.com/containerd/containerd/pull/4930) from fuweid/cherry-pick-1.3-846cb963c
* [`e97824177`](https://github.com/containerd/containerd/commit/e97824177de626d91fb083f0310f18d469adc4fd) runtime/v2: should use defer ctx to cleanup
* [`804621064`](https://github.com/containerd/containerd/commit/804621064d930b2198834570059c938cc5d7f6ca) Merge pull request  [#4875](https://github.com/containerd/containerd/pull/4875) from johnathanmdell/release/1.3
* [`ff9f916b4`](https://github.com/containerd/containerd/commit/ff9f916b4e0a017d48f3b8af0c174a11f7623e2d) Add bounds on max oom_score_adj value for AdjustOOMScore
* [`1e683ff22`](https://github.com/containerd/containerd/commit/1e683ff2250a236f64dfc0bbda72e0640900a390) Merge pull request  [#4755](https://github.com/containerd/containerd/pull/4755) from thaJeztah/1.3_backport_cancel_shim_log_ctx_by_onclose
* [`3f694f1a3`](https://github.com/containerd/containerd/commit/3f694f1a32ee19e6dd179e9c023d3cf80587b7af) v2: Cancel shim log ctx when ttrpc is closed
* [`7a2410592`](https://github.com/containerd/containerd/commit/7a2410592ae71269064863e21dda0aa28d58de55) v2: Fix missing ns when openShimLog on windows
* [`e9518fb31`](https://github.com/containerd/containerd/commit/e9518fb312cd2a19c26cd29369695ac500130662) v2: Call shim.Delete at first when create is failed
</p>
</details>

### Changes from containerd/continuity
<details><summary>53 commits</summary>
<p>

* [`1d9893e`](https://github.com/containerd/continuity/commit/1d9893e5674b5260c3fc11316d0d5fc0d12ea9e2) Merge pull request  [#169](https://github.com/containerd/continuity/pull/169) from dmcgowan/fix-usage-block-size
* [`363153d`](https://github.com/containerd/continuity/commit/363153d5cc30b7ef2f216c3dacffa23526143fea) Add directory size to usage calculation test
* [`b97555e`](https://github.com/containerd/continuity/commit/b97555e75c86a5f693aa104085036ad4eb1467de) Fix incorrect usage calculation
* [`91328d7`](https://github.com/containerd/continuity/commit/91328d7c60e71160252e8271376d9efadd16f0ad) Merge pull request  [#166](https://github.com/containerd/continuity/pull/166) from zhsj/fix-riscv64
* [`809d89c`](https://github.com/containerd/continuity/commit/809d89c6c3806de909121216d87dd2ff8860581a) go.mod: golang.org/x/sys to latest
* [`62ef0ff`](https://github.com/containerd/continuity/commit/62ef0fffa6a1bed97d4b034c146bc323b2447b72) Merge pull request  [#165](https://github.com/containerd/continuity/pull/165) from zhsj/fix-arm64
* [`25269ef`](https://github.com/containerd/continuity/commit/25269efb6192a3f31d9ef6a57d8631cd48b5f3b9) Fix building on arm64
* [`310e183`](https://github.com/containerd/continuity/commit/310e183616c481b7237980a7787a26435d311c0d) gha: fix invalid workflow definition
* [`04c754f`](https://github.com/containerd/continuity/commit/04c754faca46997ba6d0733f611c42f1816d1199) Merge pull request  [#163](https://github.com/containerd/continuity/pull/163) from dmcgowan/fix-sparse-file-usage
* [`bc5e3ed`](https://github.com/containerd/continuity/commit/bc5e3edd2b742c38c762d928f267ad82922a1b63) Fix usage calculation to account for sparse files
* [`03c371a`](https://github.com/containerd/continuity/commit/03c371a2c3bc37ed384eb4005fce5b8c8c15e5b3) gha: replace uses of deprecated "set-env", "add-path"
* [`f2cc351`](https://github.com/containerd/continuity/commit/f2cc35102c2a086e89ea40de1c0a99861713c51b) Merge pull request  [#157](https://github.com/containerd/continuity/pull/157) from thaJeztah/update_deps
* [`aaa8883`](https://github.com/containerd/continuity/commit/aaa88831d126106ba0ab769e36782be341632b52) Merge pull request  [#160](https://github.com/containerd/continuity/pull/160) from thaJeztah/test_go_1.15
* [`5b95d2d`](https://github.com/containerd/continuity/commit/5b95d2d4f17b34540302493d356909527f50c785) GH Actions: test against Go 1.15
* [`c9598ea`](https://github.com/containerd/continuity/commit/c9598ea9a71c9ec145941cd8ca17700b7c9d87b6) go.mod: github.com/opencontainers/go-digest v1.0.0
* [`71d065d`](https://github.com/containerd/continuity/commit/71d065d8e679c20aac4368e80a08f123ae041462) go.mod: github.com/dustin/go-humanize v1.0.0
* [`84c3eb7`](https://github.com/containerd/continuity/commit/84c3eb7f407ff1781ea97fcad3a1b9ab09d34eb0) go.mod: github.com/pkg/errors v0.9.1
* [`2068663`](https://github.com/containerd/continuity/commit/20686630286e8131a7ed66207f31b75bb8ca1a82) go.mod: logrus v1.6.0
* [`efbc448`](https://github.com/containerd/continuity/commit/efbc4488d8fe1bdc16bde3b2d2990d9b3a899165) Merge pull request  [#156](https://github.com/containerd/continuity/pull/156) from estesp/disable-travis
* [`e2d0145`](https://github.com/containerd/continuity/commit/e2d014531cd9518ff6da95703f4f9895c3394975) Remove travis config
* [`daa8e1c`](https://github.com/containerd/continuity/commit/daa8e1ccc0bcac5cd7d44046b0ea71e7831012ec) Merge pull request  [#155](https://github.com/containerd/continuity/pull/155) from estesp/gh-actions-ci
* [`8c3ce1b`](https://github.com/containerd/continuity/commit/8c3ce1b3ae914b0f92d03138cfe8ff4a1169336a) Update CI to use GitHub Actions
* [`6629113`](https://github.com/containerd/continuity/commit/6629113df58078d4d286df5cf79378ef84dcc525) Update linting to use golangci-lint
* [`9365a1b`](https://github.com/containerd/continuity/commit/9365a1b01a63247561eab02c7d5914a554736c69) Fix golangci-lint errors
* [`f1c9af8`](https://github.com/containerd/continuity/commit/f1c9af8e2a206bd1b9f06b2c7d250deaf4791cf7) Merge pull request  [#154](https://github.com/containerd/continuity/pull/154) from mikebrow/cleanup-nits
* [`f681eac`](https://github.com/containerd/continuity/commit/f681eac03c784dd57e8046e8e3948d4d44294629) reduce code complexity
* [`6728803`](https://github.com/containerd/continuity/commit/6728803c1b2fbe0b63edfccea8c664af8f5df4e1) update AUTHORS
* [`f265cff`](https://github.com/containerd/continuity/commit/f265cff0764e5f8155e80d532db78f617e08e021) fix gofmt issues
* [`cf53015`](https://github.com/containerd/continuity/commit/cf53015a8bae42a53c5725e0d9bef11fde50694e) Merge pull request  [#153](https://github.com/containerd/continuity/pull/153) from tomfaulhaber/empty-file-fix
* [`5a33969`](https://github.com/containerd/continuity/commit/5a339690f8eb7d69926093db01be5f1272ec0c8f) Add a comment to clarify that we're handling the empty file case
* [`11900e8`](https://github.com/containerd/continuity/commit/11900e88c487c2e28650d44cc88a95e86734f01c) Fix sameFile() to recognize empty files as the same
* [`d3ef23f`](https://github.com/containerd/continuity/commit/d3ef23f19fbb106bb73ffde425d07a9187e30745) Merge pull request  [#151](https://github.com/containerd/continuity/pull/151) from kolyshkin/readlink-win
* [`0f16d7a`](https://github.com/containerd/continuity/commit/0f16d7a0959cac64d7a54ce015e50cf4839d1970) Merge pull request  [#150](https://github.com/containerd/continuity/pull/150) from kolyshkin/xattr
* [`643e66e`](https://github.com/containerd/continuity/commit/643e66e4bb3e30dda0a35b6468fb5f35cde7d856) Remove Windows' Readlink fork
* [`da42a30`](https://github.com/containerd/continuity/commit/da42a3033a39c971c1336a8a9f70fbf323857374) driver: fail to build on Windows with go < 1.13
* [`d7961f4`](https://github.com/containerd/continuity/commit/d7961f4caa26b030e7aba76153188a1aad74437e) travis.yml: rm unsupported go releases, add 1.14
* [`bbd0be0`](https://github.com/containerd/continuity/commit/bbd0be0b8f642b9a7b2b2b4f6cccb8f3a90e4f2e) sysx/xattr: improve listxattrAll
* [`9e256e6`](https://github.com/containerd/continuity/commit/9e256e61eee8fc393366eb5c00d8b5fed8bb94fe) sysx/xattr: fix getxattrAll
* [`26c1120`](https://github.com/containerd/continuity/commit/26c1120b8d4107d2471b93ad78ef7ce1fc84c4c4) Merge pull request  [#109](https://github.com/containerd/continuity/pull/109) from nogoegst/fs-openbsd
* [`0ec5967`](https://github.com/containerd/continuity/commit/0ec596719c75bfd42908850990acea594b7593ac) Merge pull request  [#148](https://github.com/containerd/continuity/pull/148) from zhsj/fix-gccgo
* [`a7f992c`](https://github.com/containerd/continuity/commit/a7f992c52c205be63cf0b6e7543099a0c9b45700) fs: don't convert syscall.Timespec to unix.Timespec directly
* [`669de92`](https://github.com/containerd/continuity/commit/669de920ecb0fd1e96591fcb031b9e12bb9cf21c) Merge pull request  [#147](https://github.com/containerd/continuity/pull/147) from yeahdongcn/xattr
* [`b05c0fd`](https://github.com/containerd/continuity/commit/b05c0fd3fcbecc051122adcbd2d616b55bd0f7aa) xattr lost when copying directory
* [`1097c8b`](https://github.com/containerd/continuity/commit/1097c8bae83b84cf3dfccfcc542e06c8c28ea3f4) Merge pull request  [#144](https://github.com/containerd/continuity/pull/144) from SamWhited/modules
* [`91c91a7`](https://github.com/containerd/continuity/commit/91c91a736c4eb12c7bbc93face8ec47a3feb4464) Merge branch 'master' into modules
* [`f65d91d`](https://github.com/containerd/continuity/commit/f65d91d395ebd5507b567968624a4bbdbb9e8819) Merge pull request  [#146](https://github.com/containerd/continuity/pull/146) from fuweid/me-enable-root-for-testing
* [`2f58149`](https://github.com/containerd/continuity/commit/2f581495a4a9485494f737a583616789f8a07578) test: enable root for RequiresRoot cases
* [`abe3784`](https://github.com/containerd/continuity/commit/abe378447a9f73e8a4fe810aa78130b0d490dc40) Support Go Modules
* [`75bee3e`](https://github.com/containerd/continuity/commit/75bee3e2ccb6402e3a986ab8bd3b17003fc0fdec) Merge pull request  [#143](https://github.com/containerd/continuity/pull/143) from tiborvass/fix-sockets
* [`403b5be`](https://github.com/containerd/continuity/commit/403b5be3d72bcee44af7a08c32c0f7ed30ae711b) Merge pull request  [#141](https://github.com/containerd/continuity/pull/141) from WRidder/patch-1
* [`cd143ee`](https://github.com/containerd/continuity/commit/cd143ee28a838efd0d76879d7193a78ac4c40904) fstest: have CreateSocket actually create a socket
* [`38f9467`](https://github.com/containerd/continuity/commit/38f946779f570033f2af75b74d353c0589b36a56) Add src string to copyDirectory error message.
* [`cad9e55`](https://github.com/containerd/continuity/commit/cad9e557d773df5aff292893d4e36781c0164a39) fs: support for OpenBSD
</p>
</details>

### Changes from containerd/cri
<details><summary>14 commits</summary>
<p>

* [`ca9c5533`](https://github.com/containerd/cri/commit/ca9c5533489dfc4296146db858582d46ab767061) Merge pull request  [#1629](https://github.com/containerd/cri/pull/1629) from fuweid/cherry-pick-cri-1628
* [`7ea3462f`](https://github.com/containerd/cri/commit/7ea3462fc2d4e550b307cefb282b7408d7d7f997) cri: append envs from image config to empty slice to avoid env lost
* [`3a1c3b3b`](https://github.com/containerd/cri/commit/3a1c3b3b4b1ec10ab59723ba72ca2cbbc97ab6fe) Merge pull request  [#1604](https://github.com/containerd/cri/pull/1604) from samuelkarp/backport1.3-runtimes
* [`f6f5aef1`](https://github.com/containerd/cri/commit/f6f5aef15c8716a8f6f610f4cb1bc62ce8057b2a) Merge pull request  [#1610](https://github.com/containerd/cri/pull/1610) from thaJeztah/1.3_bump_containerd
* [`7945246e`](https://github.com/containerd/cri/commit/7945246e5c665aff65aaebca9c151504029180b3) vendor: containerd v1.3.7 and dependencies
* [`473085cb`](https://github.com/containerd/cri/commit/473085cb680990c23ef4679ee37592e96d8b15c7) vendor.conf: sort dependencies
* [`87913363`](https://github.com/containerd/cri/commit/87913363195fc72271dd4bd879a2fcf9859c7770) reformat vendor.conf, and use tags again, to match containerd
* [`fa4724b7`](https://github.com/containerd/cri/commit/fa4724b7b19c8f0dba9429ee4afb7b48ee07e501) Merge pull request  [#1611](https://github.com/containerd/cri/pull/1611) from thaJeztah/1.3_fix_golangci_install
* [`c04aabc3`](https://github.com/containerd/cri/commit/c04aabc3ab78f44b767079281856ca3526063c0f) Fix golangci-lint installation
* [`8c742677`](https://github.com/containerd/cri/commit/8c742677af64e8442bf63bd8182e463c532d42ff) enable test-integration target to specify runtime
* [`9528e306`](https://github.com/containerd/cri/commit/9528e30672d7005b6b9a87b36b4e0553a5e9a5bb) Merge pull request  [#1558](https://github.com/containerd/cri/pull/1558) from cpuguy83/1.3_no_libseccomp
* [`52678022`](https://github.com/containerd/cri/commit/52678022c3f2c764270706fcfb81c3f02fcd9b49) Fix header for new seccomp files.
* [`2cc11e5e`](https://github.com/containerd/cri/commit/2cc11e5ef099054b1df855e5958c743612415714) fix for image pull linter change
* [`7f1124c9`](https://github.com/containerd/cri/commit/7f1124c97d97816778aa723c81bc68ce8b65c72f) remove libseccomp cgo dependency
</p>
</details>

### Dependency Changes

* **github.com/containerd/continuity**  f2a389ac0a02 -> 1d9893e5674b
* **github.com/containerd/cri**         f864905c93b9 -> ca9c5533489d

Previous release can be found at [v1.3.9](https://github.com/containerd/containerd/releases/tag/v1.3.9)
