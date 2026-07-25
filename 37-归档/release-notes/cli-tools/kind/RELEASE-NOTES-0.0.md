---
title: kind v0.0 Release Notes
description: kind v0.0 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.0 Release Notes 是什么
- 如何 kind v0.0 Release Notes
trigger_keywords:
- kind
- v0.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kind v0.0 Release Notes

Source: [0.0.1](https://github.com/kubernetes-sigs/kind/releases/tag/0.0.1)

### UPDATE 2019-02-11

Due to [CVE-2019-5736](https://access.redhat.com/security/vulnerabilities/runcescape) ("the runc escape")

`kindest/node:v1.12.5@sha256:2cc6e8153dbe32db0b962cda060e193615951ae8d2a0b808535726a9b6b5e583` 
or
`kindest/node:v1.13.2@sha256:d070e091e7c0e515a37d0834ea72828b2338acdc4bc7a13cbb84274fb14e5e83` 
or:
`kindest/node:v1.13.3@sha256:d1af504f20f3450ccb7aed63b67ec61c156f9ed3e8b0d973b3dee3c95991753c`

Should be used instead of the default image in this release. Example:

```console
kind create cluster --image=kindest/node:v1.13.3@sha256:d1af504f20f3450ccb7aed63b67ec61c156f9ed3e8b0d973b3dee3c95991753c
```

In the future we are looking at not pinning to an **exact** image by default, so that minimal fixes can be silently pushed.
We are additionally hoping to adopt [rootless containers](https://rootlesscontaine.rs/) once support lands in [[Kubernetes|Kubernetes]].

Please consider upgrading to kind [0.1.0](https://github.com/kubernetes-sigs/kind/releases/tag/0.1.0) or newer as well.

----

**UPDATE**:  Kubernetes `v1.12.2` is affected by [CVE-2018-1002105](https://github.com/kubernetes/kubernetes/issues/71411); when using `kind create cluster` please set the image to a non-default image via the `--image` flag. The following Kubernetes `v1.12.3` image should be suitable: `kindest/node:v1.12.3@sha256:f0ecb1066697d9417365ca58410132e512ce2010763470bb28c1e8f7fef55464` 

A patch release will be made to upgrade this default, see https://github.com/kubernetes-sigs/kind/issues/180.

-----

`kind` is still alpha-grade software, and as such breaking changes will be made to future releases.

This release allows early users to pin to these pre-built binaries and avoid these changes until they intend to upgrade, rather than installing from HEAD with `go get  ...`

NOTE: the default `kind create cluster` pins to a specific node image in order to avoid breakage there.

This image is: `kindest/node:v1.12.2@sha256:6ac1dc1750fc0efd13d4e294115f9012a21282957e4380a5535bd32154193d4d` on [the current official registry](https://hub.docker.com/r/kindest/node/).

[Contributors](https://github.com/kubernetes-sigs/kind/graphs/contributors) to this release are:
- @BenTheElder 
- @munnerz 
- @alejandrox1 
- @tao12345666333 
- @neolit123 
- @fabriziopandini 
- @Lion-Wei 
- @mooncak 
- @radu-matei 
- @AdamDang 
- @cblecker 

Thank you for your contributions!

<!-- risk-assessed -->
