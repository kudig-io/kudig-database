---
title: etcd × IaC
summary: etcd × IaC：etcd与IaC是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- platform
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[23-实体/08-交付与制品/helm.md]]'
  type: uses
- target: '[[23-实体/07-可观测性/prometheus.md]]'
  type: uses
- target: '[[23-实体/08-交付与制品/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd × IaC

## 概述
IaC 工具（Terraform、Pulumi）通过 apiserver 间接操作 etcd 中存储的资源。同时，etcd 基础设施本身（虚拟机、磁盘、网络、安全组）也经常由 IaC 管理。这两层关系构成了 "IaC 管理 etcd 基础设施" 和 "IaC 通过 etcd 管理 K8s 资源" 的双向依赖。理解这层关系对于设计可靠的集群基础设施自动化方案至关重要。

## 技术关联机制

1. **IaC 管理 etcd 基础设施**：在使用自建 Kubernetes 集群时，etcd 节点的虚拟机、磁盘、安全组规则通常由 Terraform 管理。Terraform 负责创建 etcd 节点的 VM、挂载高性能 SSD 磁盘、配置节点间安全组放行 2379/2380 端口、配置 DNS 解析。这些基础设施的变更是 etcd 集群稳定性的基础。

2. **IaC 管理 K8s 资源 → 写入 etcd**：当 Terraform Kubernetes Provider 执行 `terraform apply` 时，通过 apiserver 向 etcd 写入资源对象。Terraform 的 state 文件记录了"上次 apply 后的期望状态"，而 etcd 记录了"集群的实际状态"。两者之间的同步依赖 Terraform 的 plan-apply 循环。

3. **State 漂移与 etcd**：如果有人手动通过 kubectl 修改了 etcd 中的资源（绕过 Terraform），Terraform 在下一次 `terraform plan` 时通过 apiserver 读取 etcd 中的当前状态，检测到与 state 文件的差异（漂移）。但 Terraform 不会自动修复漂移——需要人工确认后执行 apply。

4. **etcd 灾难恢复与 IaC**：当 etcd 发生灾难性故障需要重建时，IaC 工具可以快速重建 etcd 基础设施（VM/磁盘/网络），然后通过 etcdctl 恢复数据，最后通过 Terraform Kubernetes Provider 重新 apply 集群级资源（Namespace/RBAC/StorageClass）作为基线配置。这个 IaC 驱动的恢复流程比纯手动操作更可靠、更可重复。

## 实践场景

- **集群全生命周期管理**：Terraform 管理 VPC/安全组/etcd 节点/worker 节点 → kubeadm 初始化集群 → Terraform apply 基础设施资源（RBAC/Namespace/StorageClass）
- **etcd 磁盘扩容**：通过 Terraform 修改 etcd 节点的磁盘大小 → VM 重建/重启 → 文件系统扩容 → etcd 可用空间增加
- **多集群基线**：使用 Terraform modules 为 dev/staging/prod 集群 apply 统一的 RBAC 和 NetworkPolicy 基线，确保安全策略一致
- **灾备重建**：etcd 故障后，使用 Terraform 快速创建新的 etcd 节点，恢复快照，重新 apply 基础资源

## 常见问题

### 问题1：Terraform apply 后 etcd 中资源与 state 不一致
**症状**：Terraform apply 成功但 kubectl 查看到的资源与预期不同
**根因**：Controller 在资源创建后修改了默认值（如添加 default labels/annotations），Terraform state 未更新
**修复**：执行 `terraform apply -refresh-only` 更新 state；使用 `ignore_changes` 忽略自动管理的字段

### 问题2：Terraform 无法连接 apiserver（etcd 故障）
**症状**：`terraform plan` 报错连接 apiserver 超时
**根因**：etcd 故障导致 apiserver 不可用，Terraform Provider 无法读写资源
**修复**：优先恢复 etcd；在恢复期间使用 `terraform import` 记录手动创建的资源

### 问题3：IaC 管理的 etcd 基础设施变更导致 etd 不稳定
**症状**：Terraform 修改 etcd 节点配置后 etcd 集群出现 leader 选举频繁或成员不可达
**根因**：安全组规则变更未放行 2380 端口（peer 通信）；或 VM 规格变更导致重启
**修复**：确保安全组规则正确放行 2379/2380；etcd 节点变更逐个进行（滚动更新），确保 Quorum 不丢失

## 关键命令

```bash
# 🟢 检查 Terraform 管理的 K8s 资源
terraform state list | grep kubernetes_

# 🟢 验证 etcd 基础设施状态
terraform show <etcd_module>

# 🟢 刷新 Terraform state 与 etcd 的同步
terraform apply -refresh-only

# 🟢 检查 etcd 健康状态
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint health

# 🟡 通过 Terraform 扩容 etcd 磁盘
terraform apply -var="etcd_disk_size=500GB"
```

## 权衡取舍

| 维度 | etcd 倾向 | IaC 倾向 | 权衡点 |
|------|----------|---------|--------|
| 基础设施管理 | IaC 管理确保一致 | 手动操作快速灵活 | 一致性 vs 灵活性 |
| 变更审批 | 自动 apply 快速生效 | plan→review→apply 安全 | 效率 vs 安全性 |
| State 管理 | etcd 是运行时真实状态 | Terraform state 是历史快照 | 实时性 vs 可追溯性 |
| 灾备重建 | etcd 快照恢复数据 | IaC 重建基础设施 | 数据恢复 vs 基础恢复 |

## 最佳实践
1. 使用 Terraform 管理 etcd 基础设施（VM/磁盘/网络），确保基础设施可重复创建
2. etcd 节点变更使用滚动更新策略，逐个修改并验证 Quorum 健康后再变更下一个
3. 将 Terraform state 存储在远程 backend（S3+DynamoDB），与 etcd 快照分开存储
4. 定期执行 `terraform plan -detailed-exitcode` 检测配置漂移，确保 IaC state 与 etcd 实际状态一致

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- IaC
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
