---
title: kind v0.9 Release Notes
description: kind v0.9 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.9 Release Notes 是什么
- 如何 kind v0.9 Release Notes
trigger_keywords:
- kind
- v0.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# kind v0.9 Release Notes

Source: [v0.9.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.9.0)

`v0.9.0` Focuses on stability enhancements following `v0.8.0` / `v0.8.1`, a new wave of features will ship in `v0.10.0`. 

Breaking changes have been kept to a minimum, primarily that [[Kubernetes|Kubernetes]] `v1.12.X` is no longer supported to make way for some fixes requiring beta-grade `kubeadm`.


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.19.1` image: `kindest/node:v1.19.1@sha256:98cf5288864662e37115e362b23e4369c8c4a408f99cbc06e58ac30ddc721600`
- Node images built with kind `v0.9.0` require using kind `v0.9.0+`.
  - These images should mostly work with older releases but may not work offline.
  - Images from kind `v0.8.0+` should still work but lack some internal improvements
- Support has been dropped for Kubernetes older than `v1.13.0`.
  - A detailed support policy is in the works. The Kubernetes project only supports `v1.16+` currently.
  - **UPDATE**:  Kubernetes `v1.14+` is required for multi-node clusters. See https://github.com/kubernetes-sigs/kind/pull/1744#issuecomment-692905480
- Building Kubernetes with bazel will only work with Kubernetes `v1.15+`
  - Building without bazel still works back to `v1.13.0`
- `v1alpha3` kind config is no longer supported. Please upgrade to `v1alpha4` (which was already the current version in previous releases)
- Capital letters in cluster names are explicitly rejected by kind, in order to prevent encountering upstream issues with name validation

**UPDATE**: If you are hitting `label key and value greater than maximum size (4096 bytes), key: [[containerd|containerd]]: invalid argument` (https://github.com/containerd/cri/pull/1572), then try the following KIND config:
```
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
containerdConfigPatches:
- |-
  [plugins."io.containerd.grpc.v1.cri".containerd]
  disable_snapshot_annotations = true
```
See: https://kind.sigs.k8s.io/docs/user/configuration/#getting-started for how to use a KIND config file.

<h1 id="new-features">New Features</h1>

- NFS volumes should now work
- Kubernetes RuntimeConfig is now first class in kind config
- Upgraded dependencies across the board
- Nodes now have a tenative providerID value set. We commit to the `kind://` prefix but the rest of the contents may change.
- The base image is tentatively multi-arch by default. The node image is still not as of yet.
- More values are automatically included in `no_proxy` when proxies are detected
- The default CNI shoult automatically match MTU to the underlying bridge
- Binaries are now stripped of debugger (not stacktrace) info for smaller binaries

New Node images have been built for kind `v0.9.0`, please use these **exact** images (IE like `v1.19.1:@sha256:98cf5288864662e37115e362b23e4369c8c4a408f99cbc06e58ac30ddc721600` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.19: `kindest/node:v1.19.1@sha256:98cf5288864662e37115e362b23e4369c8c4a408f99cbc06e58ac30ddc721600`
  - 1.18: `kindest/node:v1.18.8@sha256:f4bcc97a0ad6e7abaf3f643d890add7efe6ee4ab90baeb374b4f41a4c95567eb`
  - 1.17: `kindest/node:v1.17.11@sha256:5240a7a2c34bf241afb54ac05669f8a46661912eab05705d660971eeb12f6555`
  - 1.16: `kindest/node:v1.16.15@sha256:a89c771f7de234e6547d43695c7ab047809ffc71a0c3b65aa54eda051c45ed20`
  - 1.15: `kindest/node:v1.15.12@sha256:d9b939055c1e852fe3d86955ee24976cab46cba518abcb8b13ba70917e6547a6`
  - 1.14: `kindest/node:v1.14.10@sha256:ce4355398a704fca68006f8a29f37aafb49f8fc2f64ede3ccd0d9198da910146`
  - 1.13: `kindest/node:v1.13.12@sha256:1c1a48c2bfcbae4d5f4fa4310b5ed10756facad0b7a2ca93c7a4b5bae5db29f5`

<h1 id="fixes">Fixes</h1>

- Limited fixes related to HA mode and restart support
- Fixed issues with ipv6 network overlap
- Fixed bugs with NO_PROXY generation
- Mitigated issues with concurrent cluster creation on clean hosts
- KUBECONFIG writing has retries to mitigate concurrency / locking issues
- Docker data root on ZFS should be fixed
- Fixed building with bazel in kubernetes 1.20 development
- Implemented assorted workarounds for breaking bugs in podman v2.X
- Upstream CNI fixes identified by the project have been upstreamed and picked up to mitigate excessive iptables calls in testing
- Replaced broken component IP auto-detection with explicit addresses to work around upstream Kubernetes limitations (pending an agreement on how to move forward upstream)
- Fixed some issues with userns-remap support
- Fixed port forwarding in some cases

<h1 id="contributors">Contributors</h1>

**Thanks again to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @afbjorklund
- @aledbf
- @amwat
- @aojea
- @babilen5
- @BenTheElder
- @damdo
- @dims
- @ericsyh
- @faiq
- @giuseppe
- @HadesArchitect
- @Hellcatlk
- @inercia
- @izzyleung
- @jayunit100
- @k8s-ci-robot
- @liggitt
- @MaXinjian
- @MaxRenaud
- @mikedanese
- @oomichi
- @peteroneilljr
- @prasadkatti
- @rcarrillocruz
- @rikatz
- @SataQiu
- @sozercan
- @steigr
- @tao12345666333
- @vdice
- @yashbhutwala
- @zhijianli88
