---
title: Pod Hostname
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- statefulset
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Hostname 是什么
- 如何 Pod Hostname
trigger_keywords:
- Pod
- Hostname
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Hostname

## 概述
本页解释 Pod 主机名的设置方式、配置后的潜在副作用以及底层机制。Pod 内部观察到的主机名默认来自 `metadata.name`。

## 核心概念/原理
- **默认主机名**：Pod 创建时，其主机名和完全限定域名（FQDN）均默认为 `metadata.name` 的值。
- **自定义主机名（`spec.hostname`）**：设置该字段后，其值优先于 `metadata.name` 作为 Pod 内部的主机名。
- **子域（`spec.subdomain`）**：
  - 若设置了 `spec.hostname=foo` 和 `spec.subdomain=bar`，则主机名为 `foo`，FQDN 为 `foo.bar.<namespace>.svc.<cluster-domain>`。
  - 同时设置 `hostname` 和 `subdomain` 时，集群 DNS 服务器会为 Pod 创建 A/AAAA 记录。
- **FQDN 作为主机名（`setHostnameAsFQDN`）**：
  - 默认情况下，`hostname` 命令返回短主机名。
  - 设置 `setHostnameAsFQDN: true` 后，[[kubelet|kubelet]] 会将 FQDN 写入 Pod 的 hostname 命名空间，`hostname` 和 `hostname --fqdn` 均返回 FQDN。
  - Linux 内核的 hostname 字段限制为 64 个字符；若 FQDN 超过此长度，Pod 将无法启动（停留在 `ContainerCreating`）。
- **主机名覆盖（`hostnameOverride`）**：
  - Beta 特性（v1.35 默认启用）。
  - 无条件将 Pod 内部的主机名和 FQDN 都设置为 `hostnameOverride` 的值。
  - 长度限制 64 字符，遵循 RFC 1123 DNS 子域名标准。
  - **注意**：`hostnameOverride` 不影响集群 DNS 中的 A/AAAA 记录；若同时设置了 `hostname` 和 `subdomain`，DNS 记录仍基于后者生成。
  - 不能与 `hostNetwork` 和 `setHostnameAsFQDN` 同时设置。

## 关键机制或特性
- 主机名配置仅影响 Pod 内部进程看到的名称。
- DNS 记录的生成取决于 `hostname` + `subdomain`，而非 `hostnameOverride`。
- `hostnameOverride` 适用于需要 Pod 内部进程看到特定主机名，但不想改变 DNS 记录的场景。

## 使用场景
- 应用依赖特定主机名进行许可证验证或集群成员识别。
- 需要为 [[StatefulSet|StatefulSet]] Pod 提供稳定且可预测的网络标识。
- 在 Pod 内部模拟特定的域名环境。

## 最佳实践/注意事项
- 确保 `metadata.name` 或 `spec.hostname` 与 `subdomain` 组合后的 FQDN 不超过 64 字符（若启用 `setHostnameAsFQDN`）。
- 使用 `hostnameOverride` 时，注意其不会修改 DNS 记录，且不能与 `hostNetwork` 同时使用。
- Pod 名称应符合 DNS Label 规则，以获得最佳兼容性。

## 生产 YAML 示例

### 基本主机名配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-server-0
  namespace: production
spec:
  hostname: app-server         # 自定义主机名（覆盖 metadata.name）
  subdomain: app-cluster       # 子域名
  # FQDN = app-server.app-cluster.production.svc.cluster.local
  containers:
  - name: app
    image: registry.example.com/apps/server:v3.0
    command: ["sh", "-c"]
    args:
    - |
      echo "Hostname: $(hostname)"
      echo "FQDN: $(hostname -f)"
      exec /app/start.sh
```

### StatefulSet 的稳定网络标识

```yaml
# StatefulSet 自动为每个 Pod 设置 hostname 和 subdomain
apiVersion: v1
kind: Service
metadata:
  name: cassandra
  namespace: data
spec:
  clusterIP: None              # Headless Service
  selector:
    app: cassandra
  ports:
  - port: 9042
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: cassandra
  namespace: data
spec:
  serviceName: cassandra       # 对应 Headless Service 名称
  replicas: 3
  selector:
    matchLabels:
      app: cassandra
  template:
    metadata:
      labels:
        app: cassandra
    spec:
      containers:
      - name: cassandra
        image: cassandra:4.1
        env:
        - name: CASSANDRA_SEEDS
          # 使用稳定的 DNS 名称作为种子节点
          value: "cassandra-0.cassandra.data.svc.cluster.local,cassandra-1.cassandra.data.svc.cluster.local"
        ports:
        - containerPort: 9042
# Pod DNS 记录：
# cassandra-0.cassandra.data.svc.cluster.local
# cassandra-1.cassandra.data.svc.cluster.local
# cassandra-2.cassandra.data.svc.cluster.local
```

### hostnameOverride 使用场景

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: license-server
  namespace: software
spec:
  hostnameOverride: "license-node-prod"    # Beta，v1.35 默认启用
  # Pod 内部 hostname 命令返回 "license-node-prod"
  # 不影响 DNS 记录（DNS 仍基于 hostname+subdomain）
  # 不能与 hostNetwork 同时使用
  # 不能与 setHostnameAsFQDN 同时使用
  containers:
  - name: app
    image: registry.example.com/vendor/license-app:v2.0
    # 应用通过 hostname 验证许可证绑定
```

### setHostnameAsFQDN 示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: fqdn-app
  namespace: production
spec:
  hostname: myapp
  subdomain: cluster
  setHostnameAsFQDN: true
  # hostname 命令返回：myapp.cluster.production.svc.cluster.local

  containers:
  - name: app
    image: registry.example.com/apps/fqdn-aware:v1.0
```

## 主机名配置决策树

```
需要自定义 Pod 内部主机名？
  │
  ├─ 否 → 使用默认 metadata.name
  │
  └─ 是 → 需要 DNS 记录吗？
        │
        ├─ 是 → 设置 spec.hostname + spec.subdomain
        │       └─ 还需要 hostname 命令返回 FQDN？
        │             ├─ 是 → 设置 setHostnameAsFQDN: true（注意 64 字符限制）
        │             └─ 否 → 不设置
        │
        └─ 否 → 使用 hostnameOverride（不影响 DNS）

```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 卡在 ContainerCreating | `setHostnameAsFQDN: true` 但 FQDN 超过 64 字符 | 计算 FQDN 长度：`hostname.subdomain.namespace.svc.cluster.local` |
| `hostname -f` 返回短名而非 FQDN | 未设置 `setHostnameAsFQDN: true` 或 subdomain 为空 | `kubectl get pod -o yaml` 检查相关字段 |
| DNS 解析 Pod 失败 | 未同时设置 hostname 和 subdomain，或 Headless Service 不存在 | 确认 Headless Service 的 selector 匹配 Pod labels |
| hostnameOverride 与 hostNetwork 冲突 | 两者不能同时使用 | 移除其中一个配置 |
| 应用许可证验证失败 | 主机名与预期不匹配 | `kubectl exec -- hostname` 验证实际主机名 |

## 生产检查清单

- [ ] FQDN 总长度不超过 64 字符（如果使用 `setHostnameAsFQDN`）
- [ ] hostname 和 subdomain 符合 RFC 1123 DNS 标签规范
- [ ] Headless Service 已创建（如果需要 DNS A/AAAA 记录）
- [ ] `hostnameOverride` 不与 `hostNetwork` 或 `setHostnameAsFQDN` 同时使用
- [ ] 依赖稳定主机名的应用优先使用 StatefulSet + Headless Service

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Pod 内部主机名
kubectl exec <pod> -- hostname

# 查看 Pod FQDN
kubectl exec <pod> -- hostname -f

# 检查 DNS 解析（从集群内）
kubectl exec <pod> -- nslookup <hostname>.<subdomain>.<namespace>.svc.cluster.local

# 查看 Pod hostname 相关配置
kubectl get pod <name> -o jsonpath='{.spec.hostname} {.spec.subdomain} {.spec.setHostnameAsFQDN}'

# 计算 FQDN 长度
echo -n "myhost.mysub.mynamespace.svc.cluster.local" | wc -c
```
## 交叉引用

- [[系统基础/topic-dictionary/workloads/statefulsets.md|StatefulSets]]](statefulsets.md) — 自动管理稳定网络标识的首选方案
- [Pods](pods.md) — Pod 基础概念和 metadata.name
- [Downward API](downward-api.md) — 在容器内获取 Pod 元数据的其他方式

## 参考链接
- https://[[entities/kubernetes.md|kubernetes]].io/docs/concepts/workloads/pods/pod-hostname/

## Related

- [[系统基础/topic-dictionary/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[系统基础/topic-dictionary/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[系统基础/topic-dictionary/workloads/autoscaling-workloads.md|Autoscaling Workloads]]

```

<!-- risk-assessed -->
