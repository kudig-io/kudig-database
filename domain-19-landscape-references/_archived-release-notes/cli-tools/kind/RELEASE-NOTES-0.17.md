---
title: kind v0.17 Release Notes
description: kind v0.17 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.17 Release Notes 是什么
- 如何 kind v0.17 Release Notes
trigger_keywords:
- kind
- v0.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kind v0.17 Release Notes

Source: [v0.17.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.17.0)

`v0.17.0` is a small release centered around fixing a bug loading docker hub / docker.io tagged images with identical content but different tags (including images with no explicit host) https://github.com/kubernetes-sigs/kind/pull/2955
and support for cross-architecture `kind load ...`.

This release also ships [[Kubernetes|Kubernetes]] 1.25.3 and [[containerd|containerd]] 1.6.9 with their respective fixes.

This release comes to you live from KubeCon NA 2022 😄 

<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.25.3` image: `kindest/node:v1.25.3@sha256:f52781bc0d7a19fb6c405c2af83abfeb311f130707a0e219175677e366cc45d1`
- Internal config generation now defends against yaml-injection
  - This should not be a breaking change if you were using the config fields as documented
  - This does not apply to config *patches* which are applied after config generation, and by definition patch arbitrary yaml


<h1 id="new-features">New Features</h1>

- **Support for loading cross-architecture images**
  - When using `kind load docker-image` or `kind load image-archive`, 
  kind now instructs containerd to import all architectures.
  - This means that *if* you have multi-arch `docker run` enabled on your host (binfmt_misc qemu-userspace),
  such as in the Docker Desktop application out-of-the box, you may be able to load and run [[Pods|pods]] with images
  for the wrong architecture (e.g. an amd64 image on an M1 mac).
- containerd 1.6.9
- go 1.19.2
- upgraded metallb https://github.com/kubernetes-sigs/kind/pull/2973
- overhauled docs code snippets https://github.com/kubernetes-sigs/kind/pull/2894

New Node images have been built for kind `v0.17.0`, please use these **exact** images (IE like `kindest/node:v1.25.3@sha256:f52781bc0d7a19fb6c405c2af83abfeb311f130707a0e219175677e366cc45d1` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.25: `kindest/node:v1.25.3@sha256:f52781bc0d7a19fb6c405c2af83abfeb311f130707a0e219175677e366cc45d1`
  - 1.24: `kindest/node:v1.24.7@sha256:577c630ce8e509131eab1aea12c022190978dd2f745aac5eb1fe65c0807eb315`
  - 1.23: `kindest/node:v1.23.13@sha256:ef453bb7c79f0e3caba88d2067d4196f427794086a7d0df8df4f019d5e336b61`
  - 1.22: `kindest/node:v1.22.15@sha256:7d9708c4b0873f0fe2e171e2b1b7f45ae89482617778c1c875f1053d4cef2e41`
  - 1.21: `kindest/node:v1.21.14@sha256:9d9eb5fb26b4fbc0c6d95fa8c790414f9750dd583f5d7cee45d92e8c26670aa1`
  - 1.20: `kindest/node:v1.20.15@sha256:a32bf55309294120616886b5338f95dd98a2f7231519c7dedcec32ba29699394`
  - 1.19: `kindest/node:v1.19.16@sha256:476cb3269232888437b61deca013832fee41f9f074f9bed79f57e4280f7c48b7`

Additional images known compatible with this release:
- 1.26: `kindest/node:v1.26.0@sha256:691e24bd2417609db7e589e1a479b902d2e209892a10ce375fab60a8407c7352`

NOTE: These node images support amd64 and arm64. It remains possible to build custom images for other architectures (see the docs).

<h1 id="fixes">Fixes</h1>

- Fix loading docker hub / docker.io tagged images with identical content but different tags (including images with no explicit host) https://github.com/kubernetes-sigs/kind/pull/2955
- [kindnetd](https://github.com/kubernetes-sigs/kind/tree/main/images/kindnetd) (kind's lightweight networking daemonset) now supports removing wrong routes when nodes are added and removed
  - currently, kind does not explicitly have support for adding or removing nodes
    however, [Cluster API Provider Docker](https://github.com/kubernetes-sigs/cluster-api/tree/main/test/infrastructure/docker) (which is based on KIND), does support this.


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @aude
- @BenTheElder
- @chrischdi
- @converge
- @curtbushko
- @flash-me
- @hrittikhere
- @k8s-ci-robot
- @mdurand54
- @raphaelauv
- @Vlatombe

And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏


<!-- risk-assessed -->
