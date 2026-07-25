---
title: 专有云（Apsara Stack）- 盘古存储排障
description: 专有云盘古分布式存储后端的 PVC Pending、IO 延迟、快照/扩容失败排障与升级路径
summary: 专有云（Apsara Stack）盘古分布式存储后端的 K8s 存储故障排障：PVC 卡 Pending、IO 延迟飙升、快照/扩容失败、盘古集群健康检查与升级路径。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- storage
- pangu
- csi
- troubleshooting
- essd
- nas
tier: core
sources:
- 阿里云专有云存储运维手册
- 盘古分布式存储排障指南
created: 2026-07-23
last_updated: 2026-07-23
relationships:
- target: '[[18-云厂商/01-阿里云/04-阿里云存储集成.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/apsara-stack-components.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 存储运维
- 远程顾问
- 驻场运维
estimated_read_time: 17min
intent_queries:
- 专有云 PVC Pending 怎么排查
- 盘古存储 IO 延迟高怎么办
- 专有云云盘快照失败
- 盘古集群健康检查
trigger_keywords:
- 盘古
- Pangu
- PVC Pending
- IO 延迟
- 快照失败
- 存储扩容
prerequisites:
- alicloud-basics
- k8s-storage
- csi-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 专有云（Apsara Stack）- 盘古存储排障

本文档面向在客户数据中心运维 [[18-云厂商/01-阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE 与远程顾问，聚焦飞天分布式存储底座——**盘古（Pangu）**及其上层云盘（ESSD）、NAS、OSS、CPFS 的 K8s 存储故障排障：PVC 卡 Pending、IO 延迟飙升、快照/扩容失败、盘古集群健康检查与升级路径。

> **关键认知**：专有云中 ESSD/NAS/OSS/CPFS 的底层都是盘古。当「全集群存储 IO 异常」或「所有 PVC 卡 Pending」时，根因多在盘古集群，而非 CSI 插件。CSI 日志说明症状，盘古集群状态决定根因。

---

## 1. 存储架构与故障分层

### 1.1 专有云存储栈

```
┌─────────────────────────────────────┐
│      K8s CSI 插件层（症状层）          │
│  diskplugin / nasplugin / ossfs      │
└────────────────┬────────────────────┘
                 │ 调用 OpenAPI（POP）
                 ↓
┌─────────────────────────────────────┐
│      云产品层（ESSD/NAS/OSS/CPFS）     │
│  Provision / Attach / Snapshot       │
└────────────────┬────────────────────┘
                 │ 后端存储
                 ↓
┌─────────────────────────────────────┐
│      盘古（Pangu）分布式存储（根因层）   │
│  分布式块/文件/对象存储引擎              │
└─────────────────────────────────────┘
```

| 故障层级 | 典型症状 | 排障入口 |
|----------|----------|----------|
| CSI 插件 | 单 PVC 卡 Pending；CSI Pod CrashLoop | `kubectl describe pvc`、CSI Pod 日志 |
| 云产品层 | 云盘 Attach 失败；NAS 挂载点无响应 | ASO 存储运维；aliyun CLI |
| 盘古层 | 全集群 IO 延迟飙升；所有 PVC 失败 | 天基 `盘古集群健康`；ASO `底座运维 > 存储 > 盘古` |

---

## 2. PVC 卡 Pending 排障

### 2.1 通用排障流程

```bash
# 🟢 低风险：只读/信息收集
# Step 1: 查看 PVC 状态与事件
kubectl get pvc <pvc-name> -n <ns>
kubectl describe pvc <pvc-name> -n <ns>   # 看 Events

# Step 2: 查看 CSI Provisioner 日志
kubectl get pods -n kube-system | grep -i csi
kubectl logs -n kube-system <csi-provisioner-pod> -c csi-provisioner --tail=100
kubectl logs -n kube-system <csi-plugin-pod> -c csi-plugin --tail=100

# Step 3: 查看 StorageClass
kubectl get sc
kubectl describe sc <sc-name>

# Step 4: 检查节点 CSI Node 插件
kubectl get pods -n kube-system -o wide | grep csi | grep <node>
```

### 2.2 常见 Pending 原因与处理

| 原因 | 症状（Events/日志） | 处理 |
|------|---------------------|------|
| **配额不足** | `QuotaExceeded`、`disk quota exceeded` | ASO 资源管理扩租户配额 |
| **盘古后端异常** | `InternalError`、批量 PVC 失败 | 查盘古集群健康（§4）；联系存储团队 |
| **AZ 不匹配** | `no available zone`、`volume zone mismatch` | 调整 StorageClass zoneAffinity 或节点调度 |
| **规格不支持** | `disk type not supported` | 确认专有云支持的盘类型（pl1/pl2/pl3/ESSD） |
| **Multi-Attach 冲突** | `Multi-Attach error`、盘已被占用 | 确认盘是否 ReadOnly 多挂；或先 detach |
| **节点漂移** | `node not found`、盘 Attach 到错误节点 | 节点 ProviderID 异常；查 CCM/伏羲 |
| **CSI 插件异常** | CSI Pod CrashLoop | 重启 CSI Pod；查 OOM/配置 |

### 2.3 云盘 Attach 失败专项

```bash
# 🟢 低风险：只读
# 查看云盘状态（驻场或堡垒机 aliyun CLI）
aliyun ecs DescribeDisks --RegionId cn-apsara-local --InstanceId <ecs-id> \
  --endpoint ecs.aliyuncs.com
# 查看云盘是否已 Attach、Attach 到哪个实例
aliyun ecs DescribeDisks --RegionId cn-apsara-local --DiskId d-xxx \
  --endpoint ecs.aliyuncs.com

# K8s 侧：查看 VolumeAttachment
kubectl get volumeattachment
kubectl describe volumeattachment <va-name>   # 看 AttachError
```

---

## 3. IO 延迟飙升排障

### 3.1 现象与定位

业务侧表现：接口延迟突增、数据库慢查询、Pod 响应超时。存储侧定位：

```bash
# 🟢 低风险：只读
# 节点侧 IO 延迟（驻场节点执行）
iostat -x 1 5            # 关注 await、%util、svctm
# 关注盘的 await（毫秒），持续高（>50ms）说明存储后端慢
# K8s 侧：是否有存储密集型 Pod
kubectl top pods -A --sort-by=cpu
```

### 3.2 盘古集群健康检查（驻场/TAM）

| 检查项 | 查看位置 | 健康标准 |
|--------|----------|----------|
| 盘古集群水位 | 天基 `运维大盘 > 存储 > 盘古集群` | 容量/元数据水位 < 80% |
| 副本健康 | 盘古集群详情 | 副本数满足，无降级 |
| ChunkServer 健康 | 盘古集群详情 | 无大量 ChunkServer 离线 |
| IO 延迟大盘 | 天目监控 | P99 延迟在基线内 |
| 重建任务 | 盘古运维页面 | 无大量重建任务堆积 |

> **判断原则**：若盘古集群整体延迟高（多个业务同时受影响），根因在盘古；若仅单 Pod/单盘延迟高，先查 CSI/节点/该盘。

### 3.3 IO 优化方向

| 方向 | 措施 |
|------|------|
| 盘类型升级 | 普通云盘 → ESSD PL1/PL2/PL3（按 IOPS/吞吐需求） |
| 预读/缓存 | 应用层调整预读、Page Cache |
| IOPS 限流 | 避免单 Pod 打满盘 IOPS（节点侧 cgroup） |
| 数据倾斜 | 检查盘古是否数据倾斜，联系存储团队 rebalance |

---

## 4. 快照与扩容失败

### 4.1 云盘快照失败

```bash
# 🟢 低风险：只读
# 查看快照状态
kubectl get volumesnapshot,volumesnapshotcontent -A
kubectl describe volumesnapshot <vs-name> -n <ns>   # 看 Events
# CSI Snapshot Controller 日志
kubectl logs -n kube-system <snapshot-controller-pod> --tail=100
```

| 失败原因 | 处理 |
|----------|------|
| 盘古快照后端异常 | 联系存储团队/TAM |
| 配额不足（快照数） | ASO 扩配额 |
| 源盘状态异常 | 确认源盘 Attached 且 IO 正常 |
| 版本不支持 | 确认 CSI/快照 CRD 版本兼容 |

### 4.2 云盘在线扩容失败

```bash
# 🟡 中风险：扩容是变更，执行前确认
# 触发扩容（修改 PVC requests.storage）
kubectl patch pvc <pvc> -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
# 查看扩容进度
kubectl get pvc <pvc> -n <ns> -w
# 节点侧文件系统扩容（如 CSI 未自动 resize）
# ext4: resize2fs /dev/vdb
# xfs:  xfs_growfs /mnt/data
```

| 失败原因 | 处理 |
|----------|------|
| 盘古后端容量不足 | 联系存储团队扩盘古集群 |
| 文件系统不支持在线扩容 | 需卸载后扩（停业务） |
| CSI 版本旧不支持 resize | 升级 CSI 插件 |
| 源盘正在快照 | 等待快照完成再扩容 |

---

## 5. NAS 专项排障

### 5.1 NAS 挂载失败/无响应

| 率见原因 | 排查 | 处理 |
|----------|------|------|
| 挂载点不通 | 节点 `mount -t nfs` 测试；网络连通性 | 检查 NAS 挂载点安全组、VPC 路由 |
| NFS 版本不兼容 | `nfsstat`、挂载选项 | 确认专有云 NAS 支持的 NFS 版本（v3/v4） |
| 权限不一致 | 多 Pod 挂载权限 | NAS 侧配 POSIX 权限/ACL |
| CSI Node CrashLoop | CSI Pod 日志 | 重启 CSI Pod；查 OOM |
| 盘古 NAS 后端慢 | 多业务同时慢 | 查盘古集群健康 |

```bash
# 🟢 低风险：只读
# 节点侧验证 NAS 挂载（驻场执行）
mount | grep nfs
showmount -e <nas-mount-target>
# 临时挂载测试
mount -t nfs -o vers=4,minorversion=0 <mount-target>:/ /tmp/nas-test
```

### 5.2 OSS/CPFS 专项

| 后端 | 常见问题 | 处理 |
|------|----------|------|
| OSS（OSSFS） | 大文件写入卡顿、权限拒绝 | 检查 OSSFS 凭证（RRSA）、并发上传参数 |
| CPFS | 客户端与内核不兼容、训练带宽不足 | 联系存储专家；升级 CPFS 客户端 |

---

## 6. 排障决策树

```
PVC/存储异常
├── 单个 PVC/Pod 受影响？
│   ├── 是 → CSI 插件层排障（describe/events/CSI 日志）
│   └── 否（批量） → ↓
├── 单个节点受影响？
│   ├── 是 → 节点层（iostat/盘 Attach/ProviderID）
│   └── 否（全集群） → ↓
├── 仅块存储（ESSD）受影响？
│   ├── 否（NAS/OSS 也异常） → 盘古层（查盘古集群健康）
│   └── 是 → 云盘产品层（DescribeDisks） + 盘古块存储子集群
└── 盘古集群健康异常？
    ├── 水位高/副本降级/ChunkServer 离线 → 联系存储团队/TAM
    └── 健康 → 回到 CSI/云产品层深入排查
```

---

## 7. 何时联系存储团队 / TAM

| 场景 | 处理方 |
|------|--------|
| 盘古集群异常（水位/副本/ChunkServer） | 阿里云存储团队 + TAM |
| 全集群存储 IO 中断 | TAM 立即升级（P0） |
| 盘古集群扩容/升级 | 驻场工程师执行 |
| NAS 协议变更/后端维护 | TAM 协调窗口 |
| CPFS 协议栈升级 | 阿里云存储专家 |

> ⚠️ **高危边界**：盘古集群的任何变更（扩容、重建、配置）必须由驻场工程师或存储团队执行，**禁止**客户/SRE 自行操作。

---

## 8. 排障检查清单

- [ ] **确认影响范围**：单 PVC/Pod、单节点、单命名空间，还是全集群？
- [ ] **PVC/VolumeAttachment 事件**：`kubectl describe pvc/volumeattachment`
- [ ] **CSI 插件状态**：Provisioner/Node 插件 Pod 是否 Running，日志是否有错误
- [ ] **节点 IO**：`iostat -x` 延迟与利用率
- [ ] **云盘状态**：`aliyun ecs DescribeDisks` Attach 状态
- [ ] **盘古集群健康**：天基/ASO 盘古集群水位、副本、ChunkServer、延迟
- [ ] **配额**：ASO 资源管理，租户存储/快照配额
- [ ] **区分层级**：CSI=症状，云产品=中间，盘古=根因
- [ ] **记录证据**：保存 `kubectl describe`、CSI 日志、aliyun CLI 输出、盘古大盘截图

---

## 相关文档

- [[18-云厂商/01-阿里云/04-阿里云存储集成.md|04 阿里云存储集成]]
- [[18-云厂商/01-阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]
- [[18-云厂商/01-阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md|253 天基/ASO 运维]]
- [[18-云厂商/01-阿里云/专有云-Apsara/254-apsara-upgrade-patch-management.md|254 升级与补丁管理]]
- [[18-云厂商/01-阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]

## Related

- [[23-实体/csi.md|CSI]]
- [[17-系统基础/06-知识字典/storage/storageclasses.md|StorageClass]]

<!-- risk-assessed -->
