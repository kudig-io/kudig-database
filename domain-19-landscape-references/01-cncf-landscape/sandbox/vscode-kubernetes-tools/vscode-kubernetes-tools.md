---
title: VS Code Kubernetes Tools
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- helm
- docker
- statefulset
- daemonset
- ingress
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- VS Code Kubernetes Tools 是什么
- 如何 VS Code Kubernetes Tools
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- VS
- Code
- Kubernetes
- Tools
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
---

title: VS Code Kubernetes Tools
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- docker
- statefulset
- daemonset
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- VS Code Kubernetes Tools 是什么
- 如何 VS Code Kubernetes Tools
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- VS
- Code
- Kubernetes
- Tools
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# VS Code Kubernetes Tools

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/vscode-kubernetes-tools/vscode-kubernetes-tools |
| **VS Code Marketplace** | https://marketplace.visualstudio.com/items?itemName=ms-kubernetes-tools.vscode-kubernetes-tools |
| **许可证** | MIT |
| **开发语言** | TypeScript |
| **CNCF 分类** | Developer Tools |
| **下载量** | 5M+ |

---

## 项目概述

VS Code Kubernetes Tools 是一个功能强大的 Visual Studio Code 扩展，为 Kubernetes 开发者提供完整的开发体验。它集成了集群浏览、YAML 编辑、资源管理、日志查看、调试等功能，让开发者可以在 IDE 中完成几乎所有 Kubernetes 操作，大幅提升开发效率。

### 核心价值

- **集群管理**: 多集群连接与资源浏览
- **智能编辑**: YAML 自动补全、验证、悬浮提示
- **快速部署**: 一键部署和资源管理
- **调试支持**: 远程调试运行中的容器
- **Helm 集成**: Helm Charts 开发与部署

---

## 核心特性

### 功能概览

```
┌─────────────────────────────────────────────────────────────────┐
│                VS Code Kubernetes Tools                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                   VS Code IDE                              │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │             Kubernetes Extension                     │  │  │
│  │  │                                                      │  │  │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐          │  │  │
│  │  │  │ Cluster  │  │  YAML    │  │  Helm    │          │  │  │
│  │  │  │ Explorer │  │  Editor  │  │  Tools   │          │  │  │
│  │  │  └──────────┘  └──────────┘  └──────────┘          │  │  │
│  │  │                                                      │  │  │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐          │  │  │
│  │  │  │   Log    │  │ Terminal │  │ Debugger │          │  │  │
│  │  │  │  Viewer  │  │  Access  │  │          │          │  │  │
│  │  │  └──────────┘  └──────────┘  └──────────┘          │  │  │
│  │  │                                                      │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                          │                                 │  │
│  └──────────────────────────│─────────────────────────────────┘  │
│                             │                                    │
│                             ▼                                    │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                 Kubernetes Clusters                        │  │
│  │                                                            │  │
│  │  ┌─────────┐    ┌─────────┐    ┌─────────┐               │  │
│  │  │  Dev    │    │ Staging │    │  Prod   │               │  │
│  │  │ Cluster │    │ Cluster │    │ Cluster │               │  │
│  │  └─────────┘    └─────────┘    └─────────┘               │  │
│  │                                                            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 功能列表

| 功能 | 描述 | 快捷键/命令 |
|:---|:---|:---|
| **集群浏览器** | 可视化浏览集群资源 | 侧边栏 Kubernetes 面板 |
| **YAML 智能补全** | 资源定义自动补全 | `Ctrl+Space` |
| **资源验证** | YAML 语法和 Schema 验证 | 自动 |
| **应用部署** | 从当前文件部署资源 | `Kubernetes: Apply` |
| **日志查看** | 实时查看 Pod 日志 | `Kubernetes: Logs` |
| **终端连接** | 连接到容器终端 | `Kubernetes: Terminal` |
| **端口转发** | 本地端口转发 | `Kubernetes: Port Forward` |
| **Helm 管理** | Chart 部署和模板预览 | `Helm: Template` |

---

## 安装配置

### 安装扩展

```bash
# VS Code 内安装
# 1. 打开 Extensions (Ctrl+Shift+X)
# 2. 搜索 "Kubernetes"
# 3. 安装 "Kubernetes" by Microsoft

# 或通过命令行安装
code --install-extension ms-kubernetes-tools.vscode-kubernetes-tools
```

### 前置依赖

```bash
# kubectl (必需)
# macOS
brew install kubectl

# Linux
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
sudo install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl

# Helm (可选，用于 Helm 功能)
brew install helm  # macOS
# 或
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash
```

### 配置设置

```json
// .vscode/settings.json
{
  // kubectl 路径
  "vs-kubernetes": {
    "vs-kubernetes.kubectl-path": "/usr/local/bin/kubectl",
    "vs-kubernetes.helm-path": "/usr/local/bin/helm"
  },
  
  // 自动刷新集群视图
  "vs-kubernetes.autoRefresh": true,
  "vs-kubernetes.autoRefreshInterval": 10,
  
  // 日志配置
  "vs-kubernetes.log-viewer.follow": true,
  "vs-kubernetes.log-viewer.timestamps": true,
  
  // YAML 编辑
  "vs-kubernetes.yaml-schema.enabled": true,
  
  // 资源过滤
  "vs-kubernetes.resource-filter": [
    "!Event",
    "!Endpoints"
  ]
}
```

---

## 集群浏览器

### 多集群管理

```
┌─────────────────────────────────────────────┐
│  KUBERNETES                              ▼  │
├─────────────────────────────────────────────┤
│                                             │
│  ▼ Clusters                                 │
│    ▼ dev-cluster (current)                  │
│      ▼ Namespaces                           │
│        ▶ default                            │
│        ▼ production                         │
│          ▶ Workloads                        │
│            ▶ Deployments                    │
│              ▶ nginx-deployment             │
│              ▶ api-server                   │
│            ▶ StatefulSets                   │
│            ▶ DaemonSets                     │
│          ▶ Network                          │
│            ▶ Services                       │
│            ▶ Ingresses                      │
│          ▶ Config                           │
│            ▶ ConfigMaps                     │
│            ▶ Secrets                        │
│          ▶ Storage                          │
│            ▶ PersistentVolumeClaims         │
│    ▶ staging-cluster                        │
│    ▶ prod-cluster                           │
│                                             │
│  ▼ Helm Releases                            │
│    ▶ nginx-ingress                          │
│    ▶ prometheus                             │
│                                             │
└─────────────────────────────────────────────┘
```

### 资源操作

```
右键菜单选项:
├── Describe       - 查看资源详情
├── Get YAML       - 获取 YAML 定义
├── Edit           - 编辑资源
├── Delete         - 删除资源
├── Logs           - 查看日志 (Pod)
├── Terminal       - 连接终端 (Pod)
├── Port Forward   - 端口转发 (Pod/Service)
├── Scale          - 扩缩容 (Deployment)
├── Rollout        - 滚动更新操作
│   ├── Restart
│   ├── Pause
│   └── Resume
└── Copy Name      - 复制资源名称
```

---

## YAML 编辑支持

### 智能补全

```yaml
# 输入 "apiV" 后按 Ctrl+Space
apiVersion: apps/v1  # 自动补全
kind: Deployment     # 自动补全
metadata:
  name: my-app
  labels:
    app: my-app      # 标签自动建议
spec:
  replicas: 3        # 数字验证
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:    # 容器定义模板补全
        - name: app
          image: nginx:1.25
          ports:
            - containerPort: 80
          resources:
            requests:
              cpu: "100m"      # 资源单位补全
              memory: "128Mi"
            limits:
              cpu: "500m"
              memory: "512Mi"
```

### 悬浮提示

```yaml
# 将鼠标悬停在字段上查看文档
apiVersion: apps/v1
# ┌─────────────────────────────────────────────────────────┐
# │ apiVersion (string)                                      │
# │ APIVersion defines the versioned schema of this          │
# │ representation of an object.                             │
# │                                                          │
# │ Examples: "apps/v1", "v1", "batch/v1"                   │
# └─────────────────────────────────────────────────────────┘
```

### 验证错误提示

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: "3"  # ⚠️ 错误: replicas 必须是数字类型
  selector:
    # ⚠️ 错误: selector 是必需字段
  template:
    spec:
      containers: []  # ⚠️ 警告: containers 不能为空
```

---

## 常用命令

### 命令面板操作

```
Ctrl+Shift+P (Cmd+Shift+P on macOS)

# 集群操作
Kubernetes: Use Context           - 切换集群上下文
Kubernetes: Show Cluster Info     - 显示集群信息
Kubernetes: Add Existing Cluster  - 添加集群

# 资源操作
Kubernetes: Apply                 - 应用当前文件
Kubernetes: Delete                - 删除当前文件定义的资源
Kubernetes: Describe              - 描述资源
Kubernetes: Get                   - 获取资源

# Pod 操作
Kubernetes: Logs                  - 查看日志
Kubernetes: Terminal              - 打开终端
Kubernetes: Port Forward          - 端口转发
Kubernetes: Debug (Attach)        - 附加调试器

# Helm 操作
Helm: Lint                        - 检查 Chart
Helm: Template                    - 预览模板
Helm: Install                     - 安装 Chart
Helm: Upgrade                     - 升级 Release
```

### 快捷键配置

```json
// keybindings.json
[
  {
    "key": "ctrl+k ctrl+a",
    "command": "extension.vsKubernetesApply",
    "when": "editorTextFocus"
  },
  {
    "key": "ctrl+k ctrl+l",
    "command": "extension.vsKubernetesLogs",
    "when": "editorTextFocus"
  },
  {
    "key": "ctrl+k ctrl+t",
    "command": "extension.vsKubernetesTerminal",
    "when": "editorTextFocus"
  },
  {
    "key": "ctrl+k ctrl+d",
    "command": "extension.vsKubernetesDescribe",
    "when": "editorTextFocus"
  }
]
```

---

## Helm 支持

### Chart 开发

```
my-chart/
├── Chart.yaml           # Chart 元数据
├── values.yaml          # 默认值
├── templates/           # 模板目录
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── ingress.yaml
│   ├── _helpers.tpl     # 辅助模板
│   └── NOTES.txt        # 安装说明
└── charts/              # 依赖 Charts
```

### 模板预览

```yaml
# templates/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "my-chart.fullname" . }}
  labels:
    {{- include "my-chart.labels" . | nindent 4 }}
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      {{- include "my-chart.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      labels:
        {{- include "my-chart.selectorLabels" . | nindent 8 }}
    spec:
      containers:
        - name: {{ .Chart.Name }}
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          ports:
            - containerPort: {{ .Values.service.port }}
```

```bash
# 使用命令预览渲染结果
# Command Palette > Helm: Preview Template

# 输出:
# ---
# apiVersion: apps/v1
# kind: Deployment
# metadata:
#   name: my-release-my-chart
#   labels:
#     app.kubernetes.io/name: my-chart
#     ...
```

---

## 调试功能

### 配置调试

```json
// .vscode/launch.json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Kubernetes: Attach to Pod",
      "type": "cloudcode.kubernetes",
      "request": "attach",
      "podSelector": {
        "app": "my-app"
      },
      "containerName": "app",
      "localRoot": "${workspaceFolder}",
      "remoteRoot": "/app"
    },
    {
      "name": "Kubernetes: Run and Debug",
      "type": "cloudcode.kubernetes",
      "request": "launch",
      "skaffoldConfig": "${workspaceFolder}/skaffold.yaml",
      "watch": true,
      "cleanUp": true,
      "portForward": true
    }
  ]
}
```

### 端口转发调试

```bash
# 通过扩展进行端口转发
# 1. 右键点击 Pod > Port Forward
# 2. 选择容器端口和本地端口
# 3. 访问 localhost:本地端口

# 或使用命令面板
# Kubernetes: Port Forward
# 选择 Pod: my-app-xxx
# 容器端口: 8080
# 本地端口: 8080
```

---

## 扩展集成

### 推荐搭配扩展

```json
// .vscode/extensions.json
{
  "recommendations": [
    // Kubernetes 核心
    "ms-kubernetes-tools.vscode-kubernetes-tools",
    
    // YAML 增强
    "redhat.vscode-yaml",
    
    // Docker 支持
    "ms-azuretools.vscode-docker",
    
    // Cloud Code (GCP)
    "googlecloudtools.cloudcode",
    
    // Lens IDE
    "mikestead.dotenv",
    
    // GitOps
    "weaveworks.vscode-gitops-tools"
  ]
}
```

### YAML Schema 配置

```json
// settings.json
{
  "yaml.schemas": {
    "kubernetes": [
      "*.k8s.yaml",
      "**/kubernetes/**/*.yaml",
      "**/k8s/**/*.yaml"
    ],
    "https://json.schemastore.org/helm-chart.json": [
      "Chart.yaml"
    ],
    "https://json.schemastore.org/kustomization.json": [
      "kustomization.yaml"
    ]
  },
  
  // Kubernetes 资源文件关联
  "files.associations": {
    "**/templates/**/*.yaml": "helm",
    "**/templates/**/*.tpl": "helm"
  }
}
```

---

## 代码片段

### 内置代码片段

```yaml
# 输入 "kdep" + Tab
# Kubernetes Deployment 代码片段
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${1:name}
spec:
  replicas: ${2:1}
  selector:
    matchLabels:
      app: ${1:name}
  template:
    metadata:
      labels:
        app: ${1:name}
    spec:
      containers:
        - name: ${1:name}
          image: ${3:image}
          ports:
            - containerPort: ${4:80}
```

### 自定义代码片段

```json
// .vscode/k8s.code-snippets
{
  "Kubernetes Service": {
    "prefix": "ksvc",
    "body": [
      "apiVersion: v1",
      "kind: Service",
      "metadata:",
      "  name: ${1:name}",
      "spec:",
      "  type: ${2|ClusterIP,NodePort,LoadBalancer|}",
      "  ports:",
      "    - port: ${3:80}",
      "      targetPort: ${4:8080}",
      "  selector:",
      "    app: ${1:name}"
    ],
    "description": "Kubernetes Service template"
  },
  
  "Kubernetes ConfigMap": {
    "prefix": "kcm",
    "body": [
      "apiVersion: v1",
      "kind: ConfigMap",
      "metadata:",
      "  name: ${1:name}",
      "data:",
      "  ${2:key}: ${3:value}"
    ],
    "description": "Kubernetes ConfigMap template"
  }
}
```

---

## 最佳实践

### 项目结构

```
my-k8s-project/
├── .vscode/
│   ├── settings.json      # 项目设置
│   ├── launch.json        # 调试配置
│   ├── extensions.json    # 推荐扩展
│   └── k8s.code-snippets  # 代码片段
├── kubernetes/
│   ├── base/              # 基础资源
│   │   ├── deployment.yaml
│   │   ├── service.yaml
│   │   └── kustomization.yaml
│   └── overlays/          # 环境覆盖
│       ├── dev/
│       ├── staging/
│       └── prod/
├── helm/
│   └── my-chart/
│       ├── Chart.yaml
│       ├── values.yaml
│       └── templates/
└── skaffold.yaml          # 开发工作流
```

### 工作流建议

```
1. 开发阶段
   └── 使用 YAML 智能补全编写资源定义
   └── 利用验证功能检查错误
   └── 使用 Helm Template 预览渲染结果

2. 测试阶段
   └── Kubernetes: Apply 部署到开发集群
   └── 查看 Pod 日志和状态
   └── 端口转发进行本地测试

3. 调试阶段
   └── 连接容器终端
   └── 使用调试器附加到进程
   └── 查看实时日志

4. 部署阶段
   └── 切换到目标集群上下文
   └── 应用生产配置
   └── 监控部署状态
```

---

## 参考资源

- [GitHub 仓库](https://github.com/vscode-kubernetes-tools/vscode-kubernetes-tools)
- [VS Code Marketplace](https://marketplace.visualstudio.com/items?itemName=ms-kubernetes-tools.vscode-kubernetes-tools)
- [官方文档](https://github.com/vscode-kubernetes-tools/vscode-kubernetes-tools/wiki)
- [Kubernetes 官方文档](https://kubernetes.io/docs/)
- [Helm 官方文档](https://helm.sh/docs/)
- [CNCF Sandbox](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
