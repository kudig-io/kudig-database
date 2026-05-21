---
title: Telepresence
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- docker
- redis
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Telepresence 是什么
- 如何 Telepresence
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Telepresence
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
---

title: Telepresence
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- docker
- redis
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Telepresence 是什么
- 如何 Telepresence
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Telepresence
- cncf
- landscape
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

# Telepresence

> **成熟度**: Sandbox | **加入时间**: 2021-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.telepresence.io |
| **GitHub** | https://github.com/telepresenceio/telepresence |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | App Definition & Build |
| **维护组织** | Ambassador Labs |

---

## 项目概述

Telepresence 是一个 Kubernetes 本地开发工具，它在本地开发环境和远程 Kubernetes 集群之间创建网络隧道。开发者可以在本地运行服务，同时访问集群中的其他服务和资源，也可以将集群流量拦截到本地进行调试。

---

## 核心特性

- **流量拦截**: 将 K8s 服务的请求重定向到本地
- **双向代理**: 本地访问集群服务，集群流量到本地
- **选择性拦截**: 基于 Header 条件拦截特定请求
- **DNS 代理**: 自动解析集群内服务 DNS
- **卷挂载**: 远程 Pod 卷挂载到本地
- **环境变量**: 同步远程 Pod 环境变量
- **多人协作**: 支持团队成员同时拦截不同服务

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                  Telepresence Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  Developer Laptop                         │   │
│  │                                                           │   │
│  │  ┌───────────────────┐  ┌────────────────────────────┐   │   │
│  │  │  Local Service    │  │   Telepresence CLI          │   │   │
│  │  │  (e.g. Node.js)  │  │   (User Daemon)             │   │   │
│  │  │                   │  │  ┌──────────────────────┐   │   │   │
│  │  │  localhost:3000   │  │  │  Root Daemon         │   │   │   │
│  │  │                   │  │  │  ┌────────────────┐  │   │   │   │
│  │  │                   │  │  │  │ DNS Proxy      │  │   │   │   │
│  │  │                   │  │  │  │ Network Tunnel │  │   │   │   │
│  │  │                   │  │  │  │ Volume Mount   │  │   │   │   │
│  │  │                   │  │  │  └────────────────┘  │   │   │   │
│  │  └───────────────────┘  │  └──────────────────────┘   │   │   │
│  │           ▲              └──────────────┬──────────────┘   │   │
│  │           │                             │                  │   │
│  │           │      Intercepted Traffic    │                  │   │
│  │           └─────────────────────────────┘                  │   │
│  └──────────────────────────────┬────────────────────────────┘   │
│                                 │ Encrypted Tunnel               │
│                                 │                                │
│  ┌──────────────────────────────▼────────────────────────────┐  │
│  │                   Kubernetes Cluster                        │  │
│  │                                                             │  │
│  │  ┌─────────────────────────────────────────────────────┐   │  │
│  │  │              Traffic Manager (Deployment)            │   │  │
│  │  │  ┌─────────────────────────────────────────────┐    │   │  │
│  │  │  │ Manages intercepts and traffic routing       │    │   │  │
│  │  │  └─────────────────────────────────────────────┘    │   │  │
│  │  └─────────────────────────────────────────────────────┘   │  │
│  │                           │                                 │  │
│  │  ┌────────────────────────▼──────────────────────────────┐ │  │
│  │  │              Intercepted Service                       │ │  │
│  │  │  ┌─────────────────────────────────────────────────┐  │ │  │
│  │  │  │              Original Pod                        │  │ │  │
│  │  │  │  ┌─────────────┐  ┌─────────────────────────┐  │  │ │  │
│  │  │  │  │   App       │  │   Traffic Agent         │  │  │ │  │
│  │  │  │  │  Container  │  │   (Sidecar)             │  │  │ │  │
│  │  │  │  │             │  │   Routes traffic to     │  │  │ │  │
│  │  │  │  │             │  │   local or cluster      │  │  │ │  │
│  │  │  │  └─────────────┘  └─────────────────────────┘  │  │ │  │
│  │  │  └─────────────────────────────────────────────────┘  │ │  │
│  │  └────────────────────────────────────────────────────────┘ │  │
│  │                                                             │  │
│  │  ┌─────────────────────────────────────────────────────┐   │  │
│  │  │              Other Cluster Services                  │   │  │
│  │  │  ┌────────────┐  ┌────────────┐  ┌──────────────┐  │   │  │
│  │  │  │ Database   │  │  Cache     │  │  Message Q   │  │   │  │
│  │  │  │ Service    │  │  Service   │  │  Service     │  │   │  │
│  │  │  └────────────┘  └────────────┘  └──────────────┘  │   │  │
│  │  └─────────────────────────────────────────────────────┘   │  │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **CLI (User Daemon)** | 本地客户端，管理连接和拦截 |
| **Root Daemon** | 本地网络代理，处理 DNS 和路由 |
| **Traffic Manager** | 集群内部署，管理所有拦截 |
| **Traffic Agent** | Sidecar 注入到目标 Pod |

---

## 快速开始

### 安装 CLI

```bash
# macOS
brew install datawire/blackbird/telepresence-oss

# Linux
sudo curl -fL https://app.getambassador.io/download/tel2oss/releases/download/v2.17.0/telepresence-linux-amd64 -o /usr/local/bin/telepresence
sudo chmod a+x /usr/local/bin/telepresence

# Windows
choco install telepresence
```

### 连接到集群

```bash
# 连接 (自动安装 Traffic Manager)
telepresence connect

# 验证连接
telepresence status

# 查看可拦截的服务
telepresence list

# 断开连接
telepresence quit
```

---

## 基本使用

### 流量拦截

```bash
# 拦截服务 (所有流量)
telepresence intercept my-service --port 3000:8080

# 拦截后在本地运行服务
npm start  # 本地运行，接收集群流量

# 结束拦截
telepresence leave my-service
```

### 选择性拦截 (基于 Header)

```bash
# 只拦截包含特定 Header 的请求
telepresence intercept my-service \
  --port 3000:8080 \
  --http-header x-telepresence-intercept-id=my-dev-session

# 其他请求继续发送到集群中的原始 Pod
```

### 访问集群服务

```bash
# 连接后，可以直接访问集群服务
telepresence connect

# 使用服务名称访问
curl http://backend-service.default:8080/api
curl http://redis.default:6379

# DNS 自动代理
ping database.production.svc.cluster.local
```

---

## 环境变量和卷挂载

### 获取远程环境变量

```bash
# 拦截时导出环境变量
telepresence intercept my-service --port 3000 --env-file=.env

# 使用环境变量文件
source .env
npm start

# 或直接在命令中使用
telepresence intercept my-service --port 3000 -- npm start
```

### 挂载远程卷

```bash
# 拦截时挂载远程卷
telepresence intercept my-service \
  --port 3000 \
  --mount=/tmp/tel-mount

# 查看挂载的文件
ls /tmp/tel-mount/var/run/secrets/kubernetes.io/serviceaccount/

# 禁用卷挂载
telepresence intercept my-service --port 3000 --mount=false
```

---

## 高级配置

### 配置文件 (config.yml)

```yaml
# ~/.config/telepresence/config.yml
timeouts:
  agentInstall: 120s
  apply: 1m
  clusterConnect: 60s
  intercept: 30s
  proxyDial: 30s
  trafficManagerConnect: 60s

logLevels:
  userDaemon: info
  rootDaemon: info

images:
  registry: docker.io/datawire
  agentImage: ambassador-telepresence-agent:latest

intercept:
  defaultPort: 8080

dns:
  excludeSuffixes:
    - .com
    - .io
  includeSuffixes:
    - .cluster.local
  lookupTimeout: 8s

routing:
  alsoProxySubnets:
    - 10.0.0.0/8
  neverProxySubnets:
    - 192.168.1.0/24
```

### Traffic Manager 配置

```yaml
# values.yaml for Helm
trafficManager:
  image:
    registry: docker.io/datawire
    name: ambassador-telepresence-manager
  
  resources:
    requests:
      cpu: 100m
      memory: 128Mi
    limits:
      cpu: 500m
      memory: 256Mi

  agentInjector:
    enabled: true
    agentImage:
      registry: docker.io/datawire
      name: ambassador-telepresence-agent
```

### Helm 安装 Traffic Manager

```bash
helm install traffic-manager datawire/telepresence \
  --namespace ambassador \
  --create-namespace
```

---

## 多人协作

### 个人拦截

```bash
# 开发者 A 拦截 frontend
telepresence intercept frontend --port 3000 \
  --http-header x-dev=alice

# 开发者 B 拦截 backend
telepresence intercept backend --port 8080 \
  --http-header x-dev=bob

# 没有匹配 header 的请求走原始 Pod
```

### 查看当前拦截

```bash
# 列出所有拦截
telepresence list --intercepts

# 查看状态
telepresence status

# 查看特定拦截
telepresence list --namespace production
```

---

## Docker 模式

### 在 Docker 容器中运行拦截

```bash
# 使用 Docker 运行拦截的本地服务
telepresence intercept my-service --port 3000 \
  --docker-run -- \
  -v $(pwd):/app \
  -p 3000:3000 \
  node:18 npm start

# Docker Compose 集成
telepresence intercept my-service --port 3000 \
  --docker-run -- \
  docker compose up app
```

---

## 常用命令

```bash
# 连接管理
telepresence connect                    # 连接到集群
telepresence connect --context staging  # 指定 context
telepresence quit                       # 断开连接
telepresence status                     # 查看状态

# 拦截管理
telepresence list                       # 列出可拦截的服务
telepresence intercept svc --port 3000  # 创建拦截
telepresence leave svc                  # 取消拦截

# Traffic Manager
telepresence helm install               # 安装 Traffic Manager
telepresence helm upgrade               # 升级
telepresence helm uninstall             # 卸载

# 调试
telepresence loglevel debug             # 设置日志级别
telepresence gather-logs                # 收集日志
telepresence version                    # 查看版本
```

---

## 与 IDE 集成

### VS Code 插件

```json
// .vscode/launch.json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Telepresence Debug",
      "type": "node",
      "request": "launch",
      "program": "${workspaceFolder}/src/index.js",
      "envFile": "${workspaceFolder}/.env",
      "console": "integratedTerminal"
    }
  ]
}
```

### JetBrains 集成

```bash
# 使用环境变量文件
telepresence intercept my-service --port 8080 --env-file=.env

# 在 IDE 的 Run Configuration 中引用 .env 文件
```

---

## 最佳实践

1. **选择性拦截**: 使用 Header 条件避免影响其他开发者
2. **环境变量**: 同步远程环境变量保持一致性
3. **Docker 模式**: 使用 Docker 模式确保环境一致
4. **DNS 配置**: 合理配置 DNS 排除规则
5. **资源清理**: 开发完成后及时 leave 和 quit
6. **安全**: 注意拦截的流量可能包含敏感数据

---

## 参考资源

- [官方文档](https://www.telepresence.io/docs/latest/)
- [GitHub Repo](https://github.com/telepresenceio/telepresence)
- [快速入门](https://www.telepresence.io/docs/latest/quick-start/)
- [拦截指南](https://www.telepresence.io/docs/latest/howtos/intercepts/)
- [配置参考](https://www.telepresence.io/docs/latest/reference/config/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
