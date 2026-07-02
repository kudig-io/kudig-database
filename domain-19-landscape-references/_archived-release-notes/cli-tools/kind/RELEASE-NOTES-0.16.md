---
title: kind v0.16 Release Notes
description: kind v0.16 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.16 Release Notes 是什么
- 如何 kind v0.16 Release Notes
trigger_keywords:
- kind
- v0.16
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




# kind v0.16 Release Notes

Source: [v0.16.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.16.0)

`v0.16.0` is a quick release centered around shipping [[Kubernetes|Kubernetes]] v1.25.2 fixes by default. Additional fixes and features are listed below.

<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.25.2` image: `kindest/node:v1.25.2@sha256:9be91e9e9cdf116809841fc77ebdb8845443c4c72fe5218f3ae9eb57fdb4bace`
- kind no longer attempts misguided symlink `/dev/kmsg` to `/dev/console` when `/dev/kmsg` is missing. please ensure your host has `/dev/kmsg` https://github.com/kubernetes-sigs/kind/issues/662#issuecomment-1238911235
- **Kubernetes v1.15.X and lower are no longer supported, as warned in KIND v0.15.0**


<h1 id="new-features">New Features</h1>

- open-iscsi / support for iSCSI volumes
- [[containerd|containerd]] 1.6.8
- crictl 1.25.0
- go 1.19.1

New Node images have been built for kind `v0.16.0`, please use these **exact** images (IE like `kindest/node:v1.25.2@sha256:9be91e9e9cdf116809841fc77ebdb8845443c4c72fe5218f3ae9eb57fdb4bace` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.25: `kindest/node:v1.25.2@sha256:9be91e9e9cdf116809841fc77ebdb8845443c4c72fe5218f3ae9eb57fdb4bace`
  - 1.24: `kindest/node:v1.24.6@sha256:97e8d00bc37a7598a0b32d1fabd155a96355c49fa0d4d4790aab0f161bf31be1`
  - 1.23: `kindest/node:v1.23.12@sha256:9402cf1330bbd3a0d097d2033fa489b2abe40d479cc5ef47d0b6a6960613148a`
  - 1.22: `kindest/node:v1.22.15@sha256:bfd5eaae36849bfb3c1e3b9442f3da17d730718248939d9d547e86bbac5da586`
  - 1.21: `kindest/node:v1.21.14@sha256:ad5b7446dd8332439f22a1efdac73670f0da158c00f0a70b45716e7ef3fae20b`
  - 1.20: `kindest/node:v1.20.15@sha256:45d0194a8069c46483a0e509088ab9249302af561ebee76a1281a1f08ecb4ed3`
  - 1.19: `kindest/node:v1.19.16@sha256:a146f9819fece706b337d34125bbd5cb8ae4d25558427bf2fa3ee8ad231236f2`

NOTE: These node images support amd64 and arm64. It remains possible to build custom images for other architectures (see the docs).

<h1 id="fixes">Fixes</h1>

- Fix for detecting new podman network overlap errors
- Updated metallb docs to current


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @benmoss
- @BenTheElder
- @bornaivankovic
- @fedepaol
- @fukuta-tatsuya-intec
- @k8s-ci-robot

And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏


<!-- risk-assessed -->
