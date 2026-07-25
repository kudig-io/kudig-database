---
title: KUDIG Man Pages Index (references)
description: '| `kubernetes(8)` | Kubernetes 核心组件与架构参考 |'
summary: '| `kubernetes(8)` | Kubernetes 核心组件与架构参考 |'
category: reference
tags:
- k8s
- man-pages
- cli
- tool-reference
- etcd
- prometheus
- istio
- cilium
- helm
- argocd
tier: core
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Man Pages Index 是什么
- 如何 KUDIG Man Pages Index
trigger_keywords:
- KUDIG
- Man
- Pages
- Index
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- tls-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Man Pages Index

> KUDIG 工具命令行参考文档索引

---

## KUDIG CLI 子命令 (Section 1)

| 命令 | 说明 |
|------|------|
| `kudig-fta-viz(1)` | FTA 故障树可视化生成器 |
| `kudig-quality(1)` | 知识库质量检查与评分 |
| `kudig-stats(1)` | 知识库统计信息汇总 |
| `kudig-validate(1)` | 知识库格式与链接校验 |

---

## Kubernetes 生态组件参考 (Section 8)

| 组件 | 说明 |
|------|------|
| `kubernetes(8)` | Kubernetes 核心组件与架构参考 |
| `etcd(8)` | etcd 分布式 KV 存储参考 |
| `containerd(8)` | containerd 容器运行时参考 |
| `helm(8)` | Helm 包管理器参考 |
| `prometheus(8)` | Prometheus 监控系统参考 |
| `cilium(8)` | Cilium eBPF 网络插件参考 |
| `istio(8)` | Istio 服务网格参考 |
| `argocd(8)` | ArgoCD GitOps 工具参考 |
| `cert-manager(8)` | cert-manager 证书管理参考 |
| `velero(8)` | Velero 备份恢复工具参考 |

---

## 安装与使用

```bash
# 查看 KUDIG 子命令帮助
man 1 kudig-fta-viz
man 1 kudig-quality
man 1 kudig-stats
man 1 kudig-validate

# 查看组件参考
man 8 kubernetes
man 8 etcd
```

---

## 相关文档

- [[23-实体/15-参考与索引/KUDIG Man Pages Index.md|原版 Man Pages]]
- [[23-实体/15-参考与索引/kubectl Scenario Quick Reference.md|kubectl 场景速查]]
- 模板与 Prompt 集合

## Related

- [[etcd]] — etcd
- [[cert-manager]] — cert-manager
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/08-交付与制品/argocd.md|argocd]] — ArgoCD


<!-- risk-assessed -->
