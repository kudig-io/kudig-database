---
title: ChaosBlade [entities]
description: '## 概述'
summary: 'ChaosBlade 是阿里巴巴开源的混沌工程实验工具，用于模拟各种问题场景以测试系统的韧性。'
category: entities
tags:
- k8s
- cncf
- chaos
- chaosblade
- containerd
- docker
- mysql
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ChaosBlade 是什么
- 如何 ChaosBlade
trigger_keywords:
- ChaosBlade
prerequisites:
- kubectl-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# ChaosBlade

> **CNCF 状态**: Sandbox | **类别**: Chaos | **主要语言**: Go, Java

## 概述

ChaosBlade 是阿里巴巴开源的混沌工程实验工具，2019 年加入 CNCF 沙箱。它用于模拟各种问题场景以测试系统的韧性，支持对主机、容器、Kubernetes 和各种中间件（Dubbo、RocketMQ、MySQL、Redis）进行故障注入。ChaosBlade 提供统一的 CLI 和 Kubernetes Operator 两种使用方式，将混沌实验的创建、执行和销毁标准化。其设计理念是"遵循混沌工程实验原则，结合混沌工程模型，提供简单易用、功能强大的混沌实验实施工具"。ChaosBlade 在阿里巴巴内部经过多年双十一验证，覆盖了网络延迟/丢包、CPU/内存飙高、磁盘满、进程崩溃、DNS 异常等数百种故障场景。

## 核心能力

- **多平台支持**: 主机（物理机/虚拟机）、Docker 容器、Kubernetes 集群
- **丰富场景**: CPU 飙高、内存 OOM、网络延迟/丢包/分区、磁盘满/IO 高、进程杀死
- **中间件故障**: Java 应用（Dubbo、JVM）、RocketMQ、MySQL、Redis、Nginx 等
- **Kubernetes 原生**: Operator 模式，CRD 声明式实验（ChaosBlade CRD）
- **安全机制**: 实验自动恢复（timeout）、UID 标识实验便于追踪和销毁
- **统一 CLI**: `blade` 命令行工具，一致的实验创建/销毁接口

## 架构

ChaosBlade 采用 CLI + Agent + Operator 架构：

- **blade CLI**: 统一的命令行工具，支持 create/destroy/status 命令
- **blade-server**: 服务端守护进程，管理主机上的实验执行
- **ChaosBlade Operator**: Kubernetes Operator，解析 ChaosBlade CRD 并调度实验
- **ChaosBlade CRD**: 声明式混沌实验定义（实验类型、目标、参数）
- **Chaos Daemon**: 部署在目标节点上的 DaemonSet，实际执行故障注入
- **Experiment Model**: 标准化实验模型（Scope → Target → Action → Matcher）

实验流程：`CRD/CLI → Operator/Server → Daemon/blade → 执行故障注入 → 系统响应 → 销毁`

## K8s 集成

ChaosBlade 通过 ChaosBlade Operator 实现 Kubernetes 原生集成。用户创建 ChaosBlade CRD 定义故障实验（如"对某 Pod 注入 100ms 网络延迟"），Operator 解析 CRD 并创建 Chaos Daemon Job。Chaos Daemon 在目标 Pod 所在节点上执行故障注入（如使用 tc/netem 配置网络延迟）。实验通过 UID 追踪，支持通过 `kubectl delete chaosblade` 或设置 timeout 自动销毁。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准资源调度机制兼容——可以通过 namespaceSelector、labelSelector 精确控制故障注入范围。

## 生产场景

1. **高可用验证**: 注入网络分区/节点故障，验证集群自动恢复和故障转移能力
2. **中间件韧性测试**: 注入 MySQL/Redis 延迟或错误，验证应用的降级和重试逻辑
3. **微服务容错**: 注入 Dubbo/HTTP 调用超时，验证熔断器和服务降级
4. **演练前置验证**: 在生产环境混沌实验前，先在测试环境验证故障注入效果和影响

## 安装与配置

```bash
# 安装 ChaosBlade CLI
wget https://github.com/chaosblade-io/chaosblade/releases/download/v1.7.0/chaosblade-1.7.0-linux-amd64.tar.gz
tar -xzf chaosblade-*.tar.gz && cd chaosblade-1.7.0
./blade version

# 在 Kubernetes 中安装 Operator
kubectl apply -f https://github.com/chaosblade-io/chaosblade-operator/releases/download/v1.7.0/chaosblade-operator-v1.7.0.yaml
kubectl get pods -n chaosblade
```

### 混沌实验 CRD 配置

```yaml
# 网络延迟实验
apiVersion: chaosblade.io/v1alpha1
kind: ChaosBlade
metadata:
  name: network-delay-pod
spec:
  experiments:
  - scope: container
    target: network
    action: delay
    desc: "inject 100ms network delay to target pod"
    matchers:
    - name: time
      value: ["100"]
    - name: interface
      value: ["eth0"]
    - name: names
      value: ["my-app-pod"]
    - name: namespace
      value: ["production"]
---
# CPU 满载实验
apiVersion: chaosblade.io/v1alpha1
kind: ChaosBlade
metadata:
  name: cpu-fullload-node
spec:
  experiments:
  - scope: node
    target: cpu
    action: fullload
    desc: "node CPU fullload for 60s"
    matchers:
    - name: cpu-percent
      value: ["80"]
    - name: time
      value: ["60"]
    - name: names
      value: ["worker-node-01"]
```

### CLI 直接执行

```bash
# 创建网络丢包实验
./blade create network loss --percent 30 --interface eth0

# 查看实验状态
./blade status <experiment-id>

# 销毁实验
./blade destroy <experiment-id>
```

## 运维操作

```bash
# 🟢 查看实验状态
kubectl get chaosblade -A
kubectl describe chaosblade network-delay-pod

# 🟢 查看 Operator 日志
kubectl logs -n chaosblade -l app=chaosblade-operator --tail=50

# 🟡 创建混沌实验（影响目标 Pod/Node）
kubectl apply -f experiment.yaml

# 🟡 销毁实验（恢复故障）
kubectl delete chaosblade network-delay-pod

# 🔴 批量销毁所有实验
kubectl delete chaosblade --all

# 🔴 强制清理（实验残留）
kubectl patch chaosblade <name> -p '{"metadata":{"finalizers":null}}' --type=merge
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 实验未执行 | Operator 未就绪 | `kubectl get pods -n chaosblade` | 检查 Operator 状态 |
| Pod 未被注入 | matcher 不匹配 | `kubectl get pod -l app=my-app` | 确认 Pod 名称/命名空间 |
| 实验无法销毁 | finalizer 阻塞 | `kubectl get chaosblade -o yaml` | 移除 finalizer |
| 节点实验失败 | blade agent 未安装 | `kubectl get ds -n chaosblade` | 检查 DaemonSet 状态 |
| 网络实验无效 | 接口名错误 | `ip link show` | 确认正确的网络接口名 |

```
排查流程:
├── 实验未触发
│   ├── kubectl get chaosblade → 检查 Phase 状态
│   ├── kubectl describe chaosblade → 查看 Events
│   └── kubectl logs operator → 查看执行错误
├── 实验无法恢复
│   ├── kubectl delete chaosblade <name>
│   └── 手动清理: 进入 Pod 执行 ./blade destroy <id>
└── 节点级实验异常
    ├── 检查 chaosblade-tool DaemonSet 状态
    └── 确认目标节点上 blade agent 进程存活
```

## 生产案例

### 案例 1: 混沌实验导致生产事故

- **场景**: 测试环境网络延迟实验误匹配生产 Pod
- **排查**: matcher 中 namespace 未限制，匹配了所有同名 Pod
- **方案**: 立即 `kubectl delete chaosblade --all`；后续强制要求所有实验指定 namespace matcher；添加准入控制禁止在 production ns 创建 ChaosBlade CR
- **效果**: 建立混沌实验安全规范，生产环境零误注入

### 案例 2: 微服务韧性验证

- **场景**: 验证订单服务在下游支付服务 500ms 延迟下的表现
- **方案**: 创建 network delay 实验，matcher 指定支付服务 Pod；观察订单服务超时/重试/降级行为
- **效果**: 发现订单服务超时设置过短(200ms)，调整为 1s + 重试 + 降级，系统韧性显著提升

## 对比

| 特性 | ChaosBlade | Chaos Mesh | LitmusChaos | Krkn | 适用场景 |
|------|-----------|------------|-------------|------|----------|
| 中间件故障 | ✅ 丰富 | ⚠️ 有限 | ⚠️ | ⚠️ | Java/微服务 |
| K8s 原生 | ✅ CRD | ✅ CRD | ✅ CRD | ⚠️ | 云原生 |
| CLI | ✅ blade | ✡ chaos | ✡ litmusctl | ⚠️ | 命令行操作 |
| 节点级实验 | ✅ | ✅ | ✅ | ✅ | 基础设施 |
| CNCF 状态 | Sandbox | Incubating | Incubating | 非 CNCF | 生态成熟度 |

## 架构定位

在 CNCF 生态中，ChaosBlade 属于 **Chaos** 类别，为云原生应用提供全场景混沌工程能力。

## 参考链接

- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[pod-lifecycle]]

## Related

- [[kcp]] — kcp
- [[23-实体/15-参考与索引/cncf-security.md|cncf-security]] — CNCF 安全与合规项目全景
- [[07-containerd-disaster-recovery]] — containerd 灾难恢复
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- chaosblade
- [[23-实体/14-可靠性与混沌/krkn.md|Krkn]]
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
