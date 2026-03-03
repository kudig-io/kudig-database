# Kubernetes

> **成熟度**: Graduated | **加入时间**: 2016-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubernetes.io |
| **GitHub** | https://github.com/kubernetes/kubernetes |
| **文档** | https://kubernetes.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Orchestration & Management |

---

## 项目概述

### 简介
Kubernetes（K8s）是一个开源的容器编排平台，用于自动化容器化应用的部署、扩展和管理。

### 核心定位
Kubernetes 解决了大规模容器化应用的编排、调度、服务发现、负载均衡、存储编排、自动恢复等核心问题，是云原生生态系统的基石。

### 发展历程
- **2014-06**: Google 开源 Kubernetes 项目
- **2015-07**: Kubernetes v1.0 发布，CNCF 成立
- **2016-03**: 成为 CNCF 首个托管项目
- **2018-03**: 成为 CNCF 首个毕业项目
- **2024-04**: Kubernetes v1.30 发布

---

## 核心功能

### 主要特性
- **容器编排**: 自动化容器的部署、扩展和运维
- **服务发现**: 内置 DNS 和负载均衡
- **自动恢复**: 自动重启失败容器、替换节点
- **滚动更新**: 零停机部署和回滚
- **配置管理**: ConfigMap 和 Secret 管理
- **存储编排**: 自动挂载存储系统
- **批处理**: Job 和 CronJob 支持

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                      Control Plane                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │  API Server  │ │   Scheduler  │ │ Controller Manager   ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                         etcd                            ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                       Worker Nodes                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │    kubelet   │ │  kube-proxy  │ │  Container Runtime   ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                        Pods                             ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
Kubernetes 采用主从架构，由控制平面（Control Plane）和工作节点（Worker Nodes）组成。控制平面负责集群的全局决策，工作节点运行实际的应用负载。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| API Server | 集群入口 | 所有操作的统一入口，RESTful API |
| etcd | 状态存储 | 分布式键值存储，保存集群状态 |
| Scheduler | 调度器 | 为 Pod 选择合适的节点 |
| Controller Manager | 控制器 | 运行各种控制器，维护期望状态 |
| kubelet | 节点代理 | 管理节点上的容器生命周期 |
| kube-proxy | 网络代理 | 维护节点网络规则，实现 Service |

### 工作原理
1. 用户通过 kubectl 或 API 提交期望状态
2. API Server 将状态存储到 etcd
3. Controller 监听状态变化，执行调谐逻辑
4. Scheduler 为新 Pod 分配节点
5. kubelet 在节点上创建和管理容器
6. kube-proxy 配置网络规则实现服务访问

---

## 使用场景

### 典型应用
- **微服务架构**: 部署和管理微服务应用
- **CI/CD 平台**: 构建云原生持续交付流水线
- **大数据处理**: 运行 Spark、Flink 等数据处理任务
- **机器学习**: 部署 ML 训练和推理工作负载
- **边缘计算**: 管理边缘节点上的应用

### 适用条件
- 需要自动化容器编排和管理
- 需要高可用和自动恢复能力
- 需要灵活的扩展和升级策略
- 有专业的运维团队支持

### 不适用场景
- 简单的单机应用部署
- 资源极度受限的环境
- 不需要容器化的传统应用

---

## 快速开始

### 安装部署
```bash
# 使用 kind 创建本地集群
kind create cluster --name my-cluster

# 使用 minikube 创建本地集群
minikube start

# 使用 kubeadm 初始化生产集群
kubeadm init --pod-network-cidr=10.244.0.0/16
```

### 基础配置
```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-deployment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
        ports:
        - containerPort: 80
```

### 验证测试
```bash
# 查看集群状态
kubectl cluster-info
kubectl get nodes

# 部署应用
kubectl apply -f deployment.yaml

# 查看 Pod 状态
kubectl get pods -w
```

---

## 最佳实践

### 生产环境建议
- 使用高可用控制平面（3+ 节点）
- 配置资源限制（requests/limits）
- 启用 RBAC 和 Pod Security Standards
- 定期备份 etcd 数据
- 使用节点亲和性和反亲和性

### 性能优化
- 合理配置 API Server 限流参数
- 使用 Pod Disruption Budget
- 优化容器镜像大小
- 配置合适的探针参数

### 安全加固
- 启用审计日志
- 使用 NetworkPolicy 隔离网络
- 定期更新 Kubernetes 版本
- 加密 etcd 数据和 Secret

---

## 生态集成

### 相关 CNCF 项目
- **Helm**: Kubernetes 包管理器
- **Prometheus**: 监控和告警
- **Envoy/Istio**: 服务网格
- **Argo**: GitOps 和工作流
- **containerd**: 容器运行时

### 常见集成方案
- Prometheus + Grafana 监控栈
- Istio 服务网格
- ArgoCD GitOps 部署
- Cert-manager 证书管理

---

## 社区与支持

### 社区资源
- Slack: https://slack.k8s.io
- 论坛: https://discuss.kubernetes.io
- Stack Overflow: kubernetes 标签

### 贡献指南
访问 https://www.kubernetes.dev/docs/guide/ 了解如何参与贡献

---

## 参考资源

- [官方文档](https://kubernetes.io/docs)
- [GitHub Repo](https://github.com/kubernetes/kubernetes)
- [CNCF 项目页面](https://www.cncf.io/projects/kubernetes/)
- [Kubernetes Blog](https://kubernetes.io/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT
