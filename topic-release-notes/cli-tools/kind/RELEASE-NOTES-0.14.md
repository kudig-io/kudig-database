# kind v0.14 Release Notes

Source: [v0.14.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.14.0)

`v0.14.0` is quick follow-up to [`v0.13.0`](https://github.com/kubernetes-sigs/kind/releases/tag/v0.13.0), upgrading packages and fixing cgroups on some non-systemd-based hosts such as WSL2 and Alpine based tools like colima and rancher-desktop. 

Besides the cgroups fix, the update to the latest version of the local-path-provisioner may be a desirable upgrade worth noting with various downstream improvements.

Be sure to see [the previous release notes](https://github.com/kubernetes-sigs/kind/releases/tag/v0.13.0) as well!

<h1 id="breaking-changes">Breaking Changes</h1>

NOTE: The systemd change is from KIND v0.13.0, but we're re-iterating it here since v0.13.0 was so recent.

- **systemd cgroups driver will be used for Kubernetes v1.24.0+** (rather than 1.21.0+ when [kubeadm changed the default](https://github.com/kubernetes/kubernetes/pull/99471), which we previously overrode).
  - **NOTE**: **You must use kind v0.13.0+ with Kubernetes v1.24.0+ images**, and if you built your own Kubernetes v1.24.0+ image
with a previous kind version you will need to re-built when switching to kind v0.13.0+.
  - **NOTE**: **You do not need to be using systemd on the host machine**, In kind v0.13.0 we had a bug related to this, it should be fixed this release. systemd is used *inside* the kind node containers but should not be necessary on the host. We are now using it for Kubernetes pods in addition to running kubelet, containerd etc.
  - KIND will **continue to use cgroupfs for Kubernetes versions prior to v1.24.0**.
- The default node image is a Kubernetes `v1.24.0` image: `kindest/node:v1.24.0@sha256:0866296e693efe1fed79d5e6c7af8df71fc73ae45e3679af05342239cdc5bc8e`


<h1 id="new-features">New Features</h1>

- digest image references should be populated when loading images
- base image updates
  - crictl v1.24.0
- kind binary built with Go 1.18.2
- all kindnetd dependencies updated to latest
- updated haproxy image
- latest local-path-provisioner v0.0.22 with updated packaging
- support for Kubernetes v1.25.0-alpha pre-releases builds without the old kubeadm node taint
- updated cgroupsv2 CI to Fedora 36


New Node images have been built for kind `v0.14.0`, please use these **exact** images (IE like `kindest/node:v1.24.0@sha256:0866296e693efe1fed79d5e6c7af8df71fc73ae45e3679af05342239cdc5bc8e` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.24: `kindest/node:v1.24.0@sha256:0866296e693efe1fed79d5e6c7af8df71fc73ae45e3679af05342239cdc5bc8e`
  - 1.23: `kindest/node:v1.23.6@sha256:b1fa224cc6c7ff32455e0b1fd9cbfd3d3bc87ecaa8fcb06961ed1afb3db0f9ae`
  - 1.22: `kindest/node:v1.22.9@sha256:8135260b959dfe320206eb36b3aeda9cffcb262f4b44cda6b33f7bb73f453105`
  - 1.21: `kindest/node:v1.21.12@sha256:f316b33dd88f8196379f38feb80545ef3ed44d9197dca1bfd48bcb1583210207`
  - 1.20: `kindest/node:v1.20.15@sha256:6f2d011dffe182bad80b85f6c00e8ca9d86b5b8922cdf433d53575c4c5212248`
  - 1.19: `kindest/node:v1.19.16@sha256:d9c819e8668de8d5030708e484a9fdff44d95ec4675d136ef0a0a584e587f65c`
  - 1.18: `kindest/node:v1.18.20@sha256:738cdc23ed4be6cc0b7ea277a2ebcc454c8373d7d8fb991a7fcdbd126188e6d7`

NOTE: these node images support amd64 and arm64. It remains possible to build custom images for other architectures (see the docs).

<h1 id="fixes">Fixes</h1>

- fixed running kind with kubernetes v1.24.0+ on some non-systemd-based hosts
- setting migrated kubelet flags in kubelet config where possible, for now we set both the old flags and the config, in some future release we'll set only kubelet config where possible
- fixed kong ingress name in docs


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @AkihiroSuda
- @AppalaKarthik
- @BenTheElder
- @k8s-ci-robot
- @neolit123

A special shoutout to these folks who helped report, test, and review the systemd cgroups fix: 
- @aojea
- @cndoit18
- @ckoenig
- @jankoprowski
- @longwuyuan
- @mariomac
- @stmcginnis
- @Unveraenderbar

And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏