---
title: kind v0.13 Release Notes
description: kind v0.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.13 Release Notes 是什么
- 如何 kind v0.13 Release Notes
trigger_keywords:
- kind
- v0.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# kind v0.13 Release Notes

Source: [v0.13.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.13.0)

`v0.13.0` is all about cgroups -- We're making the switch to the systemd cgroup driver to align with current [Kubernetes container runtime recommendations](https://kubernetes.io/docs/setup/production-environment/container-runtimes/) and [kubeadm defaults](https://github.com/kubernetes/kubeadm/issues/2376).

<h1 id="breaking-changes">Breaking Changes</h1>

- **systemd cgroups driver will be used for Kubernetes v1.24.0+** (rather than 1.21.0+ when [kubeadm changed the default](https://github.com/kubernetes/kubernetes/pull/99471), which we previously overrode).
  - **NOTE**: **You must use kind v0.13.0+ with Kubernetes v1.24.0+ images**, and if you built your own Kubernetes v1.24.0+ image
with a previous kind version you will need to re-built when switching to kind v0.13.0+.
  - **NOTE**: ~**You do not need to be using systemd on the host machine**~, systemd is used *inside* the kind node containers. We are now using it for Kubernetes pods in addition to running kubelet, containerd etc.
     - **UPDATE**: There is a bug on hosts that are (cgroupv1, not-systemd) https://github.com/kubernetes-sigs/kind/issues/2765, a fix is pending and [the next release](https://github.com/kubernetes-sigs/kind/releases/tag/v0.14.0) will address this. See the linked issue for discussion and workarounds in the meantime.
     - **UPDATE**: [v0.14.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.14.0) is released with a fix for this bug. Upgrading should be the preferred solution, but if you need to use v0.13.0 for some reason, see the issue linked above for other workarounds.
  - KIND will **continue to use cgroupfs for Kubernetes versions prior to v1.24.0**.
- The default node image is a Kubernetes `v1.24.0` image: `kindest/node:v1.24.0@sha256:406fd86d48eaf4c04c7280cd1d2ca1d61e7d0d61ddef0125cb097bc7b82ed6a1`


<h1 id="new-features">New Features</h1>

- Limited support for PPC64LE architecture (you will need to build your own node images)
- `kind export logs` now dumps a list of images on each node
- Base image updates
  - Containerd version 1.6.4
  - CNI plugins 1.1.1
- kind binary built with Go 1.18
- General Go dependency updates
- [registry.k8s.io](https://github.com/kubernetes/k8s.io/wiki/New-Registry-url-for-Kubernetes-(registry.k8s.io)) is used as the primary mirror for k8s.gcr.io in kind nodes / image building
  - If registry.k8s.io is not reachable, the node runtime is configured to fallback to k8s.gcr.io directly as the next endpoint

New Node images have been built for kind `v0.13.0`, please use these **exact** images (IE like `kindest/node:v1.24.0@sha256:406fd86d48eaf4c04c7280cd1d2ca1d61e7d0d61ddef0125cb097bc7b82ed6a1` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.24: `kindest/node:v1.24.0@sha256:406fd86d48eaf4c04c7280cd1d2ca1d61e7d0d61ddef0125cb097bc7b82ed6a1`
  - 1.23: `kindest/node:v1.23.6@sha256:1af0f1bee4c3c0fe9b07de5e5d3fafeb2eec7b4e1b268ae89fcab96ec67e8355`
  - 1.22: `kindest/node:v1.22.9@sha256:6e57a6b0c493c7d7183a1151acff0bfa44bf37eb668826bf00da5637c55b6d5e`
  - 1.21: `kindest/node:v1.21.12@sha256:ae05d44cc636ee961068399ea5123ae421790f472c309900c151a44ee35c3e3e`
  - 1.20: `kindest/node:v1.20.15@sha256:a6ce604504db064c5e25921c6c0fffea64507109a1f2a512b1b562ac37d652f3`
  - 1.19: `kindest/node:v1.19.16@sha256:dec41184d10deca01a08ea548197b77dc99eeacb56ff3e371af3193c86ca99f4`
  - 1.18: `kindest/node:v1.18.20@sha256:38a8726ece5d7867fb0ede63d718d27ce2d41af519ce68be5ae7fcca563537ed`

NOTE: these node images support amd64 and arm64. It remains possible to build custom images for other architectures (see the docs).

<h1 id="fixes">Fixes</h1>

- In cgroup v1, unmount cgroups that are not supported by the runtime used to create nodes
- Pinned metallb to a stable version in documentation

<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

Contributors since v0.12.0:

- @aojea
- @BenTheElder
- @chendave
- @dims
- @iamtodor
- @jpmcb
- @k8s-ci-robot
- @kolyshkin
- @mkumatag
- @pacoxu
- @stmcginnis
- @yxxhero
- @zaunist
