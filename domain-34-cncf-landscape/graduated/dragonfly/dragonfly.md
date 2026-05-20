---
title: Dragonfly
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- scheduler
- prometheus
- grafana
- helm
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Dragonfly 是什么
- 如何 Dragonfly
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Dragonfly
- cncf
- landscape
---


# Dragonfly

> **成熟度**: Graduated | **加入时间**: 2018-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://d7y.io |
| **GitHub** | https://github.com/dragonflyoss/Dragonfly2 |
| **文档** | https://d7y.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Dragonfly 是基于 P2P 技术的智能镜像和文件分发系统，由阿里巴巴开源并捐赠给 CNCF。它通过对等网络加速大规模文件分发，显著降低带宽成本和分发时间。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | 阿里巴巴内部使用 |
| 2018-11 | Dragonfly 1.x 加入 CNCF Sandbox |
| 2020-04 | 晋升为 CNCF Incubating |
| 2021-04 | Dragonfly2 重写发布 |
| 2023-04 | 晋升为 CNCF Graduated |

### 核心定位
Dragonfly 是企业级的镜像分发加速方案，通过 P2P 技术解决大规模集群镜像拉取的带宽瓶颈问题，特别适合 Kubernetes 集群的容器镜像分发场景。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    Dragonfly 架构                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│    ┌───────────────────────────────────────────────────────┐    │
│    │                     Manager                            │    │
│    │  • 调度器管理         • 客户端管理                     │    │
│    │  • 动态配置           • 控制台界面                     │    │
│    └───────────────────────────────────────────────────────┘    │
│                              │                                   │
│                              ▼                                   │
│    ┌───────────────────────────────────────────────────────┐    │
│    │                   Scheduler                            │    │
│    │  • P2P 调度策略       • 任务分配                       │    │
│    │  • 最优父节点选择     • 负载均衡                       │    │
│    └───────────────────────────────────────────────────────┘    │
│                              │                                   │
│          ┌───────────────────┼───────────────────┐              │
│          ▼                   ▼                   ▼              │
│    ┌──────────┐        ┌──────────┐        ┌──────────┐        │
│    │Seed Peer │        │Seed Peer │        │Seed Peer │        │
│    │(种子节点)│        │(种子节点)│        │(种子节点)│        │
│    └────┬─────┘        └────┬─────┘        └────┬─────┘        │
│         │                   │                   │               │
│    ┌────┴────┐         ┌────┴────┐         ┌────┴────┐         │
│    ▼         ▼         ▼         ▼         ▼         ▼         │
│  ┌─────┐ ┌─────┐   ┌─────┐ ┌─────┐   ┌─────┐ ┌─────┐          │
│  │Peer │ │Peer │   │Peer │ │Peer │   │Peer │ │Peer │          │
│  │(节点)│ │(节点)│   │(节点)│ │(节点)│   │(节点)│ │(节点)│          │
│  └─────┘ └─────┘   └─────┘ └─────┘   └─────┘ └─────┘          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### P2P 传输流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    P2P 分发流程                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. 初始请求                                                     │
│     Client ────► Scheduler ────► Seed Peer ────► Registry       │
│                                                                  │
│  2. P2P 分发形成                                                 │
│                                                                  │
│     Registry                                                     │
│         │                                                        │
│         ▼                                                        │
│    ┌─────────┐                                                   │
│    │Seed Peer│ ◄── 从源拉取                                      │
│    └────┬────┘                                                   │
│         │                                                        │
│    ┌────┴────┬────────┬────────┐                                │
│    ▼         ▼        ▼        ▼                                │
│  Peer A   Peer B   Peer C   Peer D ◄── P2P 传输                 │
│    │         │        │                                          │
│    ├─────────┼────────┘                                          │
│    ▼         ▼                                                   │
│  Peer E   Peer F              ◄── 节点间互传                     │
│                                                                  │
│  3. 效果对比                                                     │
│     传统方式: 100 节点 × 1GB = 100GB 出口带宽                    │
│     P2P 方式: ~5-10GB 出口带宽 (90%+ 节省)                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 | 说明 |
|:---|:---|:---|
| **Manager** | 管理中心 | 配置管理、权限控制、UI 控制台 |
| **Scheduler** | 调度器 | P2P 网络调度、任务分配 |
| **Seed Peer** | 种子节点 | 回源缓存、P2P 分发源 |
| **Peer (Dfdaemon)** | 客户端 | 每个节点的代理进程 |

---

## 安装部署

### Kubernetes Helm 安装

```bash
# 添加 Helm 仓库
helm repo add dragonfly https://dragonflyoss.github.io/helm-charts/
helm repo update

# 安装 Dragonfly
helm install dragonfly dragonfly/dragonfly \
  --namespace dragonfly-system \
  --create-namespace \
  --set scheduler.replicas=3 \
  --set seedPeer.replicas=3 \
  --set manager.replicas=1
```

### 配置示例

```yaml
# values.yaml
manager:
  replicas: 1
  resources:
    requests:
      cpu: "500m"
      memory: "512Mi"

scheduler:
  replicas: 3
  config:
    scheduler:
      algorithm: default  # 调度算法
      backToSourceCount: 200
      retryBackToSourceLimit: 5

seedPeer:
  replicas: 3
  config:
    seedPeer:
      type: super
      clusterID: 1

dfdaemon:
  # 以 DaemonSet 部署
  config:
    proxy:
      registryMirror:
        url: https://index.docker.io
    scheduler:
      netAddrs:
        - type: tcp
          addr: scheduler.dragonfly-system.svc:8002
```

### containerd 集成

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".registry]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
      endpoint = ["http://127.0.0.1:65001", "https://registry-1.docker.io"]
    
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."gcr.io"]
      endpoint = ["http://127.0.0.1:65001", "https://gcr.io"]
```

---

## 核心功能

### 1. 镜像预热 (Preheat)

```yaml
# 通过 API 预热镜像
POST /api/v1/preheats
{
  "type": "image",
  "url": "https://index.docker.io/v2/library/nginx/manifests/latest",
  "tag": "nginx:latest",
  "filter": "tag=latest&arch=amd64",
  "headers": {
    "Authorization": "Bearer xxx"
  },
  "scope": {
    "cidrs": ["10.0.0.0/8"],
    "hosts": ["node-1", "node-2"]
  }
}
```

```bash
# dfget 命令行预热
dfget preheat --url https://example.com/file.tar.gz \
  --digest sha256:xxx \
  --scope node-1,node-2
```

### 2. 智能调度

```yaml
# 调度策略配置
scheduler:
  algorithm: default  # 或 ml (机器学习)
  
  # 评分权重
  evaluator:
    idc: 0.1          # IDC 权重
    location: 0.3     # 地理位置权重
    netTopology: 0.2  # 网络拓扑权重
    
  # 父节点选择
  parentLimit: 4      # 每个任务最多父节点数
  
  # 回源限制
  backToSourceCount: 200
```

### 3. 多协议支持

```yaml
# dfdaemon 代理配置
proxy:
  # HTTP/HTTPS 代理
  server:
    port: 65001
  
  # 注册表镜像
  registryMirror:
    url: https://index.docker.io
    
  # 支持的协议
  rules:
    - regx: ".*\\.example\\.com.*"
      direct: false  # 使用 P2P
    - regx: ".*internal.*"
      direct: true   # 直连
```

### 4. 文件分发

```bash
# 大文件 P2P 分发
dfget -u https://example.com/large-file.tar.gz \
  -o /tmp/large-file.tar.gz \
  --digest sha256:xxx

# 分发任务状态查询
dfget stat --task-id xxx
```

---

## 使用场景

### 1. Kubernetes 镜像加速

```
传统方式                          Dragonfly P2P
┌────────────┐                   ┌────────────┐
│  Registry  │                   │  Registry  │
│            │                   │            │
└─────┬──────┘                   └─────┬──────┘
      │ 100GB                          │ ~10GB
┌─────┼─────────────────┐        ┌─────┼──────┐
▼     ▼     ▼     ▼     ▼        ▼     │      │
N1    N2    N3   ...   N100     Seed ──┼──► P2P
                                       │  传输
                                 N1 ◄──┴──► N2 ◄──► N3...

100 节点 × 1GB 镜像               出口带宽节省 90%+
```

### 2. AI/ML 模型分发

```yaml
# 大模型文件分发
apiVersion: batch/v1
kind: Job
metadata:
  name: model-distribute
spec:
  template:
    spec:
      containers:
        - name: dfget
          image: dragonflyoss/dfget
          command:
            - dfget
            - -u
            - https://huggingface.co/models/llama-7b.bin
            - -o
            - /models/llama-7b.bin
          volumeMounts:
            - name: models
              mountPath: /models
```

### 3. 大规模集群更新

```bash
# 批量推送配置文件
for node in $(kubectl get nodes -o name); do
  kubectl exec -n dragonfly-system $node -- \
    dfget -u https://config.example.com/app.conf \
    -o /etc/app/app.conf
done
```

---

## 性能优化

### 调优建议

```yaml
# 生产环境配置
seedPeer:
  replicas: 5  # 根据集群规模
  resources:
    requests:
      cpu: "2"
      memory: "4Gi"
  config:
    seedPeer:
      # 缓存配置
      gcInterval: 1h
      taskExpireTime: 24h
      
scheduler:
  replicas: 3
  config:
    scheduler:
      # 并发控制
      peerTaskLimit: 1000
      peerCount: 200
      
dfdaemon:
  config:
    download:
      # 下载并发
      totalRateLimit: 1024Mi
      perPeerRateLimit: 512Mi
      pieceDownloadTimeout: 30s
```

### 监控指标

```bash
# Prometheus 指标
# dragonfly_scheduler_peer_task_total - 任务计数
# dragonfly_scheduler_peer_host_traffic - 流量统计
# dragonfly_dfdaemon_download_traffic_total - 下载流量
# dragonfly_seed_peer_download_traffic_total - 种子节点流量

# Grafana Dashboard
# https://grafana.com/grafana/dashboards/15945
```

---

## 参考资源

- [官方文档](https://d7y.io/docs)
- [GitHub Repo](https://github.com/dragonflyoss/Dragonfly2)
- [CNCF 项目页面](https://www.cncf.io/projects/dragonfly/)
- [Helm Charts](https://github.com/dragonflyoss/helm-charts)
- [阿里云最佳实践](https://help.aliyun.com/document_detail/209936.html)

---

**维护者**: Kudig Team | **许可证**: MIT
