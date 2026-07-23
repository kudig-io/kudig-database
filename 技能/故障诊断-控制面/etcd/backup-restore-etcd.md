---
title: etcd 备份与恢复操作技能
description: etcd 集群备份与恢复的标准操作流程，覆盖快照创建、验证、定时备份策略、灾难恢复、多节点恢复及常见备份失败排查
summary: etcd 备份恢复 SOP，覆盖快照/验证/定时策略/灾难恢复/多节点恢复全流程
category: skill
tags:
- k8s
- etcd
- backup
- restore
- snapshot
- disaster-recovery
- sop
- runbook
sources:
- 故障诊断/topic-fta/list/etcd-fta.md
- 故障诊断/topic-fta/list/backup-restore-fta.md
- code/etcd-3.7.0/
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- etcd 如何备份
- etcd 快照恢复流程
- etcd 备份策略怎么配置
- etcd 灾难恢复步骤
- etcd 备份失败怎么排查
trigger_keywords:
- etcd 备份
- etcd 恢复
- snapshot
- 灾难恢复
- etcdctl snapshot
- 备份策略
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd 备份与恢复操作技能

## 1. 概述

### 覆盖范围

本技能覆盖 etcd 备份与恢复的完整操作流程：

- **快照备份**：手动/自动快照创建与验证
- **备份策略**：定时备份、保留策略、远程存储
- **灾难恢复**：单节点恢复、全集群恢复、跨版本恢复
- **备份验证**：快照完整性检查、恢复演练
- **故障排查**：备份失败、恢复失败的诊断与修复

### 适用场景

| 适用 | 不适用 |
|------|--------|
| etcd 定期备份配置 | Velero 应用级备份（→ backup-restore-fta.md） |
| etcd 灾难恢复操作 | 存储后端（S3/OSS）故障排查 |
| 备份策略设计与验证 | 数据库应用层备份 |
| 备份/恢复失败排查 | etcd 性能调优（→ etcd-fta.md） |

### 前置条件

- 持有 etcd 客户端证书（`/etc/kubernetes/pki/etcd/`）
- 具备 etcd 节点 SSH 权限
- 恢复操作需要变更窗口和双人复核

---

## 2. 备份操作（快照）

### 2.1 手动创建快照

```bash
# 🟢 低风险：只读操作，不影响运行中的 etcd
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M%S).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

### 2.2 验证快照完整性

```bash
# 🟢 低风险：只读
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-20260723-030000.db --write-out=table
```

**验证要点**：
- 文件大小 > 0（空文件说明备份失败）
- `revision` > 0
- `totalKey` 合理（通常 > 1000）

### 2.3 自动备份脚本

```bash
#!/bin/bash
# 🟢 低风险：只读备份操作
BACKUP_DIR="/backup/etcd"
KEEP_DAYS=7
TIMESTAMP=$(date +%Y%m%d-%H%M%S)

# 创建快照
ETCDCTL_API=3 etcdctl snapshot save ${BACKUP_DIR}/etcd-${TIMESTAMP}.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证备份（关键！防止静默失败）
if [ $? -ne 0 ]; then
  echo "ERROR: etcd snapshot failed" >&2
  exit 1
fi

# 验证文件大小
FILE_SIZE=$(stat -f%z ${BACKUP_DIR}/etcd-${TIMESTAMP}.db 2>/dev/null || stat -c%s ${BACKUP_DIR}/etcd-${TIMESTAMP}.db)
if [ ${FILE_SIZE} -lt 1024 ]; then
  echo "ERROR: snapshot file too small (${FILE_SIZE} bytes), likely corrupted" >&2
  exit 1
fi

# 上传到远程存储（S3/OSS）
# aliyun oss cp ${BACKUP_DIR}/etcd-${TIMESTAMP}.db oss://backup-bucket/etcd/

# 清理过期备份
find ${BACKUP_DIR} -name "etcd-*.db" -mtime +${KEEP_DAYS} -delete

echo "SUCCESS: etcd backup completed - etcd-${TIMESTAMP}.db (${FILE_SIZE} bytes)"
```

### 2.4 备份策略建议

| 项目 | 频率 | 存储位置 | 保留期 |
|------|------|---------|--------|
| etcd 快照 | 每 30 分钟 | 本地 + 远程（S3/OSS） | 7 天 |
| 证书备份 | 每次变更后 | 加密存储 | 永久 |
| 集群 manifests | 每次变更后 | Git 仓库 | 永久 |
| 应用 PV 数据 | 每日（Velero） | 对象存储 | 30 天 |

---

## 3. 恢复操作

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足：变更窗口 + 双人复核 + 事前确认备份可用 + 回滚方案

### 3.1 恢复前检查清单

- [ ] 确认快照文件完整（`etcdctl snapshot status`）
- [ ] 确认 etcd 版本与备份时一致
- [ ] 确认所有控制平面组件可停止
- [ ] 通知相关业务方（恢复期间集群不可用）
- [ ] 记录当前 etcd 数据目录（用于回滚）

### 3.2 单节点恢复

```bash
# 🔴 高风险：覆盖 etcd 数据目录
# Step 1: 停止 API Server（移走 static Pod manifest）
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/

# Step 2: 停止 etcd
mv /etc/kubernetes/manifests/etcd.yaml /tmp/

# Step 3: 备份当前数据目录
mv /var/lib/etcd /var/lib/etcd-backup-$(date +%Y%m%d)

# Step 4: 从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260723.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-<node-name> \
  --initial-cluster=<node-name>=https://<node-ip>:2380 \
  --initial-advertise-peer-urls=https://<node-ip>:2380

# Step 5: 恢复 etcd manifest
mv /tmp/etcd.yaml /etc/kubernetes/manifests/

# Step 6: 等待 etcd 启动，验证健康
ETCDCTL_API=3 etcdctl endpoint health

# Step 7: 恢复 API Server
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
```

### 3.3 全集群恢复（多节点）

```bash
# 🔴 高风险：全集群数据回退
# 必须在所有 etcd 节点执行，使用同一快照

# 在每个节点上：
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
mv /etc/kubernetes/manifests/etcd.yaml /tmp/
mv /var/lib/etcd /var/lib/etcd-backup-$(date +%Y%m%d)

ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260723.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-<N> \
  --initial-cluster=etcd-1=https://<ip1>:2380,etcd-2=https://<ip2>:2380,etcd-3=https://<ip3>:2380 \
  --initial-advertise-peer-urls=https://<ipN>:2380

# 所有节点恢复后，同时启动 etcd
mv /tmp/etcd.yaml /etc/kubernetes/manifests/  # 每个节点

# 验证集群健康
ETCDCTL_API=3 etcdctl endpoint health --cluster
ETCDCTL_API=3 etcdctl endpoint status --cluster --write-out=table

# 最后恢复 API Server
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/  # 每个节点
```

### 3.4 恢复后验证

```bash
# 🟢 低风险
kubectl get nodes                              # 所有节点 Ready
kubectl get pods -A | grep -v Running          # 无异常 Pod
kubectl get --raw /healthz?verbose             # 全部组件健康
ETCDCTL_API=3 etcdctl endpoint health --cluster  # etcd 健康
```

---

## 4. 快速分级

| 严重性 | 定义 | 响应策略 |
|--------|------|---------|
| P0 | 数据丢失且需紧急恢复 | 立即执行恢复，全员上线 |
| P1 | 备份任务失败（无最新备份） | 15min 内修复备份脚本 |
| P2 | 备份策略优化/恢复演练 | 计划维护窗口 |

---

## 5. 根因分类（备份/恢复失败）

| 编号 | 根因 | 概率 | 关键证据 |
|------|------|------|----------|
| RC-001 | 备份脚本证书路径错误/过期 | 高 | 脚本退出码非 0 / 空文件 |
| RC-002 | 备份目标磁盘空间不足 | 中 | "no space left on device" |
| RC-003 | etcd 过载导致快照超时 | 中 | 快照耗时 > 30s |
| RC-004 | 恢复时 etcd 版本不匹配 | 高 | "database version mismatch" |
| RC-005 | 恢复时 initial-cluster 配置错误 | 中 | etcd 启动失败 |
| RC-006 | 仅恢复部分节点导致数据不一致 | 中 | 节点间数据不同步 |
| RC-007 | 备份文件损坏（传输中断） | 低 | snapshot status 报错 |
| RC-008 | 恢复后 API 版本不兼容 | 低 | 资源对象无法反序列化 |

---

## 6. 修复操作

| 编号 | 对应根因 | 修复操作 | 风险等级 |
|------|---------|---------|:--------:|
| REM-001 | RC-001 | 修正证书路径，添加备份验证（文件大小检查） | 🟢 |
| REM-002 | RC-002 | 清理磁盘空间或更换备份目标 | 🟢 |
| REM-003 | RC-003 | 在低峰期执行备份，增加超时时间 | 🟢 |
| REM-004 | RC-004 | 使用与备份时相同版本的 etcd 二进制执行恢复 | 🔴 |
| REM-005 | RC-005 | 核对所有节点名称和 IP，重新执行恢复 | 🔴 |
| REM-006 | RC-006 | 停止所有节点，从同一快照全量恢复 | 🔴 |
| REM-007 | RC-007 | 使用远程存储的备份副本 | 🟢 |
| REM-008 | RC-008 | 使用兼容版本恢复后逐步升级 | 🔴 |

---

## 7. 验证确认

### 备份验证（每次备份后）

```bash
# 🟢 低风险
ETCDCTL_API=3 etcdctl snapshot status <backup-file> --write-out=table
# 确认：revision > 0, totalKey > 1000, 文件大小合理
```

### 恢复验证

| 条件 | 判定 |
|------|------|
| etcd endpoint health 全部通过 | ✅ |
| kubectl get nodes 全部 Ready | ✅ |
| 关键业务 Pod Running | ✅ |
| 数据时间点与备份一致 | ✅ |

### 定期恢复演练

> **未经恢复验证的备份不是备份。** 建议每季度执行一次完整恢复演练。

---

## 8. 升级协议

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 数据丢失需恢复 | 立即执行恢复，全员上线 |
| P1 | 备份连续失败 > 24h | 紧急修复备份脚本 |
| P2 | 恢复演练计划 | 安排维护窗口 |

---

## 9. 版本兼容矩阵

| etcd 版本 | 备份兼容性 | 注意事项 |
|----------|-----------|---------|
| 3.4.x | 可恢复到 3.4.x | 不支持跨大版本恢复 |
| 3.5.x | 可恢复到 3.5.x | 3.5.0-3.5.3 有已知数据损坏 Bug |
| 3.6.x | 可恢复到 3.6.x | 新 Raft 实现，不兼容 3.5 快照 |
| 3.7.x | 可恢复到 3.7.x | 与 3.6 快照兼容 |

> [存疑：etcd 3.6 与 3.7 之间快照是否完全兼容，需确认官方升级文档]

**关键原则**：恢复时必须使用与备份时**相同大版本**的 etcd 二进制。

---

## 10. 知识进化

### 常见误诊模式

| 误诊模式 | 表现 | 正确做法 |
|---------|------|---------|
| 备份脚本静默失败未发现 | 需要恢复时发现备份为空 | 添加备份验证（文件大小 + 返回码检查） |
| 只恢复部分节点 | 节点间数据不一致 | 全集群从同一快照恢复 |
| 恢复后未验证数据完整性 | 业务数据缺失 | 恢复后全面验证关键资源 |

### 变更记录

| 版本 | 日期 | 变更内容 | 触发原因 |
|------|------|---------|---------|
| 1.0.0 | 2026-05-23 | 初版备份恢复操作文档 | 技能库初始化 |
| 2.0.0 | 2026-07-23 | 重构为标准技能结构，补全根因/修复/验证/版本矩阵 | 技能建设最佳实践对标 |

---

## 生产案例

### 案例 1: etcd 备份脚本静默失败

| 时间 | 事件 |
|------|------|
| - | 需要恢复时发现最近 7 天备份全部为空文件 |
| - | 备份脚本证书路径错误，etcdctl 连接失败但未检查返回码 |
| - | 🟡 REM-001 修复脚本 + 添加备份验证（文件大小检查） |

**根因**: RC-001。备份脚本未检查 etcdctl 返回码，证书过期后静默失败。

### 案例 2: etcd 恢复后节点数据不一致

**现象**: 恢复 etcd 后部分节点显示旧数据。

**诊断**: 只恢复了 1/3 etcd 节点，其他节点数据未同步

**修复**: 🔴 REM-006 停止所有 etcd 节点，从快照恢复全部节点

### 案例 3: etcd 备份恢复失败——版本不匹配

| 时间 | 事件 |
|------|------|
| 03:00 | 误删 namespace，尝试从 etcd 快照恢复 |
| 03:05 | `etcdctl snapshot restore` 报错: "database version mismatch" |
| 03:10 | 确认备份时 etcd 3.5.9，当前使用了 etcd 3.6.0 二进制 |
| 03:15 | 🔴 REM-004 使用相同版本 etcd 二进制执行恢复 |
| 03:30 | 集群恢复，但丢失备份后的 2h 数据 |

**根因**: RC-004。备份脚本未记录 etcd 版本，恢复时使用了不同版本。

---

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[技能/故障诊断-控制面/etcd/etcd-fta.md|etcd 异常诊断]] — 同域技能
- [[技能/故障诊断-控制面/etcd/backup-restore-fta.md|备份恢复故障树]] — 同域技能
- [[技能/故障诊断-存储/csi-storage/manage-persistent-storage.md|PV/PVC 存储管理]] — 跨域关联
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]] — 知识索引
- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 知识图谱索引]] — 知识索引

<!-- risk-assessed -->
