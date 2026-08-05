---
title: Kubernetes 灾难恢复与业务连续性 Runbook
description: 覆盖 RTO/RPO 定义、etcd 仲裁丢失恢复、Velero 集群级还原、可用区/区域故障转移、DR 演练节奏与 BC Runbook 模板的生产级手册
summary: 覆盖 RTO/RPO 定义、etcd 仲裁丢失恢复、Velero 集群级还原、可用区/区域故障转移、DR 演练节奏与 BC Runbook 模板的生产级手册
category: reliability-engineering
tags:
- production
- best-practices
- playbook
- disaster-recovery
- business-continuity
- etcd
- velero
- rto
- rpo
- multi-region
- az-failover
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 灾难恢复与业务连续性 Runbook 是什么
- 如何定义 RTO RPO
- etcd 仲裁丢失怎么恢复
- Velero 集群级还原步骤
- 可用区故障转移怎么做
- Kubernetes DR 演练节奏
trigger_keywords:
- disaster recovery
- business continuity
- RTO
- RPO
- etcd quorum
- velero restore
- az failover
- region failover
- dr drill
prerequisites:
- kubectl-basics
- etcd-basics
- velero-basics
- storage-basics
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 灾难恢复与业务连续性 Runbook

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本 Runbook 为 Kubernetes 生产平台建立灾难恢复（DR）与业务连续性（BC）的统一操作框架，明确定义 RTO/RPO、etcd 仲裁丢失恢复、基于 Velero 的集群级还原、可用区与区域级故障转移、DR 演练节奏，并提供可复用的 BC Runbook 模板。BC 不是备份工具的配置堆砌，而是一套从目标定义、恢复流程、验证准则到组织演练的完整工程实践。有效的 DR 体系需要技术、流程与人员三者协同：技术上确保备份可靠、恢复路径可重复；流程上明确决策链、升级路径与通报机制；人员上通过定期演练保持响应熟练度。任何只备份不演练的环境都不能称为具备 BC 能力。

---

## 1. 适用场景与范围

- **etcd 仲裁丢失**：3 节点 etcd 集群中 2 节点永久损坏或网络分区导致无 leader。
- **控制平面全损**：所有控制平面节点无法恢复，需要从备份重建集群。
- **集群级数据损坏**：etcd 数据不一致、勒索软件加密、误删命名空间。
- **可用区（AZ）级故障**：单个 AZ 网络/电力/存储中断，工作负载需切换至其他 AZ。
- **区域（Region）级故障**：需将核心服务切换至灾备区域。
- **业务连续性模板**：为每个关键业务系统生成专属 BC Runbook，并纳入新员工培训与季度演练。

### 1.1 DR 架构模式选型

生产环境常见的 Kubernetes DR 架构有三种：

- **备份-恢复模式（Backup & Restore）**：主集群正常运行，定期备份 etcd 与 Velero；灾备区域按需重建集群并恢复。RTO 通常以小时计，RPO 取决于备份频率，适合成本敏感、可接受较长恢复时间的 Tier 1/2 业务。
- **Pilot Light**：灾备区域保持最小化控制平面与关键组件运行，数据持续同步；故障时快速扩容。RTO 通常在 30 分钟到 2 小时之间，成本与恢复速度较为平衡。
- **热备/多活（Warm Standby / Active-Active）**：灾备区域保持完整或部分业务在线，流量按 GeoDNS 分配；故障时自动或半自动切换。RTO 可低至分钟级，但成本最高，适合 Tier 0 关键业务。

选择架构时应综合考虑业务等级、RTO/RPO 要求、成本预算与运维复杂度。不要盲目追求多活，对于可接受小时级中断的业务，备份-恢复模式往往更具性价比。

---

## 2. 前置条件与工具

### 2.1 备份基础设施

- **etcd 快照**：每 4 小时自动备份，保留 30 天，异地副本存储于对象存储。备份任务应通过 CronJob 在控制平面节点运行，并配置失败告警。
- **Velero 备份**：每日全量 + 每小时增量，包含 PV 快照（CSI 快照 provider）与命名空间资源。备份策略应排除临时数据与可重建的缓存，减少存储成本。
- **Cluster State 导出**：GitOps 仓库中保存所有工作负载声明，集群重建后可重新同步。Git 是 DR 的最后一道防线，必须保证仓库高可用与访问权限。
- **镜像仓库多区域副本**：关键镜像至少同步到灾备区域。灾难发生时，若无法拉取镜像，即使配置恢复也无法启动服务。
- **DNS 与负载均衡配置备份**：将 Ingress、Service、ExternalDNS 记录纳入版本控制或对象存储，便于快速重建入口。

### 2.2 必备工具

| 工具 | 用途 | 推荐版本 |
|------|------|----------|
| `etcdctl` | etcd 快照恢复、成员管理 | v3.5+ |
| `velero` | 集群资源与 PV 恢复 | v1.14+ |
| `kubectl` | 资源检查与恢复后验证 | v1.28+ |
| `kubeadm` | 集群重建 | 与原集群版本一致 |
| `aws/azure/gcloud cli` | 区域级 DNS/负载均衡切换 | 最新 |

---

## 3. 标准操作流程

### 3.1 RTO/RPO 定义与分级

| 业务等级 | RTO | RPO | 示例 | 恢复手段 |
|----------|-----|-----|------|----------|
| Tier 0 关键 | ≤ 15 分钟 | ≤ 5 分钟 | 支付核心、身份认证 | 多活 + 自动故障转移 |
| Tier 1 重要 | ≤ 4 小时 | ≤ 1 小时 | 订单、库存 | etcd 快照 + Velero 恢复 |
| Tier 2 普通 | ≤ 24 小时 | ≤ 24 小时 | 内部工具、报表 | 每日 Velero 恢复 |
| Tier 3 可延迟 | ≤ 72 小时 | ≤ 7 天 | 日志归档、测试环境 | 对象存储重新导入 |

RTO 从故障声明开始到服务恢复可用计时；RPO 以可恢复的最新有效数据点计时。两者必须得到业务方书面确认，并每半年复审。制定目标时应避免“拍脑袋”，建议通过历史故障数据、业务容忍度调研与成本测算共同决定。例如，支付核心若中断 1 分钟可能导致数万元收入损失，则 RTO 应设定为分钟级；而内部报表系统中断一天对业务影响有限，RTO 可放宽至 24 小时。RPO 同样如此，交易系统每丢失一分钟数据都会造成财务对账困难，而日志系统丢失一小时数据通常可接受。

在落地层面，RTO 与 RPO 会直接影响备份频率、存储成本、灾备集群规模与自动化程度。RPO 要求越短，etcd 快照与 Velero 备份的频率就越高，对象存储费用也会线性增长；RTO 要求越短，就越需要 Pilot Light 或多活架构，计算成本显著上升。因此，DR 目标应在技术可行性与业务价值之间取得平衡，并通过季度成本复盘进行动态调整。

### 3.2 etcd 仲裁丢失恢复

#### 场景：2/3 etcd 节点永久丢失

etcd 采用 Raft 协议，3 节点集群可容忍 1 节点故障。当 2 节点同时不可用时，剩余节点无法获得多数派，集群进入只读或不可用状态。此时必须尽快停止写入，避免数据分叉。恢复原则是：先利用最新快照重建一个单节点 etcd，再逐步扩展回 3 节点集群。

1. **停止剩余 etcd 成员上的 kube-apiserver**，避免进一步写入：
   ```bash
   systemctl stop kubelet
   ```

2. **从最新快照恢复单节点 etcd**：
   ```bash
   ETCDCTL_API=3 etcdctl snapshot restore /root/etcd-snapshot-latest.db \
     --name etcd-0 \
     --initial-cluster "etcd-0=https://10.0.0.10:2380" \
     --initial-cluster-token etcd-cluster-1 \
     --initial-advertise-peer-urls https://10.0.0.10:2380 \
     --data-dir /var/lib/etcd-restored
   ```

3. **替换数据目录并重启 etcd**：
   ```bash
   mv /var/lib/etcd /var/lib/etcd-old
   mv /var/lib/etcd-restored /var/lib/etcd
   systemctl start kubelet
   ```

4. **重建 etcd 集群**：
   - 在恢复后的 etcd-0 上启动。
   - 清理原集群中已丢失的成员：
     ```bash
     etcdctl member list
     etcdctl member remove <member-id-of-lost-node>
     ```
   - 使用 `etcdctl member add` 加入新节点，再执行 `kubeadm init phase etcd local` 生成静态 Pod 配置。

5. **验证**：
   ```bash
   etcdctl endpoint health --cluster
   etcdctl endpoint status --cluster -w table
   kubectl get nodes
   ```

详细操作参考 [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/02-etcd-corruption-recovery-playbook|etcd 损坏恢复手册]]。恢复完成后，应持续观察 etcd 的 leader 切换次数、提交延迟与 compact 状态，确认集群已稳定。若原故障由存储损坏引起，需替换故障节点的磁盘并重新加入集群，避免再次触发相同问题。

### 3.3 集群级还原（Velero）

Velero 是 Kubernetes 最常用的备份恢复工具，支持命名空间级、标签级与全集群级恢复。恢复时需注意目标集群版本应尽量与原集群一致，尤其是 CRD 版本；若目标集群版本较新，可能因 API 废弃导致恢复失败。对于 StatefulSet 与 Deployment，建议先恢复 kube-system 等基础命名空间，再恢复业务命名空间，避免依赖倒置。

#### 灾备集群准备

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 在新区域部署同版本 Kubernetes 集群
# 2. 安装 Velero 并指向原备份 Bucket
velero install \
  --provider aws \
  --bucket <dr-backup-bucket> \
  --prefix <cluster-name> \
  --plugins velero/velero-plugin-for-aws:v1.10.0 \
  --backup-location-config region=dr-region \
  --snapshot-location-config region=dr-region \
  --secret-file ./cloud-credentials

# 3. 查看可用备份
velero backup get
velero backup describe <backup-name>
```
#### 恢复关键命名空间

```bash
# 先恢复基础组件
velero restore create restore-kube-system \
  --from-backup <backup-name> \
  --include-namespaces kube-system \
  --wait

# 再恢复业务命名空间（跳过已存在的资源避免冲突）
velero restore create restore-production \
  --from-backup <backup-name> \
  --include-namespaces production,staging \
  --exclude-resources events,pods \
  --restore-volumes \
  --wait
```

#### 恢复后验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -A
kubectl get pvc -A
kubectl get svc -A
kubectl get ingress -A
velero restore logs restore-production
```
验证时不仅要检查资源是否存在，还要确认 Pod 实际可用、Service Endpoint 正常、Ingress TLS 证书有效、存储读写正常。对于有状态服务，应执行数据一致性校验，例如数据库表记录数、消息队列堆积量等。

### 3.4 可用区/区域故障转移

AZ 级故障是云环境最常见的故障类型之一，通常由电力、网络或存储后端异常引起。Kubernetes 本身无法感知云厂商 AZ 故障，需要结合云厂商告警、节点健康检查与负载均衡器状态综合判断。

#### AZ 级故障转移

1. **确认故障范围**：仅单个 AZ 不可用，其他 AZ 健康。可通过云厂商状态页、节点 `Ready` 状态与跨 AZ 探测共同确认。
2. **剔除故障 AZ 节点**：
   ```bash
   kubectl drain <az-failed-nodes> --ignore-daemonsets --delete-emptydir-data --force
   kubectl delete node <az-failed-nodes>
   ```
3. **调整负载均衡器**：将云厂商 ELB/ALB 目标组切换至健康 AZ 节点。
4. **扩容健康 AZ 副本**：
   ```bash
   kubectl scale deployment/<app> -n <ns> --replicas=<higher>
   ```
5. **重新调度 StatefulSet**：确认 PV 在健康 AZ 有副本或跨区域快照可恢复。

#### Region 级故障转移

1. 在灾备区域启用预热集群（warm standby）。
2. 切换全局 DNS/GeoDNS 到灾备区域入口。
3. 从对象存储恢复 Velero 备份。
4. 验证数据库主从切换或跨区域只读提升。
5. 宣布业务恢复，启动主区域修复。

区域级故障转移通常涉及多个团队协同，包括 SRE、DBA、网络、安全与业务方。建议预先编写自动化 Runbook 脚本，将 DNS 切换、Velero 恢复、数据库提升、服务验证等步骤脚本化，以减少人为决策时间。转移完成后，应保留主区域故障现场，待业务稳定后再进行根因分析与修复。

### 3.5 恢复后验证与 RTO/RPO 测算

每次恢复或演练结束后，必须记录实际 RTO 与 RPO，并与目标对比。RTO 从故障声明时间戳开始计算，到业务健康检查全部通过为止；RPO 从最后一个有效备份或数据同步点计算。若实际值超过目标，需分析瓶颈并优化流程。

验证应包括：核心 Pod 全部 Running、Service 可访问、PVC 已 Bound、DNS 解析正常、数据库读写一致、关键业务冒烟用例通过、监控告警无异常。所有验证结果应写入演练报告，并归档到 `_meta/journal/`。

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 合格标准 |
|--------|------|----------|
| etcd 健康 | `etcdctl endpoint health --cluster` | 所有节点 healthy |
| 集群节点 Ready | `kubectl get nodes` | 目标节点全部 Ready |
| 核心 Pod 运行 | `kubectl get pods -n kube-system` | Running/Ready |
| Velero 备份完成 | `velero backup get` | Completed，无错误 |
| PVC 绑定 | `kubectl get pvc -A` | Bound |
| DNS 解析 | `nslookup <service>.<ns>.svc.cluster.local` | 成功 |
| 业务入口可达 | `curl https://<dr-endpoint>/health` | 返回 200 |
| RTO/RPO 达成 | 变更平台记录 | 实际恢复时间 ≤ 目标 |

---

## 5. 回滚/应急方案

- **Velero 恢复失败**：使用备份的 manifest + GitOps 重新 apply，再手动挂载对象存储中的 PV 数据。若 CRD 版本不兼容，可先恢复 CRD，再恢复自定义资源。对于关键有状态服务，建议优先恢复数据库，再恢复依赖数据库的应用。
- **etcd 恢复后数据仍然不一致**：切换至更早时间点的快照，牺牲 RPO 保证一致性。同时检查原 etcd 成员磁盘是否存在硬件故障。
- **灾备区域资源不足**：启用预留实例或按预设脚本扩容节点池。建议在平时保留最小规模 warm standby，灾备时一键扩容。
- **DNS 切换未生效**：降低 TTL 并强制刷新，必要时通过 GSLB/Cloudflare 强制流量切走。切换前应确认灾备区域证书有效。
- **数据库未随应用恢复**：Velero 仅恢复 Kubernetes 资源与 PV，数据库主从关系需 DBA 按自身流程处理。应在 BC Runbook 中明确数据库恢复责任人与命令。

---

## 6. 风险与注意事项

1. **备份不等于可恢复**：必须每季度执行恢复演练，验证备份完整性与 RTO/RPO 可达性。未经验证的备份只是心理安慰。
2. **etcd 快照包含旧证书**：若证书已轮换，恢复后可能需要重新签发 kubeconfig 与节点证书。建议在 DR 演练中模拟证书轮换后的恢复场景。
3. **跨云恢复差异大**：Velero 的 VolumeSnapshot 依赖 CSI driver，灾备集群必须部署相同或兼容的 CSI。跨云恢复通常需要先将快照数据迁移到目标云的兼容存储。
4. **StatefulSet 跨区域恢复需数据库配合**：Velero 恢复 PV 后，数据库仍需按自身 DR 流程执行主从切换或备份还原。单纯恢复 PVC 不能保证数据一致性。
5. **保留故障现场**：恢复前先收集 etcd 日志、节点事件、网络抓包，用于事后复盘。恢复操作可能会覆盖关键证据。
6. **DR 演练应避开业务高峰**：演练中的流量切换、节点隔离可能影响生产，需选择低峰时段并设置演练窗口。
7. **联系清单必须定期更新**：值班人员、云厂商 TAM、数据库 DBA、网络团队联系方式应每季度复审。

---

## 7. DR 演练节奏

| 演练类型 | 频率 | 目标 |
|----------|------|------|
| 桌面推演 | 每月 | 流程熟悉、角色分工、联系清单更新，确保新人了解各自职责 |
| etcd 快照恢复 | 每季度 | 验证 RPO、恢复时间、数据一致性，确认快照可用 |
| 单 AZ 故障转移 | 每半年 | 验证 PDB、拓扑分布、负载均衡切换，检查应用跨 AZ 韧性 |
| 跨区域全量恢复 | 每年 | 验证 Velero、数据库 DR、DNS 切换、RTO，评估整体 BC 能力 |

---

## 8. BC Runbook 模板

为每个 Tier 0/1 业务生成一页纸 BC Runbook，包含以下字段。模板应保持极简，确保值班人员在高压下可快速执行。

- **服务名称**、**SLA**、**RTO/RPO**
- **依赖组件**：数据库、缓存、消息队列、外部 API
- **故障检测**：关键告警、SLO Burn Rate
- **恢复步骤**：1/2/3/4/5，每步含负责人与命令
- **验证清单**：健康检查、冒烟用例
- **通信模板**：内部通知、客服话术、高管通报
- **升级路径**：何时升级到 P0、何时启动灾备区域

### 模板示例：订单服务

| 字段 | 内容 |
|------|------|
| 服务名称 | order-service |
| RTO/RPO | 30 分钟 / 5 分钟 |
| 依赖 | PostgreSQL 主库、Redis 缓存、Kafka 订单topic、支付网关 |
| 故障检测 | `order_service_availability` < 99.9%，P99 延迟 > 500ms |
| 恢复步骤 | 1. 检查数据库主从状态 2. 切换只读副本 3. 扩容 Pod 副本 4. 验证订单创建 5. 通知客服 |
| 验证清单 | 下单成功率 100%，P99 延迟 < 200ms，无未处理异常 |
| 升级路径 | 5 分钟未恢复则升级 P0，15 分钟未恢复启动灾备区域 |

---

## 9. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/06-production-readiness-operations-guide|可靠性工程 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/06-production-readiness-operations-guide|生产运维 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/02-etcd-corruption-recovery-playbook|etcd 损坏恢复手册]]
- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-09-reliability-engineering/05-disaster-recovery-playbooks/01-az-failure-playbook|可用区故障手册]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/05-control-plane-loss-recovery-playbook|控制平面丢失恢复]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-01-cluster-fundamentals/03-control-plane/03-plane-backup-disaster-recovery|控制平面备份与灾难恢复]]


<!-- risk-assessed -->
