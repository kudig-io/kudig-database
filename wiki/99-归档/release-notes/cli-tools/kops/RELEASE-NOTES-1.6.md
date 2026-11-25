---
title: kops v1.6 Release Notes
description: kops v1.6 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.6 Release Notes 是什么
- 如何 kops v1.6 Release Notes
trigger_keywords:
- kops
- v1.6
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




# kops v1.6 Release Notes

Source: [1.6.2](https://github.com/kubernetes/kops/releases/tag/1.6.2)

*Please see [1.6-NOTES.md](https://github.com/kubernetes/kops/blob/1.6.2/docs/releases/1.6-NOTES.md) for known issues*

* Weave upgraded to 1.9.8, to fix NodePort issue (thanks @jordanjennings, @justinsb)
* Fixes for (experimental) k8s.local DNS-free configurations (thanks @justinsb)
* Weave now configured with the correct pod CIDR (thanks @jordanjennings)
* Initial support for kube-router networking (thanks @murali-reddy)
* Apply cloud-labels to EBS volumes (thanks @pastjean)
* Support empty `--resolv-conf` (thanks @austinmoore-)
* Add --subnet and --role flags to create ig command (thanks @dtan4)
* Improvments to `kops delete` output (thanks @chrislovecnm)
* Match type (public/private) of DNS zones when matching (thanks @justinsb)
* CoreOS command now finds the latest image (thanks @gianrubio)
* Protokube now checks if [[kubelet|kubelet]] is already running before calling systemctl start (thanks @aledbf)
* Added index to make documentation much easier to navigate (thanks @WillemMali)
* Makefile improvements (thanks @WillemMali)
* Refactor instance group / rolling-update code (thanks @andrewsykim)
* Lots of documentation and polish (thanks @chrislovecnm, @cordoval, @justinsb, @WillemMali)

<!-- risk-assessed -->
