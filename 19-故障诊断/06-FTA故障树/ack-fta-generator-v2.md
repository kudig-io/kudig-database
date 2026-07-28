---
title: ACK-FTA 生成器增强版提示词
description: '## 0. 核心设计理念'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- etcd
- scheduler
- prometheus
- jaeger
- istio
- envoy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- ACK-FTA 生成器增强版提示词 是什么
- 如何 ACK-FTA 生成器增强版提示词
- ACK-FTA 生成器增强版提示词 根因分析
- ACK-FTA 生成器增强版提示词 故障树
trigger_keywords:
- ACK-FTA
- 生成器增强版提示词
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- etcd-basics
- tls-basics
- tracing-basics
- observability-basics
tier: supporting
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# ACK-FTA 生成器增强版提示词

> **版本**: v2.0 Enhanced
> **适用场景**: 基于 ACK（Alibaba Cloud Kubernetes）源码生成故障树分析
> **更新日期**: 2026-05-18

---

## 0. 核心设计理念

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ACK-FTA 生成器核心设计理念                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  三大方法论融合:                                                             │
│                                                                             │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐      │
│  │ FTA (演绎法)     │     │ FEBM (归纳法)    │     │ 自动化生成       │      │
│  │ 故障树分析       │ ──► │ 法医鉴定循证     │ ──► │ 源码/告警驱动    │      │
│  │ 自上而下假设验证 │     │ 自下而上证据推理 │     │ 智能化推导       │      │
│  └────────┬────────┘     └────────┬────────┘     └────────┬────────┘      │
│           │                     │                     │                  │
│           └─────────────────────┴─────────────────────┘                  │
│                               ↓                                           │
│                    ┌─────────────────────┐                                │
│                    │  ACK-FTA 知识体系    │                                │
│                    │  演绎 + 归纳 + 自动化 │                                │
│                    └─────────────────────┘                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 1. 输入 Schema（强制）

```yaml
input:
  # 必填字段
  component:
    type: string
    description: "ACK 组件名称，如 'ack-scheduler', 'terway', 'ack-api-server'"
    examples: ["ack-api-server", "terway-eni", "asm-istio", "arms-prometheus"]

  scope:
    type: enum
    description: "故障分析范围"
    enum:
      - control-plane          # 控制平面组件
      - worker-node            # 工作节点组件
      - network                # 网络组件（含 Terway/CNI）
      - storage                # 存储组件（含 OSS/CSI/NAS）
      - security               # 安全组件（含 RAM/证书/PSP）
      - ack-native             # ACK 特有组件（ASM/MSP/ARMS/ACK-One）
      - iaas-dependency        # 阿里云底层依赖（ECS/ENI/ESSD/SLB）

  severity:
    type: enum
    description: "问题严重程度"
    enum: [P0, P1, P2, P3]

  # 可选字段
  ack_version:
    type: string
    default: "v1.28"
    description: "ACK/Kubernetes 版本"

  constraints:
    max_depth:
      type: integer
      default: 5
      description: "故障树最大递归深度"
    include_cloud_provider:
      type: boolean
      default: true
      description: "是否包含阿里云底层 IaaS 依赖"
    include_arms:
      type: boolean
      default: true
      description: "是否包含 ARMS 应用实时监控"
    include_asm:
      type: boolean
      default: true
      description: "是否包含 ASM 服务网格"
    include_terway:
      type: boolean
      default: true
      description: "是否包含 Terway 网络插件"

  source_context:
    type: enum
    description: "输入来源"
    enum:
      - source_code      # 直接从源码分析
      - alert_rule       # 从告警规则反推
      - incident_log     # 从问题日志分析
      - architecture_doc # 从架构文档生成

  # 源码分析专用（当 source_context=source_code 时）
  source_details:
    repo_url: string           # 源码仓库地址（内网）
    entry_points: [string]     # 关键入口函数
    error_handling_patterns: [string]  # 错误处理模式

  # 告警规则专用（当 source_context=alert_rule 时）
  alert_details:
    alert_name: string         # 告警名称
    alert_expression: string   # PromQL/时序查询表达式
    alert_labels: object       # 标签键值对

  # 问题日志专用（当 source_context=incident_log 时）
  incident_details:
    ticket_id: string          # 工单编号
    symptoms: [string]        # 观察到的症状
    time_window: object        # 时间窗口
    affected_services: [string]  # 影响的服务列表

  # 架构文档专用（当 source_context=architecture_doc 时）
  architecture_details:
    component_diagram: string   # 组件架构图（文本描述）
    dependencies: [string]     # 依赖关系列表
    sla_objectives: object      # SLO 目标
```

---

## 2. 输出 Schema（强制）

所有 FTA 生成**必须**输出以下 Schema：

```yaml
output:
  schema_version: "2.0"

  # 顶事件定义（必须）
  top_event:
    id: string                  # TE-{编号}
    name: string                # 事件名称
    description: string          # 一句话描述
    severity: enum[P0,P1,P2,P3]  # 严重程度
    slo_impact: string          # SLO 影响描述
    affected_scope: string      # 影响范围

  # 完整故障树（必须）
  fault_tree:
    nodes:                     # 所有节点
      - id: string
        type: enum[TE, IE, BE, UE, HE]  # 顶/中间/底/未展开/外部事件
        name: string
        gate_type: enum[OR, AND, K/N, INHIBIT, PAND, XOR]  # 逻辑门类型
        parent_id: string      # 父节点 ID（TE/IE 无父节点）
        children_ids: [string] # 子节点 ID 列表
        level: integer         # 层级（1=顶事件）
        probability: float     # 概率（仅 BE 必须）
        mttr_minutes: integer  # 平均修复时间
    edges:                     # 边关系
      - from: string
        to: string
        gate_type: enum       # 逻辑门类型
        condition: string     # 条件描述（如 "n/3"）

  # 底事件详情（必须）
  bottom_events:
    - id: string               # BE-{编号}
      name: string
      description: string

      # 可观测性（必须）
      observable:
        metrics:
          - expression: string  # PromQL 表达式
            threshold: string   # 阈值条件
            source: string     # 数据源（prometheus/arms/arms-java）
        logs:
          - pattern: string    # 日志正则/关键字
            source: string     # 日志来源（stdout/kern/dmesg）
        events:
          - filter: string     # kubectl get events 过滤条件
        traces:
          - type: enum[jaeger/arms]  # 链路追踪类型
            pattern: string    # 追踪模式

      # 根因列表（必须）
      root_causes:
        - cause: string
          probability: float
          evidence_type: enum[metric/log/event/trace]

      # 诊断命令（必须，至少 3 条）
      diagnosis_commands:
        - command: string
          expected_result: string
          timeout_seconds: integer

      # 修复动作（必须）
      healing_actions:
        - id: string            # HA-{BE编号}.{序号}
          description: string
          risk_level: enum[low, medium, high, critical]
          auto_healable: boolean
          requires_approval: boolean
          command: string       # 可执行命令或 Playbook 引用
          rollback_command: string
          success_rate: float   # 历史成功率

      # 概率数据（必须）
      probability:
        annual_rate: float      # 年问题率
        monthly_rate: float     # 月问题率
        mtbf_hours: float       # 平均问题间隔
        mttr_minutes: integer   # 平均修复时间
        auto_heal_rate: float   # 自动修复成功率
        detection_rate: float   # 可检测率

      # ACK 特有扩展
      ack_specific:
        cloud_provider: "Alibaba Cloud"  # 固定值
        related_aliyun_services: [string]  # 相关阿里云服务
        ecs_instance_types: [string]      # 涉及 ECS 实例类型
        eni_attachments: boolean          # 是否涉及弹性网卡
        essd_performance_tier: enum[PL0/PL1/PL2/PL3]  # ESSD 性能等级

  # 概率矩阵（必须）
  probability_matrix:
    events: [string]            # 事件 ID 列表
    matrix: float           # 概率矩阵（二维数组）
    min_cut_sets:               # 最小割集
      - cut_set: [string]       # 割集事件列表
        probability: float      # 该割集概率
        importance: float       # 重要度

  # Mermaid 图形（必须）
  mermaid_diagram:
    diagram: string            # Mermaid 代码
    theme: enum[default/dark]  # 主题

  # ACK 特有问题路径（可选）
  ack_specific_paths:
    - path_id: string
      description: string
      involves_aliyun_services: [string]
      mitigation: string

  # FTA 完整度评分（必须）
  confidence_score:
    overall: float              # 0.0-1.0
    coverage: float            # 问题覆盖率
    observability: float       # 可观测性评分
    automation: float          # 自动化程度
    completeness: float        # 完整性评分
    missing_items: [string]    # 缺失项列表
    recommended_actions: [string]  # 改进建议

  # FTA-FEBM 融合数据（可选）
  febm_integration:
    related_febm_cases: [string]   # 关联的 FEBM 案例
    evidence_patterns: [string]    # 证据模式
    forensic_timeline: string       # 取证时间线描述

  # 元数据
  metadata:
    generated_at: string        # 生成时间
    generator_version: string   # 生成器版本
    source_context: enum       # 输入来源
    language: "zh-CN"          # 输出语言
```

---

## 3. ACK 特有组件覆盖要求

### 3.1 阿里云底层 IaaS 依赖层

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ACK IaaS 依赖层故障域                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ECS 实例问题                                                              │
│  ├── BE-IaaS-1: ECS 实例被驱逐 (节点压力驱逐/竞价实例中断)                    │
│  ├── BE-IaaS-2: ECS 实例网络分区 (ENI 链路中断)                              │
│  ├── BE-IaaS-3: ECS 实例硬件问题 (内存/CPU/磁盘)                            │
│  └── BE-IaaS-4: ECS 实例计划维护 (阿里云系统事件)                            │
│                                                                             │
│  弹性网卡 (ENI) 问题                                                        │
│  ├── BE-IaaS-5: ENI 多队列压力 (高并发网络流量)                             │
│  ├── BE-IaaS-6: ENI 绑定数超限 (安全组/SG限制)                               │
│  └── BE-IaaS-7: ENI 带宽限制 (实例规格瓶颈)                                  │
│                                                                             │
│  云盘 (ESSD/Cloud盘) 问题                                                   │
│  ├── BE-IaaS-8: ESSD 性能降级 (PL3→PL1 自动降级)                            │
│  ├── BE-IaaS-9: 云盘 IOPS 抖动 (共享带宽冲突)                               │
│  ├── BE-IaaS-10: 云盘容量不足 (磁盘满)                                       │
│  └── BE-IaaS-11: 云盘延迟突增 (阿里云底层存储问题)                            │
│                                                                             │
│  SLB (Server Load Balancer) 问题                                            │
│  ├── BE-IaaS-12: SLB 配置异常 (后端权重/健康检查)                           │
│  ├── BE-IaaS-13: SLB 连接数超限 (七层监听器限制)                             │
│  ├── BE-IaaS-14: SLB SSL 证书问题 (阿里云证书服务)                            │
│  └── BE-IaaS-15: SLB DDoS 触发 (流量清洗阈值)                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 ACK 管控面组件

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ACK 管控面故障域                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  阿里云托管 Kubernetes 管控节点                                              │
│  ├── BE-ACM-1: 管控节点资源不足 (API Server 调度受限)                         │
│  ├── BE-ACM-2: 管控节点网络抖动 (阿里云内网质量问题)                         │
│  └── BE-ACM-3: 管控节点计划维护 (阿里云升级事件)                              │
│                                                                             │
│  资源配额与限制                                                              │
│  ├── BE-ACM-4: 集群资源配额超限 (CPU/Memory/Node 限制)                       │
│  ├── BE-ACM-5: ACK 资源组配额耗尽 (企业级资源管理)                           │
│  └── BE-ACM-6: API 对象数量限制 (Pods/Services 数量)                         │
│                                                                             │
│  升级与维护事件                                                              │
│  ├── BE-ACM-7: 集群版本升级中 (控制平面短暂不可用)                           │
│  ├── BE-ACM-8: 组件升级回滚 (如 cert-manager 升级失败)                       │
│  └── BE-ACM-9: 阿里云平台迁移 (可用区切换)                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.3 Terway 网络插件

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Terway 网络插件故障域                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Terway ENI 模式                                                            │
│  ├── BE-TW-1: ENI 多队列压力 (Pod 密度过高)                                 │
│  ├── BE-TW-2: ENI 带宽限制 (VSwitch 带宽瓶颈)                               │
│  ├── BE-TW-3: Pod IP 耗尽 (VPC CIDR 不足)                                   │
│  └── BE-TW-4: ENI 安全组冲突 (安全组规则覆盖)                                │
│                                                                             │
│  Terway IPVLAN 模式                                                         │
│  ├── BE-TW-5: IPVLAN 网络策略不生效 (内核版本不兼容)                          │
│  ├── BE-TW-6: IPVLAN 连接泄漏 (netlink 资源耗尽)                            │
│  └── BE-TW-7: IPVLAN MTU 问题 (巨型帧分片)                                  │
│                                                                             │
│  Terway BGP 模式                                                            │
│  ├── BE-TW-8: BGP 会话中断 (动态路由失效)                                    │
│  ├── BE-TW-9: BGP 路由黑洞 (路由优先级冲突)                                  │
│  └── BE-TW-10: BGP AS 号冲突 (多集群场景)                                    │
│                                                                             │
│  Service/Traffic 异常                                                       │
│  ├── BE-TW-11: kube-proxy 状态异常 (Terway vs kube-proxy)                  │
│  ├── BE-TW-12: NodePort 端口冲突 (多组件同时占用)                            │
│  └── BE-TW-13: LoadBalancer 注解失效 (Terway CRD 配置)                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.4 ASM (Alibaba Cloud Service Mesh)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ASM 服务网格故障域                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  controlplane                                                              │
│  ├── BE-ASM-1: Istio 控制面组件问题 (Citadel/ Galley/ Pilot)                 │
│  ├── BE-ASM-2: xDS 资源配置错误 (VirtualService/ DestinationRule)           │
│  └── BE-ASM-3: mTLS 证书过期/撤销 (双向认证失效)                             │
│                                                                             │
│  data_plane                                                                │
│  ├── BE-ASM-4: Envoy sidecar 资源耗尽 (内存/CPU)                            │
│  ├── BE-ASM-5: Envoy 健康检查失败 (主动/被动探测)                            │
│  └── BE-ASM-6: Envoy 连接池耗尽 (熔断器触发)                                 │
│                                                                             │
│  traffic_management                                                        │
│  ├── BE-ASM-7: 灰度发布异常 (VirtualService 权重配置)                       │
│  ├── BE-ASM-8: 熔断规则触发 (CircuitBreaker)                              │
│  ├── BE-ASM-9: 限流规则冲突 (RateLimit 资源不足)                           │
│  └── BE-ASM-10: 流量镜像异常 (TrafficMirror 配置错误)                       │
│                                                                             │
│  可观测性                                                                   │
│  ├── BE-ASM-11: Envoy 指标丢失 (Prometheus 采集问题)                       │
│  └── BE-ASM-12: 追踪链路断裂 (Jaeger/ARMSTrace 采样率)                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.5 ARMS (Application Real-Time Monitoring Service)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ARMS 应用实时监控故障域                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  指标采集                                                                   │
│  ├── BE-ARMS-1: ACK 托管 Prometheus 不可用                                  │
│  ├── BE-ARMS-2: ARMS Java Agent 注入失败 (字节码增强)                       │
│  ├── BE-ARMS-3: ARMS SDK 数据上报失败 (网络隔离)                             │
│  └── BE-ARMS-4: ServiceMonitor 配置错误 (抓取目标丢失)                      │
│                                                                             │
│  应用性能监控                                                               │
│  ├── BE-ARMS-5: 慢调用检测误报 (采样率问题)                                 │
│  ├── BE-ARMS-6: 异常堆栈采集不完整 (OOM 场景)                               │
│  └── BE-ARMS-7: 调用链采样偏差 (高流量场景)                                  │
│                                                                             │
│  前端监控                                                                   │
│  ├── BE-ARMS-8: Browser SDK 数据丢失 (CDN 缓存)                             │
│  └── BE-ARMS-9: 真实用户监控 (RUM) 配置错误                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.6 ACK-One 多集群/混合云

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ACK-One 多集群故障域                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  多集群管理                                                                 │
│  ├── BE-AO-1: 集群注册异常 (注册代理通信失败)                                │
│  ├── BE-AO-2: 集群状态同步延迟 (etcd 延迟/网络抖动)                         │
│  └── BE-AO-3: 跨集群服务发现失败 (Federation DNS 解析)                       │
│                                                                             │
│  分布式运维                                                                 │
│  ├── BE-AO-4: 配置同步不一致 (GitOps 传播延迟)                              │
│  ├── BE-AO-5: 统一监控数据丢失 (中心集群采集失败)                             │
│  └── BE-AO-6: 统一日志聚合失败 (Logstores 写入失败)                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. FTA-FEBM 双向融合机制

### 4.1 融合架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FTA-FEBM 双向融合架构                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   FTA (演绎法)                        FEBM (归纳法)                          │
│   "系统可能在哪里出问题？"            "系统实际发生了什么？"                   │
│   ┌─────────────┐                    ┌─────────────┐                        │
│   │  故障树      │◄────── 映射 ──────►│  证据链      │                      │
│   │  预设路径    │                    │  实时案例    │                      │
│   └──────┬──────┘                    └──────┬──────┘                        │
│          │                                 │                               │
│          │    ┌────────────────────────────┐ │                              │
│          └───►│     融合知识库             │◄┘                              │
│               │                             │                               │
│               │  FTA 路径 ←→ FEBM 案例      │                               │
│               │  概率更新 ←→ 证据验证        │                               │
│               │  新问题发现 ←→ 根因确认      │                               │
│               └────────────────────────────┘                               │
│                                    │                                        │
│                                    ▼                                        │
│                         ┌─────────────────┐                                  │
│                         │ 智能推理引擎    │                                  │
│                         │ FTA+FEBM 联合   │                                  │
│                         │ 诊断            │                                  │
│                         └─────────────────┘                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 融合映射规则

```yaml
fta_febm_mapping:

  # FTA 节点 → FEBM 证据模式
  node_to_evidence:
    BE-2.3:
      febm_pattern: "OOMKilled"
      evidence_types:
        - "container_memory_usage_bytes > limit"
        - "Exit Code: 137"
        - "dmesg | grep -i oom"
      forensic_commands:
        - "kubectl exec <pod> -- cat /proc/<pid>/status"
        - "dmesg -T | grep -E 'oom|kill'"

    BE-1.2:
      febm_pattern: "etcd集群问题"
      evidence_types:
        - "etcd_server_has_leader == 0"
        - "etcd_mvcc_db_total_size_in_bytes / quota > 0.8"
        - "connection refused to 2379"
      forensic_commands:
        - "kubectl exec -n kube-system etcd-<node> -- etcdctl endpoint status"
        - "kubectl exec -n kube-system etcd-<node> -- etcdctl endpoint health"

  # FEBM 案例 → FTA 路径扩展
  case_to_fta_expansion:
    FEBM-case-INC-2026-0215:
      root_cause: "Java heap space 泄漏"
      fta_expansion:
        - BE-2.3 + "根因: OrderCache.loadAll 内存泄漏"
        - new_root_cause: "应用代码级内存管理"
        - recommended_fix: "优化 JVM 堆内存配置 + 代码修复"

  # FTA 概率更新 ← FEBM 证据验证
  probability_update:
    trigger:
      - event: "同一 BE 路径在多个 FEBM 案例中出现"
      - event: "同一 BE 路径的证据链完整度 > 90%"
      - event: "修复动作成功率与 FTA 预估偏差 > 20%"
    actions:
      - "更新 BE 的 probability.annual_rate"
      - "更新 BE 的 probability.auto_heal_rate"
      - "补充新的 root_causes 条目"
      - "标记为 confirmed_path（已验证路径）"

  # FTA 新路径发现 ← FEBM 推断
  new_path_detection:
    trigger:
      - event: "FEBM 案例的根因不在现有 FTA 中"
      - event: "FEBM 证据链指向新的问题传播路径"
    actions:
      - "生成 PROPOSED（待评审）状态的新 FTA 路径"
      - "通知 FTA 维护团队进行评审"
      - "在 FTA 中标记为 '需人工确认'"
```

### 4.3 联合诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FTA+FEBM 联合诊断时序                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  T+0s:   告警触发                                                           │
│          ↓                                                                  │
│  T+1s:   FTA 路径匹配 (快速定位假设)                                        │
│          → 候选路径: TE-2 → IE-2.1 → BE-2.3 (OOMKilled)                     │
│          ↓                                                                  │
│  T+2s:   FEBM 证据收集 (验证假设)                                          │
│          → 采集: container_memory_usage_bytes, Exit Code, dmesg           │
│          ↓                                                                  │
│  T+5s:   联合推理                                                           │
│          → FTA 假设: 内存 limit 设置过低                                    │
│          → FEBM 证据: JVM heap 1.2GB / limit 1Gi (120%)                    │
│          → 匹配度: 95% ✅                                                   │
│          ↓                                                                  │
│  T+6s:   根因确认                                                           │
│          → FTA: BE-2.3 根因列表包含 "资源 limits 设置过低"                   │
│          → FEBM: 确认 OrderCache.loadAll 内存泄漏                           │
│          → 融合结论: 应用内存泄漏 + limits 配置不当                          │
│          ↓                                                                  │
│  T+10s:  修复动作执行                                                       │
│          → HA-2.3.1: 增加内存 limit (自动)                                  │
│          → 通知开发团队修复 OrderCache (人工)                                │
│          ↓                                                                  │
│  T+30s:  验证与反馈                                                         │
│          → FEBM 记录完整证据链                                              │
│          → FTA 更新概率数据 (本月第 4 次 → 5%)                              │
│          → 如果发现新根因 → 触发 FTA 路径扩展                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. 自动化 FTA 生成能力

### 5.1 从源码自动生成 FTA

```python
class ACKFTAGenerator:
    """基于 ACK 源码的自动化 FTA 生成器"""

    def generate_from_source_code(self, repo_url, component):
        """从源码分析生成 FTA"""

        # 1. 解析错误处理路径
        error_paths = self.extract_error_handling_paths(repo_url, component)

        # 2. 识别故障模式
        fault_modes = self.identify_fault_modes(error_paths)

        # 3. 推断 FTA 结构
        fta_structure = self.infer_fta_structure(fault_modes)

        # 4. 生成底事件定义
        bottom_events = self.generate_bottom_events(fta_structure)

        # 5. 映射到 ACK 故障域
        ack_mapped = self.map_to_ack_fault_domains(bottom_events)

        return self.build_complete_fta(ack_mapped)

    def extract_error_handling_paths(self, repo_url, component):
        """提取错误处理路径"""
        patterns = [
            # Kubernetes 错误
            "err != nil",
            "apierrors.IsNotFound(err)",
            "k8s.io/apimachinery/pkg/api/errors",
            # ACK 特有错误
            "aliyungo/openapi",
            "rosaclient",
            "slbclient",
        ]
        return []

    def identify_fault_modes(self, error_paths):
        """识别故障模式"""
        fault_modes = {
            "network_timeout": ["context.DeadlineExceeded", "i/o timeout"],
            "auth_failure": ["Unauthorized", "Forbidden", "token expired"],
            "quota_exceeded": ["QuotaExceeded", "LimitExceeded", "too many"],
            "resource_not_found": ["NotFound", "does not exist", "404"],
            "dependency_failure": ["connection refused", "dial tcp", "unavailable"],
        }
        return fault_modes

    def map_to_ack_fault_domains(self, fault_modes):
        """映射到 ACK 故障域"""
        ack_mapping = {
            "ECS 实例问题": ["BE-IaaS-1", "BE-IaaS-2", "BE-IaaS-3", "BE-IaaS-4"],
            "弹性网卡问题": ["BE-IaaS-5", "BE-IaaS-6", "BE-IaaS-7"],
            "云盘问题": ["BE-IaaS-8", "BE-IaaS-9", "BE-IaaS-10", "BE-IaaS-11"],
            "SLB 问题": ["BE-IaaS-12", "BE-IaaS-13", "BE-IaaS-14", "BE-IaaS-15"],
        }
        return ack_mapping
```

### 5.2 从告警规则反推 FTA

```python
class AlertToFTAGenerator:
    """从告警规则自动推导 FTA 路径"""

    def generate_from_alert(self, alert_name, alert_expression, labels):
        """从告警规则生成 FTA"""

        # 1. 解析告警表达式
        parsed = self.parse_alert_expression(alert_expression)

        # 2. 映射到顶事件
        top_event = self.map_to_top_event(parsed, labels)

        # 3. 自动展开中间事件
        intermediate_events = self.expand_intermediate_events(top_event)

        # 4. 生成底事件
        bottom_events = self.generate_bottom_events_from_alert(parsed)

        return self.build_fta_path(top_event, intermediate_events, bottom_events)

    def parse_alert_expression(self, expr):
        """解析 PromQL 表达式"""
        # 提取指标名、条件、操作符
        return {
            "metric": "kube_pod_status_phase",
            "condition": "==",
            "value": "Failed",
            "labels": ["namespace", "pod", "phase"]
        }

    def map_to_top_event(self, parsed, labels):
        """映射到顶事件"""
        if parsed["metric"].startswith("kube_pod"):
            return "TE-3: Pod启动失败"
        elif parsed["metric"].startswith("kube_node"):
            return "TE-1: 集群完全不可用"
        return "TE-2: 应用服务不可用"
```

### 5.3 从问题日志生成 FTA

```python
class IncidentToFTAGenerator:
    """从问题日志生成 FTA 扩展"""

    def generate_from_incident(self, ticket_id, symptoms, time_window):
        """从问题工单生成 FTA 路径扩展"""

        # 1. 收集证据
        evidence = self.collect_evidence(ticket_id, time_window)

        # 2. 重建时间线
        timeline = self.reconstruct_timeline(evidence)

        # 3. 因果推理
        causal_chain = self.infer_causality(timeline)

        # 4. 与现有 FTA 匹配
        existing_path = self.match_to_existing_fta(causal_chain)

        if existing_path:
            # 更新现有路径的概率和证据
            return self.update_existing_fta(existing_path, causal_chain)
        else:
            # 生成新的 FTA 路径（待评审）
            return self.propose_new_fta_path(causal_chain)
```

---

## 6. 可执行格式转换

### 6.1 FTA → 可执行命令

```yaml
# 从底事件自动生成可执行命令模板
executable_conversion:

  bottom_event_to_script:
    BE-2.3:  # OOMKilled
      diagnosis_script: |
        #!/bin/bash
        # 自动诊断 OOMKilled
        NAMESPACE=$1
        POD=$2

        echo "=== OOMKilled 诊断脚本 ==="

        # 1. 检查 Pod 状态
        kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.status.phase}'

        # 2. 检查最后状态
        kubectl describe pod $POD -n $NAMESPACE | grep -A5 "Last State"

        # 3. 检查资源限制
        kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.spec.containers[0].resources}'

        # 4. 检查实际内存使用
        kubectl top pod $POD -n $NAMESPACE --containers

        # 5. 检查 OOM 日志
        kubectl logs $POD -n $NAMESPACE --previous | grep -i "outofmemory" || echo "无 OOM 日志"

      healing_script: |
        #!/bin/bash
        # 自动修复 OOMKilled - 增加内存 limit
        NAMESPACE=$1
        DEPLOYMENT=$2
        CONTAINER=$3
        NEW_MEMORY_LIMIT=$4  # 如 "2Gi"

        kubectl patch deployment $DEPLOYMENT -n $NAMESPACE -p \
          "{\"spec\":{\"template\":{\"spec\":{\"containers\":[{\"name\":\"$CONTAINER\",\"resources\":{\"limits\":{\"memory\":\"$NEW_MEMORY_LIMIT\"}}}]}}}}"

        # 验证
        sleep 30
        kubectl rollout status deployment/$DEPLOYMENT -n $NAMESPACE
```

### 6.2 FTA → Kubernetes Operator/Controller

```yaml
# FTA 驱动的自动修复 Operator
apiVersion: v1
kind: ConfigMap
metadata:
  name: fta-healing-policies
  namespace: kube-system
data:
  policy-BE-2.3.yaml: |
    # OOMKilled 自动修复策略
    fault_id: BE-2.3
    name: OOMKilled 自动修复
    triggers:
      - alert: KubePodOOMKilled
      - metric: container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.95
    diagnosis:
      commands:
        - "kubectl describe pod {pod} | grep -A5 'Last State'"
        - "kubectl top pod {pod} --containers"
    healing_actions:
      - priority: 1
        action: increase_memory_limit
        risk: low
        auto_execute: true
        parameters:
          increment: "100%"
          max_limit: "4Gi"
      - priority: 2
        action: restart_pod
        risk: medium
        auto_execute: false
        requires_approval: true
    verification:
      - command: "kubectl get pod {pod} -o jsonpath='{.status.phase}'"
        expected: "Running"
      - command: "kubectl top pod {pod}"
        expected: "memory < limit * 0.8"
```

### 6.3 FTA → Ansible Playbook

```yaml
# FTA 驱动的 Ansible Playbook 模板
- name: FTA 驱动的问题修复 - etcd 磁盘空间
  hosts: etcd_master_nodes
  gather_facts: yes
  vars:
    fta_event: BE-1.2
    threshold: 0.8

  tasks:
    - name: "检查 etcd 磁盘使用率"
      command: |
        kubectl exec -n kube-system etcd-{{ inventory_hostname }} -- \
          etcdctl endpoint status --cluster -w table
      register: etcd_status

    - name: "计算磁盘使用率"
      set_fact:
        disk_usage_ratio: "{{ etcd_status.stdout | regex_search('(\d+\.\d+)%$') }}"

    - name: "执行碎片整理 (当使用率 > {{ threshold }})"
      when: disk_usage_ratio | float > threshold | float
      block:
        - name: "执行 defrag"
          command: |
            kubectl exec -n kube-system etcd-{{ inventory_hostname }} -- \
              etcdctl defrag --cluster
          register: defrag_result

        - name: "解除告警"
          command: |
            kubectl exec -n kube-system etcd-{{ inventory_hostname }} -- \
              etcdctl alarm disarm
      rescue:
        - name: "回滚: 恢复 etcd 快照"
          command: |
            kubectl exec -n kube-system etcd-{{ inventory_hostname }} -- \
              etcdctl snapshot restore /backup/etcd-latest.snap
```

### 6.4 FTA → OpenTelemetry 指标映射

```yaml
# FTA 底事件 → OpenTelemetry 指标定义
otel_mapping:
  BE-2.3:  # OOMKilled
    metrics:
      - name: k8s.container.memory_usage_ratio
        type: gauge
        description: "容器内存使用率 / limits"
        formula: "container_memory_usage_bytes / container_spec_memory_limit_bytes"
        labels:
          - namespace
          - pod
          - container
        alert_threshold: 0.95

      - name: k8s.container.oom_events_total
        type: counter
        description: "OOM 事件计数"
        labels:
          - namespace
          - pod
          - container
          - oom_reason

    traces:
      - name: k8s.pod.oom.reconstruction
        description: "OOM 事件追踪重建"
        spans:
          - container_start
          - memory_growth
          - oom_trigger
          - container_restart

  BE-1.2:  # etcd 集群问题
    metrics:
      - name: etcd.db.size.in.bytes
        type: gauge
        description: "etcd 数据库大小（字节）"

      - name: etcd.quota.usage.ratio
        type: gauge
        description: "etcd 配额使用率"
        formula: "etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes"
        alert_threshold: 0.8

      - name: etcd.has.leader
        type: gauge
        description: "etcd 是否有 leader"
        values: [0, 1]
        alert_threshold: 0
```

---

## 7. 质量保证清单

### 7.1 生成前检查

```yaml
pre_generation_checks:
  required_fields:
    - component          # 组件名称
    - scope             # 故障域
    - severity          # 严重程度
    - source_context    # 输入来源

  input_validation:
    component_valid: "ACK 支持的组件列表中"
    scope_valid: "6+1 个故障域之一"
    severity_valid: "P0/P1/P2/P3"
    source_context_valid: "4 种输入类型之一"

  ack_specific_checks:
    if_scope_is_iaas_dependency:
      - validate_cloud_provider: "Alibaba Cloud"
      - validate_related_services: ["ECS", "SLB", "VPC", "ESSD"]
    if_include_terway:
      - validate_version: "Terway >= 1.0"
      - validate_network_mode: ["ENI", "IPVLAN", "BGP"]
    if_include_asm:
      - validate_mesh_version: "ASM >= 1.9"
```

### 7.2 生成后检查

```yaml
post_generation_checks:

  structural_checks:
    - name: "顶事件定义完整性"
      check: "top_event 包含 id/name/description/severity/slo_impact"

    - name: "故障树节点连通性"
      check: "所有节点都有父节点（TE 除外），所有节点都有子节点或为 BE"

    - name: "逻辑门类型正确性"
      check: |
        - OR 门: 至少 2 个子节点
        - AND 门: 至少 2 个子节点
        - K/N 门: k <= n，且 k >= 1, n >= k

    - name: "底事件独立性"
      check: "任意两个 BE 不存在因果依赖关系"

    - name: "层级深度限制"
      check: "最大深度 <= constraints.max_depth (默认 5)"

  content_checks:
    - name: "底事件可观测性"
      check: "每个 BE 包含至少 1 个 observable.metrics/logs/events"

    - name: "底事件诊断命令"
      check: "每个 BE 包含至少 3 条 diagnosis_commands"

    - name: "底事件修复动作"
      check: "每个 BE 包含至少 1 个 healing_actions，且有 risk_level 和 auto_healable"

    - name: "概率数据完整性"
      check: "每个 BE 包含 annual_rate/mttr_minutes/auto_heal_rate"

  ack_specific_checks:
    - name: "阿里云服务关联"
      check: "涉及阿里云服务的 BE 必须包含 related_aliyun_services"

    - name: "Terway 网络配置"
      check: "Terway 相关 BE 必须包含 network_mode 和相关的诊断命令"

  format_checks:
    - name: "JSON Schema 验证"
      check: "输出符合 output.schema_version: '2.0' 定义"

    - name: "Mermaid 语法正确性"
      check: "mermaid_diagram.diagram 可被 Mermaid 渲染"

    - name: "编号连续性"
      check: "TE/IE/BE/HA 编号连续，无跳跃"

  confidence_score_threshold:
    overall_minimum: 0.70
    coverage_minimum: 0.75
    observability_minimum: 0.80
    automation_minimum: 0.60

  quality_gates:
    - gate: "通过所有 structural_checks"
      action: "如果失败，生成错误并退出"
    - gate: "通过所有 content_checks"
      action: "警告并记录 missing_items"
    - gate: "confidence_score >= 0.70"
      action: "如果失败，降低 quality_level 并通知"
```

---

## 8. 输出格式模板

### 8.1 完整输出示例

```yaml
# ACK-FTA 生成器输出示例
schema_version: "2.0"

top_event:
  id: "TE-2"
  name: "应用服务不可用"
  description: "用户无法访问应用，HTTP 5xx 错误率 > 5%"
  severity: "P0"
  slo_impact: "服务可用性 < 99.5%"
  affected_scope: "全地域用户"

fault_tree:
  nodes:
    - id: "TE-2"
      type: "TE"
      name: "应用服务不可用"
      level: 1

    - id: "IE-2.1"
      type: "IE"
      name: "Pod运行异常"
      gate_type: "OR"
      parent_id: "TE-2"
      children_ids: ["BE-2.1", "BE-2.2", "BE-2.3", "BE-2.4"]
      level: 2

    - id: "BE-2.3"
      type: "BE"
      name: "OOMKilled"
      parent_id: "IE-2.1"
      gate_type: null
      children_ids: []
      level: 3
      probability: 0.05
      mttr_minutes: 15

  edges:
    - from: "TE-2"
      to: "IE-2.1"
      gate_type: "OR"
    - from: "IE-2.1"
      to: "BE-2.3"
      gate_type: "OR"

bottom_events:
  - id: "BE-2.3"
    name: "OOMKilled"
    description: "容器因内存使用超过 limits 被 Linux OOM Killer 终止"

    observable:
      metrics:
        - expression: "container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.95"
          threshold: "0.95"
          source: "prometheus"
        - expression: "kube_pod_container_status_last_terminated_reason{reason='OOMKilled'}"
          threshold: ">= 1"
          source: "prometheus"
      logs:
        - pattern: "OOMKilled"
          source: "stdout"
        - pattern: "Exit Code: 137"
          source: "stderr"
      events:
        - filter: "reason=OOMKilling"
      traces:
        - type: "arms"
          pattern: "memory.leak"

    root_causes:
      - cause: "应用内存泄漏"
        probability: 0.40
        evidence_type: "metric"
      - cause: "JVM 堆内存设置过大"
        probability: 0.25
        evidence_type: "metric"
      - cause: "资源 limits 设置过低"
        probability: 0.20
        evidence_type: "log"
      - cause: "突发流量导致内存突增"
        probability: 0.10
        evidence_type: "metric"
      - cause: "sidecar 容器内存未计入"
        probability: 0.05
        evidence_type: "event"

    diagnosis_commands:
      - command: |
          kubectl describe pod {pod} -n {namespace} | grep -A5 "Last State"
        expected_result: "Last State: Terminated, Reason: OOMKilled"
        timeout_seconds: 10

      - command: |
          kubectl top pod {pod} -n {namespace} --containers
        expected_result: "memory usage > 90% of limit"
        timeout_seconds: 15

      - command: |
          kubectl logs {pod} -n {namespace} --previous --tail=100 | grep -i "outofmemory"
        expected_result: "java.lang.OutOfMemoryError"
        timeout_seconds: 30

    healing_actions:
      - id: "HA-2.3.1"
        description: "增加内存 limits（推荐）"
        risk_level: "low"
        auto_healable: true
        requires_approval: false
        command: |
          kubectl patch deployment {deployment} -n {namespace} -p \
            '{"spec":{"template":{"spec":{"containers":[{
              "name":"{container}",
              "resources":{"limits":{"memory":"2Gi"},
                           "requests":{"memory":"1Gi"}}}]}}}}'
        rollback_command: |
          kubectl rollout undo deployment/{deployment} -n {namespace}
        success_rate: 0.90

      - id: "HA-2.3.2"
        description: "分析内存泄漏（需开发介入）"
        risk_level: "none"
        auto_healable: false
        requires_approval: false
        command: "通知开发团队使用 pprof/heapdump 分析"
        rollback_command: null
        success_rate: null

    probability:
      annual_rate: 0.05
      monthly_rate: 0.004
      mtbf_hours: 8760
      mttr_minutes: 15
      auto_heal_rate: 0.70
      detection_rate: 0.95

    ack_specific:
      cloud_provider: "Alibaba Cloud"
      related_aliyun_services: ["ECS", "OSS"]
      ecs_instance_types: ["ecs.c7", "ecs.g7"]
      eni_attachments: false
      essd_performance_tier: null

probability_matrix:
  events: ["BE-2.1", "BE-2.2", "BE-2.3", "BE-2.4"]
  matrix:
    - [0.10, 0.02, 0.01, 0.01]
    - [0.02, 0.08, 0.01, 0.01]
    - [0.01, 0.01, 0.05, 0.01]
    - [0.01, 0.01, 0.01, 0.03]
  min_cut_sets:
    - cut_set: ["BE-2.3"]
      probability: 0.05
      importance: 0.75

mermaid_diagram:
  diagram: |
    graph TD
      TE-2["应用服务不可用<br>P0"] --> IE-2.1["Pod运行异常"]
      IE-2.1 --> BE-2.1["CrashLoopBackOff"]
      IE-2.1 --> BE-2.2["ImagePullBackOff"]
      IE-2.1 --> BE-2.3["OOMKilled"]
      IE-2.1 --> BE-2.4["Evicted"]
      style BE-2.3 fill:#f96
  theme: "default"

ack_specific_paths:
  - path_id: "ACK-2.3-1"
    description: "ACK 特有: ESSD 云盘导致的 OOM"
    involves_aliyun_services: ["ECS", "ESSD"]
    mitigation: "使用本地 NVMe SSD 或 ESSD PL3"

confidence_score:
  overall: 0.85
  coverage: 0.88
  observability: 0.90
  automation: 0.75
  completeness: 0.82
  missing_items:
    - "ACK One 多集群场景未覆盖"
  recommended_actions:
    - "补充 ACK One 问题路径"
    - "增加 ARMS Java Agent 内存泄漏检测"

febm_integration:
  related_febm_cases:
    - "FEBM-case-INC-2026-0215"
  evidence_patterns:
    - "OOMKilled"
    - "Exit Code: 137"
    - "java.lang.OutOfMemoryError"
  forensic_timeline: "2026-02-15T16:32:15 内存突增 → 16:32:20 OOM → 16:32:25 Pod 重启"

metadata:
  generated_at: "2026-05-18T10:00:00Z"
  generator_version: "2.0"
  source_context: "source_code"
  language: "zh-CN"
```

---

## 9. 错误处理与回退机制

```yaml
error_handling:

  invalid_input:
    - error: "component 不在支持列表中"
      response: |
        生成错误: "Unsupported component: {component}"
        建议: "请使用以下组件之一: ack-api-server, terway, asm-istiod, ..."

    - error: "source_context=source_code 但未提供 repo_url"
      response: |
        生成错误: "repo_url required when source_context=source_code"

  generation_failures:
    - error: "无法解析源码错误处理路径"
      fallback: "使用标准 K8s 故障树模板 + ACK 特有层"

    - error: "Mermaid 生成失败"
      fallback: "输出纯文本故障树结构，跳过图形"

    - error: "confidence_score < 0.70"
      warning: "输出低置信度警告，建议人工评审"

  partial_success:
    - scenario: "部分底事件可观测性不足"
      response: |
        标记 missing_items: ["observable.metrics 缺失"]
        输出 warning 但继续生成

    - scenario: "ACK 特有组件未找到"
      response: |
        回退到标准 K8s 故障树
        建议: "请提供 {component} 的架构文档"
```

---

## 10. 完整示例对话

### 输入 1：源码分析

```yaml
input:
  component: "ack-scheduler"
  scope: "control-plane"
  severity: "P0"
  source_context: "source_code"
  source_details:
    repo_url: "gitlab.alibaba-inc.com/acs/ack-scheduler"
    entry_points:
      - "pkg/scheduler/core.Evaluate"
      - "pkg/scheduler/core.GenericScheduler.Schedule"
    error_handling_patterns:
      - "frameworkext.ErrorInsufficientResource"
      - "frameworkext.ErrorNotEnoughSpace"
      - "apierrors.IsUnexpected"
```

### 输出 1：生成的 FTA

```yaml
output:
  schema_version: "2.0"

  top_event:
    id: "TE-SCH-1"
    name: "调度器无法完成 Pod 调度"
    description: "Pod 处于 Pending 状态超过 5 分钟"
    severity: "P0"
    slo_impact: "部署成功率 < 99%"
    affected_scope: "集群内所有命名空间"

  fault_tree:
    nodes:
      - id: "TE-SCH-1"
        type: "TE"
        name: "调度器无法完成 Pod 调度"
        level: 1

      - id: "IE-SCH-1.1"
        type: "IE"
        name: "调度决策失败"
        gate_type: "OR"
        parent_id: "TE-SCH-1"
        level: 2

      - id: "BE-SCH-1.1"
        type: "BE"
        name: "节点资源不足"
        parent_id: "IE-SCH-1.1"
        level: 3
        probability: 0.35
        mttr_minutes: 20

      # ... 更多节点

  bottom_events:
    - id: "BE-SCH-1.1"
      name: "节点资源不足"
      description: "所有可用节点的 CPU/内存/存储不满足 Pod 需求"

      observable:
        metrics:
          - expression: "sum(kube_node_status_allocatable{resource='cpu'}) - sum(kube_pod_container_resource_requests_cpu) < {request}"
            threshold: "负数表示资源不足"
            source: "prometheus"

      diagnosis_commands:
        - command: "kubectl describe node {node} | grep -A10 'Allocated resources'"
          expected_result: "Allocated: cpu/memory 接近 limits"
        - command: "kubectl top node"
          expected_result: "cpu/memory 使用率 > 90%"

      healing_actions:
        - id: "HA-SCH-1.1.1"
          description: "扩容节点池"
          risk_level: "medium"
          auto_healable: true
          command: "ack autoscaler scale-up --node-pool {pool} --count {delta}"

  # ... 其他输出字段

  metadata:
    generated_at: "2026-05-18T10:00:00Z"
    generator_version: "2.0"
    source_context: "source_code"
    language: "zh-CN"
```

---

## 附录 A: 错误代码对照表

```yaml
error_codes:

  # 输入验证错误 (1000-1999)
  E1001: "无效的 component 名称"
  E1002: "无效的 scope 值"
  E1003: "无效的 severity 级别"
  E1004: "缺少必需的输入字段"
  E1005: "source_context 与 source_details 不匹配"

  # 生成错误 (2000-2999)
  E2001: "无法解析源码结构"
  E2002: "无法识别错误处理模式"
  E2003: "故障树层级超限"
  E2004: "循环依赖检测"
  E2005: "逻辑门类型矛盾"

  # 底事件错误 (3000-3999)
  E3001: "底事件可观测性不足"
  E3002: "底事件缺少诊断命令"
  E3003: "底事件缺少修复动作"
  E3004: "概率数据缺失"

  # 输出错误 (4000-4999)
  E4001: "JSON Schema 验证失败"
  E4002: "Mermaid 语法错误"
  E4003: "编号连续性错误"
  E4004: "输出格式不完整"

  # ACK 特有错误 (5000-5999)
  E5001: "阿里云服务依赖未解析"
  E5002: "Terway 配置无法识别"
  E5003: "ASM Istio 版本不兼容"
```

---

## 附录 B: 版本变更日志

```yaml
changelog:

  v2.0 (2026-05-18):
    - 增强: 输入 Schema 增加 source_context 区分
    - 增强: 输出 Schema 增加 confidence_score
    - 增强: ACK 特有组件覆盖 Terway/ASM/ARMS/ACK-One
    - 增强: FTA-FEBM 双向融合机制
    - 增强: 自动化 FTA 生成能力（源码/告警/日志）
    - 增强: 可执行格式转换（Shell/Ansible/K8s Operator）
    - 新增: 质量保证清单（生成前/后检查）
    - 新增: 错误处理与回退机制

  v1.0 (2026-02):
    - 初始版本: 基础 FTA 生成能力
    - 基础 K8s 故障树覆盖
    - 通用 FTA 方法论支持
```

<!-- risk-assessed -->
