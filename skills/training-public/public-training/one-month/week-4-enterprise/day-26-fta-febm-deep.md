---
title: 'Day 26: FTA/FEBM 专题深化'
description: 'title: Day 26: FTA/FEBM 专题深化'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- prometheus
- jaeger
- coredns
- daemonset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 26: FTA/FEBM 专题深化 是什么'
- '如何 Day 26: FTA/FEBM 专题深化'
trigger_keywords:
- Day
- '26:'
- FTA
- FEBM
- 专题深化
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- etcd-basics
- tracing-basics
created: "2026-05-23"
---

---
title: Day 26: FTA/FEBM 专题深化
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[entities/kubernetes|[[Kubernetes|kubernetes]]]] 故障树分析进阶
  - FEBM 取证循证方法深化
  - AI Agent 运维模式
  - K8s 问题全景树
trigger_keywords:
  - FTA
  - FEBM
  - 故障树
  - 取证循证
  - AI Agent
  - 故障诊断
  - 根因分析
  - 问题全景树
reading_level: advanced
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-25-production-best-practices
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 26: FTA/FEBM 专题深化

> **学习时间**: 4-5 小时 | **主题**: 故障诊断方法论进阶

---

## 概述

FTA（故障树分析，Fault Tree Analysis）和 FEBM（取证循证方法，Forensic Evidence-Based Method）是两种互补的故障诊断方法论。FTA 提供系统化的问题原因分析框架，帮助你从顶层事件出发，逐层分解所有可能的问题路径；FEBM 提供严谨的证据收集和假设验证方法，确保故障排查过程有据可依，避免凭直觉猜测导致的误判。

本课程将深入探讨这两种方法论在生产环境中的实际应用。你将学习如何为 Kubernetes 集群构建完整的故障树，如何使用 FEBM 方法系统化地排查复杂问题，以及如何将 AI Agent 技术与传统的故障诊断方法相结合，提升故障定位的效率和准确性。

**学习目标**：
- 深入学习 FTA 生产落地方法和实践技巧
- 掌握 Kubernetes 完整故障树的构建和分析
- 了解 AI Agent 运维模式与 FTA/FEBM 的结合方式
- 能够运用 FTA + FEBM 方法独立排查复杂生产问题

**前置条件**：
- 已完成 Day 19-20 的故障排查基础学习
- 了解 Kubernetes 核心组件和工作原理
- 有基本的排障实践经验

---

## 核心概念

### FTA 故障树分析

FTA（故障树分析）是一种自顶向下的故障分析方法，从一个不希望发生的顶事件（Top Event）出发，通过逻辑门（AND/OR）逐层分解为更具体的中间事件和基本事件。FTA 的核心价值在于：

1. **系统性**: 确保所有可能的问题路径都被考虑到，不会遗漏
2. **可视化**: 通过图形化的故障树直观展示问题传播路径
3. **定量化**: 可以为每个基本事件赋予权率，计算顶事件发生概率
4. **预防性**: 可以提前构建故障树，在问题发生时快速定位

#### FTA 逻辑门类型

| 逻辑门 | 符号 | 含义 | 示例 |
|--------|------|------|------|
| OR 门 | ⊕ | 任一子事件发生即触发 | Pod 问题 OR [[Service|Service]] 问题 → 应用不可用 |
| AND 门 | ⊗ | 所有子事件同时发生才触发 | CPU 满载 AND 内存不足 → 节点问题 |
| XOR 门 | ⊖ | 有且仅有一个子事件发生 | 人为误操作 XOR 程序 Bug → 数据丢失 |
| NOT 门 | ¬ | 子事件不发生 | NOT 告警触发 → 问题未被发现 |
| K/N 门 | K/N | N 个子事件中有 K 个发生 | 3/5 节点问题 → [[etcd|etcd]] 不可用 |

### FEBM 取证循证方法

FEBM（Forensic Evidence-Based Method）是一种基于证据的故障排查方法，核心原则是"先取证，后假设，再验证"。与传统的"凭直觉猜测"不同，FEBM 要求在每一步都有证据支撑。

#### FEBM 工作流程

```
证据收集 ──> 假设生成 ──> 假设验证 ──> 根因确认
   │              │              │              │
   ▼              ▼              ▼              ▼
 日志/指标     列出可能     收集验证证据    确认根因
 Events        原因列表     排除/确认假设   制定修复
 抓包/trace                            方案
```

#### 证据类型分类

| 证据类型 | 来源 | 收集方法 | 可靠度 |
|----------|------|---------|--------|
| **直接证据** | 日志、Events、Metrics | kubectl、Prometheus | 高 |
| **环境证据** | 配置文件、版本信息 | kubectl get/describe | 高 |
| **关联证据** | Trace、关联事件 | Jaeger、SLS | 中 |
| **推测证据** | 排除法、类比推理 | 逻辑推理 | 低 |

### AI Agent 与故障诊断

AI Agent 可以在 FTA/FEBM 流程中发挥重要作用：

1. **自动证据收集**: AI Agent 可以自动收集和整理分散在不同系统中的日志、指标和事件
2. **智能假设生成**: 基于历史问题数据和知识库，自动生成可能的问题原因
3. **模式识别**: 识别故障模式与历史案例的相似性，提供参考解决方案
4. **自动化修复**: 对于已知的故障模式，自动执行预定义的修复流程

---

## 实战演练

### 任务 1: 构建 K8s 问题全景树 (1h)

```
K8s 应用问题 (顶事件)
├─ [OR] 应用层问题
│  ├─ [OR] 代码缺陷
│  │  ├─ 未处理异常导致崩溃
│  │  ├─ 内存泄漏导致 OOM
│  │  └─ 死锁导致超时
│  │
│  ├─ [OR] 配置错误
│  │  ├─ 环境变量缺失/错误
│  │  ├─ ConfigMap 引用错误
│  │  ├─ Secret 内容过期
│  │  └─ 启动参数不正确
│  │
│  └─ [OR] 依赖服务不可用
│     ├─ 数据库连接失败
│     ├─ 缓存服务宕机
│     └─ 外部 API 超时
│
├─ [OR] 平台层问题
│  ├─ [OR] Pod 问题
│  │  ├─ Pending: 资源不足 / 调度约束
│  │  ├─ CrashLoopBackOff: 启动失败 / 探针失败
│  │  ├─ OOMKilled: 内存限制过低 / 内存泄漏
│  │  ├─ ImagePullBackOff: 镜像不存在 / 凭证错误
│  │  └─ Terminating: Finalizer 阻塞 / 优雅退出超时
│  │
│  ├─ [OR] Service 问题
│  │  ├─ Endpoints 为空: Selector 不匹配
│  │  ├─ 端口映射错误: targetPort 不正确
│  │  └─ ClusterIP 冲突: 手动指定冲突 IP
│  │
│  ├─ [OR] Ingress 问题
│  │  ├─ 路由规则错误: Path/Host 配置不匹配
│  │  ├─ TLS 证书过期: 证书未及时更新
│  │  ├─ 后端超时: upstream 超时配置不当
│  │  └─ Controller 问题: Ingress Controller Pod 异常
│  │
│  └─ [OR] 存储问题
│     ├─ PVC Pending: StorageClass 不存在 / 配额不足
│     ├─ 磁盘空间满: 日志/数据未清理
│     ├─ IO 性能差: 磁盘类型不匹配 / IO 瓶颈
│     └─ 挂载失败: 节点权限 / NFS 服务异常
│
├─ [OR] 控制平面问题
│  ├─ [AND] etcd 不可用 (多数节点问题)
│  │  ├─ etcd 磁盘 IO 高
│  │  ├─ etcd 网络分区
│  │  └─ etcd 数据损坏
│  │
│  ├─ [OR] API Server 过载
│  │  ├─ 大量 List 请求
│  │  ├─ 审计日志写入慢
│  │  └─ etcd 响应慢
│  │
│  ├─ [OR] Scheduler 问题
│  │  ├─ 调度插件错误
│  │  └─ 调度队列阻塞
│  │
│  └─ [OR] Controller Manager 问题
│     ├─ Leader 选举失败
│     └─ 控制循环卡住
│
└─ [OR] 基础设施问题
   ├─ [OR] 节点问题
   │  ├─ ECS 实例宕机
   │  ├─ kubelet 进程崩溃
   │  ├─ 容器运行时问题
   │  └─ 内核 Panic
   │
   ├─ [OR] 网络问题
   │  ├─ VPC 路由异常
   │  ├─ 安全组规则变更
   │  ├─ DNS 服务不可用
   │  └─ CNI 插件问题
   │
   └─ [OR] 存储后端问题
      ├─ 云盘服务降级
      ├─ NAS 服务不可用
      └─ OSS 访问异常
```

### 任务 2: FEBM 实战演练 - 应用间歇性超时 (1h)

```bash
# === FEBM 完整案例: 应用间歇性超时 ===

# Phase 1: 证据收集 (Evidence Collection)

echo "=== Phase 1: 证据收集 ==="

# 证据 E1: Prometheus 指标 - P99 延迟飙升
echo "E1: Prometheus P99 延迟指标"
curl -s 'http://prometheus:9090/api/v1/query?query=histogram_quantile(0.99,sum(rate(http_request_duration_seconds_bucket[5m]))by(le))' | jq '.data.result[0].value'
# 预期输出: ["1716019200","4.857"]  # P99 延迟从 200ms 飙升到 4.8s

# 证据 E2: Pod 日志 - 连接超时错误
echo "E2: Pod 日志中的错误"
kubectl logs -l app=my-app --tail=100 | grep -i "timeout\|connection refused"
# 预期输出:
# 2026-05-18 10:25:30 ERROR Failed to connect to db-service:3306 - Connection timed out
# 2026-05-18 10:26:15 ERROR Failed to connect to db-service:3306 - Connection timed out
# 2026-05-18 10:27:02 ERROR Failed to connect to db-service:3306 - Connection timed out

# 证据 E3: 节点资源使用率
echo "E3: 节点资源使用率"
kubectl top nodes
# 预期输出:
# NAME           CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-worker-1  800m         20%    4096Mi          25%
# node-worker-2  780m         19%    4096Mi          25%
# CPU和内存使用率正常

# 证据 E4: Pod 状态
echo "E4: Pod 状态"
kubectl get pods -l app=my-app -o wide
# 预期输出:
# NAME                      READY   STATUS    RESTARTS   AGE   IP            NODE
# my-app-7d9f8b6c4-abc12   1/1     Running   0          5d    10.0.1.100   node-worker-1
# my-app-7d9f8b6c4-def34   1/1     Running   0          5d    10.0.1.101   node-worker-2

# 证据 E5: DNS 解析测试
echo "E5: DNS 解析测试"
kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup db-service.default.svc.cluster.local
# 预期输出:
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      db-service.default.svc.cluster.local
# Address 1: 10.96.50.100 db-service.default.svc.cluster.local
# DNS 解析正常 -> 排除 DNS 问题

# 证据 E6: 网络连通性测试
echo "E6: 网络连通性"
kubectl run netshoot --image=nicolaka/netshoot --rm -it --restart=Never -- curl -so /dev/null -w '%{time_connect}\n' http://db-service:3306
# 预期输出 (异常):
# 5.001  # 连接时间 5秒，说明有连接超时

# 证据 E7: CNI 组件状态
echo "E7: CNI 组件状态"
kubectl get pods -n kube-system -l app=terway
# 预期输出:
# NAME           READY   STATUS    RESTARTS   AGE
# terway-xxx     1/1     Running   3          30d    # 注意: 有3次重启

# 证据 E8: CNI 日志
echo "E8: CNI 日志中的错误"
kubectl logs -n kube-system terway-xxx --tail=200 | grep -i "error\|warn"
# 预期输出:
# 2026-05-18 10:25:28 WARN IP allocation retry for pod 10.0.1.100
# 2026-05-18 10:26:10 ERROR Failed to setup network for sandbox abc123: timeout
# 2026-05-18 10:27:05 WARN ENI attachment timeout for node node-worker-2
```

**假设列表和验证结果**：

| 序号 | 假设 | 验证方法 | 验证证据 | 结果 |
|------|------|---------|---------|------|
| H1 | DNS 解析慢 | nslookup 测试 | E5: DNS 解析正常 | **排除** |
| H2 | Pod 资源不足 | top pod / describe | E3: 资源正常 | **排除** |
| H3 | NetworkPolicy 阻断 | 检查策略 | 无策略存在 | **排除** |
| H4 | Service selector 不匹配 | get endpoints | Endpoints 正常 | **排除** |
| H5 | CNI 网络不稳定 | CNI 日志分析 | E7+E8: CNI 重启和超时 | **确认** |
| H6 | 数据库连接池满 | 检查数据库指标 | DB 连接数正常 | **排除** |

**根因确认**: Terway CNI 插件的 ENI 附加超时导致部分 Pod 网络间歇性不稳定。

**修复方案**:

```bash
# 临时缓解: 重启受影响的 Terway Pod
kubectl delete pod -n kube-system terway-xxx

# 永久修复: 更新 Terway 配置，增加超时参数
kubectl edit configmap terway-config -n kube-system
# 修改:
#   eni_attach_timeout: "60"     # 增加ENI附加超时
#   eni_max_pool_size: "5"       # 增加ENI池大小

# 重启 Terway DaemonSet 使配置生效
kubectl rollout restart daemonset terway -n kube-system

# 验证修复
kubectl get pods -n kube-system -l app=terway
kubectl logs -n kube-system terway-xxx --tail=50 | grep -i "error"
```

### 任务 3: 为复杂问题构建完整 FTA + FEBM (30min)

**练习: 构建你的故障分析案例**

选择一个你遇到过的（或想象的）生产问题，按照以下模板完整记录：

```markdown
# 故障分析报告: [问题名称]

## 1. FTA 故障树

[在此构建故障树，至少包含3层]

## 2. FEBM 证据收集

| 证据编号 | 证据内容 | 来源 | 收集时间 |
|---------|---------|------|---------|
| E1 | | | |
| E2 | | | |
| E3 | | | |

## 3. 假设列表

| 假设 | 验证方法 | 结果 |
|------|---------|------|
| | | |

## 4. 根因分析

[确认的根因描述]

## 5. 修复方案

- 临时:
- 永久:

## 6. 预防措施

- [ ] 监控告警:
- [ ] 变更管理:
- [ ] 架构优化:
```

---

## 配置参考

### FTA 故障树 YAML 格式

```yaml
apiVersion: fta.kudig.io/v1
kind: FaultTree
metadata:
  name: k8s-application-failure
  annotations:
    author: sre-team
    last-updated: "2026-05-18"
spec:
  topEvent:
    name: "K8s 应用问题"
    description: "应用无法正常提供服务"
  gates:
  - id: gate-1
    type: OR
    parent: topEvent
    children: [app-layer, platform-layer, control-plane, infra]
  - id: gate-2
    type: OR
    parent: platform-layer
    children: [pod-issues, service-issues, ingress-issues, storage-issues]
  - id: gate-3
    type: AND
    parent: etcd-unavailable
    children: [etcd-disk-io, etcd-network-partition, etcd-data-corruption]
    description: "需要多数节点同时问题"
  events:
  - id: app-layer
    name: "应用层问题"
    probability: 0.4
  - id: platform-layer
    name: "平台层问题"
    probability: 0.35
  - id: control-plane
    name: "控制平面问题"
    probability: 0.1
  - id: infra
    name: "基础设施问题"
    probability: 0.15
```

### FEBM 证据收集脚本

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: feb-evidence-collector
  namespace: troubleshooting
spec:
  template:
    spec:
      serviceAccountName: evidence-collector
      containers:
      - name: collector
        image: busybox:1.36
        command:
        - /bin/sh
        - -c
        - |
          echo "=== K8S Evidence Collection $(date) ==="
          echo ""
          echo "--- Cluster Info ---"
          kubectl version --short 2>&1 || true
          kubectl get nodes -o wide 2>&1 || true
          echo ""
          echo "--- Pod Status ---"
          kubectl get pods -A --field-selector=status.phase!=Running 2>&1 || true
          echo ""
          echo "--- Recent Events ---"
          kubectl get events -A --sort-by='.lastTimestamp' 2>&1 | tail -50 || true
          echo ""
          echo "--- Node Resources ---"
          kubectl top nodes 2>&1 || true
          echo ""
          echo "--- Component Status ---"
          kubectl get cs 2>&1 || true
          echo ""
          echo "--- CoreDNS Status ---"
          kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide 2>&1 || true
      restartPolicy: Never
  backoffLimit: 0
```

### FTA 概率参数参考

| 问题层级 | 典型概率 | MTBF | MTTR |
|----------|---------|------|------|
| 应用代码 Bug | 35-40% | 720h | 2-4h |
| 配置错误 | 15-20% | 2160h | 0.5-2h |
| Pod 崩溃/重启 | 15-20% | 720h | 1-5min |
| 网络问题 | 10-15% | 4320h | 5-30min |
| 存储问题 | 5-10% | 8640h | 10-60min |
| 节点问题 | 3-5% | 8760h | 5-30min |
| 控制平面问题 | 1-3% | 43800h | 15-60min |

---

## 常见问题

### Q1: FTA 和 FEBM 如何选择使用？

**A**: 两种方法互补使用：
- **FTA 适合**: 问题发生前的预防性分析、构建问题知识库、新人培训
- **FEBM 适合**: 问题发生时的实时排查、需要证据支撑的根因分析
- **最佳实践**: 先用 FTA 框架缩小范围，再用 FEBM 方法验证确认

### Q2: 故障树构建到什么粒度合适？

**A**: 粒度原则：
1. **第一层**: 问题大类别（应用/平台/控制面/基础设施）
2. **第二层**: 具体组件（Pod/Service/Ingress/存储等）
3. **第三层**: 具体问题现象（CrashLoopBackOff/Pending/OOMKilled 等）
4. **第四层**: 根因级别（配置错误/代码 Bug/资源不足等）
一般到第四层即可，过细的粒度反而降低实用性

### Q3: 如何在日常工作中持续积累故障树？

**A**: 建立问题知识库：
1. 每次问题修复后，更新对应的故障树分支
2. 定期（每月）回顾问题报告，补充新的故障模式
3. 将故障树与监控告警关联，告警触发时自动展示对应的分析路径
4. 团队共享故障树，作为排障 Runbook 的一部分

### Q4: FEBM 中证据和假设的关系是什么？

**A**: 证据是客观事实，假设是主观推断：
1. **先收集证据，再生成假设**：避免先入为主的偏见
2. **一个证据可以验证多个假设**：如 Pod 日志异常可以支持"代码 Bug"和"配置错误"两个假设
3. **假设必须有对应的验证方法**：无法验证的假设没有价值
4. **证据的可靠性分级**：直接证据 > 环境证据 > 关联证据 > 推测证据

### Q5: 如何将 AI Agent 融入 FTA/FEBM 流程？

**A**: AI Agent 可以在以下环节发挥作用：
1. **自动证据收集**: AI Agent 连接多个数据源（日志/指标/Trace），自动聚合相关证据
2. **智能假设推荐**: 基于历史问题数据库，推荐最可能的问题原因
3. **自动化验证**: 对于常见故障模式，自动执行验证命令
4. **修复建议**: 基于确认的根因，推荐修复方案并自动执行（需审批）
5. **知识沉淀**: 自动将故障分析过程整理为知识库文档

---

## 要点总结

- **FTA** 从顶事件出发，通过逻辑门逐层分解问题原因，确保系统性覆盖
- **FEBM** 要求先收集证据，再生成假设，然后逐一验证，避免直觉猜测
- **问题全景树** 是团队共同维护的知识资产，应持续更新
- **证据的可靠性分级** 有助于判断分析结论的可信度
- **FTA + FEBM 组合使用** 效果最佳：FTA 提供分析框架，FEBM 提供验证方法
- **AI Agent** 可以加速证据收集和假设生成，但最终的根因判断仍需要人的经验

---

## 延伸阅读

- [文件: `../../domain-10-troubleshooting-diagnostics/topic-fta/23-fta-production-quick-start.md`](../../domain-10-troubleshooting-diagnostics/topic-fta/23-fta-production-quick-start.md)
- [文件: `../../domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis.md`](../../domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis.md)
- [文件: `../../domain-10-troubleshooting-diagnostics/topic-febm/08-febm-production-quick-start.md`](../../domain-10-troubleshooting-diagnostics/topic-febm/08-febm-production-quick-start.md)
- [文件: `../../domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md`](../../domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md)
- [NASA FTA Handbook](https://ntrs.nasa.gov/citations/20020003100)
- [Google SRE Book - Understanding Outages](https://sre.google/sre-book/understanding-outages/)
