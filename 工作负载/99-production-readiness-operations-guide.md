---
title: 工作负载与应用 生产就绪运维指南
description: 面向生产环境的 Kubernetes 工作负载与应用运维入口指南
summary: 面向生产环境的 Kubernetes 工作负载与应用运维入口指南
category: workloads
tags:
- production
- best-practices
- workloads
- operations
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 工作负载与应用 生产就绪运维指南是什么
- 如何按生产环境要求运维 工作负载与应用
trigger_keywords:
- 生产就绪
- 运维指南
- 工作负载
- 应用
- Deployment
- StatefulSet
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 工作负载与应用 生产就绪运维指南

本指南是 `工作负载` 的**生产运维入口文档**，覆盖 Deployment、StatefulSet、DaemonSet、Job/CronJob 等原生工作负载，以及 Java on Kubernetes 应用在生产环境交付前后的核心检查项、风险缓解、日常操作与故障排查路径。Java 应用镜像构建、JVM 调优、Spring Boot 探针与可观测性等深度内容，请参阅同域专题文章。

生产就绪不是一次性动作，而是贯穿设计、发布、运行、退役全生命周期的持续治理。本清单中的每一项都应纳入 CI/CD 门禁与变更评审；关键变更（如资源限制、网络策略、ServiceAccount 权限）建议在非生产环境预演后，再通过 GitOps 同步到生产集群，并在必要时先进行灰度验证。下文所有命令均以 `production` 命名空间为例，实际使用时请替换为真实命名空间。以下配置与命令在 Kubernetes v1.28 及以上版本验证通过。

---

## 一、生产环境检查清单

在将任一工作负载标记为生产就绪前，建议逐项确认并保留审计记录。检查结果建议以版本控制的形式保存在对应应用的运维仓库中，便于后续审计与回滚。

| 序号 | 检查项 | 验证命令 / 配置要点 |
|---|---|---|
| 1 | **资源请求与限制已声明** | `kubectl get pod <pod> -o jsonpath='{.spec.containers[*].resources}'`；确保 `requests` ≤ `limits`，关键服务设为 Guaranteed 或 Burstable 且 requests 贴近真实负载。 |
| 2 | **探针配置避免误判** | `kubectl describe pod` 查看 Liveness/Readiness/Startup；Startup 给足 `failureThreshold×periodSeconds`，Readiness 检测依赖项，Liveness 不检测依赖项。 |
| 3 | **滚动更新策略可控制中断** | Deployment `maxUnavailable=0`、`maxSurge=1`、`minReadySeconds≥10`、`progressDeadlineSeconds=300`。 |
| 4 | **PodDisruptionBudget 已配置** | `kubectl get pdb -n <ns>`；关键服务 `minAvailable` 不少于副本数的 51% 或 `maxUnavailable=1`。 |
| 5 | **自动伸缩策略已接入且指标有效** | `kubectl get hpa -n <ns>` 与 `kubectl top pods`；HPA `minReplicas≥2`，扩缩容行为设置稳定窗口。 |
| 6 | **拓扑分布保障高可用** | `topologySpreadConstraints` 跨可用区，`podAntiAffinity` 或 `topologySpreadConstraints` 避免同一节点堆叠。 |
| 7 | **ServiceAccount 最小权限** | 不使用 default SA；专用 SA 仅绑定必要 RBAC；`automountServiceAccountToken=false` 对无需 API 访问的 Pod。 |
| 8 | **网络策略已落地** | `kubectl get networkpolicy -n <ns>`；默认拒绝 + 白名单放行，限制东西向横向移动。 |
| 9 | **配置与密钥分离且加密** | ConfigMap 只读挂载，Secret 使用 external-secrets/Sealed Secrets 或 KMS 加密；不将密码写入环境变量快照。 |
| 10 | **镜像与供应链可信** | 使用固定 tag 或 digest；`imagePullPolicy: IfNotPresent` 或 `Always` 视场景而定；镜像扫描无高危漏洞。 |
| 11 | **可观测三支柱已接入** | 容器 stdout/json 日志、Prometheus 指标暴露、分布式追踪（OpenTelemetry）；关键告警覆盖错误率、延迟、饱和度。 |
| 12 | **安全上下文已收紧** | `runAsNonRoot: true`、`readOnlyRootFilesystem: true`、`seccompProfile: RuntimeDefault`、禁用特权。 |
| 13 | **Job/CronJob 有退出治理** | `activeDeadlineSeconds`、`backoffLimit`、`ttlSecondsAfterFinished`、`concurrencyPolicy=Forbid/Replace`。 |
| 14 | **StatefulSet 有备份与恢复方案** | PVC 与 Headless Service 就绪；Velero/PV snapshot 定期演练；应用级数据一致性校验。 |

对于核心生产服务，建议由 SRE、应用负责人与安全负责人等关键角色在变更评审中共同签字确认清单完成。未通过项应在发布前整改或记录为接受风险。

---

## 二、关键风险与缓解措施

生产环境中最常见的工作负载故障通常源于资源、发布、权限、网络与配置五个方面。以下风险按影响范围与发生频率排序，并给出可立即执行的缓解命令与配置。

### 2.1 资源不足导致驱逐与 OOMKilled

风险：`requests` 设置过低会让调度器将 Pod 压缩到已接近满载的节点；当节点触发 eviction 信号时，低优先级或 Burstable Pod 会首先被驱逐。Java 应用由于堆、Metaspace、DirectBuffer 与 GC 开销共同占用内存，更容易因堆外内存超过 `limits.memory` 而被内核 OOMKilled。

排查时首先区分是 kubelet 驱逐还是内核 OOM：驱逐会在 Event 中显示 `The node was low on resource: memory`，OOMKilled 则显示 `Reason: OOMKilled` 且退出码 137。

缓解：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod QoS 与最近终止原因
kubectl get pod -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.qosClass}{"\t"}{.status.containerStatuses[0].lastState.terminated.reason}{"\n"}{end}'

# 查看节点压力与驱逐信号
kubectl describe node <node> | grep -A 10 "Conditions"
```
- 对关键服务设置 `priorityClassName: production-critical`，避免在节点压力下被优先驱逐。
- 将 requests 设置为基于压测或历史监控的 P95 使用量，limits 不超过节点可分配资源的 80%。
- Java 应用将 `MaxRAMPercentage` 控制在 65%–75%，并显式限制 `-XX:MaxMetaspaceSize` 与 `-XX:MaxDirectMemorySize`；详见 [[工作负载/03-jvm-gc-container-tuning.md|JVM GC 容器调优深度指南]]。

### 2.2 发布期间服务中断

风险：滚动更新时若 `maxUnavailable` 设置为 100% 或缺失 PodDisruptionBudget，升级窗口内可能出现零可用副本；Readiness 探针配置错误还会让未就绪 Pod 提前接入 Service Endpoints，导致请求失败或雪崩。

缓解：

```yaml
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxSurge: 1
    maxUnavailable: 0
minReadySeconds: 10
progressDeadlineSeconds: 300
```

- 配合 PDB 与 Readiness 探针；对高敏感服务使用 Argo Rollouts/Flagger 渐进式交付（参见推荐新文件 渐进式交付 Argo Rollouts（待补充））。
- 在业务低峰期执行发布，并保留上一版本镜像 tag 以便快速回滚。

### 2.3 默认 ServiceAccount 权限放大

风险：默认 ServiceAccount 会被所有未显式指定 SA 的 Pod 自动挂载；若该 SA 绑定过宽 RBAC，容器逃逸或应用漏洞将直接转化为对 API Server 的横向移动能力。

缓解：

```yaml
spec:
  serviceAccountName: myapp-sa
  automountServiceAccountToken: false
```

- 为每个应用创建专用 SA；RBAC 规则精确到 verbs 与 resources；在云中启用 Workload Identity（参见 工作负载身份安全（待补充））。
- 对无需访问 Kubernetes API 的 Pod，显式设置 `automountServiceAccountToken: false`。

### 2.4 东西向流量未隔离

风险：默认情况下 Kubernetes 集群内所有 Pod 可互相通信；单点失陷后，攻击者可在 Namespace 间横向扫描、访问数据库或内部管理接口，扩大影响面。

缓解：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 默认拒绝所有入站
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes: [Ingress]
EOF
```
- 按标签与 Namespace 放行最小集合（详见 工作负载网络分段（待补充））。
- 对数据库、缓存等中间件 Namespace 单独设置更严格的 NetworkPolicy，仅允许指定工作负载访问。

### 2.5 配置与密钥不同步

风险：修改 ConfigMap 或 Secret 后，已运行的 Pod 不会自动重新加载挂载内容，导致新旧配置混用；Secret 若明文存储在 Git 中，会在仓库泄露时直接暴露生产凭证。

缓解：

```yaml
metadata:
  annotations:
    reloader.stakater.com/auto: "true"
```

- 使用 external-secrets 对接 Vault/云 KMS；ConfigMap 以 `subPath` 只读挂载，Secret 作为环境变量时启用 etcd 加密 at rest。
- 对敏感配置变更采用金丝雀或滚动重启，避免全量同时加载异常配置。

---

## 三、日常运维操作

以下操作是 SRE 在生产环境值班与变更窗口中最常执行的步骤。所有命令建议在执行前先使用 `--dry-run=client` 或 diff 确认影响范围，避免 unintended change，并保留操作审计日志。

### 3.1 发布与回滚

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 观察滚动更新
kubectl rollout status deployment/myapp -n production --timeout=300s

# 查看历史版本
kubectl rollout history deployment/myapp -n production

# 快速回滚到上一版本
kubectl rollout undo deployment/myapp -n production

# Helm 原子发布（失败自动回滚）
helm upgrade myapp ./myapp-chart -n production --install --atomic --wait --timeout 600s
```
### 3.2 扩缩容与容量审计

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动扩容
kubectl scale deployment/myapp -n production --replicas=6

# 查看资源使用
kubectl top pod -n production -l app=myapp
kubectl top node

# 使用 kubectl-resource_capacity 插件做容量盘点
kubectl resource-capacity --pods --util --sort cpu.util

# 查看 HPA 状态
kubectl describe hpa myapp-hpa -n production
```
### 3.3 节点维护与驱逐

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
# 标记节点不可调度并驱逐工作负载
kubectl cordon <node>
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force --timeout=120s

# 维护完成后恢复
kubectl uncordon <node>

# 检查 PDB 阻止事件
kubectl get events -n production --field-selector reason=NoPods
```
> 节点维护与 PodDisruptionBudget 的 interplay 详见 [[工作负载/核心工作负载/18-node-management-operations.md|节点管理操作]]。

### 3.4 配置 / 密钥轮换

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 触发滚动重启以加载新配置
kubectl rollout restart deployment/myapp -n production

# 对单个 Pod 验证新配置是否生效
kubectl exec -n production deploy/myapp -- env | grep SECRET_
```
### 3.5 Job / CronJob 管理

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动触发一次 CronJob
kubectl create job --from=cronjob/my-cronjob my-cronjob-manual-001 -n production

# 查看 Job 日志
kubectl logs job/my-batch-job -n production --tail=200

# 暂停 CronJob
kubectl patch cronjob my-cronjob -n production -p '{"spec":{"suspend":true}}'
```
---

## 四、故障排查速查

排查工作负载故障时，建议遵循“先状态、后日志、再事件、最后深入容器”的顺序。下表列出生产环境最常见症状，并提供对应的确认命令与修复措施。若症状复杂或跨多个域，请结合 [[工作负载/核心工作负载/07-workload-troubleshooting-handbook.md|工作负载故障排查手册]] 进行系统分析。

| 症状 | 可能原因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 长期 `Pending` | 资源不足、污点不匹配、PVC 未绑定 | `kubectl describe pod <pod> -n <ns>` | 增加节点、调整 requests/tolerations、修复 StorageClass |
| `CrashLoopBackOff` | 应用启动失败、探针配置错误、依赖不可达 | `kubectl logs --previous` + `kubectl describe pod` | 修复代码/配置；放宽 startupProbe；检查依赖服务 |
| `OOMKilled` | 内存 limit 过低或堆外内存泄漏 | `dmesg | grep -i oom` / `kubectl exec -- jcmd 1 VM.flags` | 增大 limit 或降低 `MaxRAMPercentage`；限制 Metaspace/DirectBuffer |
| 服务 502 / 连接超时 | Readiness 失败、Endpoints 为空、NetworkPolicy 阻断 | `kubectl get endpoints` + `kubectl describe networkpolicy` | 修复 readiness；确认 selector；添加允许规则 |
| HPA 不扩容 | metrics-server 异常、目标阈值过高、稳定窗口 | `kubectl describe hpa` / `kubectl get --raw /apis/metrics.k8s.io/v1beta1/pods` | 修复指标源；调整 target；观察 stabilizationWindow |
| Job 失败或超时 | backoffLimit 用尽、资源不足、任务死锁 | `kubectl describe job` / `kubectl logs job/...` | 提高 limit、拆分任务、调整 activeDeadlineSeconds |
| StatefulSet 启动卡住 | PVC Pending、Headless DNS 未解析、init 容器阻塞 | `kubectl describe sts` / `kubectl get pvc` / `kubectl logs -c init` | 修复存储、检查 DNS、重跑 init 逻辑 |

更系统的排查流程请参考 [[工作负载/核心工作负载/07-workload-troubleshooting-handbook.md|工作负载故障排查手册]]。

---

## 五、与其他域的协作边界

工作负载与应用处于 Kubernetes 技术栈的中心位置，向上承接业务发布，向下依赖集群、网络、存储、安全与可观测能力。明确协作边界可避免重复建设，也能在故障时快速定位责任域。

- **网络**：Service、Ingress、NetworkPolicy、Service Mesh、eBPF 可观测等由网络域主导；本域负责在工作负载侧正确声明 `containerPort`、标签选择器、Readiness 探针，确保与网络策略协同。
- **安全**：RBAC、Pod Security Admission、镜像签名、Secret 加密、审计日志由安全域主导；本域负责落实最小权限 SA、安全上下文、不可变根文件系统等 workload 层加固。
- **可观测性**：监控平台、日志收集、告警规则、SLO 定义由可观测域主导；本域负责暴露 `/metrics`、结构化日志、trace context 传播，并按 SLO 设置 HPA/PDB。
- **平台工程**：GitOps、租户隔离、配额、成本治理由平台域主导；本域负责在 CI/CD 流水线中注入镜像 digest、资源请求与标签规范。
- **可靠性**：灾备、混沌工程、容量规划由可靠性域主导；本域负责执行 StatefulSet/有状态应用备份、定期发布演练与 PDB 验证。

---

## 六、推荐阅读

### 同域专题

- [[工作负载/README.md|domain-02 目录与总览]]
- [[工作负载/02-spring-boot-kubernetes-production.md|Spring Boot on Kubernetes 生产实践指南]]
- [[工作负载/03-jvm-gc-container-tuning.md|JVM GC 容器调优深度指南]]
- [[工作负载/04-java-operator-sdk-development.md|Java Operator SDK 开发指南]]
- [[工作负载/05-quarkus-native-kubernetes.md|Quarkus Native on Kubernetes]]
- [[工作负载/06-java-cicd-tekton-argocd.md|Java CI/CD 与 Tekton/ArgoCD]]
- [[工作负载/07-java-observability-kubernetes.md|Java 可观测性指南]]
- [[工作负载/核心工作负载/21-hpa-vpa-autoscaling.md|HPA/VPA 自动伸缩]]
- [[工作负载/核心工作负载/23-resource-management.md|资源管理]]
- [[工作负载/核心工作负载/03-statefulset-advanced-operations.md|StatefulSet 高级操作]]

### 计划补充的新文件（来自缺口分析）

- 工作负载身份安全（待补充）
- 工作负载网络分段（待补充）
- KEDA 事件驱动自动伸缩（待补充）
- 渐进式交付 Argo Rollouts（待补充）
- 有状态工作负载备份与灾备（待补充）

### 相关域

- [[网络/README.md|网络]]
- [[安全/README.md|安全]]
- [[可观测性/README.md|可观测性]]
- [[平台工程/README.md|平台工程]]
- [[可靠性/README.md|可靠性]]

---

## 七、容量规划与成本优化

生产环境的工作负载容量规划需要平衡性能、成本与可靠性。以下提供从资源评估到成本优化的完整方法论。

### 7.1 资源评估方法

| 评估维度 | 数据来源 | 计算方法 | 安全系数 |
|---------|---------|---------|----------|
| CPU requests | 历史 P95 使用量 | `max(7d P95) × 1.2` | 1.2x |
| CPU limits | 峰值使用量 | `max(7d max) × 1.5` | 1.5x |
| Memory requests | 历史 P95 + 堆外 | `P95 × 1.3 + 200Mi` | 1.3x + buffer |
| Memory limits | OOM 阈值 | `requests × 1.5` | 1.5x |
| 副本数 | 流量预测 | `peak_qps / per_pod_qps × 1.3` | 1.3x |

### 7.2 容量规划 PromQL

```promql
# 工作负载实际资源使用率
sum by (deployment) (
  rate(container_cpu_usage_seconds_total{namespace="production", container!="POD"}[5m])
) / sum by (deployment) (
  kube_pod_container_resource_requests{namespace="production", resource="cpu"}
) * 100

# 节点资源碎片率（已分配但未使用）
1 - (
  sum(kube_pod_container_resource_requests{resource="cpu"}) by (node)
  / sum(kube_node_status_allocatable{resource="cpu"}) by (node)
)

# HPA 扩容预测（基于历史趋势）
predict_linear(
  sum(rate(http_requests_total{namespace="production"}[5m]))[1h:5m],
  3600 * 4  # 预测 4 小时后
)
```

### 7.3 成本优化策略

| 策略 | 适用场景 | 节省比例 | 实施难度 |
|-----|---------|---------|----------|
| 资源右sizing | 过度配置的工作负载 | 20-40% | 低 |
|  spot/抢占式实例 | 无状态/容错工作负载 | 60-80% | 中 |
| 自动缩容到零 | 开发/测试环境 | 50-70% | 低 |
| 混合实例类型 | 不同优先级工作负载 | 30-50% | 中 |
| 存储分层 | 冷热数据分离 | 40-60% | 中 |
| 镜像瘦身 | 所有工作负载 | 10-20% | 低 |

### 7.4 成本监控 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cost-report
  namespace: monitoring
spec:
  schedule: "0 8 * * 1"  # 每周一 8:00
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: reporter
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 周度成本报告 $(date) ==="
                  
                  # 1. 资源使用率 Top 10
                  echo "[1] CPU 使用率 Top 10:"
                  kubectl top pods -A --sort-by=cpu | head -11
                  
                  # 2. 过度配置检测
                  echo "[2] 过度配置检测 (requests > 2x actual):"
                  kubectl get pods -n production -o json | jq -r '
                    .items[] |
                    select(.spec.containers[].resources.requests.cpu != null) |
                    "\(.metadata.name): requests=\(.spec.containers[].resources.requests.cpu)"'
                  
                  # 3. 未使用 PVC
                  echo "[3] 未绑定 PVC:"
                  kubectl get pvc -A --field-selector=status.phase!=Bound
                  
                  # 4. 低使用率节点
                  echo "[4] 节点资源使用率:"
                  kubectl top nodes
                  
                  echo "=== 报告完成 ==="
```

---

## 八、变更管理与发布策略

### 8.1 变更分级

| 级别 | 定义 | 审批要求 | 回滚时限 | 示例 |
|-----|------|---------|---------|------|
| P0 紧急 | 生产故障修复 | 值班 SRE 口头确认 | 5min | 热修复、回滚 |
| P1 重要 | 功能发布、配置变更 | 变更评审会 | 30min | 新版本发布、HPA 调整 |
| P2 常规 | 基础设施变更 | 工单审批 | 2h | 节点扩容、存储扩容 |
| P3 低危 | 文档、标签变更 | 自动审批 | N/A | 标签更新、注释修改 |

### 8.2 发布检查清单

| 阶段 | 检查项 | 验证方法 |
|-----|--------|----------|
| 发布前 | 镜像已扫描无高危漏洞 | Trivy/Grype 扫描报告 |
| 发布前 | 资源请求已更新 | 对比压测结果 |
| 发布前 | 回滚方案已准备 | 确认上一版本镜像可用 |
| 发布前 | 监控告警已配置 | 检查 PrometheusRule |
| 发布中 | 灰度比例逐步扩大 | 5% → 25% → 50% → 100% |
| 发布中 | 错误率/延迟无异常 | Grafana 实时观察 |
| 发布后 | 72h 稳定性观察 | SLO 达标率 |
| 发布后 | 资源使用符合预期 | kubectl top + Prometheus |

### 8.3 渐进式交付配置

```yaml
# Argo Rollouts 渐进式交付示例
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: production-api
spec:
  replicas: 5
  strategy:
    canary:
      steps:
        - setWeight: 5
        - pause: { duration: 10m }
        - analysis:
            templates:
              - templateName: success-rate
        - setWeight: 25
        - pause: { duration: 10m }
        - setWeight: 50
        - pause: { duration: 10m }
        - setWeight: 100
      canaryService: api-canary
      stableService: api-stable
  selector:
    matchLabels:
      app: api
  template:
    metadata:
      labels:
        app: api
    spec:
      containers:
        - name: api
          image: myregistry/api:v1.2.3
---
# 自动分析模板
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  metrics:
    - name: success-rate
      interval: 5m
      successCondition: result[0] >= 0.99
      failureLimit: 3
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(http_requests_total{status=~"5.."}[5m]))
            / sum(rate(http_requests_total[5m]))
```

---

## 九、灾备与恢复演练

### 9.1 备份策略

| 资源类型 | 备份工具 | 频率 | 保留期 | RPO |
|---------|---------|------|-------|-----|
| etcd | etcdctl snapshot | 每小时 | 7 天 | 1h |
| PVC 数据 | Velero + CSI Snapshot | 每日 | 30 天 | 24h |
| 集群配置 | GitOps 仓库 | 实时 | 永久 | 0 |
| Secret | external-secrets + Vault | 实时 | 永久 | 0 |
| 应用数据 | 应用级备份 | 按业务 | 按合规 | 按业务 |

### 9.2 恢复演练检查清单

| 序号 | 演练项 | 频率 | 成功标准 |
|-----|--------|------|----------|
| 1 | etcd 恢复 | 季度 | 集群正常启动，数据完整 |
| 2 | PVC 恢复 | 月度 | 数据一致，应用正常 |
| 3 | 命名空间级恢复 | 月度 | 所有资源恢复，服务正常 |
| 4 | 集群级恢复 | 半年度 | 全集群重建，RTO < 4h |
| 5 | 跨区域故障转移 | 年度 | 流量切换，RTO < 1h |

### 9.3 Velero 备份配置

```yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: production-daily
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每日 2:00
  template:
    includedNamespaces:
      - production
      - staging
    includedResources:
      - deployments
      - statefulsets
      - configmaps
      - secrets
      - persistentvolumeclaims
    storageLocation: default
    volumeSnapshotLocations:
      - default
    ttl: 720h  # 30 天保留
---
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: k8s-backups
    prefix: production
  config:
    region: cn-hangzhou
```

---

## 十、自动化运维工具链

### 10.1 生产就绪自动化检查脚本

```bash
#!/bin/bash
# 🟢 低风险：生产就绪自动化检查
set -euo pipefail

NAMESPACE=${1:-production}
PASS=0
FAIL=0

check() {
  local desc="$1"
  local result="$2"
  if [ "$result" = "true" ]; then
    echo "✓ $desc"
    ((PASS++))
  else
    echo "✗ $desc"
    ((FAIL++))
  fi
}

echo "=== 生产就绪检查: $NAMESPACE ==="

# 1. 资源限制检查
NO_LIMITS=$(kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.spec.containers[].resources.limits == null)] | length')
check "所有 Pod 已设置资源限制" "$([ "$NO_LIMITS" -eq 0 ] && echo true || echo false)"

# 2. 探针检查
NO_PROBES=$(kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.spec.containers[].readinessProbe == null)] | length')
check "所有 Pod 已配置 Readiness 探针" "$([ "$NO_PROBES" -eq 0 ] && echo true || echo false)"

# 3. PDB 检查
PDB_COUNT=$(kubectl get pdb -n $NAMESPACE --no-headers 2>/dev/null | wc -l)
check "PDB 已配置" "$([ "$PDB_COUNT" -gt 0 ] && echo true || echo false)"

# 4. HPA 检查
HPA_COUNT=$(kubectl get hpa -n $NAMESPACE --no-headers 2>/dev/null | wc -l)
check "HPA 已配置" "$([ "$HPA_COUNT" -gt 0 ] && echo true || echo false)"

# 5. NetworkPolicy 检查
NP_COUNT=$(kubectl get networkpolicy -n $NAMESPACE --no-headers 2>/dev/null | wc -l)
check "NetworkPolicy 已配置" "$([ "$NP_COUNT" -gt 0 ] && echo true || echo false)"

# 6. 安全上下文检查
PRIVILEGED=$(kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.spec.containers[].securityContext.privileged == true)] | length')
check "无特权容器" "$([ "$PRIVILEGED" -eq 0 ] && echo true || echo false)"

# 7. 镜像标签检查
LATEST=$(kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.spec.containers[].image | endswith(":latest"))] | length')
check "无 :latest 标签镜像" "$([ "$LATEST" -eq 0 ] && echo true || echo false)"

echo ""
echo "=== 结果: $PASS 通过, $FAIL 失败 ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
```

### 10.2 日常运维自动化

| 任务 | 频率 | 工具 | 自动化程度 |
|-----|------|------|------------|
| 资源使用报告 | 每日 | CronJob + Prometheus | 全自动 |
| 镜像漏洞扫描 | 每次发布 | Trivy + CI | 全自动 |
| 证书轮换 | 自动 | cert-manager | 全自动 |
| 备份验证 | 每周 | Velero + CronJob | 半自动 |
| 容量审计 | 每月 | 自定义脚本 | 半自动 |
| 灾备演练 | 每季 | 手动 + 脚本 | 手动 |

---

## 十一、生产就绪评分体系

### 评分维度与权重

| 维度 | 权重 | 满分 | 评分标准 |
|-----|------|------|----------|
| 资源配置 | 20% | 20 | requests/limits 合理、QoS 正确 |
| 高可用 | 20% | 20 | 多副本、跨 AZ、PDB、HPA |
| 可观测性 | 20% | 20 | 日志/指标/追踪完整、告警覆盖 |
| 安全合规 | 20% | 20 | 最小权限、安全上下文、网络策略 |
| 运维就绪 | 20% | 20 | 回滚方案、备份、文档、演练 |

### 评分等级

| 等级 | 分数范围 | 含义 | 发布决策 |
|-----|---------|------|----------|
| A | 90-100 | 生产就绪 | 可直接发布 |
| B | 75-89 | 基本就绪 | 需补充缺失项后发布 |
| C | 60-74 | 部分就绪 | 需整改后重新评估 |
| D | <60 | 未就绪 | 禁止发布 |

### 评分计算脚本

```bash
#!/bin/bash
# 🟢 低风险：生产就绪评分计算
set -euo pipefail

NAMESPACE=${1:-production}
SCORE=0

echo "=== 生产就绪评分: $NAMESPACE ==="

# 维度 1: 资源配置 (20分)
echo "[1] 资源配置 (20分)"
# 检查资源限制
HAS_LIMITS=$(kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.spec.containers[].resources.limits != null)] | length * 100 / (.items | length)')
echo "  资源限制覆盖率: ${HAS_LIMITS}%"
SCORE=$((SCORE + HAS_LIMITS / 5))  # 20分制

# 维度 2: 高可用 (20分)
echo "[2] 高可用 (20分)"
REPLICAS=$(kubectl get deploy -n $NAMESPACE -o json | jq '[.items[] | select(.spec.replicas >= 2)] | length * 100 / (.items | length)')
echo "  多副本部署比例: ${REPLICAS}%"
PDB=$(kubectl get pdb -n $NAMESPACE --no-headers 2>/dev/null | wc -l)
echo "  PDB 数量: $PDB"
SCORE=$((SCORE + REPLICAS / 10 + (PDB > 0 ? 5 : 0)))

# 维度 3: 可观测性 (20分)
echo "[3] 可观测性 (20分)"
# 检查 ServiceMonitor
SM=$(kubectl get servicemonitor -n $NAMESPACE --no-headers 2>/dev/null | wc -l || echo 0)
echo "  ServiceMonitor 数量: $SM"
SCORE=$((SCORE + (SM > 0 ? 10 : 0)))

# 维度 4: 安全合规 (20分)
echo "[4] 安全合规 (20分)"
NP=$(kubectl get networkpolicy -n $NAMESPACE --no-headers 2>/dev/null | wc -l)
echo "  NetworkPolicy 数量: $NP"
SCORE=$((SCORE + (NP > 0 ? 10 : 0)))

# 维度 5: 运维就绪 (20分)
echo "[5] 运维就绪 (20分)"
HPA=$(kubectl get hpa -n $NAMESPACE --no-headers 2>/dev/null | wc -l)
echo "  HPA 数量: $HPA"
SCORE=$((SCORE + (HPA > 0 ? 10 : 0)))

echo ""
echo "=== 总分: $SCORE / 100 ==="
if [ $SCORE -ge 90 ]; then
  echo "等级: A (生产就绪)"
elif [ $SCORE -ge 75 ]; then
  echo "等级: B (基本就绪)"
elif [ $SCORE -ge 60 ]; then
  echo "等级: C (部分就绪)"
else
  echo "等级: D (未就绪，禁止发布)"
fi
```

---

本指南将随着 KUDIG 知识库持续迭代。当你在生产环境中验证了新检查项或发现新的高频故障模式时，请将经验补充到同域故障排查手册与长期记忆文档中，以便整个团队共享。


<!-- risk-assessed -->
