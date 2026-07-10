---
title: 'Day 2: Docker 网络 + 存储 + 安全'
description: '# Day 2: Docker 网络 + 存储 + 安全'
summary: '在理解了 Docker 基础概念之后，今天深入 Docker 的三大高级特性：网络、存储和安全。这三个方面直接影响容器化应用的生产可靠性。Docker 网络决定了容器间如何通信，存储决定了数据如何持久化，安全决定了容器运行时的隔离程度。掌握这些内容是理解 [[系统基础/速查卡/k8s|k8s]] 的基础。'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- docker
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 2: Docker 网络 + 存储 + 安全 是什么'
- '如何 Day 2: Docker 网络 + 存储 + 安全'
trigger_keywords:
- Day
- '2:'
- Docker
- 网络
- 存储
- 安全
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 2: Docker 网络 + 存储 + 安全

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY2
title: Day 2 - Docker 网络 + 存储 + 安全
topic: docker
type: hands-on-guide
tags: [docker, network, bridge, host, overlay, volume, bind-mount, security, hands-on, week-1]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Docker 网络模式有哪些"
  - "Volume 和 Bind Mount 区别"
  - "Docker 安全最佳实践"
  - "docker-compose 网络怎么配"
trigger_keywords:
  - Docker 网络
  - bridge
  - host
  - overlay
  - none
  - macvlan
  - Volume
  - Bind Mount
  - tmpfs
  - docker-compose
  - 非 root 用户
  - 资源限制
  - 只读文件系统
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - 容器运行时
related_topics:
  - docker
  - networking
  - storage
  - security
related:
  - 生产运维/topic-learn/public-training/one-month/week-1-foundation/day-1-docker-basics.md
  - 容器运行时/04-docker-networking-deep-dive.md
---
```

> **学习时间**: 4-5 小时 | **主题**: Docker 高级特性

---

## 概述

在理解了 Docker 基础概念之后，今天深入 Docker 的三大高级特性：网络、存储和安全。这三个方面直接影响容器化应用的生产可靠性。Docker 网络决定了容器间如何通信，存储决定了数据如何持久化，安全决定了容器运行时的隔离程度。掌握这些内容是理解 [[系统基础/速查卡/k8s|k8s]] 网络 (CNI)、存储 (CSI) 和安全 (PSS) 的基础。

---

## 今日目标

- [ ] 理解 Docker 网络模式 (bridge/host/overlay)
- [ ] 掌握 Docker 存储机制 (Volume/Bind Mount)
- [ ] 了解 Docker 安全最佳实践

---

## 核心概念

### 1. Docker 网络模式

| 模式 | 原理 | 性能 | 隔离性 | 适用场景 |
|------|------|------|--------|---------|
| bridge | 虚拟网桥 + NAT | 中 | 高 | 默认模式，单机容器通信 |
| host | 共享宿主机网络栈 | 高 | 低 | 需要高性能网络的应用 |
| overlay | VXLAN 隧道跨主机 | 低 | 高 | Docker Swarm 跨主机通信 |
| none | 无网络 | - | 完全隔离 | 安全敏感、无需网络的容器 |
| macvlan | 容器拥有独立 MAC | 高 | 中 | 需要直接接入物理网络 |

### 2. Docker 存储类型

| 类型 | 存储位置 | 生命周期 | 性能 | 适用场景 |
|------|---------|---------|------|---------|
| Volume | /var/lib/docker/volumes/ | 由 Docker 管理 | 好 | 持久化数据、数据库 |
| Bind Mount | 宿主机任意路径 | 依赖宿主机 | 好 | 开发环境、配置文件 |
| tmpfs | 内存 | 容器停止即消失 | 最好 | 临时数据、敏感信息 |

### 3. Docker 安全要点

| 安全层面 | 风险 | 防护措施 |
|----------|------|---------|
| 镜像安全 | 漏洞、后门 | 使用可信基础镜像、定期扫描 |
| 运行时安全 | 容器逃逸 | 非 root 运行、只读文件系统 |
| 网络安全 | 横向移动 | 网络隔离、限制暴露端口 |
| 资源安全 | 资源耗尽 | 设置 CPU/内存限制 |

---

## 理论学习 (2h)

### 必读文档

1. **Docker 网络深入**
   - 文件: `../../容器运行时/04-docker-networking-deep-dive.md`
   - 重点: bridge、host、overlay 网络模式的区别和使用场景

2. **Docker 存储**
   - 文件: `../../容器运行时/05-docker-storage-volumes.md`
   - 重点: Volume vs Bind Mount vs tmpfs 的选择

### 补充阅读

3. **Docker 安全最佳实践**
   - 文件: `../../容器运行时/07-docker-security-best-practices.md`
   - 重点: 非 root 用户、只读文件系统、资源限制

---

## 实战演练 (2.5h)

### 任务 1: Docker 网络实验 (1h)

#### 1.1 Bridge 网络实验

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 创建自定义 bridge 网络
docker network create my-network
# 查看网络列表
docker network ls
# NETWORK ID     NAME         DRIVER    SCOPE
# xxxxxxxx       bridge       bridge    local
# xxxxxxxx       host         host      local
# xxxxxxxx       my-network   bridge    local

# 运行两个容器在同一网络
docker run -d --name web --network my-network nginx:alpine
docker run -d --name app --network my-network alpine sleep 3600

# 测试容器间通信 (通过容器名解析)
docker exec app ping -c 3 web
# PING web (172.18.0.2): 56 data bytes
# 64 bytes from 172.18.0.2: seq=0 ttl=64 time=0.100 ms
# 64 bytes from 172.18.0.2: seq=1 ttl=64 time=0.080 ms

# 查看网络详情
docker network inspect my-network
# "Containers": {
#     "xxx": { "Name": "web", "IPv4Address": "172.18.0.2/16" },
#     "yyy": { "Name": "app", "IPv4Address": "172.18.0.3/16" }
# }

# 测试不同网络隔离
docker run -d --name isolated alpine sleep 3600
docker exec isolated ping -c 2 web
# ping: bad address 'web'  ← 无法解析，不在同一网络

# 清理
docker stop web app isolated
docker rm web app isolated
docker network rm my-network
```
#### 1.2 Host 网络实验

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# host 模式: 容器直接使用宿主机网络栈
docker run -d --name host-nginx --network host nginx:alpine

# 验证: 直接通过 localhost 访问
curl -s http://localhost | head -5
# <!DOCTYPE html>
# <html>
# <head>
# <title>Welcome to nginx!</title>

# 查看端口映射 (host 模式无映射)
docker port host-nginx
# 无输出

# 注意: host 模式下端口冲突会导致启动失败
# 如果宿主机 80 端口已被占用，容器将无法启动

# 清理
docker stop host-nginx && docker rm host-nginx
```
#### 1.3 None 网络实验

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# none 模式: 无网络
docker run --rm --network none alpine ip addr
# 1: lo: <LOOPBACK,UP> mtu 65536
#     inet 127.0.0.1/8 scope host lo
# 只有 loopback，无外部网络

# 清理
docker network prune -f
```
---

### 任务 2: Docker Compose 网络实验 (45min)

创建多容器应用:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
mkdir -p ~/compose-practice && cd ~/compose-practice

cat > docker-compose.yml << 'EOF'
version: '3.8'
services:
  web:
    image: nginx:alpine
    ports:
      - "8080:80"
    networks:
      - frontend
      - backend
    volumes:
      - ./html:/usr/share/nginx/html:ro

  api:
    image: alpine
    command: sh -c "apk add --no-cache curl && while true; do curl -s http://db:80 > /dev/null 2>&1 && echo 'db OK' || echo 'db FAIL'; sleep 5; done"
    networks:
      - backend
    depends_on:
      - db

  db:
    image: nginx:alpine
    networks:
      - backend

networks:
  frontend:
    driver: bridge
  backend:
    driver: bridge
    internal: true
EOF

# 创建测试页面
mkdir -p html
echo "<h1>Docker Compose Network Test</h1><p>Time: $(date)</p>" > html/index.html

# 启动服务
docker-compose up -d

# 查看网络
docker network ls | grep compose
# xxxxxxxx   compose-practice_backend    bridge    local
# xxxxxxxx   compose-practice_frontend   bridge    local

# 测试网络连通性
# web 可以访问 api 和 db (同在 backend 网络)
docker-compose exec web ping -c 2 api
docker-compose exec web ping -c 2 db

# api 可以访问 db
docker-compose exec api ping -c 2 db

# 测试 frontend 网络隔离
curl -s http://localhost:8080 | head -3

# 查看日志
docker-compose logs api

# 清理
docker-compose down
```
---

### 任务 3: Docker 存储实验 (45min)

#### 3.1 Volume 存储

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 创建 Volume
docker volume create my-data
docker volume ls
# DRIVER    VOLUME NAME
# local     my-data

# 查看 Volume 详情
docker volume inspect my-data
# "Mountpoint": "/var/lib/docker/volumes/my-data/_data"

# 使用 Volume 运行容器 (写入数据)
docker run -d --name db \
  -v my-data:/var/lib/data \
  alpine sh -c "echo 'test data from volume' > /var/lib/data/test.txt && sleep 3600"

# 验证数据
docker exec db cat /var/lib/data/test.txt
# test data from volume

# 删除容器 (数据不丢失)
docker stop db && docker rm db

# 数据仍然存在
docker run --rm -v my-data:/data alpine cat /data/test.txt
# test data from volume

# 多容器共享 Volume
docker run -d --name reader -v my-data:/shared alpine sleep 3600
docker exec reader cat /shared/test.txt
# test data from volume

# 清理
docker stop reader && docker rm reader
docker volume rm my-data  # ⚠️ 强制清理，可能杀运行中容器
```
#### 3.2 Bind Mount 存储

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# Bind Mount: 将宿主机目录挂载到容器
mkdir -p ~/bind-test
echo "bind mount data from host" > ~/bind-test/data.txt
echo "config file content" > ~/bind-test/config.yaml

# 挂载到容器 (只读)
docker run --rm -v ~/bind-test:/app:ro alpine cat /app/data.txt
# bind mount data from host

# 挂载到容器 (读写)
docker run --rm -v ~/bind-test:/app alpine sh -c "echo 'written by container' >> /app/data.txt"
cat ~/bind-test/data.txt
# bind mount data from host
# written by container

# 单个文件挂载
docker run --rm -v ~/bind-test/config.yaml:/etc/app/config.yaml:ro alpine cat /etc/app/config.yaml
# config file content

# 清理
rm -rf ~/bind-test  # ⚠️ 删除系统/数据文件
```
#### 3.3 tmpfs 存储

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# tmpfs: 内存存储，容器停止后消失
docker run --rm --tmpfs /tmp:rw,size=100m,mode=1777 alpine sh -c "echo 'temp data' > /tmp/test.txt && df -h /tmp && cat /tmp/test.txt"
# Filesystem           Size    Used Available Use% Mounted on
# tmpfs               100.0M      0    100.0M   0% /tmp
# temp data

# tmpfs 适用场景:
# - 临时文件处理
# - 敏感信息 (不希望写入磁盘)
# - 高速缓存
```
---

### 任务 4: Docker 安全实践 (30min)

#### 4.1 非 root 用户运行

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 不安全: 默认以 root 运行
docker run --rm alpine id
# uid=0(root) gid=0(root)

# 安全: 指定非 root 用户
docker run --rm --user 1000:1000 alpine id
# uid=1000 gid=1000

# Dockerfile 中指定非 root 用户
cat > Dockerfile.secure << 'EOF'
FROM nginx:alpine
RUN adduser -D appuser
USER appuser
EOF

docker build -t secure-nginx -f Dockerfile.secure .
docker run --rm secure-nginx id
# uid=1000(appuser) gid=1000(appuser)
```
#### 4.2 资源限制

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# CPU 限制
docker run --rm --cpus="0.5" alpine sh -c "while true; do :; done" &
# 最多使用 0.5 个 CPU 核心

# 内存限制
docker run --rm -m 256m nginx:alpine
# 限制最大内存 256MB

# 完整资源限制示例
docker run -d \
  --name secured-app \
  --cpus="1.0" \
  -m 512m \
  --pids-limit 100 \
  --restart unless-stopped \
  nginx:alpine

# 查看资源使用
docker stats secured-app --no-stream
# CONTAINER ID   NAME           CPU %   MEM USAGE / LIMIT   MEM %
# xxxxxxxxxxxx   secured-app    0.00%   4.5MiB / 512MiB     0.88%

# 清理
docker stop secured-app && docker rm secured-app
kill %1 2>/dev/null
rm -f Dockerfile.secure
```
#### 4.3 只读文件系统

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 只读根文件系统 + tmpfs 写入目录
docker run --rm \
  --read-only \
  --tmpfs /tmp:rw,noexec \
  --tmpfs /var/run:rw,noexec \
  --tmpfs /var/cache/nginx:rw \
  nginx:alpine sh -c "nginx -t && echo 'read-only fs works'"

# 注意: 只读文件系统需要为所有写入目录提供 tmpfs 或 Volume
```
---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Docker 的 bridge、host、overlay 网络模式各适用于什么场景？**

| 模式 | 适用场景 | K8s 中对应 |
|------|---------|-----------|
| bridge | 单机容器通信 | Pod 内容器通信 (pause 容器的网络栈) |
| host | 高性能网络需求 | hostNetwork: true |
| overlay | 跨主机容器通信 | CNI 插件 (Flannel VXLAN) |

2. **Volume 和 Bind Mount 的区别是什么？什么时候用哪个？**

| 特性 | Volume | Bind Mount |
|------|--------|------------|
| 管理 | Docker 管理 | 用户管理 |
| 位置 | /var/lib/docker/volumes/ | 任意路径 |
| 可移植性 | 好 | 差 |
| 适用 | 生产环境 | 开发环境 |

3. **为什么容器应该以非 root 用户运行？**
   - 容器内 root 与宿主机 root UID 相同 (0)
   - 容器逃逸后攻击者获得宿主机 root 权限
   - 非 root 运行是纵深防御的关键一层

---

## 今日检验

- [ ] 能够创建自定义 Docker 网络并实现容器间通信
- [ ] 能够编写 docker-compose.yml 管理多容器应用和网络隔离
- [ ] 能够使用 Volume 和 Bind Mount 实现数据持久化
- [ ] 了解 Docker 安全最佳实践 (非 root、资源限制、只读文件系统)

---

## 配置参考

### Docker 网络命令速查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker network create <name>              # 创建网络
docker network ls                          # 列出网络
docker network inspect <name>             # 查看网络详情
docker network connect <net> <container>  # 连接容器到网络
docker network disconnect <net> <container>  # 断开网络
docker network rm <name>                  # 删除网络
docker network prune                      # 清理未使用的网络
```
### Docker 存储命令速查

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker volume create <name>               # 创建 Volume
docker volume ls                          # 列出 Volume
docker volume inspect <name>             # 查看 Volume 详情
docker volume rm <name>                   # 删除 Volume  # ⚠️ 强制清理，可能杀运行中容器
docker volume prune                       # 清理未使用的 Volume  # ⚠️ 强制清理，可能杀运行中容器
```
---

## 常见问题

### Q1: 容器间无法通过名称通信？

确保两个容器在同一个自定义 bridge 网络中。默认 bridge 网络不支持名称解析。

### Q2: Bind Mount 后容器内文件权限不对？

使用 `:ro` 只读挂载或调整宿主机文件权限。也可以在 Dockerfile 中指定 USER。

### Q3: Volume 数据如何备份？

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --rm -v my-data:/data -v $(pwd):/backup alpine tar czf /backup/data.tar.gz /data
```
---

## 要点总结

| 主题 | 关键知识点 | K8s 对应 |
|------|-----------|---------|
| Docker 网络 | bridge/host/overlay 模式 | CNI (Flannel/Terway) |
| Docker 存储 | Volume/BindMount/tmpfs | PV/PVC/emptyDir |
| Docker 安全 | 非 root、资源限制、只读 FS | PSS/SecurityContext |

---

## 延伸阅读 (可选)

- `../../容器运行时/06-docker-compose-orchestration.md` - Docker Compose 编排
- `../../容器运行时/08-container-runtime-variants.md` - 容器运行时变体
- `../../容器运行时/99-docker-commands-reference.md` - Docker 命令参考

---

## 明日预告

Day 3 将进入 Linux 基础，学习进程管理、系统架构，这是理解 K8s 底层原理的关键。


<!-- risk-assessed -->
