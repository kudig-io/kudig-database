---
title: Dragonfly (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- dragonfly
- scheduler
- prometheus
- grafana
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dragonfly 是什么
- 如何 Dragonfly
trigger_keywords:
- Dragonfly
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Dragonfly

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Dragonfly（蜻蜓）是一个 CNCF 孵化项目，由阿里巴巴开源，是一个基于 P2P 技术的智能文件分发系统。它旨在解决大规模容器集群中镜像分发和文件下载的带宽瓶颈问题。在数千节点的集群中，传统的镜像拉取方式会导致 Registry 带宽被打满，Dragonfly 通过 P2P 协议让节点间互相分享数据，将带宽消耗从集中式变为分布式，显著提升大规模部署的效率。

## Key Features（核心能力）

- **P2P 文件分发**：通过 P2P 协议将文件分发负载分散到所有节点
- **镜像预热**：支持在部署前预热镜像到所有节点，加速 Pod 启动
- **多源支持**：支持从 Registry、HTTP、NAS 等多种数据源分发文件
- **智能限速**：支持基于主机级别的速率限制，避免影响业务流量
- **主机级缓存**：通过本地缓存避免重复下载相同文件
- **安全传输**：支持 TLS 加密和镜像签名验证

## 架构与工作原理

Dragonfly v2 架构包含三个核心组件：Scheduler（调度器）负责 P2P 网络的节点管理和调度决策；Seed Peer（种子节点）作为 P2P 网络中的数据源，从 Registry 拉取数据并分发给其他 Peer；Dfdaemon（守护进程）作为 DaemonSet 运行在每个节点，拦截镜像拉取请求并利用 P2P 网络加速下载。通过 Manager 组件提供统一的管理控制台。

## K8s 集成

Dragonfly 通过 Dfget 代理拦截 containerd/docker 的镜像拉取请求。在 K8s 中以 DaemonSet 方式部署 dfdaemon，配置 containerd 使用 Dragonfly 作为镜像代理。Dragonfly 支持 K8s 原生的 Pod 安全策略和 RBAC。Manager 组件通过 Deployment 部署，提供 Web UI 和 API 管理界面。

## 生产用例

- **大规模镜像分发**：数千节点集群的镜像拉取加速，避免 Registry 带宽瓶颈
- **边缘计算节点更新**：在带宽受限的边缘场景高效分发镜像
- **CI/CD 并发部署**：大规模并行构建和部署的镜像拉取加速
- **软件包分发**：大规模集群的软件包和配置文件分发

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add dragonfly https://dragonflyoss.github.io/helm-charts
helm install dragonfly dragonfly/dragonfly \
  -n dragonfly-system --create-namespace \
  --set scheduler.replicas=3 \
  --set seedPeer.replicas=3

# 🟢 验证安装
kubectl get pods -n dragonfly-system
kubectl get crd | grep dragonfly

# 🟢 配置 containerd 使用 Dragonfly 代理
# 修改 /etc/containerd/config.toml
# [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
#   endpoint = ["http://127.0.0.1:65001"]

# 🟡 重启 containerd
systemctl restart containerd

# 🟢 验证 P2P 分发
crictl pull docker.io/library/nginx:latest
kubectl logs -n dragonfly-system -l app=dfdaemon --tail=20
```

### 组件配置示例

```yaml
# Dragonfly Manager 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: dragonfly-manager
  namespace: dragonfly-system
data:
  manager.yaml: |
    server:
      port: 8080
    database:
      type: postgres
      host: postgres.dragonfly-system.svc
      port: 5432
      dbname: dragonfly
---
# Scheduler 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: dragonfly-scheduler
  namespace: dragonfly-system
data:
  scheduler.yaml: |
    server:
      port: 8002
    scheduler:
      algorithm: default
      backToSourceCount: 3
    host:
      idc: idc-1
      netTopology: nt-1
```

## 运维操作

### 常用命令

```bash
# 🟢 查看组件状态
kubectl get pods -n dragonfly-system
kubectl get pods -n dragonfly-system -l app=dfdaemon -o wide

# 🟢 查看 Scheduler 日志
kubectl logs -n dragonfly-system -l app=scheduler --tail=50

# 🟢 查看 Seed Peer 日志
kubectl logs -n dragonfly-system -l app=seed-peer --tail=50

# 🟢 查看 Dfdaemon 日志
kubectl logs -n dragonfly-system -l app=dfdaemon --tail=50

# 🟢 查看 P2P 任务状态 (Manager API)
curl http://dragonfly-manager.dragonfly-system.svc:8080/api/v1/tasks

# 🟢 查看 Peer 状态
curl http://dragonfly-manager.dragonfly-system.svc:8080/api/v1/peers

# 🟡 预热镜像 (Preheat)
curl -X POST http://dragonfly-manager.dragonfly-system.svc:8080/api/v1/preheats \
  -H 'Content-Type: application/json' \
  -d '{"type":"image","url":"docker.io/library/nginx:latest"}'

# 🟡 重启 Dfdaemon
kubectl rollout restart daemonset/dfdaemon -n dragonfly-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 镜像拉取失败 | Dfdaemon 未就绪/配置错误 | `kubectl logs -l app=dfdaemon` | 检查 containerd mirror 配置 |
| P2P 未生效 | Scheduler 不可达 | `kubectl logs -l app=scheduler` | 检查 Scheduler Service 和网络 |
| 下载速度慢 | Seed Peer 带宽不足 | 查看 Manager 任务状态 | 增加 Seed Peer 副本/调整限速 |
| 节点未加入 P2P | Dfdaemon 未运行 | `kubectl get pods -l app=dfdaemon -o wide` | 检查 DaemonSet 调度 |
| 缓存未命中 | 磁盘空间不足 | `df -h /var/lib/dragonfly` | 清理缓存或扩展磁盘 |

### 排查流程

```
1. kubectl get pods -n dragonfly-system → 确认组件状态
2. kubectl logs -l app=dfdaemon → 查看节点代理日志
3. kubectl logs -l app=scheduler → 查看调度决策
4. 检查 containerd mirror 配置指向 127.0.0.1:65001
5. 验证 Seed Peer 可从 Registry 拉取数据
```

## 生产案例

### 案例1: 5000节点集群镜像分发
- **场景**: 5000节点集群同时拉取新镜像，Registry 带宽打满
- **方案**: 部署 Dragonfly P2P 分发，配置 10 个 Seed Peer
- **效果**: Registry 带宽降低 95%，镜像拉取时间从 5min 降至 30s

### 案例2: 边缘节点镜像更新
- **场景**: 100+ 边缘节点带宽受限 (10Mbps)，镜像更新耗时过长
- **方案**: 边缘部署 Dragonfly，节点间 P2P 共享镜像层
- **效果**: 镜像更新时间从 20min 降至 2min

## 对比替代方案

| 维度 | Dragonfly | Kraken | Registry Mirror | 直接拉取 |
|------|-----------|--------|-----------------|----------|
| P2P 分发 | 支持 | 支持 | 不支持 | 不支持 |
| CNCF 状态 | Incubating | 非 CNCF | N/A | N/A |
| 镜像预热 | 支持 | 不支持 | 不支持 | 不支持 |
| 多源支持 | Registry/HTTP/NAS | 仅 Registry | 仅 Registry | 仅 Registry |
| 社区活跃度 | 高 | 低 | N/A | N/A |
| 大规模验证 | 万级节点 | 千级节点 | 百级节点 | 百级节点 |

## 检查清单

- [ ] Scheduler 副本数 >= 3 (HA)
- [ ] Seed Peer 副本数充足 (建议每 IDC 至少 3个)
- [ ] Dfdaemon DaemonSet 在所有节点运行
- [ ] containerd mirror 配置正确指向 Dfdaemon
- [ ] 磁盘缓存空间充足
- [ ] 配置了合理的限速 (避免影响业务)
- [ ] 监控 P2P 任务成功率和延迟
- [ ] 定期清理过期缓存

## Related

- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[strimzi]] — Strimzi
- [[hwameistor]] — HwameiStor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- dragonfly
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
