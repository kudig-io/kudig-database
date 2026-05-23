---
title: kind v0.15 Release Notes
description: kind v0.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.15 Release Notes 是什么
- 如何 kind v0.15 Release Notes
trigger_keywords:
- kind
- v0.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kind v0.15 Release Notes

Source: [v0.15.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.15.0)

`v0.15.0` contains important fixes for cluster reboots and various other improvements.


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a [[Kubernetes|Kubernetes]] `v1.25.0` image: `kindest/node:v1.25.0@sha256:428aaa17ec82ccde0131cb2d1ca6547d13cf5fdabcc0bbecf749baa935387cbf`


<h1 id="new-features">New Features</h1>

- New single letter flag aliases `-n` for `--name` and `-A` for `--all`, in-line with `kubectl` etc.
- Optimized image loading to re-tag images when the image contents are identical to previously loaded images but the tags are different
- Support for Kubernetes 1.25, fix for handling rootless + 1.25
- [[containerd|Containerd]] 1.6.7
- Go 1.19
- Updated base image distro to latest, **NOTE**: depend on the contents of the image at your own risk! our images enable running Kubernetes with KIND, we reserve the right to switch distros etc as needed
- Support for Podman 4.0 / netavark
- enhanced pre-release versions to include commit counts


New Node images have been built for kind `v0.15.0`, please use these **exact** images (IE like `kindest/node:v1.25.0@sha256:428aaa17ec82ccde0131cb2d1ca6547d13cf5fdabcc0bbecf749baa935387cbf` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.25: `kindest/node:v1.25.0@sha256:428aaa17ec82ccde0131cb2d1ca6547d13cf5fdabcc0bbecf749baa935387cbf`
  - 1.24: `kindest/node:v1.24.4@sha256:adfaebada924a26c2c9308edd53c6e33b3d4e453782c0063dc0028bdebaddf98`
  - 1.23: `kindest/node:v1.23.10@sha256:f047448af6a656fae7bc909e2fab360c18c487ef3edc93f06d78cdfd864b2d12`
  - 1.22: `kindest/node:v1.22.13@sha256:4904eda4d6e64b402169797805b8ec01f50133960ad6c19af45173a27eadf959`
  - 1.21: `kindest/node:v1.21.14@sha256:f9b4d3d1112f24a7254d2ee296f177f628f9b4c1b32f0006567af11b91c1f301`
  - 1.20: `kindest/node:v1.20.15@sha256:d67de8f84143adebe80a07672f370365ec7d23f93dc86866f0e29fa29ce026fe`
  - 1.19: `kindest/node:v1.19.16@sha256:707469aac7e6805e52c3bde2a8a8050ce2b15decff60db6c5077ba9975d28b98`
  - 1.18: `kindest/node:v1.18.20@sha256:61c9e1698c1cb19c3b1d8151a9135b379657aee23c59bde4a8d87923fcb43a91`

NOTE: These node images support amd64 and arm64. It remains possible to build custom images for other architectures (see the docs).

<h1 id="fixes">Fixes</h1>

- Fixed rebooted node certificates
- Fixed snapshotter selection on ZFS + overlayfs-fuse
- Podman provider now includes node names in no_proxy env, matching the docker provider
- Assorted documentation fixes
- Fixed Kubernetes 1.13 configuration
  - **NOTE**: This will be the last release supporting Kubernetes versions below v1.15.0

<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @a-palchikov
- @AkihiroSuda
- @aojea
- @arielmorelli
- @Benny-Git
- @BenTheElder
- @bryanasdev000
- @cavokz
- @cpanato
- @danwinship
- @harshanarayana
- @jkremser
- @k8s-ci-robot
- @keypointt
- @lepistone
- @lixin963
- @naveensrinivasan
- @pacoxu
- @rewanthtammana
- @tnqn
- @vanhtuan0409
- @wherka-ama
- @zaunist

And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏
