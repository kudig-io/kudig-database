---
title: Platform Engineer Learning Path — From Developer to Platform Architect
description: K8s 平台工程师学习路径 — 从开发者到平台架构师的系统化成长路线、技能矩阵、认证规划
summary: 面向平台工程师的完整学习路径，涵盖容器基础、K8s 运维、平台工程、架构设计四大阶段
category: reference
tags:
- learning-path
- platform-engineer
- career
- certification
- skill-matrix
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: beginner
domain: documentation
---
# 平台工程师学习路径

> 从开发者到平台架构师的系统化成长路线图。

## 学习路径总览

```
┌─────────────────────────────────────────────────────────────────┐
│  阶段 1          阶段 2          阶段 3          阶段 4          │
│  容器基础        K8s 运维        平台工程        架构设计        │
│  (1-2 月)       (3-6 月)       (6-12 月)      (12-24 月)      │
│                                                                 │
│  ┌─────┐       ┌─────┐       ┌─────┐       ┌─────┐           │
│  │Linux│──────▶│集群 │──────▶│IDP  │──────▶│多集群│           │
│  │容器 │       │运维 │       │构建 │       │架构 │           │
│  │网络 │       │网络 │       │GitOps│       │治理 │           │
│  │镜像 │       │存储 │       │可观测│       │成本 │           │
│  └─────┘       └─────┘       └─────┘       └─────┘           │
│                                                                 │
│  CKA 备考       CKS 备考      平台实践       架构输出          │
└─────────────────────────────────────────────────────────────────┘
```

## 阶段 1: 容器基础（1-2 月）

### 技能矩阵

| 技能 | 级别要求 | 学习资源 | 验证方式 |
|------|----------|----------|----------|
| Linux 基础 | 熟练 | 本知识库/系统基础 | 日常操作无阻碍 |
| 网络基础 | 理解 | TCP/IP/HTTP/DNS | 能抓包分析 |
| Docker/容器 | 熟练 | 官方文档 | 构建多阶段镜像 |
| 镜像优化 | 掌握 | distroless/alpine | 镜像 < 50MB |
| 容器网络 | 理解 | bridge/host/overlay | 能解释通信原理 |
| Shell 脚本 | 熟练 | bash/zsh | 自动化日常任务 |

### 实践项目

```bash
# 项目 1: 从零构建容器化应用
# 目标: 理解容器隔离原理

# 1. 编写多阶段 Dockerfile
cat <<'EOF' > Dockerfile
FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -o /server .

FROM gcr.io/distroless/static:nonroot
COPY --from=builder /server /server
USER nonroot:nonroot
ENTRYPOINT ["/server"]
EOF

# 2. 构建并优化
docker build -t myapp:v1 .
docker images myapp  # 目标 < 20MB

# 3. 运行并观察隔离
docker run -d --name app1 -p 8080:8080 myapp:v1
docker exec app1 cat /proc/1/cgroup  # 观察 cgroup
docker exec app1 ip addr             # 观察网络隔离
```

### 阶段 1 检查点

- [ ] 能解释 Namespace 和 Cgroup 的作用
- [ ] 能编写生产级 Dockerfile（多阶段、非 root、最小镜像）
- [ ] 能使用 docker-compose 编排多服务
- [ ] 理解容器网络模型（bridge、overlay）
- [ ] 能使用 tcpdump 排查基本网络问题

## 阶段 2: Kubernetes 运维（3-6 月）

### 技能矩阵

| 技能 | 级别要求 | 学习资源 | 验证方式 |
|------|----------|----------|----------|
| K8s 核心概念 | 精通 | 本知识库/概念 | CKA 通过 |
| 工作负载管理 | 精通 | Deployment/StatefulSet | 生产部署 |
| 网络（Service/Ingress） | 精通 | 本知识库/网络 | 配置完整链路 |
| 存储（PV/PVC/SC） | 掌握 | 本知识库/存储 | 动态供给配置 |
| RBAC/安全 | 掌握 | 本知识库/安全 | 最小权限配置 |
| 故障排查 | 精通 | 本知识库/故障诊断 | 独立排障 |
| Helm/Kustomize | 精通 | 本知识库/清单模式 | Chart 开发 |
| 监控告警 | 掌握 | Prometheus/Grafana | 搭建监控栈 |

### 实践项目

```yaml
# 项目 2: 搭建完整微服务环境
# 目标: 掌握 K8s 核心运维能力

# 1. 部署 3 个微服务（API + Worker + DB）
# 2. 配置 Ingress + TLS
# 3. 配置 HPA 自动扩缩
# 4. 配置 NetworkPolicy 隔离
# 5. 配置 Prometheus 监控
# 6. 模拟故障并排查

# HPA 示例
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: api-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  minReplicas: 2
  maxReplicas: 10
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
        target:
          type: AverageValue
          averageValue: "1000"
```

### 阶段 2 检查点

- [ ] 通过 CKA 认证
- [ ] 能独立管理生产 K8s 集群
- [ ] 能配置完整的网络链路（Ingress → Service → Pod）
- [ ] 能使用 Helm 管理复杂应用
- [ ] 能独立排查 Pod/网络/存储故障
- [ ] 能配置 RBAC 最小权限
- [ ] 理解 etcd 备份恢复

## 阶段 3: 平台工程（6-12 月）

### 技能矩阵

| 技能 | 级别要求 | 学习资源 | 验证方式 |
|------|----------|----------|----------|
| GitOps | 精通 | ArgoCD/Flux | 多环境流水线 |
| CI/CD | 精通 | GitHub Actions/Tekton | 完整 Pipeline |
| 可观测性体系 | 精通 | OTel/Prometheus/Loki | 全链路追踪 |
| 策略即代码 | 掌握 | Kyverno/OPA | 策略库建设 |
| IDP 构建 | 掌握 | Backstage/Crossplane | 开发者门户 |
| 安全加固 | 精通 | CKS 内容 | 通过 CKS |
| IaC | 精通 | Terraform/Pulumi | 基础设施代码化 |
| 服务网格 | 掌握 | Istio/Linkerd | mTLS 全链路 |

### 实践项目

```yaml
# 项目 3: 构建内部开发者平台
# 目标: 让开发者自助部署

# 1. GitOps 仓库结构
# 2. ArgoCD ApplicationSet 多环境
# 3. 自助服务目录（Backstage）
# 4. 策略即代码（Kyverno）
# 5. 可观测性即服务（自动注入）
# 6. 密钥管理（External Secrets + Vault）

# ArgoCD ApplicationSet
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: team-apps
  namespace: argocd
spec:
  generators:
    - git:
        repoURL: https://github.com/myorg/platform.git
        revision: main
        directories:
          - path: apps/*
  template:
    metadata:
      name: '{{path.basename}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/myorg/platform.git
        targetRevision: main
        path: '{{path}}'
      destination:
        server: https://kubernetes.default.svc
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

### 阶段 3 检查点

- [ ] 通过 CKS 认证
- [ ] 搭建完整 GitOps 流水线
- [ ] 构建可观测性体系（指标+日志+追踪）
- [ ] 实施策略即代码（≥ 10 条策略）
- [ ] 搭建开发者自助门户
- [ ] 配置服务网格 mTLS

## 阶段 4: 架构设计（12-24 月）

### 技能矩阵

| 技能 | 级别要求 | 学习资源 | 验证方式 |
|------|----------|----------|----------|
| 多集群架构 | 精通 | 本知识库/集群基础 | 多集群方案设计 |
| 成本治理 | 精通 | Kubecost/FinOps | 成本降低 30% |
| 灾备设计 | 精通 | 本知识库/可靠性 | DR 演练通过 |
| 平台治理 | 精通 | 本知识库/生产运维 | 治理体系建设 |
| AI 基础设施 | 掌握 | 本知识库/AI基础设施 | GPU 集群管理 |
| 技术领导力 | 掌握 | 架构决策/文档 | 技术方案评审 |

### 阶段 4 检查点

- [ ] 设计并实施多集群架构
- [ ] 建立成本治理体系
- [ ] 完成 DR 演练（RTO < 5min）
- [ ] 输出平台架构文档
- [ ] 指导初中级工程师成长

## 认证规划

| 认证 | 阶段 | 难度 | 价值 |
|------|------|------|------|
| CKA | 阶段 2 | ★★★ | 运维能力证明 |
| CKAD | 阶段 2 | ★★☆ | 应用开发视角 |
| CKS | 阶段 3 | ★★★★ | 安全专业能力 |
| Terraform Associate | 阶段 3 | ★★☆ | IaC 能力 |
| AWS/GCP SA Pro | 阶段 4 | ★★★★ | 云架构能力 |

## 每日学习建议

| 时间 | 活动 | 说明 |
|------|------|------|
| 30 min | 阅读文档/知识库 | 本知识库每日一篇 |
| 60 min | 动手实践 | Lab/生产操作 |
| 15 min | 社区参与 | CNCF Slack/GitHub |
| 周末 2h | 项目实践 | 阶段性项目 |
| 月度 | 复盘总结 | 输出笔记/博客 |

## Related

- [[29-文档/learning-paths/index.md|学习路径]]
- [[29-文档/learning-paths/kubernetes-sre-engineer-learning-path.md|SRE 学习路径]]
- [[26-技能/index.md|技能体系]]
- [[22-概念/index.md|核心概念]]
