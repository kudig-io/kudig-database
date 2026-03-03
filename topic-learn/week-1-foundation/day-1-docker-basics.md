# Day 1: Docker 容器基础

> **学习时间**: 4-5 小时 | **主题**: Docker 架构与容器生命周期

---

## 今日目标

- [ ] 理解 Docker 与 Kubernetes 的关系
- [ ] 掌握容器生命周期管理
- [ ] 能够构建和运行自定义 Docker 镜像

---

## 理论学习 (2h)

### 必读文档

1. **Docker 架构总览**
   - 文件: `../../domain-13-docker/01-docker-architecture-overview.md`
   - 重点: Docker Engine 组件、镜像与容器的关系、Docker 与 K8s 的协作

2. **容器生命周期**
   - 文件: `../../domain-13-docker/03-docker-container-lifecycle.md`
   - 重点: 容器状态转换、与 Pod 生命周期的类比

### 阅读要点

- Docker Client、Docker Daemon、Registry 三者的交互流程
- 镜像 (Image) 是只读模板，容器 (Container) 是可写实例
- 容器的创建、运行、暂停、停止、删除状态

---

## 实践任务 (2.5h)

### 任务 1: 基础容器操作 (45min)

```bash
# 拉取镜像
docker pull nginx:latest
docker pull alpine:latest

# 查看本地镜像
docker images

# 运行容器
docker run -d --name my-nginx -p 8080:80 nginx:latest

# 查看运行中的容器
docker ps

# 进入容器
docker exec -it my-nginx /bin/bash

# 查看容器日志
docker logs my-nginx

# 停止和删除容器
docker stop my-nginx
docker rm my-nginx
```

### 任务 2: 构建自定义镜像 (45min)

创建一个简单的 Dockerfile:

```dockerfile
# 创建目录
mkdir -p ~/docker-practice && cd ~/docker-practice

# 创建 Dockerfile
cat > Dockerfile << 'EOF'
FROM nginx:alpine
COPY index.html /usr/share/nginx/html/
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
EOF

# 创建自定义首页
cat > index.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>My First Docker App</title></head>
<body><h1>Hello from Docker!</h1></body>
</html>
EOF

# 构建镜像
docker build -t my-nginx:v1 .

# 运行自定义镜像
docker run -d -p 8080:80 my-nginx:v1

# 访问测试
curl http://localhost:8080
```

### 任务 3: 镜像管理 (30min)

```bash
# 查看镜像历史 (理解分层)
docker history my-nginx:v1

# 标记镜像
docker tag my-nginx:v1 my-nginx:latest

# 导出镜像
docker save -o my-nginx.tar my-nginx:v1

# 导入镜像
docker load -i my-nginx.tar

# 清理未使用的资源
docker system prune -f
```

### 任务 4: 容器资源查看 (30min)

```bash
# 查看容器资源使用
docker stats

# 查看容器详细信息
docker inspect my-nginx

# 查看容器进程
docker top my-nginx

# 查看容器文件系统变化
docker diff my-nginx
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题，可以录音或写笔记:

1. **容器和虚拟机有什么本质区别？**
   - 提示: 从资源隔离、启动速度、资源占用三个角度

2. **Docker 镜像和容器的关系是什么？**
   - 提示: 类比"类"与"实例"的关系

3. **为什么说 Docker 镜像是分层的？这有什么好处？**
   - 提示: Union Filesystem、层复用、构建效率

---

## 今日检验

完成以下操作来验证学习成果:

- [ ] 能够独立拉取、运行、停止、删除容器
- [ ] 能够编写简单的 Dockerfile 并构建镜像
- [ ] 能够解释容器生命周期的各个状态
- [ ] 能够使用 `docker inspect` 查看容器详情

---

## 延伸阅读 (可选)

- `../../domain-13-docker/02-docker-image-build-optimization.md` - 镜像构建优化
- Docker 官方文档: https://docs.docker.com/get-started/

---

## 明日预告

Day 2 将学习 Docker 网络、存储和安全，理解容器间如何通信、数据如何持久化。
