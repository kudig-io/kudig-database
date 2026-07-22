---
title: 发布说明索引 — 存储
description: '# 发布说明索引 — 存储'
summary: '# 发布说明索引 — 存储'
category: references
tags:
- k8s
- release-notes
- storage
- longhorn
- rook
- velero
- ceph
- crd
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — 存储 是什么
- 如何 发布说明索引 — 存储
trigger_keywords:
- 发布说明索引
- 存储
prerequisites:
- kubectl-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — 存储

> 本文档汇总存储领域 3 个核心项目的发布说明索引，共覆盖 **76 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| Longhorn | 19 | v1.11 | v1.7 | 云原生块存储 |
| Rook | 29 | v1.19 | v1.4 | Ceph 存储编排 |
| Velero | 28 | v1.18 | v1.18 | 备份与灾难恢复 |

---

## 项目详情

### Longhorn

- **实体页面**: [[longhorn|Longhorn]]
- **最新版本**: v1.11
- **发布说明目录**: `生态参考/_archived-release-notes/storage/longhorn/`
- **版本覆盖**: v0.1 → v1.11（19 个版本）
- **Breaking Changes 提醒**:
  - v1.7: 存储引擎和快照格式变更
- **升级要点**: v1.x 引入数据引擎 v2（基于 SPDK）和备份增强

### Rook

- **实体页面**: [[rook|Rook]]
- **最新版本**: v1.19
- **发布说明目录**: `生态参考/_archived-release-notes/storage/rook/`
- **版本覆盖**: v0.1 → v1.19（29 个版本）
- **Breaking Changes 提醒**:
  - v1.4: CRD API 版本升级和集群配置格式变更
- **升级要点**: Ceph 集群全生命周期管理，支持 Ceph Reef+

### Velero

- **实体页面**: Velero
- **最新版本**: v1.18
- **发布说明目录**: `生态参考/_archived-release-notes/storage/velero/`
- **版本覆盖**: v0.1 → v1.18（28 个版本）
- **Breaking Changes 提醒**:
  - v1.18: 备份存储位置 API 和插件接口变更
- **升级要点**: v1.x 支持 CSI 快照备份和数据移动器

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v1.7 | Longhorn | 存储引擎和快照格式变更 |
| v1.4 | Rook | CRD API 版本和集群配置格式变更 |
| v1.18 | Velero | 备份存储位置 API 和插件接口变更 |

---

## 相关导航

- [[概念/storage-tool-evolution.md|存储工具演进]]
- [[生态参考/98-merged-indexes/index.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## 存储组件升级检查

```bash
# 🟢 检查 Longhorn 版本
kubectl get pods -n longhorn-system -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image | head -5
kubectl get settings.longhorn.io current-version -n longhorn-system

# 🟢 检查 Rook-Ceph 版本
kubectl get pods -n rook-ceph -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image | grep operator
kubectl get cephcluster -n rook-ceph -o jsonpath='{.items[0].status.ceph.version}'

# 🟢 检查 Velero 版本
velero version
kubectl get deployment velero -n velero -o jsonpath='{.spec.template.spec.containers[0].image}'

# 🟢 检查 CSI Driver 版本
kubectl get csidrivers -o custom-columns=NAME:.metadata.name
kubectl get pods -n kube-system -l app=csi-plugin -o wide

# 🟢 检查 PV/PVC 状态
kubectl get pv | grep -v Bound
kubectl get pvc -A | grep -v Bound

# 🟢 检查 StorageClass
kubectl get storageclass
kubectl describe storageclass <default-sc>
```

## 存储组件升级路径

| 组件 | 当前版本 | 目标版本 | 关键注意事项 |
|------|----------|----------|----------------|
| Longhorn | v1.5 | v1.7+ | 存储引擎格式变更，需滚动升级 |
| Longhorn | v1.7 | v1.11 | 数据引擎 v2 (SPDK)，备份格式变更 |
| Rook | v1.12 | v1.16+ | CRD API 版本升级 |
| Rook | v1.16 | v1.19 | Ceph Reef 支持，OSD 升级流程 |
| Velero | v1.12 | v1.15+ | 插件接口变更 |
| Velero | v1.15 | v1.18 | 备份存储位置 API 变更 |

## 升级前备份检查

```bash
# 🟢 Velero 备份状态检查
velero backup get
velero schedule get
velero backup describe <latest-backup> --details

# 🟢 检查备份存储位置可访问性
velero backup-location get
velero backup-location get -o yaml | grep -A5 status

# 🟢 检查 CSI 快照状态
kubectl get volumesnapshot -A
kubectl get volumesnapshotclass
kubectl get volumesnapshotcontent

# 🟢 检查 Longhorn 备份
kubectl get backups.longhorn.io -n longhorn-system
kubectl get recurringjobs.longhorn.io -n longhorn-system

# 🟢 检查 Ceph 集群健康
kubectl exec -n rook-ceph deploy/rook-ceph-tools -- ceph status
kubectl exec -n rook-ceph deploy/rook-ceph-tools -- ceph df
```

## 存储故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| PVC Pending | StorageClass 不存在/配额不足 | `kubectl describe pvc` | 检查 SC/扩容配额 |
| PV 无法挂载 | CSI Driver 未运行 | `kubectl get pods -n kube-system -l app=csi` | 重启 CSI Pod |
| 数据丢失 | 备份未执行/备份失败 | `velero backup get` | 检查备份策略 |
| Ceph 集群降级 | OSD 故障 | `ceph status` | 替换故障 OSD |
| Longhorn 副本不足 | 节点故障 | `kubectl get volumes.longhorn.io` | 检查节点/重建副本 |
| 快照失败 | CSI 快照控制器未安装 | `kubectl get volumesnapshotclass` | 安装 snapshot-controller |

## 检查清单

- [ ] 存储组件版本已确认
- [ ] 升级前备份已完成并验证
- [ ] CSI Driver 所有 Pod Running
- [ ] PV/PVC 全部 Bound
- [ ] 备份策略已配置并定期执行
- [ ] 存储容量使用率 < 80%
- [ ] 升级回滚方案已准备
- [ ] 监控告警覆盖存储健康状态

## Related

- [[实体/k8s-storage-ecosystem.md|k8s-storage-ecosystem]] — 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
- [[rook]] — Rook
- [[longhorn]] — Longhorn
- [[生态参考/98-merged-indexes/index.md|release-notes-networking]] — 发布说明索引 — 网络
- [[生态参考/98-merged-indexes/index.md|release-notes-observability]] — 发布说明索引 — 可观测性

<!-- risk-assessed -->
