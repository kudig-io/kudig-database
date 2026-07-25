---
title: Kubernetes 变更日志索引
description: '# Kubernetes 变更日志索引'
summary: '此外，还包含 19 个 RELEASE-NOTES 文件（v0.4 - v1.1），记录了 Kubernetes 早期版本的关键变更。'
category: entities
tags:
- k8s
- release-notes
- changelog
- kubernetes
- coredns
- docker
- statefulset
- job
- cronjob
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 变更日志索引 是什么
- 如何 Kubernetes 变更日志索引
trigger_keywords:
- Kubernetes
- 变更日志索引
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 变更日志索引

> 本文档是 `生态参考/_archived-release-notes/kubernetes/` 目录下 Kubernetes 变更日志的索引和摘要 ^[inferred]

## CHANGELOG 文件索引

| K8s 版本 | 文件大小 | 说明 |
|---|---|---|
| v1.2 | 42 KB | 早期版本，多容器 Pod 支持 |
| v1.3 | 85 KB | 企业级功能引入 |
| v1.4 | 137 KB | 自动扩缩增强 |
| v1.5 | 140 KB | [[StatefulSet|StatefulSet]]、RBAC alpha |
| v1.6 | 312 KB | 动态供给、[[CronJob|CronJob]] |
| v1.7 | 317 KB | 核心功能扩展 |
| v1.8 | 320 KB | RBAC/NetworkPolicy GA |
| v1.9 | 322 KB | Apps API GA |
| v1.10 | 351 KB | CSI beta、Windows 支持 |
| v1.11 | 337 KB | CoreDNS GA |
| v1.12 | 302 KB | kubeadm GA |
| v1.13 | 281 KB | 调度改进 |
| v1.14 | 279 KB | kubectl GA |
| v1.15 | 286 KB | CRD Webhooks |
| v1.16 | 354 KB | 15 个 GA API |
| v1.17 | 355 KB | 拓扑感知调度 |
| v1.18 | 383 KB | Ephemeral Containers |
| v1.19 | 502 KB | 大规模版本 |
| v1.20 | 420 KB | Docker 弃用警告 |
| v1.21 | 377 KB | PSP 弃用 |
| v1.22 | 466 KB | PSP 移除 |
| v1.23 | 435 KB | 结构化日志 |
| v1.24 | 485 KB | dockershim 移除 |
| v1.25 | 430 KB | PSA GA |
| v1.26 | 436 KB | Sidecar alpha |
| v1.27 | 478 KB | RWOOP |
| v1.28 | 469 KB | 资源健康检查 |
| v1.29 | 441 KB | CEL 验证 |
| v1.30 | 408 KB | 调度优化 |
| v1.31 | 463 KB | 安全增强 |
| v1.32 | 482 KB | 存储网络 |
| v1.33 | 379 KB | 持续演进 |
| v1.34 | 378 KB | 持续演进 |
| v1.35 | 273 KB | 持续演进 |
| v1.36 | 146 KB | 最新版本 |

## RELEASE-NOTES 索引

此外，还包含 19 个 RELEASE-NOTES 文件（v0.4 - v1.1），记录了 Kubernetes 早期版本的关键变更。

## 重大版本里程碑

### 安全与访问控制演进

| 版本 | 变更 | 影响 | 迁移方案 |
|------|------|------|----------|
| v1.8 | RBAC GA | 默认启用 RBAC | 所有集群必须配置 RBAC |
| v1.21 | PSP 弃用 | 警告日志 | 迁移到 PSA/OPA/Kyverno |
| v1.25 | PSP 移除 | API 不可用 | 必须完成 PSA 迁移 |
| v1.25 | PSA GA | 替代 PSP | 配置 Namespace 级 PSA 标签 |
| v1.29 | CEL 验证 | ValidatingAdmissionPolicy GA | 替代部分 Webhook 验证 |

### 容器运行时演进

| 版本 | 变更 | 影响 | 迁移方案 |
|------|------|------|----------|
| v1.20 | dockershim 弃用警告 | 日志警告 | 开始评估 containerd/CRI-O |
| v1.24 | dockershim 移除 | Docker 不可直接使用 | 切换到 containerd 或 cri-dockerd |
| v1.26 | Sidecar alpha | 原生 Sidecar 容器 | 测试 initContainers.restartPolicy=Always |
| v1.28 | Sidecar beta | 生产可用 | 迁移 Sidecar 模式到原生支持 |

### 网络与存储演进

| 版本 | 变更 | 影响 | 迁移方案 |
|------|------|------|----------|
| v1.10 | CSI beta | 存储插件标准化 | 迁移 in-tree 到 CSI |
| v1.11 | CoreDNS GA | 替代 kube-dns | 升级集群 DNS 到 CoreDNS |
| v1.20 | in-tree 云提供商弃用 | 警告 | 迁移到 cloud-provider 外部 |
| v1.26 | in-tree 存储插件移除 | 必须使用 CSI | 确认所有存储使用 CSI 驱动 |

## 升级前检查命令

```bash
# 🟢 检查当前集群版本
kubectl version --short

# 🟢 检查已弃用 API 使用情况
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 🟢 使用 kubent 检查弃用 API
kubent

# 🟢 使用 pluto 扫描弃用 API
pluto detect-all-in-cluster

# 🟡 检查节点就绪状态
kubectl get nodes -o wide

# 🟢 检查 etcd 健康状态
etcdctl --endpoints=https://etcd:2379 endpoint health

# 🟡 模拟升级（dry-run）
kubeadm upgrade plan v1.30.0
```

## 升级最佳实践

### 升级顺序

```
1. 备份 etcd 数据
2. 升级控制平面（API Server → Controller Manager → Scheduler）
3. 逐个升级 Worker 节点（cordon → drain → upgrade → uncordon）
4. 升级 kubectl 客户端
5. 验证集群状态
```

### 版本兼容矩阵

| 组件 | 版本偏差规则 |
|------|----------------|
| kubectl | 与 API Server 偏差 ≤ 1 个 minor |
| kubelet | 与 API Server 偏差 ≤ 2 个 minor |
| kube-proxy | 与 kubelet 同版本 |
| etcd | 与 K8s 版本配套（参考官方文档） |
| CoreDNS | 与 K8s 版本配套 |

### 升级回滚策略

```bash
# 🔴 etcd 备份（升级前必做）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-pre-upgrade.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 🔴 回滚 etcd（升级失败时）
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-pre-upgrade.db \
  --data-dir=/var/lib/etcd-restore

# 🟡 回滚 kubeadm 升级
kubeadm upgrade apply v1.29.0 --force
```

## 使用方式

1. 参考 [[22-概念/12-研究/kubernetes-version-evolution.md|Kubernetes 版本演进]] 了解里程碑版本的关键变更
2. 查看具体 CHANGELOG 文件了解某个版本的完整变更详情
3. 关注弃用和移除的 API，在升级前做好准备
4. 使用 `pluto` 和 `kubent` 工具自动检测弃用 API
5. 每次升级前执行完整的备份和检查流程

## 检查清单

- [ ] 升级前备份 etcd 数据
- [ ] 使用 pluto/kubent 扫描弃用 API
- [ ] 确认版本偏差在兼容范围内
- [ ] 在非生产环境验证升级流程
- [ ] 制定回滚方案并测试
- [ ] 升级后验证所有节点 Ready
- [ ] 升级后检查核心服务运行状态
- [ ] 更新 kubectl 客户端版本

## 来源文档

生态参考/_archived-release-notes/kubernetes/ 目录下全部 54 个文件。

## Related

- [[docker]] — Docker
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[23-实体/02-K8s核心组件/statefulset.md|statefulset]] — StatefulSet
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/12-研究/kubernetes-version-evolution.md|Kubernetes 版本演进]]


<!-- risk-assessed -->
