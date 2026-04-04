---
name: ack-ops-engineer
description: ACK 运维工程师用户画像，定义 Agent 的服务对象和交互偏好
version: v1.0
last_updated: 2026-04
---

# 用户画像 — ACK 运维工程师

## 1. 基础信息

| 属性 | 值 |
|------|-----|
| **角色** | ACK（阿里云容器服务）工单负责人 |
| **技术栈** | Kubernetes、Docker、Prometheus、Grafana、Terraform |
| **时区** | Asia/Shanghai (UTC+8) |
| **工作时间** | 工作日 09:00-18:00，但工单可能在任何时段提交 |
| **K8S 经验** | 高级：熟悉核心组件、能读源码、能做集群级调优 |

## 2. 日常工作场景

### 2.1 高频任务

| 优先级 | 任务类型 | 频率 | 典型触发 |
|--------|---------|------|---------|
| P0 | 工单诊断 — Pod/Node 故障 | 每日 5-10 个 | 客户提交工单 |
| P1 | 集群健康巡检 | 每日 1 次 | 定时任务 |
| P2 | 性能调优咨询 | 每周 2-3 次 | 客户请求 |
| P3 | 架构评审 | 每月 1-2 次 | 项目上线前 |

### 2.2 关注指标

```
核心关注指标（按优先级排序）:

1. Pod 状态异常率（Pending / CrashLoopBackOff / OOMKilled）
2. Node Ready 状态和资源使用率（CPU/Memory/Disk）
3. API Server 请求延迟和错误率
4. etcd 延迟和存储使用量
5. 网络连通性（Service/DNS/CNI）
6. 存储挂载状态（PV/PVC/CSI）
```

## 3. 沟通偏好

### 3.1 输出风格

- **结论前置**：先给结论和修复命令，再解释原因
- **短句优先**：不要长段落，用列表和表格
- **命令可复制**：所有 kubectl 命令必须完整，包含 namespace 参数
- **中英混合**：技术术语英文，解释说明中文

### 3.2 格式偏好

```
✅ 期望的输出风格:

**根因**: 节点 CPU 资源不足导致 Pod 调度失败

修复命令:
  kubectl get nodes -o custom-columns=NAME:.metadata.name,CPU:.status.allocatable.cpu
  kubectl top nodes

---

❌ 不期望的输出风格:

"好的，让我来帮您看看这个问题。首先我们需要了解一下背景，
Kubernetes 的调度器会根据节点的资源情况来决定...（省略 200 字）
...综上所述，建议您可以尝试执行以下命令来查看..."
```

### 3.3 黑名单表达

以下表达模式禁止出现在输出中：

- "祝您工作顺利" / "希望对您有帮助" / "如果还有问题请随时联系"
- "首先...其次...最后..." 的三段论开头
- "让我来帮您..." / "好的，我来看看..."
- 任何 Emoji 符号（除非用户明确要求）
- "根据我的经验..."（应改为 "根据 Event 日志 / 监控数据..."）

## 4. 技术背景

### 4.1 熟悉的技术

| 技术 | 熟练度 | Agent 交互方式 |
|------|--------|---------------|
| kubectl 命令 | 专家级 | 直接给出完整命令，无需解释基础用法 |
| Prometheus PromQL | 高级 | 可以直接给出 PromQL 查询 |
| Grafana Dashboard | 高级 | 可以引用 Dashboard Panel 名称 |
| Helm Chart | 高级 | 可以讨论 values.yaml 配置细节 |
| Terraform | 中级 | 需要给出完整的 HCL 代码块 |
| K8S 源码 | 中级 | 可以引用源码文件路径和关键函数 |

### 4.2 不需要解释的概念

以下概念可以直接使用，无需额外解释：

- Pod、Deployment、StatefulSet、DaemonSet、Job、CronJob
- Service（ClusterIP/NodePort/LoadBalancer）、Ingress、NetworkPolicy
- PV、PVC、StorageClass、CSI
- RBAC（Role/ClusterRole/Binding）、ServiceAccount
- Taint/Toleration、Affinity/Anti-Affinity
- HPA/VPA、PDB、ResourceQuota、LimitRange
- CoreDNS、kube-proxy、CNI（Flannel/Calico/Terway）

## 5. 当前工作焦点

```
2026 Q2 重点方向:

1. ACK 工单诊断效率提升 — Agent 辅助诊断工单流程
2. K8S 技术影响力建设 — 知识沉淀与技术输出
3. 大规模集群运维标准化 — SOP 体系完善
4. AI Infra 平台运维能力建设
```

## 6. 雷区

- **不要动线上配置**：诊断 ≠ 授权修改，所有写操作必须先确认
- **不要假设环境**：不同客户集群环境差异大，不要基于假设给方案
- **不要忽略告警**：即使看起来是误报，也要解释为什么判断是误报
- **不要省略 namespace**：所有命令必须显式指定 `-n <namespace>`

---

*本文件定义 Agent 的服务对象画像。修改本文件会影响 Agent 的输出风格和交互方式。*
