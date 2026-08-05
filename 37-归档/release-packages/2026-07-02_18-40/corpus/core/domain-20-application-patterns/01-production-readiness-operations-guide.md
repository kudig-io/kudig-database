---
title: 应用架构模式 生产就绪运维指南
description: 面向 Kubernetes 生产环境的应用架构模式运维指南，覆盖上线前检查、风险缓解、日常运维、故障排查与跨域协作边界。
summary: 面向 Kubernetes 生产环境的应用架构模式运维指南，覆盖上线前检查、风险缓解、日常运维、故障排查与跨域协作边界。
category: application-architecture
tags:
- production
- best-practices
- application-architecture
- operations
- runbooks
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
- 应用架构模式 生产就绪运维指南是什么
- 如何按生产环境要求运维 应用架构模式
trigger_keywords:
- 生产就绪
- 运维指南
- 应用架构模式
- application patterns
- 生产检查清单
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
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


# 应用架构模式 生产就绪运维指南

> **适用场景**: 基于 Kubernetes 运行的业务应用架构（电商、小程序、IM/RTC、SaaS、金融科技等）在生产环境上线前检查与日常运维。
> **适用版本**: Kubernetes v1.28 - v1.33
> **目标读者**: SRE、运维工程师、平台工程师

本指南聚焦 [[domain-20-application-patterns/README.md|Application Patterns]] 领域中**应用工作负载投产与持续运维**的实操要求，不重复阐述具体行业架构，而是把各垂直场景（[[domain-20-application-patterns/行业架构/01-ecommerce-architecture.md|电商]]、[[domain-20-application-patterns/行业架构/02-mini-program-architecture.md|小程序]]、[[domain-20-application-patterns/行业架构/06-fintech-architecture.md|金融科技]]、[[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-20-application-patterns/topic-application-architecture/06-saas-multitenant-architecture|SaaS 多租户]] 等）共性的生产就绪动作抽象为可执行清单与命令。

区别于[[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/02-production-sre-daily-ops|通用生产巡检手册]]，本文更强调**面向应用架构特性的检查项**：例如有状态服务的 PVC 快照、电商秒杀的 HPA 行为、多租户 SaaS 的网络隔离等。

---

## 1. 生产环境检查清单

在将任一应用架构模式推入生产前，SRE 必须逐项确认以下检查项。建议以 Helm Chart / Kustomize Overlay 为最小交付单元进行 gate review。每项检查应留下可审计的痕迹（命令输出截图或 CI 报告），关键项未通过不得进入生产流量。

检查清单按**稳定性、弹性、可观测性、安全、可恢复性**五个维度组织。前 12 项为最小必要集，后续可根据行业合规要求（如金融科技 PCI-DSS、政务等保）追加。

| # | 检查项 | 验收标准 | 验证命令/动作 |
|---|---|---|---|
| 1 | 工作负载类型匹配 | 有状态服务使用 StatefulSet，无状态服务使用 Deployment，守护进程使用 DaemonSet，批处理使用 Job/CronJob | `kubectl get deploy,sts,ds,job -n <ns>` |
| 2 | 健康探针配置 | 同时配置 livenessProbe、readinessProbe、startupProbe；路径与端口与业务真实健康接口一致 | `kubectl get pod -n <ns> -o yaml | grep -A5 probe` |
| 3 | 优雅停机 | `terminationGracePeriodSeconds` ≥ 30；存在 preStop Hook 执行优雅关闭 | `kubectl get deploy <app> -o yaml | grep -A3 preStop` |
| 4 | 资源请求与限制 | requests ≤ limits；内存 limits 设置合理；QoS 非 BestEffort | `kubectl top pod -n <ns>` + `kubectl get pod <pod> -o yaml | grep qosClass` |
| 5 | Pod 中断预算 | 关键服务配置 PDB，`minAvailable` 或 `maxUnavailable` 与副本数匹配 | `kubectl get pdb -n <ns>` |
| 6 | 多可用区拓扑分布 | 配置 `topologySpreadConstraints` 或 PodAntiAffinity，避免同节点/同可用区单点 | `kubectl get pod -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,ZONE:.spec.nodeSelector` |
| 7 | 自动扩缩容 | HPA/KEDA 已启用并验证；指标阈值与业务容量模型一致 | `kubectl get hpa -n <ns>` |
| 8 | 入口与熔断 | Ingress/ Gateway 配置超时、重试、熔断；TLS 证书有效期 > 30 天 | `kubectl get ingress -n <ns>` + `kubectl get certificate -n <ns>` |
| 9 | 网络隔离 | Namespace 级别 NetworkPolicy 已生效；敏感服务默认拒绝入站 | `kubectl get networkpolicy -n <ns>` |
| 10 | 可观测性埋点 | RED（Rate/Errors/Duration）指标、结构化日志、链路追踪均已接入 | `kubectl get servicemonitor -n <ns>` |
| 11 | 密钥与配置管理 | Secret/ConfigMap 版本化；敏感数据使用 ExternalSecret/Secrets Store CSI；配置变更需滚动更新 | `kubectl get externalsecret -n <ns>` |
| 12 | 备份与回滚 | 有状态应用 PVC 快照策略就绪；发布策略支持回滚（Helm rollback / ArgoCD sync） | `helm history <release> -n <ns>` |

---

## 2. 关键风险与缓解措施

### 2.1 流量突增导致 Pod 级联过载

**风险**: 大促、热点事件或上游重试风暴触发 HPA 扩容不及，进而导致 OOMKilled 或 CPU 限流，引发级联故障。该风险在电商秒杀、直播推流、社交热点事件中尤为常见。

**关键指标**: P99 延迟 > 阈值、CPU 利用率陡升、队列堆积、HPA 当前副本数持续接近 maxReplicas。

**缓解措施**:

```yaml
# HPA 行为配置：快速扩容、缓慢缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  minReplicas: 3
  maxReplicas: 100
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
        - type: Percent
          value: 200
          periodSeconds: 15
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
```

**验证命令**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa app-hpa -n production --watch
kubectl top pods -n production -l app=app
```
### 2.2 有状态服务因节点故障导致数据不一致

**风险**: StatefulSet 副本集中于同一可用区或同一节点，节点维护/故障时 PVC 漂移异常，或副本选举失败。电商订单、金融支付、IM 消息等场景的有状态组件必须避免此类拓扑聚集。

**关键指标**: 同一可用区 Pod 数超过副本数的一半、PVC 处于 Lost/Released 状态、StatefulSet 长时间未 Ready。

**缓解措施**:

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: order-db
  namespace: ecommerce
spec:
  serviceName: order-db
  replicas: 3
  template:
    spec:
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: order-db
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app: order-db
              topologyKey: kubernetes.io/hostname
```

**验证命令**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n ecommerce -l app=order-db -o wide
kubectl get pvc -n ecommerce -l app=order-db
```
### 2.3 配置/Secret 变更未触发滚动更新导致配置漂移

**风险**: 直接修改 ConfigMap/Secret 后，旧 Pod 仍使用旧配置，引发灰度期间状态不一致。该问题在蓝绿发布、A/B 测试窗口最容易被忽视，导致新旧版本混跑。

**关键指标**: 同一 Deployment 不同 Pod 的环境变量/配置文件内容不一致、业务日志出现配置版本号分叉。

**缓解措施**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 推荐：通过 helm upgrade 或 kustomize 变更，触发滚动更新
helm upgrade --install app ./chart -n production -f values-prod.yaml

# 推荐：在 Deployment 模板中为 ConfigMap 计算 checksum，实现内容变更自动触发滚动更新
template:
  metadata:
    annotations:
      checksum/config: {{ include (print $.Template.BasePath "/configmap.yaml") . | sha256sum }}

# 手动场景：为 ConfigMap 增加版本后缀并在 Deployment 中引用
kubectl rollout restart deployment/app -n production
```
**验证命令**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout status deployment/app -n production
kubectl get pods -n production -l app=app -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.config\.checksum}{"\n"}{end}'
```
### 2.4 发布过程中可用性受损

**风险**: 滚动更新时 maxSurge/maxUnavailable 设置激进，或 readinessProbe 不可靠，导致流量进入未就绪 Pod。金融交易、在线支付等对可用性敏感的场景应严格保证 `maxUnavailable: 0`。

**关键指标**: 发布期间错误率上升、Ingress 返回 503、Pod 状态在 Running 与 NotReady 之间抖动。

**缓解措施**:

```yaml
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%
      maxUnavailable: 0
  template:
    spec:
      terminationGracePeriodSeconds: 60
      containers:
        - name: app
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 5
            failureThreshold: 3
            successThreshold: 1
          lifecycle:
            preStop:
              exec:
                command: ["/bin/sh", "-c", "sleep 10 && curl -X POST localhost:8080/shutdown"]
```

**发布窗口建议**: 核心交易服务安排在业务低峰期；发布前确保 HPA 当前副本数 ≥ PDB `minAvailable` + `maxSurge`。

### 2.5 权限过大与运行时入侵

**风险**: 容器以 root 运行、privileged 模式或挂载敏感宿主机路径，扩大攻击面。金融科技、政务、SaaS 多租户等场景必须强制执行最小权限原则。

**关键指标**: Pod Security Admission 触发 `Restricted` 告警、Falco 报告特权容器启动、镜像扫描发现高危 CVE。

**缓解措施**:

```yaml
spec:
  template:
    spec:
      automountServiceAccountToken: false
      securityContext:
        runAsNonRoot: true
        seccompProfile:
          type: RuntimeDefault
      containers:
        - name: app
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            runAsUser: 1000
            capabilities:
              drop:
                - ALL
          resources:
            requests:
              cpu: "100m"
              memory: "128Mi"
            limits:
              cpu: "1"
              memory: "512Mi"
```

**验证命令**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查特权容器
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].securityContext.privileged==true) | .metadata.name'

# 检查 root 运行容器
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].securityContext.runAsUser==0) | .metadata.name'
```
---

## 3. 日常运维操作

日常运维应围绕**资源健康、容量管理、发布变更、可观测性、成本**五个维度展开。以下命令按使用频率排序，建议在值班手册中配置为 alias 或封装为脚本。

### 3.1 例行巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查异常 Pod（排除 Succeeded 的 Job）
kubectl get pods -A -w
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

# 2. 检查资源使用 Top 10
kubectl top pods -A --sort-by=cpu | head -n 11
kubectl top nodes --sort-by=cpu | head -n 11

# 3. 检查最近事件，过滤 Warning 与 Error
kubectl get events -A --sort-by='.lastTimestamp' | tail -n 50
kubectl get events -A --field-selector type=Warning --sort-by='.lastTimestamp' | tail -n 30

# 4. 检查证书过期（cert-manager）
kubectl get certificates -A
kubectl get certificaterequests -A
kubectl get secrets -A -o json | jq -r '.items[] | select(.type=="kubernetes.io/tls") | "\(.metadata.namespace)/\(.metadata.name)"' | \
  xargs -I{} sh -c 'echo {} ; kubectl get secret {} -o jsonpath="{.data.tls\.crt}" | base64 -d | openssl x509 -noout -dates'
```
### 3.2 扩缩容操作

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动水平扩容（常用于预演或大促前的预热）
kubectl scale deployment/app --replicas=10 -n production

# 查看 HPA 指标与建议
kubectl describe hpa app-hpa -n production

# 垂直扩容（VPA 推荐模式，生产建议使用 Off 或 Initial 模式，避免自动重启）
kubectl get vpa app-vpa -n production -o jsonpath='{.status.recommendation.containerRecommendations}'

# 检查节点池容量是否充足
kubectl describe nodes | grep -A5 "Allocated resources"
```
### 3.3 发布与回滚

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 发布：使用 --wait 等待就绪，--atomic 失败自动回滚
helm upgrade --install app ./helm-chart \
  -n production \
  -f values-prod.yaml \
  --set image.tag=v1.2.3 \
  --wait --timeout 600s --atomic

# 查看发布历史
helm history app -n production

# 紧急回滚到上一版本
helm rollback app 0 -n production

# 指定版本回滚
helm rollback app <revision> -n production

# ArgoCD 回滚
argocd app rollback app <revision>

# 查看滚动更新进度
kubectl rollout status deployment/app -n production
kubectl rollout history deployment/app -n production
```
### 3.4 日志与链路查询

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 多 Pod 聚合日志（ stern ）
stern app -n production --since 10m --template '{{.PodName}} {{.Message}}'

# 多容器 Pod 日志
kubectl logs <pod> -n production --all-containers --prefix

# Loki 查询示例：ERROR 级别日志
logcli query '{namespace="production", app="app"} |= "ERROR"' --since=1h

# Loki 查询示例：特定 Trace ID
logcli query '{namespace="production", app="app"} |= "trace_id=abc123"' --since=1h

# Jaeger 查询示例
# 访问 jaeger-ui / search service=app, operation=PlaceOrder
# 或通过 jaeger-cli
jaeger-query --service app --operation PlaceOrder --lookback 1h
```
### 3.5 云厂商节点池巡检（阿里云 ACK 示例）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池与自动伸缩状态
aliyun cs GET /clusters/<cluster-id>/nodes
aliyun cs GET /clusters/<cluster-id>/nodepools

# 检查集群 autoscaler 事件
kubectl get events -n kube-system --field-selector reason=TriggeredScaleUp
kubectl get events -n kube-system --field-selector reason=FailedToScaleUpGroup

# AWS EKS 节点组检查示例
aws eks describe-nodegroup --cluster-name <cluster> --nodegroup-name <ng>
aws autoscaling describe-scaling-activities --auto-scaling-group-name <asg>

# GKE 节点池检查示例
gcloud container node-pools list --cluster <cluster> --zone <zone>
gcloud container clusters describe <cluster> --zone <zone> --format "value(autoscaling)"
```
---

## 4. 故障排查速查

故障排查遵循**先状态、后事件、再日志、最后链路**的顺序。下表覆盖应用架构模式中最常见的高频故障，SRE 可直接按表执行。

| 现象 | 可能根因 | 确认命令 | 修复动作 |
|---|---|---|---|
| Pod 处于 `CrashLoopBackOff` | 启动依赖缺失、配置错误、健康探针失败 | `kubectl logs <pod> -n <ns> --previous`<br>`kubectl describe pod <pod> -n <ns>` | 修复应用代码/配置；调整探针；检查 Init Container |
| Pod 处于 `Pending` | 资源不足、污点容忍、PVC 未绑定、亲和性无法满足 | `kubectl describe pod <pod> -n <ns>`<br>`kubectl get nodes -l <label>` | 扩容节点池；调整 requests；检查 StorageClass |
| Pod 处于 `ImagePullBackOff` | 镜像不存在、镜像仓库鉴权失败、网络不通 | `kubectl describe pod <pod> -n <ns>`<br>`kubectl get secret regcred -n <ns>` | 确认镜像 tag；更新 imagePullSecret；检查仓库网络 |
| Pod `OOMKilled` | 内存 limits 过小或内存泄漏 | `kubectl describe pod <pod> -n <ns>`<br>`kubectl top pod <pod> -n <ns>` | 优化应用内存；提高 limits；引入 VPA |
| Pod `Evicted` | 节点磁盘/内存压力、临时存储超过 limit | `kubectl describe pod <pod> -n <ns>`<br>`kubectl describe node <node>` | 清理节点临时文件；调整 emptyDir 大小限制；驱逐污点处理 |
| HPA 不扩容 | 未配置 metrics-server、指标未暴露、已达到 maxReplicas | `kubectl describe hpa <hpa> -n <ns>`<br>`kubectl get --raw /apis/metrics.k8s.io/` | 配置自定义指标；调整 HPA 阈值；提高 maxReplicas |
| 服务 5xx 突增 | 下游依赖超时、配置漂移、滚动更新异常 | `kubectl get pods -n <ns>`<br>查询 RED 指标<br>`stern <app> -n <ns> --since 5m` | 触发回滚；限流降级；检查下游健康状态 |
| 服务 P99 延迟升高 | 慢 SQL、下游调用超时、GC 停顿、网络拥塞 | `kubectl logs <pod> -n <ns>`<br>查询链路追踪<br>`kubectl top pod <pod> -n <ns>` | 优化慢查询；调整超时与重试；扩容或优化 JVM |
| PVC 无法绑定 | StorageClass 未配置、Provisioner 异常、容量不足 | `kubectl describe pvc <pvc> -n <ns>`<br>`kubectl get storageclass` | 修复 CSI driver；调整 StorageClass；扩容后端存储 |
| StatefulSet 副本不同步 | 网络分区、存储性能不足、副本选举异常 | `kubectl logs <pod> -n <ns>`<br>`kubectl get pvc -n <ns>` | 检查 StatefulSet 启动顺序；验证 PVC 性能；手动触发 Failover |
| Ingress 返回 502/503 | 后端 Pod 未就绪、Endpoint 为空、健康检查失败 | `kubectl get endpoints <svc> -n <ns>`<br>`kubectl get pods -n <ns> -l app=<app>` | 修复 readinessProbe；检查 Service Selector；重启异常 Pod |
| 证书过期导致 TLS 握手失败 | cert-manager 未续期、Challenge 失败 | `kubectl describe certificate <cert> -n <ns>`<br>`kubectl get certificaterequests -n <ns>` | 检查 DNS01/HTTP01 Challenge；手动更新 Secret；修复 Issuer |

**排查顺序速记**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 状态
kubectl get pods -n <ns> -l app=<app>
# 2. 事件
kubectl describe pod <pod> -n <ns>
# 3. 日志
kubectl logs <pod> -n <ns> --previous
# 4. 指标
kubectl top pod <pod> -n <ns>
# 5. 链路
# 在 Jaeger/Grafana Tempo 中按 trace_id 查询
```
---

## 5. 与其他域的协作边界

Application Patterns 位于业务架构与平台能力的交汇点，生产就绪工作必须明确与其他 Domain 的职责边界，避免重复造轮子或遗漏关键点。本域的核心交付物是**面向应用架构的运行手册与检查清单**，而平台能力（网络、安全、可观测性、发布系统）由对应 Domain 负责建设与维护。

例如，当电商大促需要扩容时：本域提供应用级 HPA 配置与容量模型；[[domain-02-workloads-applications/README.md|domain-02]] 确认工作负载选型；[[domain-03-networking-traffic/README.md|domain-03]] 处理入口限流与灰度路由；[[domain-09-reliability-engineering/README.md|domain-09]] 评估全链路 SLO 与降级预案；[[domain-11-production-operations/README.md|domain-11]] 统一值班响应与变更窗口。

| 相关 Domain | 本域职责 | 协作边界 / 引用 |
|---|---|---|
| [[domain-02-workloads-applications/README.md|domain-02-workloads-applications]] | 工作负载类型选型与生命周期策略 | 本域决定**何时使用 StatefulSet/Deployment/Job**，该域提供**工作负载深度配置与运行时最佳实践**。 |
| [[domain-03-networking-traffic/README.md|domain-03-networking-traffic]] | 服务暴露、灰度路由、入口超时重试 | 本域定义应用级路由与熔断需求，该域提供 Service Mesh、Ingress、Gateway API 实现。 |
| [[domain-05-security-compliance/README.md|domain-05-security-compliance]] | 应用层安全上下文、NetworkPolicy、Secret 管理 | 本域落实 Pod Security、最小权限、密钥注入；该域制定整体零信任与合规框架。 |
| [[domain-06-observability/README.md|domain-06-observability]] | RED 指标、日志、链路、SLO 定义 | 本域输出业务语义指标与告警规则；该域提供可观测平台、存储与告警路由。 |
| [[domain-08-release-change-management/README.md|domain-08-release-change-management]] | 应用发布策略、回滚、配置版本化 | 本域执行 Helm/ArgoCD 发布；该域制定 GitOps 规范、变更窗口与审批流。 |
| [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] | 应用级高可用、PDB、多活/灾备模式 | 本域实现 Pod 级与 Namespace 级韧性；该域负责集群级/多集群 DR、混沌工程与 SLO 治理。 |
| [[domain-11-production-operations/README.md|domain-11-production-operations]] | 生产 checklist、值班响应、运行手册 | 本域提供应用专属运行手册；该域统一日常巡检、值班升级与事件管理。 |

---

## 6. 推荐阅读

以下文档按使用场景分类。若需了解具体行业架构（如电商秒杀、小程序 Serverless、金融支付隔离），优先阅读本域专题；若需深入平台能力（网络、安全、可观测性、GitOps），跳转至对应 Domain。

### 本域核心参考

- [[domain-20-application-patterns/README.md|Application Patterns 目录]]
- [[domain-20-application-patterns/行业架构/README.md|应用层架构设计最佳实践]]
- [[domain-20-application-patterns/行业架构/01-ecommerce-architecture.md|电商系统 Kubernetes 生产架构设计]]
- [[domain-20-application-patterns/行业架构/02-mini-program-architecture.md|小程序平台 Kubernetes 生产架构设计]]
- [[domain-20-application-patterns/行业架构/06-fintech-architecture.md|金融科技 FinTech Kubernetes 生产架构设计]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-20-application-patterns/topic-application-architecture/06-saas-multitenant-architecture|SaaS 多租户架构设计]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-20-application-patterns/topic-application-architecture/09-microservice-governance-architecture|微服务治理架构设计]]

### 计划补充的生产模式专题（参考 Gap Analysis）

- `topic-production-patterns/pod-availability-lifecycle.md` — PDB、探针、优雅停机
- `topic-production-patterns/resource-qos-rightsizing.md` — requests/limits、QoS、VPA
- `topic-production-patterns/scheduling-topology-patterns.md` — 拓扑分布、亲和性、Spot 节点
- `topic-production-patterns/stateful-app-patterns.md` — StatefulSet、PVC 快照、备份恢复
- `topic-production-patterns/application-runbooks.md` — CrashLoopBackOff、OOMKilled、Ingress 5xx

### 相关域推荐

- [[domain-02-workloads-applications/README.md|domain-02-workloads-applications]] — 工作负载选型与生命周期
- [[domain-03-networking-traffic/README.md|domain-03-networking-traffic]] — 服务网格与流量治理
- [[domain-05-security-compliance/README.md|domain-05-security-compliance]] — 安全合规与零信任
- [[domain-06-observability/README.md|domain-06-observability]] — 可观测性与 SLO 体系
- [[domain-08-release-change-management/README.md|domain-08-release-change-management]] — GitOps 与发布变更
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] — 可靠性工程与灾备
- [[domain-11-production-operations/README.md|domain-11-production-operations]] — 生产运维与值班手册

---

*维护者*: KUDIG SRE Team | *许可证*: MIT | *最后更新*: 2026-07-01


<!-- risk-assessed -->
