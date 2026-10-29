---
title: 集群升级与迁移故障诊断 Runbook
description: 'K8s 集群版本升级、节点滚动升级、跨集群迁移失败的完整诊断排障指南'
summary: '覆盖 kubeadm 升级卡死、API 废弃拦截、etcd 版本兼容、节点升级后 NotReady、CNI/CSI 不兼容、迁移流量切换失败等 10 类根因的三阶段诊断工作流与风险分级修复'
category: skills
tags:
- k8s
- skills
- runbook
- upgrade
- migration
- kubeadm
- version-skew
tier: core
created: '2026-08-27'
last_updated: 2026-08
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 15min
skill_id: SKILL-CP-002
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
agent_execution_mode: L1-advisory
intent_queries:
- kubeadm upgrade 卡住怎么排查
- API deprecated 无法升级如何处理
- 节点升级后 NotReady 怎么办
- etcd 版本兼容问题如何定位
trigger_keywords:
- kubeadm upgrade
- preflight failed
- api deprecation
- version skew
- node notready after upgrade
- 回滚失败
- 升级卡死
- 迁移切换失败
prerequisites:
- kubectl-basics
- kubeadm-basics
- control-plane-basics
related_skills:
- "./ts-cluster-operations.md"
- "../kubeadm/"
cross_refs:
- type: doc
  path: ./ts-cluster-operations.md
  label: '集群操作速查排查'
- type: doc
  path: '../migration/'
  label: '集群迁移方法论系列'
- type: doc
  path: '../../02-控制面/etcd/backup-restore-etcd.md'
  label: 'etcd 备份与恢复（升级前快照依赖）'
- type: fta
  path: './cluster-upgrade-fta.md'
  label: '集群升级故障树分析'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 集群升级与迁移故障诊断 / Cluster Upgrade & Migration Failure Diagnosis

集群升级是一次涉及「控制平面 → 节点组件 → 插件生态」三层的有序变更，核心约束是**次版本偏斜规则**（kubelet 可落后 apiserver 至多 N-3，apiserver 各副本至多错开一个 minor）。跨集群/云迁移则叠加数据一致性与流量切换两个高危环节。两类的共同特征是：入口看起来是一条命令，实际成败取决于前置水位检查是否完整。

## 快速症状定位

| # | 症状 | 检测方法 | 置信度 |
|---|------|---------|--------|
| S1 | `kubeadm upgrade` preflight 检查报错退出 | 🟢 命令输出含 preflight 字样 | 0.95 |
| S2 | 控制面 pod CrashLoop，日志有 unsupported api/keg mismatch | 🟢 pod 日志 + 二进制版本 | 0.90 |
| S3 | 节点完成系统包升级后 NotReady | 🟢 `kubectl get nodes` + kubelet 状态 | 0.95 |
| S4 | CNI daemonset 大量 ImagePullBackOff 或 CrashLoop | 🟢 ds 状态 + 事件 | 0.90 |
| S5 | CSI Pod 反复重启提示 driver 版本不匹配 CSIDriver 注册信息 | 🟢 csi pod events/describe | 0.85 |
| S6 | API 审计发现某个 v1beta 资源在升级后写入失败 | 🟢 `kubectl get --raw /metrics \| grep -i deprecated` 方向审查 | 0.85 |
| S7 | 迁移时新集群应用起了但外部域名解析仍指旧 LB | 🟢 DNS TTL/cutover 清单比对 | 0.90 |

**排除条件**：单纯 DNS 问题无版本相关性 → 网络 Runbook；仅业务镜像不兼容新集群配置（如 seccomp 默认值变化）→ 按 Pod Runbook 处理。

## 快速分级

```
阶段 × 影响半径
├── 升级中控制面 quorum 丢失 ─────────────────→ P0 CRITICAL（走灾备通道）
├── 升级已完成但半数 worker 未就绪 ──────────────→ P0
├── preflight 阶段被拦（未发生任何变更）────────→ P2（窗口重排即可）
├── 迁移只读阶段发现的数据差异 ────────────────→ P1（暂停写切换决策）
└── 插件兼容性预警但暂不影响现网 ────────────→ P2/P3
```

**立即升级条件**：etcd 快照从未成功过而正在执行升级——必须先补快照再继续（这是不可跳过的硬门槛）；升级过程出现多 apiserver 同时不可用。

## Phase 1 快速检查（🟢 只读）

```bash
# D1.1 全局版本矩阵：预期 vs 实际的每个组件落点
kubectl get nodes -o custom-columns='NODE:.metadata.name,VER:.status.nodeInfo.kubeletVersion,READY:.status.conditions[-1].type'
kubectl get pods -n kube-system -o wide | grep -E "apiserver|controller|scheduler|proxy|etcd"

# D1.2 当前偏斜合规性验证（N-3 规则）
major_minor=$(kubectl version -o json | jq -r '.serverVersion.major+"."+.serverVersion.minor')
echo "server=$major_minor —— 每个 kubelet ≤ server，且 ≥ (server-0.3)"

# D1.3 preflight 已知阻塞源快速核对
kubeadm upgrade plan                     # 🟢 自身即输出全部 preflight 项及缺口建议

# D1.4 准入链上的废弃告警扫描
kubectl get events -A --field-selector reason=FailedCreateResource
kubectl apply --dry-run=server -f <关键workload清单> -v=8 2>&1 | grep -iE "deprecated|removed"

# D1.5 升级窗口健康基线（有没有已经在抖动的问题被掩盖）
kubectl get pods -A --field-selector=status.phase!=Succeeded \
  | grep -vE "Running|Completed" | head -20
```

## Phase 2 深度检查（🟢 只读）

```bash
# D2.1 etcd 层状态（quorum 成员对齐与磁盘余量）
ETCDCTL_API=3 etcdctl --endpoints=$(host):2379 member list -w table    # 登录 cp 节点执行
df -h /var/lib/etcd         # 目标应保持 >30% 余量，且 WAL 盘独立更佳

# D2.2 节点侧 kubelet 为何起不来（结合节点登录）
journalctl -u kubelet --since "2 hours ago" --no-pager | tail -60
systemctl status kubelet containerd     # 半坏态常见于 kube-proxy 先升容器运行时滞后

# D2.3 CNI 兼容断点定位（以 calico 为例；其他插件类推）
kubectl get ds calico-node -n kube-system -o wide
kubectl logs -n kube-system ds/calico-node | grep -iE "version|incompatible" | tail -20
calicoctl version                        # 🟢 若 crictl/exec 可达

# D2.4 CSI 驱动注册一致性
kubectl get csidrivers
kubectl describe csidriver <driver-name>      # 对照插件 chart 的 appVersion 说明

# D2.5 迁移场景的双向数据核对
# 新旧集群分别采集：
kubectl get deployments,sts,cronjobs,ingress,secret -A -o name > <side>-inventory.txt
diff old-inventory.txt new-inventory.txt          # 数量差与命名漂移逐项归因

# D2.6 在大版本跨越前强制审计deprecations（提前一个 minor 做，别等当口）
# 推荐 pluto 工具链：
#   pluto detect-all-in-cluster --target-versions k8s=vX.YZ   （只读）
```

## Phase 3 主动探测（🟡 低风险）

```bash
# D3.1 干跑整条升级编排，产出真实 will-do 清单
kubeadm upgrade apply v<x.y.z> --dry-run
# 云托管等价物：ACK/EKS/GKE console 的 upgrade preview（各产品均有预检按钮，勿直接点提交）

# D3.2 单节点影子升级路径验证（先拿一台非承载业务的 pool member）
kubectl drain <canary-node> --ignore-daemonsets --delete-emptydir-data --timeout=10m   # 🔴 需审批
# …在隔离环境等价物上演练到 healthy 再回到生产 pool

# D3.3 流量切换前真实业务探针穿越两套栈（迁移专用）
for ep in old-cluster-lb new-cluster-lb; do curl -sI https://$ep/<health> ; done
# 双跑期间对比 RT/err rate 分布再决定切流节奏
```

## 根因分类与修复

### 根因清单

| RC ID | 根因 | 典型证据 | 首选修复 | 风险 |
|-------|------|---------|---------|------|
| RC-001 | Preflight 拦截：swap on/内核参数/端口占用/cgroup driver 漂移 | 输出明确列出每项 FAIL | 逐项处置后再进 apply | 🟢 |
| RC-002 | 版本跳跃违规（跳过 patch 或一次跨两个 minor） | plan 输出 missing intermediate 建议 | 补齐中间 patch 步骤 | 🟡 |
| RC-003 | etcd 升级中断（多数出现在存储紧张/成员异常时） | snapshot 缺位、WAL 报错 | 先保障备份完备，按 etcd runbook 恢复成员 | 🔴 |
| RC-004 | kubeadm config 与现存 cluster state 漂移 | preflight 输出 config map 差异 | 以 `kubeadm init phase upload-config` 对齐 | 🟡 |
| RC-005 | 已废弃 API 拦截工作负载更新（aggregated 后仍走老 gvk 的 CR） | controller 日志 no matches for kind | 升级前 pluto 扫描+CRD manifest 刷新 | 🟡 |
| RC-006 | 节点升级顺序错误造成 kube-proxy/CNI 断档 | ds replica 短缺时段内 services 部分不通 | 修正 SOP：先 CNI 再 runtime 后 kubelet | 🟡 |
| RC-007 | CSI plugin 未随驱动接口同步升级 | PV attach 权限 denied/mount 失败集中爆发 | 先升 CSI chart 到兼容区间再看 storage 路径 | 🔴 |
| RC-008 | Webhook admission 服务自身在新版下失效并 blocking | 大量 requests blocked by webhook | FAIL-CLOSE 政策临时降级为 FailOpen 需评估；优先修 webhook 镜像兼容性 | 🔴 |
| RC-009 | 迁移时 PV 数据绑定策略不一致导致新集群 rebind 失败 | claimRef 与 storageclass provisioner 映射缺失 | 制定 storage class 等价表 + PV 手工 reclaim 队列 | 🔴 |
| RC-010 | 迁移切流后遗留僵尸客户端持续打旧集群（陈旧 endpoint/DNS cache） | 旧集群持续收到非零 QPS、监控双高 | 客户端部署版本清点 + TTL 收敛计划 | 🟡 |

### 关键修复动作详解

**REM-A 完成 Preflight 整改（RC-001）🟢**

逐项对应官方 preflight 输出，常见四件套：

```bash
sudo swapoff -a && sudo sed -i '/ swap /d' /etc/fstab                # 🟡 仅节点维护窗
sudo sysctl -w net.bridge.bridge-nf-call-iptables=1                  # 🟡 内核参数
# cgroup driver 统一：containerd 使用 systemd 时，kubelet 配置也必须是 systemd
grep cgroupDriver /var/lib/kubelet/config.yaml
```

**REM-B 可控回退（RC-002/RC-004）🔴 — 需审批**

原则：**能前滚不回滚**。确需回退时的安全序列是：
1. 恢复 `kubeadm upgrade` 前的 etcd snapshot（联动备份恢复 Runbook 中 REM-D）
2. 按同版本降级 kubeadm/kubelet 二进制
3. 重启静态 pod manifests 目录管理器让控制面挂回旧版本
4. 保留全部现场文件到事件工单（`/etc/kubernetes/*.conf`、journal 片段），无论走哪条路

**REM-C 迁移期一致性闸门（RC-009/RC-010）🟡**

在 migration 文档的五段式流程（assessment → target design → workload → storage/data → network cutover）之上叠加三道闸门：
- **数据闸门**：源库 checkpoint hash == 目标端恢复校验值后才允许双向写开启
- **切流闸门**：DNS TTL 预收敛到 <60s 且观察 2 个周期以上稳定再降权旧线路
- **回切预案**：明确多少小时内允许一键回切（超过此窗口的数据增量须评估合并冲突成本）

## 验证清单

| 编号 | 项目 | 通过标准 |
|-----|------|---------|
| V1 | `kubeadm upgrade plan` 无 ERROR 项 | ✅ |
| V2 | 所有节点 READY、所有 system daemons 就绪且版本落在合规偏斜带内 | ✅ |
| V3 | Canary workload 端到端冒烟（in-cluster svc、LB 出口、存储读写）通过 | ✅ |
| V4 | Prometheus 基线对比：apiserver P99 延迟变化 <10%、无新增告警风暴 | ✅ |
| V5 | 迁移类：新旧集群 inventory diff 全部归零或有书面豁免 | ✅ |
| V6 | 72h 观察期内无本次变更引入的新事件（回归确认） | ✅ |

## 附录 A：云厂商特异性

| 环境 | 关键差异 |
|------|---------|
| ACK | 节点池滚动升级由 K8s 内部编排，禁止手工替换 master 节点二进制；利用 ACK 的「升级预检」产物作为唯一事实来源 |
| EKS | 严格禁用 modify master（managed）；node group 一次只 step 一个 AMI 版本以便定位回归 |
| GKE | 自动 release channel 与手动升级互斥操作需先脱离 channel；maintenance window 影响排队时间 |
| 自建 kubeadm | 一切自管——务必固化「snapshot → canary → phase-by-phase」的 SOP 矩阵和对应 checklist |

## 附录 B：常见坑速记

- `--ignore-preflight-errors` 是逃生舱不是快捷方式，永远不要用它绕过 etcd 相关项。
- 跨大版本时 N-3 偏斜是**纪律而非建议**——哪怕单节点看起来一切正常。
- 迁移过程中最贵的错误是把「还没迁完的对象」当成「应该留在旧集群」的证据；用 diff + bookkeeping 表说话。
