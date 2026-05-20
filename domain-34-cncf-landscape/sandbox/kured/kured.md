---
title: Kured
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- daemonset
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kured 是什么
- 如何 Kured
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kured
- cncf
- landscape
---


# Kured

> **成熟度**: Sandbox | **加入时间**: 2021-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kured.dev |
| **GitHub** | https://github.com/kubereboot/kured |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Scheduling & Orchestration |
| **适用场景** | Kubernetes 节点自动重启 |

---

## 项目概述

Kured (KUbernetes REboot Daemon) 是一个 Kubernetes 守护进程，用于在节点需要重启时安全地执行重启操作。它检测节点上的重启信号 (如 /var/run/reboot-required 文件)，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐工作负载。

---

## 核心特性

- **自动检测**: 检测系统重启需求信号
- **协调重启**: 一次只重启一个节点，避免服务中断
- **Cordon/Drain**: 重启前自动隔离和驱逐 Pod
- **时间窗口**: 支持配置允许重启的时间窗口
- **Prometheus 集成**: 暴露指标供监控
- **通知集成**: 支持 Slack、Teams 等通知
- **锁机制**: 使用 Kubernetes annotation 实现分布式锁

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Kured Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Kubernetes Cluster                      │   │
│  │                                                           │   │
│  │  ┌───────────────────────────────────────────────────┐   │   │
│  │  │                Kured DaemonSet                     │   │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐ │   │   │
│  │  │  │  Node 1     │  │   Node 2    │  │   Node 3   │ │   │   │
│  │  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌────────┐ │ │   │   │
│  │  │  │ │ Kured   │ │  │ │ Kured   │ │  │ │ Kured  │ │ │   │   │
│  │  │  │ │  Pod    │ │  │ │  Pod    │ │  │ │  Pod   │ │ │   │   │
│  │  │  │ └────┬────┘ │  │ └────┬────┘ │  │ └───┬────┘ │ │   │   │
│  │  │  │      │      │  │      │      │  │     │      │ │   │   │
│  │  │  │ ┌────▼────┐ │  │ ┌────▼────┐ │  │┌────▼────┐ │ │   │   │
│  │  │  │ │Sentinel │ │  │ │Sentinel │ │  ││Sentinel │ │ │   │   │
│  │  │  │ │ File    │ │  │ │ File    │ │  ││ File    │ │ │   │   │
│  │  │  │ │Watcher  │ │  │ │Watcher  │ │  ││Watcher  │ │ │   │   │
│  │  │  │ └─────────┘ │  │ └─────────┘ │  │└─────────┘ │ │   │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘ │   │   │
│  │  └───────────────────────────────────────────────────┘   │   │
│  │                           │                               │   │
│  │                    Lock Coordination                      │   │
│  │                    (Node Annotation)                      │   │
│  │                           │                               │   │
│  │  ┌────────────────────────▼──────────────────────────┐   │   │
│  │  │               Reboot Process                       │   │   │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────┐ │   │   │
│  │  │  │ Acquire │──►│ Cordon  │──►│  Drain  │──►│Reboot │ │   │   │
│  │  │  │  Lock   │  │  Node   │  │   Node  │  │ Node  │ │   │   │
│  │  │  └─────────┘  └─────────┘  └─────────┘  └───────┘ │   │   │
│  │  │       │            │            │           │      │   │   │
│  │  │       ▼            ▼            ▼           ▼      │   │   │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────┐ │   │   │
│  │  │  │Uncordon │◄─│  Wait   │◄─│ System  │◄─│Release│ │   │   │
│  │  │  │  Node   │  │ Ready   │  │ Reboot  │  │ Lock  │ │   │   │
│  │  │  └─────────┘  └─────────┘  └─────────┘  └───────┘ │   │   │
│  │  └────────────────────────────────────────────────────┘   │   │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  Host Filesystem                          │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  /var/run/reboot-required (Ubuntu/Debian)           │ │   │
│  │  │  /var/run/reboot-required.pkgs                      │ │   │
│  │  │  Custom sentinel file path (configurable)           │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Kured DaemonSet** | 在每个节点运行的守护进程 |
| **Sentinel Watcher** | 监控重启信号文件 |
| **Lock Manager** | 分布式锁，确保串行重启 |
| **Reboot Executor** | 执行 cordon、drain、reboot 流程 |

---

## 快速开始

### Helm 安装

```bash
# 添加 Helm 仓库
helm repo add kubereboot https://kubereboot.github.io/charts
helm repo update

# 安装 Kured
helm install kured kubereboot/kured \
  --namespace kured \
  --create-namespace \
  --set configuration.startTime="2am" \
  --set configuration.endTime="6am" \
  --set configuration.rebootDays="mon,tue,wed,thu,fri"
```

### Manifest 安装

```bash
kubectl apply -f https://github.com/kubereboot/kured/releases/latest/download/kured-ds.yaml
```

---

## 配置参数

### 完整配置示例

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kured
  namespace: kured
spec:
  selector:
    matchLabels:
      name: kured
  template:
    metadata:
      labels:
        name: kured
    spec:
      tolerations:
        - key: node-role.kubernetes.io/control-plane
          effect: NoSchedule
        - key: node-role.kubernetes.io/master
          effect: NoSchedule
      hostPID: true
      containers:
        - name: kured
          image: ghcr.io/kubereboot/kured:latest
          command:
            - /usr/bin/kured
            - --reboot-sentinel=/var/run/reboot-required
            - --reboot-sentinel-command=ls /var/run/reboot-required
            - --start-time=2am
            - --end-time=6am
            - --time-zone=Asia/Shanghai
            - --reboot-days=mon,tue,wed,thu,fri
            - --period=1h
            - --ds-namespace=kured
            - --ds-name=kured
            - --lock-annotation=kured.weave.works/lock
            - --lock-ttl=4h
            - --prometheus-url=http://prometheus:9090
            - --alert-filter-regexp=^RebootRequired$
            - --alert-firing-only=true
            - --slack-hook-url=https://hooks.slack.com/services/xxx
            - --slack-username=kured
            - --slack-channel=#ops
            - --message-template-drain=Draining node %s for reboot
            - --message-template-reboot=Rebooting node %s
            - --message-template-uncordon=Node %s back online
            - --drain-grace-period=600
            - --drain-timeout=0
            - --skip-wait-for-delete-timeout=60
            - --drain-pod-selector=app!=critical
            - --prefer-no-schedule-taint=kured
          env:
            - name: KURED_NODE_ID
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
          securityContext:
            privileged: true
          volumeMounts:
            - name: hostrun
              mountPath: /var/run
      volumes:
        - name: hostrun
          hostPath:
            path: /var/run
```

### 关键参数说明

| 参数 | 说明 | 默认值 |
|:---|:---|:---|
| `--reboot-sentinel` | 重启信号文件路径 | /var/run/reboot-required |
| `--start-time` | 允许重启的开始时间 | 无限制 |
| `--end-time` | 允许重启的结束时间 | 无限制 |
| `--reboot-days` | 允许重启的日期 | 每天 |
| `--period` | 检查间隔 | 1h |
| `--lock-ttl` | 锁超时时间 | 0 (永不超时) |
| `--drain-grace-period` | Pod 优雅终止时间 | -1 (使用 Pod 设置) |
| `--drain-timeout` | Drain 超时 | 0 (无超时) |

---

## Helm Values 配置

```yaml
# values.yaml
configuration:
  # 重启信号文件
  rebootSentinel: /var/run/reboot-required
  
  # 时间窗口配置
  startTime: "2:00"
  endTime: "6:00"
  timeZone: "Asia/Shanghai"
  rebootDays: [mon, tue, wed, thu, fri]
  
  # 检查周期
  period: 1h
  
  # 锁配置
  lockTtl: 4h
  lockAnnotation: kured.weave.works/lock
  lockReleaseDelay: 5m
  
  # Drain 配置
  drainGracePeriod: 600
  drainTimeout: 0
  skipWaitForDeleteTimeout: 60
  drainPodSelector: ""
  
  # Prometheus 集成
  prometheusUrl: ""
  alertFilterRegexp: ""
  alertFiringOnly: false
  
  # Slack 通知
  slackHookUrl: ""
  slackUsername: "kured"
  slackChannel: ""
  
  # 消息模板
  messageTemplateDrain: "Draining node %s"
  messageTemplateReboot: "Rebooting node %s"
  messageTemplateUncordon: "Node %s is back"
  
  # 并发控制
  concurrency: 1

# 容忍度配置
tolerations:
  - key: node-role.kubernetes.io/control-plane
    effect: NoSchedule
  - key: node-role.kubernetes.io/master
    effect: NoSchedule

# 资源限制
resources:
  limits:
    cpu: 100m
    memory: 128Mi
  requests:
    cpu: 10m
    memory: 64Mi

# 指标配置
metrics:
  create: true
  service:
    port: 8080
```

---

## 使用场景

### 基于 unattended-upgrades (Ubuntu)

```bash
# /etc/apt/apt.conf.d/50unattended-upgrades
Unattended-Upgrade::Automatic-Reboot "false";  # 禁用自动重启
# Kured 会检测 /var/run/reboot-required 并处理重启
```

### 自定义重启信号

```bash
# 创建自定义信号文件触发重启
sudo touch /var/run/reboot-required

# Kured 配置
--reboot-sentinel=/var/run/reboot-required
```

### 使用命令检测重启需求

```yaml
# 使用命令而非文件
command:
  - /usr/bin/kured
  - --reboot-sentinel-command=needs-restarting -r 2>/dev/null || echo "reboot needed"
```

---

## Prometheus 集成

### 抑制重启 (Alert-based blocking)

```yaml
# 当存在关键告警时不执行重启
command:
  - /usr/bin/kured
  - --prometheus-url=http://prometheus:9090
  - --alert-filter-regexp=^(Critical|HighSeverity).*$
  - --alert-firing-only=true
```

### 监控指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kured
  namespace: kured
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: kured
  endpoints:
    - port: metrics
      interval: 30s
```

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `kured_reboot_required` | 节点是否需要重启 |
| `kured_drain_blocked_nodes` | 被阻止 drain 的节点数 |

---

## 通知配置

### Slack 通知

```yaml
command:
  - /usr/bin/kured
  - --slack-hook-url=https://hooks.slack.com/services/xxx/yyy/zzz
  - --slack-username=Kured
  - --slack-channel=#kubernetes-ops
  - --message-template-drain=⚠️ Draining node %s for kernel update
  - --message-template-reboot=🔄 Rebooting node %s
  - --message-template-uncordon=✅ Node %s is back online
```

### Teams 通知

```yaml
command:
  - /usr/bin/kured
  - --notify-url=https://outlook.office.com/webhook/xxx
```

---

## 故障排查

### 查看 Kured 日志

```bash
kubectl logs -n kured -l name=kured -f
```

### 检查锁状态

```bash
# 查看哪个节点持有锁
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}: {.metadata.annotations.weave\.works/kured-most-recent-reboot-needed}{"\n"}{end}'

# 查看锁注解
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}: {.metadata.annotations.kured\.weave\.works/lock}{"\n"}{end}'
```

### 手动释放锁

```bash
# 移除锁注解
kubectl annotate node <node-name> kured.weave.works/lock-
```

---

## 最佳实践

1. **时间窗口**: 配置业务低峰期的重启窗口
2. **告警集成**: 使用 Prometheus 告警阻止关键时期重启
3. **通知配置**: 启用 Slack/Teams 通知及时了解状态
4. **Pod 保护**: 使用 PodDisruptionBudget 保护关键应用
5. **锁超时**: 设置合理的 lock-ttl 防止死锁
6. **控制平面**: 谨慎处理控制平面节点的重启

---

## 参考资源

- [官方文档](https://kured.dev/docs/)
- [GitHub Repo](https://github.com/kubereboot/kured)
- [配置参考](https://kured.dev/docs/configuration/)
- [Helm Chart](https://github.com/kubereboot/charts)
- [故障排查](https://kured.dev/docs/operation/)

---

**维护者**: Kudig Team | **许可证**: MIT
