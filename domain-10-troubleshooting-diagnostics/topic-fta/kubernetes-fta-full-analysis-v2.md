---
title: Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 (domain-10-troubleshooting-diagnostics)
description: 'title: Kubernetes 全量故障树分析(FTA)排查手册 - 增强版'
category: fta
tags:
- fta
- troubleshooting
- etcd
- kubelet
- scheduler
- prometheus
- grafana
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
estimated_read_time: 35min
intent_queries:
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 是什么
- 如何 Kubernetes 全量故障树分析(FTA)排查手册 - 增强版
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 故障排查
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 排障步骤
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 根因分析
trigger_keywords:
- Kubernetes
- 全量故障树分析
- FTA
- 排查手册
- 增强版
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- cni-basics
- etcd-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- tracing-basics
- observability-basics
fta_id: FTA-KUBERNETES_FULL_ANALYSIS_V2-001
component: Kubernetes Full Analysis V2
severity: critical
created: "2026-05-23"
---

title: [[Kubernetes|Kubernetes]]es 全量故障树分析(FTA)排查手册|Kubernetes 全量故障树分析(FTA)排查手册]] - 增强版
description: '# Kubernetes 全量故障树分析(FTA)排查手册 - 增强版'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[etcd|etcd]]
- kubelet
- scheduler
- prometheus
- grafana
- jaeger
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 是什么
- 如何 Kubernetes 全量故障树分析(FTA)排查手册 - 增强版
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 根因分析
- Kubernetes 全量故障树分析(FTA)排查手册 - 增强版 故障树
trigger_keywords:
- Kubernetes
- 全量故障树分析
- FTA
- 排查手册
- 增强版
- fta
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

# Kubernetes 全量故障树分析(FTA)排查手册 - 增强版

> **文档版本**: v2.0 Enhanced
> **适用范围**: Kubernetes 生产环境全量问题场景 + ACK 特有场景
> **更新日期**: 2026-05-18

---

<!-- chunk: 一、故障树总览 -->## 一、故障树总览

## 1.1 顶部事件定义表（增强版）

| 编号 | 顶部事件 | 严重程度 | 影响范围 | 典型症状 | ACK 特有 |
|:---|:---|:---:|:---|:---|:---|
| TE-1 | 集群完全不可用 | 🔴 P0 | 整个集群 | kubectl无法连接，所有服务中断 | ECS/ESSD/SLB |
| TE-2 | 应用服务不可用 | 🔴 P0 | 特定应用 | 用户无法访问应用，HTTP 5xx错误 | ASM/ARMS/Terway |
| TE-3 | Pod启动失败 | 🟠 P1 | 特定Pod | Pod处于Pending/Error状态 | ACK 调度器 |
| TE-4 | 网络通信异常 | 🟠 P1 | 网络层面 | DNS解析失败，Pod间无法通信 | Terway ENI/IPVLAN |
| TE-5 | 存储访问失败 | 🟠 P1 | 存储层面 | PVC无法绑定，卷挂载失败 | OSS/CSI/NAS |
| TE-6 | 资源调度异常 | 🟡 P2 | 调度层面 | Pod无法调度，调度结果异常 | ACK 资源配额 |
| TE-7 | 安全认证失败 | 🟠 P1 | 安全层面 | 认证/授权失败，证书过期 | ACK RAM/PSP |
| TE-8 | 监控告警异常 | 🟡 P2 | 监控层面 | 指标丢失，告警不触发 | ARMS/MSP |
| TE-9 | Terway 网络问题 | 🟠 P1 | Pod 网络 | Pod 无法获取 IP/网络不通 | Terway 独有 |
| TE-10 | ASM 服务网格问题 | 🟠 P1 | 网格流量 | .sidecar 无法连接/mTLS 失败 | ASM 独有 |
| TE-11 | ACK-One 多集群异常 | 🟠 P1 | 多集群 | 集群注册失败/配置同步延迟 | ACK-One 独有 |
| TE-12 | 资源配额超限 | 🟡 P2 | 账户级 | API 对象创建失败/配额耗尽 | ACK 独有 |
| TE-13 | 变更管理问题 | 🟠 P1 | 变更过程 | 升级失败/回滚/配置漂移 | GitOps/RAC |
| TE-14 | 容量规划失效 | 🟡 P2 | 资源容量 | 节点资源耗尽/存储容量不足 | 自动扩容 |
| TE-15 | 灾难恢复失败 | 🔴 P0 | 业务连续性 | 备份恢复失败/DR 演练失败 | 备份/DR |
| TE-16 | 可观测性完整性缺失 | 🟡 P2 | 监控盲区 | 关键指标丢失/追踪断裂 | OTel/可观测性 |

## 1.2 故障树总览图 (ASCII)

```
                                    ┌─────────────────────────────────────────────────┐
                                    │         Kubernetes + ACK 问题空间              │
                                    └────────────────────┬────────────────────────────┘
                                                       │
    ┌───────────┬───────────┬───────────┬────────────┴────────┬───────────┬───────────┬───────────┐
    │           │           │           │                     │           │           │           │
    ▼           ▼           ▼           ▼                     ▼           ▼           ▼           ▼
┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐             ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
│  TE-1  │ │  TE-2  │ │  TE-3  │ │  TE-4  │             │  TE-9  │ │ TE-10  │ │ TE-11  │ │ TE-12  │
│ 集群   │ │ 应用   │ │ Pod    │ │ 网络   │             │ Terway │ │  ASM   │ │ ACK-One│ │ 资源   │
│ 完全   │ │ 服务   │ │ 启动   │ │ 通信   │             │ 网络   │ │ 服务   │ │ 多集群 │ │ 配额   │
│ 不可用 │ │ 不可用 │ │ 失败   │ │ 异常   │             │ 问题   │ │ 网格   │ │ 异常   │ │ 超限   │
│  🔴P0  │ │  🔴P0  │ │  🟠P1  │ │  🟠P1  │             │  🟠P1  │ │  🟠P1  │ │  🟠P1  │ │  🟡P2  │
└───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘             └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘
    │           │           │           │                     │           │           │           │
    ▼           ▼           ▼           ▼                     ▼           ▼           ▼           ▼
┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐             ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
│  TE-5  │ │  TE-6  │ │  TE-7  │ │  TE-8  │             │ TE-13  │ │ TE-14  │ │ TE-15  │ │ TE-16  │
│ 存储   │ │ 资源   │ │ 安全   │ │ 监控   │             │ 变更   │ │ 容量   │ │ 灾难   │ │ 可观测 │
│ 访问   │ │ 调度   │ │ 认证   │ │ 告警   │             │ 管理   │ │ 规划   │ │ 恢复   │ │ 缺失   │
│ 失败   │ │ 异常   │ │ 失败   │ │ 异常   │             │ 问题   │ │ 失效   │ │ 失败   │ │        │
│  🟠P1  │ │  🟡P2  │ │  🟠P1  │ │  🟡P2  │             │  🟠P1  │ │  🟡P2  │ │  🔴P0  │ │  🟡P2  │
└────────┘ └────────┘ └────────┘ └────────┘             └────────┘ └────────┘ └────────┘ └────────┘


详细故障树结构:
═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════
```

---

<!-- chunk: 二、TE-1: 集群完全不可用 🔴 P0 -->## 二、TE-1: 集群完全不可用 🔴 P0

## 2.1 完整故障树（5 层深度 + ACK IaaS 层）

```
TE-1: 集群完全不可用 [OR门] 🔴 P0
│
├── IE-1.1 控制平面问题 [OR门]
│   ├── BE-1.1 API Server问题
│   │   ├── BE-1.1.1 API Server OOM
│   │   │   └── BE-1.1.1.1 etcd 数据量过大导致内存压力
│   │   ├── BE-1.1.2 API Server 证书过期
│   │   │   ├── BE-1.1.2.1 阿里云控制台证书配置错误
│   │   │   └── BE-1.1.2.2 ACK 托管 API Server 证书轮换失败
│   │   ├── BE-1.1.3 API Server 网络不可达
│   │   │   ├── BE-1.1.3.1 SLB 后端权重配置异常
│   │   │   └── BE-1.1.3.2 安全组规则阻止 6443 端口
│   │   ├── BE-1.1.4 API Server 启动参数错误
│   │   └── BE-1.1.5 API Server 依赖组件问题
│   │
│   ├── BE-1.2 etcd集群问题
│   │   ├── BE-1.2.1 etcd 磁盘空间耗尽
│   │   │   ├── BE-1.2.1.1 ESSD 云盘性能降级 (PL3→PL1)
│   │   │   │   └── BE-1.2.1.1.1 阿里云存储 burst  credits 用尽
│   │   │   └── BE-1.2.1.2 快照积累导致磁盘满
│   │   ├── BE-1.2.2 etcd 仲裁丢失
│   │   │   ├── BE-1.2.2.1 网络分区导致节点间通信中断
│   │   │   │   └── BE-1.2.2.1.1 ECS ENI 多队列压力导致网络延迟
│   │   │   └── BE-1.2.2.2 三个节点同时认为自己是 leader
│   │   ├── BE-1.2.3 etcd 数据损坏
│   │   │   └── BE-1.2.3.1 ESSD 盘缓写导致数据不一致
│   │   ├── BE-1.2.4 etcd 性能降级
│   │   │   └── BE-1.2.4.1 高频写入导致 WAL 日志堆积
│   │   └── BE-1.2.5 etcd 证书问题
│   │
│   ├── BE-1.3 Scheduler问题
│   │   ├── BE-1.3.1 Scheduler 无法连接 API Server
│   │   └── BE-1.3.2 Scheduler 调度算法异常
│   │
│   └── BE-1.4 Controller Manager问题
│       ├── BE-1.4.1 CM 无法连接 API Server
│       └── BE-1.4.2 控制器循环问题
│
├── IE-1.2 工作节点批量问题 [AND门 - 需多数节点同时问题]
│   ├── BE-1.5 Kubelet服务问题
│   │   ├── BE-1.5.1 Kubelet OOM
│   │   ├── BE-1.5.2 Kubelet 与 API Server 通信断开
│   │   └── BE-1.5.3 Kubelet 证书过期
│   │
│   ├── BE-1.6 容器运行时问题
│   │   ├── BE-1.6.1 containerd 无法连接 containerd-shim
│   │   ├── BE-1.6.2 Docker daemon 无响应
│   │   └── BE-1.6.3 CNI 插件调用失败
│   │
│   └── BE-1.7 节点网络问题
│       ├── BE-1.7.1 节点到控制平面网络中断
│       ├── BE-1.7.2 节点 DNS 解析失败
│       └── BE-1.7.3 节点 ARP 表耗尽
│
├── IE-1.3 网络基础设施问题 [OR门]
│   ├── BE-1.8 CNI插件问题
│   │   ├── BE-1.8.1 Terway ENI 模式问题
│   │   │   ├── BE-1.8.1.1 ENI 多队列资源耗尽
│   │   │   │   └── BE-1.8.1.1.1 高密度 Pod 部署导致 ENI 带宽瓶颈
│   │   │   └── BE-1.8.1.2 Pod IP 分配耗尽
│   │   │       └── BE-1.8.1.2.1 VPC CIDR 子网容量不足
│   │   ├── BE-1.8.2 Terway IPVLAN 模式问题
│   │   │   └── BE-1.8.2.1 IPVLAN 连接泄漏 (netlink 资源耗尽)
│   │   └── BE-1.8.3 Flannel 模式问题
│   │
│   └── BE-1.9 核心网络设备问题
│       ├── BE-1.9.1 阿里云交换机问题
│       └── BE-1.9.2 NAT 网关问题
│
├── IE-1.4 阿里云 IaaS 层问题 [OR门] ← ACK 特有层
│   ├── BE-1.10 ECS 实例批量问题
│   │   ├── BE-1.10.1 ECS 实例被驱逐（竞价实例中断）
│   │   ├── BE-1.10.2 ECS 实例网络分区 (ENI 链路中断)
│   │   └── BE-1.10.3 ECS 实例硬件问题 (内存/CPU/磁盘)
│   │
│   ├── BE-1.11 阿里云 SLB 问题
│   │   ├── BE-1.11.1 SLB 后端所有 ECS 健康检查失败
│   │   └── BE-1.11.2 SLB 连接数达到上限 (七层监听器限制)
│   │
│   └── BE-1.12 可用区问题
│       └── BE-1.12.1 阿里云可用区级问题导致多节点不可用
│
└── IE-1.5 运维人为失误 [OR门]
    ├── BE-1.13 误删除集群核心组件
    └── BE-1.14 错误的批量操作导致集群不可用
```

---

<!-- chunk: 三、TE-2: 应用服务不可用 🔴 P0 -->## 三、TE-2: 应用服务不可用 🔴 P0

## 3.1 完整故障树（5 层深度 + ASM/ARMS 层）

```
TE-2: 应用服务不可用 [OR门] 🔴 P0
│
├── IE-2.1 Pod运行异常 [OR门]
│   ├── BE-2.1 CrashLoopBackOff
│   │   ├── BE-2.1.1 应用配置错误
│   │   │   ├── BE-2.1.1.1 ConfigMap 挂载路径错误
│   │   │   └── BE-2.1.1.2 Secret 引用不存在
│   │   ├── BE-2.1.2 应用启动命令错误
│   │   │   └── BE-2.1.2.1 command/args 配置不兼容
│   │   └── BE-2.1.3 健康检查配置错误
│   │
│   ├── BE-2.2 ImagePullBackOff
│   │   ├── BE-2.2.1 镜像不存在
│   │   │   └── BE-2.2.1.1 阿里云容器镜像仓库网络隔离
│   │   ├── BE-2.2.2 镜像仓库认证失败
│   │   │   └── BE-2.2.2.1 ACK 默认 ServiceAccount 镜像拉取凭证失效
│   │   └── BE-2.2.3 镜像拉取超时
│   │
│   ├── BE-2.3 OOMKilled
│   │   ├── BE-2.3.1 应用内存泄漏
│   │   │   └── BE-2.3.1.1 Java JVM heap space 泄漏 (OrderCache.loadAll)
│   │   ├── BE-2.3.2 资源 limits 设置不当
│   │   │   └── BE-2.3.2.1 JVM heap 设置 > container limit (1.2Gi > 1Gi)
│   │   ├── BE-2.3.3 突发流量导致内存突增
│   │   │   └── BE-2.3.3.1 HPA 扩容后连接池配置未同步调整
│   │   └── BE-2.3.4 Sidecar 内存未计入
│   │       └── BE-2.3.4.1 Istio envoy sidecar 内存占用未考虑
│   │
│   └── BE-2.4 Evicted
│       ├── BE-2.4.1 节点压力驱逐 (MemoryPressure)
│       └── BE-2.4.2 磁盘压力驱逐 (DiskPressure)
│
├── IE-2.2 Service/Endpoint 访问异常 [OR门]
│   ├── BE-2.5 无可用 Endpoint
│   │   ├── BE-2.5.1 Pod readinessProbe 失败
│   │   │   ├── BE-2.5.1.1 应用启动时间过长 (readinessProbe 超时)
│   │   │   └── BE-2.5.1.2 健康检查端口配置错误
│   │   ├── BE-2.5.2 Selector 匹配为空
│   │   │   └── BE-2.5.2.1 Deployment label 变更导致 selector 失效
│   │   └── BE-2.5.3 所有 Pod 处于 terminating 状态
│   │
│   ├── BE-2.6 端口配置错误
│   │   ├── BE-2.6.1 Service port 与容器 port 不匹配
│   │   └── BE-2.6.2 NodePort 冲突
│   │
│   └── BE-2.7 kube-proxy 问题
│       └── BE-2.7.1 iptables 规则丢失
│
├── IE-2.3 Ingress/IngressController 访问异常 [OR门]
│   ├── BE-2.8 Ingress Controller 问题
│   │   ├── BE-2.8.1 Nginx Ingress Controller OOM
│   │   ├── BE-2.8.2 Ingress Controller 与 API Server 通信断开
│   │   └── BE-2.8.3 ACK MSE Ingress 网关问题
│   │
│   ├── BE-2.9 Ingress 规则配置错误
│   │   ├── BE-2.9.1 Host 路径冲突
│   │   └── BE-2.9.2 TLS 证书配置错误
│   │
│   └── BE-2.10 负载均衡器问题
│       ├── BE-2.10.1 SLB 健康检查失败
│       └── BE-2.10.2 后端权重配置错误
│
├── IE-2.4 ASM 服务网格问题 [OR门] ← ACK 特有层
│   ├── BE-2.11 Envoy sidecar 问题
│   │   ├── BE-2.11.1 Envoy 内存溢出 (envoy OOMKilled)
│   │   │   └── BE-2.11.1.1 高并发请求导致 envoy 内存飙升
│   │   ├── BE-2.11.2 Envoy 连接池耗尽
│   │   │   └── BE-2.11.2.1 CircuitBreaker 触发导致连接池满
│   │   ├── BE-2.11.3 Envoy 健康检查失败
│   │   │   └── BE-2.11.3.1 readinessProbe 配置不当
│   │   └── BE-2.11.4 mTLS 证书过期
│   │
│   ├── BE-2.12 控制平面问题
│   │   ├── BE-2.12.1 Istiod (Pilot) 问题
│   │   │   └── BE-2.12.1.1 xDS 配置推送失败
│   │   ├── BE-2.12.2 VirtualService 配置错误
│   │   └── BE-2.12.3 目标规则 (DestinationRule) 冲突
│   │
│   └── BE-2.13 流量管理异常
│       ├── BE-2.13.1 灰度发布导致流量不均
│       │   └── BE-2.13.1.1 VS 权重配置 0.1% 导致流量不均
│       └── BE-2.13.2 限流规则触发
│           └── BE-2.13.2.1 全局限流导致请求被拒
│
└── IE-2.5 ARMS 应用监控问题 [OR门] ← ACK 特有层
    ├── BE-2.14 ARMS Java Agent 注入失败
    │   └── BE-2.14.1 字节码增强失败导致应用启动失败
    ├── BE-2.15 调用链追踪断裂
    │   └── BE-2.15.1 采样率配置错误导致追踪丢失
    └── BE-2.16 异常堆栈采集不完整
        └── BE-2.16.1 OOM 场景下堆栈采集进程崩溃
```

---

<!-- chunk: 四、TE-3: Pod启动失败 🟠 P1 -->## 四、TE-3: Pod启动失败 🟠 P1

## 4.1 完整故障树

```
TE-3: Pod启动失败 [OR门] 🟠 P1
│
├── IE-3.1 调度失败 [OR门]
│   ├── BE-3.1 节点资源不足
│   │   ├── BE-3.1.1 CPU 资源不足
│   │   │   └── BE-3.1.1.1 ACK 资源组 CPU 配额耗尽
│   │   └── BE-3.1.2 Memory 资源不足
│   │       └── BE-3.1.2.1 节点 memory pressure 导致调度失败
│   │
│   ├── BE-3.2 节点选择器不匹配
│   │   └── BE-3.2.1 nodeSelector 标签无匹配节点
│   │
│   ├── BE-3.3 污点阻止调度
│   │   ├── BE-3.3.1 节点污点未容忍
│   │   └── BE-3.3.2 ACK 托管节点污点 (aliyun.oshift)
│   │
│   └── BE-3.4 资源配额超限
│       ├── BE-3.4.1 命名空间 CPU quota 耗尽
│       └── BE-3.4.2 命名空间 Memory quota 耗尽
│
├── IE-3.2 镜像拉取失败 [OR门]
│   ├── BE-3.5 镜像不存在
│   │   ├── BE-3.5.1 镜像 tag 不存在
│   │   └── BE-3.5.2 私有仓库网络隔离
│   │
│   ├── BE-3.6 镜像仓库认证失败
│   │   ├── BE-3.6.1 ACR 凭证过期
│   │   └── BE-3.6.2 ImagePullSecret 引用错误
│   │
│   └── BE-3.7 网络不可达
│       ├── BE-3.7.1 节点无法访问镜像仓库
│       │   └── BE-3.7.1.1 VPC 安全组阻止出站 443 端口
│       └── BE-3.7.2 DNS 解析失败
│
└── IE-3.3 容器创建失败 [OR门]
    ├── BE-3.8 CNI 配置失败
    │   └── BE-3.8.1 Terway ENI 分配失败
    │       └── BE-3.8.1.1 ENI 绑定数达到上限 (ecs.g5 实例 max 3)
    │
    ├── BE-3.9 存储挂载失败
    │   ├── BE-3.9.1 PVC 绑定超时
    │   │   └── BE-3.9.1.1 StorageClass 配置错误导致 PVC pending
    │   ├── BE-3.9.2 挂载点不存在
    │   │   └── BE-3.9.2.1 SubPath 引用错误
    │   └── BE-3.9.3 CSI 驱动异常
    │       └── BE-3.9.3.1 OSS CSI driver 无法连接 OSS
    │
    └── BE-3.10 Init 容器失败
        ├── BE-3.10.1 Init 容器镜像拉取失败
        └── BE-3.10.2 Init 容器命令执行失败
```

---

<!-- chunk: 五、TE-4: 网络通信异常 🟠 P1 -->## 五、TE-4: 网络通信异常 🟠 P1

## 5.1 完整故障树（包含 Terway 特有层）

```
TE-4: 网络通信异常 [OR门] 🟠 P1
│
├── IE-4.1 DNS 解析异常 [OR门]
│   ├── BE-4.1 CoreDNS Pod 问题
│   │   ├── BE-4.1.1 CoreDNS Pod 处于 CrashLoopBackOff
│   │   └── BE-4.1.2 CoreDNS Pod 所有副本均不可用
│   │
│   ├── BE-4.2 DNS 配置错误
│   │   ├── BE-4.2.1 /etc/resolv.conf 配置错误
│   │   │   └── BE-4.2.1.1 node-networking 配置导致 DNS cluster domain 错误
│   │   └── BE-4.2.2 kube-dns ConfigMap 配置错误
│   │
│   └── BE-4.3 网络策略阻止 DNS
│       └── BE-4.3.1 NetworkPolicy 误阻断 CoreDNS
│
├── IE-4.2 Pod 间通信异常 [OR门]
│   ├── BE-4.4 CNI 插件问题
│   │   ├── BE-4.4.1 Terway ENI 模式问题
│   │   │   ├── BE-4.4.1.1 ENI 多队列带宽耗尽
│   │   │   │   └── BE-4.4.1.1.1 高密度 Pod 导致 ENI 带宽瓶颈
│   │   │   ├── BE-4.4.1.2 Pod IP 漂移 (IP 分配冲突)
│   │   │   │   └── BE-4.4.1.2.1 Terway IPAM 锁竞争导致分配延迟
│   │   │   └── BE-4.4.1.3 安全组规则覆盖
│   │   │
│   │   ├── BE-4.4.2 Terway IPVLAN 模式问题
│   │   │   ├── BE-4.4.2.1 IPVLAN 网络策略不生效
│   │   │   │   └── BE-4.4.2.1.1 内核版本不兼容 (需要 >= 5.10)
│   │   │   └── BE-4.4.2.2 IPVLAN 连接泄漏
│   │   │       └── BE-4.4.2.2.1 netlink 资源耗尽 (socket fd 泄漏)
│   │   │
│   │   └── BE-4.4.3 Flannel 模式问题
│   │       └── BE-4.4.3.1 Flannel VXLAN 隧道断裂
│   │
│   ├── BE-4.5 网络策略阻止
│   │   ├── BE-4.5.1 Calico NetworkPolicy 误阻断
│   │   └── BE-4.5.2 过于严格的 default-deny 策略
│   │
│   └── BE-4.6 iptables 规则错误
│       └── BE-4.6.1 kube-proxy 规则丢失
│
├── IE-4.3 集群外部访问异常 [OR门]
│   ├── BE-4.7 Egress 配置错误
│   │   ├── BE-4.7.1  egressGateway 配置错误
│   │   └── BE-4.7.2 NATGW 配置异常
│   │
│   ├── BE-4.8 NAT 配置问题
│   │   └── BE-4.8.1 SNAT 规则丢失导致无法访问公网
│   │
│   └── BE-4.9 防火墙阻止
│       └── BE-4.9.1 VPC 安全组阻止出站流量
│
├── IE-4.4 SLB/Ingress 外部访问异常 [OR门] ← ACK 特有层
│   ├── BE-4.10 SLB 后端健康检查失败
│   │   ├── BE-4.10.1 SLB 健康检查超时
│   │   │   └── BE-4.10.1.1 健康检查路径响应慢 (应用未就绪)
│   │   └── BE-4.10.2 SLB 健康检查端口不匹配
│   │
│   └── BE-4.11 SLB 连接异常
│       ├── BE-4.11.1 SLB 连接数超限 (七层监听器限制 5000)
│       └── BE-4.11.2 SSL 证书问题导致 HTTPS 失败
│
└── IE-4.5 跨可用区网络延迟 [OR门]
    ├── BE-4.12 可用区网络抖动
    └── BE-4.13 跨可用区流量路由错误
```

---

<!-- chunk: 六、TE-5: 存储访问失败 🟠 P1 -->## 六、TE-5: 存储访问失败 🟠 P1

## 6.1 完整故障树（包含 OSS/CSI 特有层）

```
TE-5: 存储访问失败 [OR门] 🟠 P1
│
├── IE-5.1 PVC 绑定失败 [OR门]
│   ├── BE-5.1 StorageClass 配置错误
│   │   ├── BE-5.1.1 StorageClass provisioner 不存在
│   │   └── BE-5.1.2 ACK 托管 StorageClass 不可用
│   │
│   ├── BE-5.2 PV 资源不足
│   │   ├── BE-5.2.1 云盘盘容量不足
│   │   │   └── BE-5.2.1.1 ESSD PL0 容量规划不足 (500G 限制)
│   │   └── BE-5.2.2 NFS 挂载点饱和
│   │
│   └── BE-5.3 CSI 驱动异常
│       ├── BE-5.3.1 CSI driver 无法连接云盘后端
│       │   └── BE-5.3.1.1 ACK 控制台云盘服务临时不可用
│       └── BE-5.3.2 CSI driver 版本不兼容
│
├── IE-5.2 存储卷挂载失败 [OR门]
│   ├── BE-5.4 挂载参数错误
│   │   ├── BE-5.4.1 mountOptions 不兼容
│   │   │   └── BE-5.4.1.1 ESSD 不支持 barrier mount 选项
│   │   └── BE-5.4.2 挂载路径不存在
│   │
│   ├── BE-5.5 权限不足
│   │   └── BE-5.5.1 Pod 使用 non-root 用户无法写入挂载卷
│   │
│   └── BE-5.6 文件系统损坏
│       ├── BE-5.6.1 ext4 文件系统元数据损坏
│       │   └── BE-5.6.1.1 突然断电导致文件系统 journal 损坏
│       └── BE-5.6.2 XFS 文件系统损坏
│
├── IE-5.3 存储性能/数据异常 [OR门]
│   ├── BE-5.7 存储后端性能下降
│   │   ├── BE-5.7.1 ESSD IOPS 抖动
│   │   │   └── BE-5.7.1.1 阿里云存储 burst credits 用尽导致降级
│   │   ├── BE-5.7.2 NAS 延迟突增
│   │   │   └── BE-5.7.2.1 NAS 挂载点网络分区
│   │   └── BE-5.7.3 OSS 吞吐量下降
│   │
│   ├── BE-5.8 数据损坏
│   │   ├── BE-5.8.1 云盘数据块损坏 (静默数据腐蚀)
│   │   │   └── BE-5.8.1.1 ECS 云盘底层存储介质问题
│   │   └── BE-5.8.2 数据库文件损坏
│   │
│   └── BE-5.9 快照恢复失败
│       └── BE-5.9.1 快照策略配置错误导致恢复超时
│
└── IE-5.4 ACK 特有存储问题 [OR门]
    ├── BE-5.10 OSS 挂载超时
    │   └── BE-5.10.1 OSSFS 挂载配置错误导致 Kennedy 读取失败
    │
    └── BE-5.11 云盘自动扩容失败
        └── BE-5.11.1 PVC 到达上限 (ack-node-pool 磁盘配额)
```

---

<!-- chunk: 七、TE-6: 资源调度异常 🟡 P2 -->## 七、TE-6: 资源调度异常 🟡 P2

## 7.1 完整故障树

```
TE-6: 资源调度异常 [OR门] 🟡 P2
│
├── IE-6.1 Pod 无法调度 [OR门]
│   ├── BE-6.1 节点资源不足
│   │   ├── BE-6.1.1 ACK 节点池资源不足 (无节点可调度)
│   │   │   └── BE-6.1.1.1 自动扩容延迟导致资源短暂不足
│   │   └── BE-6.1.2 节点资源碎片化 (CPU 碎片/Memory 碎片)
│   │
│   ├── BE-6.2 亲和性冲突
│   │   ├── BE-6.2.1 Pod affinity 与反亲和冲突
│   │   └── BE-6.2.2 节点亲和性无法满足
│   │
│   └── BE-6.3 污点不匹配
│       └── BE-6.3.1 节点污点无对应 Toleration
│
├── IE-6.2 调度结果不符合预期 [OR门]
│   ├── BE-6.4 调度器配置错误
│   │   ├── BE-6.4.1 调度策略配置错误
│   │   │   └── BE-6.4.1.1 调度器被配置为仅使用特定可用区
│   │   └── BE-6.4.2 优先级配置异常
│   │       └── BE-6.4.2.1 system cluster-critical priorityClass 优先级错误
│   │
│   └── BE-6.5 优先级抢占问题
│       └── BE-6.5.1 抢占导致低优先级 Pod 反复驱逐
│
├── IE-6.3 自定义调度器问题 [OR门]
│   ├── BE-6.6 调度器插件错误
│   │   └── BE-6.6.1 Volcano 调度器插件崩溃
│   │
│   └── BE-6.7 扩展点配置错误
│       └── BE-6.7.1 自定义调度器 webhook 超时
│
└── IE-6.4 ACK 资源配额限制 [OR门] ← ACK 特有层
    ├── BE-6.8 集群对象数量限制
    │   └── BE-6.8.1 API Server 对象数量限制 (默认 11000)
    │
    └── BE-6.9 资源组配额超限
        └── BE-6.9.1 ACK 企业资源组 CPU 配额耗尽
```

---

<!-- chunk: 八、TE-7: 安全认证失败 🟠 P1 -->## 八、TE-7: 安全认证失败 🟠 P1

## 8.1 完整故障树

```
TE-7: 安全认证失败 [OR门] 🟠 P1
│
├── IE-7.1 证书相关问题 [OR门]
│   ├── BE-7.1 证书过期
│   │   ├── BE-7.1.1 API Server 证书过期
│   │   │   └── BE-7.1.1.1 ACK 托管 API Server 证书轮换失败
│   │   ├── BE-7.1.2 Kubelet 证书过期
│   │   └── BE-7.1.3 阿里云 SLB 证书过期
│   │
│   ├── BE-7.2 证书链不完整
│   │   └── BE-7.2.1 根CA证书缺失
│   │
│   └── BE-7.3 CA 配置错误
│       └── BE-7.3.1 bootstrap 认证失败
│
├── IE-7.2 RBAC 权限问题 [OR门]
│   ├── BE-7.4 Role 配置错误
│   │   ├── BE-7.4.1 Role 规则配置错误导致权限不足
│   │   └── BE-7.4.2 ClusterRoleBinding 绑定错误
│   │
│   ├── BE-7.5 ServiceAccount 权限问题
│   │   ├── BE-7.5.1 ServiceAccount token 未挂载
│   │   │   └── BE-7.5.1.1 自动注入 SA token 失败 (Istio sidecar)
│   │   └── BE-7.5.2 RBAC 默认限制导致访问 API Server 失败
│   │
│   └── BE-7.6 PSP (Pod Security Policy) 问题
│       └── BE-7.6.1 PSP 限制导致 Pod 无法创建
│
├── IE-7.3 准入控制问题 [OR门]
│   ├── BE-7.7 Webhook 不可用
│   │   ├── BE-7.7.1 验证 webhook 服务不可达
│   │   │   └── BE-7.7.1.1 ACK 策略管理 webhook 不可用
│   │   └── BE-7.7.2 webhook 超时
│   │
│   └── BE-7.8 准入控制器配置错误
│       └── BE-7.8.1 MutatingWebhookConfiguration 冲突
│
└── IE-7.4 阿里云安全服务问题 [OR门] ← ACK 特有层
    ├── BE-7.9 RAM 服务问题
    │   └── BE-7.9.1 RAM 子账号凭证失效
    │
    └── BE-7.10 安全策略冲突
        └── BE-7.10.1 ACK 集群级安全策略与 Pod 冲突
```

---

<!-- chunk: 九、TE-8: 监控告警异常 🟡 P2 -->## 九、TE-8: 监控告警异常 🟡 P2

## 9.1 完整故障树（包含 ARMS/MSP 层）

```
TE-8: 监控告警异常 [OR门] 🟡 P2
│
├── IE-8.1 监控数据采集异常 [OR门]
│   ├── BE-8.1 Prometheus 问题
│   │   ├── BE-8.1.1 Prometheus OOM
│   │   │   └── BE-8.1.1.1 指标数量过多导致内存耗尽 ( > 100k metrics)
│   │   ├── BE-8.1.2 Prometheus 无法连接 API Server
│   │   └── BE-8.1.3 Prometheus WAL 损坏
│   │
│   ├── BE-8.2 ServiceMonitor 错误
│   │   ├── BE-8.2.1 ServiceMonitor selector 匹配为空
│   │   └── BE-8.2.2 ServiceMonitor endpoint 配置错误
│   │
│   └── BE-8.3 指标丢失
│       └── BE-8.3.1 kube-state-metrics 异常导致 missing series
│
├── IE-8.2 告警系统异常 [OR门]
│   ├── BE-8.4 Alertmanager 问题
│   │   ├── BE-8.4.1 Alertmanager 无法发送告警
│   │   │   └── BE-8.4.1.1 Alertmanager webhook 配置错误
│   │   └── BE-8.4.2 Alertmanager 无法去重导致告警风暴
│   │
│   ├── BE-8.5 告警规则错误
│   │   ├── BE-8.5.1 PromQL 表达式错误
│   │   └── BE-8.5.2 告警阈值配置不当导致误报
│   │
│   └── BE-8.6 通知渠道失败
│       └── BE-8.6.1 钉钉/WebHook 通知失败
│
├── IE-8.3 可视化系统异常 [OR门]
│   ├── BE-8.7 Grafana 问题
│   │   ├── BE-8.7.1 Grafana 无法连接 Prometheus 数据源
│   │   └── BE-8.7.2 Grafana 面板加载超时
│   │
│   └── BE-8.8 Dashboard 配置错误
│       └── BE-8.8.1 Dashboard 变量查询错误
│
└── IE-8.4 ARMS/MSP 特有问题 [OR门] ← ACK 特有层
    ├── BE-8.9 ACK 托管 Prometheus 问题
    │   ├── BE-8.9.1 MSP 服务临时不可用
    │   │   └── BE-8.9.1.1 阿里云 MSP 控制面问题导致采集中断
    │   └── BE-8.9.2 MSP API Server 连接异常
    │
    ├── BE-8.10 ARMS 应用监控问题
    │   ├── BE-8.10.1 ARMS SDK 数据上报失败 (内网隔离)
    │   │   └── BE-8.10.1.1 VPC 内网无法访问 ARMS 采集端点
    │   └── BE-8.10.2 ARMS Java Agent 采集线程阻塞
    │
    └── BE-8.11 链路追踪断裂
        └── BE-8.11.1 Jaeger 采集端超载导致追踪丢失
```

---

<!-- chunk: 十、TE-9: Terway 网络问题 🟠 P1 （新增） -->## 十、TE-9: Terway 网络问题 🟠 P1 （新增）

```
TE-9: Terway 网络问题 [OR门] 🟠 P1
│
├── IE-9.1 ENI 模式问题 [OR门]
│   ├── BE-9.1 ENI 多队列压力
│   │   ├── BE-9.1.1 单 Pod ENI 带宽瓶颈
│   │   │   └── BE-9.1.1.1 高并发 Pod 导致 VSwitch 带宽饱和
│   │   └── BE-9.1.2 ENI 绑定数超限
│   │       └── BE-9.1.2.1 ecs.g5 实例最大 3 个 ENI，实际需求 5 个
│   │
│   ├── BE-9.2 Pod IP 分配失败
│   │   ├── BE-9.2.1 VPC CIDR 子网容量耗尽
│   │   │   └── BE-9.2.1.1 集群节点数 > 200 导致 Pod IP 不够
│   │   └── BE-9.2.2 IPAM 锁竞争导致分配超时
│   │
│   └── BE-9.3 ENI 安全组冲突
│       └── BE-9.3.1 安全组规则被覆盖导致 Pod 无法通信
│
├── IE-9.2 IPVLAN 模式问题 [OR门]
│   ├── BE-9.4 IPVLAN 网络策略不生效
│   │   └── BE-9.4.1 内核版本 < 5.10 不支持 ipvlan network policy
│   │
│   ├── BE-9.5 IPVLAN 连接泄漏
│   │   └── BE-9.5.1 netlink socket fd 耗尽
│   │
│   └── BE-9.6 IPVLAN MTU 问题
│       └── BE-9.6.1 巨型帧 (9000) 导致分片
│
├── IE-9.3 BGP 模式问题 [OR门]
│   ├── BE-9.7 BGP 会话中断
│   │   └── BE-9.7.1 Terway BGP 进程崩溃
│   │
│   ├── BE-9.8 BGP 路由黑洞
│   │   └── BE-9.8.1 路由优先级冲突
│   │
│   └── BE-9.9 BGP AS 号冲突
│       └── BE-9.9.1 多集群使用相同 AS 号导致路由混乱
│
└── IE-9.4 Service/Ingress 流量异常 [OR门]
    ├── BE-9.10 kube-proxy 与 Terway 冲突
    │   └── BE-9.10.1 双协议栈导致 iptables 规则冲突
    │
    └── BE-9.11 LoadBalancer 注解失效
        └── BE-9.11.1 Terway CLB 注解配置错误
```

---

<!-- chunk: 十一、TE-10: ASM 服务网格问题 🟠 P1 （新增） -->## 十一、TE-10: ASM 服务网格问题 🟠 P1 （新增）

```
TE-10: ASM 服务网格问题 [OR门] 🟠 P1
│
├── IE-10.1 数据面问题 [OR门]
│   ├── BE-10.1 Envoy sidecar 资源耗尽
│   │   ├── BE-10.1.1 Envoy 内存溢出 (OOMKilled)
│   │   │   └── BE-10.1.1.1 高并发请求导致 envoy memory 飙升
│   │   ├── BE-10.1.2 Envoy CPU 满载
│   │   │   └── BE-10.1.2.1 大量并发长连接导致 CPU 100%
│   │   └── BE-10.1.3 Envoy 连接池耗尽
│   │       └── BE-10.1.3.1 CircuitBreaker 触发导致连接池满
│   │
│   └── BE-10.2 Envoy 健康检查失败
│       ├── BE-10.2.1 readinessProbe 超时
│       │   └── BE-10.2.1.1 应用响应延迟导致 readinessProbe 失败
│       └── BE-10.2.2 被动健康检查失败
│
├── IE-10.2 控制面问题 [OR门]
│   ├── BE-10.3 Istiod (Pilot/ Citadel/ Galley) 问题
│   │   ├── BE-10.3.1 xDS 配置推送失败
│   │   │   └── BE-10.3.1.1 Istiod OOM 导致配置推送中断
│   │   ├── BE-10.3.2 mTLS 证书轮换失败
│   │   │   └── BE-10.3.2.1 Citadel 无法签发新证书
│   │   └── BE-10.3.3 SDS 配置错误
│   │
│   └── BE-10.4 配置下发延迟
│       └── BE-10.4.1 VirtualService 配置变更延迟 > 30s
│
├── IE-10.3 流量管理问题 [OR门]
│   ├── BE-10.5 灰度发布异常
│   │   ├── BE-10.5.1 VS 权重配置错误导致流量倾斜
│   │   │   └── BE-10.5.1.1 新版本权重 0.1% 导致流量不均
│   │   └── BE-10.5.2 DestinationRule 连接池配置冲突
│   │
│   ├── BE-10.6 熔断规则触发
│   │   └── BE-10.6.1 CircuitBreaker 触发导致服务不可用
│   │
│   └── BE-10.7 限流规则冲突
│       └── BE-10.7.1 全局限流导致正常请求被拒绝
│
├── IE-10.4 可观测性问题 [OR门]
│   ├── BE-10.8 Envoy 指标丢失
│   │   └── BE-10.8.1 Prometheus 抓取 envoy metrics 失败
│   │
│   └── BE-10.9 链路追踪断裂
│       └── BE-10.9.1 采样率配置错误导致追踪丢失 90%
│
└── IE-10.5 ASM 特有配置问题 [OR门]
    ├── BE-10.10 阿里云 ASM 控制面问题
    │   └── BE-10.10.1 ASM 控制台临时不可用
    │
    └── BE-10.11 Annotations 配置错误
        └── BE-10.11.1 service.annotations 配置冲突导致 CLB 异常
```

---

<!-- chunk: 十二、TE-11: ACK-One 多集群异常 🟠 P1 （新增） -->## 十二、TE-11: ACK-One 多集群异常 🟠 P1 （新增）

```
TE-11: ACK-One 多集群异常 [OR门] 🟠 P1
│
├── IE-11.1 集群注册问题 [OR门]
│   ├── BE-11.1 注册代理通信失败
│   │   └── BE-11.1.1 网络隔离导致注册代理无法连接中心集群
│   │
│   └── BE-11.2 集群状态同步延迟
│       └── BE-11.2.1 etcd 延迟导致集群状态不一致
│
├── IE-11.2 跨集群服务发现问题 [OR门]
│   ├── BE-11.3 Federation DNS 解析失败
│   │   └── BE-11.3.1 全局 DNS 配置错误导致跨集群访问失败
│   │
│   └── BE-11.4 服务注册表不一致
│       └── BE-11.4.1 多集群服务 endpoint 同步延迟
│
├── IE-11.3 配置同步问题 [OR门]
│   ├── BE-11.5 GitOps 配置同步不一致
│   │   └── BE-11.5.1 ArgoCD 同步失败导致配置漂移
│   │
│   └── BE-11.6 策略同步失败
│       └── BE-11.6.1 Kyverno 策略未同步到子集群
│
└── IE-11.4 统一监控/日志问题 [OR门]
    ├── BE-11.7 中心集群采集失败
    │   └── BE-11.7.1 子集群 Prometheus 无法上报到中心集群
    │
    └── BE-11.8 日志聚合失败
        └── BE-11.8.1 Logstores 写入失败导致日志丢失
```

---

<!-- chunk: 十三、TE-12: 资源配额超限 🟡 P2 （新增） -->## 十三、TE-12: 资源配额超限 🟡 P2 （新增）

```
TE-12: 资源配额超限 [OR门] 🟡 P2
│
├── IE-12.1 API 对象数量限制 [OR门]
│   ├── BE-12.1 Pod 数量限制
│   │   └── BE-12.1.1 命名空间最大 Pod 数限制 (quota)
│   │
│   └── BE-12.2 Service 数量限制
│       └── BE-12.2.1 命名空间最大 Service 数限制
│
├── IE-12.2 ACK 资源组配额 [OR门] ← ACK 特有层
│   ├── BE-12.3 CPU 配额耗尽
│   │   └── BE-12.3.1 资源组 CPU 配额 100c，实际申请 120c
│   │
│   └── BE-12.4 内存配额耗尽
│       └── BE-12.4.1 资源组内存配额 256Gi，实际申请 300Gi
│
├── IE-12.3 存储配额 [OR门]
│   ├── BE-12.5 云盘配额耗尽
│   │   └── BE-12.5.1 账户级云盘数量限制
│   │
│   └── BE-12.6 OSS 配额耗尽
│       └── BE-12.6.1 OSS bucket 数量达到上限
│
└── IE-12.4 网络配额 [OR门]
    ├── BE-12.7 SLB 配额耗尽
    │   └── BE-12.7.1 区域级 SLB 实例数达到上限
    │
    └── BE-12.8 EIP 配额耗尽
        └── BE-12.8.1 账户级 EIP 数量限制
```

---

<!-- chunk: 十四、TE-13: 变更管理问题 🟠 P1 （新增） -->## 十四、TE-13: 变更管理问题 🟠 P1 （新增）

```
TE-13: 变更管理问题 [OR门] 🟠 P1
│
├── IE-13.1 升级失败 [OR门]
│   ├── BE-13.1 集群升级失败
│   │   ├── BE-13.1.1 控制平面升级卡住
│   │   │   └── BE-13.1.1.1 API Server 升级超时导致集群不可用
│   │   └── BE-13.1.2 节点池升级失败
│   │       └── BE-13.1.2.1 节点池升级导致 Pod 反复重启
│   │
│   └── BE-13.2 组件升级失败
│       ├── BE-13.2.1 cert-manager 升级失败
│       │   └── BE-13.2.1.1 cert-manager CRD 迁移失败
│       └── BE-13.2.2 Ingress Controller 升级失败
│
├── IE-13.2 回滚失败 [OR门]
│   ├── BE-13.3 Deployment 回滚失败
│   │   └── BE-13.3.1 回滚后镜像与配置不兼容
│   │
│   └── BE-13.4 配置回滚失败
│       └── BE-13.4.1 ConfigMap 变更无法回滚
│
├── IE-13.3 配置漂移 [OR门]
│   ├── BE-13.5 GitOps 同步漂移
│   │   └── BE-13.5.1 ArgoCD 检测到 drift 但无法自动修复
│   │
│   └── BE-13.6 手动修改未同步
│       └── BE-13.6.1 手动修改 Deployment 导致 GitOps 状态不一致
│
└── IE-13.4 变更窗口问题 [OR门]
    ├── BE-13.7 变更窗口重叠
    │   └── BE-13.7.1 多个变更同时执行导致冲突
    │
    └── BE-13.8 变更窗口超时
        └── BE-13.8.1 变更窗口内未完成导致自动回滚
```

---

<!-- chunk: 十五、TE-14: 容量规划失效 🟡 P2 （新增） -->## 十五、TE-14: 容量规划失效 🟡 P2 （新增）

```
TE-14: 容量规划失效 [OR门] 🟡 P2
│
├── IE-14.1 节点容量耗尽 [OR门]
│   ├── BE-14.1 CPU 容量耗尽
│   │   ├── BE-14.1.1 节点 CPU 使用率持续 > 90%
│   │   │   └── BE-14.1.1.1 缺乏自动扩容机制导致容量不足
│   │   └── BE-14.1.2 CPU 碎片化
│   │
│   └── BE-14.2 Memory 容量耗尽
│       └── BE-14.2.1 节点 memory pressure 导致 Pod eviction
│
├── IE-14.2 存储容量耗尽 [OR门]
│   ├── BE-14.3 节点磁盘容量耗尽
│   │   ├── BE-14.3.1 日志目录磁盘满
│   │   │   └── BE-14.3.1.1 fluentd 日志积压导致磁盘满
│   │   └── BE-14.3.2 镜像目录磁盘满
│   │
│   └── BE-14.4 PVC 容量耗尽
│       └── BE-14.4.1 PVC 使用量达到存储上限
│
├── IE-14.3 自动扩容问题 [OR门] ← ACK 特有层
│   ├── BE-14.5 HPA 扩容失败
│   │   └── BE-14.5.1 HPA 扩容达到 maxReplicas 但仍无法满足负载
│   │
│   ├── BE-14.6 VPA 配置错误
│   │   └── BE-14.6.1 VPA 资源建议未应用
│   │
│   └── BE-14.7 节点池扩容失败
│       └── BE-14.7.1 节点池扩容达到上限 (max nodes)
│
└── IE-14.4 容量规划不当 [OR门]
    ├── BE-14.8 容量预估不足
    │   └── BE-14.8.1 业务增长超出规划导致容量不足
    │
    └── BE-14.9 容量浪费
        └── BE-14.9.1 资源预留过多导致浪费
```

---

<!-- chunk: 十六、TE-15: 灾难恢复失败 🔴 P0 （新增） -->## 十六、TE-15: 灾难恢复失败 🔴 P0 （新增）

```
TE-15: 灾难恢复失败 [OR门] 🔴 P0
│
├── IE-15.1 备份失败 [OR门]
│   ├── BE-15.1 etcd 备份失败
│   │   ├── BE-15.1.1 etcd snapshot 写入失败
│   │   │   └── BE-15.1.1.1 OSS 挂载失败导致无法写入备份
│   │   └── BE-15.1.2 备份计划未执行
│   │       └── BE-15.1.2.1 CronJob 备份任务失败
│   │
│   └── BE-15.2 应用数据备份失败
│       └── BE-15.2.1 Velero 备份失败
│
├── IE-15.2 恢复失败 [OR门]
│   ├── BE-15.3 etcd 恢复失败
│   │   ├── BE-15.3.1 快照数据损坏
│   │   │   └── BE-15.3.1.1 快照文件校验失败
│   │   └── BE-15.3.2 恢复后数据不一致
│   │
│   └── BE-15.4 应用恢复失败
│       └── BE-15.4.1 Velero restore 失败
│
├── IE-15.3 DR 演练失败 [OR门]
│   ├── BE-15.5 DR 切换失败
│   │   └── BE-15.5.1 DNS 切换超时导致 RTO 超标
│   │
│   └── BE-15.6 DR 数据同步失败
│       └── BE-15.6.1 跨区域复制延迟导致数据丢失
│
└── IE-15.4 跨区域问题 [OR门]
    ├── BE-15.7 主区域完全不可用
    │   └── BE-15.7.1 可用区级问题导致主区域失效
    │
    └── BE-15.8 跨区域网络中断
        └── BE-15.8.1 跨区域专线中断导致 DR 无法连接
```

---

<!-- chunk: 十七、TE-16: 可观测性完整性缺失 🟡 P2 （新增） -->## 十七、TE-16: 可观测性完整性缺失 🟡 P2 （新增）

```
TE-16: 可观测性完整性缺失 [OR门] 🟡 P2
│
├── IE-16.1 指标完整性缺失 [OR门]
│   ├── BE-16.1 关键业务指标缺失
│   │   └── BE-16.1.1 自定义指标未采集 (missing metrics)
│   │
│   └── BE-16.2 指标精度不足
│       └── BE-16.2.1 采样率过低导致告警延迟
│
├── IE-16.2 日志完整性缺失 [OR门]
│   ├── BE-16.3 关键服务日志缺失
│   │   └── BE-16.3.1 日志采集 agent 异常导致日志丢失
│   │
│   └── BE-16.4 日志保留不足
│       └── BE-16.4.1 日志保留期过短导致问题回溯困难
│
├── IE-16.3 链路追踪完整性缺失 [OR门]
│   ├── BE-16.5 追踪断层
│   │   ├── BE-16.5.1 采样率 1% 导致问题排查困难
│   │   │   └── BE-16.5.1.1 低流量路径未被采样，高流量路径正常
│   │   └── BE-16.5.2 跨服务追踪断裂
│   │       └── BE-16.5.2.1 B3 传播格式不兼容导致 trace ID 丢失
│   │
│   └── BE-16.6 追踪延迟过高
│       └── BE-16.6.1 Jaeger collector 超载导致追踪延迟 > 10s
│
├── IE-16.4 OpenTelemetry 集成问题 [OR门]
│   ├── BE-16.7 OTel Collector 配置错误
│   │   └── BE-16.7.1 exporter 配置错误导致数据无法上报
│   │
│   └── BE-16.8 OTel SDK 初始化失败
│       └── BE-16.8.1 应用内 OTel SDK 导致内存泄漏
│
└── IE-16.5 可观测性盲区 [OR门]
    ├── BE-16.9 新服务未接入监控
    │   └── BE-16.9.1 新部署服务缺少 ServiceMonitor
    │
    └── BE-16.10 第三方依赖监控缺失
        └── BE-16.10.1 数据库/缓存依赖无监控
```

---

<!-- chunk: 十八、底事件完整索引 -->## 十八、底事件完整索引

## 18.1 按故障域分类

| 故障域 | 底事件数量 | 顶事件覆盖 |
|:---|:---:|:---|
| 控制平面 (API Server/etcd/Scheduler/CM) | 25+ | TE-1 |
| 工作节点 (Kubelet/Container Runtime/CNI) | 15+ | TE-1, TE-4 |
| 阿里云 IaaS (ECS/ENI/ESSD/SLB) | 20+ | TE-1, TE-4, TE-5, TE-12 |
| 应用运行时 (Pod/Container/Init) | 30+ | TE-2, TE-3 |
| 网络 (DNS/Service/Ingress/Terway) | 40+ | TE-2, TE-4, TE-9 |
| 存储 (PVC/PV/CSI/OSS/NAS) | 25+ | TE-3, TE-5 |
| 安全 (证书/RBAC/Webhook/RAM) | 20+ | TE-7 |
| 可观测性 (Prometheus/Grafana/ARMS/MSP) | 25+ | TE-8 |
| 服务网格 (ASM/Istiod/Envoy) | 15+ | TE-2, TE-10 |
| 多集群 (ACK-One/Federation/GitOps) | 10+ | TE-11 |
| 变更管理 (升级/回滚/配置漂移) | 12+ | TE-13 |
| 容量/DR (备份/恢复/扩容) | 15+ | TE-14, TE-15 |

## 18.2 问题传播路径示例

```
路径1: etcd 磁盘满 → API Server 不可用 → 集群不可用
  BE-1.2.1 → IE-1.1 → BE-1.1 → TE-1

路径2: ENI 带宽瓶颈 → Pod 网络中断 → Service 不可用
  BE-1.8.1.1 → BE-4.4.1.1 → BE-2.5 → TE-2

路径3: JVM heap 泄漏 → OOMKilled → Service 不可用
  BE-2.3.1.1 → BE-2.3 → IE-2.1 → TE-2

路径4: Istiod OOM → xDS 推送失败 → Envoy 无法连接 → Service 不可用
  BE-10.3.1.1 → BE-10.3 → IE-10.2 → BE-10.1 → TE-2

路径5: VPC CIDR 耗尽 → Pod IP 分配失败 → Pod 无法启动
  BE-9.2.1.1 → BE-9.2 → IE-9.1 → TE-9
```

---

<!-- chunk: 十九、故障树元数据 -->## 十九、故障树元数据

## 19.1 版本信息

```yaml
fta_metadata:
  version: "2.0"
  last_updated: "2026-05-18"
  total_top_events: 16
  total_intermediate_events: 80+
  total_bottom_events: 300+
  ack_specific_events: 80+
  coverage:
    kubernetes_standard: 100%
    alibaba_cloud_enhanced: 95%
    multi_cluster: 80%
```

## 19.2 维护要求

```
更新触发条件:
  1. 新增 ACK 组件/服务
  2. 发现现有故障树未覆盖的故障模式
  3. 问题回溯发现新的根因路径
  4. 阿里云 IaaS 层变更（如新 ECS 实例类型）

更新流程:
  1. FTA 维护团队评审新问题路径
  2. 补充底事件定义（含 observable/diagnosis/healing）
  3. 更新概率数据
  4. 通知所有相关方
  5. 更新版本号
```

---

<!-- chunk: 二十、与现有 FTA 知识库的差异 -->## 二十、与现有 FTA 知识库的差异

| 对比项 | v1.0 (原有) | v2.0 (增强版) |
|:---|:---|:---|
| 顶事件数量 | 8 | 16 |
| 故障树层级 | 3-4 层 | 4-5 层 |
| 底事件总数 | ~90 | ~300+ |
| ACK 特有覆盖 | 无 | Terway/ASM/ARMS/ACK-One/IaaS |
| IaaS 层问题 | 无 | ECS/ENI/ESSD/SLB/VPC |
| 服务网格问题 | 无 | ASM/Istiod/Envoy/xDS |
| 多集群问题 | 无 | ACK-One/Federation/GitOps |
| 容量/DR 问题 | 无 | 备份/恢复/扩容 |
| 可观测性完整性 | 基础监控 | OTel/ARMS/MSP/链路追踪 |

---

> **文档版本**: v2.0 Enhanced
> **生成日期**: 2026-05-18
> **维护团队**: SRE Team / Platform Team
> **关联文档**: [ack-fta-generator-v2.md](./[[domain-10-troubleshooting-diagnostics/topic-fta/ack-fta-generator-v2.md|ack-fta-generator-v2]].md) | [fta-methodology-and-agentic-practices.md](./fta-methodology-and-agentic-practices.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-index.md|fta-index]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md|fta-methodology-and-agentic-practices]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis.md|kubernetes-fta-full-analysis]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/problem-solving-architecture.md|problem-solving-architecture]]
