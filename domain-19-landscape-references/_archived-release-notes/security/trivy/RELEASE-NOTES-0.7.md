---
title: trivy v0.7 Release Notes
description: trivy v0.7 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- trivy v0.7 Release Notes 是什么
- 如何 trivy v0.7 Release Notes
trigger_keywords:
- trivy
- v0.7
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




# [[Trivy|trivy]] v0.7 Release Notes

Source: [v0.7.0](https://github.com/aquasecurity/trivy/releases/tag/v0.7.0)

## New Feature
### Support [OCI Image Format](https://github.com/opencontainers/image-spec)

An image directory compliant with "Open Container Image Layout Specification".

Buildah:

```
# 🟢 低风险：只读/信息收集，通常无副作用
$ buildah push docker.io/library/alpine:3.11 oci:/path/to/alpine
$ trivy --input /path/to/alpine
```
Skopeo:

```
# 🟢 低风险：只读/信息收集，通常无副作用
$ skopeo copy docker-daemon:alpine:3.11 oci:/path/to/alpine
$ trivy --input /path/to/alpine
```
### [BREAKING] Override severity with vendor [[Score|score]] if exists

Trivy displayed a severity from NVD, which is generic, but it's more accurate to use the severity from vendor such as Red Hat and Debian. Currently, the vendor's severity is preferred than NVD's severity.

**NOTE** If you filter vulnerabilities with `--severity` option, the result may be different because v0.7.0 uses vendor severity.

## Bugs
### rpc: fix output to use templates when in client/server mode. (#469)
A template didn't work in client/server mode.

### fix: handle a scratch/busybox/DockerSlim image gracefully (#476)
Trivy can't detect vulnerabilities of OS packages for an image based on scratch/busybox because those images don't have any package manager such as `yum` and `apt`. But it should detect vulnerabilities of library dependencies according to lock files such as package-lock.json. This commit enables it.

## Changelog

09442d6 chore(ci): move integration tests to GitHub Actions (#485)
415b99d feat: support OCI Image Format (#475)
35b038e chore(github): fix issue templates (#483)
34a95c1 contrib/gitlab.tpl: Add new id field (#468)
b282142 chore(docs): add triage.md (#473)
216a33b fix: handle a scratch/busybox/DockerSlim image gracefully (#476)
ad0bb7c rpc: Fix output to use templates when in client server mode. (#469)
17b84f6 Override with Vendor score if exists (#433)
7629f7f docs: Update installation docs for pointing to Trivy Releases. (#463)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.7.0`
- `docker pull docker.io/aquasec/trivy:latest`


<!-- risk-assessed -->
