---
title: kind v0.18 Release Notes
description: kind v0.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.18 Release Notes 是什么
- 如何 kind v0.18 Release Notes
trigger_keywords:
- kind
- v0.18
- Release
- Notes
- release
- notes
---

# kind v0.18 Release Notes

Source: [v0.18.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.18.0)

KIND v0.18.0 Comes with a big shoutout to [Docker, Inc.](https://www.docker.com/company/) for accepting us into the updated [Docker Sponsored OSS Program](https://www.docker.com/community/open-source/application/). **Thanks Docker!** 🎉 

**Images should no longer have pull rate limits as a result**.

The project will still consider mirroring on or switching primarily to [registry.k8s.io](https://registry.k8s.io) in the future, after determining an updated immutable tagging scheme to comply with requirements there.

Otherwise of particular note are a fix for iptables nf_tables v1.8.8+, updated dependencies including [runc v1.1.5 with CVE fixes](https://github.com/opencontainers/runc/releases/tag/v1.1.5), and a new networking option to control the DNS search list.


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.26.3` image: `kindest/node:v1.26.3@sha256:61b92f38dff6ccc29969e7aa154d34e38b89443af1a2c14e6cfbd2df6419c66f`
- **Dropped support for PPC64LE and S390x**, which only had limited support previously
  - These platforms had very slow and flaky builds despite attempts at fixing and very limited demand. We've dropped these to focus on the vast majority of our users. These platforms never reached the point of having official node images.
-  Removed registry mirror config for `k8s.gcr.io` => `registry.k8s.io`
   - k8s.gcr.io is partially redirected already now, and being phased out. **Please update your references to use registry.k8s.io directly instead!** See: https://registry.k8s.io, https://kubernetes.io/blog/2023/03/10/image-registry-redirect/


<h1 id="new-features">New Features</h1>

- New `networking.dnsSearch` config field for overriding the cluster nodes' DNS search list
- Documented how to use KIND on chromeOS
- Automated builds for most images
- Improved output for `kind delete cluster`


New Node images have been built for kind `v0.18.0`, please use these **exact** images (IE like `kindest/node:v1.26.3@sha256: 61b92f38dff6ccc29969e7aa154d34e38b89443af1a2c14e6cfbd2df6419c66f` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images pre-built for this release:
 - 1.26: `kindest/node:v1.26.3@sha256:61b92f38dff6ccc29969e7aa154d34e38b89443af1a2c14e6cfbd2df6419c66f`
 - 1.25: `kindest/node:v1.25.8@sha256:00d3f5314cc35327706776e95b2f8e504198ce59ac545d0200a89e69fce10b7f`
 - 1.24: `kindest/node:v1.24.12@sha256:1e12918b8bc3d4253bc08f640a231bb0d3b2c5a9b28aa3f2ca1aee93e1e8db16`
 - 1.23: `kindest/node:v1.23.17@sha256:e5fd1d9cd7a9a50939f9c005684df5a6d145e8d695e78463637b79464292e66c`
 - 1.22: `kindest/node:v1.22.17@sha256:c8a828709a53c25cbdc0790c8afe12f25538617c7be879083248981945c38693`
 - 1.21: `kindest/node:v1.21.14@sha256:27ef72ea623ee879a25fe6f9982690a3e370c68286f4356bf643467c552a3888` 

Additional Images built for this release:
 - 1.27: `kindest/node:v1.27.1@sha256:9915f5629ef4d29f35b478e819249e89cfaffcbfeebda4324e5c01d53d937b09`
 - 1.27: `kindest/node:v1.27.0@sha256:c6b22e613523b1af67d4bc8a0c38a4c3ea3a2b8fbc5b367ae36345c9cb844518`

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images


<!-- Additional images known compatible with this release: -->
<!-- TODO: add 1.27.0 when possible -->

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Fixed iptables rules when the host has iptables 1.8.8+ in nf_tables (not legacy) mode
- Updated all dependencies to latest as of release time (including go version, go modules, containerd, crictl, runc, local-path-provisioner, base images, ...)
  - Package updates are now installed against all packages in the base image when building the kindest/node base image, which should help us stay on top of these
- Fixed containerd snapshotter selection on ZFS hosts
- Generally made containerd snapshotter selection more robust
- Limited haproxy max connections for multiple control-plane node clusters to prevent excessive memory use
- Dedupe nodes correctly in `kind load ...`
- Documented how to configure and use `kubeadmConfigPatches`
- Documented default subnets
- Fixed ingress guide "usage" example
- cgroupsv2 CI updated to Fedora 37
- Updated README and site home to use `go install` and not `go get` (more detailed install docs remain at https://kind.sigs.k8s.io/docs/user/quick-start/#installation)
- Documented rootless podman systemd scope fix




<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @AaronFriel 
- @AkihiroSuda
- @aojea  
- @BenTheElder 
- @carslen 
- @daman1807
- @devenes 
- @dex4er 
- @dlipovetsky 
- @elmiko 
- @em-r 
- @howardjohn 
- @jonas 
- @k8s-ci-robot  
- @ktKongTong 
- @liggitt 
- @lixin963 
- @mattcary 
- @mweibel 
- @mengjiao-liu 
- @nr-dbuckwalter 
- @oxr463 (thank you for your patience!)
- @RijulGulati 
- @rmfitzpatrick
- @saschagrunert  
- @VannTen 


And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏