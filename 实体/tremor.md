---
title: Tremor [entities]
description: '## 概述'
summary: 'Tremor 是一个高性能的事件处理引擎，专为处理大规模数据流（日志、指标、追踪数据）而设计。它由 Wayfair 开源，用 Rust 实现，通过自定义的查询语言（Troy/Trickle）定义数据管道，支持背压处理、有保证的交付和复杂事件处理。'
category: entities
tags:
- k8s
- cncf
- streaming
- tremor
- argocd
- elasticsearch
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tremor 是什么
- 如何 Tremor
trigger_keywords:
- Tremor
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tremor

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Rust

## 概述

Tremor 是一个 CNCF 沙箱项目，由 Wayfair 创建，是一个用 Rust 编写的高性能事件处理和路由引擎。它旨在替代 Logstash/Fluentd 等传统事件处理工具，提供更高的吞吐量和更低的资源消耗。Tremor 支持事件源（Source）、处理流水线（Pipeline）和输出目标（Sink）的声明式定义，特别适合日志处理、指标富化、事件路由和流式 ETL 场景。项目完全用 Rust 实现，具有内存安全和零成本抽象优势。

## Key Features（核心能力）

- **Rust 高性能**：基于 Rust 实现，吞吐量是 Logstash 的 10 倍以上
- **声明式流水线**：通过 Tremor Script 和 Trickle SQL 定义事件处理逻辑
- **多协议支持**：支持 Kafka、HTTP、gRPC、Syslog、File、NATS 等
- **Tremor Script**：专用脚本语言，支持事件过滤、变换和路由
- **Trickle SQL**：基于 SQL 的流式查询语言，支持窗口聚合和 JOIN
- **QoS 控制**：内置背压、断路器、重试等质量保障机制

## 架构与工作原理

Tremor 架构采用 Source-Pipeline-Sink 模型：Source 从数据源（Kafka、HTTP、Syslog）接收事件；Pipeline 通过 Tremor Script 或 Trickle SQL 对事件进行过滤、变换、聚合和路由；Sink 将处理结果发送到目标系统（Elasticsearch、S3、Kafka）。所有组件通过声明式 YAML 配置连接。Tremor 运行时使用异步事件循环和零拷贝设计实现高吞吐低延迟。

## K8s 集成

Tremor 可作为 DaemonSet 或 Deployment 部署到 Kubernetes。在日志处理场景中，以 DaemonSet 形式运行在每个节点，接收 Fluent Bit 转发的日志，进行富化后发送到 Elasticsearch。通过 ConfigMap 管理处理流水线配置。与 K8s 的集成包括：消费 K8s Events 做告警处理、聚合 Pod 指标做实时分析。

## 生产用例

- **高性能日志处理**：替代 Logstash/Fluentd 做日志聚合和富化
- **实时指标管道**：从 Prometheus 指标流中提取异常并告警
- **事件路由**：根据事件内容将数据路由到不同的下游系统
- **流式 ETL**：实时数据清洗和格式转换

## 安装与配置

```bash
# 🟢 Docker 部署
docker run -d \
  -v $(pwd)/tremor:/etc/tremor:ro \
  -p 9898:9898 \
  tremorproject/tremor

# 🟢 Helm 部署到 K8s
helm repo add tremor https://tremor-project.github.io/tremor-helm/
helm repo update
helm install tremor tremor/tremor \
  -n logging --create-namespace \
  --set replicaCount=3 \
  --set resources.limits.memory=2Gi

# 🟢 验证安装
kubectl get pods -n logging
kubectl logs -n logging -l app=tremor --tail=20

# 🟢 本地安装 CLI
curl -LO https://github.com/tremor-rs/tremor-runtime/releases/latest/download/tremor-linux-amd64
chmod +x tremor-linux-amd64 && mv tremor-linux-amd64 /usr/local/bin/tremor
```

### 事件处理流水线配置

```yaml
# /etc/tremor/main.troy
define flow main
flow
    use std;
    use integration;

    # 定义 Source：接收 Syslog
    define source syslog_in from syslog
    with
        config = {
            "codec": "syslog",
            "host": "0.0.0.0",
            "port": 514
        }
    end;

    # 定义 Pipeline：日志富化和过滤
    define pipeline log_enrichment
    pipeline
        use std::string;
        use std::time;

        # 过滤调试日志
        select event from in where event.severity != "debug" into out;
    script
        # 添加时间戳和集群信息
        let event.processed_at = time::format::rfc3339(time::now());
        let event.cluster = "prod-east";
        let event.severity_upper = string::uppercase(event.severity);
        event
    end;

    # 定义 Sink：发送到 Elasticsearch
    define sink es_out from elastic
    with
        config = {
            "url": "http://elasticsearch:9200",
            "index": "logs-%{+YYYY.MM.dd}"
        }
    end;

    # 连接组件
    connect /source/syslog_in to /pipeline/log_enrichment;
    connect /pipeline/log_enrichment to /sink/es_out;
end;
```

### K8s DaemonSet 部署

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: tremor-log-processor
  namespace: logging
spec:
  selector:
    matchLabels:
      app: tremor
  template:
    metadata:
      labels:
        app: tremor
    spec:
      containers:
        - name: tremor
          image: tremorproject/tremor:latest
          ports:
            - containerPort: 9898
              name: api
          volumeMounts:
            - name: config
              mountPath: /etc/tremor
            - name: varlog
              mountPath: /var/log
              readOnly: true
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 2Gi
      volumes:
        - name: config
          configMap:
            name: tremor-config
        - name: varlog
          hostPath:
            path: /var/log
```

## 运维操作

```bash
# 🟢 查看 Tremor 状态
kubectl get pods -n logging -l app=tremor
kubectl logs -n logging -l app=tremor --tail=50

# 🟢 查看处理指标
curl -s http://tremor:9898/version | jq .
curl -s http://tremor:9898/flow | jq .

# 🟡 重新加载配置
kubectl rollout restart daemonset/tremor-log-processor -n logging

# 🟡 更新流水线配置
kubectl edit configmap tremor-config -n logging
kubectl rollout restart daemonset/tremor -n logging

# 🔴 停止 Tremor（会丢失未处理的事件）
kubectl delete daemonset tremor -n logging
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 事件丢失 | 背压触发丢弃 | 查看 Tremor 指标 | 增加 Sink 并发或缓冲 |
| 处理延迟高 | Pipeline 逻辑复杂 | 查看处理时间指标 | 简化 Script 或增加副本 |
| Sink 连接失败 | 目标不可达 | 查看错误日志 | 检查目标服务状态 |
| 内存 OOM | 缓冲区过大 | `kubectl describe pod` | 调整缓冲区大小和 limits |

```bash
# 排查流程
# 1. 检查 Tremor Pod 状态
kubectl get pods -n logging -l app=tremor
kubectl top pods -n logging -l app=tremor

# 2. 检查处理日志
kubectl logs -n logging -l app=tremor --tail=100 | grep -i error

# 3. 检查流水线状态
curl -s http://tremor:9898/flow/main | jq .

# 4. 检查背压状态
curl -s http://tremor:9898/metrics | grep backpressure
```

## 生产案例

### 案例1：高性能日志处理平台
- **场景**：电商平台每日 50GB 日志，Logstash 资源消耗过高
- **方案**：Tremor 替代 Logstash；DaemonSet 部署每节点；Syslog Source + 富化 Pipeline + ES Sink
- **效果**：资源消耗降低 80%，吞吐量提升 10x，日志延迟 < 1s

### 案例2：实时异常检测管道
- **场景**：需要从指标流中实时检测异常并触发告警
- **方案**：Tremor + Trickle SQL 窗口聚合；滑动窗口计算 P99 延迟；超阈值事件路由到告警系统
- **效果**：异常检测延迟从 5min 缩短到 10s，误报率降低 50%

## 对比替代方案

| 维度 | Tremor | Fluentd | Logstash | Vector |
|------|--------|---------|----------|--------|
| 语言 | Rust | Ruby | JRuby | Rust |
| 吐吐量 | 极高 | 中 | 低 | 高 |
| 内存占用 | 低 | 高 | 极高 | 低 |
| CEP 能力 | 强 | 弱 | 中 | 弱 |
| 学习曲线 | 高 | 中 | 中 | 低 |

## 检查清单

- [ ] Tremor 已部署且 Pod Running
- [ ] 流水线配置已验证（测试环境）
- [ ] Source 连接已验证（数据源可达）
- [ ] Sink 连接已验证（目标可达）
- [ ] 资源限制已配置（CPU/内存）
- [ ] 背压和重试策略已配置
- [ ] 监控指标已接入 Prometheus

## Related

- [[kaito]] — KAITO
- [[youki]] — youki
- [[easegress]] — Easegress
- [[perses]] — Perses
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tremor
- [[实体/drasi.md|[[Drasi|Drasi]]]]
- observability|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
