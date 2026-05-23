---
title: topic-skills — 工单智能体 Kubernetes 诊断 Skill 库
description: '│  Layer 2: topic-structural-     (怎么查 — 深度排查参考层)         │'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- prometheus
- coredns
- hpa
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- topic-skills — 工单智能体 Kubernetes 诊断 Skill 库 是什么
- 如何 topic-skills — 工单智能体 Kubernetes 诊断 Skill 库
trigger_keywords:
- topic-skills
- 工单智能体
- Kubernetes
- 诊断
- Skill
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
- gpu-scheduling-basics
skill_id: SKILL-README-001
skill_name: topic-skills — 工单智能体 Kubernetes 诊断 Skill 库
version: 1.0.0
created: "2026-05-23"
---

# topic-skills — 工单智能体 [[Kubernetes|Kubernetes]] 诊断 [[SKILL|Skill]] 库

> **适用版本**: Kubernetes v1.28 - v1.32  
> **Skill 数量**: 18 个核心场景  
> **覆盖领域**: Node · Pod · Network · Storage · Security · Workload · Scaling · Configuration · Observability · Performance  
> **定位**: 面向 AI Agent 运行时的自包含工单处理 Runbook  
> **最后更新**: 2026-04

---

## 1. 什么是 Skill？

Skill 是工单智能体（Ticket Handling Agent）在运行时可直接调用的**自包含诊断-修复执行单元**。每个 Skill 覆盖一类特定的 Kubernetes 问题场景，包含从症状识别到修复验证的完整闭环。

### 与现有知识资产的定位区分

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 4: domain-10-troubleshooting-diagnostics/topic-skills/         (做什么 — Agent 执行层)          │
│  自包含 Runbook：症状触发 → 诊断 → 修复 → 验证 → 升级            │
├─────────────────────────────────────────────────────────────────┤
│  Layer 3: domain-10-troubleshooting-diagnostics/topic-fta/list/       (为什么 — 故障分析模型层)         │
│  FTA 故障树：概率模型、因果链、底事件分解                          │
├─────────────────────────────────────────────────────────────────┤
│  Layer 2: topic-structural-     (怎么查 — 深度排查参考层)         │
│           trouble-shooting/                                      │
├─────────────────────────────────────────────────────────────────┤
│  Layer 1: domain-*/             (背景知识 — 理论与架构层)          │
│  组件架构、设计原理、理论基础                                     │
└─────────────────────────────────────────────────────────────────┘
```

| 维度 | domain-10-troubleshooting-diagnostics/topic-fta/list/ | domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/ | **domain-10-troubleshooting-diagnostics/topic-skills/** |
|------|----------------|-----------------------------------|-------------------|
| **定位** | 故障树分析模型 | 人类可读深度排查指南 | Agent 可执行工单处理 Runbook |
| **结构** | Mermaid 图 + JSON 工作流 | 决策树 + 解释性文字 | YAML 元数据 + 症状触发 + 分步诊断 + 风险分级修复 |
| **受众** | FTA 分析师 / Agent 推理引擎 | 初级到高级运维人员 | AI Agent 运行时（工单处理循环） |
| **粒度** | 按组件（37 个） | 按组件（40+ 文档） | 按问题场景（高频工单类型） |
| **输出** | 根因路径 + 概率 | 解释 + 命令 | 结构化动作序列 + 风险门控 + 验证关卡 |

---

## 2. Skill 全景索引表

| 编号 | Skill ID | 名称 | 类别 | 文件 | 成熟度 |
|------|----------|------|------|------|--------|
| 01 | SKILL-NODE-001 | 节点 NotReady 诊断与修复 | Node | [01-node-notready.md](./01-node-notready.md) | GA |
| 02 | SKILL-POD-001 | Pod CrashLoop/OOMKilled 诊断 | Pod | [02-pod-crashloop-oomkilled.md](./02-pod-crashloop-oomkilled.md) | GA |
| 03 | SKILL-POD-002 | Pod Pending 调度失败诊断 | Pod | [03-pod-pending.md](./03-pod-pending.md) | GA |
| 04 | SKILL-NET-001 | DNS 解析故障诊断 | Network | [04-dns-resolution-failure.md](./04-dns-resolution-failure.md) | GA |
| 05 | SKILL-NET-002 | Service 连通性故障诊断 | Network | [05-service-connectivity.md](./05-service-connectivity.md) | GA |
| 06 | SKILL-SEC-001 | 证书过期与 TLS 故障诊断 | Security | [06-certificate-expiry.md](./06-certificate-expiry.md) | GA |
| 07 | SKILL-STORE-001 | PVC/PV/CSI 存储故障诊断 | Storage | [07-pvc-storage-failure.md](./07-pvc-storage-failure.md) | GA |
| 08 | SKILL-WORK-001 | Deployment 滚动更新故障诊断 | Workload | [08-deployment-rollout-failure.md](./08-deployment-rollout-failure.md) | GA |
| 09 | SKILL-SEC-002 | RBAC 权限与 ResourceQuota 问题 | Security | [09-rbac-quota-failure.md](./09-rbac-quota-failure.md) | GA |
| 10 | SKILL-IMAGE-001 | 镜像拉取与仓库故障诊断 | Image | [10-image-pull-failure.md](./10-image-pull-failure.md) | GA |
| 11 | SKILL-CP-001 | etcd 与控制平面故障诊断 | ControlPlane | [11-control-plane-failure.md](./11-control-plane-failure.md) | GA |
| 12 | SKILL-SCALE-001 | HPA/VPA/CA 弹性伸缩问题 | Scaling | [12-autoscaling-failure.md](./12-autoscaling-failure.md) | GA |
| 13 | SKILL-NET-003 | Ingress/Gateway 路由故障诊断 | Network | [13-ingress-gateway-failure.md](./13-ingress-gateway-failure.md) | GA |
| 14 | SKILL-CONFIG-001 | ConfigMap/Secret 配置管理问题 | Configuration | [14-configmap-secret-failure.md](./14-configmap-secret-failure.md) | GA |
| 15 | SKILL-MONITOR-001 | 监控告警体系故障诊断 | Observability | [15-monitoring-alerting-failure.md](./15-monitoring-alerting-failure.md) | GA |
| 16 | SKILL-LOG-001 | 日志收集与管理故障诊断 | Observability | [16-logging-pipeline-failure.md](./16-logging-pipeline-failure.md) | GA |
| 17 | SKILL-PERF-001 | 性能瓶颈诊断与调优 | Performance | [17-performance-bottleneck.md](./17-performance-bottleneck.md) | GA |
| 18 | SKILL-SECURITY-001 | 安全事件应急响应 | Security | [18-security-incident-response.md](./18-security-incident-response.md) | GA |

---

## Skill 成熟度标识

| 标识 | 说明 |
|------|------|
| **GA** (General Availability) | 生产就绪，10-Section 完整，经过验证 |
| **Beta** | 功能完整但部分场景未验证 |
| **Alpha** | 初始版本，覆盖核心场景 |

---

## 按运维场景快速导航

### 故障诊断
- 节点问题: [01-node-notready](./01-node-notready.md) | [11-control-plane](./11-control-plane-failure.md)
- Pod 异常: [02-crashloop](./02-pod-crashloop-oomkilled.md) | [03-pending](./03-pod-pending.md) | [10-image-pull](./10-image-pull-failure.md)
- 网络问题: [04-dns](./04-dns-resolution-failure.md) | [05-service](./05-service-connectivity.md) | [13-ingress](./13-ingress-gateway-failure.md)
- 存储问题: [07-pvc-storage](./07-pvc-storage-failure.md)
- 配置问题: [14-configmap-secret](./14-configmap-secret-failure.md)

### 工作负载管理
- 部署与更新: [08-deployment-rollout](./08-deployment-rollout-failure.md)
- 弹性伸缩: [12-autoscaling](./12-autoscaling-failure.md)

### 安全合规
- 权限管理: [09-rbac-quota](./09-rbac-quota-failure.md)
- 证书管理: [06-certificate-expiry](./06-certificate-expiry.md)
- 安全事件: [18-security-incident](./18-security-incident-response.md)

### 可观测性
- 监控告警: [15-monitoring-alerting](./15-monitoring-alerting-failure.md)
- 日志管理: [16-logging-pipeline](./16-logging-pipeline-failure.md)

### 性能调优
- 性能瓶颈: [17-performance-bottleneck](./17-performance-bottleneck.md)

---

## 3. 症状 → Skill 快速查找

### 按错误现象查找

| 常见现象 / 告警 | 对应 Skill | 置信度 |
|----------------|-----------|--------|
| `kubectl get nodes` 显示 NotReady | [01-node-notready](./01-node-notready.md) | 0.95 |
| 节点状态频繁在 Ready/NotReady 间切换 | [01-node-notready](./01-node-notready.md) | 0.85 |
| Pod 状态显示 CrashLoopBackOff | [02-pod-crashloop-oomkilled](./02-pod-crashloop-oomkilled.md) | 0.95 |
| Pod 被 OOMKilled (exit code 137) | [02-pod-crashloop-oomkilled](./02-pod-crashloop-oomkilled.md) | 0.95 |
| Pod 长期处于 Pending 状态 | [03-pod-pending](./03-pod-pending.md) | 0.95 |
| Events 中出现 FailedScheduling | [03-pod-pending](./03-pod-pending.md) | 0.90 |
| 容器内 DNS 解析失败 (NXDOMAIN / timeout) | [04-dns-resolution-failure](./04-dns-resolution-failure.md) | 0.95 |
| CoreDNS Pod 不健康或频繁重启 | [04-dns-resolution-failure](./04-dns-resolution-failure.md) | 0.85 |
| Service ClusterIP 无法访问 | [05-service-connectivity](./05-service-connectivity.md) | 0.90 |
| Endpoints 为空 / EndpointSlice 无条目 | [05-service-connectivity](./05-service-connectivity.md) | 0.90 |
| `x509: certificate has expired` 错误 | [06-certificate-expiry](./06-certificate-expiry.md) | 0.95 |
| TLS handshake failure | [06-certificate-expiry](./06-certificate-expiry.md) | 0.80 |
| kubelet 无法连接 apiserver | [01-node-notready](./01-node-notready.md) + [06-certificate-expiry](./06-certificate-expiry.md) | 0.70 |
| PVC 一直 Pending 状态 | [07-pvc-storage-failure](./07-pvc-storage-failure.md) | 0.95 |
| StorageClass 不存在或配置错误 | [07-pvc-storage-failure](./07-pvc-storage-failure.md) | 0.90 |
| Deployment rollout 卡住 | [08-deployment-rollout-failure](./08-deployment-rollout-failure.md) | 0.90 |
| ReplicaSet 无法创建新 Pod | [08-deployment-rollout-failure](./08-deployment-rollout-failure.md) | 0.85 |
| RBAC Forbidden / Unauthorized | [09-rbac-quota-failure](./09-rbac-quota-failure.md) | 0.95 |
| ResourceQuota exceeded | [09-rbac-quota-failure](./09-rbac-quota-failure.md) | 0.90 |
| ImagePullBackOff / ErrImagePull | [10-image-pull-failure](./10-image-pull-failure.md) | 0.95 |
| 私有仓库认证失败 | [10-image-pull-failure](./10-image-pull-failure.md) | 0.90 |
| etcd 集群不健康 / leader 选举失败 | [11-control-plane-failure](./11-control-plane-failure.md) | 0.95 |
| API Server 无响应 | [11-control-plane-failure](./11-control-plane-failure.md) | 0.90 |
| HPA 不触发扩容 | [12-autoscaling-failure](./12-autoscaling-failure.md) | 0.90 |
| Metrics Server 无数据 | [12-autoscaling-failure](./12-autoscaling-failure.md) | 0.85 |
| Ingress 404/502/503 错误 | [13-ingress-gateway-failure](./13-ingress-gateway-failure.md) | 0.90 |
| ConfigMap/Secret 未挂载 | [14-configmap-secret-failure](./14-configmap-secret-failure.md) | 0.90 |
| Prometheus 指标缺失 | [15-monitoring-alerting-failure](./15-monitoring-alerting-failure.md) | 0.85 |
| AlertManager 告警不发送 | [15-monitoring-alerting-failure](./15-monitoring-alerting-failure.md) | 0.85 |
| 日志收集中断 | [16-logging-pipeline-failure](./16-logging-pipeline-failure.md) | 0.85 |
| 应用响应延迟高 | [17-performance-bottleneck](./17-performance-bottleneck.md) | 0.80 |
| 容器 CPU/内存持续高位 | [17-performance-bottleneck](./17-performance-bottleneck.md) | 0.85 |
| 疑似入侵/异常访问 | [18-security-incident-response](./18-security-incident-response.md) | 0.90 |

### 按 Kubernetes Event Reason 查找

| Event Reason | 对应 Skill |
|-------------|-----------|
| `NodeNotReady`, `NodeStatusUnknown` | 01-node-notready |
| `KubeletNotReady`, `NodeHasDiskPressure`, `NodeHasMemoryPressure`, `NodeHasPIDPressure` | 01-node-notready |
| `BackOff`, `Killing` (OOMKilled) | 02-pod-crashloop-oomkilled |
| `FailedScheduling`, `Unschedulable` | 03-pod-pending |
| DNS 相关 error message | 04-dns-resolution-failure |
| `FailedToUpdateEndpoint`, `FailedToUpdateEndpointSlices` | 05-service-connectivity |
| TLS / x509 相关 error | 06-certificate-expiry |

### 按 Prometheus 告警查找

| 告警规则 / 指标模式 | 对应 Skill |
|-------------------|-----------|
| `kube_node_status_condition{condition="Ready",status="false"}` | 01-node-notready |
| `kube_node_status_condition{condition="MemoryPressure",status="true"}` | 01-node-notready |
| `kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"}` | 02-pod-crashloop-oomkilled |
| `kube_pod_container_status_last_terminated_reason{reason="OOMKilled"}` | 02-pod-crashloop-oomkilled |
| `kube_pod_status_phase{phase="Pending"} > 0` (持续 > 5min) | 03-pod-pending |
| `coredns_dns_responses_total{rcode="SERVFAIL"}` rate 升高 | 04-dns-resolution-failure |
| `kube_endpoint_address_available == 0` | 05-service-connectivity |
| `apiserver_client_certificate_expiration_seconds < 86400` | 06-certificate-expiry |

---

## 4. Agent 集成指南

### 4.1 Skill 路由（Intent → Skill Matching）

```
工单/告警输入
    │
    ▼
┌─────────────────┐
│ 1. 关键词匹配     │ ← trigger_keywords (YAML front matter)
│ 2. Event 匹配    │ ← trigger_events
│ 3. Metric 匹配   │ ← trigger_metrics
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 症状识别表验证    │ ← Section 2: 置信度 > 阈值 + 排除标准检查
└────────┬────────┘
         │ 选中 Skill
         ▼
┌─────────────────┐
│ 快速分级 (2min)  │ ← Section 3: 影响评估 + P0-P3 分级
└────────┬────────┘
         │
    ┌────┴────┐
    │ P0/P1?  │──Yes──→ 检查立即升级条件
    └────┬────┘
         │ No
         ▼
┌─────────────────┐
│ 诊断工作流       │ ← Section 4: Phase 1 → 2 → 3
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 根因确认         │ ← Section 5: 匹配根因分类表
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 修复操作         │ ← Section 6: 按风险等级执行/建议/升级
│ 🟢低风险→自动    │
│ 🟡中风险→审批    │
│ 🔴高风险→指导    │
│ ⚫严重→升级      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 验证确认         │ ← Section 7: 即时 + 短期 + 回归
└─────────────────┘
```

### 4.2 YAML Front Matter 解析

每个 Skill 以 YAML front matter 开头，Agent 应解析以下关键字段进行路由决策：

```yaml
# 必选字段
skill_id: string          # 唯一标识，格式 SKILL-{CATEGORY}-{SEQ}
skill_name: string        # 中英文双语名称
category: string          # node | pod | network | storage | control-plane | security
severity_range: string    # 适用严重性范围，如 P0-P2
k8s_versions: list        # 兼容的 K8s 版本列表
risk_level: string        # low | medium | high | critical
agent_execution_mode: string  # L1-advisory | L2-semi-auto | L3-full-auto

# 路由匹配字段
trigger_keywords: list    # NLP 匹配关键词（中英文）
trigger_events: list      # Kubernetes Event Reason
trigger_metrics: list     # Prometheus 指标模式

# 关联引用
related_skills: list      # 关联 Skill ID
fta_refs: list            # 对应 FTA 文件路径
knowledge_refs: list      # 深度知识参考路径
```

### 4.3 风险门控（Human-in-the-Loop）

| Agent 模式 | 🟢 低风险 | 🟡 中风险 | 🔴 高风险 | ⚫ 严重 |
|-----------|----------|----------|----------|--------|
| **L1-advisory** | 建议 | 建议 | 建议 | 升级 |
| **L2-semi-auto** | 自动执行 | 人工审批后执行 | 建议 | 升级 |
| **L3-full-auto** | 自动执行 | 自动执行 | 人工审批后执行 | 升级 |

### 4.4 反馈闭环

Agent 在执行 Skill 后应记录：
1. **诊断路径**: 实际执行了哪些 Step、每步的输出摘要
2. **根因确认**: 最终确认的根因 ID 及置信度
3. **修复结果**: 执行了哪个修复操作、是否成功
4. **验证状态**: 即时验证是否通过
5. **新发现**: 诊断过程中发现的未在 Skill 中覆盖的情况 → 反馈到知识进化

---

## 5. Kubernetes 版本兼容总表 (v1.28 - v1.32)

### 影响 Skills 的关键版本变更

| 版本 | 关键变更 | 影响的 Skill |
|------|---------|-------------|
| **v1.28** | Native Sidecar Containers (alpha); `kubectl debug` 增强 ephemeral containers; ValidatingAdmissionPolicy (beta) | 02-pod-crashloop, 03-pod-pending |
| **v1.29** | ReadWriteOncePod GA; KMS v2 GA; nftables kube-proxy (alpha); load balancer IP mode API | 05-service-connectivity, 06-certificate-expiry |
| **v1.30** | Node swap support (beta); Structured auth config GA; CEL for admission; HPA container resource metrics | 01-node-notready, 05-service-connectivity |
| **v1.31** | AppArmor GA; Multiple service CIDRs (beta); Traffic distribution for Services; Consistent reads from cache | 05-service-connectivity |
| **v1.32** | Auto-remove PV claim policy; Structured authorization config; Custom resource field selectors GA; 改进的 Pod scheduling readiness | 03-pod-pending |

### kubectl debug 可用性矩阵

| 功能 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Ephemeral Containers | GA | GA | GA | GA | GA |
| Node Debug (`kubectl debug node/`) | GA | GA | GA | GA | GA |
| Custom Debug Profiles | beta | beta | GA | GA | GA |
| Sidecar Container Debug | alpha | beta | beta | GA | GA |

### kube-proxy 模式矩阵

| 模式 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| iptables | 默认 | 默认 | 默认 | 默认 | 默认 |
| IPVS | 稳定 | 稳定 | 稳定 | 稳定 | 稳定 |
| nftables | - | alpha | beta | beta | GA |

---

## 6. 关联资源

| 资源 | 路径 | 用途 |
|------|------|------|
| FTA 故障树库 | [domain-10-troubleshooting-diagnostics/topic-fta/list/](../domain-10-troubleshooting-diagnostics/topic-fta/list/) | 每个 Skill 对应的故障分析模型 |
| FEBM 循证方法论 | [domain-10-troubleshooting-diagnostics/topic-febm/](../domain-10-troubleshooting-diagnostics/topic-febm/) | Agent 工单处理的理论基础 |
| 结构化故障排查 | [domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/) | 深度排查参考指南 |
| Agent 设计 | [domain-14-ai-ml-infra/topic-ai-agent/](../domain-14-ai-ml-infra/topic-ai-agent/) | AI Agent 工程与架构设计 |
| 事件管理 Runbook | [domain-17-system-foundation/topic-dictionary/12-incident-management-runbooks.md](../domain-17-system-foundation/topic-dictionary/12-incident-management-runbooks.md) | 事件管理流程模板 |
| 生产排障 Playbook | [domain-17-system-foundation/topic-dictionary/16-production-troubleshooting-playbook.md](../domain-17-system-foundation/topic-dictionary/16-production-troubleshooting-playbook.md) | 生产环境排障手册 |
| Skill 文档模板 | [skill-schema.md](./skill-schema.md) | 新建 Skill 的规范化模板 |
| IDE 目录格式 Skill | [k8s-node-notready/](./skill-set/k8s-node-notready/) | Node NotReady 的 IDE 标准 Skill 目录（含脚本、数据、参考文档） |
| **本地 Demo** | [skills-run/](./skills-run/) | **本地 Kind 集群运行 Skill 执行闭环 Demo** |
| Demo 运行指南 | [19-skill-local-demo-guide.md](./19-skill-local-demo-guide.md) | 详细的 Demo 场景说明与 Skill 映射 |

---

## 7. 本地运行 Demo

在本地 Kind 集群中实际运行 Skill 的完整执行闭环。详见 [Demo 运行指南](./19-skill-local-demo-guide.md) 和 [skills-run/README.md](./skills-run/README.md)。

```bash
# 快速开始
cd domain-10-troubleshooting-diagnostics/topic-skills/skills-run
bash setup-kind-cluster.sh     # 创建 1 CP + 2 Worker 的 Kind 集群
bash run-skill-demo.sh          # 交互式选择场景
bash teardown.sh                # 清理
```

| # | 场景 | 对应 Skill | 注入方式 | 修复方式 |
|---|------|-----------|---------|--------|
| 01 | 节点 Cordon | SKILL-NODE-001 / RC-012 | `kubectl cordon` | REM-001: uncordon |
| 02 | Pod CrashLoop | SKILL-POD-001 | 错误启动命令 | 修正 Deployment |
| 03 | Pod Pending | SKILL-POD-002 | 资源请求超限 | 调整 requests |
| 04 | DNS 问题 | SKILL-NET-001 | CoreDNS 缩容 | 恢复 CoreDNS |
| 05 | Service 无 EP | SKILL-NET-002 | Selector typo | 修正 selector |
| 06 | PVC Pending | SKILL-STORE-001 / RC-001 | 无效 StorageClass | 创建 StorageClass |
| 07 | Deployment 卡住 | SKILL-WORK-001 / RC-002 | readinessProbe 失败 | 修正配置 |
| 08 | RBAC 拒绝 | SKILL-SEC-002 / RC-001 | 权限不足 | 调整 Role |
| 09 | HPA 不触发 | SKILL-SCALE-001 / RC-002 | 未设置 requests | 添加资源配置 |
| 10 | 镜像拉取失败 | SKILL-IMAGE-001 / RC-001 | 镜像不存在 | 修正镜像名 |

> 更多 Demo 场景说明见 [skills-run/README.md](./skills-run/README.md)

---

## 8. IDE 目录格式 Skill

除单文件 Skill 外，本目录还提供符合主流 IDE（Qoder/Cursor）标准的**目录格式 Skill**，包含可执行脚本、机器可解析数据和模块化参考文档：

### skill-set/k8s-node-notready/

```
skill-set/k8s-node-notready/
├── SKILL.md                        # 入口: Skill 定义 + Agent 执行指令
├── reference/
│   ├── diagnostic-workflow.md      # 完整 Phase 1-3 诊断工作流
│   ├── root-cause-catalog.md       # 12 个根因详细说明 + 证据映射
│   ├── remediation-playbook.md     # 10 个修复操作 + 验证 + 升级协议
│   └── version-matrix.md           # K8s v1.28-v1.32 版本兼容 + 知识进化
├── scripts/
│   ├── diagnose-quick.sh           # Phase 1: kubectl 快速检查（只读）
│   ├── diagnose-deep.sh            # Phase 2: SSH 深度检查（只读）
│   ├── check-resources.sh          # 资源压力检查（磁盘/内存/PID/inode）
│   ├── cleanup-disk.sh             # 修复: 磁盘空间清理 (REM-002)
│   └── verify-node.sh              # 修复后: 节点健康验证
└── assets/
    ├── skill-metadata.yaml         # 机器可解析的完整元数据
    ├── symptom-patterns.yaml       # 症状→Skill 匹配规则
    ├── root-cause-map.yaml         # 根因决策树数据
    └── escalation-template.md      # 升级消息模板
```

---

## 9. 后续规划

### 进行中

| 项目 | 说明 | 状态 |
|------|------|------|
| IDE 目录格式 Skill | 为每个 Skill 提供可执行脚本和机器可解析数据 | 进行中 |
| 云厂商特异性补充 | 为现有 Skill 添加 ACK/EKS/GKE/AKS 差异化内容 | 规划中 |
| Demo 场景扩展 | 新增 10 个本地演示场景，覆盖全部 Skill 分类 | 进行中 |

### 待开发 Skill

| 优先级 | Skill ID | 场景 | 分类 |
|--------|---------|------|------|
| P1 | SKILL-NODE-002 | 节点磁盘压力诊断 | node |
| P1 | SKILL-NET-004 | NetworkPolicy 连通性问题 | network |
| P2 | SKILL-WORK-002 | StatefulSet 故障诊断 | workload |
| P2 | SKILL-WORK-003 | DaemonSet 故障诊断 | workload |
| P2 | SKILL-WORK-004 | Job/CronJob 故障诊断 | workload |

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
