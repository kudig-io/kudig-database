---
title: flux v0.11 Release Notes
description: flux v0.11 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.11 Release Notes 是什么
- 如何 flux v0.11 Release Notes
trigger_keywords:
- flux
- v0.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Flux|flux]] v0.11 Release Notes

Source: [v0.11.0](https://github.com/fluxcd/flux2/releases/tag/v0.11.0)

CHANGELOG
- PR #1164 - @fluxcdbot - Update toolkit components
- PR #1163 - @dholbach - Fix cmd links
- PR #1162 - @hiddeco - Tidy up command descriptions
- PR #1157 - @hiddeco - Allow supplying PK from file for Git source/secret
- PR #1156 - @Legion2 - Fix CRD deletion instruction in [[Helm|Helm]] Operator migration docs
- PR #1150 - @stefanprodan - Update dev guide to controller-runtime v0.8
- PR #1149 - @SomtochiAma - Refactor remaining create, delete, export, get cmd to use adapter
- PR #1141 - @hiddeco - Add frontmatter to command documentation
- PR #1134 - @kingdonb - Fixup a broken reference and a typo in Azure doc
- PR #1128 - @jestallin - Remove branch switch for image update cmd in guide
- PR #1122 - @stefanprodan - Add AWS IAM role binding example to [[SOPS|SOPS]] guide
- PR #1119 - @mfamador - Azure ACR reconcile script fix parameter order



<!-- risk-assessed -->
