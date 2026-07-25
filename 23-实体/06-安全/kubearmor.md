---
title: KubeArmor (entities)
description: '## 概述'
summary: 'KubeArmor 是一个云原生运行时安全引擎，利用 Linux 安全模块 (LSM - AppArmor, BPF-LSM, SELinux) 在系统级别执行安全策略。它保护 Kubernetes Pod、容器和节点免受已知和未知的威胁，包括进程执行、文件访问和网络操作的细粒度控制。'
category: entities
tags:
- k8s
- cncf
- security
- kubearmor
- prometheus
- grafana
- cilium
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeArmor 是什么
- 如何 KubeArmor
trigger_keywords:
- KubeArmor
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeArmor

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

KubeArmor 是由 Accuknox 开发的云原生运行时安全引擎，2021 年加入 CNCF Sandbox。它利用 Linux 安全模块（LSM - AppArmor、BPF-LSM、SELinux）在系统级别执行安全策略，保护 Kubernetes Pod、容器和节点免受已知和未知的威胁。KubeArmor 提供进程执行、文件访问和网络操作的细粒度控制，是容器运行时安全防护的重要工具。

## 核心特性

- **LSM 强制执行**: 基于 AppArmor、BPF-LSM、SELinux 的内核级安全策略
- **进程控制**: 限制容器内可执行的进程（白名单/黑名单）
- **文件保护**: 控制文件和目录的读写执行访问
- **网络控制**: 限制容器的网络连接行为
- **系统调用过滤**: 细粒度的 syscall 控制（基于 seccomp）
- **安全遥测**: 实时安全事件日志和 Prometheus 指标

## 架构

KubeArmor 由 KubeArmor Operator（管理策略部署）、KubeArmor DaemonSet（每个节点的策略执行器）和 Policy CRD 组成。DaemonSet 中的 KubeArmor 进程监听 K8s API 获取 KubeArmorPolicy 和 KubeArmorHostPolicy CRD，将策略翻译为 AppArmor Profile 或 BPF-LSM 程序，加载到容器运行时和主机内核。当容器内进程触发策略规则时，LSM 拦截操作（Allow/Audit/Block），KubeArmor 将安全事件记录为日志并导出为 Prometheus 指标。karmor CLI 工具辅助策略生成和测试。

## Kubernetes 集成

KubeArmor 通过 KubeArmorPolicy CRD（命名空间级）和 KubeArmorHostPolicy CRD（节点级）声明式管理安全策略。策略通过标签选择器匹配目标 Pod。DaemonSet 以特权模式运行，加载 LSM 策略到节点内核。支持三种策略动作：Allow（白名单模式，仅允许列出的操作）、Audit（记录但不阻止）、Block（阻止并记录）。与容器运行时（containerd、CRI-O）集成，自动为容器应用 AppArmor Profile。

## 生产使用场景

1. **容器加固**: 限制容器只能执行必要的进程和访问必要的文件
2. **合规要求**: 满足 PCI-DSS、HIPAA 等安全合规对运行时防护的要求
3. **零信任安全**: 实施 "deny by default" 策略，最小化攻击面
4. **入侵检测**: 以 Audit 模式运行，检测异常进程执行和文件访问

## 安装

```bash
# Helm 安装
helm repo add kubearmor https://kubearmor.github.io/charts
helm install kubearmor kubearmor/kubearmor-operator -n kubearmor --create-namespace

# 验证部署
kubectl get pods -n kubearmor
kubectl get crd | grep kubearmor
```

### 安全策略配置

```yaml
# 阻止特定进程执行
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: ksp-block-exec
  namespace: default
spec:
  severity: 8
  selector:
    matchLabels:
      app: web
  process:
    matchPaths:
    - path: /bin/bash
    - path: /bin/sh
  action: Block
---
# 文件系统只读保护
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: ksp-readonly-etc
  namespace: default
spec:
  severity: 7
  selector:
    matchLabels:
      app: web
  file:
    matchDirectories:
    - dir: /etc/
      readOnly: true
  action: Block
---
# 网络访问控制
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: ksp-restrict-network
  namespace: default
spec:
  severity: 5
  selector:
    matchLabels:
      app: web
  network:
    matchProtocols:
    - protocol: tcp
      command: ["curl", "wget"]
  action: Audit
```

### 策略建议生成

```bash
# 基于 Pod 行为生成策略建议
karmor recommend --pod web-app --namespace default

# 应用生成的策略
kubectl apply -f recommended-policies/
```

## 运维操作

```bash
# 🟢 查看安全策略
kubectl get ksp -A
kubectl describe ksp ksp-block-exec

# 🟢 查看安全日志
karmor logs --pod web-app -n default

# 🟢 检查策略执行状态
kubectl get ksp -o wide

# 🟡 应用新策略
kubectl apply -f new-policy.yaml

# 🟡 切换策略模式（Audit → Block）
kubectl patch ksp ksp-restrict-network --type merge -p '{"spec":{"action":"Block"}}'

# 🔴 删除安全策略（会解除保护）
kubectl delete ksp ksp-block-exec

# 🔴 禁用 KubeArmor
kubectl scale deployment kubearmor -n kubearmor --replicas=0
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 策略未生效 | LSM 未启用 | `kubectl logs -n kubearmor -l app=kubearmor` | 确认内核支持 AppArmor/SELinux/BPF-LSM |
| Pod 被意外阻止 | 策略过严 | `karmor logs --pod <pod>` | 切换为 Audit 模式观察 |
| KubeArmor CrashLoop | 内核不兼容 | `kubectl logs -n kubearmor` | 确认内核 >= 5.4，检查 BTF |
| 日志丢失 | 日志量过大 | `kubectl top pod -n kubearmor` | 调整日志采集频率 |
| 性能下降 | 策略过多 | `kubectl get ksp -A \| wc -l` | 合并策略，减少规则数 |

**排查流程：**
```
策略未生效
├── 检查 KubeArmor 状态 → kubectl get pods -n kubearmor
├── 检查 LSM 支持 → cat /sys/kernel/security/lsm
├── 检查策略状态 → kubectl describe ksp <name>
├── 检查 Pod 标签匹配 → kubectl get pod --show-labels
└── 查看安全日志 → karmor logs --pod <pod>
```

## 生产案例

### 案例一：容器逃逸防护

- **场景**: 多租户集群，需防止容器内用户执行危险操作（如访问 /proc、执行 shell）
- **排查**: 使用 KubeArmor 阻止 /bin/bash、/bin/sh 执行，限制 /proc 访问
- **方案**: 为所有租户 Pod 应用基线安全策略，阻止特权操作，允许业务进程
- **效果**: 阻止了 12 次容器逃逸尝试，安全事件降低 90%

### 案例二：合规审计

- **场景**: 金融合规要求记录所有容器内文件访问和网络连接
- **排查**: 使用 KubeArmor Audit 模式记录所有文件/网络/进程事件
- **方案**: 配置 Audit 策略覆盖关键目录和网络协议，日志发送到 SIEM
- **效果**: 满足等保三级要求，审计日志完整，无性能影响

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **KubeArmor** | LSM 内核级、策略丰富 | LSM 支持因 OS 而异 | 强制执行 |
| Falco | 运行时检测、eBPF 原生 | 仅检测，不执行阻止 | 威胁检测 |
| Tetragon | eBPF 高性能 | 配置复杂 | 高性能场景 |
| NeuVector | 全栈安全 | 商业产品 | 企业全栈 |

## 架构定位

在 CNCF 生态中，KubeArmor 属于 **Security / Runtime Protection** 类别，是容器运行时强制执行（Enforcement）的代表性项目。它与 Falco（检测）、Cilium（网络）互补。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- networking.md|cilium-ebpf-networking]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[argo]] — Argo Workflows
- [[keycloak]] — Keycloak
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubearmor
- [[23-实体/06-安全/tokenetes.md|Tokenetes]]
- [[23-实体/06-安全/containerssh.md|ContainerSSH]]
- [[23-实体/06-安全/parsec.md|Parsec]]
- [[23-实体/06-安全/athenz.md|Athenz]]
- [[23-实体/06-安全/keylime.md|Keylime]]
- [[23-实体/06-安全/cartography.md|Cartography]]
- [[23-实体/06-安全/bank-vaults.md|Bank-Vaults]]
- [[23-实体/06-安全/hexa.md|Hexa]]
- [[23-实体/06-安全/paralus.md|Paralus]]
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
