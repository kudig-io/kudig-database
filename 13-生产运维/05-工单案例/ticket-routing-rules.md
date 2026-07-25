---
title: 工单分类与路由规则（阿里云/专有云版）
description: 工单优先级分类、关键词/意图到 Domain/Skill/FTA 的映射规则，以及阿里云专有云高频工单类型的路由示例。
summary: 工单优先级分类、关键词/意图到 Domain/Skill/FTA 的映射规则，以及阿里云专有云高频工单类型的路由示例。
category: production-operations
tags:
- ai-agent
- ticket-agent
- ticket-routing
- classification
- alicloud
- apsara-stack
- sre
- skill
- fta
- priority
tier: core
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
difficulty: intermediate
reading_level: intermediate
audience:
- AI 工程师
- SRE
- 技术支持
- 工单智能体开发
estimated_read_time: 25min
intent_queries:
- 工单怎么分类
- 工单如何路由到对应 Skill
- 阿里云专有云高频工单有哪些
- P0 P1 P2 P3 怎么区分
trigger_keywords:
- 工单路由
- 工单分类
- P0
- P1
- P2
- P3
- 升级
- skill
- fta
prerequisites:
- kudig-agent-basics
- domain-overview
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




# 工单分类与路由规则（阿里云/专有云版）

> **适用版本**：Kubernetes v1.28 - v1.32 | **最后更新**：2026-06-26
> **文档定位**：为 KUDIG 工单智能体提供从工单描述到 Domain/Skill/FTA 的自动路由规则。

---

## 1. 工单分类目标

工单智能体需要完成以下分类决策：

1. **优先级**：P0 / P1 / P2 / P3
2. **技术域（Domain）**：domain-01 至 domain-20
3. **处理技能（Skill）**：具体诊断/修复 Skill 编号
4. **故障树（FTA）**：可选，用于复杂根因分析
5. **升级判定**：是否需要立即转人工

准确的路由可以显著缩短首次响应时间和根因定位时间。分类时不能只看表面关键词，还需要结合环境、影响范围和紧急程度综合判断。

---

## 2. 优先级分类

### 2.1 判定标准

| 优先级 | 条件 | 响应时限 | 处理策略 |
|:---|:---|:---:|:---|
| **P0 紧急** | 生产环境服务完全不可用、数据丢失、安全事件、超过 50% 节点异常 | 立即 | 立即响应，快速止血，必要时立即升级 |
| **P1 高** | 生产环境服务降级、部分功能不可用、单可用区故障、核心应用异常 | 15 分钟 | 完整诊断 + 修复，1 小时内给出方案 |
| **P2 中** | 非生产环境异常、预警类问题、性能波动、可延后修复的缺陷 | 30 分钟 | 标准诊断流程，给出修复建议 |
| **P3 低** | 咨询、优化建议、文档问题、低影响配置询问 | 按队列 | 提供方案或转知识库 |

### 2.2 自动优先级关键词

| 关键词 | 默认优先级 |
|:---|:---:|
| 服务中断 / 不可用 / 全部失败 / 数据丢失 / 集群崩溃 | P0 |
| 调度失败 / Pod 起不来 / 服务访问慢 / 节点 NotReady | P1 |
| 告警 / 容量不足 / 证书即将过期 / 备份失败 | P2 |
| 怎么配置 / 如何优化 / 最佳实践 / 文档 | P3 |

### 2.3 优先级调整规则

- 若客户明确说明为测试环境，自动降级一级（P0 不变）
- 若工单中同时出现多个关键词，取最高优先级
- 若连续三次回复未解决问题，建议升级为 P1

---

## 3. 意图 → Domain/Skill/FTA 映射

### 3.1 节点与基础设施

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| 节点 NotReady / 节点异常 | 故障诊断 | k8s-node-notready | node-fta | 优先检查 kubelet/containerd |
| Pod 调度失败 / Pending | 故障诊断 | k8s-pod-pending | pod-fta | 资源、污点、亲和性 |
| 节点资源不足 / CPU 高 / 内存满 | 可靠性 | k8s-capacity-planning | resource-fta | 扩容或优化 |
| 证书过期 / apiserver 证书 | 安全 | k8s-certificate-rotation | cert-fta | 需区分 kubelet/apiserver |

### 3.2 工作负载与应用

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| Pod CrashLoopBackOff / 重启 | 故障诊断 | k8s-pod-crashloop | pod-fta | 退出码 + 日志 |
| OOMKilled / 内存溢出 | 平台工程 | k8s-memory-optimization | oom-fta | limits/request 调优 |
| Java 应用异常 / GC 问题 | 工作负载 | java-jvm-tuning | jvm-fta | JVM 参数、堆dump |
| Spring Boot 启动失败 | 工作负载 | java-springboot-k8s | app-fta | 配置、探针 |

### 3.3 网络与流量

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| Service 不通 / DNS 解析失败 | 网络 | k8s-service-unreachable | network-fta | CoreDNS、SLB、安全组 |
| Ingress 不通 / 证书错误 | 网络 | k8s-ingress-troubleshoot | ingress-fta | ALB/NLB、Secret |
| Pod 网络不通 / 跨节点 | 网络 | k8s-cni-troubleshoot | cni-fta | Terway/Calico/Cilium |
| 服务延迟高 / 超时 | 网络 | k8s-network-latency | latency-fta | 链路与延迟 |

### 3.4 存储

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| PVC 绑定失败 / Pending | 存储 | k8s-pvc-pending | pvc-fta | StorageClass、配额 |
| PV 挂载失败 / MountVolume 错误 | 存储 | k8s-pv-mount-fail | mount-fta | CSI、节点路径 |
| 存储扩容失败 | 存储 | k8s-volume-expansion | expansion-fta | SC allowVolumeExpansion |
| 备份失败 / Velero 错误 | 存储 | velero-backup-recovery | backup-fta | OSS、快照 |
| Rook-Ceph / Longhorn 异常 | 存储 | rook-ceph-production / longhorn-production | storage-fta | 分布式存储 |

### 3.5 安全与合规

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| RBAC 权限不足 / 拒绝访问 | 安全 | k8s-rbac-troubleshoot | rbac-fta | Role/ClusterRole |
| 镜像安全 / 漏洞 | 安全 | k8s-image-security | image-fta | ACR 扫描 |
| 网络策略拦截 | 安全 | k8s-network-policy | netpol-fta | NetworkPolicy |
| Pod 安全策略拒绝 | 安全 | k8s-pod-security | pss-fta | PSS/PSP |

### 3.6 发布与变更

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| Helm 发布失败 / 回滚 | 发布变更 | helm-production-guide | helm-fta | values、依赖 |
| ArgoCD 同步失败 | 发布变更 | argocd-troubleshoot | gitops-fta | Git、权限 |
| GitLab CI / 镜像构建失败 | 发布变更 | cicd-troubleshoot | cicd-fta | Runner、Dockerfile |

### 3.7 可观测性

| 意图关键词 | Domain | Skill | FTA | 备注 |
|:---|:---|:---|:---|:---|
| 监控缺失 / Prometheus 异常 | 可观测性 | prometheus-troubleshoot | prom-fta | Target、规则 |
| 日志采集失败 / SLS | 可观测性 | logging-troubleshoot | logging-fta | Fluent Bit/Logtail |
| 告警风暴 | 可观测性 | alert-management | alert-fta | 降噪、分组 |

---

## 4. 阿里云专有云高频工单类型

| 工单类型 | 典型描述 | 优先级 | 路由 Skill | 特殊说明 |
|:---|:---|:---:|:---|:---|
| 节点 NotReady | 某可用区大量节点 NotReady | P0 | k8s-node-notready | 可能涉及 ASO/天基 |
| Pod 调度失败 | 生产命名空间 Pod 全部 Pending | P1 | k8s-pod-pending | 检查节点池、配额 |
| 服务无法访问 | 通过 SLB 无法访问业务 | P0/P1 | k8s-service-unreachable | 检查 Terway、SLB、安全组 |
| PVC 挂载失败 | MySQL Pod 启动报挂载错误 | P1 | k8s-pv-mount-fail | 检查 CSI、云盘状态 |
| 证书过期 | kubelet/apiserver 证书告警 | P0/P1 | k8s-certificate-rotation | 专有云底座证书需升级 |
| 镜像拉取失败 | Pod ImagePullBackOff | P1 | k8s-image-pull-fail | ACR/专有云 Harbor |
| 应用发布失败 | Helm/ArgoCD 同步失败 | P1 | helm-production-guide | 检查 values、依赖 |
| 监控告警异常 | Prometheus 无法抓取 | P2 | prometheus-troubleshoot | 检查 Target、网络 |
| 集群升级失败 | ACK/专有云版本升级卡住 | P1 | k8s-upgrade-troubleshoot | 可能需提交天基工单 |
| 安全策略拦截 | Kyverno/OPA 拒绝 Pod | P2 | k8s-policy-governance | 检查策略规则 |

### 4.1 专有云底座相关判定

当工单描述中出现以下信息时，应提高对 ASO/天基问题的关注：

- 多个独立租户同时出现同类问题
- 节点批量异常且与近期底座变更时间吻合
- ASO 控制台显示产品任务失败
- 天基告警邮件或短信触发

---

## 5. 路由示例

### 示例 1

> 工单描述："生产环境 mysql-0 Pod 一直 Pending，PVC 显示 Pending，集群是阿里云 ACK。"

- 优先级：P1
- Domain：存储
- Skill：k8s-pvc-pending
- 诊断路径：
  1. `kubectl describe pvc mysql-data -n production`
  2. `kubectl get sc`
  3. `kubectl get events -n production`
- 可能根因：StorageClass 不存在、云盘售罄、命名空间 ResourceQuota 不足

### 示例 2

> 工单描述："所有节点突然 NotReady，kubectl get nodes 卡住，怀疑是 apiserver 问题。"

- 优先级：P0
- Domain：故障诊断
- Skill：k8s-node-notready
- 升级：立即升级人工
- 诊断路径：
  1. 检查控制平面节点状态
  2. 检查 apiserver/etcd 日志
  3. 检查专有云底座 / ASO 告警

### 示例 3

> 工单描述："请问如何在 ACK 上给 StatefulSet 配置 ESSD 云盘 StorageClass？"

- 优先级：P3
- Domain：存储
- Skill：stateful-app-storage-patterns
- 处理：直接给出配置示例与检查清单

### 示例 4

> 工单描述："昨晚发布后人脸识别服务延迟从 50ms 涨到 2s，CPU 和内存都正常。"

- 优先级：P1
- Domain：网络
- Skill：k8s-network-latency
- 可能根因：SLB 后端健康检查异常、CNI 插件问题、新 Pod 跨可用区访问

---

## 6. 路由异常与兜底策略

| 场景 | 处理 |
|:---|:---|
| 关键词匹配多个 Skill | 返回候选列表，按置信度排序，默认选择最高 |
| 无匹配关键词 | 询问用户："请描述具体异常现象、涉及资源和影响范围" |
| 描述模糊 | 触发信息收集模板，先确认优先级和影响范围 |
| 同时命中 P0 条件 | 跳过分类，直接进入紧急响应流程并提示升级 |
| 置信度低于 0.6 | 列出候选 Skill，由用户或值班工程师确认 |

### 6.1 置信度评分参考

| 评分 | 含义 | 动作 |
|:---:|:---|:---|
| 0.9 - 1.0 | 高度匹配 | 直接路由 |
| 0.7 - 0.9 | 较匹配 | 路由并列出备选 |
| 0.5 - 0.7 | 可能匹配 | 请求用户确认 |
| < 0.5 | 不匹配 | 触发兜底话术 |

---

## 7. 路由决策流程

```
用户输入
  │
  ▼
提取关键词与实体（命名空间、Pod、节点、云产品）
  │
  ▼
判断优先级（P0/P1/P2/P3）
  │
  ▼
匹配 Domain/Skill/FTA
  │
  ▼
是否需要升级？
  ├─ 是 → 输出升级通知 + 交接单
  └─ 否 → 输出首条诊断命令 + 信息收集模板
```

---

## 8. FAQ

**Q: 一个工单同时涉及网络和存储问题怎么办？**
A: 优先选择影响最大的根因方向。如果无法判断，先按网络 Skill 排查，因为网络问题常表现为存储挂载失败。

**Q: 客户未说明环境是否生产，如何判定优先级？**
A: 默认按 P2 处理，同时回复中询问环境与影响范围。若描述中有“全部失败”“无法访问”等词，可上调至 P1。

**Q: 如何处理专有云天基告警触发的工单？**
A: 直接路由至专有云底座相关 Skill，并在回复中引导客户提供 ASO/天基任务 ID。

---

## 9. 质量检查清单

- [ ] 优先级判定是否有明确的数据支撑（环境、影响范围、关键字）
- [ ] Skill 选择是否匹配根因而非表面现象
- [ ] 是否标注了需要立即升级的条件
- [ ] 是否提供了首条诊断命令
- [ ] 是否区分了阿里云/专有云特有路径
- [ ] 是否在无法分类时给出兜底话术
- [ ] 是否记录了置信度和备选 Skill

---

## Related

- [[13-生产运维/03-事件响应/escalation-playbook|升级与交接协议]]
- [[13-生产运维/06-回复话术/README|回复话术库]]
- [[AGENTS.md|Agent 行为规范与工作流]]

## See Also

- 专有云 ASO 组件索引
- 天基运维工单处理指南


<!-- risk-assessed -->
