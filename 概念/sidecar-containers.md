---
title: Sidecar Containers
summary: Sidecar Containers：Sidecar 模式是一种常见的 Kubernetes 设计模式，将辅助功能容器与主应用容器部署在同一 Pod
  中。
category: concepts
tags:
- sidecar
- pod
- patterns
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Sidecar Containers

## 概述

Sidecar（边车）是一种将**辅助功能容器**与**主应用容器**部署在同一 Pod 中的设计模式。两者共享网络命名空间（localhost 互通）、存储卷和生命周期，协同提供服务。Sidecar 把横切关注点（cross-cutting concerns）——日志采集、网络代理、配置同步、密钥轮换、健康检查、指标暴露——从主应用中解耦，让业务镜像保持单一职责。Service Mesh（Istio/Linkerd）的 Envoy 注入是最广为人知的 sidecar 应用。

## 架构与工作原理

```
┌──────────────────── Pod（共享 netns / volumes）────────────────────┐
│                                                                     │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                │
│  │ 主应用容器  │   │ Sidecar A   │   │ Sidecar B   │                │
│  │ app: webapp │   │ log-shipper │   │ envoy proxy │                │
│  │ :8080       │   │ (Fluent Bit)│   │ (Mesh 注入) │                │
│  └──────┬──────┘   └──────┬──────┘   └──────┬──────┘                │
│         │ localhost        │ 读 log 卷      │ 透明拦截流量           │
│         │                  │                │                        │
│         └────── 共享 Volume (emptyDir) ─────┴────────────────────────┘ │
│                                                                     │
└──────────────────────────────────────────────────────────────────────┘
```

**关键特性**：
- **共享网络命名空间**：sidecar 与主应用通过 `127.0.0.1:<port>` 互通，无需 Service。
- **共享存储卷**：主应用写日志到 emptyDir，sidecar（Fluent Bit）读并转发。
- **协同生命周期**：默认 sidecar 与主应用并行启动、随 Pod 一起销毁。
- **Mesh 流量拦截**：Istio 通过 iptables redirect 把进出流量重定向到 Envoy sidecar，实现 mTLS/熔断/可观测。

**经典 sidecar 用例**：
1. **日志采集**：主应用写文件，sidecar（Fluent Bit / Filebeat）读转发到 Loki/ES。
2. **Service Mesh**：Istio/Linkerd 注入 Envoy/proxy，提供 mTLS、重试、熔断、链路追踪。
3. **配置/密钥同步**：sidecar 定期从 Vault/ConfigMap 拉取并写到共享卷，主应用 fsnotify 热加载。
4. **本地代理/适配器**：把 gRPC 转 HTTP、老协议适配、数据库连接池代理。
5. **健康/指标暴露**：主应用不暴露 metrics，由 sidecar 抓 JMX/Prometheus 格式化后暴露。

## 关键组件与特性

| 元素 | 说明 |
|------|------|
| 多容器 Pod | 同一 Pod spec.containers[] 多个 |
| 共享 netns | localhost 直连，无需 Service |
| 共享 volumes | emptyDir / configMap 桥接数据 |
| 启动顺序（1.28 前） | 并行，无法保证 sidecar 先就绪 |
| Sidecar init（1.28+ beta，1.29 GA） | `restartPolicy: Always` 的 init 容器，先于应用启动 |
| 资源开销 | 每个 sidecar 增加 CPU/内存，需计入 Pod 总资源 |

## 配置示例

```yaml
---
# 1. 经典：主应用 + 日志 sidecar
apiVersion: apps/v1
kind: Deployment
metadata: {name: webapp, namespace: production}
spec:
  replicas: 3
  selector: {matchLabels: {app: webapp}}
  template:
    metadata: {labels: {app: webapp}}
    spec:
      containers:
      - name: webapp
        image: webapp:v1.2.0
        volumeMounts:
        - {name: logs, mountPath: /var/log/app}
      - name: log-shipper                # sidecar
        image: fluent-bit:2.2
        resources:
          requests: {cpu: 50m, memory: 64Mi}
          limits: {cpu: 200m, memory: 128Mi}
        volumeMounts:
        - {name: logs, mountPath: /var/log/app, readOnly: true}
      volumes:
      - name: logs
        emptyDir: {}
---
# 2. 1.29+ 正确的 Sidecar：用 restartPolicy: Always 的 init
spec:
  initContainers:
  - name: envoy
    image: envoyproxy/envoy:v1.29
    restartPolicy: Always        # ← 关键：常驻且先于应用启动就绪
    # 应用容器直到 envoy 进入 Ready 才启动
    volumeMounts:
    - {name: envoy-cfg, mountPath: /etc/envoy}
  containers:
  - {name: webapp, image: webapp:v1.2.0}
```

## 常用操作与命令

```bash
# 查看 Pod 内多容器
kubectl get pod webapp-xxx -o jsonpath='{range .spec.containers[*]}{.name}{"\t"}{end}'
kubectl describe pod webapp-xxx | grep -A30 Containers

# 分别查看日志
kubectl logs webapp-xxx -c webapp
kubectl logs webapp-xxx -c log-shipper
kubectl logs webapp-xxx --all-containers --tail=100

# 进入特定容器
kubectl exec -it webapp-xxx -c envoy -- /bin/bash

# Istio 自动注入的 sidecar 观察
kubectl get pod webapp-xxx -o yaml | grep -A5 'name: istio-proxy'

# 资源占用（每个 sidecar 都有成本）
kubectl top pod webapp-xxx --containers

# 1.29+ sidecar init 优雅停止验证（应用退出后 sidecar 自动退出）
kubectl delete pod webapp-xxx && kubectl get pod webapp-xxx -w
```

## 最佳实践

1. **sidecar 资源要设 limit**：每个 Pod 多个 sidecar 累加 CPU/内存可观（Istio Envoy 默认 ~100m CPU / 128Mi），不可忽略。
2. **1.29+ 用 sidecar init**：解决启动顺序（sidecar 先就绪）与优雅停止（sidecar 跟随应用退出），避免老模式各种坑。
3. **sidecar 与主应用镜像解耦**：sidecar 镜像独立升级，不绑死主应用发版节奏。
4. **日志 sidecar 用文件而非 stdout**：主应用写文件到共享卷，sidecar 采集转发，避免 stdout 多行 JSON 被截断。
5. **Mesh sidecar 用自动注入**：Istio/Linkerd 通过 namespace label 自动注入，避免手写。
6. **共享卷用 emptyDir.sizeLimit**：限制日志卷大小，避免打满节点磁盘。
7. **减少 sidecar 数量**：每个 sidecar 都是复杂度和资源成本，能用 init 解决的别用常驻 sidecar。

## 常见陷阱

- **启动顺序竞态**：1.28 前 sidecar 与应用并行启动，应用先启动时 sidecar（envoy）未就绪导致首次请求失败；升级到 1.29+ 用 sidecar init。
- **优雅停止问题**：旧模式下主应用退出后 sidecar 仍存活，Pod 卡在 Terminating；Pod 内最后一个容器退出前 sidecar 不会退。
- **资源放大**：未给 sidecar 设 limit，OOM 或抢 CPU 影响主应用。
- **共享卷写冲突**：多个容器同时写同一 emptyDir 数据损坏，约定单写入者。
- **端口冲突**：sidecar（envoy 15000/15001）与主应用端口冲突，需规划。
- **sidecar 升级独立性问题**：sidecar 镜像变更会触发整个 Pod 滚动，影响主应用发布节奏。
- **Mesh 性能开销**：每个 hop 多一层 Envoy，P99 延迟可能增加，关键路径评估 eBPF 模式（Cilium Service Mesh）。
- **日志卷打满节点**：emptyDir 默认无限制，应用疯狂写日志时占满节点 ephemeral storage。

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/pods.md|Pod]] — sidecar 的宿主
- [[概念/init-containers.md|Init Containers]]
- [[概念/ephemeral-containers.md|Ephemeral Containers]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
