---
title: Sermant (entities)
description: '## 概述'
summary: 'Sermant 是华为开源的基于 Java Agent 的无代理服务网格方案，通过 Java Instrumentation 机制（字节码增强）为 Java 微服务提供服务治理能力，无需修改应用代码或部署 Sidecar 代理。它支持流量路由、限流熔断、负载均衡、服务注册发现等功能，特别适合 Java 技术栈的微服务架构。'
category: entities
tags:
- k8s
- cncf
- service-mesh
- sermant
- prometheus
- grafana
- istio
- cilium
- opa
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Sermant 是什么
- 如何 Sermant
trigger_keywords:
- Sermant
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Sermant

> **CNCF 状态**: Sandbox | **类别**: [[service|Service]]Service Mesh）|Service Mesh]] | **主要语言**: Java

## 概述

Sermant 是华为开源的基于 Java Agent 的无代理（Agentless）服务网格方案，2022 年加入 CNCF Sandbox。它通过 Java Instrumentation 机制（字节码增强）为 Java 微服务提供服务治理能力，无需修改应用代码或部署 Sidecar 代理。Sermant 支持流量路由、限流熔断、负载均衡、服务注册发现等功能，特别适合 Java 技术栈的微服务架构。它是从 ServiceComb 生态演进而来的轻量级治理方案。

## 核心特性

- **字节码增强**: 无侵入式增强 Java 应用，无需修改业务代码
- **无 Sidecar**: 直接在 JVM 内运行，消除 Sidecar 代理开销
- **插件化架构**: 按需加载治理插件（路由、限流、监控等）
- **配置热更新**: 通过 Sermant Backend 动态调整治理策略，无需重启
- **多框架支持**: Spring Cloud、Dubbo、gRPC 等 Java 微服务框架
- **监控集成**: 上报治理指标到 Prometheus，追踪数据到 Zipkin

## 架构

Sermant 由三部分组成。Agent（sermant-agent）通过 `-javaagent` 参数挂载到 Java 应用 JVM 中，利用 Java Instrumentation API 在类加载时增强字节码。框架核心（sermant-framework）提供插件加载、字节码增强、配置管理和服务治理框架。Backend（sermant-backend）是控制面，提供配置管理、心跳监控和治理策略下发。插件（sermant-plugins）按功能组织，如路由插件增强 HTTP 客户端实现流量路由，限流插件增强服务入口实现 QPS 控制。

## Kubernetes 集成

在 Kubernetes 中，Sermant Agent 通过 Init Container 或镜像构建阶段注入到 Java 应用 Pod。无需 Sidecar 代理容器，降低资源消耗约 30-50%。服务注册发现通过 Sermant 插件连接注册中心（如 ZooKeeper、Nacos）。Backend 以 Deployment 形式部署在集群中，通过 ConfigMap 或后端服务下发治理策略。与 Kubernetes Service 和 Endpoint 配合实现负载均衡。

## 生产使用场景

1. **Java 微服务迁移**: 从传统微服务框架迁移到云原生架构，保持治理能力
2. **Sidecar 替代**: 在 Sidecar 代理性能开销不可接受时，使用 Sermant 降低开销
3. **灰度发布**: 通过路由插件实现 Java 服务的标签路由和金丝雀发布
4. **限流降级**: 在服务入口通过限流插件实现 QPS 控制和熔断降级

## 安装与配置

```bash
# 下载 Sermant Agent
wget https://github.com/huaweicloud/Sermant/releases/latest/sermant-agent.zip
unzip sermant-agent.zip
# 挂载到 Java 应用
java -javaagent:/path/to/sermant-agent/agent/sermant-agent.jar \
  -Dsermant.plugins=flowcontrol,router,service-registry \
  -jar application.jar
# 部署 Backend
kubectl apply -f https://raw.githubusercontent.com/huaweicloud/Sermant/main/sermant-backend/deploy/kubernetes.yaml
```

### Kubernetes 部署示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: java-app
spec:
  template:
    spec:
      initContainers:
        - name: sermant-agent
          image: ghcr.io/huaweicloud/sermant-agent:latest
          volumeMounts:
            - name: sermant
              mountPath: /opt/sermant
      containers:
        - name: app
          image: my-java-app:latest
          env:
            - name: JAVA_OPTS
              value: "-javaagent:/opt/sermant/agent/sermant-agent.jar"
            - name: SERMANT_PLUGINS
              value: "flowcontrol,router,monitor"
          volumeMounts:
            - name: sermant
              mountPath: /opt/sermant
      volumes:
        - name: sermant
          emptyDir: {}
```

### 治理策略配置

```yaml
# 限流规则示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: sermant-flowcontrol
data:
  flowcontrol.yaml: |
    rules:
      - name: order-service-qps
        target: order-service
        type: qps
        threshold: 1000
        action: reject
      - name: payment-circuit-breaker
        target: payment-service
        type: circuit-breaker
        errorRate: 50
        windowSize: 10s
        sleepTime: 30s
```

## 运维操作

```bash
# 🟢 查看 Sermant Backend 状态
kubectl get pods -l app=sermant-backend

# 🟢 查看已注册服务
kubectl exec deploy/sermant-backend -- curl -s localhost:8900/api/v1/services

# 🟡 更新治理策略（热更新）
kubectl apply -f flowcontrol-configmap.yaml

# 🟢 查看应用治理指标
kubectl exec deploy/java-app -- curl -s localhost:12345/metrics | grep sermant

# 🟢 检查 Agent 加载状态
kubectl logs deploy/java-app | grep -i sermant

# 🟡 动态启用/禁用插件
kubectl exec deploy/sermant-backend -- curl -X POST \
  localhost:8900/api/v1/plugins/router/enable
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Agent 未加载 | javaagent 路径错误 | `kubectl logs deploy/java-app \| grep sermant` | 检查 -javaagent 参数和文件路径 |
| 插件未生效 | 插件未启用 | 检查 SERMANT_PLUGINS 环境变量 | 确认插件名称正确 |
| 配置未更新 | Backend 连接失败 | `kubectl logs deploy/sermant-backend` | 检查 Backend Service 地址 |
| 性能下降 | 字节码增强冲突 | 检查 JVM 日志 | 排除冲突的 Java Agent |
| 服务注册失败 | 注册中心不可用 | 检查 ZooKeeper/Nacos 连接 | 确认注册中心地址配置 |

### 排查流程

```
Sermant 异常
├─ Agent 未加载？
│  ├─ 文件不存在 → 检查 Init Container 挂载
│  ├─ 路径错误 → 检查 -javaagent 参数
│  └─ JVM 版本不兼容 → 确认 Java 8+
├─ 治理不生效？
│  ├─ 插件未启用 → 检查 SERMANT_PLUGINS
│  ├─ 规则未下发 → 检查 Backend 连接
│  └─ 框架不支持 → 确认 Spring Cloud/Dubbo 版本
└─ 性能问题？
   ├─ 增强冲突 → 排除其他 Java Agent
   └─ 规则过多 → 精简治理规则
```

## 生产案例

### 案例 1: 大型银行 Java 微服务治理

**场景**: 某银行 200+ Java 微服务需服务治理能力，但无法接受 Sidecar 代理的性能开销。

**方案**:
1. 使用 Sermant Agent 替代 Istio Sidecar
2. 通过 Init Container 注入 Agent
3. 配置限流熔断和灰度路由策略
4. 监控指标上报 Prometheus

**效果**: 延迟增加 < 2ms（Sidecar 方案 10-20ms），资源消耗减少 40%。

### 案例 2: 传统微服务云原生迁移

**场景**: 企业 Spring Cloud 微服务迁移到 K8s，需保持原有治理能力。

**方案**:
1. Sermant 接管服务注册发现（替代 Eureka）
2. 路由插件实现灰度发布
3. 限流插件实现 QPS 控制
4. 无需修改业务代码

**效果**: 迁移周期从 6 个月缩短到 2 个月，业务代码零修改。

## 对比与替代方案

| 维度 | Sermant | Istio Sidecar | Spring Cloud | Kmesh |
|------|---------|---------------|--------------|-------|
| 语言支持 | 仅 Java | 语言无关 | 仅 Java | 语言无关 |
| 侵入性 | 无（Agent） | 无（Sidecar） | 高（SDK） | 无（eBPF） |
| 性能开销 | 极低 | 中 | 低 | 极低 |
| 资源消耗 | 低 | 高 | 低 | 低 |
| 治理能力 | 全面 | 全面 | 全面 | 基础 |
| 成熟度 | 中 | 高 | 高 | 低 |

## 检查清单

- [ ] Java 版本 >= 8（Agent 兼容性）
- [ ] Sermant Agent 通过 Init Container 注入
- [ ] 所需插件已启用（flowcontrol/router/monitor）
- [ ] Backend 已部署并可访问
- [ ] 治理规则已通过 ConfigMap 配置
- [ ] 监控指标已对接 Prometheus
- [ ] 与其他 Java Agent 无冲突
- [ ] 灰度路由策略已测试验证

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Sermant** | 无 Sidecar、低开销 | 仅支持 Java |
| Istio Sidecar | 语言无关、功能全面 | 资源开销大、延迟增加 |
| Spring Cloud | Java 原生、成熟 | 需修改代码、耦合 SDK |
| Dubbo | 高性能 RPC | 需引入 Dubbo 框架 |

## 架构定位

在 CNCF 生态中，Sermant 属于 **Service Mesh** 类别，代表了 Sidecar-less 服务网格的发展方向。它与 Istio、Kmesh 等项目互补，专注于 Java 场景。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[deployment]]
- networking.md|cilium-ebpf-networking]]

## Related

- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[serverless-devs]] — Serverless Devs
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- sermant
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
