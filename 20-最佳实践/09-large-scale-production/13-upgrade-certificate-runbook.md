---
title: 升级与证书生命周期 Runbook
description: 大规模 Kubernetes 集群版本升级与证书生命周期管理 Runbook：版本策略、skew 规则、升级顺序与预检、节点分批轮换、回滚策略、证书巡检与轮换流程
summary: 可执行的集群升级 Runbook（预检 → 预发 → 控制面 → 节点池 → Addon → 验证）与证书生命周期管理（巡检、轮换、演练）操作手册
category: references
tags:
- k8s
- upgrade
- certificate
- runbook
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
---

# 升级与证书生命周期 Runbook

> 升级是大规模集群风险最高的例行操作：爆炸半径是"整个集群"。本文给出可直接执行的 Runbook。原则：**预发先行、控制面不可回滚所以预检必须苛刻、节点分批、随时可停**。

## 1. 版本策略

| 项目 | 策略 |
|---|---|
| 版本选择 | 社区仍在维护的 N-1 小版本；避开 x.y.0（等 2–3 个 patch） |
| 升级节奏 | 每 6–12 个月一次；落后两个以上小版本即列入技术债 |
| 跨度 | 逐小版本升级（1.30 → 1.31 → 1.32），禁止跨版本跳升 |
| skew 约束 | kubelet 最多落后 apiserver 3 个小版本（1.28 起扩展支持 n-3；更早版本为 n-2，以目标版本官方文档为准）；kube-proxy 与 kubelet 同版；但不要主动利用 skew 长期滞留 |

## 2. 升级前预检（Pre-flight）

| # | 检查项 | 工具/方法 |
|---|---|---|
| 1 | 阅读目标版本 changelog + 云厂商发行说明，整理 API 废弃与行为变更 | 官方文档 |
| 2 | 废弃 API 扫描：集群内是否仍有将移除的 API 版本调用 | Pluto / kubepug / `kubectl get --raw /metrics` 中 `apiserver_requested_deprecated_apis` |
| 3 | PDB 全量核验：所有有状态/关键负载 PDB 就位且允许至少 1 个驱逐 | `kubectl get pdb -A` |
| 4 | 集群健康基线：节点全 Ready、etcd 无 alarm、无进行中的故障 | 监控 Dashboard |
| 5 | 备份：etcd 快照 + Velero 全量备份（升级前 1 小时内执行） | 备份平台 |
| 6 | 证书有效期确认：升级后证书有效期被重置/检查，过期证书先轮换 | `kubeadm certs check-expiration` |
| 7 | Addon 兼容矩阵：CNI/CSI/Ingress/监控组件与目标版本兼容版本清单 | 各组件文档 |
| 8 | 弹性组件确认：CA/Karpenter 允许节点替换（不要被 budget 卡死） | 配置核验 |
| 9 | 容量余量：升级期间滚动腾挪需要额外 headroom（≥ 15%） | 容量视图 |
| 10 | 通知与窗口：变更窗口审批、业务方通知、值班加强 | 流程 |

## 3. 升级执行顺序

```text
预发集群全流程演练（必须）
   │
   ▼
1. 控制面（逐台：drain → 升级 → 验证 → 下一台）
   │  验证点：apiserver /readyz、etcd 成员健康、kubectl 可用
   ▼
2. 节点池（分批轮换，每批 ≤ 5% 节点）
   │  drain（--ignore-daemonsets --delete-emptydir-data）→ 升级/替换 → uncordon
   │  每批后观察 10–15 分钟：业务错误率、Pod 驱逐风暴、调度积压
   ▼
3. Addon（CNI → CSI → Ingress → 监控/日志 Agent → 其他）
   │  注意：Addon 升级顺序错误（如先升级依赖新内核能力的 CNI）会放大故障
   ▼
4. 验证与收尾
```

**托管集群（EKS/GKE/AKS/ACK）注意：**

- 顺序为：控制面 → 托管节点组 → 托管 Addon
- **控制面升级不可回滚**（升级后控制面不可降级）——这是预发验证必须苛刻的根本原因
- 节点组"回滚"方式是新建旧版本节点组并 drain 新节点组
- 节点与控面允许 n-1 skew，但落后两个版本等于跑不受支持的节点软件，应尽快跟上

## 4. 节点分批轮换细则

| 项 | 标准 |
|---|---|
| 批次大小 | ≤ 5% 节点数；首批（金丝雀批）只放 1–2 个节点，观察 30 分钟 |
| 观察指标 | 业务 5xx 率、P99 时延、Pod 重启率、`scheduler_pending_pods`、etcd 延迟 |
| 熔断条件 | 任一核心指标超基线 2 倍 → 停止后续批次，回滚本批 |
| 有状态负载节点 | 单独小批次，逐台确认数据副本重建完成再下一台 |
| 时间预算 | 大集群全量轮换预留 1–2 天，避免赶工跳批 |

## 5. 回滚策略

| 场景 | 回滚方式 |
|---|---|
| 控制面升级失败（kubeadm 自建） | etcd 快照恢复到升级前状态 |
| 控制面升级失败（托管） | 不支持降级——依赖预发拦截；极端情况用备份重建新集群切流 |
| 节点升级问题 | 该批次节点 drain 后回退镜像/版本，或新建旧版节点池替换 |
| Addon 升级问题 | Helm/GitOps 回滚到上一 release |
| 全局性灾难 | 终极路径：新集群 + GitOps 重建 + Velero/快照恢复数据（必须提前演练） |

## 6. 证书生命周期管理

### 6.1 证书台账（每集群必须维护）

| 证书 | 默认有效期 | 管理要点 |
|---|---|---|
| 根 CA（kubernetes / etcd / front-proxy） | 10 年 | 到期前 1 年启动 CA 轮换项目（高风险操作，需专项方案） |
| apiserver / etcd server / kubelet 等叶子证书 | 1 年（kubeadm） | 例行轮换 |
| SA 签名密钥 | 无过期 | 泄露时轮换（需重启控制面 + 重建 token） |

### 6.2 巡检与告警

- 每日巡检：`kubeadm certs check-expiration` 或证书 exporter，剩余 < 90 天告警、< 30 天升级
- kubelet 证书自动轮转确认开启（`rotateCertificates: true` + server TLS bootstrap）

### 6.3 轮换流程（kubeadm 自建集群）

1. 低峰窗口，逐台 Master：`kubeadm certs renew all` → 重启控制面静态 Pod
2. 验证：`kubectl get nodes`、组件日志无 x509 报错
3. 更新所有 kubeconfig 分发（管理员、CI/CD、监控）
4. **年度演练**：完整执行一次全集群证书轮换，验证流程与文档有效性

### 6.4 托管集群

证书由厂商托管轮换，但仍需确认：CA 到期时间（部分托管集群 CA 仍为固定期限）、kubeconfig 自动更新机制、客户端证书缓存刷新策略。

## 7. 升级后验证清单

- [ ] 全部节点 Ready 且版本符合预期（`kubectl get nodes -o wide`）
- [ ] 关键业务 SLI 与基线一致（对比升级前 24h 数据）
- [ ] Addon 版本全部符合兼容矩阵
- [ ] 新废弃 API 告警为零
- [ ] 变更记录归档：实际执行步骤、偏差、耗时、问题清单 → 回写 Runbook

## Related

- [[06-initialization-checklist|初始化配置检查项（证书登记）]]
- [[07-pre-production-checklist|生产上线前检查项（灾备演练）]]
- [[02-cluster-configuration|集群配置最佳实践（版本策略）]]
- [[20-最佳实践/07-scenarios/upgrade-migration|升级迁移场景]]
