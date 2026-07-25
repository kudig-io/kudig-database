---
title: K8GB
description: '## 概述'
summary: 'K8GB 是一个 Kubernetes 原生的全局负载均衡解决方案，基于 DNS 实现跨集群的流量调度。它使用 Kubernetes CRD (GslbStrategy) 定义全局负载均衡策略，通过 CoreDNS 和 ExternalDNS 实现多集群间的 DNS 基础的流量管理，支持轮询、地理位置和故障转移策略。'
category: entities
tags:
- k8s
- cncf
- networking
- k8gb
- coredns
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
- K8GB 是什么
- 如何 K8GB
trigger_keywords:
- K8GB
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# K8GB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

K8GB（Kubernetes Global Balancer）是一个 Kubernetes 原生的**全局负载均衡（Global Server Load Balancing, GSLB）**解决方案，2020 年进入 CNCF Sandbox。它基于 DNS 实现跨多个 Kubernetes 集群的流量调度，使用标准的 Kubernetes CRD（`GslbStrategy`）定义全局负载均衡策略。K8GB 通过 CoreDNS 和 ExternalDNS 在多集群间同步 DNS 记录，实现轮询（RoundRobin）、地理位置（GeoIP）和故障转移（Failover）策略。

K8GB 解决了多集群场景下的全局流量管理痛点：传统 GSLB 解决方案（F5 GTM、AWS Route53）昂贵且与 K8s 割裂，K8GB 提供完全开源、K8s 原生的替代方案。它与 Ingress Controller 配合工作，在 DNS 层面实现跨集群的流量分发和故障切换。

## Key Features

- **全局负载均衡**：跨多个 K8s 集群的 DNS 流量调度
- **多策略支持**：轮询（RoundRobin）、地理位置（GeoIP）、故障转移（Failover）
- **K8s 原生 CRD**：通过 `Gslb` CRD 声明式定义全局负载均衡策略
- **CoreDNS 集成**：使用 CoreDNS 的 DNATS 插件实现多集群 DNS 记录同步
- **健康检查**：基于 Ingress 后端健康状态自动调整 DNS 记录
- **脑裂保护**：通过 `splitBrainThresholdSeconds` 防止网络分区导致的误判

## Architecture

K8GB 由 **K8GB Operator**（管理 Gslb CRD，协调 DNS 记录）、**CoreDNS**（运行在每个集群中的 DNS 服务器，响应 GSLB 查询）、**Infoblox/ExternalDNS**（可选，同步 DNS 记录到外部 DNS 供应商）和 **DNATs 同步层**（跨集群 DNS 记录状态同步）组成。当客户端查询 `app.gslb.example.com` 时，CoreDNS 根据策略和健康状态返回最合适的集群 IP 地址。

## K8s 集成

K8GB 通过 `Gslb` CRD 扩展 Kubernetes API。一个 Gslb 资源关联一个标准的 Ingress，定义全局策略和参与集群的地理标签（geoTag）。Operator 监控关联 Ingress 的后端健康状态，健康/不健康自动更新 DNS 记录。CoreDNS 作为集群 DNS 的上游解析器，处理 GSLB 域名的智能应答。

## 生产部署要点

- **DNS 委托**：正确配置 DNS 委托，将 gslb 子域委托给 K8GB 管理的 CoreDNS
- **TTL 配置**：设置合理的 DNS TTL，平衡故障切换速度和 DNS 缓存效率
- **健康检查**：确保后端服务有适当的健康检查端点
- **脑裂保护**：配置 splitBrainThresholdSeconds 防止网络分区导致的脑裂
- **地理标签**：为每个集群配置准确的地理标签（geoTag）

## 生产场景

1. **多区域高可用**：应用部署在多区域集群，K8GB 自动将用户路由到最近/健康的区域
2. **故障自动转移**：主集群故障时，DNS 自动将流量切换到备用集群
3. **金丝雀多集群发布**：通过 DNS 权重逐步将流量迁移到新集群
4. **蓝绿多集群部署**：新旧集群同时运行，DNS 切换实现秒级流量切换

## 安装与配置

### Helm 部署

```bash
# 添加 Helm 仓库
helm repo add k8gb https://www.k8gb.io
helm repo update

# 安装 K8GB（主集群 us-east-1）
helm install k8gb k8gb/k8gb -n k8gb --create-namespace \
  --set k8gb.clusterGeoTag=us-east-1 \
  --set k8gb.extGslbClustersGeoTags=eu-west-1,ap-southeast-1 \
  --set k8gb.edgeDNSZone=gslb.example.com \
  --set coredns.enabled=true

# 验证部署
kubectl get pods -n k8gb
kubectl get crd gslbs.k8gb.absa.oss
```

### Gslb 资源配置

```yaml
apiVersion: k8gb.absa.oss/v1beta1
kind: Gslb
metadata:
  name: myapp-gslb
spec:
  ingress:
    rules:
      - host: myapp.gslb.example.com
        http:
          paths:
            - path: /
              pathType: Prefix
              backend:
                service:
                  name: myapp
                  port:
                    number: 80
  strategy:
    type: roundRobin
    splitBrainThresholdSeconds: 300
    dnsTtlSeconds: 30
    primaryGeoTag: us-east-1
---
# failover 策略示例
apiVersion: k8gb.absa.oss/v1beta1
kind: Gslb
metadata:
  name: critical-app-gslb
spec:
  ingress:
    rules:
      - host: critical.gslb.example.com
        http:
          paths:
            - path: /
              pathType: Prefix
              backend:
                service:
                  name: critical-app
                  port:
                    number: 443
  strategy:
    type: failover
    primaryGeoTag: us-east-1
    splitBrainThresholdSeconds: 180
```

## 运维操作

```bash
# 🟢 查看 GSLB 状态
kubectl get gslb -A
kubectl describe gslb myapp-gslb

# 🟢 检查 DNS 解析
kubectl exec -n k8gb deploy/coredns -- nslookup myapp.gslb.example.com 127.0.0.1

# 🟢 查看各集群健康状态
kubectl get gslb myapp-gslb -o jsonpath='{.status.healthyRecords}'

# 🟡 修改 GSLB 策略
kubectl patch gslb myapp-gslb --type merge -p '{"spec":{"strategy":{"type":"failover"}}}'

# 🟡 模拟故障转移（删除主集群 Pod）
kubectl scale deployment myapp --replicas=0

# 🔴 删除 Gslb 资源
kubectl delete gslb myapp-gslb
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| DNS 解析失败 | CoreDNS 未就绪 | `kubectl get pods -n k8gb -l app=coredns` | 检查 CoreDNS 配置和日志 |
| 故障转移未触发 | splitBrainThreshold 未到 | `kubectl describe gslb <name>` | 等待超时或调整阈值 |
| 跨集群同步失败 | 集群间网络不通 | `kubectl logs -n k8gb -l app=k8gb` | 检查集群间 DNS 和网络 |
| 流量未均衡 | 策略配置错误 | `kubectl get gslb -o yaml` | 检查 strategy.type 和 geoTag |
| 脑裂 | 多集群同时认为自己是主 | `kubectl get gslb -A -o wide` | 检查 splitBrainThreshold 和网络分区 |

**排查流程：**
```
GSLB 故障转移失败
├── 检查 Gslb CR 状态 → kubectl describe gslb <name>
├── 检查健康检查 → kubectl get endpoints <svc>
├── 检查 CoreDNS 记录 → nslookup <host> <coredns-ip>
├── 检查集群间通信 → kubectl logs -n k8gb
└── 检查 DNS TTL → 确认客户端未缓存旧记录
```

## 生产案例

### 案例一：多区域故障转移

- **场景**: 电商应用部署在 3 个区域（US/EU/AP），需要 DNS 层自动故障转移
- **排查**: 使用 K8GB failover 策略，主区域故障后 30s 内切换
- **方案**: 3 个集群各部署 K8GB，配置 failover 策略，主区域 US，故障时自动切换到 EU
- **效果**: RTO < 60s，无需人工介入，年度可用性从 99.9% 提升至 99.99%

### 案例二：全球流量均衡

- **场景**: SaaS 服务需要全球用户就近接入，降低延迟
- **排查**: 使用 K8GB roundRobin 策略，结合 GeoDNS 实现就近接入
- **方案**: 各区域 K8GB 互相感知健康状态，DNS 返回所有健康集群 IP
- **效果**: 全球用户平均延迟从 200ms 降至 50ms，单集群故障无感知

## 对比

| 特性 | K8GB | AWS Route53 | F5 GTM | GlobalNet (Submariner) | 适用场景 |
|------|------|-------------|--------|----------------------|----------|
| 开源 | ✅ | ❌ | ❌ | ✅ | - |
| K8s 原生 | ✅ | ❌ | ❌ | ✅ | K8GB 首选 |
| DNS 层 GSLB | ✅ | ✅ | ✅ | ❌ 网络层 | - |
| 成本 | 免费 | 按查询付费 | 高昂许可 | 免费 | - |
| 多集群感知 | ✅ | ❌ | ❌ | ✅ | - |

## 参考链接

- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]

## Related

- [[chaos-mesh]] — Chaos Mesh
- [[kubean]] — Kubean
- [[tikv]] — TiKV
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k8gb
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/dns-index.md|DNS 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
