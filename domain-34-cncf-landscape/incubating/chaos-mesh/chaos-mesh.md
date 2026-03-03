# Chaos Mesh

> **成熟度**: Incubating | **加入时间**: 2020-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://chaos-mesh.org |
| **GitHub** | https://github.com/chaos-mesh/chaos-mesh |
| **文档** | https://chaos-mesh.org/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability |

---

## 项目概述

### 简介
Chaos Mesh 是 Kubernetes 云原生混沌工程平台，由 PingCAP 开源。它提供丰富的故障注入能力，帮助在生产环境问题发生前发现系统潜在弱点。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2019-12 | PingCAP 开源 |
| 2020-07 | 加入 CNCF Sandbox |
| 2022-02 | 晋升为 CNCF Incubating |

### 核心定位
Chaos Mesh 是 Kubernetes 生态最完整的混沌工程平台，支持多种故障类型和复杂的实验编排。

---

## 故障类型

```
┌─────────────────────────────────────────────────────────────────┐
│                   Chaos Mesh 故障类型                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Pod 故障                         网络故障                       │
│  ┌─────────────────┐             ┌─────────────────┐            │
│  │ • Pod Kill      │             │ • Network Delay │            │
│  │ • Pod Failure   │             │ • Packet Loss   │            │
│  │ • Container Kill│             │ • Partition     │            │
│  └─────────────────┘             │ • Bandwidth     │            │
│                                  └─────────────────┘            │
│                                                                  │
│  IO 故障                          时间故障                       │
│  ┌─────────────────┐             ┌─────────────────┐            │
│  │ • IO Delay      │             │ • Time Skew     │            │
│  │ • IO Error      │             │ • Time Stop     │            │
│  │ • IO Attr       │             └─────────────────┘            │
│  └─────────────────┘                                            │
│                                                                  │
│  压力故障                         内核故障                       │
│  ┌─────────────────┐             ┌─────────────────┐            │
│  │ • CPU Stress    │             │ • Kernel Fault  │            │
│  │ • Memory Stress │             └─────────────────┘            │
│  └─────────────────┘                                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 使用示例

### 网络延迟

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-delay
spec:
  action: delay
  mode: all
  selector:
    namespaces:
      - default
    labelSelectors:
      app: web
  delay:
    latency: "100ms"
    jitter: "10ms"
  duration: "5m"
```

### Pod 故障

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-kill
spec:
  action: pod-kill
  mode: one
  selector:
    namespaces:
      - default
    labelSelectors:
      app: backend
  scheduler:
    cron: "@every 5m"
```

### 工作流编排

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: Workflow
metadata:
  name: chaos-workflow
spec:
  entry: serial-chaos
  templates:
    - name: serial-chaos
      templateType: Serial
      children:
        - network-delay
        - pod-kill
    - name: network-delay
      templateType: NetworkChaos
      networkChaos:
        action: delay
        delay:
          latency: "50ms"
    - name: pod-kill
      templateType: PodChaos
      podChaos:
        action: pod-kill
```

---

## 安装

```bash
helm repo add chaos-mesh https://charts.chaos-mesh.org
helm install chaos-mesh chaos-mesh/chaos-mesh \
  -n chaos-mesh --create-namespace
```

---

## 参考资源

- [官方文档](https://chaos-mesh.org/docs)
- [GitHub Repo](https://github.com/chaos-mesh/chaos-mesh)
- [CNCF 项目页面](https://www.cncf.io/projects/chaos-mesh/)

---

**维护者**: Kudig Team | **许可证**: MIT
