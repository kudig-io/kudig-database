---
title: 用户画像 — ACK 运维工程师 (02-ai-agents)
description: 'title: 用户画像 — ACK 运维工程师'
summary: 'title: 用户画像 — ACK 运维工程师'
category: general
tags:
- ai
- ai-agent
- etcd
- prometheus
- grafana
- flannel
- calico
- coredns
- helm
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 用户画像 — ACK 运维工程师 是什么
- 如何 用户画像 — ACK 运维工程师
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 用户画像
- ACK
- 运维工程师
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 用户画像 — ACK 运维工程师
description: ACK 运维工程师用户画像，定义 Agent 的服务对象和交互偏好
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- [[Prometheus|prometheus]]
- grafana
- flannel
- calico
last_updated: 2026-04
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 用户画像 — ACK 运维工程师 是什么
- 如何 用户画像 — ACK 运维工程师
trigger_keywords:
- 用户画像
- ACK
- 运维工程师
- ai
- agent
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# 用户画像 — ACK 运维工程师

## 1. 基础信息

| 属性 | 值 |
|------|-----|
| **角色** | ACK（阿里云容器服务）工单负责人 |
| **技术栈** | [[Kubernetes|Kubernetes]]、Docker、Prometheus、Grafana、Terraform |
| **时区** | Asia/Shanghai (UTC+8) |
| **工作时间** | 工作日 09:00-18:00，但工单可能在任何时段提交 |
| **K8S 经验** | 高级：熟悉核心组件、能读源码、能做集群级调优 |

## 2. 日常工作场景

### 2.1 高频任务

| 优先级 | 任务类型 | 频率 | 典型触发 |
|--------|---------|------|---------|
| P0 | 工单诊断 — Pod/Node 问题 | 每日 5-10 个 | 客户提交工单 |
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
# 🟢 低风险：只读/信息收集，通常无副作用
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

## 用户画像配置最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 技术背景 | 明确定义用户技术等级 | 影响解释深度和术语使用 |
| 偏好语言 | 指定输出语言和格式 | 中文/英文，Markdown/纯文本 |
| 工作场景 | 描述典型任务类型 | 故障排查/代码开发/架构设计 |
| 约束条件 | 列出禁止行为 | 不执行危险命令、不修改生产 |
| 反馈机制 | 定义纠正方式 | 用户说"不对"时如何响应 |

## 用户画像模板

```markdown
# USER.md 模板

## 1. 基本信息
- 角色：[SRE/开发者/架构师]
- 经验：[X 年 K8s 运维经验]
- 环境：[阿里云 ACK / AWS EKS / 自建]

## 2. 技术栈
- 语言：Go, Python, Shell
- 工具：kubectl, helm, terraform
- 监控：Prometheus, Grafana

## 3. 交互偏好
- 语言：中文
- 格式：结构化表格 + 命令示例
- 深度：生产级，包含故障排查

## 4. 约束
- 禁止执行 rm/delete 命令
- 生产环境变更需确认
- 不存储敏感信息
```

## 用户画像与 Agent 行为映射

| 用户特征 | Agent 行为调整 | 示例 |
|---------|--------------|------|
| 初级用户 | 增加解释和背景知识 | 每个命令后加说明 |
| 高级用户 | 精简输出，直接给方案 | 省略基础解释 |
| 紧急故障 | 优先给恢复步骤 | 先止血后分析 |
| 学习模式 | 提供原理和延伸 | 加入"为什么"解释 |
| 审计模式 | 完整记录操作 | 每步可回溯 |

## 多用户场景处理

| 场景 | 策略 | 说明 |
|------|------|------|
| 团队共享 | 按角色分文件 | USER-sre.md, USER-dev.md |
| 临时切换 | 会话级覆盖 | 不修改基础文件 |
| 权限分级 | 约束条件差异化 | 生产/测试环境不同权限 |
| 新人入职 | 渐进式解锁 | 从只读到可执行 |

## 用户画像维护指南

| 操作 | 频率 | 说明 |
|------|------|------|
| 技术栈更新 | 每月 | 新工具/框架加入时 |
| 偏好调整 | 按需 | 用户反馈后即时更新 |
| 约束审查 | 每季度 | 确认禁止行为仍有效 |
| 场景扩展 | 按需 | 新任务类型出现时 |
| 完整性检查 | 每月 | 确保必填字段不为空 |

## Related

- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/helm.md|helm]]
- [[系统基础/速查卡/promql.md|promql]]
- [[系统基础/速查卡/k8s.md|k8s]]
- [[系统基础/速查卡/docker.md|docker]]

## 变更日志

| 版本 | 日期 | 变更内容 |
|------|------|--------|
| v1.4.0 | 2026-07 | 新增多用户场景处理 |
| v1.3.0 | 2026-06 | 增加行为映射表 |
| v1.2.0 | 2026-05 | 补充画像模板 |
| v1.1.0 | 2026-04 | 增加维护指南 |
| v1.0.0 | 2026-03 | 初始用户画像 |

## 相关文档

| 文档 | 关系 | 说明 |
|------|------|------|
| SOUL.md | 核心人格 | 用户画像的约束基础 |
| IDENTITY.md | 外在身份 | 用户画像的表现层 |
| AGENTS.md | 行为规范 | 用户画像的执行方式 |
| MEMORY.md | 经验积累 | 用户画像的反馈来源 |

## 用户画像质量指标

| 指标 | 目标 | 测量方式 |
|------|------|--------|
| 完整性 | 100% 必填字段 | 自动化检查脚本 |
| 准确性 | 与实际技术栈一致 | 季度审查 |
| 时效性 | 每月更新 | 变更日志追踪 |
| 可操作性 | Agent 行为符合预期 | 用户满意度反馈 |
| 一致性 | 多文件无冲突 | 交叉引用检查 |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 如何切换用户角色？ | 修改 USER.md 中的角色字段，或使用会话级覆盖 |
| 多团队如何共享？ | 按角色拆分文件，通过 include 机制引用 |
| 如何验证画像有效性？ | 观察 Agent 输出是否符合预期风格 |
| 约束条件如何生效？ | Agent 启动时加载，运行时强制执行 |
| 画像变更何时生效？ | 下次会话启动时，当前会话不受影响 |

## 用户画像审查检查清单

| 检查项 | 频率 | 说明 |
|--------|------|------|
| 技术栈准确性 | 每月 | 确认与实际使用一致 |
| 约束有效性 | 每季度 | 确认禁止行为仍适用 |
| 偏好时效性 | 按需 | 用户反馈后即时更新 |
| 多文件一致性 | 每月 | 确认无冲突配置 |
| 完整性 | 每月 | 确认必填字段不为空 |

## 用户画像版本兼容性

| USER.md 版本 | 兼容框架 | 说明 |
|-------------|----------|------|
| v1.0 | OpenClaw 1.x | 基础用户信息 |
| v1.1 | OpenClaw 1.x+ | 增加多用户支持 |
| v2.0 | OpenClaw 2.x | 支持动态用户切换 |

## See Also

- SOUL
- TOOLS
- AGENTS
- IDENTITY


<!-- risk-assessed -->
