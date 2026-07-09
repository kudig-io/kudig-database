---
title: 多集群与灾备生产模式
description: 生产级多集群架构与 DR：Active-Active/Active-Passive 拓扑、跨集群流量切换、RTO/RPO 设计与 Velero 灾备实践
summary: 生产级多集群架构与 DR：Active-Active/Active-Passive 拓扑、跨集群流量切换、RTO/RPO 设计与 Velero 灾备实践，含灾备演练与故障切换清单。
category: application-patterns
tags:
- multi-cluster
- disaster-recovery
- active-active
- velero
- rto-rpo
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- K8s 多集群灾备怎么做
- Active-Active 和 Active-Passive 区别
trigger_keywords:
- 多集群
- 灾备
- Active-Active
- RTO
- RPO
- Velero
- 故障切换
prerequisites:
- kubectl-basics
- multi-cluster-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含涉及故障切换和数据恢复的操作。切换前务必确认数据同步状态和回滚方案。命令风险等级：🔴 高风险（可能导致数据不一致或服务中断）、🟡 中风险、🟢 低风险/只读。

# 多集群与灾备生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

单集群故障（控制平面崩溃、可用区整体瘫痪、云厂商区域中断）是 P0 级灾难。多集群架构通过在物理隔离的集群间冗余部署，保障极端故障下的业务连续性。本文涵盖 Active-Active/Active-Passive 拓扑选型、跨集群流量切换、RTO/RPO 设计和 Velero 灾备实践。

---

## 1. 拓扑选型

### 1.1 三种基础拓扑

| 拓扑 | 工作方式 | RTO | 成本 | 复杂度 | 适用场景 |
|---|---|---|---|---|---|
| **Active-Passive** | 主集群承载流量，备集群热待命 | 分钟级 | 中（1.5x） | 中 | 数据库、强一致服务 |
| **Active-Active** | 多集群同时承载流量，负载均衡 | 接近零 | 高（2x+） | 高 | 无状态 API、CDN 边缘 |
| **Pilot-Light** | 核心数据持续复制，应用按需拉起 | 分钟-小时 | 低 | 低 | 成本敏感的 DR |

### 1.2 决策矩阵

```
应用是否无状态?
  ├─ 是 → Active-Active（最高可用）
  │     └─ 数据层需多活同步（复杂，评估 ROI）
  └─ 否 → 应用层 Active-Active + 数据层 Active-Passive（最常见）
        └─ 或全栈 Active-Passive（最简单）

容灾预算（能容忍多长时间中断）?
  ├─ < 1 分钟 → Active-Active + 全局负载均衡
  ├─ 1-15 分钟 → Active-Passive + 自动切换
  └─ > 15 分钟 → Pilot-Light + 手动拉起
```

---

## 2. Active-Active 架构

### 2.1 流量分发层

```yaml
# 全局负载均衡（DNS 层 / Anycast）
# 使用 ExternalName Service 或 Global Accelerator 将流量分发到多集群
apiVersion: v1
kind: Service
metadata:
  name: api-global
spec:
  type: ExternalName
  externalName: api.global-lb.example.com   # 全局 LB 按健康检查分发
---
# 每个集群的 Gateway 配置健康检查
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: api-gateway
spec:
  listeners:
    - name: http
      port: 80
      protocol: HTTP
      allowedRoutes:
        namespaces:
          from: All
```

### 2.2 跨集群服务发现

| 方案 | 工具 | 延迟 | 复杂度 |
|---|---|---|---|
| **DNS 轮询** | CoreDNS + 多集群 endpoint | 低 | 低 |
| **Service Mesh 多集群** | Istio Multi-Cluster / Cilium Cluster Mesh | 低 | 高 |
| **应用层路由** | Spring Cloud Gateway / Kong | 中 | 中 |

### 2.3 数据一致性挑战

Active-Active 的核心难点是数据层。无状态应用可以自由 Active-Active，但有状态数据需要：

| 数据类型 | 多活策略 | 工具 |
|---|---|---|
| 缓存（Redis） | 各集群独立缓存，失效广播 | Redis Pub/Sub |
| 数据库（PostgreSQL/MySQL） | 异步双向复制（谨慎）或读多写单 | PostgreSQL 逻辑复制 |
| 消息队列（Kafka） | MirrorMaker / Cluster Linking 跨集群复制 | Kafka MirrorMaker 2 |
| 对象存储 | 跨区域复制 | S3 Cross-Region Replication |

> ⚠️ **Active-Active 数据红线**: 双写冲突是最危险的故障模式。除非数据层原生支持多活冲突解决（如 CRDT、DynamoDB Global Tables），否则推荐"应用多活 + 数据单写 + 读副本"模式。

---

## 3. Active-Passive 架构与故障切换

### 3.1 同步策略

| 层级 | 同步方式 | RPO | 工具 |
|---|---|---|---|
| **配置（GitOps）** | Git 仓库共享，多集群 ArgoCD 同步 | ~0 | Argo CD ApplicationSet |
| **密钥** | External Secrets Operator 统一管理 | ~0 | ESO + Vault/AWS SM |
| **持久化数据** | 定期快照 + 持续复制 | 分钟级 | Velero + CSI Snapshot / 存储层复制 |
| **数据库** | 异步流复制 | 秒级 | PostgreSQL streaming replication |

### 3.2 故障切换 Runbook

```bash
# 🔴 高风险：Active-Passive 故障切换
# 执行前确认：主集群确实不可用 + 备集群数据同步已追平

# Step 1: 🟢 确认主集群不可用（排除网络抖动）
for i in 1 2 3; do
  kubectl --context=primary get --raw=/healthz
  sleep 10
done
# 连续 3 次失败 → 确认故障

# Step 2: 🟢 确认备集群健康且数据已同步
kubectl --context=standby get nodes
kubectl --context=standby exec -it <db-pod> -- pg_is_in_recovery  # 应返回 t (replica)
# 检查复制延迟: SELECT * FROM pg_stat_replication;

# Step 3: 🟡 提升备集群为新的写入端
# 数据库 promote
kubectl --context=standby exec -it <db-pod> -- pg_ctl promote

# Step 4: 🟡 全局 DNS/LB 切换流量到备集群
# 更新 DNS TTL 提前调低（切换前 24h 设为 60s）
# DNS 记录指向备集群入口

# Step 5: 🟢 验证服务
curl -f https://api.example.com/healthz

# Step 6: 记录切换事件，准备反向切换（原主恢复后）
```

> ⚠️ **DNS 切换延迟**: DNS TTL 决定了客户端感知切换的最大延迟。生产建议 TTL ≤ 60s。切换前 24 小时预先调低 TTL。考虑使用 Global Accelerator / Anycast 替代 DNS 以实现更快的切换。

---

## 4. Velero 灾备实践

### 4.1 备份策略

```bash
# 🟡 定期全量备份（CronJob 化）
velero schedule create daily-backup \
  --schedule="0 2 * * *" \
  --include-namespaces production \
  --snapshot-volumes=true \
  --ttl=720h        # 保留 30 天

# 🟡 备份前钩子（确保数据一致性）
velero backup create db-backup \
  --include-namespaces production \
  --hook-backup-configmap=backup-hooks \
  --snapshot-volumes=true
```

```yaml
# 备份钩子：备份前 fsync 数据库
apiVersion: v1
kind: ConfigMap
metadata:
  name: backup-hooks
data:
  postgres.hook.backup.kubernetes.io/command: '["/bin/bash", "-c", "pg_dump -F c -f /tmp/dump.psql"]'
  postgres.hook.backup.kubernetes.io/timeout: 5m
```

### 4.2 跨集群恢复

```bash
# 🔴 高风险：跨集群恢复（务必在隔离环境验证）
# Step 1: 在目标集群配置备份存储位置
velero backup-location create shared-backup \
  --provider aws \
  --bucket velero-backups \
  --config region=us-east-1

# Step 2: 同步备份元数据
velero backup get

# Step 3: 恢复到目标集群（先 namespace 映射避免覆盖）
velero restore create --from-backup daily-backup-20260702 \
  --namespace-mappings production:production-restored

# Step 4: 🟢 验证恢复完整性
kubectl get all -n production-restored
```

> ⚠️ **Velero 恢复陷阱**: PV 恢复依赖 CSI 快照或 restic。跨云恢复（如 AWS→阿里云）CSI 快照不兼容，必须用 restic 做文件级备份。生产 DR 跨云场景必须预先验证 restic 恢复。

---

## 5. RTO/RPO 设计

### 5.1 指标定义

| 指标 | 含义 | 影响 |
|---|---|---|
| **RTO** (Recovery Time Objective) | 从故障到恢复服务的最大容忍时间 | 切换速度 |
| **RPO** (Recovery Point Objective) | 可容忍的最大数据丢失窗口 | 同步频率 |

### 5.2 RTO/RPO 对应架构

| RTO | RPO | 所需架构 | 成本 |
|---|---|---|---|
| < 1 分钟 | ~0 | Active-Active + 全局 LB | ⭐⭐⭐⭐⭐ |
| < 5 分钟 | < 1 分钟 | Active-Passive + 自动切换 + 流复制 | ⭐⭐⭐ |
| < 30 分钟 | < 15 分钟 | Active-Passive + 定期快照 | ⭐⭐ |
| < 4 小时 | < 1 小时 | Pilot-Light + Velero 恢复 | ⭐ |

---

## 6. 灾备演练

### 6.1 演练清单

| # | 演练项 | 频率 | 验证目标 |
|---|---|---|---|
| 1 | 备集群拉起 | 季度 | 应用能在 RTO 内启动 |
| 2 | 数据库故障切换 | 季度 | promote 后数据一致 |
| 3 | DNS/LB 流量切换 | 季度 | 客户端在 TTL 内感知 |
| 4 | Velero 跨集群恢复 | 半年 | PV 数据完整恢复 |
| 5 | 全链路故障注入 | 年度 | 模拟主集群完全瘫痪 |
| 6 | 回切（Failback） | 年度 | 原主恢复后安全切回 |

### 6.2 演练记录模板

```
演练名称: 2026Q3 主集群故障切换演练
演练时间: 2026-07-02 14:00-15:30
演练范围: payment-service (Active-Passive)
RTO 目标: ≤ 5 分钟 | 实际: 3 分 42 秒 ✅
RPO 目标: ≤ 1 分钟 | 实际: 8 秒 ✅
发现问题:
  1. DNS TTL 切换前未提前调低，部分客户端 5 分钟才生效
  2. 备集群 HPA minReplicas 配置偏低，切换后冷启动慢
改进项:
  1. 切换 SOP 增加"提前 24h 调低 TTL"
  2. 备集群 minReplicas 调至与主集群一致
```

---

## 7. 生产检查清单

| # | 检查项 | 验证方法 | 合格标准 |
|---|---|---|---|
| 1 | 灾备拓扑已定义 | 架构文档 | 每个核心服务有明确的 Active-Active/Passive |
| 2 | RTO/RPO 已定义 | SLO 文档 | 每个服务有明确数值 |
| 3 | 备集群配置同步 | ArgoCD 同步状态 | GitOps 配置多集群一致 |
| 4 | 密钥跨集群可用 | ESO 状态 | 备集群密钥可解析 |
| 5 | 数据库复制正常 | 复制延迟监控 | lag < 5s |
| 6 | DNS TTL 已调低 | dig 输出 | ≤ 60s |
| 7 | 灾备演练已执行 | 演练记录 | 半年内有成功演练记录 |
| 8 | Velero 备份可用 | restore dry-run | 季度验证恢复成功 |

---

## 8. 排障速查

| 症状 | 可能根因 | 诊断 | 修复 |
|---|---|---|---|
| 故障切换后数据不一致 | 复制延迟未追平就 promote | 检查 WAL/GTID 延迟 | 回切 + 等同步后重切 |
| DNS 切换后部分用户仍访问旧集群 | TTL 过长 / 客户端 DNS 缓存 | dig + trace | 预调 TTL + 客户端重试逻辑 |
| Velero 恢复 PV 失败 | CSI 快照不兼容 / restic 未配 | 检查 backup describe | 改用 restic + 验证 |
| 备集群应用启动慢 | 镜像未预拉取 / minReplicas=0 | 检查 Pod 启动时间 | 预拉镜像 + 调高 minReplicas |

---

## 9. 跨域协作

- **灾备 Runbook 深入**: 见 `可靠性/09-disaster-recovery-playbooks/03-disaster-recovery-bc-runbook.md`
- **Stateful 应用备份**: 见 [[stateful-app-patterns|Stateful 应用生产模式]]
- **Fleet GitOps 多集群**: 见 `发布变更/01-gitops/08-fleet-gitops-operations-guide.md`
- **多集群运维**: 见 `生产运维/06-multi-cluster-operations.md`


<!-- risk-assessed -->
