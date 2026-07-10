---
title: calico v3.27 Release Notes
description: calico v3.27 Release Notes — Kubernetes 生产运维知识库
summary: calico v3.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
- calico
- helm
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- calico v3.27 Release Notes 是什么
- 如何 calico v3.27 Release Notes
trigger_keywords:
- calico
- v3.27
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# calico v3.27 Release Notes

Source: [v3.27.5](https://github.com/projectcalico/calico/releases/tag/v3.27.5)

Release notes can be found [on GitHub](https://github.com/projectcalico/calico/blob/v3.27.5/release-notes/v3.27.5-release-notes.md)

> [!IMPORTANT]
> Breaking change: On upgrade, the UID of `projectcalico.org/v3` resources will change. It is recommended that you restart any controllers that may care about this after upgrading Calico, including the kube-controller-manager.

> [!WARNING]
> The v3.27 release Calico uses libraries from [[Kubernetes|Kubernetes]] version v1.27.16 which is end-of-life and is not receiving security updates. There may be unfixed security issues in these Kubernetes libraries. Please consider upgrading to a newer version of Calico to receive the latest security fixes and mitigations.

Attached to this release are the following artifacts:

- `release-v3.27.5.tgz`: container images, binaries, and kubernetes manifests.
- `calico-windows-v3.27.5.zip`: Calico for Windows.
- `tigera-operator-v3.27.5.tgz`: Calico [[Helm|Helm]] v3 chart.
- ocp.tgz: Manifest bundle for OpenShift.

Additional links:

- [VPP data plane release information](https://github.com/projectcalico/vpp-dataplane/blob/master/RELEASE_NOTES.md)



<!-- risk-assessed -->
