---
title: ConfigMap/Secret 更新后应用未生效
description: 专有云 ACK 集群客户修改 ConfigMap 与 Secret 后，业务 Pod 仍使用旧配置的工单闭环样本。
summary: 专有云 ACK 集群客户修改 ConfigMap 与 Secret 后，业务 Pod 仍使用旧配置的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- configmap
- secret
- configuration
- p2
- application-behavior
tier: supporting
created: '2026-06-26T13:30:00+08:00'
updated: '2026-06-26T15:10:00+08:00'
incident_id: INC-2026-ACK-013
priority: P2
severity: medium
affected_cluster: ack-zyy-prod-04
affected_namespace: scm-platform
ticket_type: 配置变更异常
skill_ref:
- ConfigMap 与 Secret 使用指南
- 配置变更 SOP
fta_ref:
- 'FTA: 配置更新未生效'
last_updated: 2026-06-26 15:10:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- ConfigMap/Secret 更新后应用未生效 如何处理
trigger_keywords:
- ConfigMap/Secret
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[domain-17-system-foundation/topic-dictionary/configuration/configmap.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[domain-17-system-foundation/topic-dictionary/security/secret.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-008-coredns-vpc-dns-forward.md]]'
  type: related_to
---



# 工单描述

客户在 `ack-zyy-prod-04` 集群修改了 `scm-platform` 命名空间下的数据库连接串 ConfigMap 与 TLS 证书 Secret，但业务 Pod 重启后仍然使用旧的数据库地址和旧证书。客户描述如下：

> “我们在 ACK 控制台改了 scm-platform 命名空间里的 app-config 这个 ConfigMap，把数据库地址从旧实例切到了新实例，也更新了 tls-cert 这个 Secret。然后 kubectl rollout restart 了 order-sync 这个 Deployment，但是进去 Pod 里看环境变量还是旧的，证书也没变。已经重启两遍了，是不是缓存没清？”

影响范围为 `scm-platform/order-sync` Deployment，共 6 个副本，负责供应链订单同步。

## 分类与优先级判定

- **工单类型**：配置变更异常。
- **优先级**：P2。
- **严重级别**：medium。

判定依据：
1. 配置变更未生效影响数据同步目标，但当前服务仍在运行，未完全中断。
2. 问题集中在配置注入与 Pod 重启机制，排查范围明确。
3. 需在 30 分钟内定位原因并指导客户完成正确变更。

## 诊断步骤

按“先核对配置对象、再检查挂载方式、最后验证 Pod 内文件”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 确认 ConfigMap 与 Secret 当前值
kubectl get configmap app-config -n scm-platform -o yaml
kubectl get secret tls-cert -n scm-platform -o jsonpath='{.data.tls\.crt}' | base64 -d

# 2. 确认 Deployment 中引用的 ConfigMap/Secret 名称与 key
kubectl get deployment order-sync -n scm-platform -o yaml | grep -A 30 envFrom
kubectl get deployment order-sync -n scm-platform -o yaml | grep -A 30 volumes

# 3. 查看 Pod 实际环境变量与挂载文件
kubectl exec -n scm-platform deploy/order-sync -- env | grep DB_HOST
kubectl exec -n scm-platform deploy/order-sync -- cat /etc/ssl/tls.crt | head -5

# 4. 检查 Pod 创建时间，确认是否为新 Pod
kubectl get pod -n scm-platform -l app=order-sync -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.creationTimestamp}{"\n"}{end}'

# 5. 检查 ReplicaSet 历史，确认 rollout 是否真正发生
kubectl get rs -n scm-platform -l app=order-sync
kubectl rollout history deployment/order-sync -n scm-platform

# 6. 检查是否有 sidecar 或 init 容器缓存配置
kubectl get deployment order-sync -n scm-platform -o jsonpath='{.spec.template.spec.initContainers[*].name}'
kubectl get deployment order-sync -n scm-platform -o jsonpath='{.spec.template.spec.containers[*].name}'
```

## 根因分析

`order-sync` Deployment 将 `app-config` 以 `envFrom` 方式注入为环境变量，将 `tls-cert` 以 Volume 方式挂载到 `/etc/ssl`。客户通过 ACK 控制台修改 ConfigMap 和 Secret 后，仅执行了 `kubectl rollout restart deployment/order-sync`，但新 Pod 创建后仍然读取到旧值。进一步检查发现：

1. 客户实际修改的是另一个同名 ConfigMap，但位于 `default` 命名空间，而非 `scm-platform`；
2. `tls-cert` Secret 虽然修改成功，但 Deployment 中使用了 `subPath` 挂载证书文件，而 `subPath` 卷在容器启动时会被 kubelet 缓存为本地文件，更新 Secret 后不会自动刷新；
3. 由于 Deployment 的 `envFrom` 引用的是同一命名空间下的 ConfigMap，新 Pod 应该能读到新值，但因客户改错了命名空间，导致 `scm-platform/app-config` 实际未被更新。

根本原因：跨命名空间误操作 + `subPath` 挂载的文件不会随 Secret 更新自动刷新。

## 修复命令

**第一步：在正确命名空间下重新创建或更新 ConfigMap**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -n scm-platform -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: scm-platform
data:
  DB_HOST: "new-db.scm-platform.svc.cluster.local"
  DB_PORT: "5432"
EOF
```

**第二步：更新 Secret（证书内容已存在，确认命名空间正确）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl get secret tls-cert -n scm-platform -o yaml
# 若证书确实已更新但 subPath 未刷新，需重新创建 Secret 并触发 Pod 重建
kubectl create secret tls tls-cert-new \
  --cert=/path/to/new.crt \
  --key=/path/to/new.key \
  -n scm-platform --dry-run=client -o yaml | kubectl apply -f -
```

**第三步：修改 Deployment，将 subPath 挂载改为常规卷挂载，或变更卷名触发重建**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch deployment order-sync -n scm-platform --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/volumes/0/secret/secretName", "value": "tls-cert-new"}
]'
```

**第四步：触发新的滚动更新，确保 Pod 重建而非原地重启**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment/order-sync -n scm-platform
kubectl rollout status deployment/order-sync -n scm-platform --timeout=300s
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 确认 ConfigMap 与 Secret 值正确
kubectl get configmap app-config -n scm-platform -o jsonpath='{.data.DB_HOST}'
kubectl get secret tls-cert-new -n scm-platform -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -subject -dates

# 2. 确认 Pod 为新创建
kubectl get pod -n scm-platform -l app=order-sync -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.creationTimestamp}{"\n"}{end}'

# 3. 验证 Pod 内配置已更新
kubectl exec -n scm-platform deploy/order-sync -- env | grep DB_HOST
kubectl exec -n scm-platform deploy/order-sync -- cat /etc/ssl/tls.crt | openssl x509 -noout -subject -dates

# 4. 业务功能验证
kubectl logs -n scm-platform -l app=order-sync --tail=100 | grep -iE "connected|database|sync"
```

## 回复客户话术

> 您好，经排查，配置更新未生效存在两个原因：
>
> 1. **ConfigMap 修改到了错误的命名空间**：您修改的是 `default` 命名空间下的 `app-config`，而 `order-sync` 实际引用的是 `scm-platform` 命名空间下的同名 ConfigMap；
> 2. **Secret 使用了 subPath 挂载**：`subPath` 挂载的文件在容器启动时被 kubelet 缓存到本地，更新 Secret 后不会自动刷新，必须重建 Pod 才能生效。
>
> 我们已完成以下处置：
> - 在 `scm-platform` 命名空间下重新更新了 `app-config`；
> - 创建了新 Secret `tls-cert-new` 并更新 Deployment 引用；
> - 滚动重启 `order-sync`，所有 Pod 已重建并读取到新配置。
>
> 建议后续：
> - 通过 `kubectl -n <namespace>` 确认操作目标命名空间，避免跨 namespace 误改；
> - 如需 Secret 更新后自动刷新，避免使用 `subPath`，可采用常规卷挂载或配置重载 sidecar；
> - 将配置变更纳入 GitOps 流程，使用 Kustomize/Helm 统一管理，减少手动控制台操作；
> - 在变更窗口中增加配置生效验证步骤，确保重建后的 Pod 读取到最新值。
>
> 当前订单同步服务已连接到新数据库并使用新证书，请继续观察。

## 复盘与沉淀

ConfigMap 与 Secret 更新未生效是 Kubernetes 运维中的高频问题，常见原因包括：修改了错误的命名空间、使用 `subPath` 挂载、应用自身缓存配置未监听文件变化、Pod 未真正重建等。本次案例同时命中了命名空间误操作与 `subPath` 刷新问题，具有典型教学价值。

需要向客户强调以下几点：
1. ConfigMap 和 Secret 是命名空间作用域的资源，修改时务必带 `-n` 参数；
2. `envFrom` 注入的环境变量只在 Pod 创建时读取 ConfigMap，更新 ConfigMap 不会自动更新已运行 Pod 的环境变量；
3. `subPath` 挂载会触发 kubelet 的本地缓存机制，Secret/ConfigMap 更新后必须重建 Pod；
4. 对于需要热加载的配置，建议应用监听文件变化或使用配置中心（如 Nacos、Apollo）；
5. 在 GitOps 流程中应对 ConfigMap 与 Secret 进行版本控制，避免多人同时修改造成配置覆盖或遗漏。

建议在 SOP 中增加配置变更检查清单：
- [ ] 确认目标命名空间；
- [ ] 确认引用方式（env/envFrom/volume/subPath）；
- [ ] 确认是否需要滚动重启；
- [ ] 验证 Pod 内配置已生效；
- [ ] 记录变更到配置管理仓库。

可参考 配置变更 SOP 模板，将本次经验沉淀为标准化流程。建议团队定期开展配置变更演练，模拟 ConfigMap 与 Secret 更新场景，验证应用能否正确读取新配置。对于关键业务配置，可以采用 ConfigMap/Secret 版本化与金丝雀发布策略，先在小范围验证再全量推送，降低配置错误带来的风险。同时，建议在应用启动日志中打印关键配置项的哈希值或版本号，便于快速核对当前运行配置是否为预期版本。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，不需要升级。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-013`
  - 根因：`ConfigMap 修改至错误命名空间；Secret subPath 挂载更新后未重建 Pod`
  - 影响集群：`ack-zyy-prod-04`
  - 影响命名空间：`scm-platform`
  - 修复操作：更新正确命名空间 ConfigMap、创建新 Secret、滚动重启 Deployment
  - 长期方案：配置变更纳入 GitOps，建立配置变更检查清单
  - 待跟进：无

## Related

- 配置映射
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- 密钥
- 阿里云专有云 DNS 解析异常：CoreDNS 配置被 ConfigMap 误改 + VPC DNS 转发
