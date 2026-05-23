---
title: ConfigMaps
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- prometheus
- helm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ConfigMaps 是什么
- 如何 ConfigMaps
trigger_keywords:
- ConfigMaps
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- prometheus-basics
created: "2026-05-23"
---

# ConfigMaps

## 概述

ConfigMap 是 [[Kubernetes|Kubernetes]] 中用于存储非机密数据的 API 对象，以键值对（key-value）形式保存。Pod 可以将 ConfigMap 用作环境变量、命令行参数，或者作为卷中的配置文件。通过 ConfigMap，你可以将环境相关的配置与容器镜像解耦，使应用更易于移植。

## 核心概念/原理

- **键值对存储**：ConfigMap 使用 `data` 字段存储 UTF-8 字符串，使用 `binaryData` 字段存储 base64 编码的二进制数据。
- **命名规范**：ConfigMap 的名称必须是合法的 DNS 子域名；`data` 和 `binaryData` 中的键名只能包含字母、数字、`-`、`_` 或 `.`，且两个字段中的键不能重复。
- **大小限制**：ConfigMap 的数据总量不能超过 1 MiB，不适合存储大块数据。
- **不可变 ConfigMap**：从 v1.19 开始，可以设置 `immutable: true` 创建不可变 ConfigMap，防止意外修改并降低 API Server 的负载。

## 关键机制或特性

Pod 中使用 ConfigMap 的四种方式：

1. **容器命令和参数**：在 `command` 或 `args` 中引用 ConfigMap 的值。
2. **环境变量**：通过 `env.valueFrom.configMapKeyRef` 或 `envFrom.configMapRef` 将键值注入为环境变量。
3. **只读卷挂载**：将 ConfigMap 挂载为卷中的文件，供应用读取。
4. **[[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api|Kubernetes API]] 读取**：在 Pod 内通过代码直接调用 Kubernetes API 读取 ConfigMap，可订阅变更事件，也能访问其他命名空间的 ConfigMap。

**自动更新机制**：
- 通过卷挂载的 ConfigMap 在更新后会自动同步到 Pod（ eventual consistency，延迟取决于 [[kubelet|kubelet]] 同步周期和缓存策略）。
- 通过环境变量注入的 ConfigMap 不会自动更新，需要重启 Pod 才能生效。
- 使用 `subPath` 挂载的 ConfigMap 不会接收更新。

## 使用场景

- 将开发环境（`localhost`）和生产环境（Kubernetes [[Service|Service]]）的配置分离，例如数据库主机地址。
- 为同一应用在不同命名空间或集群中提供不同的配置，而无需重新构建镜像。
- 存储小型配置文件（如 `.properties`、`.conf`），供应用启动时读取。

## 最佳实践/注意事项

- **不存储机密数据**：ConfigMap 不提供加密或保密能力，敏感信息应使用 Secret 或第三方加密工具管理。
- **控制数据大小**：超过 1 MiB 的数据应使用持久卷、对象存储或数据库。
- **静态 Pod 限制**：静态 Pod（Static Pod）的 spec 不能引用 ConfigMap 或其他 API 对象。
- **使用不可变 ConfigMap**：对于大规模使用的 ConfigMap，建议标记为 `immutable`，以避免意外更新导致的应用中断，并提升集群性能。
- **键名合法性**：确保键名符合环境变量命名规则，否则部分键可能无法注入为环境变量。

## 生产 YAML 示例

### 多格式 ConfigMap（键值 + 配置文件）

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: production
  labels:
    app: order-service
    environment: production
data:
  # 简单键值对 — 用于环境变量
  LOG_LEVEL: "info"
  DB_HOST: "postgres-primary.database.svc.cluster.local"
  DB_PORT: "5432"
  CACHE_TTL: "300"
  # 配置文件 — 用于卷挂载
  application.yaml: |
    server:
      port: 8080
      shutdown: graceful
    spring:
      datasource:
        hikari:
          maximum-pool-size: 20
          minimum-idle: 5
          connection-timeout: 30000
    management:
      endpoints:
        web:
          exposure:
            include: health,info,prometheus
  nginx.conf: |
    worker_processes auto;
    events { worker_connections 1024; }
    http {
      upstream backend {
        server 127.0.0.1:8080;
      }
      server {
        listen 80;
        location / { proxy_pass http://backend; }
        location /healthz { return 200 'ok'; }
      }
    }
immutable: false                           # 生产环境建议设为 true（稳定后）
```

### Pod 引用 ConfigMap（环境变量 + 卷挂载）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  template:
    metadata:
      labels:
        app: order-service
      annotations:
        checksum/config: "{{ sha256sum .Values.configmap }}"  # Helm：ConfigMap 变更时触发滚动更新
    spec:
      containers:
        - name: app
          image: registry.example.com/order:v3.0
          envFrom:
            - configMapRef:
                name: app-config            # 注入所有键值对为环境变量
          env:
            - name: SPECIFIC_KEY
              valueFrom:
                configMapKeyRef:
                  name: app-config
                  key: DB_HOST
          volumeMounts:
            - name: config-volume
              mountPath: /etc/config
              readOnly: true
          resources:
            requests:
              cpu: "500m"
              memory: 512Mi
      volumes:
        - name: config-volume
          configMap:
            name: app-config
            items:                          # 只挂载配置文件类型的 key
              - key: application.yaml
                path: application.yaml
              - key: nginx.conf
                path: nginx.conf
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 启动失败，提示 ConfigMap 不存在 | ConfigMap 未创建或名称拼写错误 | `kubectl get configmap -n <ns>` 确认 ConfigMap 存在 |
| 环境变量值未更新 | 环境变量注入不会自动更新 | 需要重启 Pod 或使用卷挂载方式 |
| 卷挂载文件内容过期 | kubelet 同步周期（默认 60s）+ configMap 缓存 TTL | 等待同步周期完成；或设置 `kubelet --sync-frequency` |
| subPath 挂载后内容不更新 | subPath 挂载不接收自动更新 | 改为挂载整个目录或重启 Pod |
| 键名含特殊字符导致环境变量注入失败 | 键名包含 `.` 或 `-` 等不合法字符 | 使用 `envFrom` 时检查键名是否符合环境变量命名规则 |

## 生产检查清单

- [ ] 敏感数据使用 Secret 而非 ConfigMap
- [ ] 大规模使用的 ConfigMap 设置 `immutable: true`
- [ ] ConfigMap 数据总量不超过 1 MiB
- [ ] 使用 Helm checksum annotation 或 Reloader 确保 ConfigMap 变更触发 Pod 滚动更新
- [ ] 避免使用 `subPath` 挂载（无法自动更新）
- [ ] 为 ConfigMap 添加版本标签（如 `config-version: v2`）方便回滚
- [ ] 确认 Static Pod 不引用 ConfigMap（不支持）

## 命令快速参考

```bash
# 从文件创建 ConfigMap
kubectl create configmap app-config --from-file=application.yaml --from-file=nginx.conf -n production

# 从字面量创建
kubectl create configmap app-config --from-literal=LOG_LEVEL=info --from-literal=DB_HOST=postgres -n production

# 查看 ConfigMap 内容
kubectl get configmap app-config -n production -o yaml

# 编辑 ConfigMap
kubectl edit configmap app-config -n production

# 查看 Pod 中挂载的配置文件
kubectl exec -n production <pod-name> -- cat /etc/config/application.yaml

# 查看 ConfigMap 在哪些 Pod 中被引用
kubectl get pods -n production -o json | jq '.items[] | select(.spec.volumes[]?.configMap.name == "app-config") | .metadata.name'
```

## 交叉引用

- [Secrets](./secrets.md) — 机密数据存储，与 ConfigMap 互补
- [存活、就绪和启动探针](./liveness-readiness-and-startup-probes.md) — 探针可检测配置加载是否就绪
- [Pod 和容器的资源管理](./resource-management-for-pods-and-containers.md) — 资源配置与应用配置分离

## 参考链接

- [Kubernetes 官方文档 - ConfigMaps](https://kubernetes.io/docs/concepts/configuration/configmap/)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
