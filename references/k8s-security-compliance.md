---
title: 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
description: '# 安全合规'
category: reference
tags:
- k8s
- security
- rbac
- networkpolicy
- runtime-security
- zero-trust
- istio
- falco
- ebpf
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全合规：RBAC、网络安全策略、运行时安全与零信任架构 是什么
- 如何 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
trigger_keywords:
- 安全合规：RBAC
- 网络安全策略
- 运行时安全与零信任架构
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
created: "2026-05-23"
---

# 安全合规

## RBAC 权限模型

四元组：Role（规则）+ Subject（主体）+ Binding（绑定）+ Resource（资源）

- **Role/ClusterRole**：定义可执行的操作
- **RoleBinding/ClusterRoleBinding**：将角色绑定到用户/组/ServiceAccount
- 最小权限原则：为每个工作负载创建独立 ServiceAccount

## Pod Security Standards（PSS）

三个级别：
- **Privileged**：无限制（系统组件）
- **Baseline**：最低限制（阻止已知提权路径）
- **Restricted**：严格限制（安全最佳实践）

替代已废弃的 PodSecurityPolicy（PSP）。

## 运行时安全

- **Falco**：基于系统调用的运行时威胁检测
- **Tetragon**：基于 eBPF 的安全可观测性
- **KubeArmor**：容器级别强制访问控制

## 零信任架构

原则：永不信任，始终验证。
- mTLS 服务间加密通信（Istio/Linkerd）
- 最小权限 RBAC
- NetworkPolicy 默认拒绝所有入站
- 镜像签名验证（Sigstore/Cosign）

---

> 来源：.zread/wiki/drafts/11-an-quan-he-gui-rbac-wang-luo-an-quan-ce-lue-yun-xing-shi-an-quan-yu-ling-xin-ren-jia-gou.md

## Related

- [[entities/tetragon|tetragon]] — Tetragon
- [[istio]] — Istio
- [[falco]] — Falco
- [[linkerd]] — Linkerd
- [[kubearmor]] — KubeArmor

- [[Deployment × Secret 管理]]
- [[synthesis/CNI 插件 × NetworkPolicy|CNI 插件 × NetworkPolicy]]