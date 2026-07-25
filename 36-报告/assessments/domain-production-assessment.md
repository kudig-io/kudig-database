---
title: Domain Production Assessment
description: '**评估视角**: 生产环境运维（SRE / Platform Ops / 运维工程师）'
summary: '**评估视角**: 生产环境运维（SRE / Platform Ops / 运维工程师）'
category: references
tags:
- assessment
- production-readiness
- prometheus
- grafana
- istio
- envoy
- cilium
- docker
- opa
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Domain Production Assessment 是什么
- 如何 Domain Production Assessment
- Kubernetes production assessment.md 最佳实践
trigger_keywords:
- Domain
- Production
- Assessment
- production
- assessment.md
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- policy-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain 结构生产环境维度评估报告

**评估日期**: 2026-05-21  
**评估视角**: 生产环境运维（SRE / Platform Ops / 运维工程师）  
**当前 Domain 数量**: 43 个  
**总文档数**: ~1,050+

---

## 一、核心结论

**当前 43 个 Domain 存在严重的维度混杂与内容重叠，从生产环境视角看，建议整合为 18 个核心 Domain。**

当前结构按「技术组件 + 工具品牌 + 内容载体 + 生命周期阶段 + 部署场景」五维混杂分类，导致：
- 同一生产场景的知识分散在 3-5 个 Domain
- 排查问题时需跨 7+ 个目录检索
- 内容重复率估计达 25-30%

---

## 二、当前结构诊断

### 2.1 分类维度混杂（5 种维度混用）

| 维度类型 | 代表 Domain | 问题 |
|---------|------------|------|
| **技术组件** | networking, storage, security, workloads | ✅ 合理 |
| **工具品牌** | enterprise-monitoring-alerting, logging-management-analytics | ❌ 按品牌分，知识碎片化 |
| **内容载体** | papers, yaml-manifests, kubernetes-events | ❌ 不是知识域，是格式 |
| **生命周期** | design-principles, production-operations, disaster-recovery | ⚠️ 与其他维度正交 |
| **部署场景** | multi-cloud-hybrid, edge-computing | ⚠️ 与架构域耦合 |

### 2.2 严重重叠区域（Top 5）

#### 🔴 重叠区 1：可观测性 — 3 个 Domain 共 62 文件
- `domain-8-observability` (35 文件)：架构、指标、日志、链路、告警、SLO、混沌工程
- `domain-20-enterprise-monitoring-alerting` (15 文件)：Prometheus, Grafana, OpenTelemetry, Thanos, Datadog...
- `domain-21-logging-management-analytics` (12 文件)：ELK, [[fluentd|Fluentd]], Loki, Splunk...

**生产影响**：排查一个监控问题时，需要在 observability、monitoring-alerting、logging 三个目录间跳转。

#### 🔴 重叠区 2：安全 — 3 个 Domain 共 56 文件
- `domain-7-security` (24 文件)：认证授权、网络策略、运行时安全、合规、零信任
- `domain-25-cloud-native-security` (18 文件)：Falco, Sysdig, Aqua, Kyverno, Vault, OPA
- `domain-39-supply-chain-security` (14 文件)：SBOM, SLSA, Sigstore, Cosign

**生产影响**：安全事件响应时，不知道去 security 还是 cloud-native-security 查资料。供应链安全与其他安全域边界模糊。

#### 🔴 重叠区 3：平台运维 — 3 个 Domain 共 80 文件
- `domain-9-platform-ops` (31 文件)：集群生命周期、容量规划、监控告警、GitOps、灾备、排障
- `domain-36-platform-engineering` (15 文件)：IDP, Backstage, Kratix, Crossplane, Golden Path
- `domain-18-production-operations` (34 文件)：监控、日志、APM、GitOps、IaC、变更管理、事件响应、容量规划、灾备...

**生产影响**：`production-operations` 实质上是一个「大杂烩」，几乎覆盖了 platform-ops 的全部内容，加上一些 engineering 内容。三个 Domain 的目录树有大量同名/同主题文档。

#### 🟡 重叠区 4：网络 — 4 个 Domain 共 104 文件
- `domain-5-networking` (57 文件)：K8s 网络架构、CNI、Service、Ingress、NetworkPolicy
- `domain-15-network-fundamentals` (10 文件)：TCP/IP、DNS、负载均衡、SDN
- `domain-35-ebpf-technology` (13 文件)：eBPF、Cilium
- `domain-26-service-mesh-microservices` (16 文件)：Istio, Linkerd, Envoy
- `domain-40-cloud-native-api-gateway` (18 文件)：Gateway API, Higress

**生产影响**：网络问题排查时，ServiceMesh 和 API Gateway 到底算不算 networking？Cilium 是网络还是 eBPF？

#### 🟡 重叠区 5：存储 — 2 个 Domain 共 30 文件
- `domain-6-storage` (21 文件)：K8s 存储、PV/PVC、CSI
- `domain-16-storage-fundamentals` (9 文件)：块/文件/对象存储、RAID

**生产影响**：区分度过低，生产环境中两者常同时需要。

### 2.3 内容分布极度不均

| Domain | 文件数 | 评估 |
|--------|--------|------|
| domain-42-application-architecture | 98 | 🔴 过大，且偏向业务架构 |
| domain-5-networking | 57 | 🟡 偏大 |
| domain-12-troubleshooting | 50 | 🟡 偏大（跨所有域） |
| domain-41-ai-agent | 52 | 🟡 偏大 |
| domain-1-architecture-fundamentals | 35 | ✅ 正常 |
| domain-8-observability | 35 | 🟡 正常但内容重复 |
| domain-18-production-operations | 34 | 🟡 过大（大杂烩） |
| domain-3-control-plane | 39 | ✅ 正常 |
| domain-32-yaml-manifests | 39 | 🔴 过大（内容载体不应作为独立域） |
| domain-9-platform-ops | 31 | ✅ 正常 |
| domain-17-cloud-provider | 3 | 🔴 过小 |
| domain-16-storage-fundamentals | 9 | 🔴 过小 |

### 2.4 生产环境关键缺失

| 缺失领域 | 说明 | 当前分散在 |
|---------|------|-----------|
| **SRE / 可靠性工程** | SLO/SLI 只在 observability 中有一点 | domain-8, domain-18 |
| **变更管理 / 发布工程** | 生产环境最核心的风险来源 | domain-18, domain-23, domain-9 |
| **容量与成本管理 (FinOps)** | 只在 platform-ops 和 production-ops 各有一篇 | domain-9, domain-18 |
| **数据保护 / 备份恢复** | 生产必备，但内容零散 | domain-18, domain-30, domain-9 |
| **事件响应 (Incident Response)** | 只在 security 和 production-ops 各有一篇 | domain-7, domain-18 |
| **多集群 / 联邦管理** | 仅 platform-ops 有一篇 | domain-9 |

---

## 三、整合方案建议

### 3.1 整合原则

1. **单一维度**：全部按「运维职能领域」分类，不再混用工具品牌/内容载体/生命周期等维度
2. **问题域优先**：以「什么出问题、怎么定位、怎么恢复」为首要组织逻辑
3. **消除重复**：同一知识点只存在于一个 Domain
4. **规模均衡**：目标每个 Domain 20-50 个文件

### 3.2 建议的 18 个核心 Domain

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────────────────┐
│                        生产环境 Domain 架构（建议版）                          │
├─────────────────────────────────────────────────────────────────────────────┤
│ TIER 1: 核心技术域（横向能力）                                                │
├─────────────────────────────────────────────────────────────────────────────┤
│ 01-cluster-fundamentals     ← 合并 architecture + design-principles          │
│                              + control-plane 核心内容                         │
│ 02-workloads-applications   ← workloads + java-k8s                          │
│ 03-networking-traffic       ← networking + network-fundamentals             │
│                              + service-mesh + api-gateway + ebpf(Cilium)    │
│ 04-storage-data            ← storage + storage-fundamentals                  │
│ 05-security-compliance     ← security + cloud-native-security                │
│ 06-observability           ← observability + monitoring-alerting + logging  │
├─────────────────────────────────────────────────────────────────────────────┤
│ TIER 2: 平台与工程域（平台能力）                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│ 07-platform-engineering    ← platform-ops + platform-engineering             │
│                             （分「平台构建」和「平台运维」两个子目录）          │
│ 08-release-change-management ← gitops-ci-cd + iac + 变更管理                 │
│ 09-reliability-engineering  ← SRE, SLO/SLI, 混沌工程, 容量规划, 灾备          │
│                              （从 production-operations 抽取）                │
├─────────────────────────────────────────────────────────────────────────────┤
│ TIER 3: 运维场景域（场景能力）                                                │
├─────────────────────────────────────────────────────────────────────────────┤
│ 10-troubleshooting-diagnostics ← troubleshooting + 全链路排障                │
│ 11-production-operations   ← 精简：事件响应、FinOps、成本优化、                 │
│                              多租户治理、绿色计算                              │
├─────────────────────────────────────────────────────────────────────────────┤
│ TIER 4: 部署与生态域（环境能力）                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│ 12-cloud-providers         ← cloud-provider + multi-cloud-hybrid             │
│ 13-container-runtime       ← docker + container-image-management             │
│                             + supply-chain-security                          │
│ 14-ai-ml-infra             ← ai-infra + ai-agent                             │
│ 15-specialized-tech        ← edge-computing + wasm + extensions              │
├─────────────────────────────────────────────────────────────────────────────┤
│ TIER 5: 基础与参考域（支撑能力）                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│ 90-system-foundation       ← linux + hardware + kubernetes-events            │
│ 91-manifests-patterns      ← yaml-manifests（作为参考手册，非知识域）          │
│ 92-landscape-references    ← cncf-landscape + papers                         │
│ 93-application-patterns    ← application-architecture（业务架构参考库）         │
└─────────────────────────────────────────────────────────────────────────────┘
```
### 3.3 关键整合动作

#### 动作 1：合并可观测性三域 → `06-observability`

```
observability/
├── 01-overview/
│   ├── architecture-overview.md        ← from domain-8
│   ├── three-pillars.md                ← from domain-8
│   └── enterprise-scale.md             ← from domain-8
├── 02-metrics/
│   ├── prometheus-deep-dive.md         ← from domain-20
│   ├── thanos-federation.md            ← from domain-20
│   ├── custom-metrics-adapter.md       ← from domain-8
│   └── cost-optimization.md            ← from domain-8
├── 03-logging/
│   ├── logging-architecture.md         ← from domain-8
│   ├── elk-stack.md                    ← from domain-21
│   ├── loki-aggregation.md             ← from domain-21
│   └── fluentd-processing.md           ← from domain-21
├── 04-tracing/
│   ├── distributed-tracing.md          ← from domain-8
│   └── opentelemetry.md                ← from domain-20
├── 05-alerting/
│   ├── alerting-management.md          ← from domain-8
│   └── on-call-playbooks.md            ← from domain-8
├── 06-slo-sli/
│   └── slo-sli-implementation.md       ← from domain-8
└── 07-tools/
    ├── grafana.md                      ← from domain-20
    ├── datadog.md                      ← from domain-20
    └── zabbix.md                       ← from domain-20
```

#### 动作 2：合并安全三域 → `05-security-compliance`

```
security-compliance/
├── 01-identity-access/
│   ├── authentication-authorization.md  ← from domain-7
│   ├── rbac-matrix.md                   ← from domain-7
│   └── vault-secrets.md                 ← from domain-25
├── 02-network-security/
│   ├── network-policies.md              ← from domain-7
│   ├── zero-trust.md                    ← from domain-7
│   └── network-defense.md               ← from domain-7
├── 03-runtime-security/
│   ├── falco-runtime-detection.md       ← from domain-25
│   ├── gvisor-sandbox.md                ← from domain-25
│   └── runtime-defense.md               ← from domain-7
├── 04-policy-governance/
│   ├── opa-gatekeeper.md                ← from domain-7/25
│   ├── kyverno-policies.md              ← from domain-7/25
│   └── pod-security-standards.md        ← from domain-7
├── 05-supply-chain/
│   ├── sbom-management.md               ← from domain-39
│   ├── slsa-implementation.md           ← from domain-39
│   ├── sigstore-cosign.md               ← from domain-39
│   └── image-scanning.md                ← from domain-7/25
├── 06-compliance/
│   ├── cis-benchmarks.md                ← from domain-7
│   ├── audit-logging.md                 ← from domain-7
│   └── certification.md                 ← from domain-7
└── 07-incident-response/
    └── security-incident-response.md    ← from domain-7
```

#### 动作 3：拆分 `domain-18-production-operations`

当前 34 个文件的大杂烩，按内容归属拆分到 5 个目标 Domain：

| 原文件 | 目标 Domain | 理由 |
|--------|------------|------|
| 04-enterprise-monitoring-system | 06-observability | 监控体系 |
| 05-logging-collection-analysis-platform | 06-observability | 日志体系 |
| 06-apm-application-performance-monitoring | 06-observability | APM |
| 07-zero-trust-security-architecture | 05-security-compliance | 安全架构 |
| 08-cis-benchmark-compliance-audit | 05-security-compliance | 合规 |
| 09-software-bill-of-materials | 05-security-compliance | 供应链 |
| 10-gitops-pipeline-practices | 08-release-change-management | 发布 |
| 11-infrastructure-as-code | 08-release-change-management | IaC |
| 13-kubernetes-cost-governance | 11-production-operations | FinOps |
| 14-resource-quota-management | 07-platform-engineering | 平台治理 |
| 16-enterprise-backup-strategy | 09-reliability-engineering | 备份 |
| 17-disaster-recovery-drills | 09-reliability-engineering | 灾备 |
| 18-cross-region-disaster-recovery | 09-reliability-engineering | 跨区灾备 |
| 19-22 performance tuning (cluster/network/storage) | 01-cluster-fundamentals | 性能调优 |
| 22-change-management-process | 08-release-change-management | 变更管理 |
| 23-incident-response-handling | 11-production-operations | 事件响应 |
| 24-capacity-planning-forecasting | 09-reliability-engineering | 容量规划 |

#### 动作 4：合并平台运维两域 → `07-platform-engineering`

```
platform-engineering/
├── build/                           # 平台构建（来自 domain-36）
│   ├── idp-design-principles.md
│   ├── backstage-deployment.md
│   ├── crossplane-composition.md
│   └── golden-paths.md
├── operate/                         # 平台运维（来自 domain-9）
│   ├── cluster-lifecycle.md
│   ├── multi-cluster-management.md
│   ├── platform-upgrade-migration.md
│   └── multi-tenant-management.md
├── governance/                      # 平台治理（来自 domain-9/18）
│   ├── capacity-planning.md
│   ├── resource-quota-management.md
│   └── cost-optimization-finops.md
└── developer-experience/            # DevEx（来自 domain-36）
    ├── developer-experience-metrics.md
    ├── platform-team-topology.md
    └── cli-plugin-ecosystem.md
```

---

## 四、生产环境收益分析

### 4.1 问题排查路径优化

**当前**（以 "Pod 无法访问 Service" 为例）：
```
故障诊断/ → 网络/ → 可观测性/
→ domain-26-service-mesh/ → domain-40-api-gateway/
= 跨 5 个 Domain，平均 8-12 次文件跳转
```

**整合后**：
```
故障诊断/（入口决策树）
  → 网络/（网络排查）
  → 可观测性/（指标验证）
= 跨 3 个 Domain，决策树引导直达
```

### 4.2 变更管理知识聚合

**当前**：变更管理知识分散在：
- `可靠性/22-change-management-process.md`
- `平台工程/07-gitops-configuration-management.md`
- `发布变更/` (15 个文件)
- `发布变更/` (9 个文件)

**整合后**：全部聚合到 `发布变更/`

### 4.3 新人 onboarding 路径

**当前**：43 个 Domain，没有明确的层级关系，新人不知道从哪里开始。

**整合后**：
```
Tier 1（必学）→ Tier 2（进阶）→ Tier 3（专家）
    ↓                ↓                ↓
  核心技术         平台工程         场景实战
```

---

## 五、实施建议

### 5.1 实施优先级

| 优先级 | 动作 | 工作量 | 收益 |
|--------|------|--------|------|
| P0 | 合并可观测性三域 → `06-observability` | 高 | 🔴 最高（排查最频繁） |
| P0 | 合并安全三域 → `05-security-compliance` | 高 | 🔴 最高（安全事件紧迫） |
| P1 | 拆分 `domain-18-production-operations` | 高 | 🟡 高（消除大杂烩） |
| P1 | 合并平台运维两域 | 中 | 🟡 高 |
| P2 | 合并网络相关四域 | 中 | 🟡 中 |
| P2 | 合并存储两域 | 低 | 🟢 中 |
| P3 | 迁移内容载体 Domain（yaml-manifests, events） | 低 | 🟢 低（结构调整） |
| P3 | 整合 application-architecture | 中 | 🟢 低（偏业务） |

### 5.2 迁移策略

推荐 **"渐进式迁移"** 而非一次性重构：

1. **Phase 1（2-4 周）**：创建新 Domain 目录结构，使用符号链接或 `MOVED_TO` 标记文件指向新位置
2. **Phase 2（4-8 周）**：逐 Domain 迁移内容，保留旧文件为只读 redirect，更新所有内部链接
3. **Phase 3（8-12 周）**：删除旧 Domain 目录，更新自动化脚本和索引
4. **Phase 4（持续）**：建立 Domain 归属审查机制，防止再次出现大杂烩

### 5.3 需要更新的文件

- `_meta/taxonomy.md`：更新 Domain 分类体系
- `_meta/dashboard.md`：更新 Dataview 查询范围
- `AGENTS.md`：更新 agent 的 Domain 路由逻辑
- `index.md`：重新构建主索引
- 所有 `00-open-source-projects-index.md`：需要合并或重建

---

## 六、风险与注意事项

| 风险 | 缓解措施 |
|------|---------|
| 外部链接失效（Wiki 内引用） | 迁移脚本批量替换 `domain-X/...` 链接 |
| 历史 commit 记录关联断裂 | 保留旧目录的 `README.md` 说明迁移历史 |
| 内容归属争议（某文件应归哪个 Domain） | 建立「主归属 + 交叉引用」原则 |
| 迁移期间贡献者冲突 | 迁移期间冻结相关 Domain 的写入 |

---

## 七、附录：当前 Domain 全量映射

| 原 Domain | 文件数 | 建议归属 | 处理方式 |
|-----------|--------|---------|---------|
| domain-1-architecture-fundamentals | 35 | 01-cluster-fundamentals | 合并 |
| domain-2-design-principles | 22 | 01-cluster-fundamentals | 合并 |
| domain-3-control-plane | 39 | 01-cluster-fundamentals | 合并 |
| domain-4-workloads | 30 | 02-workloads-applications | 保留主体 |
| domain-5-networking | 57 | 03-networking-traffic | 合并 |
| domain-6-storage | 21 | 04-storage-data | 合并 |
| domain-7-security | 24 | 05-security-compliance | 合并 |
| domain-8-observability | 35 | 06-observability | 合并 |
| domain-9-platform-ops | 31 | 07-platform-engineering | 合并 |
| domain-10-extensions | 22 | 15-specialized-tech | 迁移 |
| domain-11-ai-infra | 41 | 14-ai-ml-infra | 合并 |
| domain-12-troubleshooting | 50 | 10-troubleshooting-diagnostics | 保留主体 |
| domain-13-docker | 16 | 13-container-runtime | 合并 |
| domain-14-linux | 13 | 90-system-foundation | 迁移 |
| domain-15-network-fundamentals | 10 | 03-networking-traffic | 合并 |
| domain-16-storage-fundamentals | 9 | 04-storage-data | 合并 |
| domain-17-cloud-provider | 3 | 12-cloud-providers | 合并 |
| domain-18-production-operations | 34 | 多域拆分 | 拆分 |
| domain-19-papers | 29 | 92-landscape-references | 迁移 |
| domain-20-enterprise-monitoring-alerting | 15 | 06-observability | 合并 |
| domain-21-logging-management-analytics | 12 | 06-observability | 合并 |
| domain-22-container-image-management | 11 | 13-container-runtime | 合并 |
| domain-23-gitops-ci-cd | 15 | 08-release-change-management | 合并 |
| domain-24-infrastructure-as-code | 9 | 08-release-change-management | 合并 |
| domain-25-cloud-native-security | 18 | 05-security-compliance | 合并 |
| domain-26-service-mesh-microservices | 16 | 03-networking-traffic | 合并 |
| domain-27-multi-cloud-hybrid | 13 | 12-cloud-providers | 合并 |
| domain-28-enterprise-database-middleware | 12 | 02-workloads-applications | 迁移 |
| domain-29-automated-testing-quality | 8 | 08-release-change-management | 迁移 |
| domain-30-disaster-recovery-business-continuity | 12 | 09-reliability-engineering | 迁移 |
| domain-31-hardware | 21 | 90-system-foundation | 迁移 |
| domain-32-yaml-manifests | 39 | 91-manifests-patterns | 迁移 |
| domain-33-kubernetes-events | 18 | 90-system-foundation | 迁移 |
| domain-34-cncf-landscape | 7 | 92-landscape-references | 迁移 |
| domain-35-ebpf-technology | 13 | 03-networking-traffic | 合并 |
| domain-36-platform-engineering | 15 | 07-platform-engineering | 合并 |
| domain-37-edge-computing | 14 | 15-specialized-tech | 迁移 |
| domain-38-webassembly-cloud-native | 14 | 15-specialized-tech | 迁移 |
| domain-39-supply-chain-security | 14 | 05-security-compliance | 合并 |
| domain-40-cloud-native-api-gateway | 18 | 03-networking-traffic | 合并 |
| domain-41-ai-agent | 52 | 14-ai-ml-infra | 合并 |
| domain-42-application-architecture | 98 | 93-application-patterns | 迁移 |
| domain-43-java-kubernetes | 8 | 02-workloads-applications | 合并 |


<!-- risk-assessed -->
