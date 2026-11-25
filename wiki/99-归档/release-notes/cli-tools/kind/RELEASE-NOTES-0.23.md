---
title: kind v0.23 Release Notes
description: kind v0.23 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.23 Release Notes 是什么
- 如何 kind v0.23 Release Notes
trigger_keywords:
- kind
- v0.23
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




# kind v0.23 Release Notes

Source: [v0.23.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.23.0)

This release introduces initial limited support for `nerdctl` and kube-proxy nftables mode.


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is now [[Kubernetes|Kubernetes]] 1.30.0: `kindest/node:v1.30.0@sha256:047357ac0cfea04663786a612ba1eaba9702bef25227a794b52890dd8bcd692e`
- `ipFamily` config field is now validated
   - While technically a breaking change, if the value you set here is now caught as invalid it was being silently ignored and creating an ipv4 cluster previously and you should correct your config
-  Go 1.17+ is required for `go install sigs.k8s.io/kind` / building the `kind` binary
   - Prebuilt binaries are available as an alternative to `go install`
   - For local development `make` will automatically setup the correct go version
   - Note that the go team only supports 1.21+ and major linux distros have 1.19+
   - Future releases may increase this to a more current Go release
   - Future release may adopt `toolchain` in go.mod to make this seamless if you have go 1.21+ installed even without our makefile. We highly recommend installing go 1.21+
 
 
<h1 id="new-features">New Features</h1>


- Initial support for nodes created with [nerdctl](https://github.com/containerd/nerdctl)
- Initial support for `kubeProxyMode: nftables` (ahead of Kubernetes 1.31+, see https://kind.sigs.k8s.io/docs/user/configuration/#kube-proxy-mode)
- Sweeping dependency updates, see commits for full details. https://github.com/kubernetes-sigs/kind/compare/v0.22.0...v0.23.0


Images pre-built for this release:

- v1.30.0: `kindest/node:v1.30.0@sha256:047357ac0cfea04663786a612ba1eaba9702bef25227a794b52890dd8bcd692e`
- v1.29.4: `kindest/node:v1.29.4@sha256:3abb816a5b1061fb15c6e9e60856ec40d56b7b52bcea5f5f1350bc6e2320b6f8`
- v1.28.9: `kindest/node:v1.28.9@sha256:dca54bc6a6079dd34699d53d7d4ffa2e853e46a20cd12d619a09207e35300bd0`
- v1.27.13: `kindest/node:v1.27.13@sha256:17439fa5b32290e3ead39ead1250dca1d822d94a10d26f1981756cd51b24b9d8`
- v1.26.15: `kindest/node:v1.26.15@sha256:84333e26cae1d70361bb7339efb568df1871419f2019c80f9a12b7e2d485fe19`
- v1.25.16: `kindest/node:v1.25.16@sha256:5da57dfc290ac3599e775e63b8b6c49c0c85d3fec771cd7d55b45fae14b38d3b`



**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Correctly case `kubeProxyMode: "none"`


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @aojea 
- @AkihiroSuda
- @BenTheElder 
- @dependabot[bot]
- @estesp 
- @hp685   
- @jizusun 
- @k8s-ci-robot
- @kevin85421  
- @stmcginnis 
- @tnqn  
- @yankay 

Thank you as well to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, and helping users in slack!

<!-- risk-assessed -->
