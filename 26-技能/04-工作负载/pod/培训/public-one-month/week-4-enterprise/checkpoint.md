---
title: 'Week 4 Checkpoint: 终极自测'
description: '- 企业级运维能力评估'
summary: '本文档是整个一个月学习计划的终极自测。它涵盖了四个星期的核心知识点，从基础概念到企业级实践，帮助你评估学习成果并发现薄弱环节。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- prometheus
- grafana
- istio
- cilium
- argocd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 4 Checkpoint: 终极自测 是什么'
- '如何 Week 4 Checkpoint: 终极自测'
trigger_keywords:
- Week
- 'Checkpoint:'
- 终极自测
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
- policy-basics
- backup-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 4 Checkpoint: 终极自测
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[Kubernetes|kubernetes]] 综合自测
  - K8s 终极检验
  - 毕业自测题
  - 企业级运维能力评估
trigger_keywords:
  - 自测
  - checkpoint
  - Week 4
  - 终极
  - 毕业
  - 综合评估
  - SLO
  - GitOps
  - [[ArgoCD|ArgoCD]]
reading_level: advanced
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - domain-20-enterprise-monitoring-alerting
  - 发布变更
  - domain-25-[[17-系统基础/06-知识字典/security/cloud-native-security.md|cloud-native-security]]
related_topics:
  - 生产运维/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
  - 生产运维/topic-learn/public-training/one-month/projects/p5-graduation-project
  - 生产运维/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - 生产运维/topic-learn/public-training/one-month/week-4-enterprise/day-25-production-best-practices
---

# Week 4 Checkpoint: 终极自测

## 概述

本文档是整个一个月学习计划的终极自测。它涵盖了四个星期的核心知识点，从基础概念到企业级实践，帮助你评估学习成果并发现薄弱环节。

自测规则：独立完成，不查阅资料。每道题先写下自己的答案，然后再对照参考要点。最终评分将帮助你判断自己的掌握程度和后续学习方向。

---

## 一、综合概念 (每题 3 分，共 15 分)

### 1. ArgoCD Sync Policy 中 automated vs manual 的选择依据是什么？生产环境如何设计？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
- Automated: 自动检测 Git 仓库变更并同步到集群，适合开发/测试环境，减少手动操作
- Manual: 需要手动触发同步，适合生产环境需要审批的场景
- 生产建议: 使用 Automated 配合 PR 审批流程，设置 `syncPolicy.automated.prune=false`（防止自动删除资源）和 `syncPolicy.automated.selfHeal=true`（自动修复配置漂移）
- 高级策略: 使用 ApplicationSet 实现多环境的差异化同步策略

**评分标准:**
- 3 分: 能区分 automated/manual 并给出生产环境的合理设计
- 2 分: 知道区别但生产环境设计不够完善
- 1 分: 只了解基本概念

---

### 2. Kyverno ClusterPolicy 如何实现"禁止使用 latest 镜像标签"的策略？

**你的回答:**

```yaml
(在此写下你的策略 YAML)



```

**参考要点:**
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: require-image-tag
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "An image tag is required."
      pattern:
        spec:
          containers:
          - image: "*:*"
  - name: validate-image-tag
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "Using 'latest' image tag is not allowed."
      pattern:
        spec:
          containers:
          - image: "!*:latest"
```
关键点：两条规则——第一条确保有 tag，第二条确保 tag 不是 latest。`background: true` 会扫描已有资源。

**评分标准:**
- 3 分: YAML 正确且完整
- 2 分: 逻辑正确但语法有小问题
- 1 分: 只知道概念无法写出 YAML

---

### 3. 在 1000+ 节点的生产集群中，etcd 性能瓶颈通常在哪里？如何排查和优化？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
- **磁盘 IO 瓶颈**: etcd 每个 Write 操作需要 fsync 到 WAL 日志，磁盘延迟直接影响写入性能。排查: `etcdctl endpoint status` 查看 DB SIZE 和 RAFT APPLIED INDEX；Prometheus 指标 `etcd_disk_wal_fsync_duration_seconds`。优化: 使用 NVMe SSD，确保 fsync 延迟 < 10ms
- **大 Value 问题**: 在 etcd 中存储大量数据（如 ConfigMap > 1MB）会导致 etcd 性能下降。排查: `etcdctl get "" --prefix --keys-only | wc -l` 检查 key 数量；Prometheus 指标 `etcd_mvcc_db_total_size_in_bytes`。优化: 数据库大小超过 8GB 需要压缩（compaction）和碎片整理（defrag）
- **网络延迟**: Raft 协议需要 Leader 与 Follower 之间的网络通信。排查: Prometheus 指标 `etcd_network_peer_round_trip_time_seconds`。优化: 确保 Member 间 RTT < 10ms，使用专用网络
- **API Server 压力**: 大量 List 请求占用 API Server 内存。优化: 使用 API Priority and Fairness 限流、客户端使用 Informer 缓存

**评分标准:**
- 3 分: 能识别多个瓶颈点并给出排查和优化方法
- 2 分: 能识别主要瓶颈（磁盘 IO）
- 1 分: 只知道基本概念

---

### 4. 设计一个双活灾备方案：两个可用区，RTO < 5 分钟，RPO < 1 分钟，你如何架构？

**你的回答:**

```
(在此写下你的架构设计)




```

**参考要点:**
- **多集群部署**: 在两个可用区各部署一个 K8s 集群，使用联邦或多集群管理工具（如 Karmada）统一管理
- **应用层**: Deployment 在两个集群各运行一半副本，使用全局负载均衡（如阿里云 GTM/CloudDNS）实现流量分发
- **数据层**: 数据库使用同步复制（如 MySQL 半同步复制、Redis Cluster 多可用区部署），确保 RPO < 1 分钟
- **流量切换**: 全局 DNS 配合健康检查实现自动故障转移，DNS TTL 设置为 30-60 秒
- **监控**: 使用 Thanos 实现跨集群监控，统一告警面板
- **GitOps**: 两个集群使用相同的 Git 仓库，ArgoCD 自动同步
- **恢复策略**: 当主集群问题时，DNS 自动将流量切到备集群，RTO 取决于 DNS TTL + 新实例启动时间

**评分标准:**
- 3 分: 架构设计完整，覆盖应用、数据、流量、监控
- 2 分: 基本架构合理但缺少关键组件
- 1 分: 只了解基本概念

---

### 5. 深夜收到 PagerDuty 告警"Payment Service 5xx 激增"，按 FTA + FEBM 方法描述完整处置流程。

**你的回答:**

```
(在此写下完整的处置流程)





```

**参考要点:**

**FTA 故障树构建（自顶向下）**:

```
Payment Service 5xx 激增
├── 服务本身异常
│   ├── OOMKilled（内存泄漏）
│   ├── CrashLoopBackOff（启动失败）
│   └── 健康检查失败（readinessProbe）
├── 下游依赖异常
│   ├── 数据库连接池耗尽
│   ├── 缓存服务超时
│   └── 第三方 API 异常
├── 基础设施异常
│   ├── 节点 NotReady
│   ├── 网络抖动
│   └── DNS 解析失败
└── 流量异常
    ├── 突发流量超过容量
    ├── 恶意请求攻击
    └── 错误配置导致重试风暴
```

**FEBM 取证循证流程**:

1. **收集证据**: `kubectl get pods -n payment`（Pod 状态）→ `kubectl logs -n payment -l app=payment --tail=100`（日志）→ Prometheus 查看 5xx 趋势和 P99 延迟 → `kubectl describe pod` 查看 Events
2. **形成假设**: 根据证据判断最可能的问题路径（如"日志显示数据库连接超时 → 假设数据库异常"）
3. **验证假设**: 检查数据库监控指标和连接数 → 确认数据库确实有问题
4. **缓解措施**: 先恢复服务（如扩容、降级、切换），再修复根因
5. **根因分析**: 找到真正的根因（如数据库慢查询导致连接池耗尽）
6. **改进**: 添加连接池监控告警、设置慢查询阈值、增加连接池容量

---

## 二、实战场景 (每题 5 分，共 20 分)

### 6. 描述一个完整的 GitOps 工作流：从开发提交代码到生产部署

**你的回答:**

```
(在此写下完整流程)



```

**参考要点:**
1. 开发者在本地修改代码 → 推送到 Git 仓库（如 GitLab/GitHub）
2. CI 流水线触发：构建镜像 → 运行单元测试 → 镜像安全扫描 → 推送到 ACR
3. CI 更新 GitOps 仓库中的镜像 tag（自动提交 PR 或直接修改 manifests）
4. ArgoCD 检测到 GitOps 仓库变更 → 自动同步到预发集群
5. 预发环境验证通过后（自动测试 + 人工审批）→ 合并到 production 分支
6. ArgoCD 检测到 production 分支变更 → 自动同步到生产集群
7. 监控告警确认部署成功（错误率、延迟指标正常）

---

### 7. 如何设计一个 SLO 体系？包含哪些指标？如何设置错误预算告警？

**你的回答:**

```
(在此写下你的设计)



```

**参考要点:**
- **SLI 定义**: 可用性（成功请求比例）、延迟（P99 响应时间）、质量（数据新鲜度）
- **SLO 设定**: 可用性 99.9%（30 天错误预算 43.2 分钟）、延迟 P99 < 500ms
- **错误预算计算**: (1 - SLO) × 时间窗口。30 天窗口: 0.001 × 30 × 24 × 60 = 43.2 分钟
- **告警规则**: 消耗速率告警（如 1 小时内消耗了 14.4 倍的错误预算速率 → 告警）、剩余量告警（剩余 < 10% → 告警）
- **Recording Rules**: 预计算 SLI 指标，避免查询超时
- **Dashboard**: SLO 达成率趋势、错误预算消耗趋势、30 天预测线

---

### 8. 如何实现 K8s 集群的安全加固？列出至少 10 项措施

**你的回答:**

```
(在此列出安全措施)



```

**参考要点:**
1. **RBAC 最小权限**: 每个应用使用独立的 ServiceAccount，只授予必要的权限
2. **NetworkPolicy**: 默认拒绝所有流量，按需放行。使用 Kyverno Generate 自动生成
3. **Pod Security Standards**: 命名空间级别 enforce Restricted 或 Baseline 策略
4. **禁止 default ServiceAccount**: 为每个 Pod 指定独立的 ServiceAccount
5. **Secret 加密存储**: 启用 etcd EncryptionConfiguration，或使用 Vault/Sealed Secrets
6. **审计日志**: 配置 Audit Policy，将日志采集到 SLS 进行分析
7. **镜像签名验证**: 使用 Cosign 或 ACR 企业版镜像签名，只允许运行可信镜像
8. **运行时安全**: 部署 Falco 或类似工具，监控异常进程调用和文件访问
9. **漏洞扫描**: 使用 ACR 的镜像安全扫描，定期扫描集群中运行的镜像
10. **证书自动轮转**: 启用 kubelet 和 API Server 证书的自动轮换
11. **禁止特权容器**: 使用 Kyverno 策略禁止 privileged: true
12. **资源限制**: 强制所有容器设置 resources.limits，防止资源耗尽

---

### 9. 设计一个高可用的 Prometheus 监控方案

**你的回答:**

```
(在此写下你的设计)



```

**参考要点:**
- **高可用 Prometheus**: 部署 2 个 Prometheus 副本（使用 StatefulSet），配置相同的外部标签（`prometheus_replica`）
- **Thanos Sidecar**: 每个 Prometheus 旁部署 Sidecar，将数据上传到 OSS
- **Thanos Querier**: 部署 2 个 Querier 实例，配置 `--query.replica-label=prometheus_replica` 实现去重
- **Thanos Store Gateway**: 从 OSS 读取历史数据，支持长期查询
- **Thanos Compactor**: 定期压缩和降采样 OSS 中的数据
- **Grafana**: 配置 Thanos Querier 作为数据源，实现统一查询面板
- **Alertmanager**: 部署 3 个实例组成集群，使用 mesh gossip 协议实现告警去重

---

## 三、综合评估 (15 分)

### 10. 综合项目评审

回顾你的毕业项目 P5，自评以下各项:

| 项目 | 完成情况 | 自评分 (0-3) |
|------|----------|--------------|
| Deployment + StatefulSet | | |
| StorageClass + PVC | | |
| Ingress + TLS | | |
| NetworkPolicy | | |
| RBAC 权限控制 | | |
| Prometheus 监控 | | |
| Loki 日志 | | |
| ArgoCD GitOps | | |
| 故障排查手册 | | |
| 文档完整性 | | |

---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 综合概念 | __ | 15 |
| 实战场景 | __ | 20 |
| 综合评估 | __ | 15 |
| **总分** | __ | **50** |

### 最终评估

- **45-50 分**: 优秀，完全掌握 K8s 全栈运维，具备独立管理生产集群的能力
- **35-44 分**: 良好，具备生产运维能力，部分领域需要继续深化
- **25-34 分**: 及格，核心知识掌握，需要在实战中持续练习
- **< 25 分**: 需要针对性复习薄弱领域，建议重新学习得分低的模块

### 各领域薄弱项分析

| 得分率 | 建议 |
|--------|------|
| 综合概念 < 60% | 重读 Week 3-4 的理论文档 |
| 实战场景 < 60% | 增加实操练习，完成 P3-P5 项目 |
| 综合评估 < 60% | 补全毕业项目的缺失模块 |

---

## 五、学习总结

### 最大收获

```
1.


2.


3.

```

### 仍需加强

```
1.


2.


3.

```

### 下一步计划

```
1.


2.


3.

```

---

## 六、持续学习路径

通过终极自测后，建议按以下路径继续深入学习：

**认证方向**: CKA（Certified Kubernetes Administrator）→ CKS（Certified Kubernetes Security Specialist）

**技术深入方向**:
- **网络**: Cilium eBPF、服务网格（Istio/Linkerd）
- **存储**: CSI 驱动开发、数据保护（Velero）
- **安全**: 供应链安全（Sigstore）、运行时安全（Falco）
- **可观测性**: OpenTelemetry、eBPF 可观测性
- **AI**: GPU 调度（K8s Device Plugin）、模型服务（KServe/Triton）

**实践建议**:
- 在生产环境中应用所学知识
- 参与 K8s 社区贡献（文档、Bug 修复、Feature 开发）
- 建立个人的 K8s 知识库和工具集
- 定期回顾和更新知识（K8s 每年发布 3 个小版本）

---

恭喜完成一个月的学习!

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
