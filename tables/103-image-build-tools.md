# 103 - 容器镜像构建工具 (Container Image Build)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 构建工具技术对比

| 工具 (Tool) | 运行环境 (Runtime) | 核心特性 (Features) | 生产场景 |
|------------|------------------|-------------------|---------|
| **Kaniko** | Kubernetes Pod | 无需 Docker Daemon | CI/CD 流水线首选 |
| **Buildah** | 任意环境 | OCI 标准、Rootless | 安全敏感环境 |
| **BuildKit** | Docker/Standalone | 并行构建、缓存优化 | 本地开发、极致性能 |
| **Jib** | Maven/Gradle 插件 | 无需 Dockerfile | Java 应用快速构建 |

## Kaniko 生产级配置

### 1. 在 Kubernetes 中构建
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: kaniko-build
spec:
  containers:
  - name: kaniko
    image: gcr.io/kaniko-project/executor:latest
    args:
      - "--dockerfile=Dockerfile"
      - "--context=git://github.com/myorg/myapp.git#refs/heads/main"
      - "--destination=registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0"
      - "--cache=true"
      - "--cache-repo=registry.cn-hangzhou.aliyuncs.com/myns/cache"
    volumeMounts:
    - name: docker-config
      mountPath: /kaniko/.docker/
  volumes:
  - name: docker-config
    secret:
      secretName: regcred
      items:
      - key: .dockerconfigjson
        path: config.json
  restartPolicy: Never
```

### 2. 缓存优化策略
- 启用 `--cache=true` 复用层缓存
- 使用专用 `--cache-repo` 存储中间层
- 多阶段构建减少最终镜像体积

## BuildKit 高级特性

### 1. 并行构建
```dockerfile
# syntax=docker/dockerfile:1.4
FROM golang:1.21 AS build-backend
WORKDIR /app
COPY go.* ./
RUN go mod download
COPY . .
RUN go build -o server

FROM node:18 AS build-frontend
WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

FROM alpine:latest
COPY --from=build-backend /app/server /server
COPY --from=build-frontend /app/dist /static
CMD ["/server"]
```

### 2. 秘钥挂载 (避免泄露)
```dockerfile
# syntax=docker/dockerfile:1.4
RUN --mount=type=secret,id=npmrc,target=/root/.npmrc \
    npm install
```

## Buildah Rootless 构建

### 无特权构建
```bash
# 无需 root 权限
buildah bud -t myapp:v1.0 .
buildah push myapp:v1.0 docker://registry.example.com/myapp:v1.0

# 检查镜像
buildah images
```

## 镜像优化最佳实践

| 实践 (Practice) | 效果 (Impact) |
|----------------|--------------|
| **多阶段构建** | 减少 70%+ 镜像体积 |
| **Distroless 基础镜像** | 最小攻击面 |
| **Layer 缓存** | 加速 80% 构建时间 |
| **.dockerignore** | 排除无关文件 |
| **固定基础镜像 Digest** | 确保可复现构建 |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)