---
title: KubeArmor
description: 'description: ''| **适用场景** | 容器运行时安全 |'''
category: general
tags:
- cncf
- ecosystem
- kubelet
- prometheus
- grafana
- helm
- elasticsearch
- hpa
- daemonset
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KubeArmor 是什么
- 如何 KubeArmor
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeArmor
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
---

title: KubeArmor
description: '| **适用场景** | 容器运行时安全 |'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- prometheus
- grafana
- helm
- elasticsearch
- hpa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KubeArmor 是什么
- 如何 KubeArmor
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeArmor
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

# KubeArmor

> **成熟度**: Sandbox | **加入时间**: 2022-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubearmor.io |
| **GitHub** | https://github.com/kubearmor/KubeArmor |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Security & Compliance |
| **适用场景** | 容器运行时安全 |

---

## 项目概述

KubeArmor 是一个云原生运行时安全引擎，利用 Linux 安全模块 (LSM - AppArmor, BPF-LSM, SELinux) 在系统级别执行安全策略。它保护 Kubernetes Pod、容器和节点免受已知和未知的威胁，包括进程执行、文件访问和网络操作的细粒度控制。

---

## 核心特性

- **LSM 强制执行**: 基于 AppArmor/BPF-LSM/SELinux 内核级安全
- **进程控制**: 限制容器内可执行的进程
- **文件保护**: 控制文件/目录的读写访问
- **网络控制**: 限制容器的网络行为
- **系统调用过滤**: 细粒度的 syscall 控制
- **可观测性**: 实时安全遥测数据
- **零配置**: 默认安全姿态保护

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    KubeArmor Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                     User Interface                        │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │  karmor CLI │  │ KubeArmor   │  │  Kubernetes     │  │   │
│  │  │             │  │  Policy CRD │  │  API            │  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘  │   │
│  └─────────┼───────────────┼──────────────────┼────────────┘   │
│            │               │                  │                 │
│  ┌─────────▼───────────────▼──────────────────▼────────────┐   │
│  │              KubeArmor Daemon (DaemonSet)                │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                Core Components                       │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Policy    │  │  Container  │  │   Monitor   │  │ │   │
│  │  │  │  Manager    │  │  Runtime    │  │   Engine    │  │ │   │
│  │  │  │             │  │  Interface  │  │             │  │ │   │
│  │  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │ │   │
│  │  │         │                │               │          │ │   │
│  │  │  ┌──────▼────────────────▼───────────────▼──────┐  │ │   │
│  │  │  │              Enforcer Layer                   │  │ │   │
│  │  │  │  ┌─────────┐  ┌─────────┐  ┌─────────────┐  │  │ │   │
│  │  │  │  │AppArmor │  │BPF-LSM  │  │  SELinux    │  │  │ │   │
│  │  │  │  │Enforcer │  │Enforcer │  │  Enforcer   │  │  │ │   │
│  │  │  │  └─────────┘  └─────────┘  └─────────────┘  │  │ │   │
│  │  │  └──────────────────────────────────────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                   Linux Kernel                           │   │
│  │  ┌─────────────────────────────────────────────────────┐│   │
│  │  │            Linux Security Modules (LSM)             ││   │
│  │  │  ┌──────────┐  ┌──────────┐  ┌───────────────────┐ ││   │
│  │  │  │ AppArmor │  │ BPF-LSM  │  │    SELinux        │ ││   │
│  │  │  │          │  │  (eBPF)  │  │                   │ ││   │
│  │  │  └──────────┘  └──────────┘  └───────────────────┘ ││   │
│  │  └─────────────────────────────────────────────────────┘│   │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │               Telemetry & Alerts                          │   │
│  │  ┌────────────┐  ┌────────────┐  ┌──────────────────┐   │   │
│  │  │ Prometheus │  │  Grafana   │  │  Elasticsearch   │   │   │
│  │  │  Metrics   │  │ Dashboard  │  │  / SIEM          │   │   │
│  │  └────────────┘  └────────────┘  └──────────────────┘   │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **KubeArmor Daemon** | 节点级守护进程，执行安全策略 |
| **Policy Manager** | 管理和翻译安全策略到 LSM 规则 |
| **Enforcer** | 通过 LSM 在内核级执行策略 |
| **Monitor** | 收集安全遥测数据 |
| **karmor CLI** | 命令行管理工具 |

---

## 快速开始

### 安装 karmor CLI

```bash
# macOS/Linux
curl -sfL http://get.kubearmor.io/ | sh -s -- -b /usr/local/bin

# 验证
karmor version
```

### 安装 KubeArmor

```bash
# 一键安装
karmor install

# 或使用 Helm
helm repo add kubearmor https://kubearmor.github.io/charts
helm repo update

helm install kubearmor kubearmor/kubearmor \
  --namespace kubearmor \
  --create-namespace

# 验证
kubectl get pods -n kubearmor
karmor probe
```

---

## 安全策略

### 阻止特定进程执行

```yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: block-shell-access
  namespace: production
spec:
  selector:
    matchLabels:
      app: web-server
  process:
    matchPaths:
      - path: /bin/bash
      - path: /bin/sh
      - path: /usr/bin/python
      - path: /usr/bin/python3
      - path: /usr/bin/wget
      - path: /usr/bin/curl
    action: Block
  message: "Shell access blocked in production pods"
```

### 保护敏感文件

```yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: protect-sensitive-files
  namespace: default
spec:
  selector:
    matchLabels:
      app: backend
  file:
    matchPaths:
      - path: /etc/shadow
        readOnly: true
        ownerOnly: true
      - path: /etc/passwd
        readOnly: true
    matchDirectories:
      - dir: /etc/ssl/private/
        readOnly: true
        recursive: true
      - dir: /var/run/secrets/kubernetes.io/
        readOnly: true
        recursive: true
    action: Block
```

### 网络限制

```yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: restrict-network
  namespace: production
spec:
  selector:
    matchLabels:
      app: database
  network:
    matchProtocols:
      - protocol: TCP
        fromSource:
          - path: /usr/bin/curl
          - path: /usr/bin/wget
      - protocol: UDP
        fromSource:
          - path: /usr/bin/nc
    action: Block
```

### 允许列表模式 (白名单)

```yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: allow-only-nginx
  namespace: production
spec:
  selector:
    matchLabels:
      app: web
  process:
    matchPaths:
      - path: /usr/sbin/nginx
      - path: /usr/bin/envsubst
    action: Allow  # 只允许这些进程，其他全部阻止
  file:
    matchDirectories:
      - dir: /var/www/html/
        readOnly: true
        recursive: true
      - dir: /etc/nginx/
        readOnly: true
        recursive: true
    action: Allow
```

---

## 集群级策略

### KubeArmorClusterPolicy

```yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorClusterPolicy
metadata:
  name: cluster-block-crypto-miners
spec:
  selector:
    matchExpressions:
      - key: namespace
        operator: NotIn
        values: ["kube-system", "kubearmor"]
  process:
    matchPaths:
      - path: /usr/bin/xmrig
      - path: /usr/bin/minerd
    matchPatterns:
      - pattern: "*/cryptominer*"
    action: Block
```

### 主机策略

```yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorHostPolicy
metadata:
  name: protect-kubelet
spec:
  nodeSelector:
    matchLabels:
      kubernetes.io/os: linux
  process:
    matchPaths:
      - path: /usr/bin/kubelet
        ownerOnly: true
  file:
    matchDirectories:
      - dir: /var/lib/kubelet/
        readOnly: true
        recursive: true
        fromSource:
          - path: /usr/bin/kubelet
    action: Block
```

---

## 安全可观测性

### 实时监控

```bash
# 查看安全事件
karmor logs

# 过滤特定命名空间
karmor logs --namespace production

# 过滤特定 Pod
karmor logs --pod web-server-xxx

# JSON 输出
karmor logs --json

# 系统级事件
karmor logs --logFilter=system
```

### 安全遥测

```bash
# 查看安全摘要
karmor summary

# 获取推荐策略
karmor recommend -n production

# 生成策略建议
karmor recommend --image nginx:latest
```

---

## 默认安全姿态

### 全局默认行为

```yaml
# KubeArmor 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubearmor-config
  namespace: kubearmor
data:
  # 默认文件姿态: block 或 audit
  defaultFilePosture: audit
  # 默认网络姿态: block 或 audit
  defaultNetworkPosture: audit
  # 默认进程姿态: block 或 audit
  defaultCapabilitiesPosture: audit
```

### 注解控制

```yaml
# Pod 级别覆盖默认姿态
apiVersion: v1
kind: Pod
metadata:
  name: high-security-pod
  annotations:
    kubearmor-policy: enabled
    container.kubearmor.io/defaultFilePosture: block
    container.kubearmor.io/defaultNetworkPosture: block
```

---

## 与其他工具集成

### Prometheus 指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kubearmor
  namespace: kubearmor
spec:
  selector:
    matchLabels:
      app: kubearmor
  endpoints:
    - port: metrics
      interval: 15s
```

### SIEM 集成

```bash
# 配置日志输出到 Elasticsearch
karmor install --set kubearmor.alertThrottling=true \
  --set kubearmor.defaultPosture.file=block
```

---

## 最佳实践

1. **审计优先**: 先以 audit 模式运行，了解应用行为
2. **推荐策略**: 使用 `karmor recommend` 生成基线策略
3. **最小权限**: 使用 Allow 模式实现白名单
4. **渐进收紧**: 从宽松策略逐步收紧
5. **监控告警**: 配置安全事件告警
6. **容器加固**: 配合 seccomp 和 capabilities 使用

---

## 参考资源

- [官方文档](https://docs.kubearmor.io)
- [GitHub Repo](https://github.com/kubearmor/KubeArmor)
- [策略示例](https://github.com/kubearmor/KubeArmor/tree/main/getting-started)
- [安全遥测](https://docs.kubearmor.io/kubearmor/documentation/kubearmor-telemetry)
- [Slack 社区](https://kubearmor.io/slack)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
