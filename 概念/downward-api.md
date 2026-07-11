---
title: Downward API
summary: Downward API 允许容器获取自身 Pod 和节点的元数据信息，如标签、注解、资源限制等。
category: concepts
tags:
- pod
- metadata
- api
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Downward API

## 概述

Downward API 是 Kubernetes 提供的一种将 **Pod 自身和容器的元数据**（名称、命名空间、IP、节点名、标签、注解、资源请求/限制等）注入容器的机制——它并不是一个真正的网络 API，而是一种"字段引用"。容器无需调用 kube-apiserver 或 cloud metadata 即可获知自己是谁、跑在哪个节点、被分配了多少资源。这让应用能自描述（写入日志/指标）、做自适应（按 limit 调整 JVM 堆）、与平台协同（按 Pod 名生成唯一标识）。

## 架构与工作原理

```
┌──────────── Pod 元数据来源 ─────────────┐
│   metadata.name / namespace / uid       │
│   spec.nodeName / status.podIP         │
│   spec.serviceAccountName              │
│   metadata.labels / annotations        │
│   containers[].resources.limits/requests │
└───────────────────┬─────────────────────┘
                    │ DownwardAPI（字段引用，kubelet 写入）
        ┌───────────┴───────────┐
        ▼                       ▼
  环境变量 (env)            Volume 文件
  env.valueFrom.            downwardAPI volume:
   fieldRef / resourceFieldRef     items[].path
                                  (动态更新)
```

**两种消费方式**：
- **环境变量（env）**：启动时一次性注入，**不会随运行时变化更新**（如 IP 变化 env 不会变）。适合不变的元数据（name、namespace）。
- **Volume 挂载**：kubelet 定期把字段写为文件（如 `/etc/podinfo/labels`、`/etc/podinfo/annotations`），**会动态更新**（labels/annotations 变化时）。适合会变化的数据。

**可引用的字段**：
- Pod 级（fieldRef）：`metadata.name`、`metadata.namespace`、`metadata.uid`、`metadata.labels['<k>']`、`metadata.annotations['<k>']`、`spec.nodeName`、`spec.serviceAccountName`、`status.podIP`、`status.hostIP`。
- 容器级（resourceFieldRef）：`requests.cpu`、`requests.memory`、`limits.cpu`、`limits.memory`（需指定 containerName）。

## 关键组件与特性

| 方式 | 字段 | 是否动态更新 |
|------|------|--------------|
| env.fieldRef | name/namespace/uid/nodeName 等 | 否（启动注入） |
| env.resourceFieldRef | cpu/memory requests/limits | 否 |
| env.fieldRef | status.podIP | 否（启动时 IP 已定） |
| Volume fieldRef | labels/annotations | **是**（kubelet 定期刷新） |
| Volume resourceFieldRef | limits/requests | 是 |

## 配置示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata: {name: webapp, namespace: production}
spec:
  replicas: 2
  selector: {matchLabels: {app: webapp}}
  template:
    metadata:
      labels: {app: webapp, tier: frontend}
      annotations: {version: v1.2.0}
    spec:
      containers:
      - name: webapp
        image: webapp:v1.2.0
        env:
        - {name: POD_NAME,      valueFrom: {fieldRef: {fieldPath: metadata.name}}}
        - {name: POD_NAMESPACE, valueFrom: {fieldRef: {fieldPath: metadata.namespace}}}
        - {name: POD_IP,        valueFrom: {fieldRef: {fieldPath: status.podIP}}}
        - {name: NODE_NAME,     valueFrom: {fieldRef: {fieldPath: spec.nodeName}}}
        # 容器资源引用
        - name: CPU_LIMIT
          valueFrom:
            resourceFieldRef: {containerName: webapp, resource: limits.cpu}
        - name: MEM_REQUEST
          valueFrom:
            resourceFieldRef: {containerName: webapp, resource: requests.memory, divisor: 1Mi}
        volumeMounts:
        - {name: podinfo, mountPath: /etc/podinfo, readOnly: true}
      volumes:
      - name: podinfo
        downwardAPI:
          defaultMode: 0444
          items:
          - {path: labels,      fieldRef: {fieldPath: metadata.labels}}
          - {path: annotations, fieldRef: {fieldPath: metadata.annotations}}
          - {path: name,        fieldRef: {fieldPath: metadata.name}}
          - {path: cpu_limit,
             resourceFieldRef: {containerName: webapp, resource: limits.cpu}}
```

应用读取 Volume 文件示例（Go）：

```go
// /etc/podinfo/labels 内容为 key="value" 格式，每行一个
data, _ := os.ReadFile("/etc/podinfo/labels")
// fsnotify 监听变化实现热更新
watcher.Add("/etc/podinfo/labels")
```

## 常用操作与命令

```bash
# 验证注入的环境变量
kubectl exec webapp-xxx -- env | grep -E 'POD_|NODE_|CPU_LIMIT'

# 验证 Volume 文件
kubectl exec webapp-xxx -- cat /etc/podinfo/labels
kubectl exec webapp-xxx -- cat /etc/podinfo/annotations

# 动态更新测试：给 Pod 打新标签后查看文件变化（需要 Pod 模板标签才会生效，
# 实际中 labels 来自 Pod spec.metadata.labels，可直接 kubectl label pod）
kubectl label pod webapp-xxx env=canary
kubectl exec webapp-xxx -- grep env /etc/podinfo/labels

# 配合 fsnotify 在容器内监听
kubectl exec webapp-xxx -- sh -c 'ls -la /etc/podinfo/'
```

## 最佳实践

1. **日志加 Pod 标识**：应用把 `POD_NAME` / `NODE_NAME` 写入每条日志，便于在 Loki/ELK 中精确定位。
2. **按 limit 自适应**：JVM 用 `MEM_LIMIT` 设置 `-XX:MaxRAMPercentage`，Cgroup aware JVM（11+）直接读 cgroup 亦可。
3. **变化的数据用 Volume**：labels/annotations 需要热更新时用 Volume + fsnotify，不要用 env。
4. **divisor 控制单位**：CPU 用 `1` 得 nano core，用 `1m` 得 millicore；内存用 `1Mi` 得 MB，避免应用解析错误。
5. **避免暴露敏感注解**：annotations 可能含 IP/版本，注意不要写入公开日志。
6. **Prometheus 标准用法**：`prometheus.io/port` 注解 + DownwardAPI 让 sidecar 自动发现抓取端口。

## 常见陷阱

- **env 不会动态更新**：把 `status.podIP` 放 env，Pod 重建 IP 变化后应用仍用旧值；要么重启容器要么用 Volume。
- **resourceFieldRef 缺 containerName**：多容器 Pod 必须指定 containerName，否则引用错对象。
- **divisor 解析错误**：CPU 默认单位是 core 的小数，应用当整数用导致计算错误。
- **Volume 文件权限**：defaultMode 设 0644 可读，0440 需匹配运行用户。
- **labels 过多**：Pod labels 数量受限（63），大量标签会撑大 Volume 文件。
- **字段名拼错**：`metadata.labels['x']` 而非 `metadata.label.x`，引号和括号格式严格。
- **与 cloud metadata 混淆**：Downward API 是 Pod 视角，节点云元数据（实例 ID/类型）要走 cloud metadata 服务。

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/pods.md|Pod]] — 元数据来源
- [[概念/init-containers.md|Init Containers]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
