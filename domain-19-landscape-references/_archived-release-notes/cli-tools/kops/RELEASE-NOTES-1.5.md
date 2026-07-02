---
title: kops v1.5 Release Notes
description: kops v1.5 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flannel
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.5 Release Notes 是什么
- 如何 kops v1.5 Release Notes
trigger_keywords:
- kops
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kops v1.5 Release Notes

Source: [1.5.3](https://github.[[entities/kubernetes.md|kubernetes]]/kops/releases/tag/1.5.3)

* **Important for Terraform Users** Make ELB naming unambiguous by including the full cluster name.  This will cause the ELBs to be recreated if using Terraform with private topologies, causing disruption of external access to the API and of external access to the bastion (if enabled).  Expected disruption is less than 5 minutes.  Use `export KOPS_FEATURE_FLAGS=+UseLegacyELBName` to keep the legacy naming and avoid disruption.  Fix #1899

* Fix terraform output of shared subnets.  Fix #1977
* Add support for i3 instances (thanks @geojaz)

* Experimental drain rolling-update, 
* Experimental GCE support

* Update Weave to v1.9.3
* Put flannel in guaranteed class (thanks @mihok)
* DNS autoscaler fixes (thanks @MrHohn)
* Remove legacy flags (thanks @mtaufen)
* Add route53 mapper addon (thanks @itskingori)
* Build fixes (thanks @zmerlynn)
* Disable cloudformation delete (thanks @kris-nova)

* Docs fixes (thanks @bowei, @jonchiu, @dosullivan, @DualSpark, @foxylion, @kris-nova


<!-- risk-assessed -->
