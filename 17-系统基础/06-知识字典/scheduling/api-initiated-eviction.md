---
title: API-initiated Eviction
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- pdb
- daemonset
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- API-initiated Eviction 是什么
- 如何 API-initiated Eviction
trigger_keywords:
- API-initiated
- Eviction
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# API-initiated Eviction

## 概述

API 发起驱逐（API-initiated Eviction）是通过 Eviction API 创建 `Eviction` 对象来触发 Pod 优雅终止的过程。可以直接调用 Eviction API，也可以通过 `kubectl drain` 等工具间接调用。

## 核心概念/原理

使用 API 为 Pod 创建 Eviction 对象类似于对 Pod 执行受策略控制的 `DELETE` 操作。API 发起的驱逐尊重配置的 `PodDisruptionBudgets` 和 `terminationGracePeriodSeconds`。

### 调用方式

可以通过 [[kubernetes|Kubernetes]] 语言客户端访问 API 并创建 `Eviction` 对象，POST 操作示例如下：

```json
{
  "apiVersion": "policy/v1",
  "kind": "Eviction",
  "metadata": {
    "name": "quux",
    "namespace": "default"
  }
}
```

或者使用 curl：
```bash
curl -v -H 'Content-type: application/json' https://your-cluster-api-endpoint.example/api/v1/namespaces/default/pods/quux/eviction -d @eviction.json

```

## 关键机制或特性

### API 服务器响应

API 服务器执行准入检查后可能返回以下响应：

- **200 OK**：驱逐被允许，创建 Eviction 子资源，Pod 被删除（类似于发送 DELETE 请求到 Pod URL）。
- **429 Too Many Requests**：由于配置的 PodDisruptionBudget 限制，当前不允许驱逐。可以稍后重试。也可能因为 API 速率限制而返回此响应。
- **500 Internal Server Error**：由于配置错误（如多个 PodDisruptionBudget 引用同一个 Pod），驱逐不被允许。

### 驱逐流程

如果 API 服务器允许驱逐：

1. API 服务器更新 Pod 资源，添加 deletion timestamp，Pod 被视为已终止，并标记配置的宽限期。
2. Pod 所在节点的 [[kubelet|kubelet]] 注意到 Pod 被标记为终止，开始优雅关闭本地 Pod。
3. 在 kubelet 关闭 Pod 期间，控制平面将 Pod 从 EndpointSlice 对象中移除，控制器不再将该 Pod 视为有效对象。
4. Pod 的宽限期到期后，kubelet 强制终止本地 Pod。
5. kubelet 通知 API 服务器移除 Pod 资源。
6. API 服务器删除 Pod 资源。

## 使用场景

- 节点维护前通过 `kubectl drain` 安全地驱逐节点上的所有 Pod。
- 自动化运维工具需要以受控方式移除 Pod，同时尊重 PodDisruptionBudget。
- 应用发布或缩容时，通过 API 驱逐实现优雅下线。

## 最佳实践/注意事项

- 如果应用进入问题状态（如 [[replicaset|ReplicaSet]] 创建的新 Pod 无法进入 Ready 状态），Eviction API 可能持续返回 429 或 500，直到人工干预。
- 遇到卡住的驱逐时，可以尝试：
  - 中止或暂停导致问题的自动化操作，调查卡住的应用后再恢复。
  - 等待一段时间后，直接从集群控制平面删除 Pod（不使用 Eviction API）。
- API 发起的驱逐会尊重 PodDisruptionBudget，而节点压力驱逐不会。

## 生产 YAML 示例

### PodDisruptionBudget 配合 API 驱逐

```yaml
# 1. PodDisruptionBudget — 确保至少 2 个副本可用
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-gateway-pdb
  namespace: production
spec:
  minAvailable: 2                          # 驱逐时至少保持 2 个 Pod 运行
  selector:
    matchLabels:
      app: api-gateway
---
# 2. Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-gateway
  template:
    metadata:
      labels:
        app: api-gateway
    spec:
      terminationGracePeriodSeconds: 60    # 给予充足的优雅终止时间
      containers:
        - name: gateway
          image: registry.example.com/gateway:v5.1
          ports:
            - containerPort: 8080
          lifecycle:
            preStop:
              exec:
                command: ["/bin/sh", "-c", "sleep 10"]  # 等待流量排空
          readinessProbe:
            httpGet:
              path: /healthz
              port: 8080
            periodSeconds: 5
          resources:
            requests:
              cpu: "500m"
              memory: 512Mi
```

### 使用 curl 发起 API 驱逐

```bash
# 创建 Eviction 对象
cat <<EOF > eviction.json
{
  "apiVersion": "policy/v1",
  "kind": "Eviction",
  "metadata": {
    "name": "api-gateway-xyz12",
    "namespace": "production"
  }
}
EOF

curl -v -H 'Content-type: application/json' \
  -X POST \
  "https://<api-server>/api/v1/namespaces/production/pods/api-gateway-xyz12/eviction" \
  -d @eviction.json \
  --cacert /etc/kubernetes/pki/ca.crt \
  --cert /etc/kubernetes/pki/admin.crt \
  --key /etc/kubernetes/pki/admin.key

```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| `kubectl drain` 卡住不动 | PDB minAvailable 过高，无法满足 | `kubectl get pdb -n <ns>` 检查 disruptionsAllowed 是否为 0 |
| API 返回 429 Too Many Requests | PDB 限制当前不允许驱逐 | 等待副本恢复健康后重试；检查 Pod 是否能正常 Ready |
| API 返回 500 Internal Server Error | 多个 PDB 引用同一 Pod | `kubectl get pdb --all-namespaces -o wide` 检查 selector 重叠 |
| Pod 驱逐后新 Pod 无法启动 | 新 Pod 未通过 readiness probe | 检查新 Pod 日志和健康检查配置 |
| drain 时 [[daemonset\|DaemonSet]] Pod 阻塞 | 未使用 `--ignore-daemonsets` | 添加 `--ignore-daemonsets` 参数 |

## 生产检查清单

- [ ] 为所有有状态服务和关键 Deployment 配置 PodDisruptionBudget
- [ ] PDB `minAvailable` 或 `maxUnavailable` 设置合理（不超过 replicas - 1）
- [ ] Pod 配置 `terminationGracePeriodSeconds`（建议 30-120s）
- [ ] Pod 实现优雅关闭（preStop hook + readiness probe）
- [ ] `kubectl drain` 命令使用 `--timeout` 避免无限等待
- [ ] 自动化维护工具处理 429 响应时实现退避重试
- [ ] 验证 PDB selector 不与其他 PDB 重叠

## 命令快速参考

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 安全排空节点（维护前）
kubectl drain <node-name> \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --timeout=300s \
  --grace-period=60

# 查看 PDB 状态
kubectl get pdb -n production

# 查看 PDB 详情（含 disruptionsAllowed）
kubectl describe pdb api-gateway-pdb -n production

# 取消节点维护
kubectl uncordon <node-name>

# 查看驱逐相关事件
kubectl get events --field-selector reason=Evicted --all-namespaces

# 强制删除卡住的 Pod（最后手段）
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
```
## 交叉引用

- [节点压力驱逐](./node-pressure-eviction.md) — kubelet 驱逐不尊重 PDB，与 API 驱逐行为不同
- [Pod 优先级与抢占](./pod-priority-and-preemption.md) — 抢占与 API 驱逐的区别
- [污点与容忍度](./taints-and-tolerations.md) — 基于污点的驱逐（NoExecute）
- [Karpenter 自动扩缩容](./karpenter-autoscaling.md) — Karpenter 整合时使用 API 驱逐并尊重 PDB

## 参考链接

- [Kubernetes 官方文档 - API-initiated Eviction](https://kubernetes.io/docs/concepts/scheduling-eviction/api-eviction/)

## Related

- [[21-生态参考/03-领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]

```

<!-- risk-assessed -->
