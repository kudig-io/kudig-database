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

本指南是 `domain-02-workloads-applications` 的**生产运维入口文档**，覆盖 Deployment、StatefulSet、DaemonSet、Job/CronJob 等原生工作负载，以及 Java on Kubernetes 应用在生产环境交付前后的核心检查项、风险缓解、日常操作与故障排查路径。Java 应用镜像构建、JVM 调优、Spring Boot 探针与可观测性等深度内容，请参阅同域专题文章。

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
- Java 应用将 `MaxRAMPercentage` 控制在 65%–75%，并显式限制 `-XX:MaxMetaspaceSize` 与 `-XX:MaxDirectMemorySize`；详见 [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/02-jvm-gc-container-tuning|JVM GC 容器调优深度指南]]。

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
> 节点维护与 PodDisruptionBudget 的 interplay 详见 [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/00-core-workloads/06-node-management-operations|节点管理操作]]。

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

排查工作负载故障时，建议遵循“先状态、后日志、再事件、最后深入容器”的顺序。下表列出生产环境最常见症状，并提供对应的确认命令与修复措施。若症状复杂或跨多个域，请结合 [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-02-workloads-applications/00-core-workloads/03-workload-troubleshooting-handbook|工作负载故障排查手册]] 进行系统分析。

| 症状 | 可能原因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 长期 `Pending` | 资源不足、污点不匹配、PVC 未绑定 | `kubectl describe pod <pod> -n <ns>` | 增加节点、调整 requests/tolerations、修复 StorageClass |
| `CrashLoopBackOff` | 应用启动失败、探针配置错误、依赖不可达 | `kubectl logs --previous` + `kubectl describe pod` | 修复代码/配置；放宽 startupProbe；检查依赖服务 |
| `OOMKilled` | 内存 limit 过低或堆外内存泄漏 | `dmesg | grep -i oom` / `kubectl exec -- jcmd 1 VM.flags` | 增大 limit 或降低 `MaxRAMPercentage`；限制 Metaspace/DirectBuffer |
| 服务 502 / 连接超时 | Readiness 失败、Endpoints 为空、NetworkPolicy 阻断 | `kubectl get endpoints` + `kubectl describe networkpolicy` | 修复 readiness；确认 selector；添加允许规则 |
| HPA 不扩容 | metrics-server 异常、目标阈值过高、稳定窗口 | `kubectl describe hpa` / `kubectl get --raw /apis/metrics.k8s.io/v1beta1/pods` | 修复指标源；调整 target；观察 stabilizationWindow |
| Job 失败或超时 | backoffLimit 用尽、资源不足、任务死锁 | `kubectl describe job` / `kubectl logs job/...` | 提高 limit、拆分任务、调整 activeDeadlineSeconds |
| StatefulSet 启动卡住 | PVC Pending、Headless DNS 未解析、init 容器阻塞 | `kubectl describe sts` / `kubectl get pvc` / `kubectl logs -c init` | 修复存储、检查 DNS、重跑 init 逻辑 |

更系统的排查流程请参考 [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-02-workloads-applications/00-core-workloads/03-workload-troubleshooting-handbook|工作负载故障排查手册]]。

---

## 五、与其他域的协作边界

工作负载与应用处于 Kubernetes 技术栈的中心位置，向上承接业务发布，向下依赖集群、网络、存储、安全与可观测能力。明确协作边界可避免重复建设，也能在故障时快速定位责任域。

- **domain-03-networking-traffic**：Service、Ingress、NetworkPolicy、Service Mesh、eBPF 可观测等由网络域主导；本域负责在工作负载侧正确声明 `containerPort`、标签选择器、Readiness 探针，确保与网络策略协同。
- **domain-05-security-compliance**：RBAC、Pod Security Admission、镜像签名、Secret 加密、审计日志由安全域主导；本域负责落实最小权限 SA、安全上下文、不可变根文件系统等 workload 层加固。
- **domain-06-observability**：监控平台、日志收集、告警规则、SLO 定义由可观测域主导；本域负责暴露 `/metrics`、结构化日志、trace context 传播，并按 SLO 设置 HPA/PDB。
- **domain-07-platform-engineering**：GitOps、租户隔离、配额、成本治理由平台域主导；本域负责在 CI/CD 流水线中注入镜像 digest、资源请求与标签规范。
- **domain-09-reliability-engineering**：灾备、混沌工程、容量规划由可靠性域主导；本域负责执行 StatefulSet/有状态应用备份、定期发布演练与 PDB 验证。

---

## 六、推荐阅读

### 同域专题

- [[domain-02-workloads-applications/README.md|domain-02 目录与总览]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/01-spring-boot-kubernetes-production|Spring Boot on Kubernetes 生产实践指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/02-jvm-gc-container-tuning|JVM GC 容器调优深度指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/01-java-operator-sdk-development|Java Operator SDK 开发指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/02-quarkus-native-kubernetes|Quarkus Native on Kubernetes]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/03-java-cicd-tekton-argocd|Java CI/CD 与 Tekton/ArgoCD]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/04-java-observability-kubernetes|Java 可观测性指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-02-workloads-applications/00-core-workloads/11-hpa-vpa-autoscaling|HPA/VPA 自动伸缩]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/00-core-workloads/07-resource-management|资源管理]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/00-core-workloads/01-statefulset-advanced-operations|StatefulSet 高级操作]]

### 计划补充的新文件（来自缺口分析）

- 工作负载身份安全（待补充）
- 工作负载网络分段（待补充）
- KEDA 事件驱动自动伸缩（待补充）
- 渐进式交付 Argo Rollouts（待补充）
- 有状态工作负载备份与灾备（待补充）

### 相关域

- [[domain-03-networking-traffic/README.md|domain-03-networking-traffic]]
- [[domain-05-security-compliance/README.md|domain-05-security-compliance]]
- [[domain-06-observability/README.md|domain-06-observability]]
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]]
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]]

---

本指南将随着 KUDIG 知识库持续迭代。当你在生产环境中验证了新检查项或发现新的高频故障模式时，请将经验补充到同域故障排查手册与长期记忆文档中，以便整个团队共享。


<!-- risk-assessed -->
