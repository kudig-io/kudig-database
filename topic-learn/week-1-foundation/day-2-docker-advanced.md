# Day 2: Docker 网络 + 存储 + 安全

> **学习时间**: 4-5 小时 | **主题**: Docker 高级特性

---

## 今日目标

- [ ] 理解 Docker 网络模式 (bridge/host/overlay)
- [ ] 掌握 Docker 存储机制 (Volume/Bind Mount)
- [ ] 了解 Docker 安全最佳实践

---

## 理论学习 (2h)

### 必读文档

1. **Docker 网络深入**
   - 文件: `../../domain-13-docker/04-docker-networking-deep-dive.md`
   - 重点: bridge、host、overlay 网络模式的区别和使用场景

2. **Docker 存储**
   - 文件: `../../domain-13-docker/05-docker-storage-volumes.md`
   - 重点: Volume vs Bind Mount vs tmpfs 的选择

### 补充阅读

3. **Docker 安全最佳实践**
   - 文件: `../../domain-13-docker/07-docker-security-best-practices.md`
   - 重点: 非 root 用户、只读文件系统、资源限制

---

## 实践任务 (2.5h)

### 任务 1: Docker 网络实验 (1h)

```bash
# 创建自定义网络
docker network create my-network

# 查看网络列表
docker network ls

# 运行两个容器在同一网络
docker run -d --name web --network my-network nginx:alpine
docker run -d --name app --network my-network alpine sleep 3600

# 测试容器间通信 (通过容器名)
docker exec app ping -c 3 web

# 查看网络详情
docker network inspect my-network

# 测试不同网络模式
# host 模式
docker run -d --name host-nginx --network host nginx:alpine

# 查看端口映射
docker port host-nginx

# 清理
docker stop web app host-nginx
docker rm web app host-nginx
docker network rm my-network
```

### 任务 2: Docker Compose 网络实验 (45min)

创建多容器应用:

```bash
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
    command: sleep 3600
    networks:
      - backend

  db:
    image: alpine
    command: sleep 3600
    networks:
      - backend

networks:
  frontend:
  backend:
EOF

# 创建测试页面
mkdir -p html
echo "<h1>Docker Compose Network Test</h1>" > html/index.html

# 启动服务
docker-compose up -d

# 查看网络
docker network ls | grep compose

# 测试网络隔离: web 可以访问 api 和 db
docker-compose exec web ping -c 2 api
docker-compose exec web ping -c 2 db

# 清理
docker-compose down
```

### 任务 3: Docker 存储实验 (45min)

```bash
# Volume 存储
docker volume create my-data
docker volume ls

# 使用 Volume 运行容器
docker run -d --name db -v my-data:/var/lib/data alpine sh -c "echo 'test data' > /var/lib/data/test.txt && sleep 3600"

# 验证数据持久化
docker exec db cat /var/lib/data/test.txt
docker stop db && docker rm db

# 数据仍然存在
docker run --rm -v my-data:/data alpine cat /data/test.txt

# Bind Mount 存储
mkdir -p ~/bind-test
echo "bind mount data" > ~/bind-test/data.txt

docker run --rm -v ~/bind-test:/app alpine cat /app/data.txt

# tmpfs 存储 (内存存储，容器停止后消失)
docker run --rm --tmpfs /tmp:rw,size=100m alpine df -h /tmp

# 清理
docker volume rm my-data
```

### 任务 4: 命令速查练习 (30min)

参考 `../../domain-13-docker/99-docker-commands-reference.md`，练习以下命令:

```bash
# 容器管理
docker create    # 创建容器但不启动
docker start     # 启动已创建的容器
docker restart   # 重启容器
docker pause     # 暂停容器
docker unpause   # 恢复容器

# 镜像管理
docker pull      # 拉取镜像
docker push      # 推送镜像
docker rmi       # 删除镜像
docker prune     # 清理悬空镜像

# 信息查看
docker info      # Docker 系统信息
docker version   # 版本信息
docker events    # 实时事件流
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Docker 的 bridge、host、overlay 网络模式各适用于什么场景？**

2. **Volume 和 Bind Mount 的区别是什么？什么时候用哪个？**

3. **为什么容器应该以非 root 用户运行？**

---

## 今日检验

- [ ] 能够创建自定义 Docker 网络并实现容器间通信
- [ ] 能够编写简单的 docker-compose.yml 管理多容器应用
- [ ] 能够使用 Volume 实现数据持久化
- [ ] 熟悉 Docker 常用命令，无需频繁查手册

---

## 延伸阅读 (可选)

- `../../domain-13-docker/06-docker-compose-orchestration.md` - Docker Compose 编排
- `../../domain-13-docker/08-container-runtime-variants.md` - 容器运行时变体

---

## 明日预告

Day 3 将进入 Linux 基础，学习进程管理、系统架构，这是理解 K8s 底层原理的关键。
