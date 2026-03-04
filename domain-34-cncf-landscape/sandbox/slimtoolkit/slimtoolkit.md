# SlimToolkit

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/slimtoolkit/slim |
| **官网** | https://slimtoolkit.org/ |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Container Optimization |
| **支持平台** | Linux / macOS |

---

## 项目概述

SlimToolkit（原名 DockerSlim）是一个容器镜像优化工具，能够自动分析和瘦身容器镜像，将镜像大小缩减高达 30 倍，同时提升安全性。它通过动态分析识别应用实际需要的文件，移除不必要的组件，生成最小化、安全加固的生产镜像。

### 核心价值

- **极致瘦身**: 将 GB 级镜像缩减为 MB 级，最高 30x 压缩比
- **安全加固**: 移除不必要的 shell、包管理器和攻击面
- **零代码改动**: 无需修改 Dockerfile 或应用代码
- **自动分析**: 智能识别运行时依赖
- **多运行时支持**: 支持各种语言和框架的应用

---

## 核心特性

### 镜像优化效果

```
┌─────────────────────────────────────────────────────────────────┐
│                    SlimToolkit 优化效果                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  原始镜像                        优化后镜像                       │
│  ┌──────────────────┐           ┌──────────────────┐            │
│  │ node:18          │           │ node:18-slim     │            │
│  │ 1.1 GB           │  ──30x──▶ │ 35 MB            │            │
│  └──────────────────┘           └──────────────────┘            │
│                                                                  │
│  ┌──────────────────┐           ┌──────────────────┐            │
│  │ python:3.11      │           │ python:3.11-slim │            │
│  │ 920 MB           │  ──25x──▶ │ 38 MB            │            │
│  └──────────────────┘           └──────────────────┘            │
│                                                                  │
│  ┌──────────────────┐           ┌──────────────────┐            │
│  │ golang:1.21      │           │ golang:1.21-slim │            │
│  │ 850 MB           │  ──50x──▶ │ 17 MB            │            │
│  └──────────────────┘           └──────────────────┘            │
│                                                                  │
│  ┌──────────────────┐           ┌──────────────────┐            │
│  │ ubuntu:22.04     │           │ ubuntu:22.04-slim│            │
│  │ 77 MB            │  ──10x──▶ │ 7.5 MB           │            │
│  └──────────────────┘           └──────────────────┘            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 命令模式

| 命令 | 功能 | 描述 |
|:---|:---|:---|
| **build** | 构建优化镜像 | 分析并生成最小化镜像 |
| **xray** | 镜像分析 | 分析镜像层和文件 |
| **lint** | Dockerfile 检查 | 检查 Dockerfile 最佳实践 |
| **profile** | 运行时分析 | 收集运行时依赖信息 |
| **merge** | 镜像合并 | 合并多个镜像层 |
| **registry** | 仓库操作 | 镜像仓库管理 |
| **vulnerability** | 漏洞扫描 | 检测镜像安全漏洞 |

---

## 架构设计

```
┌───────────────────────────────────────────────────────────────────┐
│                        SlimToolkit                                 │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    Command Interface                          │ │
│  │   ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐    │ │
│  │   │build │ │ xray │ │ lint │ │profile│ │merge │ │ vuln │    │ │
│  │   └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘    │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                     Analysis Engine                           │ │
│  │                                                                │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │ │
│  │  │   Static    │  │  Dynamic    │  │  Syscall    │           │ │
│  │  │  Analysis   │  │  Analysis   │  │   Tracer    │           │ │
│  │  │             │  │             │  │  (ptrace)   │           │ │
│  │  │ - Layer     │  │ - Container │  │             │           │ │
│  │  │   inspect   │  │   runtime   │  │ - File I/O  │           │ │
│  │  │ - File scan │  │ - HTTP      │  │ - Network   │           │ │
│  │  │ - Manifest  │  │   probes    │  │ - Process   │           │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘           │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    Image Builder                              │ │
│  │                                                                │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │              Optimized Image Generator                   │ │ │
│  │  │                                                          │ │ │
│  │  │  1. Extract required files                               │ │ │
│  │  │  2. Apply security hardening                             │ │ │
│  │  │  3. Generate minimal filesystem                          │ │ │
│  │  │  4. Build FROM scratch image                             │ │ │
│  │  │                                                          │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│                              ▼                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                                                                │ │
│  │   Original Image: 1.1 GB  ──────▶  Slim Image: 35 MB         │ │
│  │                                                                │ │
│  └──────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS (Homebrew)
brew install slim

# Linux (下载二进制)
curl -sL https://raw.githubusercontent.com/slimtoolkit/slim/master/scripts/install-slim.sh | sudo -E bash -

# Docker 方式运行
docker pull dslim/slim

# 验证安装
slim --version
```

### 基本用法

```bash
# 优化 Node.js 镜像
slim build my-node-app:latest

# 输出:
# cmd=build info=image.optimized id='my-node-app.slim' size='35 MB' (was '1.1 GB')

# 优化 Python 镜像
slim build my-python-app:latest --http-probe

# 指定输出镜像名称
slim build my-app:latest --target my-app:slim

# 保留特定文件
slim build my-app:latest --include-path /app/config
```

### 镜像分析 (xray)

```bash
# 分析镜像结构
slim xray my-node-app:latest

# 输出:
# Layer[0]: 65 MB
#   - /usr/local/lib/node_modules (45 MB)
#   - /var/cache/apt (15 MB)
#   - /usr/share/doc (5 MB)
# Layer[1]: 25 MB
#   - /app/node_modules (20 MB)
#   - /app/src (5 MB)

# 导出分析报告
slim xray my-app:latest --report json > xray-report.json
```

---

## 高级功能

### HTTP 探针配置

```bash
# 自动 HTTP 探针（默认端口）
slim build my-web-app:latest --http-probe

# 指定探针端口
slim build my-app:latest --http-probe-ports 8080,3000

# 自定义探针路径
slim build my-app:latest \
  --http-probe-cmd /health \
  --http-probe-cmd /api/status \
  --http-probe-cmd-file probe-commands.json
```

```json
// probe-commands.json
{
  "commands": [
    {
      "resource": "/",
      "method": "GET"
    },
    {
      "resource": "/api/users",
      "method": "GET",
      "headers": {
        "Authorization": "Bearer test-token"
      }
    },
    {
      "resource": "/api/data",
      "method": "POST",
      "body": "{\"test\": true}"
    }
  ]
}
```

### 文件包含/排除

```bash
# 包含特定路径
slim build my-app:latest \
  --include-path /app/config \
  --include-path /app/certs \
  --include-path /etc/ssl/certs

# 排除不需要的文件
slim build my-app:latest \
  --exclude-pattern '*.md' \
  --exclude-pattern '*.txt' \
  --exclude-pattern 'test/*'

# 包含可执行文件和依赖
slim build my-app:latest \
  --include-exe /usr/bin/curl \
  --include-shell
```

### 安全加固选项

```bash
# 移除 shell（默认行为）
slim build my-app:latest --remove-file-artifacts

# 保留 shell（调试用）
slim build my-app:latest --include-shell

# 设置 seccomp profile
slim build my-app:latest --seccomp-profile-name default.json

# 禁用 setuid 位
slim build my-app:latest --remove-suid-bits
```

### Dockerfile Lint

```bash
# 检查 Dockerfile
slim lint Dockerfile

# 输出:
# [WARN] DL3008: Pin versions in apt-get install
# [WARN] DL3009: Delete apt-get lists after installing
# [INFO] DL3015: Avoid additional packages with apt-get
# [PASS] DL3020: Use COPY instead of ADD

# 指定规则集
slim lint Dockerfile --lint-rules .slim-lint.yaml
```

```yaml
# .slim-lint.yaml
rules:
  DL3008:  # apt-get version pinning
    enabled: true
    severity: warning
  DL3009:  # apt-get clean
    enabled: true
    severity: error
  SC2086:  # shellcheck: quote variables
    enabled: true
    severity: warning
```

---

## 运行时探测

### 动态分析流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    Dynamic Analysis Process                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Start Container                                              │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  docker run --name slim-probe my-app:latest               │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  2. Attach System Call Tracer                                    │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  ptrace(PTRACE_ATTACH, pid)                               │  │
│  │  Monitor: open(), read(), write(), exec(), stat()         │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  3. Execute HTTP Probes                                          │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  GET /                                                     │  │
│  │  GET /api/health                                           │  │
│  │  POST /api/data                                            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  4. Collect File Access Data                                     │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  /app/server.js          (accessed)                        │  │
│  │  /usr/lib/libc.so.6      (accessed)                        │  │
│  │  /usr/share/doc/*        (not accessed) ──▶ REMOVE         │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  5. Generate Optimized Image                                     │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  FROM scratch                                              │  │
│  │  COPY --from=analysis /required/files /                    │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 自定义探测脚本

```bash
# 使用自定义命令进行探测
slim build my-app:latest \
  --exec "curl http://localhost:8080/api/test" \
  --exec "wget -O- http://localhost:8080/health"

# 探测文件
slim build my-app:latest --exec-file probe-script.sh
```

```bash
#!/bin/bash
# probe-script.sh
# 执行各种 API 调用以触发代码路径

curl -X GET http://localhost:8080/
curl -X GET http://localhost:8080/api/users
curl -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username": "test", "password": "test"}'
curl -X GET http://localhost:8080/api/data?page=1
```

---

## CI/CD 集成

### GitHub Actions

```yaml
# .github/workflows/build.yml
name: Build and Optimize Image

on:
  push:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Build original image
        run: docker build -t my-app:latest .
      
      - name: Install SlimToolkit
        run: |
          curl -sL https://raw.githubusercontent.com/slimtoolkit/slim/master/scripts/install-slim.sh | sudo -E bash -
      
      - name: Optimize image
        run: |
          slim build my-app:latest \
            --target my-app:slim \
            --http-probe \
            --continue-after 60
      
      - name: Compare sizes
        run: |
          echo "Original: $(docker images my-app:latest --format '{{.Size}}')"
          echo "Optimized: $(docker images my-app:slim --format '{{.Size}}')"
      
      - name: Push optimized image
        run: |
          docker tag my-app:slim ${{ secrets.REGISTRY }}/my-app:slim
          docker push ${{ secrets.REGISTRY }}/my-app:slim
```

### GitLab CI

```yaml
# .gitlab-ci.yml
stages:
  - build
  - optimize
  - push

build:
  stage: build
  script:
    - docker build -t $CI_REGISTRY_IMAGE:latest .
    - docker push $CI_REGISTRY_IMAGE:latest

optimize:
  stage: optimize
  image: dslim/slim
  script:
    - slim build $CI_REGISTRY_IMAGE:latest --target $CI_REGISTRY_IMAGE:slim
  artifacts:
    reports:
      dotenv: slim-report.env

push:
  stage: push
  script:
    - docker push $CI_REGISTRY_IMAGE:slim
```

---

## 漏洞扫描

```bash
# 扫描镜像漏洞
slim vulnerability my-app:latest

# 输出:
# Vulnerability Report for my-app:latest
# ├── CRITICAL: 2
# │   ├── CVE-2024-1234: openssl 1.1.1k (upgrade to 1.1.1l)
# │   └── CVE-2024-5678: libcurl 7.74.0 (upgrade to 7.79.0)
# ├── HIGH: 5
# ├── MEDIUM: 12
# └── LOW: 23

# 优化后再扫描
slim build my-app:latest --target my-app:slim
slim vulnerability my-app:slim

# 输出:
# Vulnerability Report for my-app:slim
# ├── CRITICAL: 0  (removed with unused packages)
# ├── HIGH: 1
# ├── MEDIUM: 3
# └── LOW: 5
```

---

## 最佳实践

### 生产环境配置

```bash
# 完整的生产构建命令
slim build my-app:latest \
  --target my-app:production \
  --http-probe \
  --http-probe-ports 8080 \
  --http-probe-cmd /health \
  --http-probe-cmd /ready \
  --continue-after 120 \
  --include-path /app/config \
  --include-path /etc/ssl/certs \
  --include-cert-all \
  --remove-file-artifacts \
  --tag-fat my-app:debug \
  --report json > slim-report.json
```

### 多阶段构建配合

```dockerfile
# Dockerfile
FROM node:18 AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
RUN npm run build

FROM node:18-slim AS runtime
WORKDIR /app
COPY --from=builder /app/dist ./dist
COPY --from=builder /app/node_modules ./node_modules
CMD ["node", "dist/server.js"]

# 然后使用 slim 进一步优化 runtime 镜像
# slim build my-app:runtime --target my-app:slim
```

### 语言特定优化

```bash
# Node.js 应用
slim build node-app:latest \
  --http-probe \
  --include-path /app/node_modules

# Python 应用
slim build python-app:latest \
  --http-probe \
  --include-path /usr/local/lib/python3.11

# Go 应用（通常已经很小）
slim build go-app:latest \
  --include-exe /app/server
```

---

## 参考资源

- [GitHub 仓库](https://github.com/slimtoolkit/slim)
- [官方文档](https://slimtoolkit.org/docs)
- [示例集合](https://github.com/slimtoolkit/examples)
- [Dockerfile 最佳实践](https://docs.docker.com/develop/develop-images/dockerfile_best-practices/)
- [CNCF Sandbox](https://www.cncf.io/sandbox-projects/)
- [容器安全指南](https://cheatsheetseries.owasp.org/cheatsheets/Docker_Security_Cheat_Sheet.html)

---

**维护者**: Kudig Team | **许可证**: MIT
