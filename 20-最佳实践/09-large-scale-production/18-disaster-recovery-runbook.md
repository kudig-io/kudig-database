---
title: 灾备恢复 Runbook
description: 大规模 Kubernetes 集群灾难恢复端到端手册：RTO/RPO 分级、etcd 快照恢复、Velero 恢复、整集群重建流程、恢复后验证清单与演练制度
summary: 灾备恢复操作手册：恢复场景分级决策、etcd 快照逐条恢复命令、Velero 恢复流程、GitOps 整集群重建、恢复验证与定期演练要求
category: references
tags:
- k8s
- disaster-recovery
- backup
- velero
- etcd
- runbook
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 值班工程师
estimated_read_time: 20min
---

# 灾备恢复 Runbook

> 灾备的三条铁律：**备份的价值以恢复成功为准**、**恢复流程必须提前演练过**、**恢复期间冻结一切其他变更**。本文是 [[02-cluster-configuration#7. 备份与灾难恢复]] 的可执行落地版。

## 1. 恢复场景分级与决策

| 级别 | 场景 | 恢复路径 | RTO 目标参考 |
|---|---|---|---|
| S1 | 误删资源/命名空间 | Velero 单资源恢复 或 GitOps 重新同步 | < 30 min |
| S2 | 单 Master 节点损坏 | 重建该节点重新加入（成员替换） | < 1 h |
| S3 | etcd 数据损坏/集群状态不可信 | etcd 快照恢复 | < 2 h |
| S4 | 整集群不可用（控制面全损/误删集群） | 新集群重建 + GitOps + 数据恢复 | < 4–8 h |
| S5 | 整地域不可用 | 跨地域容灾切换（见 [[10-multi-cluster#5. 跨集群容灾与多活]]） | 按容灾 SLA |

**恢复前检查（任何级别）：**

- [ ] 确认最新可用备份点及其完整性（备份时间、校验值、存放位置）
- [ ] 明确 RPO：恢复到哪个时间点，该点之后的数据丢失已获业务方确认
- [ ] 组建恢复小组：执行人、复核人（双人复核关键命令）、沟通人
- [ ] 开启事件记录：所有操作进时间线

## 2. etcd 快照恢复（自建集群）

> 🔴 高风险操作。适用于 S3。所有 Master 的 etcd 都会被回滚到快照点，**快照点之后的集群变更全部丢失**。

```bash
# 1. 停止所有 Master 上的控制面静态 Pod 🟡
mv /etc/kubernetes/manifests/*.yaml /tmp/manifests-backup/

# 2. 确认快照文件完整性 🟢
etcdctl snapshot status /backup/etcd-snapshot.db -w table

# 3. 在每个 etcd 节点上恢复（注意 initial-cluster-token 保持一致）🔴
etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restore \
  --name=<本节点名> \
  --initial-cluster=<节点1>=https://<ip1>:2380,<节点2>=https://<ip2>:2380,<节点3>=https://<ip3>:2380 \
  --initial-advertise-peer-urls=https://<本节点ip>:2380

# 4. 替换数据目录并恢复静态 Pod 🟡
mv /var/lib/etcd /var/lib/etcd.bak.$(date +%s)
mv /var/lib/etcd-restore /var/lib/etcd
mv /tmp/manifests-backup/*.yaml /etc/kubernetes/manifests/

# 5. 验证 🟢
etcdctl --endpoints=https://127.0.0.1:2379 endpoint status --cluster -w table
kubectl get nodes
```

**关键注意：**

- 恢复后 API etcd 数据是旧时间点，但节点上实际运行的 Pod 是"现在"的——会出现状态漂移，控制器会逐步 reconcile 收敛，期间可能产生误删/误建，需密切观察
- 若有 Velero，S3 场景优先考虑"etcd 恢复 + Velero 差异补齐"的组合

## 3. Velero 恢复

### 3.1 常用恢复模式

```bash
# 查看可用备份 🟢
velero backup get

# S1：恢复单个命名空间 🟡
velero restore create --from-backup <backup-name> \
  --include-namespaces <ns> --wait

# S1：只恢复某类资源 🟡
velero restore create --from-backup <backup-name> \
  --include-resources deployments,configmaps,secrets \
  --include-namespaces <ns> --wait

# S4：整集群恢复到新集群（含 PV 数据，需 CSI 快照或 restic/kopia）🔴
velero restore create --from-backup <backup-name> \
  --restore-pv=true --wait
```

### 3.2 恢复要点

- 目标集群 Velero 版本 ≥ 备份集群版本；StorageClass 名称不一致时用 `--storage-class-mappings`
- 恢复顺序：先恢复 CRD → 再恢复资源（Velero 默认处理，自定义资源多时需验证）
- 恢复后检查 `velero restore describe` 中的 errors/warnings 逐项闭环
- 云盘 PV：确认快照在同一区域/账号可用，跨账号快照需提前共享

## 4. 整集群重建（S4）

> 前提：GitOps 仓库完整、etcd/PV 备份可用。这就是"集群即代码"的回报时刻。

```text
1. 用 IaC（Terraform/Cluster API/云控制台模板）重建新集群
   → 版本与原集群一致；网段沿用原规划
2. 接入基础组件（顺序重要）：
   CNI → CoreDNS/NodeLocal → 存储 CSI → cert-manager → Ingress → 监控/日志
3. 部署 Velero 并指向原备份仓库，执行数据恢复（3.1 S4 模式）
4. GitOps 接入：ArgoCD/Flux 指向配置仓库，全量同步业务清单
5. 数据层恢复：数据库从应用层备份/快照恢复（最耗时环节，决定实际 RTO）
6. 流量切换：DNS/网关切换前完成第 5 节全部验证
7. 观察期：切流后 24h 重点观察，保留回切能力
```

**RTO 分解参考**（千节点级集群，供校准演练目标）：

| 环节 | 参考耗时 |
|---|---|
| 集群重建（IaC） | 30–60 min |
| 基础组件就位 | 30 min |
| Velero 恢复 | 30–90 min |
| 数据层恢复 | 1–4 h（取决于数据量） |
| 验证与切流 | 30–60 min |

## 5. 恢复后验证清单

- [ ] 节点全 Ready、系统组件全 Running
- [ ] 关键业务端到端冒烟测试通过（登录、核心交易、读写）
- [ ] PV/PVC 绑定正确，数据抽查一致（行数/文件校验）
- [ ] HPA/PDB/NetworkPolicy 等策略对象齐全
- [ ] 监控告警链路恢复（恢复期间监控盲区已补录）
- [ ] 证书/密钥有效，外部依赖（镜像仓库、KMS）连通
- [ ] 恢复报告输出：实际 RTO/RPO vs 目标、偏差分析、改进项

## 6. 演练制度（不演练 = 没有备份）

| 频率 | 演练内容 | 验收标准 |
|---|---|---|
| 每季度 | S1 单资源恢复 + S3 etcd 快照恢复（隔离环境） | RTO 达标、流程文档与实际一致 |
| 每半年 | S4 整集群重建演练 | 端到端跑通、实测 RTO 记录归档 |
| 每年 | S5 跨地域容灾切换大考 | 业务方参与的完整切换回切 |

每次演练后更新本文档与实际流程的偏差——Runbook 的生命力在于每次都被真实使用。

## Related

- [[17-incident-playbooks|故障处置 Runbook 集（etcd 故障场景）]]
- [[02-cluster-configuration|集群配置最佳实践（备份策略）]]
- [[05-storage|存储最佳实践（三层备份体系）]]
- [[10-multi-cluster|多集群与联邦管理（跨地域容灾）]]
- [[20-最佳实践/07-scenarios/backup-restore|备份恢复场景]]
