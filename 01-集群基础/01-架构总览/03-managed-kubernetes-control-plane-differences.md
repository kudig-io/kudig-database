---
title: 托管 Kubernetes 控制平面组件差异
description: 对比 AWS EKS、Azure AKS、Google GKE、阿里云 ACK、腾讯云 TKE、华为云 CCE 在核心控制平面组件（apiserver、etcd、scheduler、controller-manager、CCM、kubelet、kube-proxy）上的托管模型、可见性与运维限制。
summary: 对比六大主流托管 Kubernetes 服务在核心控制平面组件上的托管模型、可见性与运维限制，帮助平台工程师理解不同云厂商对 Kubernetes 组件的封装程度和运维责任边界。
category: 集群基础
tags:
- k8s
- managed-kubernetes
- eks
- aks
- gke
- ack
- tke
- cce
- control-plane
- apiserver
- etcd
- kubelet
- kube-proxy
- cloud-controller-manager
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 托管 Kubernetes 控制平面组件差异
- EKS AKS GKE ACK 控制平面有什么区别
- 托管 K8s etcd 对用户是否可见
trigger_keywords:
- managed-kubernetes
- 托管 Kubernetes
- 控制平面差异
- EKS
- AKS
- GKE
- ACK
prerequisites:
- kubectl-basics
- kubernetes-concepts
- cloud-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../云厂商/
  label: '相关知识域: 云厂商'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 托管 Kubernetes 控制平面组件差异

> **适用版本**: v1.28 - v1.32 | **最后更新**: 2026-07

选择托管 Kubernetes 服务时，核心差异往往不在于 Kubernetes API 本身，而在于**云厂商对控制平面组件的托管深度、可见性限制和运维责任边界**。本文对比 AWS EKS、Azure AKS、Google GKE、阿里云 ACK、腾讯云 TKE、华为云 CCE 在关键组件上的差异。

---

<!-- chunk: 1. 控制平面托管模型总览 -->
## 1. 控制平面托管模型总览

| 服务 | 控制平面位置 | Master 节点可见性 | etcd 可见性 | 运维责任 |
|------|-------------|------------------|------------|---------|
| **AWS EKS** | AWS 托管 VPC | ❌ 不可见 | ❌ 不可见 | AWS 负责控制平面高可用与升级 |
| **Azure AKS** | Azure 托管资源组 | ❌ 不可见 | ❌ 不可见 | Azure 负责控制平面与 etcd |
| **Google GKE** | Google 托管 | ❌ 不可见（Autopilot 完全托管） | ❌ 不可见 | Google 负责控制平面、节点与网络 |
| **阿里云 ACK** | 托管版：阿里云托管；专有版：用户可见 | 托管版 ❌ / 专有版 ✅ | 专有版 ✅ | 托管版阿里云负责；专有版用户负责 |
| **腾讯云 TKE** | 托管版：腾讯云托管；独立部署版：用户可见 | 托管版 ❌ / 独立版 ✅ | 独立版 ✅ | 托管版腾讯云负责 |
| **华为云 CCE** | 托管版/CCE Turbo：华为云托管；非托管版：用户可见 | 托管版 ❌ / 非托管版 ✅ | 非托管版 ✅ | 托管版华为云负责 |

> **关键结论**：除阿里云 ACK 专有版、TKE 独立部署版、CCE 非托管版外，主流托管服务的控制平面组件对用户完全不可见，无法直接 SSH 到 Master 节点排查。

---

<!-- chunk: 2. 核心组件差异 -->
## 2. 核心组件差异

### 2.1 kube-apiserver

| 维度 | EKS | AKS | GKE | ACK 托管版 | TKE 托管版 | CCE 托管版 |
|------|-----|-----|-----|-----------|-----------|-----------|
| **可见性** | 仅通过 API Endpoint 访问 | 仅通过 API Endpoint 访问 | 仅通过 API Endpoint 访问 | 仅通过 API Endpoint 访问 | 仅通过 API Endpoint 访问 | 仅通过 API Endpoint 访问 |
| **公开 API Server** | 默认公网可访问（可关闭） | 默认公网可访问（可关闭） | 默认公网可访问（可关闭） | 默认私网 + 公网可选 | 默认公网可访问（可关闭） | 默认私网 + 公网可选 |
| **API 版本** | 紧跟上游，但延迟 1-3 个月 | 通常较快 | 最快，常与上游同步 | 基于阿里云 K8s 发行版 | 基于 TencentOS 发行版 | 基于华为云 EulerOS 发行版 |
| **可配置参数** | 极有限 | 有限 | 有限 | 有限（Pro 版可定制） | 有限 | 有限 |
| **审计日志** | 需启用控制平面日志到 CloudWatch | 需启用诊断设置到 Monitor | 默认启用，可导出 | 可采集到 SLS | 可采集到 CLS | 可采集到 LTS |

### 2.2 etcd

| 维度 | EKS | AKS | GKE | ACK 托管版 | TKE 托管版 | CCE 托管版 |
|------|-----|-----|-----|-----------|-----------|-----------|
| **用户可见性** | ❌ 完全托管 | ❌ 完全托管 | ❌ 完全托管 | ❌ 托管版不可见；专有版可见 | ❌ 托管版不可见 | ❌ 托管版不可见 |
| **备份责任** | AWS 自动备份，用户不可直接恢复 | Azure 自动备份 | Google 自动管理 | 阿里云自动备份；专有版用户负责 | 腾讯云自动备份 | 华为云自动备份 |
| **恢复能力** | 只能通过 EKS 控制平面恢复 | 通过 AKS 支持流程 | 自动处理 | 托管版工单恢复；专有版可手动恢复 | 托管版工单恢复 | 托管版工单恢复 |
| **etcd 事件** | 通过 CloudWatch 暴露 | 通过 Azure Monitor 暴露 | 通过 Cloud Logging 暴露 | 通过 Prometheus/云监控暴露 | 通过云监控暴露 | 通过云监控暴露 |
| **空间管理** | 自动 compaction | 自动 compaction | 自动 compaction | 自动 compaction；专有版需关注 | 自动 compaction | 自动 compaction |

### 2.3 kube-scheduler

| 维度 | EKS | AKS | GKE | ACK 托管版 | TKE 托管版 | CCE 托管版 |
|------|-----|-----|-----|-----------|-----------|-----------|
| **默认调度器** | 上游 kube-scheduler | 上游 kube-scheduler | 上游 + GKE 优化 | 上游 + ACK 优化 | 上游 + TKE 优化 | 上游 + CCE 优化 |
| **自定义调度器** | ✅ 支持 KubeSchedulerConfiguration | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 支持 |
| **多调度器 Profile** | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 支持 |
| **可见性** | 不可见进程，可看调度事件 | 不可见进程 | 不可见进程 | 托管版不可见 | 托管版不可见 | 托管版不可见 |
| **调度指标** | 通过 control plane logging 暴露 | 通过 Monitor 暴露 | 通过 Cloud Monitoring 暴露 | 可通过托管 Prometheus 暴露 | 云监控暴露 | 云监控暴露 |

### 2.4 kube-controller-manager

| 维度 | EKS | AKS | GKE | ACK 托管版 | TKE 托管版 | CCE 托管版 |
|------|-----|-----|-----|-----------|-----------|-----------|
| **内置控制器** | 上游完整控制器集 | 上游完整控制器集 | 上游完整 + GKE 专用控制器 | 上游 + 阿里云扩展 | 上游 + 腾讯云扩展 | 上游 + 华为云扩展 |
| **Leader Election** | 托管，用户不可见 | 托管 | 托管 | 托管；专有版可见 | 托管 | 托管 |
| **节点驱逐参数** | 默认值，不可调 | 部分可调 | 部分可调 | 部分可调 | 部分可调 | 部分可调 |
| **CCM 集成** | AWS CCM 独立部署 | Azure CCM 独立部署 | GCE CCM 集成 | 阿里云 CCM 独立部署 | 腾讯云 CCM 独立部署 | 华为云 CCM 独立部署 |

### 2.5 cloud-controller-manager (CCM)

| 维度 | EKS | AKS | GKE | ACK | TKE | CCE |
|------|-----|-----|-----|-----|-----|-----|
| **部署方式** | 独立 Deployment（aws-cloud-controller-manager） | 托管，用户不可见 | 集成在 GKE 控制平面 | 独立 Deployment（alicloud-controller-manager） | 独立 Deployment | 独立 Deployment |
| **节点控制器** | AWS CCM 提供 providerID、external IP | Azure CCM | GCE CCM | 阿里云 CCM | 腾讯云 CCM | 华为云 CCM |
| **路由控制器** | VPC CNI 负责路由 | Azure CNI / Kubenet 负责 | GKE CNI 负责 | Terway / Flannel 负责 | VPC-CNI 负责 | CNI 插件负责 |
| **Service 控制器** | 创建 AWS ELB/NLB | 创建 Azure Load Balancer | 创建 GCP Load Balancer | 创建阿里云 SLB | 创建腾讯云 CLB | 创建华为云 ELB |
| **权限模型** | IRSA / Pod Identity | AKS Managed Identity | GKE Workload Identity | ACK RAM Role / RRSA | TKE CAM Role | CCE IAM |

### 2.6 kubelet

| 维度 | EKS | AKS | GKE | ACK | TKE | CCE |
|------|-----|-----|-----|-----|-----|-----|
| **节点类型** | Managed Node / Self-managed / Fargate | System Node Pool / User Node Pool / Virtual Nodes | Standard / Autopilot | 托管节点池 / 专有节点 | 托管节点 / 独立部署 | 虚拟机 / 裸金属 / Turbo |
| **kubelet 可配置性** | 通过 Managed Node Group 启动模板部分可调 | 通过 Node Configuration 可调 | Autopilot 不可调；Standard 部分可调 | 节点池配置可调 | 节点池配置可调 | 节点池配置可调 |
| **容器运行时** | containerd（默认） | containerd（默认） | containerd（默认） | containerd（默认） | containerd（默认） | containerd/iSulad |
| **最大 Pods/节点** | 受 ENI 限制（通常 110-250） | Azure CNI 250；Kubenet 110 | 110 默认，可扩 | Terway 128-256；Flannel 110 | 64-256 | 256 |
| **证书轮换** | 自动 | 自动 | 自动 | 自动；专有版需关注 | 自动 | 自动 |
| **节点 OS** | Amazon Linux 2/2023、Bottlerocket、Ubuntu | Ubuntu、Azure Linux、Windows | Container-Optimized OS、Ubuntu | Alibaba Cloud Linux、CentOS、Windows | Tencent Linux、Ubuntu | EulerOS、CentOS、Windows |

### 2.7 kube-proxy

| 维度 | EKS | AKS | GKE | ACK | TKE | CCE |
|------|-----|-----|-----|-----|-----|-----|
| **默认模式** | iptables（旧版）；ipvs 可选 | iptables | iptables（可切换 ipvs） | iptables / ipvs 可选 | iptables | iptables |
| **部署方式** | Self-managed addon（kube-proxy DaemonSet） | 托管 DaemonSet | 托管 | 托管 DaemonSet | 托管 DaemonSet | 托管 DaemonSet |
| **可替换为 eBPF** | ✅ Cilium 可替换 | ✅ Cilium 可替换 | ✅ Dataplane V2（Cilium eBPF） | ✅ Cilium / Terway eBPF | ✅ Cilium 可替换 | ✅ Cilium 可替换 |
| **NodePort 行为** | 节点安全组需放行 | NSG 需放行 | 防火墙规则需放行 | 安全组需放行 | 安全组需放行 | 安全组需放行 |

---

<!-- chunk: 3. 运维可见性与诊断 -->
## 3. 运维可见性与诊断

### 3.1 控制平面日志/指标

| 服务 | API Server 日志 | Audit 日志 | 控制平面指标 | etcd 指标 |
|------|----------------|-----------|-------------|----------|
| EKS | CloudWatch Logs | CloudWatch / S3 | 通过 AMP/CloudWatch | 不暴露 |
| AKS | Azure Monitor | Azure Monitor | Azure Managed Prometheus | 不暴露 |
| GKE | Cloud Logging | Cloud Logging | Cloud Monitoring | 不暴露 |
| ACK | SLS | SLS | 阿里云托管 Prometheus | 专有版暴露 |
| TKE | CLS | CLS | 腾讯云 Prometheus | 不暴露 |
| CCE | LTS | LTS | 华为云 AOM | 不暴露 |

### 3.2 常用诊断命令

```bash
# 🟢 查看 API Server 版本与平台信息
kubectl version -o yaml
kubectl get nodes -o wide

# 🟢 查看控制平面组件 Pod（仅自管/专有版可见）
kubectl get pods -n kube-system -l tier=control-plane

# 🟢 查看 Lease / Leader 信息
kubectl get lease -n kube-system

# 🟢 查看 CCM 状态
kubectl get pods -n kube-system | grep controller-manager

# 🟢 查看 kube-proxy 模式与状态
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "Using"
```

---

<!-- chunk: 4. 版本升级策略差异 -->
## 4. 版本升级策略差异

| 服务 | 升级方式 | 控制面与数据面解耦 | 版本跳过限制 | 回滚能力 |
|------|---------|------------------|------------|---------|
| EKS | 控制台 / CLI / API | ✅ 支持 | 一次最多跳 1 个小版本 | 控制面不可回滚；数据面可回滚节点组 |
| AKS | 控制台 / CLI | ✅ 支持 | 一次最多跳 1 个小版本 | 控制面不可回滚；节点池可回滚 |
| GKE | 自动升级（可配置维护窗口） | ✅ 支持 | 建议逐版本 | 控制面不可回滚；节点可回滚 |
| ACK | 控制台 / API | ✅ 支持 | 一次最多跳 1 个小版本 | 控制面不可回滚；节点池可回滚 |
| TKE | 控制台 / API | ✅ 支持 | 一次最多跳 1 个小版本 | 控制面不可回滚；节点池可回滚 |
| CCE | 控制台 / API | ✅ 支持 | 一次最多跳 1 个小版本 | 控制面不可回滚；节点池可回滚 |

> **共同约束**：所有托管服务控制平面一旦升级，通常**不支持回滚**，升级前务必在测试集群验证。

---

<!-- chunk: 5. 生产选型建议 -->
## 5. 生产选型建议

| 场景 | 推荐服务 | 理由 |
|------|---------|------|
| 需要完全托管，不想运维控制平面 | GKE Autopilot / EKS / AKS | 控制平面完全托管，自动扩缩容 |
| 需要访问 Master/etcd 做深度排错 | ACK 专有版 / TKE 独立部署 | 用户可见控制平面组件 |
| 强合规/金融，需要审计控制平面 | ACK 专有版 / EKS 私有集群 + CloudTrail | 控制平面可审计，etcd 可自管 |
| 多区域/全球部署 | GKE / EKS | 全球区域覆盖最广 |
| 与云原生生态深度集成 | GKE（Anthos）/ EKS | 生态成熟，周边工具丰富 |
| 国内部署 + 阿里云生态 | ACK 托管版 / 专有版 | 与 SLB、SLS、RAM 深度集成 |
| 边缘/混合云 | EKS Anywhere / ACK Edge / CCE | 支持边缘与本地部署 |

---

<!-- chunk: 6. 检查清单 -->
## 6. 检查清单

- [ ] 明确各云厂商对控制平面组件的可见性与运维责任边界
- [ ] 确认 etcd 备份/恢复策略是否符合 RPO/RTO 要求
- [ ] 确认 API Server 访问方式（公网/私网）与安全组策略
- [ ] 确认 CCM 所需云 IAM/RAM 权限已最小化配置
- [ ] 确认 kube-proxy 模式与集群规模匹配
- [ ] 确认控制平面升级策略与回滚能力
- [ ] 确认控制平面日志、审计、指标已接入可观测性平台
- [ ] 确认节点 OS、运行时、cgroup driver 与平台建议一致

---

## Related

- [[01-集群基础/01-架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]]
- [[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|kube-apiserver 深度解析]]
- [[01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd 深度解析]]
- [[01-集群基础/03-控制平面/14-cloud-controller-manager-deep-dive.md|cloud-controller-manager 深度解析]]
- [[18-云厂商/07-多云混合/11-multicloud-comparison-decision-matrix.md|多云对比决策矩阵]]
- [[18-云厂商/02-AWS-EKS/aws-eks-overview.md|AWS EKS 概述]]
- [[18-云厂商/01-阿里云/02-ACK集群运维.md|ACK 集群运维]]


<!-- risk-assessed -->
