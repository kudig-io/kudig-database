---
title: Docker
description: '- [[14-容器运行时/README.md|Docker 容器技术深度解析]]'
summary: '- [[14-容器运行时/README.md|Docker 容器技术深度解析]]'
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




# Docker 速查卡

Docker 是容器化的开创者。K8s v1.24 移除 dockershim 后，Docker 不再作为 K8s 节点运行时，但仍是开发和镜像构建的核心工具。

## 核心架构

```
Docker CLI → dockerd (API) → containerd → containerd-shim → runc
     │            │              │              │              │
  用户命令    镜像/网络/卷   容器生命周期   守护进程     OCI 运行时
```

| 组件 | 职责 |
|------|------|
| Docker CLI | 用户接口 (docker 命令) |
| dockerd | API 服务，管理镜像/网络/卷 |
| containerd | 容器生命周期管理 |
| containerd-shim | dockerd 重启时保持容器运行 |
| runc | OCI 运行时，创建实际容器进程 |
| BuildKit | 高性能镜像构建引擎 |

## 镜像管理命令

```bash
# 🟢 查看镜像
 docker images
docker images --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}"
docker image ls -a --filter "dangling=true"

# 🟢 拉取镜像
docker pull nginx:1.25
docker pull --platform linux/arm64 nginx:1.25

# 🔴 删除镜像
docker rmi nginx:1.25
docker image prune -a  # 删除所有未使用镜像
docker image prune --filter "until=720h"  # 30天前

# 🟢 镜像历史
docker history nginx:1.25 --no-trunc
docker history nginx:1.25 --format "{{.Size}}\t{{.CreatedBy}}"

# 🟢 镜像检查
docker inspect nginx:1.25
docker manifest inspect nginx:1.25  # 多平台信息

# 🟡 镜像标签
docker tag myapp:v1 registry.example.com/myapp:v1
docker tag myapp:v1 myapp:latest

# 🟡 推送镜像
docker push registry.example.com/myapp:v1
docker push --all-tags registry.example.com/myapp

# 🟢 镜像导出/导入
docker save myapp:v1 -o myapp.tar
docker load -i myapp.tar
docker export <container_id> -o container.tar  # 导出文件系统
```

## 容器生命周期命令

```bash
# 🟡 创建并运行
docker run -d --name myapp -p 8080:80 -e ENV=prod nginx:1.25
docker run -it --rm ubuntu:22.04 /bin/bash  # 交互式
docker run -d --restart=unless-stopped myapp:v1  # 自动重启
docker run -d --memory=512m --cpus=1.5 myapp:v1  # 资源限制
docker run -d --network=host myapp:v1  # 主机网络
docker run -d -v /host/path:/container/path myapp:v1  # 卷挂载
docker run -d --read-only --tmpfs /tmp myapp:v1  # 只读文件系统

# 🟢 查看容器
docker ps -a
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker stats  # 实时资源使用
docker top myapp  # 进程列表

# 🟢 容器日志
docker logs myapp --tail=100 -f
docker logs myapp --since=1h
docker logs myapp 2>&1 | grep ERROR

# 🟡 容器操作
docker stop myapp        # 优雅停止 (SIGTERM + 10s)
docker kill myapp        # 强制停止 (SIGKILL)
docker restart myapp
docker pause myapp       # 暂停
docker unpause myapp     # 恢复
docker rm myapp          # 删除
docker rm -f myapp       # 强制删除

# 🟢 进入容器
docker exec -it myapp /bin/sh
docker exec -it myapp env
docker exec -u root myapp apt-get update

# 🟢 容器检查
docker inspect myapp
docker inspect myapp --format '{{.State.Status}}'
docker inspect myapp --format '{{.NetworkSettings.IPAddress}}'
docker port myapp

# 🟢 复制文件
docker cp myapp:/app/config.yaml ./config.yaml
docker cp ./new-config.yaml myapp:/app/config.yaml

# 🟢 容器差异
docker diff myapp  # 文件系统变更
```

## Dockerfile 最佳实践

### 多阶段构建

```dockerfile
# 构建阶段
FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -ldflags="-s -w" -o /app/server .

# 运行阶段
FROM gcr.io/distroless/static-debian12:nonroot
COPY --from=builder /app/server /server
EXPOSE 8080
USER nonroot:nonroot
ENTRYPOINT ["/server"]
```

### BuildKit 高级特性

```dockerfile
# syntax=docker/dockerfile:1

# 缓存挂载 - 加速依赖下载
FROM node:20-alpine
WORKDIR /app
COPY package*.json ./
RUN --mount=type=cache,target=/root/.npm \
    npm ci --production

# 密钥挂载 - 不泄露到镜像层
RUN --mount=type=secret,id=npm_token \
    npm config set //registry.example.com/:_authToken=$(cat /run/secrets/npm_token)

# SSH 挂载 - 私有仓库访问
RUN --mount=type=ssh git clone git@github.com:org/private.git

# 绑定挂载 - 不创建新层
RUN --mount=type=bind,source=.,target=/src \
    cd /src && make build
```

### 镜像优化检查清单

| 优化项 | 方法 | 效果 |
|--------|------|------|
| 多阶段构建 | 分离构建/运行 | 减小 70%+ |
| 精简基础镜像 | distroless/alpine/scratch | 减小 50%+ |
| 合并 RUN 层 | 用 && 连接命令 | 减少层数 |
| .dockerignore | 排除无关文件 | 加速构建 |
| 固定版本 | 避免 :latest | 可重复构建 |
| 层缓存优化 | 依赖文件先 COPY | 加速重建 |
| 非 root 用户 | USER 指令 | 安全性 |
| 健康检查 | HEALTHCHECK 指令 | 可观测性 |

## 网络命令

```bash
# 🟢 查看网络
docker network ls
docker network inspect bridge

# 🟡 创建网络
docker network create --driver bridge --subnet 172.20.0.0/16 mynet
docker network create --driver overlay --attachable swarm-net

# 🟡 连接/断开
docker network connect mynet myapp
docker network disconnect mynet myapp

# 🟢 DNS 解析
docker exec myapp nslookup other-container
# 同一网络内容器可通过名称互相访问

# 🔴 清理
docker network prune
docker network rm mynet
```

## 存储与卷

```bash
# 🟢 查看卷
docker volume ls
docker volume inspect mydata

# 🟡 创建卷
docker volume create mydata
docker volume create --driver local --opt type=nfs --opt o=addr=10.0.0.1,rw --opt device=:/export mydata

# 🟡 使用卷
docker run -d -v mydata:/app/data myapp:v1       # 命名卷
docker run -d -v /host/path:/app/data myapp:v1   # 绑定挂载
docker run -d --mount type=tmpfs,destination=/tmp myapp:v1  # tmpfs

# 🔴 清理
docker volume prune
docker volume rm mydata
```

## Docker Compose

```yaml
# docker-compose.yaml 示例
services:
  app:
    build:
      context: .
      dockerfile: Dockerfile
      target: production
    ports:
      - "8080:8080"
    environment:
      - DATABASE_URL=postgres://db:5432/app
    depends_on:
      db:
        condition: service_healthy
    restart: unless-stopped
    deploy:
      resources:
        limits:
          memory: 512M
          cpus: '1.0'

  db:
    image: postgres:16-alpine
    volumes:
      - pgdata:/var/lib/postgresql/data
    environment:
      POSTGRES_PASSWORD: ${DB_PASSWORD}
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 5s
      timeout: 3s
      retries: 5

volumes:
  pgdata:
```

```bash
# Compose 命令
docker compose up -d
docker compose down -v  # 停止并删除卷
docker compose logs -f app
docker compose exec app /bin/sh
docker compose build --no-cache
docker compose pull
docker compose ps
docker compose top
```

## 安全最佳实践

```bash
# 非 root 运行
docker run -d --user 1000:1000 myapp:v1

# 只读文件系统
docker run -d --read-only --tmpfs /tmp myapp:v1

# 禁止特权提升
docker run -d --security-opt no-new-privileges myapp:v1

# 限制 capabilities
docker run -d --cap-drop=ALL --cap-add=NET_BIND_SERVICE myapp:v1

# 资源限制
docker run -d --memory=256m --memory-swap=256m --cpus=0.5 myapp:v1

# 禁止容器间通信
docker run -d --network=none myapp:v1

# 镜像扫描
docker scout cves myapp:v1
trivy image myapp:v1
```

## 故障排查

```bash
# 容器无法启动
docker logs myapp
docker inspect myapp --format '{{.State.Error}}'
docker events --filter container=myapp

# 磁盘空间不足
docker system df
docker system prune -a --volumes  # 🔴 清理所有未使用资源

# 网络问题
docker exec myapp ping other-container
docker exec myapp cat /etc/resolv.conf
docker network inspect bridge

# 性能问题
docker stats myapp
docker exec myapp top
docker inspect myapp --format '{{.HostConfig.Memory}}'

# 守护进程问题
systemctl status docker
journalctl -u docker --since="10 min ago"
```

## 清理命令

```bash
# 🟢 查看磁盘使用
docker system df
docker system df -v

# 🔴 清理未使用资源
docker system prune          # 停止的容器 + 悬空镜像 + 未使用网络
docker system prune -a       # + 所有未使用镜像
docker system prune -a --volumes  # + 未使用卷

# 🟡 单独清理
docker container prune
docker image prune -a
docker volume prune
docker network prune
docker builder prune  # 构建缓存
```

## 常用参数速查

| 参数 | 说明 | 示例 |
|------|------|------|
| -d | 后台运行 | docker run -d nginx |
| -p | 端口映射 | -p 8080:80 |
| -v | 卷挂载 | -v /data:/app/data |
| -e | 环境变量 | -e ENV=prod |
| --name | 容器名 | --name myapp |
| --network | 网络 | --network mynet |
| --restart | 重启策略 | --restart=unless-stopped |
| --memory | 内存限制 | --memory=512m |
| --cpus | CPU 限制 | --cpus=1.5 |
| --user | 运行用户 | --user 1000:1000 |
| --read-only | 只读文件系统 | --read-only |
| --cap-drop | 移除能力 | --cap-drop=ALL |
| --health-cmd | 健康检查 | --health-cmd="curl -f http://localhost/" |
| --platform | 目标平台 | --platform linux/arm64 |
| --pull | 拉取策略 | --pull=always |
| --init | 使用 init 进程 | --init (PID 1 问题) |
| --dns | 自定义 DNS | --dns 8.8.8.8 |
| --log-driver | 日志驱动 | --log-driver=json-file |
| --log-opt | 日志选项 | --log-opt max-size=10m |

## Buildx 多平台构建

```bash
# 创建 builder
docker buildx create --name mybuilder --use
docker buildx inspect --bootstrap

# 多平台构建并推送
docker buildx build --platform linux/amd64,linux/arm64 \
  -t registry.example.com/myapp:v1 --push .

# 本地加载（单平台）
docker buildx build --platform linux/amd64 \
  -t myapp:v1 --load .

# 使用缓存
docker buildx build --cache-from=type=registry,ref=registry/myapp:buildcache \
  --cache-to=type=registry,ref=registry/myapp:buildcache,mode=max \
  -t myapp:v1 --push .

# 从文件构建
docker buildx build -f Dockerfile.prod -t myapp:v1 .

# 构建参数
docker buildx build --build-arg VERSION=1.2.3 -t myapp:v1 .
```

## 镜像仓库操作

```bash
# 登录
docker login registry.example.com
docker login --username=user --password-stdin registry.example.com <<< "$TOKEN"

# 搜索
docker search nginx --limit 5

# 查看远程标签
curl -s https://registry.example.com/v2/myapp/tags/list | jq .

# 删除远程标签
curl -X DELETE https://registry.example.com/v2/myapp/manifests/sha256:...

# 镜像复制（跨仓库）
docker pull src/myapp:v1
docker tag src/myapp:v1 dst/myapp:v1
docker push dst/myapp:v1
```

## 生产故障排查流程

### 容器启动失败

```
1. docker logs <container>          # 查看应用日志
2. docker inspect <container>       # 查看状态和错误
3. docker events --filter container=<id>  # 查看事件
4. 检查: 镜像是否存在? 端口是否冲突? 卷路径是否正确?
5. 检查: 资源限制是否合理? 健康检查是否配置正确?
```

### 磁盘空间不足

```
1. docker system df                 # 查看各类型占用
2. docker system df -v              # 详细列表
3. docker image prune -a            # 清理未使用镜像
4. docker container prune           # 清理停止的容器
5. docker builder prune             # 清理构建缓存
6. 检查 /var/lib/docker 分区大小
```

### 网络连通性问题

```
1. docker network ls                # 查看网络
2. docker network inspect <net>     # 查看网络详情
3. docker exec <c1> ping <c2>       # 容器间连通性
4. docker exec <c> curl host:port   # 外部连通性
5. 检查: iptables 规则? DNS 配置? 端口映射?
```

## Docker vs Podman vs nerdctl

| 特性 | Docker | Podman | nerdctl |
|------|--------|--------|--------|
| 架构 | 守护进程 | 无守护进程 | 无守护进程 |
| Root | 需要 root | 支持 rootless | 支持 rootless |
| Compose | docker compose | podman-compose | nerdctl compose |
| K8s 集成 | 已移除 | 原生支持 | containerd 原生 |
| 兼容性 | 标准 | Docker 兼容 | Docker 兼容 |
| 适用 | 开发/构建 | 生产/安全 | containerd 环境 |

## 环境变量与配置

```bash
# Docker daemon 配置 (/etc/docker/daemon.json)
{
  "registry-mirrors": ["https://mirror.example.com"],
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  },
  "storage-driver": "overlay2",
  "live-restore": true,
  "default-ulimits": {
    "nofile": {"Name": "nofile", "Hard": 65536, "Soft": 65536}
  }
}

# 重启生效
systemctl restart docker
```

## Related

- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]] — Container Runtime
- [[22-概念/15-运行时与系统/docker-architecture.md|Docker Architecture]]
- [[22-概念/15-运行时与系统/container-runtime-comparison.md|Container Runtime Comparison]]
- [[14-容器运行时/README.md|Docker 容器技术深度解析]]

<!-- risk-assessed -->
