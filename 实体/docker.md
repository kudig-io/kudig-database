---
title: Docker
description: '- [[容器运行时/README.md|Docker 容器技术深度解析]]'
summary: '- [[容器运行时/README.md|Docker 容器技术深度解析]]'
category: entities
tags:
- k8s
- docker
- container
- image
- build
- containerd
- cri-o
- rag
- etcd
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Docker 是什么
- 如何 Docker
trigger_keywords:
- Docker
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Docker

Docker is the platform that popularized containerization. Since K8s v1.24 removed dockershim, Docker is no longer used as a K8s node runtime, but remains essential for development and image building.

## Key Facts

- **Latest Version**: Docker 26.0+
- **Runtime**: dockerd -> containerd -> runc
- **Image Format**: OCI Image Spec (compatible with all OCI runtimes)
- **Build Engine**: BuildKit (multi-stage, cache-efficient)
- **K8s Status**: Deprecated as runtime (v1.20), removed (v1.24)

## Components

| Component | Role |
|-----------|------|
| Docker CLI | User interface (docker command) |
| dockerd | API [[Service|service]], manages images/networks/volumes |
| containerd | Container lifecycle management |
| containerd-shim | Keeps container running when dockerd restarts |
| runc | OCI runtime, creates actual container process |

## Current Best Practice

Use Docker for development and image building. Use containerd or CRI-O for K8s production nodes. Docker-built images run on any OCI-compliant runtime.

## 镜像构建最佳实践

### 多阶段构建

```dockerfile
# 构建阶段
FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -o /app/server

# 运行阶段
FROM gcr.io/distroless/static:nonroot
COPY --from=builder /app/server /server
USER nonroot:nonroot
ENTRYPOINT ["/server"]
```

### BuildKit 高级特性

```dockerfile
# syntax=docker/dockerfile:1
FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN --mount=type=cache,target=/go/pkg/mod \
    go mod download
COPY . .
RUN --mount=type=cache,target=/root/.cache/go-build \
    CGO_ENABLED=0 go build -o /app/server
```

### 镜像优化检查清单

- [ ] 使用多阶段构建
- [ ] 基础镜像用 alpine/distroless
- [ ] 合并 RUN 指令减少层数
- [ ] .dockerignore 排除无关文件
- [ ] 非 root 用户运行
- [ ] 固定镜像 tag (不用 latest)
- [ ] 扫描漏洞 (trivy)

## 运维操作

### 常用命令

```bash
# 🟢 镜像管理
docker images
docker pull nginx:1.25
docker build -t myapp:v1 .
docker push registry.example.com/myapp:v1
docker tag myapp:v1 registry.example.com/myapp:v1

# 🟢 容器管理
docker ps -a
docker run -d --name app -p 8080:80 nginx:1.25
docker exec -it app sh
docker logs -f app
docker stop app && docker rm app

# 🟢 网络
docker network ls
docker network create mynet
docker network inspect mynet

# 🟢 存储
docker volume ls
docker volume create data
docker volume inspect data

# 🟡 清理
docker system prune -a --volumes  # 🔴 危险!
docker image prune -a
docker container prune

# 🟢 资源使用
docker stats
docker system df
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 构建失败 | 网络/依赖问题 | 检查网络、使用镜像源 |
| 容器启动失败 | 配置错误 | `docker logs` 查看 |
| 磁盘空间不足 | 镜像/容器堆积 | `docker system prune` |
| 网络不通 | 网络配置错误 | `docker network inspect` |
| 性能下降 | 资源限制 | 调整 CPU/Memory |

## Docker vs Podman vs nerdctl

| 特性 | Docker | Podman | nerdctl |
|------|--------|--------|--------|
| Daemon | 有 (dockerd) | 无 (Rootless) | 无 (containerd) |
| Rootless | 支持 | 原生 | 支持 |
| Compose | 支持 | 支持 | 支持 |
| K8s 兼容 | 仅构建 | Pod 导出 | 原生 |
| 安全性 | 中 | 高 | 高 |

## 检查清单

- [ ] 理解 Docker 架构 (CLI/dockerd/containerd/runc)
- [ ] 掌握多阶段构建
- [ ] 了解 BuildKit 高级特性
- [ ] 掌握镜像优化技巧
- [ ] 理解 Docker 在 K8s 中的角色变化
- [ ] 了解 Docker vs Podman vs nerdctl

## Related

- [[实体/container-runtime.md|container-runtime]] — Container Runtime
- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[概念/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[概念/container-runtime-comparison.md|container-runtime-comparison]] — Container Runtime Comparison
- [[概念/docker-architecture.md|Docker Architecture]]
- [[概念/container-runtime-comparison.md|Container Runtime Comparison]]
- [[containerd|containerd]]

- 00-open-source-projects-index
- 02-docker-registry-enterprise-distribution
- 05-docker-storage-volumes
- 11-docker-automation-devops
- [[容器运行时/README.md|Docker 容器技术深度解析]]
- 04-docker-networking-deep-dive
- 03-docker-container-lifecycle
- 09-docker-performance-monitoring
- 99-docker-commands-reference
- 07-docker-security-best-practices
- 01-docker-architecture-overview
- 容器运行时 MOC
- 02-docker-images-management
- 12-java-containerization-guide
- 06-docker-compose-orchestration
- 08-docker-troubleshooting-guide
- 10-docker-logging-management
- [[实体/k8s-design-principles-deep-dive.md|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[实体/kudig-contribution-guide.md|贡献指南、项目概览与版本发布说明]] — Cross-reference
- [[实体/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[实体/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[实体/k8s-supply-chain-yaml-cheatsheet.md|供应链安全、YAML 配置清单与速查表]] — Cross-reference
- [[实体/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 容器运行时 MOC — Cross-reference
- [[概念/cli-tools-evolution.md|CLI 工具演进]] — Cross-reference
- [[概念/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[概念/overlayfs-storage.md|OverlayFS Storage]] — Cross-reference
- [[概念/linux-container-foundation.md|Linux Container Foundation]] — Cross-reference
- [[技能/工作负载/daemonset/培训/learn-13-daemonset-basics.md|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-01-what-is-kubernetes.md|第一课：Kubernetes 入门]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-06-configmap-secret.md|第六课：ConfigMap 和 Secret - 配置管理]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-02-pod-basics.md|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- Domain-3: Kubernetes控制平面 — Cross-reference
- [[实体/kubernetes-changelog.md|Kubernetes 变更日志索引]] — Cross-reference
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference


<!-- risk-assessed -->
