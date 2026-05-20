---
title: Docker & Containerd 速查卡
description: 容器运行时操作命令快速参考，覆盖 Docker v20.10+ 和 containerd v1.6+
category: cheatsheet
tags:
- docker
- containerd
- container
- cheatsheet
- quick-reference
- cri
- mysql
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Docker & Containerd 速查卡 是什么
- 如何 Docker & Containerd 速查卡
trigger_keywords:
- Docker
- Containerd
- 速查卡
- cheat
- sheet
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: ../domain-13-docker-container/
  desc: Docker 容器深度文档
- path: ../topic-cheat-sheet/k8s.md
  desc: Kubernetes 速查卡
- path: ../domain-3-control-plane/21-container-runtime-deep-dive.md
  desc: CRI 深度解析
---


# Docker & Containerd 速查表

> 容器运行时操作命令快速参考 | Docker v20.10+ / containerd v1.6+ | **最后更新**: 2026-05

---

## 目录

- [容器生命周期](#容器生命周期)
- [镜像管理](#镜像管理)
- [容器操作](#容器操作)
- [网络管理](#网络管理)
- [存储卷](#存储卷)
- [日志与监控](#日志与监控)
- [Docker Compose](#docker-compose)
- [Containerd (ctr)](#containerd-ctr)
- [安全与清理](#安全与清理)

---

## 容器生命周期

### 运行容器

```bash
# 基本运行
docker run nginx:latest

# 交互式运行（常用）
docker run -it --name myapp ubuntu:22.04 bash

# 后台运行 + 端口映射
docker run -d -p 8080:80 --name web nginx

# 完整参数示例
docker run -d \
  --name myapp \
  --hostname app-server \
  --restart unless-stopped \
  -p 8080:80 \
  -p 443:443 \
  -v /host/data:/app/data \
  -e DATABASE_URL=mysql://db:3306 \
  --memory=512m \
  --cpus=1.0 \
  --network mynet \
  myimage:tag
```

**常用参数速查**:
| 参数 | 说明 |
|:---|:---|
| `-d` | 后台运行 (detached) |
| `-it` | 交互式 TTY |
| `--name` | 指定容器名 |
| `-p H:C` | 端口映射 (主机:容器) |
| `-P` | 暴露 Dockerfile 中所有 EXPOSE 端口 |
| `-v H:C` | 卷挂载 (主机:容器) |
| `--rm` | 停止后自动删除 |
| `--restart` | 重启策略 (no/on-failure/always/unless-stopped) |
| `-e K=V` | 环境变量 |
| `--env-file` | 从文件加载环境变量 |
| `--network` | 指定网络 |
| `--memory` | 内存限制 |
| `--cpus` | CPU 限制 |
| `--user` | 以指定用户运行 |
| `--privileged` | 特权模式（不安全，慎用） |
| `--cap-add` | 添加 Linux 能力 |
| `--security-opt` | 安全选项 |

### 容器启停与删除

```bash
# 启动/停止/重启
docker start <container>
docker stop <container>      # 优雅停止 (SIGTERM)
docker kill <container>      # 强制停止 (SIGKILL)
docker restart <container>

# 暂停/恢复
docker pause <container>
docker unpause <container>

# 删除容器
docker rm <container>                    # 删除已停止的容器
docker rm -f <container>                 # 强制删除运行中的容器
docker rm $(docker ps -aq)               # 删除所有容器
docker container prune                   # 删除所有已停止容器

# 批量操作
docker stop $(docker ps -q)              # 停止所有容器
docker kill $(docker ps -q)              # 强制停止所有容器
```

---

## 镜像管理

### 镜像操作

```bash
# 搜索和拉取
docker search nginx
docker pull nginx:latest
docker pull nginx@sha256:abc123...       # 指定 digest

# 列出镜像
docker images
docker images --filter "dangling=true"   # 悬空镜像

# 删除镜像
docker rmi <image>
docker rmi $(docker images -q)           # 删除所有镜像
docker image prune                       # 删除悬空镜像
docker image prune -a                    # 删除所有未使用镜像

# 标签管理
docker tag nginx:latest myregistry/nginx:v1.0

# 保存和加载
docker save -o nginx.tar nginx:latest
docker load -i nginx.tar

# 导出导入（仅单层）
docker export -o container.tar <container>
docker import container.tar myimage:tag

# 构建镜像
docker build -t myapp:v1 .
docker build -t myapp:v1 -f Dockerfile.prod .
docker build --no-cache -t myapp:v1 .    # 不使用缓存

# 查看镜像历史
docker history nginx:latest

# 镜像详情
docker inspect nginx:latest
```

### 镜像仓库操作

```bash
# 登录/登出
docker login registry.example.com
docker logout registry.example.com

# 推送镜像
docker push myregistry/myapp:v1.0

# 从镜像运行命令
docker run --rm myimage cat /etc/os-release
```

---

## 容器操作

### 查看容器

```bash
# 列出运行中的容器
docker ps

# 列出所有容器
docker ps -a

# 格式化输出
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker ps -q                              # 仅显示 ID

# 容器详情
docker inspect <container>
docker inspect -f '{{.NetworkSettings.IPAddress}}' <container>

# 查看进程
docker top <container>

# 查看资源使用
docker stats
docker stats --no-stream                    # 只输出一次
```

### 进入容器

```bash
# 执行命令
docker exec -it <container> bash
docker exec -it <container> sh              # alpine 等无 bash 的系统

# 以 root 身份进入
docker exec -it -u root <container> bash

# 不进入容器执行命令
docker exec <container> ls -la /app
docker exec <container> ps aux

# 附加到容器主进程（查看输出）
docker attach <container>
# 注意: attach 后按 Ctrl+P+Q 分离，Ctrl+C 会停止容器
```

### 文件操作

```bash
# 复制文件（主机 → 容器）
docker cp ./file.txt <container>:/app/

# 复制文件（容器 → 主机）
docker cp <container>:/app/file.txt ./

# 复制目录
docker cp ./data <container>:/app/
```

---

## 网络管理

### 网络操作

```bash
# 列出网络
docker network ls

# 创建网络
docker network create mynet
docker network create --driver bridge mynet
docker network create --subnet=172.18.0.0/16 mynet

# 查看网络详情
docker network inspect mynet

# 连接/断开容器
docker network connect mynet <container>
docker network disconnect mynet <container>

# 删除网络
docker network rm mynet
docker network prune                        # 删除未使用的网络
```

### 网络模式

| 模式 | 说明 |
|:---|:---|
| `bridge` | 默认模式，容器间通过网桥通信 |
| `host` | 使用主机网络栈 |
| `none` | 无网络 |
| `container:<name>` | 共享另一个容器的网络 |
| `overlay` | Swarm 跨主机网络 |
| `macvlan` | 直接暴露到物理网络 |

---

## 存储卷

### 卷管理

```bash
# 创建卷
docker volume create myvol
docker volume create --driver local --opt type=nfs mynfs

# 列出卷
docker volume ls

# 查看卷详情
docker volume inspect myvol

# 删除卷
docker volume rm myvol
docker volume prune                         # 删除未使用的卷
```

### 绑定挂载 vs 卷

```bash
# 卷（Volume）- Docker 管理
docker run -v myvol:/data nginx

# 绑定挂载（Bind Mount）- 主机路径
docker run -v /host/path:/container/path nginx
docker run --mount type=bind,source=/host,target=/container nginx

# tmpfs 挂载（内存）
docker run --tmpfs /tmp:rw,noexec,nosuid,size=100m nginx
```

---

## 日志与监控

### 日志管理

```bash
# 查看日志
docker logs <container>
docker logs -f <container>                  # 实时跟踪
docker logs --tail 100 <container>          # 最后 100 行
docker logs --since 10m <container>         # 最近 10 分钟
docker logs -t <container>                  # 显示时间戳

# 清理日志（手动）
echo "" > $(docker inspect --format='{{.LogPath}}' <container>)
```

### 资源限制

```bash
# 内存限制
docker run -m 512m --memory-swap 1g myapp   # 512MB 内存，1GB swap

# CPU 限制
docker run --cpus=1.5 myapp                 # 1.5 核
docker run --cpuset-cpus="0-3" myapp        # 绑定到 CPU 0-3

# IO 限制
docker run --device-read-bps /dev/sda:1mb myapp
```

---

## Docker Compose

### 基本命令

```bash
# 启动服务
docker-compose up
docker-compose up -d                        # 后台运行
docker-compose up --build                   # 重新构建

# 停止服务
docker-compose down
docker-compose down -v                      # 同时删除卷

# 查看状态
docker-compose ps
docker-compose logs -f

# 扩展服务
docker-compose up -d --scale web=3

# 执行命令
docker-compose exec web bash
docker-compose run --rm app migrate
```

### Compose 文件示例

```yaml
version: '3.8'

services:
  web:
    image: nginx:latest
    ports:
      - "80:80"
    volumes:
      - ./html:/usr/share/nginx/html
    networks:
      - frontend
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost"]
      interval: 30s
      timeout: 10s
      retries: 3

  db:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: secret
      MYSQL_DATABASE: mydb
    volumes:
      - db_data:/var/lib/mysql
    networks:
      - backend
    deploy:
      resources:
        limits:
          memory: 1G

volumes:
  db_data:

networks:
  frontend:
  backend:
    internal: true
```

---

## Containerd (ctr)

### 命名空间

```bash
# 默认命名空间: default, k8s.io, moby
ctr -n k8s.io <command>                     # 指定 k8s 命名空间
```

### 镜像操作

```bash
# 列出镜像
ctr images ls
ctr -n k8s.io images ls                     # 查看 Kubernetes 镜像

# 拉取镜像
ctr images pull docker.io/library/nginx:latest

# 导入/导出
ctr images import image.tar
ctr images export nginx.tar nginx:latest

# 删除镜像
ctr images rm nginx:latest

# 查看详情
ctr images check nginx:latest
```

### 容器操作

```bash
# 列出容器
ctr containers ls

# 创建容器（不运行）
ctr containers create docker.io/library/nginx:latest nginx

# 运行容器
trunc run --rm docker.io/library/nginx:latest nginx

# 查看任务（运行中的容器）
ctr tasks ls
ctr tasks exec --exec-id shell -t <task> bash

# 查看指标
ctr tasks metrics <task>

# 删除容器
ctr containers rm <container>
```

### 对比 Docker & Containerd

| 操作 | Docker | Containerd |
|:---|:---|:---|
| 列出镜像 | `docker images` | `ctr images ls` |
| 拉取镜像 | `docker pull` | `ctr images pull` |
| 运行容器 | `docker run` | `ctr run` |
| 列出容器 | `docker ps` | `ctr containers ls` |
| 查看运行中 | `docker ps` | `ctr tasks ls` |
| 进入容器 | `docker exec` | `ctr tasks exec` |
| 查看日志 | `docker logs` | `ctr tasks attach` |

---

## 安全与清理

### 安全扫描

```bash
# 使用 Trivy 扫描镜像
trivy image nginx:latest

# 使用 Docker Scout
docker scout cves nginx:latest
```

### 系统清理

```bash
# 清理所有未使用资源
docker system prune                          # 删除停止的容器、悬空镜像、未使用网络
docker system prune -a                       # 删除所有未使用镜像
docker system prune --volumes                # 同时删除卷

# 单独清理
docker container prune
docker image prune
docker volume prune
docker network prune

# 查看磁盘使用
docker system df
docker system df -v                          # 详细视图
```

### 安全最佳实践

```bash
# 以非 root 用户运行
docker run -u 1000:1000 nginx

# 只读根文件系统
docker run --read-only -v /tmp:/tmp nginx

# 丢弃所有能力
docker run --cap-drop=ALL --cap-add=NET_BIND_SERVICE nginx

# 禁用特权
docker run --security-opt=no-new-privileges nginx

# 使用 seccomp
docker run --security-opt seccomp=default.json nginx
```

---

## 故障排查

```bash
# 容器无法启动
docker logs <container>
docker inspect <container>

# 网络问题
docker network inspect <network>
docker exec <container> cat /etc/resolv.conf

# 存储问题
docker volume inspect <volume>
df -h /var/lib/docker

# 性能问题
docker stats
docker system events

# 查看 Docker 事件
docker events --since 1h
```

---

## 相关文档

- [domain-13-docker/](../domain-13-docker/) - Docker 完整指南
- [domain-3-control-plane/21-container-runtime-deep-dive.md](../domain-3-control-plane/21-container-runtime-deep-dive.md) - CRI 深度解析
