# Podman Desktop

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://podman-desktop.io/ |
| **GitHub** | https://github.com/containers/podman-desktop |
| **许可证** | Apache-2.0 |
| **开发语言** | TypeScript, Svelte |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Podman Desktop 是一个开源的图形化容器管理工具，为开发者提供在本地管理容器、Pod 和 Kubernetes 的统一桌面体验。它支持 Podman、Docker、Lima 等多种容器引擎，并提供可扩展的插件系统，帮助开发者在 macOS、Windows 和 Linux 上无缝进行云原生开发。

### 核心特性

- **多引擎支持**: Podman, Docker, Lima, CRC (OpenShift Local) 等
- **容器管理**: 创建、启动、停止、删除容器和 Pod，管理镜像和卷
- **Kubernetes 集成**: 内置 Kind, Minikube 支持，部署到本地集群
- **扩展系统**: 丰富的插件生态，支持自定义扩展开发
- **无 Daemon 架构**: 基于 Podman 的 daemonless 设计，增强安全性
- **Compose 支持**: 兼容 Docker Compose 和 Podman Compose 工作流
- **镜像构建**: 集成 Containerfile/Dockerfile 构建能力
- **跨平台**: 支持 macOS (Intel/Apple Silicon), Windows, Linux

---

## 架构设计

```
┌────────────────────────────────────────────────┐
│            Podman Desktop (Electron)            │
│                                                  │
│  ┌──────────────────────────────────────────┐   │
│  │           Svelte UI Layer                 │   │
│  │  ┌─────────┐ ┌──────┐ ┌──────────────┐  │   │
│  │  │Container│ │Image │ │ Kubernetes   │  │   │
│  │  │  View   │ │ View │ │   View       │  │   │
│  │  └─────────┘ └──────┘ └──────────────┘  │   │
│  └────────────────────┬─────────────────────┘   │
│                       │                          │
│  ┌────────────────────┴─────────────────────┐   │
│  │         Extension API Layer               │   │
│  │  ┌──────────┐ ┌───────┐ ┌────────────┐  │   │
│  │  │ Provider │ │ Tray  │ │ Registry   │  │   │
│  │  │   API    │ │  API  │ │    API     │  │   │
│  │  └──────────┘ └───────┘ └────────────┘  │   │
│  └────────────────────┬─────────────────────┘   │
│                       │                          │
│  ┌────────────────────┴─────────────────────┐   │
│  │         Container Engine Layer            │   │
│  │  ┌────────┐ ┌───────┐ ┌──────┐ ┌─────┐ │   │
│  │  │Podman  │ │Docker │ │Lima  │ │ CRC │ │   │
│  │  │Engine  │ │Engine │ │      │ │     │ │   │
│  │  └────────┘ └───────┘ └──────┘ └─────┘ │   │
│  └──────────────────────────────────────────┘   │
└────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS (Homebrew)
brew install podman-desktop

# Windows (Winget)
winget install RedHat.PodmanDesktop

# Linux (Flatpak)
flatpak install flathub io.podman_desktop.PodmanDesktop
```

### 初始化 Podman Machine (macOS/Windows)

```bash
# Podman Desktop 会自动引导初始化，也可手动执行
podman machine init
podman machine start

# 验证
podman info
```

### 命令行操作

```bash
# 容器管理
podman run -d --name web -p 8080:80 nginx:latest
podman ps
podman logs web
podman stop web

# Pod 管理
podman pod create --name my-pod -p 8080:80
podman run -d --pod my-pod nginx:latest
podman pod ps

# 镜像管理
podman build -t my-app:latest .
podman images
podman push my-app:latest registry.example.com/my-app:latest
```

---

## 配置详解

### Kubernetes 集群管理

```bash
# 在 Podman Desktop 中创建 Kind 集群
# UI: Settings > Kubernetes > Create Kind Cluster

# 等效命令行
kind create cluster --name dev-cluster --config - <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
  - role: control-plane
    extraPortMappings:
      - containerPort: 30000
        hostPort: 30000
        protocol: TCP
  - role: worker
  - role: worker
EOF
```

### 容器从本地到 Kubernetes

```bash
# 1. 构建镜像
podman build -t my-app:v1.0 .

# 2. 通过 Podman Desktop UI 生成 Kubernetes YAML
# UI: Container > my-app > Generate Kube
podman generate kube my-app > my-app.yaml

# 3. 部署到本地 Kind 集群
# 先加载镜像到 Kind
kind load docker-image my-app:v1.0 --name dev-cluster

# 4. 应用到集群
kubectl apply -f my-app.yaml
```

### Compose 集成

```yaml
# docker-compose.yml / podman-compose.yml
version: "3.8"
services:
  frontend:
    build: ./frontend
    ports:
      - "3000:3000"
    depends_on:
      - api
  api:
    build: ./api
    ports:
      - "8080:8080"
    environment:
      - DATABASE_URL=postgres://user:pass@db:5432/app
    depends_on:
      - db
  db:
    image: postgres:16
    volumes:
      - db-data:/var/lib/postgresql/data
    environment:
      - POSTGRES_PASSWORD=pass
      - POSTGRES_USER=user
      - POSTGRES_DB=app

volumes:
  db-data:
```

```bash
# Podman Desktop 自动检测 Compose 文件
# 也可命令行启动
podman compose up -d
podman compose ps
podman compose logs -f
podman compose down
```

---

## 扩展开发

### 扩展结构

```
my-extension/
├── package.json
├── src/
│   └── extension.ts
├── icon.png
└── README.md
```

### 扩展示例

```typescript
// src/extension.ts
import * as extensionApi from '@podman-desktop/api';

export async function activate(context: extensionApi.ExtensionContext): Promise<void> {
  // 注册容器 Provider
  const provider = extensionApi.provider.createProvider({
    name: 'My Custom Runtime',
    id: 'my-runtime',
    status: 'ready',
    images: {
      icon: './icon.png',
    },
  });
  context.subscriptions.push(provider);

  // 注册命令
  const command = extensionApi.commands.registerCommand(
    'my-extension.hello',
    async () => {
      await extensionApi.window.showInformationMessage('Hello from extension!');
    }
  );
  context.subscriptions.push(command);

  // 添加状态栏项
  const statusBar = extensionApi.window.createStatusBarItem();
  statusBar.text = 'My Extension';
  statusBar.command = 'my-extension.hello';
  statusBar.show();
  context.subscriptions.push(statusBar);
}

export function deactivate(): void {
  console.log('Extension deactivated');
}
```

### package.json

```json
{
  "name": "my-extension",
  "displayName": "My Custom Extension",
  "version": "1.0.0",
  "publisher": "my-org",
  "engines": {
    "podman-desktop": ">=1.0.0"
  },
  "main": "./dist/extension.js",
  "contributes": {
    "commands": [
      {
        "command": "my-extension.hello",
        "title": "My Extension: Hello"
      }
    ]
  }
}
```

---

## 常用扩展

| 扩展 | 功能 |
|:---|:---|
| **Kind** | 本地 Kubernetes 集群管理 |
| **Minikube** | Minikube 集群集成 |
| **OpenShift Local** | Red Hat OpenShift 本地开发 |
| **Lima** | macOS 上的 Linux VM 管理 |
| **Bootc** | Bootable Container 镜像构建 |
| **AI Lab** | 本地 AI/LLM 模型运行 |
| **Headlamp** | Kubernetes Dashboard 集成 |

---

## 最佳实践

1. **Rootless 模式**: 优先使用 Podman 的 rootless 模式提升安全性
2. **资源管理**: 在 Settings 中合理配置 Podman Machine 的 CPU 和内存
3. **镜像清理**: 定期使用 `podman system prune` 清理未使用的资源
4. **Compose 优先**: 多容器开发使用 Compose 文件管理，便于团队共享
5. **Kind 开发**: 使用 Kind 集群进行本地 Kubernetes 开发和测试
6. **扩展开发**: 利用扩展 API 自定义开发工作流

---

## 参考资源

- [Podman Desktop 官方文档](https://podman-desktop.io/docs/intro)
- [Podman Desktop GitHub](https://github.com/containers/podman-desktop)
- [扩展开发指南](https://podman-desktop.io/docs/extensions/developing)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
