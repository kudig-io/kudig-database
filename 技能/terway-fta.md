---
title: Terway 异常故障树分析 (skills)
description: Terway 异常故障树分析 — Kubernetes 生产运维知识库
summary: Terway 异常故障树分析 — Kubernetes 生产运维知识库
category: general
tags:
- k8s
- statefulset
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 异常故障树分析 是什么
- 如何 Terway 异常故障树分析
trigger_keywords:
- Terway
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-TERWAY-001
component: Terway
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Terway 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n kube-system -l app=terway -o jsonpath='{range .items[?(@.status.phase!='Running')]} {.metadata.name}{\'\n\'}{end}' 显示 Terway 异常 --> - **目标**：..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/topic-fta/list/terway-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Terway 异常故障树分析

### 诊断命令速查表

> 本表列出 FTA 树各节点的实际诊断命令，供 SRE 手工执行或 AI Agent 自动化调用。
> 变量说明: `${NODE_NAME}` - 节点名称 | `${NAMESPACE}` - 命名空间 | `${POD_NAME}` - Pod 名称 | `${INSTANCE_ID}` - ECS 实例 ID | `${VSWITCH_ID}` - 交换机 ID
> 注：部分命令需要 aliyun CLI 和相应 RAM 权限；terway-cli 命令需在 Terway Pod 内执行

### 1. ENI 分配异常

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_eni` | ENI 异常分类 | `kubectl get events -n ${NAMESPACE} --field-selector reason=FailedCreatePodSandBox -o json | jq '[.items[] | select(.message | test("ENI|bindquota|AttachNetworkInterface"))] | length'` | `> 0` | → 进入 ENI 子树 |
| `evt_eni_quota` | ENI 配额不足 | `aliyun ecs DescribeInstances --InstanceIds '["${INSTANCE_ID}"]' | jq '.Instances.Instance[0].NetworkInterfaces.NetworkInterface | length'` | 达到实例类型上限 | **确认根因** |
| | | `kubectl logs -n kube-system -l app=terway --tail=50 | grep -E "bindquota exceeded|no available ENI slot"` | 包含配额超限 | **确认根因** |
| `evt_eni_bind_fail` | ENI 绑定失败 | `kubectl logs -n kube-system -l app=terway --tail=50 | grep -E "AttachNetworkInterface failed|bindENI failed"` | 包含绑定失败 | **确认根因** |
| | | `aliyun ecs DescribeNetworkInterfaces --InstanceId ${INSTANCE_ID} | jq '.NetworkInterfaceSets.NetworkInterfaceSet[] | {id: .NetworkInterfaceId, status: .Status}'` | ENI 状态非 InUse | 进一步检查 |
| `evt_eni_drift` | ENI 状态漂移 | `kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}') -- terway-cli show` | 与云平台 ENI 列表不匹配 | **确认根因** |
| | | `aliyun ecs DescribeNetworkInterfaces --InstanceId ${INSTANCE_ID} --Status Detaching | jq '.NetworkInterfaceSets.NetworkInterfaceSet | length'` | 有 Detaching 状态 ENI | **确认根因** |

### 2. IP 地址池异常

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_ip` | IP 异常分类 | `kubectl get events -n ${NAMESPACE} --field-selector reason=FailedCreatePodSandBox -o json | jq '[.items[] | select(.message | test("IP|pool|address"))] | length'` | `> 0` | → 进入 IP 子树 |

...(截断)

## 生产案例

### 案例 1: Terway ENI 配额耗尽导致新 Pod 无法获取 IP

| 时间 | 事件 |
|------|------|
| 10:00 | 扩容 Deployment 50→80 副本 |
| 10:02 | 新 Pod 全部 Pending，Events 显示 "failed to allocate IP" |
| 10:05 | `kubectl logs -n kube-system -l app=terway` 显示 ENI quota exceeded |
| 10:10 | 阿里云控制台提升 ENI 配额，或切换为 ENIIP 共享模式 |
| 10:15 | Pod 获取 IP 成功，业务恢复 |

**根因**: 每节点独立 ENI 模式受 ECS 实例规格 ENI 数量限制(通常 4-8 个)，未提前规划容量。

### 案例 2: Terway 安全组规则缺失导致跨节点 Pod 通信失败

**现象**: 同节点 Pod 互通，跨节点 Pod 访问超时。

**诊断**: `terway-cli mapping` 检查 IP 分配正常 → 安全组未放行 Pod CIDR 段

**修复**: 🟡 安全组添加入方向规则: 允许 Pod CIDR 全部端口互通

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 全集群 Pod 网络中断 | 立即检查 Terway DaemonSet + 安全组 |
| P1 | 新 Pod 无法获取 IP | 15min 内检查 ENI 配额/IP 池 |
| P2 | 偶发网络延迟 | 检查 VPC 路由表和安全组 |

## 面试要点

1. **Q: Terway 的 ENI 独占模式与 ENIIP 共享模式有何区别？**
   A: ENI 独占: 每个 Pod 占用一个独立 ENI，网络性能最佳但受 ENI 数量限制；ENIIP: 多个 Pod 共享一个 ENI 的辅助 IP，密度更高但性能略低。生产推荐 ENIIP + 固定 IP 模式。

2. **Q: Terway 与 Flannel 在阿里云 ACK 中的选型建议？**
   A: Flannel: Overlay(VXLAN)模式，简单但有封包开销，适合小规模；Terway: 基于 VPC ENI，原生网络性能，支持安全组级别隔离、固定 IP、网络策略，适合生产环境。

3. **Q: Terway Pod 获取 IP 失败的排查路径？**
   A: ① 检查 Terway DaemonSet 状态 ② 查看 terway 日志确认错误类型 ③ ENI 配额→控制台提升 ④ IP 池耗尽→检查 vSwitch 可用 IP ⑤ 安全组/路由表配置异常。

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[技能/skills-run-README.md|skills-run-README]]
- [[技能/statefulset-fta.md|statefulset-fta]]
- [[技能/troubleshoot-node-issues.md|troubleshoot-node-issues]]
- [[技能/troubleshoot-pod-issues.md|troubleshoot-pod-issues]]

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
