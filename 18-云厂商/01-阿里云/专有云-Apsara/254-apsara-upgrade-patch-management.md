---
title: 专有云（Apsara Stack）- 升级与补丁管理
description: 专有云 ACK 版本升级、飞天底座补丁、滚动升级策略、前置检查、回滚预案与停机窗口协调
summary: 专有云（Apsara Stack）ACK 集群版本升级与飞天底座补丁的完整流程：前置检查、升级策略、滚动执行、回滚预案与停机窗口协调。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- upgrade
- patching
- kubernetes
- lifecycle
tier: core
sources:
- 阿里云专有云升级运维规范
- ACK 版本兼容性矩阵
created: 2026-07-23
last_updated: 2026-07-23
relationships:
- target: '[[18-云厂商/01-阿里云/02-ACK集群运维.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/apsara-stack-components.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 平台工程师
- 远程顾问
- 驻场运维
estimated_read_time: 16min
intent_queries:
- 专有云 ACK 怎么升级
- 飞天底座补丁怎么打
- 专有云升级前置检查
- 专有云升级失败怎么回滚
trigger_keywords:
- 升级
- 补丁
- 滚动升级
- 回滚
- 版本兼容
prerequisites:
- alicloud-basics
- k8s-architecture
- tianji-aso-operations
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

# 专有云（Apsara Stack）- 升级与补丁管理

本文档面向在客户数据中心运维 [[18-云厂商/01-阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE 与远程顾问，系统梳理 ACK 集群版本升级、飞天底座补丁、滚动升级策略、前置检查、回滚预案与停机窗口协调流程。

> **关键认知**：专有云升级分两层——**ACK 层（K8s 版本）** 与 **飞天底座层（天基/伏羲/洛神/盘古/女娲补丁）**。两层升级耦合度低但相互依赖：底座补丁通常需停机窗口，ACK 升级可滚动进行但受底座版本约束。

---

## 1. 升级分层与依赖关系

| 升级层 | 内容 | 编排方 | 停机需求 | 风险等级 |
|--------|------|--------|----------|----------|
| **飞天底座补丁** | 天基/ASO、伏羲、洛神、盘古、女娲、神龙固件/驱动 | 天基编排 + 驻场执行 | 通常需停机窗口 | 🔴 高 |
| **ACK 版本升级** | K8s 小版本/补丁版本、Addon | ASO 编排（底层天基） | 可滚动，业务感知小 | 🟡 中 |
| **Addon/插件升级** | Terway、CSI、CCM、metrics-server | ASO Addon 管理 | 滚动，逐个 | 🟡 中 |
| **OS/节点补丁** | 节点 OS 内核、安全补丁 | 节点池滚动置换 | 滚动驱逐 | 🟡 中 |

```
飞天底座补丁（🔴，需窗口）
       ↓ 约束
ACK 版本升级（🟡，可滚动）
       ↓ 约束
Addon 升级（🟡）  ←→  OS/节点补丁（🟡，节点池置换）
```

> **依赖原则**：底座补丁先行，ACK 升级需匹配底座支持的 K8s 版本范围；Addon 升级需匹配 ACK 版本。

---

## 2. ACK 版本升级

### 2.1 升级前置检查清单

升级前必须逐项确认，任何一项不满足都应推迟升级。

- [ ] **版本兼容性**：目标版本在 ASO 升级页面可选（即天基支持的版本）；确认与底座版本兼容
- [ ] **API 废弃检查**：目标版本废弃的 API（如 `v1beta1` 类）业务是否在用
- [ ] **节点水位**：升级期间会有节点滚动，确保剩余节点能承载全部 Pod（建议冗余 ≥20%）
- [ ] **etcd 健康**：etcd 空间 < 70%，无碎片告警，备份已完成
- [ ] **Pod 健康度**：无大量异常 Pod；关键业务有 PDB（PodDisruptionBudget）
- [ ] **Addon 兼容**：现有 Addon 版本是否兼容目标 K8s 版本
- [ ] **备份就绪**：etcd 快照、关键资源 YAML 已备份（见第 5 节）
- [ ] **回滚预案**：已准备回滚步骤与窗口

```bash
# 🟢 低风险：只读/信息收集
# 升级前置检查（驻场/堡垒机执行）
echo "=== 集群版本 ==="
kubectl version
echo "=== 节点状态与水位 ==="
kubectl get nodes -o wide
kubectl top nodes
echo "=== etcd 健康与空间（专有版自管 etcd） ==="
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key -w table
echo "=== 异常 Pod ==="
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded
echo "=== PDB 覆盖（关键业务应有 PDB） ==="
kubectl get pdb -A
echo "=== API 资源版本（检查废弃 API） ==="
kubectl api-resources | grep -iE 'v1beta1|v1alpha'
```

### 2.2 滚动升级策略

ACK 专有云版升级经 ASO 触发，底层天基编排 Master 与 Worker 节点分批升级。

| 策略 | Master | Worker | 适用场景 |
|------|--------|--------|----------|
| **标准滚动** | 一个一个升级（保证多数派） | 按节点池分批，每批 1-N 个 | 默认推荐 |
| **分批升级** | 同上 | 自定义批次大小与间隔 | 大规模集群 |
| **停机升级** | 全部升级 | 全部升级 | 底座补丁联动，需窗口 |

> **Master 升级关键约束**：etcd 采用 Raft 协议（3 或 5 节点），Master 必须逐个升级，任何时候保证多数派存活。**严禁**同时重启多个 Master。

### 2.3 升级执行流程

| 步骤 | 操作 | 位置 | 风险 |
|------|------|------|------|
| 1 | 备份 etcd 与关键资源 | 节点/API | 🟡 |
| 2 | ASO 触发升级，选择目标版本 | ASO `集群详情 > 升级` | 🟡 |
| 3 | 天基预校验（资源/兼容性） | 自动 | 🟡 |
| 4 | Master 滚动升级 | 天基编排 | 🔴 |
| 5 | Worker 节点池滚动升级 | 天基编排 | 🟡 |
| 6 | Addon 自动/手动升级 | ASO Addon 管理 | 🟡 |
| 7 | 升级验收 | kubectl | 🟢 |

```bash
# 🟡 中风险：升级期间实时监控（只读，但升级本身是变更）
# 监控升级进度
kubectl get nodes -w &
watch -n 5 'kubectl get nodes -o wide'
# 监控异常 Pod
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded
# 监控 Master 管控面组件
kubectl get pods -n kube-system | grep -E 'apiserver|controller-manager|scheduler|etcd'
```

### 2.4 升级后验收

```bash
# 🟢 低风险：只读
kubectl version                    # 所有节点版本一致
kubectl get nodes -o wide          # 全 Ready，版本统一
kubectl get --raw=/livez && kubectl get --raw=/readyz
kubectl get pods -A | grep -vE 'Running|Completed'   # 无异常 Pod
# 确认关键业务功能正常（业务侧验证）
```

---

## 3. 飞天底座补丁

飞天底座补丁涉及天基/ASO、伏羲、洛神、盘古、女娲、神龙固件/驱动等，影响面大，**必须由驻场工程师执行，禁止客户自行操作**。

### 3.1 补丁流程

| 阶段 | 操作 | 负责方 |
|------|------|--------|
| 评估 | 阿里云 TAM 评估补丁必要性、影响范围、兼容性 | TAM |
| 计划 | 与客户协调停机窗口，制定回滚预案 | TAM + 客户 |
| 备份 | 底座配置与数据备份 | 驻场 |
| 执行 | 天基编排补丁下发，分组件滚动 | 驻场（天基辅助） |
| 验证 | 底座组件健康、ACK 集群功能验证 | 驻场 + SRE |
| 回滚（如需） | 天基回滚补丁 | 驻场 |

### 3.2 神龙固件/驱动升级

神龙裸金属服务器的固件（BIOS）、MOC 卡驱动升级需进入机房或通过 BMC 操作，**仅驻场工程师可执行**。

| 项目 | 升级方式 | 影响 |
|------|----------|------|
| BIOS | BMC/iLO 远程或机房 | 节点重启 |
| MOC 卡驱动 | 节点 OS 内升级 | 网络短暂中断 |
| GPU 驱动 | 节点 OS 内升级 | GPU 任务中断 |

---

## 4. 节点 OS/内核补丁（节点池置换）

专有云节点补丁通常通过「节点池滚动置换」实现：新建带补丁的节点，驱逐旧节点 Pod，移除旧节点。

| 步骤 | 操作 | 风险 |
|------|------|------|
| 1 | 准备带新内核/补丁的节点镜像 | 🟢 |
| 2 | 节点池扩容新节点 | 🟡 |
| 3 | 确认新节点 Ready 并调度 Pod | 🟢 |
| 4 | `kubectl drain` 旧节点（受 PDB 约束） | 🟡 |
| 5 | 移除旧节点，缩容节点池 | 🟡 |

```bash
# 🟡 中风险：会修改集群状态
# 安全驱逐节点（尊重 PDB）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --timeout=5m
# 确认节点上无业务 Pod
kubectl get pods -A --field-selector spec.nodeName=<node-name>
# 移除节点（通过 ASO 节点池缩容，或 kubectl）
kubectl delete node <node-name>
```

> ⚠️ **驱逐前确认**：节点上的本地存储/有状态工作负载（StatefulSet 单副本）是否可迁移；`--delete-emptydir-data` 会清除 emptyDir 数据。

---

## 5. 备份与回滚

### 5.1 升级前必备备份

```bash
# 🟡 中风险：备份本身低风险，但属变更前置
# 1. etcd 快照（专有版自管 etcd）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M%S).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 2. 关键资源 YAML 导出
kubectl get deploy,sts,ds,svc,cm,secret -A -o yaml > /backup/resources-$(date +%Y%m%d).yaml
kubectl get nodes -o yaml > /backup/nodes-$(date +%Y%m%d).yaml

# 3. 验证快照可用
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-*.db -w table
```

### 5.2 回滚预案

| 升级类型 | 回滚方式 | 约束 |
|----------|----------|------|
| ACK 小版本升级 | ASO 回滚（如支持）或 etcd 快照恢复 | 数据面可能需重建 |
| ACK 大版本升级 | 通常不可原地回滚，需重建集群 | 需应用层容灾 |
| 底座补丁 | 天基回滚补丁 | 驻场执行 |
| 节点池置换 | 保留旧节点镜像，可重新拉起 | 旧节点已下线则不可逆 |

> **etcd 快照恢复是最后手段**，会导致恢复点之后的所有变更丢失，且可能造成数据面不一致。仅在集群不可恢复时使用，且必须由资深 SRE + TAM 联合执行。

---

## 6. 停机窗口协调

底座补丁与神龙固件升级需停机窗口。协调要点：

| 维度 | 要点 |
|------|------|
| 业务影响 | 评估哪些业务可停、哪些需降级运行 |
| 窗口选择 | 业务低峰期；提前公告业务方 |
| 回滚时间 | 预留回滚时间，窗口 = 执行 + 验证 + 回滚余量 |
| 联系人 | 确认 TAM、驻场、客户运维、业务方的应急联系人 |
| 进度同步 | 每个阶段同步进度，异常立即决策（继续/回滚） |

---

## 7. 何时联系阿里云 TAM / 驻场工程师

| 操作 | 处理方 |
|------|--------|
| ACK 大版本升级（跨 minor） | TAM 评估 + 客户窗口 |
| 飞天底座任何补丁 | 驻场工程师执行 |
| 神龙固件/驱动升级 | 驻场工程师（机房/BMC） |
| 升级失败需回滚底座 | TAM + 驻场 |
| etcd 快照恢复 | 资深 SRE + TAM 联合 |

---

## 相关文档

- [[18-云厂商/01-阿里云/02-ACK集群运维.md|02 ACK集群运维]]
- [[18-云厂商/01-阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md|253 天基/ASO 运维流程]]
- [[18-云厂商/01-阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]
- [[18-云厂商/01-阿里云/专有云-Apsara/255-apsara-compliance-hardening.md|255 合规加固]]
- [[18-云厂商/01-阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]

## Related

- [[23-实体/02-K8s核心组件/etcd.md|etcd]]
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]]
- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]]

<!-- risk-assessed -->
