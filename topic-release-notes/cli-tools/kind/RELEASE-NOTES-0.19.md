# kind v0.19 Release Notes

Source: [v0.19.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.19.0)

KIND v0.19.0 contains a fix for airgapped node image usage and a significant overhaul over our base image and dependency management.

KIND node images now contain a `LICENSE/` directory based on go-licenses for all external go binaries, all external go binaries are built from source and contain fully patched go as of the time of release, streamlining that process and decoupling versions.

Since this release comes shortly after v0.18.0, we highly recommend seeing [v0.18.0 release notes](https://github.com/kubernetes-sigs/kind/releases/tag/v0.18.0) as well.


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.27.1` image: `kindest/node:v1.27.1@sha256:b7d12ed662b873bd8510879c1846e87c7e676a79fefc93e17b2a52989d3ff42b`
- Base distro is now Debian, not Ubuntu. 
   - While it is not supported to depend on the contents of these images beyond providing what KIND needs to create a functioning Kubernetes cluster at a given version, we know some power users depend on this anyhow. This is your warning! This is subject to change again in the future. We also dropped some now-unnecessary packages from the image.
- Go 1.16+ is now required to build the `kind` binary or import `kind` as a library (much more recent Go 1.20.4 was used for release builds, but 1.16 is the minimum required version now)



<h1 id="new-features">New Features</h1>

- Smaller node image containerd binaries with unusable snapshotters compiled out
- `LICENSES/` directory in all node / base images with license info for all dependent packages / binaries contained in images that don't come from the base distro (other packages are covered by distro standard metadata)


New node images have been built for kind `v0.19.0`, please use these **exact** images (IE like `kindest/node:v1.26.3@sha256: 61b92f38dff6ccc29969e7aa154d34e38b89443af1a2c14e6cfbd2df6419c66f` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images pre-built for this release:
 - 1.27: `kindest/node:v1.27.1@sha256:b7d12ed662b873bd8510879c1846e87c7e676a79fefc93e17b2a52989d3ff42b`
 - 1.26: `kindest/node:v1.26.4@sha256:f4c0d87be03d6bea69f5e5dc0adb678bb498a190ee5c38422bf751541cebe92e`
 - 1.25: `kindest/node:v1.25.9@sha256:c08d6c52820aa42e533b70bce0c2901183326d86dcdcbedecc9343681db45161`
 - 1.24: `kindest/node:v1.24.13@sha256:cea86276e698af043af20143f4bf0509e730ec34ed3b7fa790cc0bea091bc5dd`
 - 1.23: `kindest/node:v1.23.17@sha256:f77f8cf0b30430ca4128cc7cfafece0c274a118cd0cdb251049664ace0dee4ff`
 - 1.22: `kindest/node:v1.22.17@sha256:9af784f45a584f6b28bce2af84c494d947a05bd709151466489008f80a9ce9d5`
 - 1.21: `kindest/node:v1.21.14@sha256:220cfafdf6e3915fbce50e13d1655425558cb98872c53f802605aa2fb2d569cf` 

Additional images built for this release:
- 1.28: `kindest/node:v1.28.0@sha256:dad5a6238c5e41d7cac405fae3b5eda2ad1de6f1190fa8bfc64ff5bb86173213`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images


<!-- Additional images known compatible with this release: -->

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Fixed airgap preloading for kindnetd and local-path-provisioner
- podman: detect disabled IPv6 and create IPv4 only network instead
- Overhauled image building, fully patched all dependencies and build toolchains
- Updated containerd, crictl, CNI plugins, local-path-provisioner ... etc
- Dropped unnecessary packages from image
- Fixed dockerized site build on non-amd64 hosts



<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @aojea 
- @BenTheElder 
- @cpanato 
- @daman1807 
- @k8s-ci-robot 
- @m-Bilal
- @orange-guo 
- @pohly 
- @RijulGulati 
- @rjsadow 
- @VibhorChinda 
- @wzshiming 
- @yanggangtony 


And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏

In particular a shoutout to @stmcginnis for helping with reviews and responding to support issues! 💟 