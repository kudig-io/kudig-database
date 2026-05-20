---
title: kind v0.24 Release Notes
description: kind v0.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.24 Release Notes 是什么
- 如何 kind v0.24 Release Notes
trigger_keywords:
- kind
- v0.24
- Release
- Notes
- release
- notes
---

# kind v0.24 Release Notes

Source: [v0.24.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.24.0)

Highlights from this release include network policy support using [sigs.k8s.io/kube-network-policies](https://github.com/kubernetes-sigs/kube-network-policies) (thanks @aojea!) and support for building node images from pre-compiled Kubernetes releases (thanks @dims!).

For building images, see the docs at https://kind.sigs.k8s.io/docs/user/quick-start/#building-images


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is now Kubernetes v1.31.0: `kindest/node:v1.31.0@sha256:53df588e04085fd41ae12de0c3fe4c72f7013bba32a20e7325357a1ac94ba865`


<h1 id="new-features">New Features</h1>

- **Out-of-the-box support for network policy via [sigs.k8s.io/kube-network-policies](https://github.com/kubernetes-sigs/kube-network-policies)**
  - Advanced users can still disable kindnetd and install your own CNI with https://kind.sigs.k8s.io/docs/user/configuration/#disable-default-cni, however note that this is a "power user" feature and KIND does not offer support for any third-party CNI
- **Support for building node images from existing Kubernetes releases**
  - **NOTE**: For Kubernetes releases before v1.31.0, this will result in larger images because kind opted in to compiling out remaining in-tree cloud providers with a build tag when publishing images. For v1.31.0+ there is no difference.
  - See: https://kind.sigs.k8s.io/docs/user/quick-start/#building-images
- Support for loading multiple image archives in `kind load image-archive`
  - **NOTE**: it is still more efficient to do a single archive in most cases
- Migrated to skipPhase in Init/JoinConfiguration instead of the kubeadm flags, making it possible for users to cutomize the phases (at your own risk!) with config patches
- powershell completion
- Updated dependencies, including, but not limited to:
  - containerd 1.7.18
  - runc 1.1.13
  - go 1.22.6
  - CNI plugins to 1.5.1
  - pause 3.10
- Docs and clarification for third party install options including scoop and winget


Images pre-built for this release:

- v1.31.0: `kindest/node:v1.31.0@sha256:53df588e04085fd41ae12de0c3fe4c72f7013bba32a20e7325357a1ac94ba865`
- v1.30.4: `kindest/node:v1.30.4@sha256:976ea815844d5fa93be213437e3ff5754cd599b040946b5cca43ca45c2047114`
- v1.30.3: `kindest/node:v1.30.3@sha256:bf91e1ef2f7d92bb7734b2b896b3dddea98f0496b34d96e37dd5d7df331b7e56`
- v1.29.8: `kindest/node:v1.29.8@sha256:d46b7aa29567e93b27f7531d258c372e829d7224b25e3fc6ffdefed12476d3aa`
- v1.29.7: `kindest/node:v1.29.7@sha256:f70ab5d833fca132a100c1f95490be25d76188b053f49a3c0047ff8812360baf`
- v1.28.13: `kindest/node:v1.28.13@sha256:45d319897776e11167e4698f6b14938eb4d52eb381d9e3d7a9086c16c69a8110`
- v1.28.12: `kindest/node:v1.28.12@sha256:fa0e48b1e83bb8688a5724aa7eebffbd6337abd7909ad089a2700bf08c30c6ea`
- v1.27.16: `kindest/node:v1.27.16@sha256:3fd82731af34efe19cd54ea5c25e882985bafa2c9baefe14f8deab1737d9fabe`
- v1.26.15: `kindest/node:v1.26.15@sha256:1cc15d7b1edd2126ef051e359bf864f37bbcf1568e61be4d2ed1df7a3e87b354`
- v1.25.16: `kindest/node:v1.25.16@sha256:6110314339b3b44d10da7d27881849a87e092124afab5956f2e10ecdb463b025`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Fix kubeadm v1beta3 config template for kubeProxyMode: none
- Stop disabling LocalStorageIsolation for rootless clusters (which no longer appears to be necessary to avoid crashes)


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @abhay-krishna
- @aojea
- @BenTheElder
- @bzsuni
- @cpanato
- @dependabot[bot]
- @dominicqi
- @douglaswth
- @giuseppe
- @harshanarayana
- @joycecodes
- @k8s-ci-robot
- @kundan2707
- @netguino
- @nojnhuh
- @pohly
- @ste93cry
- @stmcginnis

Thank you as well to everyone who contributed in other ways like filing issues, giving feedback, testing fixes, and helping users in slack!
