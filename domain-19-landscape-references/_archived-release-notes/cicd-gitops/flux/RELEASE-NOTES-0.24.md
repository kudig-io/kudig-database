---
title: flux v0.24 Release Notes
description: flux v0.24 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- calico
- helm
- flux
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
- flux v0.24 Release Notes 是什么
- 如何 flux v0.24 Release Notes
trigger_keywords:
- flux
- v0.24
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Flux|flux]] v0.24 Release Notes

Source: [v0.24.1](https://github.com/fluxcd/flux2/releases/tag/v0.24.1)

## Highlights

This version comes with a change to the length of the SHA hex added to the SemVer metadata composed for a `HelmChart` from `GitRepository` and `Bucket` resources with a `Revision` reconcile strategy. Refer to the source-controller changelog for more information.

## Components changelog

- [source-controller v0.19.2](https://github.com/fluxcd/source-controller/blob/v0.19.2/CHANGELOG.md)
- [kustomize-controller v0.18.2](https://github.com/fluxcd/kustomize-controller/blob/v0.18.2/CHANGELOG.md)
- [helm-controller v0.14.1](https://github.com/fluxcd/helm-controller/blob/v0.14.1/CHANGELOG.md)

## CLI changelog

- PR #2195 - @Nalum - Removing [[Kubernetes|Kubernetes]]es API|Kubernetes API]] Request Duration Graph
- PR #2194 - @kingdonb - monitoring: Pin kube-prometheus-stack  to v19.3.0
- PR #2191 - @stefanprodan - Run the ARM64 e2e tests on Equinix hardware
- PR #2178 - @fluxcdbot - Update toolkit components
- PR #2159 - @hiddeco - cmd: start trace short description with T
- PR #2153 - @stefanprodan - e2e: Update Calico to v3.20

## Docker images

- `docker pull fluxcd/flux-cli:v0.24.1`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.24.1`


<!-- risk-assessed -->
