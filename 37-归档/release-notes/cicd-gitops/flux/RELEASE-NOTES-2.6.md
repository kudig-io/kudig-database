---
title: flux v2.6 Release Notes
description: flux v2.6 Release Notes — Kubernetes 生产运维知识库
summary: flux v2.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
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
- flux v2.6 Release Notes 是什么
- 如何 flux v2.6 Release Notes
trigger_keywords:
- flux
- v2.6
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




# [[Flux|flux]] v2.6 Release Notes

Source: [v2.6.4](https://github.com/fluxcd/flux2/releases/tag/v2.6.4)

## Highlights

Flux v2.6.4 is a patch release that comes with various fixes. Users are encouraged to upgrade for the best experience.

Fixes:

- Fix for [[SOPS|SOPS]] decryption with US Government KMS keys failing with the error:

```
STS: AssumeRoleWithWebIdentity, https response error\n   StatusCode: 0, RequestID: ,
request send failed, Post\n \"https://sts.arn.amazonaws.com/\": dial tcp:
lookupts.arn.amazonaws.com on 10.100.0.10:53: no such host
```

## Components changelog

- kustomize-controller [v1.6.1](https://github.com/fluxcd/kustomize-controller/blob/v1.6.1/CHANGELOG.md)

## CLI changed
* [release/v2.6.x] Update toolkit components by @fluxcdbot in https://github.com/fluxcd/flux2/pull/5444


**Full Changelog**: https://github.com/fluxcd/flux2/compare/v2.6.3...v2.6.4



<!-- risk-assessed -->
