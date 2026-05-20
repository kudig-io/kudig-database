---
title: Headlamp
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- flux
- statefulset
- daemonset
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Headlamp 是什么
- 如何 Headlamp
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Headlamp
- cncf
- landscape
---


# Headlamp

> **成熟度**: Sandbox | **加入时间**: 2022-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://headlamp.dev |
| **GitHub** | https://github.com/headlamp-k8s/headlamp |
| **许可证** | Apache-2.0 |
| **开发语言** | TypeScript, Go |
| **CNCF 分类** | App Definition & Build |
| **维护组织** | Kinvolk (Microsoft) |

---

## 项目概述

Headlamp 是一个现代化的 Kubernetes Web UI，提供直观的集群管理界面。它可以作为桌面应用、Web 应用或集群内应用运行，支持插件扩展系统，允许用户自定义功能。Headlamp 注重用户体验，提供清晰的资源视图和操作界面。

---

## 核心特性

- **多平台支持**: 桌面应用 (Electron)、Web 应用、集群内部署
- **多集群管理**: 单一界面管理多个 Kubernetes 集群
- **插件系统**: 通过插件扩展功能
- **实时更新**: 资源状态实时刷新
- **RBAC 集成**: 基于用户权限显示可用操作
- **YAML 编辑**: 内置 YAML 编辑器
- **日志查看**: 实时 Pod 日志流
- **终端访问**: 直接在浏览器中访问 Pod Shell

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Headlamp Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                     User Interface                        │   │
│  │                                                           │   │
│  │  ┌─────────────────┐  ┌────────────────────────────────┐ │   │
│  │  │  Desktop App    │  │      Web Browser               │ │   │
│  │  │  (Electron)     │  │                                │ │   │
│  │  │  ┌───────────┐  │  │  ┌───────────────────────────┐│ │   │
│  │  │  │ React UI  │  │  │  │   Headlamp React App     ││ │   │
│  │  │  │ ┌───────┐ │  │  │  │  ┌─────────┐ ┌─────────┐ ││ │   │
│  │  │  │ │Cluster│ │  │  │  │  │Workloads│ │ Storage │ ││ │   │
│  │  │  │ │ View  │ │  │  │  │  │  View   │ │  View   │ ││ │   │
│  │  │  │ └───────┘ │  │  │  │  └─────────┘ └─────────┘ ││ │   │
│  │  │  │ ┌───────┐ │  │  │  │  ┌─────────┐ ┌─────────┐ ││ │   │
│  │  │  │ │Network│ │  │  │  │  │ Config  │ │  RBAC   │ ││ │   │
│  │  │  │ │ View  │ │  │  │  │  │  View   │ │  View   │ ││ │   │
│  │  │  │ └───────┘ │  │  │  │  └─────────┘ └─────────┘ ││ │   │
│  │  │  └───────────┘  │  │  └───────────────────────────┘│ │   │
│  │  └─────────────────┘  └────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                    Headlamp Backend                       │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                  Go Server                           │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Proxy     │  │   Plugin    │  │   Auth     │  │ │   │
│  │  │  │   Handler   │  │   Manager   │  │   Handler  │  │ │   │
│  │  │  └──────┬──────┘  └─────────────┘  └────────────┘  │ │   │
│  │  │         │                                           │ │   │
│  │  │  ┌──────▼──────────────────────────────────────┐   │ │   │
│  │  │  │           Kubernetes API Proxy              │   │ │   │
│  │  │  └─────────────────────────────────────────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                 Kubernetes Clusters                       │   │
│  │                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │
│  │  │  Cluster A  │  │  Cluster B  │  │  Cluster C  │      │   │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │      │   │
│  │  │ │API Server│ │  │ │API Server│ │  │ │API Server│ │     │   │
│  │  │ └─────────┘ │  │ └─────────┘ │  │ └─────────┘ │      │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Plugin Ecosystem                       │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │   Flux      │  │  Prometheus │  │   Custom        │  │   │
│  │  │   Plugin    │  │   Plugin    │  │   Plugins       │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **React UI** | 前端界面，基于 React + Material UI |
| **Go Backend** | 后端服务，代理 Kubernetes API |
| **Plugin System** | 插件系统，扩展 UI 功能 |
| **Multi-cluster** | 多集群支持，统一管理 |

---

## 快速开始

### 桌面应用安装

```bash
# macOS (Homebrew)
brew install --cask headlamp

# Windows (Chocolatey)
choco install headlamp

# Linux (Flatpak)
flatpak install flathub io.kinvolk.Headlamp

# 或下载二进制
# https://github.com/headlamp-k8s/headlamp/releases
```

### 集群内部署

```bash
# Helm 安装
helm repo add headlamp https://headlamp-k8s.github.io/headlamp/
helm repo update

helm install headlamp headlamp/headlamp \
  --namespace headlamp \
  --create-namespace

# 访问服务
kubectl port-forward svc/headlamp -n headlamp 8080:80
```

### Manifest 安装

```bash
kubectl apply -f https://raw.githubusercontent.com/headlamp-k8s/headlamp/main/kubernetes-headlamp.yaml
```

---

## 配置示例

### Helm Values 配置

```yaml
# values.yaml
replicaCount: 1

config:
  oidc:
    clientID: "headlamp"
    clientSecret: ""
    issuerURL: "https://dex.example.com"
    scopes: "openid,profile,email,groups"
  
  plugins:
    - name: flux
      url: https://plugins.headlamp.dev/flux

ingress:
  enabled: true
  className: nginx
  annotations:
    nginx.ingress.kubernetes.io/proxy-buffer-size: "128k"
  hosts:
    - host: headlamp.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - hosts:
        - headlamp.example.com
      secretName: headlamp-tls

resources:
  limits:
    cpu: 500m
    memory: 256Mi
  requests:
    cpu: 100m
    memory: 128Mi
```

### ServiceAccount 配置

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: headlamp-admin
  namespace: headlamp

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: headlamp-admin
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
  - kind: ServiceAccount
    name: headlamp-admin
    namespace: headlamp
```

### 创建访问 Token

```bash
# 创建 ServiceAccount Token
kubectl create token headlamp-admin -n headlamp --duration=8760h

# 或使用 Secret
kubectl apply -f - <<EOF
apiVersion: v1
kind: Secret
metadata:
  name: headlamp-admin-token
  namespace: headlamp
  annotations:
    kubernetes.io/service-account.name: headlamp-admin
type: kubernetes.io/service-account-token
EOF

kubectl get secret headlamp-admin-token -n headlamp -o jsonpath='{.data.token}' | base64 -d
```

---

## OIDC 认证配置

### 配合 Dex 使用

```yaml
# Headlamp Helm values
config:
  oidc:
    clientID: "headlamp"
    clientSecret: "headlamp-secret"
    issuerURL: "https://dex.example.com"
    scopes: "openid,profile,email,groups"

# Dex 配置
staticClients:
  - id: headlamp
    name: Headlamp
    secret: headlamp-secret
    redirectURIs:
      - https://headlamp.example.com/oidc-callback
```

### 配合 Keycloak 使用

```yaml
config:
  oidc:
    clientID: "headlamp"
    clientSecret: "keycloak-secret"
    issuerURL: "https://keycloak.example.com/realms/kubernetes"
    scopes: "openid,profile,email"
```

---

## 插件开发

### 插件结构

```
my-headlamp-plugin/
├── package.json
├── src/
│   ├── index.tsx
│   └── components/
├── tsconfig.json
└── webpack.config.js
```

### 基本插件示例

```typescript
// src/index.tsx
import { registerRoute, registerSidebarEntry } from '@kinvolk/headlamp-plugin/lib';
import MyComponent from './components/MyComponent';

// 注册侧边栏入口
registerSidebarEntry({
  parent: 'cluster',
  name: 'my-plugin',
  label: 'My Plugin',
  url: '/my-plugin',
  icon: 'mdi:puzzle',
});

// 注册路由
registerRoute({
  path: '/my-plugin',
  sidebar: 'my-plugin',
  name: 'my-plugin',
  exact: true,
  component: () => <MyComponent />,
});
```

### 自定义资源视图

```typescript
import { 
  K8s,
  registerDetailsViewSectionTo,
  SectionBox 
} from '@kinvolk/headlamp-plugin/lib';

// 为 Pod 详情页添加自定义部分
registerDetailsViewSectionTo('Pod', (props) => {
  const { resource } = props;
  
  return (
    <SectionBox title="Custom Info">
      <p>Pod Name: {resource?.metadata?.name}</p>
      <p>Namespace: {resource?.metadata?.namespace}</p>
    </SectionBox>
  );
});
```

### 构建和安装插件

```bash
# 初始化插件项目
npx @kinvolk/headlamp-plugin create my-plugin
cd my-plugin

# 开发模式
npm run start

# 构建生产版本
npm run build

# 打包插件
npm run package

# 加载插件（桌面应用）
# 将构建产物复制到 ~/.config/Headlamp/plugins/
```

---

## 功能特性

### 资源管理

| 资源类型 | 功能 |
|:---|:---|
| **Workloads** | Deployments, StatefulSets, DaemonSets, Jobs, CronJobs |
| **Networking** | Services, Ingresses, NetworkPolicies, Endpoints |
| **Storage** | PV, PVC, StorageClasses, ConfigMaps, Secrets |
| **Configuration** | ConfigMaps, Secrets, ResourceQuotas, LimitRanges |
| **RBAC** | Roles, RoleBindings, ClusterRoles, ServiceAccounts |

### 内置功能

- **资源创建**: 通过 YAML 或表单创建资源
- **实时日志**: 流式查看 Pod 日志，支持多容器
- **Terminal**: 浏览器内 Pod Shell 访问
- **事件查看**: 集群和资源级别事件
- **资源编辑**: 在线 YAML 编辑器
- **删除确认**: 资源删除保护

---

## 多集群配置

### 桌面应用添加集群

```bash
# 自动检测 kubeconfig
# Headlamp 会读取 ~/.kube/config

# 手动添加集群
# 通过 UI: Settings -> Clusters -> Add Cluster
# 输入集群名称、API Server URL 和认证信息
```

### 集群内配置多集群

```yaml
# ConfigMap 配置多集群
apiVersion: v1
kind: ConfigMap
metadata:
  name: headlamp-config
  namespace: headlamp
data:
  config.yaml: |
    clusters:
      - name: production
        server: https://prod-k8s.example.com
        authType: ServiceAccount
      - name: staging
        server: https://staging-k8s.example.com
        authType: OIDC
```

---

## 最佳实践

1. **RBAC 配置**: 为不同用户创建适当权限的 ServiceAccount
2. **OIDC 集成**: 生产环境使用 OIDC 而非 Token 认证
3. **插件管理**: 只安装必要的插件，定期更新
4. **Ingress 安全**: 启用 TLS，配置访问控制
5. **资源限制**: 为 Headlamp Pod 设置资源限制
6. **审计日志**: 启用 Kubernetes 审计日志跟踪操作

---

## 参考资源

- [官方文档](https://headlamp.dev/docs/latest/)
- [GitHub Repo](https://github.com/headlamp-k8s/headlamp)
- [插件开发指南](https://headlamp.dev/docs/latest/development/plugins/)
- [插件市场](https://headlamp.dev/docs/latest/installation/plugins/)
- [发布说明](https://github.com/headlamp-k8s/headlamp/releases)

---

**维护者**: Kudig Team | **许可证**: MIT
