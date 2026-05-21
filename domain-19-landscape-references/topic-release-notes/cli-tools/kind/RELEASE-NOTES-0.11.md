---
title: kind v0.11 Release Notes
description: kind v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.11 Release Notes 是什么
- 如何 kind v0.11 Release Notes
trigger_keywords:
- kind
- v0.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# kind v0.11 Release Notes

Source: [v0.11.1](https://github.com/kubernetes-sigs/kind/releases/tag/v0.11.1)

`v0.11.1` fixes a [security vulnerability in runc <=1.0.0-rc94](https://github.com/opencontainers/runc/security/advisories/GHSA-c3xm-pvg7-gh7r)

For full release notes please see [v0.11.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.11.0)

<h1 id="new-features">New Features</h1>

- Base image updates
   - Containerd sandbox image to pause v3.5
   - Containerd version 1.5.2 / runc rc95
   - Ubuntu 21.04
- Documented support for installing kind via macports.

New Node images have been built for kind `v0.11.1`, please use these **exact** images (IE like `kindest/node:v1.21.1@sha256:fae9a58f17f18f06aeac9772ca8b5ac680ebbed985e266f711d936e91d113bad` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
 - 1.21: `kindest/node:v1.21.1@sha256:69860bda5563ac81e3c0057d654b5253219618a22ec3a346306239bba8cfa1a6`
 - 1.20: `kindest/node:v1.20.7@sha256:cbeaf907fc78ac97ce7b625e4bf0de16e3ea725daf6b04f930bd14c67c671ff9`
 - 1.19: `kindest/node:v1.19.11@sha256:07db187ae84b4b7de440a73886f008cf903fcf5764ba8106a9fd5243d6f32729`
 - 1.18: `kindest/node:v1.18.19@sha256:7af1492e19b3192a79f606e43c35fb741e520d195f96399284515f077b3b622c `
 - 1.17: `kindest/node:v1.17.17@sha256:66f1d0d91a88b8a001811e2f1054af60eef3b669a9a74f9b6db871f2f1eeed00 `
 - 1.16: `kindest/node:v1.16.15@sha256:83067ed51bf2a3395b24687094e283a7c7c865ccc12a8b1d7aa673ba0c5e8861`
 - 1.15: `kindest/node:v1.15.12@sha256:b920920e1eda689d9936dfcf7332701e80be12566999152626b2c9d730397a95`
 - 1.14: `kindest/node:v1.14.10@sha256:f8a66ef82822ab4f7569e91a5bccaf27bceee135c1457c512e54de8c6f7219f8`

Additionally the following image is known to work well:
- 1.22: `kindest/node:v1.22.0@sha256:b8bda84bb3a190e6e028b1760d277454a72267a5454b57db34437c34a588d047`
- 1.23: `kindest/node:v1.23.0@sha256:49824ab1727c04e56a21a5d8372a402fcd32ea51ac96a2706a12af38934f81ac`

NOTE: these node images support amd64 and arm64 now. It remains possible to build custom images for other architectures (see the docs).


