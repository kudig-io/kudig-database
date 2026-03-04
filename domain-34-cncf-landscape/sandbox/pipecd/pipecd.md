# PipeCD

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://pipecd.dev/ |
| **GitHub** | https://github.com/pipe-cd/pipecd |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

PipeCD 是一个统一的持续交付平台，为 Kubernetes、Terraform、CloudRun、Lambda、ECS 等多种应用平台提供一致的 GitOps 部署体验。它采用控制平面（Control Plane）+ 代理（Piped）架构，支持渐进式交付策略（金丝雀、蓝绿、滚动）和自动回滚。

### 核心特性

- **多平台支持**: Kubernetes, Terraform, CloudRun, Lambda, ECS
- **渐进式交付**: 金丝雀、蓝绿、滚动部署策略
- **GitOps 原生**: 以 Git 仓库作为唯一真相来源
- **自动分析和回滚**: 基于 Prometheus/Datadog 指标的自动回滚
- **多租户**: 项目和环境级别的隔离
- **审计日志**: 完整的部署审计追踪
- **Piped 代理**: 轻量级代理部署在目标环境，无需暴露集群 API

---

## 架构设计

```
┌───────────────────────────────────────────────┐
│             PipeCD Control Plane               │
│                                                │
│  ┌────────┐  ┌─────────┐  ┌───────────────┐  │
│  │  Web   │  │  API    │  │  Ops Server   │  │
│  │  UI    │  │ Server  │  │               │  │
│  └────────┘  └─────────┘  └───────────────┘  │
│                    │                           │
│  ┌─────────────────┴─────────────────────┐    │
│  │         Data Store (MySQL/Firestore)   │    │
│  │         File Store (GCS/S3/MinIO)      │    │
│  └────────────────────────────────────────┘    │
└───────────────────┬───────────────────────────┘
                    │ (gRPC)
        ┌───────────┼───────────┐
        ▼           ▼           ▼
  ┌──────────┐ ┌──────────┐ ┌──────────┐
  │  Piped   │ │  Piped   │ │  Piped   │
  │ (K8s)   │ │ (TF)    │ │(CloudRun)│
  │          │ │          │ │          │
  │ ┌──────┐│ │ ┌──────┐│ │ ┌──────┐│
  │ │Git   ││ │ │Git   ││ │ │Git   ││
  │ │Sync  ││ │ │Sync  ││ │ │Sync  ││
  │ └──────┘│ │ └──────┘│ │ └──────┘│
  └──────────┘ └──────────┘ └──────────┘
```

---

## 快速开始

### 安装 Control Plane

```bash
# 使用 Helm 安装
helm repo add pipecd https://charts.pipecd.dev
helm install pipecd pipecd/pipecd \
  --namespace pipecd \
  --create-namespace \
  --values values.yaml
```

```yaml
# values.yaml
server:
  config:
    projectConfigs:
      - id: my-project
        staticAdmin:
          username: admin
          passwordHash: "$2a$10$..."  # bcrypt hash
    datastore:
      type: MYSQL
      config:
        url: "root:password@tcp(mysql:3306)/pipecd"
    filestore:
      type: MINIO
      config:
        endpoint: http://minio:9000
        bucket: pipecd
```

### 安装 Piped 代理

```yaml
# piped-config.yaml
apiVersion: pipecd.dev/v1beta1
kind: Piped
spec:
  projectID: my-project
  pipedID: piped-01
  pipedKeyFile: /etc/piped/piped-key
  apiAddress: pipecd-server:443
  repositories:
    - repoId: my-app
      remote: git@github.com:my-org/my-app.git
      branch: main
  platformProviders:
    - name: kubernetes-default
      type: KUBERNETES
    - name: terraform-aws
      type: TERRAFORM
      config:
        vars:
          - "region=us-east-1"
```

### 定义应用部署

```yaml
# .pipe/app.pipecd.yaml (Git 仓库中)
apiVersion: pipecd.dev/v1beta1
kind: KubernetesApp
spec:
  name: my-web-app
  pipeline:
    stages:
      - name: K8S_CANARY_ROLLOUT
        with:
          replicas: 20%
      - name: ANALYSIS
        with:
          duration: 10m
          metrics:
            - strategy: THRESHOLD
              provider: prometheus
              query: |
                rate(http_requests_total{status=~"5.."}[5m]) 
                / rate(http_requests_total[5m]) * 100
              expected:
                max: 1  # 错误率 < 1%
      - name: K8S_PRIMARY_ROLLOUT
      - name: K8S_CANARY_CLEAN
```

---

## 部署策略

### 金丝雀部署

```yaml
spec:
  pipeline:
    stages:
      - name: K8S_CANARY_ROLLOUT
        with:
          replicas: 10%
      - name: WAIT_APPROVAL
        with:
          timeout: 1h
      - name: K8S_CANARY_ROLLOUT
        with:
          replicas: 50%
      - name: ANALYSIS
        with:
          duration: 15m
      - name: K8S_PRIMARY_ROLLOUT
      - name: K8S_CANARY_CLEAN
```

### 蓝绿部署

```yaml
spec:
  pipeline:
    stages:
      - name: K8S_STAGE_ROLLOUT
      - name: WAIT_APPROVAL
      - name: K8S_TRAFFIC_ROUTING
        with:
          all: stage  # 切换流量到新版本
      - name: K8S_PRIMARY_ROLLOUT
```

### Terraform 部署

```yaml
apiVersion: pipecd.dev/v1beta1
kind: TerraformApp
spec:
  name: aws-infrastructure
  pipeline:
    stages:
      - name: TERRAFORM_PLAN
      - name: WAIT_APPROVAL
      - name: TERRAFORM_APPLY
```

---

## 最佳实践

1. **渐进式交付**: 所有生产部署使用金丝雀或蓝绿策略，避免一次性全量发布
2. **自动分析**: 配置 Prometheus 指标分析，在金丝雀阶段自动检测异常
3. **审批门控**: 关键阶段设置 WAIT_APPROVAL，确保人工确认
4. **Piped 隔离**: 每个环境/集群部署独立的 Piped，缩小爆炸半径
5. **Secret 管理**: 使用 Sealed Secrets 或 SOPS 加密 Git 中的敏感配置
6. **多集群**: 通过 Piped 代理实现多集群部署，无需直连集群 API

---

## 参考资源

- [PipeCD 官方文档](https://pipecd.dev/docs/)
- [PipeCD GitHub](https://github.com/pipe-cd/pipecd)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
