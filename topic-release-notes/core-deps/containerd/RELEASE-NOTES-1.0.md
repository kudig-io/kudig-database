# containerd v1.0 Release Notes

Source: [v1.0.3](https://github.com/containerd/containerd/releases/tag/v1.0.3)

Welcome to the v1.0.3 release of containerd!

This is the third patch release for `containerd` in the 1.0 series. It includes a few small but impactful fixes.

This contains a mitigation for problems with healthchecks described in https://github.com/moby/moby/issues/36661. We now timeout the FIFO creation to avoid deadlocks in the containerd-shim.

Please see the changelog for full details.

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Akihiro Suda
* Derek McGowan
* Ian Campbell
* Justin Cormack
* Kenfe-Mickael Laventure
* Michael Crosby
* Phil Estes
* Stephen Day
* Yanqiang Miao

### Changes

* 773c489c Merge pull request #2264 from stevvooe/prepare-1.0.3
* a1debc74 release: prepare 1.0.3 release
* 6db03b75 Merge pull request #2260 from stevvooe/cherry-pick-#2258
* 8386ef28 Fix typo in CreateUnixSocket error message
* 46a104a2 Merge pull request #2256 from stevvooe/cherry-pick-#2255
* 34b7c1d2 Return a better error message is unix socket path is too long.
* 51cf56f2 Merge pull request #2248 from stevvooe/cherry-pick-#2241
* 69c2686a The set of bounding capabilities is the largest group
* 2b3b44fd Merge pull request #2242 from stevvooe/prepare-1.0.3-rc.0
* 3a9d193d release: prepare 1.0.3-rc.0
* 62aad0e4 Merge pull request #2247 from estesp/fix-typeurl-typo
* 2f27d47c Fix typo in metadata test typeurl string
* 386b4e93 Merge pull request #2240 from dmcgowan/backport-fix-uncompressed-label
* 457c658e Fix label being put on snapshot instead of content
* bde00362 Merge pull request #2234 from AkihiroSuda/cherry-pick-2221
* eaee9f54 content/testsuite: include small blob test in standard suite
* abef3899 services/content: fix reading a blob which is smaller than the read buffer.
* 21e89eb6 Merge pull request #2232 from stevvooe/cherry-pick-#2229
* d235ae94 linux/prox: timeout fifo creation
* a609ec46 Merge pull request #2224 from dmcgowan/backport-archive-gc-fixes
* 3d34cc01 Fixes a default config bug of gc scheduler
* d77d4bab Add ignore socket test
* 6f0d2809 Ignore sockets when creating a tar stream of a layer
* c0f92ddf Merge pull request #2198 from estesp/vendor-cgroups-update-1.0
* d2c460cd Merge pull request #2194 from kunalkushwaha/cherry-pick-runtime-root
* 5f0a37cf Update cgroups vendor for licenses/bug fix
* ee089eb2 linux: fix runtime-root propagation
* 6646106a linux: propagate --runtime-root to shim properly
* 9b4bbcc9 Merge pull request #2191 from stevvooe/cherry-pick-#2190
* d7069a55 vendor: update btrfs dependency
* 015bdb7c Merge pull request #2188 from crosbymichael/sigpiper
* 592af934 Handle SIGPIPE in shims
* d18de622 Merge pull request #2179 from AkihiroSuda/userns-mknod-1.0
* a3372da0 archive: fix logic for skipping mknod when running in userns

### Dependency Changes

Previous release can be found at [v1.0.2](https://github.com/containerd/containerd/releases/tag/v1.0.2)

* c0710c92e8b3a44681d1321dcfd1360fc5c6c089 -> fe281dd265766145e943a034aa41086474ea6130 **github.com/containerd/cgroups**
* cc52c4dea2ce11a44e6639e561bb5c2af9ada9e3 -> 2e1aa0ddf94f91fa282b6ed87c23bf0d64911244 **github.com/containerd/btrfs**
