---
title: 知识字典总索引
description: Kubernetes 全域知识字典总索引，覆盖 13 个子领域的完整术语体系，包括基础架构、网络、存储、调度、安全、可观测性、平台工程、生产运维、工具生态、多云、工作负载、配置管理、专项工作负载
summary: K8s 知识字典总索引，覆盖云原生全域 13 个子领域，包含核心术语、技术组件、生产实践、故障排查
category: dictionary
tags:
- dictionary
- index
- kubernetes
- cloud-native
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: beginner
audience:
- 开发工程师
- 平台工程师
- SRE
- 架构师
---

# Kubernetes 知识字典总索引

> 本词典是 Kubernetes 云原生全域知识的权威术语参考，覆盖 13 个子领域、500+ 核心术语、200+ 技术组件，是工程师从入门到专家的完整知识地图。

## 领域概述

Kubernetes 知识字典按功能域划分为 13 个子领域，每个子领域包含：
- 核心术语定义（概念、原理、关键特性）
- 技术组件索引（工具、框架、平台）
- 深度技术解析（架构、源码、配置）
- 生产案例（真实故障、根因、解决方案）
- 最佳实践（设计原则、反模式、检查清单）
- 故障排查（现象、原因、方向）
- 命令速查（常用操作、调试命令）

## 子领域索引

### 基础架构（Fundamentals）

> K8s 核心架构与内部机制

- [[17-系统基础/06-知识字典/fundamentals/index.md|基础知识词典]]
- 覆盖：控制平面、节点组件、容器运行时、核心对象、API 机制、集群架构
- 核心术语：kube-apiserver、etcd、kubelet、containerd、CRI、Controller Pattern、Namespace
- 难度：入门 → 进阶

### 网络（Networking）

> 集群网络、服务发现、流量管理

- [[17-系统基础/06-知识字典/networking/index.md|网络知识词典]]
- 覆盖：CNI、Service、Ingress、Gateway API、Service Mesh、NetworkPolicy、DNS
- 核心术语：Cilium、Istio、Envoy、CoreDNS、Gateway API、eBPF、NetworkPolicy
- 难度：中级 → 高级

### 存储（Storage）

> 容器存储、持久化、分布式存储

- [[17-系统基础/06-知识字典/storage/index.md|存储知识词典]]
- 覆盖：CSI、PV/PVC、StorageClass、分布式存储、快照、备份
- 核心术语：CSI、PersistentVolume、StorageClass、Ceph、Longhorn、Rook
- 难度：中级 → 高级

### 调度（Scheduling）

> Pod 调度、自动扩缩、资源管理

- [[17-系统基础/06-知识字典/scheduling/index.md|调度知识词典]]
- 覆盖：Scheduler、HPA/VPA/KEDA、Volcano、GPU 调度、拓扑感知
- 核心术语：Scheduler、HPA、VPA、KEDA、Volcano、Topology Spread
- 难度：中级 → 高级

### 工作负载（Workloads）

> 工作负载控制器与应用部署

- [[17-系统基础/06-知识字典/workloads/index.md|工作负载知识词典]]
- 覆盖：Pod、Deployment、StatefulSet、DaemonSet、Job/CronJob、OpenKruise
- 核心术语：Deployment、StatefulSet、ReplicaSet、Job、CronJob、Rolling Update
- 难度：入门 → 中级

### 配置管理（Configuration）

> 应用配置、资源管理、探针

- [[17-系统基础/06-知识字典/configuration/index.md|配置管理知识词典]]
- 覆盖：ConfigMap、Secret、Probe、资源请求/限制、Server-Side Apply
- 核心术语：ConfigMap、Secret、Liveness/Readiness/Startup Probe、Resources、SSA
- 难度：入门 → 中级

### 可观测性（Observability）

> 监控、日志、链路追踪、告警

- [[17-系统基础/06-知识字典/observability/index.md|可观测性知识词典]]
- 覆盖：Prometheus、OpenTelemetry、Loki、Grafana、SLO、告警
- 核心术语：Prometheus、OTel、Loki、Grafana、SLO/SLI、Alertmanager
- 难度：中级 → 高级

### 安全（Security）

> 身份认证、访问控制、供应链安全、运行时安全

- [[17-系统基础/06-知识字典/security/index.md|安全知识词典]]
- 覆盖：RBAC、Pod Security、供应链、策略引擎、密钥管理、运行时安全
- 核心术语：RBAC、PSA、Kyverno、OPA、Falco、Vault、SPIFFE、Trivy
- 难度：中级 → 专家

### 平台工程（Platform Engineering）

> Operator、CRD、IDP、多集群管理

- [[17-系统基础/06-知识字典/platform-engineering/index.md|平台工程知识词典]]
- 覆盖：Operator、CRD、GitOps、IDP、多集群、API 扩展
- 核心术语：Operator、CRD、Crossplane、Backstage、Karmada、Dapr
- 难度：高级 → 专家

### 生产运维（Operations）

> GitOps、SRE、混沌工程、备份恢复、FinOps

- [[17-系统基础/06-知识字典/operations/index.md|生产运维知识词典]]
- 覆盖：GitOps、SRE、混沌工程、备份恢复、节点运维、成本优化
- 核心术语：ArgoCD、Flux、Velero、Chaos Mesh、SLO、FinOps
- 难度：中级 → 高级

### 工具生态（Tooling）

> CLI、包管理、镜像构建、本地开发、IaC

- [[17-系统基础/06-知识字典/tooling/index.md|工具生态知识词典]]
- 覆盖：kubectl、Helm、Kustomize、Harbor、Skaffold、Podman、kubeadm
- 核心术语：kubectl、Helm、Kustomize、Kind、Minikube、Buildpacks、Harbor
- 难度：入门 → 中级

### 多云与边缘（Multi-Cloud）

> 多集群、联邦、边缘计算、混合云

- [[17-系统基础/06-知识字典/multi-cloud/index.md|多云知识词典]]
- 覆盖：KubeFed、CAPI、Karmada、Crossplane、边缘计算、混合云
- 核心术语：Cluster API、Karmada、Crossplane、KubeEdge、OpenYurt
- 难度：高级 → 专家

### 专项工作负载（Specialized Workloads）

> GPU/AI、Serverless、VM、Wasm

- [[17-系统基础/06-知识字典/specialized-workloads/index.md|专项工作负载知识词典]]
- 覆盖：GPU 调度、KServe、Knative、KubeVirt、Wasm、边缘 AI
- 核心术语：GPU Operator、KServe、Knative、KubeVirt、WasmEdge、vLLM
- 难度：高级 → 专家

## 其他文档

- [[17-系统基础/06-知识字典/README.md|知识字典说明]]
- [[17-系统基础/06-知识字典/MOC.md|内容地图 (MOC)]]
- [[17-系统基础/06-知识字典/k8s-glossary.md|K8s 术语表]]
- [[17-系统基础/06-知识字典/GAP-ANALYSIS.md|缺口分析]]

## 知识字典使用指南

### 按角色查阅

| 角色 | 优先领域 | 典型场景 |
|------|----------|----------|
| 开发工程师 | Workloads → Configuration → Tooling | 部署应用、配置管理、本地开发 |
| 平台工程师 | Platform Engineering → Networking → Security | 构建 IDP、网络设计、安全基线 |
| SRE | Operations → Observability → Fundamentals | 可靠性、监控、故障排查 |
| 安全工程师 | Security → Networking → Operations | 安全策略、网络隔离、合规 |
| 架构师 | Multi-Cloud → Platform Engineering → Scheduling | 多云架构、平台设计、资源规划 |

### 按难度进阶

```
入门: Fundamentals → Workloads → Configuration → Tooling
中级: Networking → Storage → Scheduling → Observability → Operations
高级: Security → Platform Engineering → Multi-Cloud
专家: Specialized Workloads → 自定义扩展 → 内核机制
```

### 术语查找方式

1. **按领域浏览**：进入对应子领域 index.md，查看术语表
2. **按组件查找**：在技术组件索引中找到对应 wiki 链接
3. **按问题查找**：查看故障排查要点表
4. **按命令查找**：查看命令速查章节

## 统计概览

| 子领域 | 术语数 | 组件数 | 难度范围 |
|--------|--------|--------|----------|
| Fundamentals | 50+ | 58 | 入门→进阶 |
| Networking | 60+ | 65 | 中级→高级 |
| Storage | 40+ | 35 | 中级→高级 |
| Scheduling | 35+ | 30 | 中级→高级 |
| Workloads | 30+ | 25 | 入门→中级 |
| Configuration | 30+ | 20 | 入门→中级 |
| Observability | 40+ | 30 | 中级→高级 |
| Security | 70+ | 74 | 中级→专家 |
| Platform Engineering | 45+ | 35 | 高级→专家 |
| Operations | 40+ | 30 | 中级→高级 |
| Tooling | 40+ | 42 | 入门→中级 |
| Multi-Cloud | 35+ | 25 | 高级→专家 |
| Specialized Workloads | 30+ | 20 | 高级→专家 |
| **合计** | **500+** | **500+** | - |

## 参考链接

- https://kubernetes.io/docs/concepts/
- https://kubernetes.io/docs/reference/
- https://www.cncf.io/projects/
- https://landscape.cncf.io/
- https://kubeweekly.io/
- https://github.com/cncf/curriculum

## 跨域知识图谱

### 核心概念关联

```
                    ┌─────────────┐
                    │  Kubernetes  │
                    └──────┬──────┘
           ┌───────────┼───────────┐
           │               │               │
    ┌──────┴──────┐ ┌──────┴──────┐ ┌──────┴──────┐
    │ 控制平面   │ │  数据平面   │ │  扩展平面   │
    │ API/etcd   │ │ Pod/Node   │ │ CRD/Operator│
    │ Scheduler  │ │ CNI/CSI   │ │ Webhook    │
    │ Controller │ │ kubelet    │ │ Aggregation│
    └─────────────┘ └─────────────┘ └─────────────┘
```

### 跨域典型场景

| 场景 | 涉及领域 | 关键组件 |
|------|----------|----------|
| 部署微服务 | Workloads + Networking + Configuration | Deployment + Service + ConfigMap |
| 生产监控 | Observability + Operations + Platform | Prometheus + Grafana + Alertmanager |
| 安全加固 | Security + Networking + Fundamentals | RBAC + NetworkPolicy + PSA |
| 多集群管理 | Multi-Cloud + Platform + Operations | Karmada + GitOps + ArgoCD |
| AI 推理服务 | Specialized + Scheduling + Storage | GPU + KServe + PVC |
| 故障排查 | Fundamentals + Observability + Operations | 事件 + 日志 + Runbook |
| 平台建设 | Platform + Security + Tooling | Operator + Policy + Helm |
| 成本优化 | Operations + Scheduling + Observability | FinOps + VPA + 监控 |

### 技术栈全景

```
┌─────────────────────────────────────────────────────────┐
│                    应用层                                │
│  微服务 | Serverless | AI/ML | 边缘计算 | 批处理      │
├─────────────────────────────────────────────────────────┤
│                    平台层                                │
│  IDP | GitOps | Operator | 策略引擎 | 服务网格        │
├─────────────────────────────────────────────────────────┤
│                    基础设施层                            │
│  K8s | 容器运行时 | CNI | CSI | 负载均衡            │
├─────────────────────────────────────────────────────────┤
│                    操作系统层                            │
│  Linux | cgroup | namespace | eBPF | 网络栈          │
├─────────────────────────────────────────────────────────┤
│                    硬件层                                │
│  CPU | GPU | 内存 | NVMe | 网卡 | DPU              │
└─────────────────────────────────────────────────────────┘
```

## 常见工作流索引

### 应用部署工作流

```
开发 → 构建 → 扫描 → 签名 → 推送 → 部署 → 验证 → 监控
 │       │       │       │       │       │       │       │
 IDE   BuildKit Trivy  Cosign  Harbor  ArgoCD  Probe  Prometheus
```

### 故障排查工作流

```
告警 → 定位 → 分析 → 修复 → 验证 → 复盘
 │       │       │       │       │       │
Alert  kubectl  日志   回滚   监控   文档
PagerDuty describe traces  扩容  SLO   Postmortem
```

### 集群运维工作流

```
规划 → 部署 → 配置 → 监控 → 升级 → 扩容
 │       │       │       │       │       │
容量   kubeadm  GitOps  Prometheus 滚动   CA
设计   HA    Policy  Grafana  更新   节点
```

## 术语快速查找

### A-C

| 术语 | 领域 | 定义 |
|------|------|------|
| Admission Controller | Security/Fundamentals | API 请求准入控制 |
| ArgoCD | Operations | GitOps 持续交付工具 |
| CNI | Networking | 容器网络接口 |
| ConfigMap | Configuration | 配置数据对象 |
| CoreDNS | Networking | 集群 DNS 服务 |
| CRD | Platform | 自定义资源定义 |
| CSI | Storage | 容器存储接口 |
| CRI | Fundamentals | 容器运行时接口 |

### D-H

| 术语 | 领域 | 定义 |
|------|------|------|
| Deployment | Workloads | 无状态工作负载控制器 |
| eBPF | Networking/Fundamentals | 内核可编程技术 |
| etcd | Fundamentals | 分布式 KV 存储 |
| Falco | Security | 运行时威胁检测 |
| Gateway API | Networking | 下一代流量管理 API |
| GitOps | Operations | Git 为单一事实来源 |
| Helm | Tooling | K8s 包管理器 |
| HPA | Scheduling | 水平 Pod 自动扩缩 |

### I-P

| 术语 | 领域 | 定义 |
|------|------|------|
| Ingress | Networking | L7 流量入口规则 |
| Istio | Networking | 服务网格平台 |
| kubelet | Fundamentals | 节点代理 |
| Kyverno | Security | K8s 原生策略引擎 |
| NetworkPolicy | Networking/Security | Pod 网络访问控制 |
| Operator | Platform | CR + Controller 模式 |
| OPA | Security | 开放策略代理 |
| PVC | Storage | 持久卷声明 |

### Q-Z

| 术语 | 领域 | 定义 |
|------|------|------|
| RBAC | Security | 基于角色的访问控制 |
| Scheduler | Scheduling/Fundamentals | Pod 调度器 |
| Service | Networking | 服务抽象与负载均衡 |
| SLO | Operations/Observability | 服务水平目标 |
| StatefulSet | Workloads | 有状态工作负载 |
| Vault | Security | 密钥管理平台 |
| Volcano | Scheduling | 批量调度引擎 |
| VXLAN | Networking | 虚拟可扩展局域网 |

## 学习路径总览

### 新手入门（0-3个月）

1. **Fundamentals**: 理解 K8s 架构、核心组件、控制器模式
2. **Workloads**: 掌握 Pod、Deployment、Service 基本操作
3. **Configuration**: 学会 ConfigMap、Secret、Probe 配置
4. **Tooling**: 熟练使用 kubectl、Helm、Kind

### 中级进阶（3-12个月）

5. **Networking**: 深入 CNI、Service、Ingress、NetworkPolicy
6. **Storage**: 掌握 CSI、PV/PVC、StorageClass
7. **Scheduling**: 理解调度器、HPA/VPA、资源管理
8. **Observability**: 部署 Prometheus、Grafana、日志系统
9. **Operations**: 实践 GitOps、备份恢复、混沌工程

### 高级专家（1-3年）

10. **Security**: 构建零信任安全体系、供应链安全
11. **Platform Engineering**: 开发 Operator、构建 IDP
12. **Multi-Cloud**: 多集群管理、混合云架构
13. **Specialized Workloads**: GPU/AI、Serverless、边缘计算

## 缩略语总表

| 缩略语 | 全称 | 领域 |
|--------|------|------|
| API | Application Programming Interface | 基础 |
| CNI | Container Network Interface | 网络 |
| CRI | Container Runtime Interface | 基础 |
| CSI | Container Storage Interface | 存储 |
| CRD | Custom Resource Definition | 平台 |
| HPA | Horizontal Pod Autoscaler | 调度 |
| VPA | Vertical Pod Autoscaler | 调度 |
| IDP | Internal Developer Platform | 平台 |
| mTLS | Mutual TLS | 安全 |
| OCI | Open Container Initiative | 基础 |
| OPA | Open Policy Agent | 安全 |
| OTel | OpenTelemetry | 可观测性 |
| PSA | Pod Security Admission | 安全 |
| PV | Persistent Volume | 存储 |
| PVC | Persistent Volume Claim | 存储 |
| RBAC | Role-Based Access Control | 安全 |
| SLI | Service Level Indicator | 运维 |
| SLO | Service Level Objective | 运维 |
| SLA | Service Level Agreement | 运维 |
| SBOM | Software Bill of Materials | 安全 |

## 质量检查清单

- [ ] 每个子领域 index.md 均包含完整术语表
- [ ] 所有 wiki 链接指向有效文件
- [ ] 术语定义清晰、无歧义
- [ ] 生产案例真实可复现
- [ ] 命令经过验证可执行
- [ ] 版本兼容矩阵与当前版本一致
- [ ] FAQ 覆盖常见问题
- [ ] 学习路径逻辑连贯

## 知识字典维护规范

### 新增术语规范

每个新增术语必须包含：
1. **定义**：一句话清晰定义
2. **关键特性**：3-5 个核心特征
3. **典型实现**：具体工具/项目
4. **关联术语**：相关概念链接

### 内容更新规则

- 版本发布后 30 天内更新兼容矩阵
- 生产故障后补充案例
- 新项目进入 CNCF 后补充组件索引
- 每季度审查术语准确性

### 写作风格

- 术语定义简洁明了，避免冗余
- 生产案例包含：现象→根因→解决
- 命令标注风险等级（🔴🟡🟢）
- YAML 示例可直接复制使用
- 对比表格用于选型决策

## 常见误区与纠正

| 误区 | 正确理解 |
|------|----------|
| Secret 是加密的 | Secret 仅 base64 编码，需启用 etcd 加密 |
| Namespace 是安全边界 | Namespace 仅是逻辑隔离，需配合 RBAC/NetworkPolicy |
| HPA 可以缩容到 0 | HPA 最小副本数 ≥ 1，缩容到 0 用 KEDA |
| Ingress 可以处理 TCP | Ingress 仅 L7(HTTP)，TCP 用 Gateway API 或 LB |
| PV 删除后数据就没了 | 取决于 reclaimPolicy（Retain/Delete） |
| Pod IP 是固定的 | Pod IP 随重建变化，固定 IP 用 StatefulSet/Spiderpool |
| kubectl delete 立即删除 | 有 graceful shutdown 期，强制删除用 --force |
| 所有节点都能调度 Pod | 需检查 Taint/Toleration、NodeSelector |
| Docker 是 K8s 运行时 | K8s 1.24+ 移除 dockershim，用 containerd/CRI-O |
| Service Mesh 是必须的 | 简单场景用 K8s 原生即可，Mesh 适合复杂微服务 |
| etcd 可以用 HDD | etcd 必须 SSD，HDD 延迟导致集群不稳定 |
| 一个集群解决所有问题 | 大规模场景需多集群拆分（环境/地域/业务） |

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0 | 2026-07 | 初始创建，13 个子领域索引 |
| v2.0 | 2026-07 | 全域深化，每个子领域 500+ 行 |

## 快速参考：核心 kubectl 命令

```bash
# 集群信息
kubectl cluster-info
kubectl get nodes -o wide
kubectl top nodes

# 工作负载
kubectl get pods -A -o wide
kubectl describe pod <name> -n <ns>
kubectl logs <pod> -n <ns> --tail=100
kubectl exec -it <pod> -n <ns> -- /bin/sh

# 调试
kubectl get events -A --sort-by=.metadata.creationTimestamp
kubectl debug -it <pod> --image=nicolaka/netshoot
kubectl port-forward svc/<name> 8080:80 -n <ns>

# 资源管理
kubectl apply -f manifest.yaml
kubectl delete -f manifest.yaml
kubectl rollout status deployment/<name>
kubectl rollout undo deployment/<name>

# 扩缩容
kubectl scale deployment/<name> --replicas=3
kubectl autoscale deployment/<name> --min=2 --max=10 --cpu-percent=80
```

## 参考链接

- https://kubernetes.io/docs/concepts/
- https://kubernetes.io/docs/reference/
- https://www.cncf.io/projects/
- https://landscape.cncf.io/
- https://kubeweekly.io/
- https://github.com/cncf/curriculum
- https://kubernetes.io/docs/tutorials/
- https://kubernetes.io/blog/
- https://github.com/kubernetes/community
- https://kubernetes.io/docs/contribute/

## Related

- [[17-系统基础/05-速查卡/index.md|速查卡]]
- [[17-系统基础/04-K8s事件/index.md|K8s 事件]]
- [[17-系统基础/01-Linux/index.md|Linux 知识]]
- [[17-系统基础/02-硬件/index.md|硬件知识]]

