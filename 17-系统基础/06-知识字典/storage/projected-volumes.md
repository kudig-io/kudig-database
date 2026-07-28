---
title: Projected Volumes（投射卷）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Projected Volumes（投射卷） 是什么
- 如何 Projected Volumes（投射卷）
trigger_keywords:
- Projected
- Volumes
- 投射卷
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Projected Volumes（投射卷）

## 概述

Projected Volume 是一种将多个现有的卷源（如 Secret、ConfigMap、downwardAPI、serviceAccountToken 等）映射到同一个目录中的卷类型。它提供了一种“一体化”的方式，将不同来源的数据集中投射到容器的文件系统中。

## 核心概念/原理

- **统一目录**：Projected Volume 将多个独立的卷源合并挂载到容器内的同一个路径下。
- **源类型**：所有源必须与 Pod 位于同一命名空间。
- **只读属性**：Projected Volume 中的内容默认是只读的。

## 关键机制或特性

### 支持的卷源类型

| 类型 | 说明 |
|------|------|
| `secret` | 将 Secret 的键值对作为文件投射到目录中。 |
| `configMap` | 将 ConfigMap 的键值对作为文件投射到目录中。 |
| `downwardAPI` | 将 Pod 的元数据或资源信息以文件形式投射。 |
| `serviceAccountToken` | 将当前 ServiceAccount 的 Token 注入到指定路径，用于访问 [[23-实体/kubernetes.md|[[kubernetes\|kubernetes]]]] API。 |
| `clusterTrustBundle` | 将 ClusterTrustBundle 对象的内容作为自动更新的 PEM 文件注入（v1.33 beta）。 |
| `podCertificate` | 为 Pod 安全地提供私钥和 X.509 证书链，并自动轮换（v1.35 beta）。 |

### 权限与模式

- 可在 `projected` 级别设置 `defaultMode`。
- 也可为每个单独的投影项设置 `mode`，实现对特定文件的权限控制。

### ServiceAccountToken 投射

- 可配置 `audience`（受众）、`expirationSeconds`（过期时间，最小 600 秒）和 `path`（相对挂载路径）。
- 当 Pod 设置了统一的 `runAsUser` 时，[[kubelet|kubelet]] 会将 token 文件权限设为 `0600`，确保只有指定用户可读取。

### 安全上下文交互

- **Linux**：如果 Pod 设置了 `RunAsUser`，Projected 文件的所有权会设置为对应的容器用户。
- **Windows**：由于 SAM 数据库隔离，文件所有权无法强制设置为容器用户，默认由 `BUILTIN\Administrators` 等管理。

## 使用场景

- **统一凭证与配置目录**：将 API Token、CA 证书和应用配置集中投射到一个目录，方便应用统一读取。
- **安全注入 ServiceAccount Token**：避免将 Token 直接嵌入镜像，通过投射卷动态注入并自动管理过期时间。
- **Pod 身份认证**：为工作负载提供访问 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 或其他服务所需的证书和信任链。

## 最佳实践/注意事项

- 使用 `subPath` 挂载 Projected Volume 的容器不会自动接收到卷源内容的更新。
- 在 Windows Pod 中，不建议使用 Linux 的 `RunAsUser` 选项，否则可能导致 Pod 卡在 `ContainerCreating` 状态。
- 对于 `podCertificate` 投影，推荐优先使用 `credentialBundlePath`（合并的 PEM 文件），而不是分离的 `keyPath` 和 `certificateChainPath`，以避免证书轮换时出现密钥与证书不匹配的问题。
- `clusterTrustBundle` 和 `podCertificate` 功能需要开启对应的特性门和 runtime-config。

## 生产 YAML 示例

### 多源投射卷（ServiceAccountToken + ConfigMap + Secret）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: api-client
  namespace: production
spec:
  serviceAccountName: api-client-sa
  containers:
    - name: client
      image: registry.example.com/api-client:v2.0
      volumeMounts:
        - name: credentials
          mountPath: /var/run/secrets/app
          readOnly: true
      resources:
        requests:
          cpu: "100m"
          memory: 128Mi
  volumes:
    - name: credentials
      projected:
        defaultMode: 0400
        sources:
          - serviceAccountToken:
              audience: "https://api.example.com"
              expirationSeconds: 3600        # 1 小时过期，自动轮转
              path: token
          - configMap:
              name: api-client-config
              items:
                - key: endpoints.yaml
                  path: endpoints.yaml
          - secret:
              name: api-client-tls
              items:
                - key: tls.crt
                  path: tls.crt
                - key: tls.key
                  path: tls.key
                  mode: 0400                 # 私钥严格权限
          - downwardAPI:
              items:
                - path: namespace
                  fieldRef:
                    fieldPath: metadata.namespace
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 投射卷中某个文件缺失 | 源 ConfigMap/Secret 不存在 | `kubectl get configmap/secret` 确认存在；检查 items 中的 key |
| ServiceAccountToken 过期后应用报错 | 应用未重新读取 token 文件 | 确保应用定期重新读取 token 文件（kubelet 会自动轮转） |
| Windows Pod 卡在 ContainerCreating | 使用了 Linux 的 RunAsUser | 移除 Pod SecurityContext 中的 runAsUser |
| 文件权限不正确 | defaultMode 或 mode 设置不当 | 检查 `defaultMode` 和各 items 的 `mode` |

## 生产检查清单

- [ ] ServiceAccountToken 设置合理的 `expirationSeconds`（建议 3600）
- [ ] 私钥文件 mode 设置为 0400（仅所有者可读）
- [ ] 不使用 subPath 挂载（无法自动更新）
- [ ] 应用实现 token 文件的热重载逻辑
- [ ] Windows Pod 不使用 runAsUser

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看投射卷中的文件
kubectl exec <pod-name> -- ls -la /var/run/secrets/app/

# 检查 token 内容
kubectl exec <pod-name> -- cat /var/run/secrets/app/token

# 查看 Pod 的 projected volume 配置
kubectl get pod <pod-name> -o jsonpath='{.spec.volumes[?(@.projected)]}' | jq .
```
## 交叉引用

- [卷](./volumes.md) — 卷类型总览
- [临时卷](./ephemeral-volumes.md) — 其他临时存储方式

## 参考链接

- https://kubernetes.io/docs/concepts/storage/projected-volumes/

## Related

- [[17-系统基础/06-知识字典/storage/ceph.md|Ceph]]
- [[17-系统基础/06-知识字典/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[17-系统基础/06-知识字典/storage/composefs.md|ComposeFS 只读文件系统]]


<!-- risk-assessed -->
