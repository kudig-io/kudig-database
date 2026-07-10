---
title: Manifests & Patterns 生产就绪运维指南
description: 面向 SRE 的 Kubernetes 清单与模式生产化检查、风险缓解及日常运维手册
summary: 面向 SRE 的 Kubernetes 清单与模式生产化检查、风险缓解及日常运维手册
category: yaml-manifests
tags:
- production
- best-practices
- yaml
- manifests
- patterns
- operations
- gitops
- autoscaling
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
- Manifests & Patterns 生产就绪运维指南是什么
- 如何按生产环境要求运维 Kubernetes 清单与模式
trigger_keywords:
- 生产就绪
- 运维指南
- manifests
- patterns
- gitops
- 清单生产化
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
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


# Manifests & Patterns 生产就绪运维指南

> **适用版本**: Kubernetes 1.28 - 1.33 | **最后更新**: 2026-07 | **难度**: 高级

本指南聚焦 Kubernetes 清单（Manifests）与部署模式（Patterns）在生产环境中的落地要求，覆盖 YAML 规范化、GitOps 交付、弹性伸缩、安全策略、可观测性埋点与成本优化等核心主题。生产集群与测试环境最大的差异在于：任何微小的清单缺陷都会在多租户、大规模、高并发的场景下被放大为事故。因此，SRE 必须在上线前建立一套可重复的检查与运维动作，而不是依赖人工逐行审查。

本指南适用于平台工程师制定基线规范、SRE 进行上线前评审（PRR），以及值班人员在日常巡检中快速定位清单层面的隐患。

---

## 1. 生产环境检查清单

在将一套清单或模式声明为生产就绪之前，建议逐项确认以下检查点。清单中的每一项都对应着真实生产环境中高频出现的故障模式，缺失任意一项都可能在升级、扩缩容或节点维护时触发连锁反应。

1. **版本与 API 兼容性**：所有资源使用当前集群支持的稳定 API 版本，避免已废弃的 `extensions/v1beta1`、`policy/v1beta1` PodSecurityPolicy、`batch/v1beta1` CronJob 等。建议在新版本集群上线前运行 `pluto` 或 `kubectl deprecations` 扫描，并在 CI 中固定目标版本。
   ```bash
   kubectl get --all-namespaces deploy,ing,pdb -o json | jq '.items[].apiVersion' | sort | uniq -c
   pluto detect-all-in-cluster --target-versions k8s=v1.33
   ```

2. **资源请求与限制已配置**：每个容器必须声明 `requests` 与 `limits`，关键服务应设置 QoS 为 Guaranteed 或 Burstable，并确保 requests ≤ limits。未设置 requests 的 Pod 在节点资源紧张时会被优先驱逐，未设置 limits 的 Pod 则可能耗尽节点资源导致节点 NotReady。
   ```bash
   kubectl get pods --all-namespaces -o json | jq '.items[].spec.containers[] | select(.resources.requests == null) | .name'
   kubectl resource-capacity --pods --util --sort cpuutil
   ```

3. **健康探针完整且合理**：`livenessProbe`、`readinessProbe`、`startupProbe` 必须覆盖关键路径，探针阈值应经过压测验证。需要特别避免的是：readiness 探针依赖外部数据库，当数据库抖动时导致所有 Pod 从 Service Endpoint 摘除，引发级联故障。建议探针路径独立设计，readiness 仅反映自身可服务状态。

4. **PDB 保护关键工作负载**：对多副本 StatefulSet、Deployment 配置 `PodDisruptionBudget`，确保主动驱逐（如节点维护、spot 回收）时最小可用副本数不跌破 SLO。单副本且无特殊要求的服务可豁免，但必须在文档中明确说明。

5. **网络策略默认拒绝**：命名空间内启用 default-deny NetworkPolicy，仅显式放行业务流量，禁止 Pod 间无差别互通。生产环境中常见的错误是只放行入站流量却忘记放行 DNS，导致所有服务无法解析域名。

6. **安全上下文最小权限**：容器以非 root 运行，`readOnlyRootFilesystem: true`，禁用不必要的特权，遵循 Pod Security Standards 的 Restricted 级别。对于必须挂载可写层的应用，使用 `emptyDir` 作为临时目录而不是开放 root 文件系统写入。

7. **Secret 与配置解耦**：敏感信息仅通过 Secret 注入，禁止在 ConfigMap 或环境变量中硬编码。GitOps 场景下必须使用 Sealed Secrets、External Secrets Operator 或 SOPS 加密后再提交到仓库，避免仓库泄漏导致凭证泄露。

8. **HPA/VPA 与资源对齐**：HPA 指标目标值与 requests 成比例，避免扩容后因为 limits 不足触发 OOM。VPA 推荐仅在非核心批处理或开发环境启用 Auto 模式，生产核心服务建议先用 Off 模式观察推荐值，再手动调整 requests。

9. **可观测性三要素已埋点**：每个服务暴露 `/metrics`、具备结构化日志、携带 OpenTelemetry trace context；Prometheus ServiceMonitor、Fluent Bit/OTel Collector 配置纳入基线清单。没有可观测性埋点的服务上线后，一旦出现异常将难以定位根因。

10. **备份与回滚策略就绪**：有状态工作负载配置 Velero 备份、PVC 快照或应用级备份；Deployment/Helm release 保留最近 10 个版本并记录回滚命令。建议每次重大变更前手动记录当前 revision，便于快速回滚。

11. **ResourceQuota 与 LimitRange 已生效**：多租户命名空间配置配额，防止单租户耗尽节点资源、PVC 存储或 IP 池。LimitRange 可以强制默认资源请求，避免开发者遗漏 requests 导致调度不可控。

12. **清单经过 CI 校验**：流水线中集成 `kubeconform`、`kube-linter`、`helm lint`、`kyverno test`，禁止未通过校验的变更进入生产分支。建议将校验脚本统一封装为 Makefile 或 CI reusable workflow，确保各仓库执行标准一致。

---

## 2. 关键风险与缓解措施

### 2.1 GitOps 漂移与误同步

**风险**：ArgoCD/Flux 自动同步覆盖手工救急修改，或 Application 配置错误导致全量命名空间被删。生产环境中最危险的场景之一是：运维人员在值班时手工修改了 Deployment 镜像进行止血，随后 GitOps 自动同步将变更回滚，导致故障复发。

**缓解措施**：
- 生产 Application 关闭自动 prune 与 selfHeal，关键变更走人工同步或批准流程：
  ```yaml
  syncPolicy:
    automated:
      prune: false
      selfHeal: false
  ```
- 使用 AppProject 限制可部署的源仓库、目标集群与命名空间白名单，防止误部署到生产集群。
- 配置 ArgoCD notifications 与 PrometheusRule 告警同步失败事件，并在值班手册中明确同步失败的响应流程：
  ```yaml
  groups:
    - name: argocd
      rules:
        - alert: ArgoCDAppSyncFailed
          expr: argocd_app_sync_total{phase!~"Succeeded"} > 0
          for: 5m
          labels:
            severity: warning
        - alert: ArgoCDAppUnhealthy
          expr: argocd_app_info{health_status!~"Healthy"} == 1
          for: 10m
          labels:
            severity: critical
  ```

### 2.2 自动扩缩容引发雪崩

**风险**：HPA 扩容过快可能在数秒内耗尽节点资源、IP 池或触发云厂商配额；缩容过慢导致成本失控；缩容过急则会中断长连接，影响在线用户体验。特别是在大促或突发流量场景下，未经调优的 HPA 很容易成为新的故障源。

**缓解措施**：
- HPA 设置 `behavior.scaleUp.stabilizationWindowSeconds` 与 `scaleDown` 策略，限制每秒副本变化比例，避免一次性扩容过多副本：
  ```yaml
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Percent
          value: 50
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
  ```
- 节点池配置 cluster-autoscaler/Karpenter 扩容上限，避免无限制节点创建导致成本失控或云配额耗尽。
- 对长连接服务使用 `terminationGracePeriodSeconds` 配合 `preStop` sleep，确保连接优雅释放：
  ```yaml
  lifecycle:
    preStop:
      exec:
        command: ["/bin/sh", "-c", "sleep 15"]
  ```

### 2.3 特权清单导致容器逃逸

**风险**：`privileged: true`、`hostPID: true`、`hostPath` 挂载、`allowPrivilegeEscalation: true` 等配置被滥用时，攻击者可能从容器逃逸到宿主机，进而横向移动控制整个集群。许多 DevOps 团队为了调试方便会临时开启特权，但事后忘记回滚，留下长期隐患。

**缓解措施**：
- 通过 Kyverno/OPA Gatekeeper 强制禁止特权容器，并定期审计现有清单：
  ```bash
  kubectl get pods --all-namespaces -o json | \
    jq '.items[] | {ns: .metadata.namespace, pod: .metadata.name, privileged: [.spec.containers[] | select(.securityContext.privileged == true) | .name]} | select(.privileged | length > 0)'
  ```
- 使用 Pod Security Admission 将命名空间标记为 `enforce: restricted`：
  ```bash
  kubectl label ns <namespace> pod-security.kubernetes.io/enforce=restricted
  ```
- 参考 [[domain-05-security-compliance/策略治理/06-pod-security-standards.md|Pod Security Standards]] 配置基线策略，并在 CI 中集成策略校验。

### 2.4 证书与镜像签名缺失

**风险**：Ingress TLS 证书过期会导致用户无法访问、搜索引擎降权；镜像未签名则可能在供应链环节被替换为恶意镜像，特别是在公共镜像仓库或代理镜像仓库场景下风险更高。

**缓解措施**：
- cert-manager 自动管理证书，并设置过期前 30 天告警：
  ```bash
  kubectl get certificates --all-namespaces -o json | jq '.items[].status.conditions'
  kubectl get certificaterequests,orders,challenges -A
  ```
- 启用 cosign/sigstore 镜像签名验证，通过 admission webhook 或 OPA/Kyverno 策略拒绝未签名镜像：
  ```yaml
  policies:
    - name: verify-image-signature
      rules:
        - name: check-cosign-signature
          match:
            resources:
              kinds:
                - Pod
          verifyImages:
            - imageReferences:
                - "registry.example.com/*"
              attestors:
                - entries:
                    - keys:
                        publicKeys: "-----BEGIN PUBLIC KEY-----..."
  ```

### 2.5 跨集群/多云模式缺乏一致性

**风险**：多集群场景下同一应用在不同集群的清单版本、配置参数、网络策略、资源配额不一致，会导致故障难以定位、版本回滚困难，甚至出现在 A 集群正常而在 B 集群异常的情况。

**缓解措施**：
- 使用 Kustomize overlay 或 Helm values 文件按环境分层，基线层（base）由平台团队统一维护，仅允许业务团队修改明确开放的参数。
- ApplicationSet 管理多集群分发，结合 `generators.clusters` 与 `syncPolicy` 实现一致交付：
  ```yaml
  apiVersion: argoproj.io/v1alpha1
  kind: ApplicationSet
  spec:
    generators:
      - clusters:
          selector:
            matchLabels:
              env: production
  ```
- 建立清单版本发布机制，任何基线变更需经过 staging 环境验证后再推广到全部生产集群。

---

## 3. 日常运维操作

### 3.1 清单基线审计

建议每周执行一次清单基线审计，并将结果归档到变更管理系统中。审计的核心目标是发现"配置漂移"：即集群实际状态与基线清单之间的差异。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 扫描全集群废弃 API
pluto detect-all-in-cluster --target-versions k8s=v1.33

# 2. 检查未设置资源限制的 Pod
kubectl get pods --all-namespaces -o json | \
  jq '.items[] | select(.spec.containers[].resources.limits == null) | \(.metadata.namespace)/\(.metadata.name)'

# 3. 检查未挂载 PDB 的高可用工作负载
kubectl get deploy,sts --all-namespaces -o json | \
  jq -r '.items[] | select(.spec.replicas > 1) | \(.metadata.namespace)/\(.metadata.name)' | \
  while read res; do
    ns=$(echo $res | cut -d/ -f1)
    name=$(echo $res | cut -d/ -f2)
    kubectl get pdb -n $ns -o json | jq -e ".items[].spec.selector.matchLabels == $(kubectl get deploy/$name -n $ns -o json | jq '.spec.selector.matchLabels')" >/dev/null || echo "Missing PDB: $res"
  done

# 4. 检查无标签或标签不规范的资源
kubectl get all --all-namespaces --show-labels | grep -v "app.kubernetes.io/name"
```
### 3.2 GitOps 交付检查

GitOps 交付检查应在每次发布前后执行，重点关注同步状态、健康状态和事件日志。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ArgoCD：查看应用同步状态与最近事件
argocd app list
argocd app get <app-name> --show-operation
kubectl get events -n argocd --field-selector reason=SyncFailed

# Flux：查看 Kustomization/HelmRelease 状态
flux get kustomizations --all-namespaces
flux get helmreleases --all-namespaces
flux logs --level=error --since=1h

# 检查是否有应用处于 Unknown 或 Missing 状态
kubectl get applications -A -o json | jq '.items[] | select(.status.health.status != "Healthy") | \(.metadata.namespace)/\(.metadata.name): \(.status.health.status)'
```
### 3.3 弹性伸缩调优

弹性伸缩调优需要结合业务流量特征进行，不能简单套用默认参数。建议每月回顾一次 HPA/VPA 事件和扩缩容曲线。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 HPA 当前状态与事件
kubectl get hpa -A
kubectl describe hpa <name> -n <ns>

# 验证 VPA 推荐值（推荐 mode: Off 先观察）
kubectl get vpa <name> -n <ns> -o json | jq '.status.recommendation.containerRecommendations'

# 检查 Karpenter NodePool 资源水位
kubectl get nodepool
kubectl get machines,karpenter.sh

# 查看 cluster-autoscaler 扩缩容事件
kubectl get events --all-namespaces --field-selector reason=TriggeredScaleUp,reason=ScaleDown
```
### 3.4 安全策略巡检

安全策略巡检应纳入每日或每周自动化任务，发现违规清单后通过工单系统通知负责人限期整改。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出特权 Pod
kubectl get pods --all-namespaces -o json | \
  jq '.items[] | {ns: .metadata.namespace, pod: .metadata.name, containers: [.spec.containers[] | select(.securityContext.privileged == true) | .name]} | select(.containers | length > 0)'

# 检查 hostPath / hostPID / hostNetwork 使用
kubectl get pods --all-namespaces -o json | \
  jq '.items[] | select(.spec.hostPID or .spec.hostNetwork or (.spec.volumes[]? | select(.hostPath))) | \(.metadata.namespace)/\(.metadata.name)'

# 检查 Kyverno/OPA 策略执行结果
kubectl get policyreports.wgpolicyk8s.io -A
kubectl get clusterpolicyreports.wgpolicyk8s.io -A
```
### 3.5 成本与资源优化

成本优化是持续过程，核心思路是"按需分配、及时回收"。建议每月生成资源利用率报告，识别长期低利用率的工作负载。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看各命名空间资源使用与请求偏差
kubectl top pods --all-namespaces
kubectl resource-capacity --pods --util --sort cpuutil

# 检查闲置 ConfigMap/Secret/PVC
kubectl get configmaps,secrets,pvc --all-namespaces --field-selector metadata.annotations."value"=unused

# 识别低利用率 Deployment（需要 kube-capacity 或自定义脚本）
kubectl top pods -A | awk '{print $1, $2, $3}' | sort -k3 -n | head -20
```
### 3.6 升级前清单预检

集群升级前应执行清单预检，避免升级后因 API 废弃或行为变更导致应用异常。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查目标版本不兼容的 API
pluto detect-all-in-cluster --target-versions k8s=v1.33

# 检查 admission webhook 是否依赖已废弃的 API
kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations -o yaml | grep -B2 "apiVersions:.*v1beta1"

# 检查 CRD 版本兼容性
kubectl get crd -o json | jq '.items[].spec.versions[].name'
```
---

## 4. 故障排查速查

以下速查表覆盖清单与模式相关的高频故障，建议打印或保存到值班手册中。

| 现象 | 可能原因 | 确认命令 | 修复方向 |
|---|---|---|---|
| ArgoCD 应用长期处于 `OutOfSync` | 仓库清单与集群实际状态不一致，或存在手工修改 | `argocd app diff <app>` | 回滚手工修改或强制同步；检查 `.metadata.finalizers` 残留 |
| Flux Kustomization  stuck in `Ready=False` | Git 仓库不可达、路径错误或依赖 CRD 缺失 | `flux get kustomizations -A` `flux logs --level=error` | 检查 source 连接、路径、CRD 安装情况 |
| HPA 无法获取自定义指标 | Prometheus Adapter 配置缺失或 ServiceMonitor 标签不匹配 | `kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1` | 修正 adapter rule 或 ServiceMonitor selector |
| HPA 频繁扩缩容（flapping） | 目标值设置过低或稳定窗口过短 | `kubectl describe hpa <name>` | 提高 target 阈值或增大 stabilizationWindowSeconds |
| Pod 反复 OOMKilled | limits 设置过低或内存泄漏 | `kubectl describe pod <pod>` | 上调 limits；结合 VPA 推荐值；排查应用内存 |
| Pod 持续 CrashLoopBackOff | 启动命令错误、依赖服务未就绪或配置错误 | `kubectl logs <pod> --previous` | 检查 entrypoint、env、ConfigMap/Secret 内容 |
| NetworkPolicy 误拦截业务流量 | 未放行 DNS/CoreDNS 或未允许必要端口 | `kubectl run netshoot --rm -it --image nicolaka/netshoot -- /bin/bash` 后 `nc -vz` | 补充 allow-dns 与业务端口规则 |
| 跨命名空间服务访问失败 | NetworkPolicy 仅放行同命名空间流量 | `kubectl get networkpolicies -A` | 添加显式 cross-namespace ingress/egress 规则 |
| PDB 导致节点 drain 卡住 | PDB minAvailable 等于副本数或单副本 StatefulSet | `kubectl get pdb -n <ns>` 与 `kubectl get events` | 临时调大 `maxUnavailable` 或确认维护窗口 |
| Secret 注入失败 | 引用的 Secret 不存在或 key 名称错误 | `kubectl get events --field-selector reason=FailedMount` | 创建缺失 Secret；检查 volumeMount 与 subPath |
| ConfigMap 热更新未生效 | 应用未监听文件变化或挂载为 subPath | `kubectl exec <pod> -- cat /etc/config/key` | 避免 subPath 挂载需热更新的文件；触发滚动更新 |
| 证书过期导致 Ingress 503/证书错误 | cert-manager 未续期或 Issuer 异常 | `kubectl get certificate,certificaterequest,order -A` | 删除 Certificate 触发重签；检查 DNS-01/HTTP-01 挑战 |
| Helm release 状态为 failed | 前置 Job/CRD 未就绪或 values 冲突 | `helm history <release> -n <ns>` `helm status <release> -n <ns>` | `helm rollback <release> <rev> -n <ns>` 或卸载重装 |
| 镜像拉取失败 ImagePullBackOff | 镜像不存在、仓库认证失败或网络策略拦截 | `kubectl describe pod <pod>` `kubectl get secret regcred -n <ns>` | 确认镜像 tag、创建/更新 imagePullSecret、放行网络 |
| Pod 调度失败 Pending | 资源不足、节点选择器冲突、污点未容忍或 PVC 未绑定 | `kubectl describe node <node>` `kubectl get pvc -A` | 扩容节点池、调整亲和性/污点、检查 StorageClass |
| CRD 缺失导致资源无法创建 | Helm chart 未安装 CRD 或 CRD 版本不兼容 | `kubectl get crd <name>` | 安装对应 CRD；升级 operator/CRD 到兼容版本 |
| Service 无 Endpoint | selector 标签不匹配或 Pod 未就绪 | `kubectl get endpoints <svc> -n <ns>` `kubectl get pods -n <ns> --show-labels` | 修正 label selector 或 readiness probe |
| Job/CronJob 重复执行或死锁 | startingDeadlineSeconds、concurrencyPolicy 配置不当 | `kubectl get cronjobs -A` `kubectl get jobs -A` | 设置 `concurrencyPolicy: Forbid` 或 `Replace`，配置 backoffLimit |

---

## 5. 与其他域的协作边界

- **domain-08-release-change-management**：GitOps 流水线、ArgoCD/Flux 部署策略、Helm 发布与回滚由该域主导；本域聚焦清单层面的模板、Overlay 与基线模式，二者在 `ApplicationSet`、`values.yaml` 分层、sealed secrets 等点交叉。平台团队应负责定义基线模式，业务团队负责应用层 values 与 overlay。
- **domain-05-security-compliance**：Pod Security Standards、NetworkPolicy、RBAC、Secret 加密、镜像签名等安全策略在本域以 YAML 形式落地；策略引擎（Kyverno/OPA）的选型、治理规则与审计流程由安全域制定。本域执行安全域的规范，安全域审计本域的合规性。
- **domain-06-observability**：ServiceMonitor、PrometheusRule、OTel Collector、日志采集 DaemonSet 等可观测性清单属于本域；指标命名规范、SLO/SLI 定义、告警分级与路由规则由可观测性域负责。本域确保每个工作负载携带必要的可观测性注解与端口暴露。
- **domain-09-reliability-engineering**：PDB、拓扑分布约束、多可用区部署、备份/恢复模式在本域实现；DR 场景设计、RTO/RPO 目标、混沌工程与容量测试由可靠性域主导。本域按可靠性域目标配置清单层面的高可用与备份策略。
- **domain-11-production-operations**：FinOps、容量规划、日常巡检、事件响应流程由生产运维域定义；本域提供资源优化、配额管理、自动伸缩等清单实现，并按运维域要求输出审计数据与报表。
- **domain-02-workloads-applications**：工作负载类型（Deployment/StatefulSet/DaemonSet/Job）的具体配置、生命周期管理与业务场景最佳实践由该域深入；本域关注这些配置在平台级复用与生产化封装，例如将业务域的最佳实践固化为 Kustomize base 或 Helm library chart。
- **domain-04-storage-data**：PVC、StorageClass、VolumeSnapshot 等存储清单由本域引用与封装；存储后端选型、性能调优与灾难恢复由存储域负责。本域需确保有状态应用的清单正确声明存储类、访问模式与备份策略。
- **domain-03-networking-traffic**：Ingress、Gateway API、Service Mesh 等网络清单由本域定义；CNI、负载均衡、DNS、服务网格控制平面由网络域运维。本域需遵循网络域制定的命名空间隔离、TLS 终止与流量治理规范。

---

## 6. 推荐阅读

### 同域资料

- [[domain-18-manifests-patterns/YAML参考/36-ecosystem-kustomize-helm-argocd.md|Kustomize / Helm / ArgoCD YAML 配置参考]]
- [[domain-18-manifests-patterns/YAML参考/27-hpa-autoscaling-v2.md|HorizontalPodAutoscaler v2 YAML 配置参考]]
- [[domain-18-manifests-patterns/YAML参考/22-networkpolicy-reference.md|NetworkPolicy 配置参考]]
- [[domain-18-manifests-patterns/YAML参考/23-pod-security-standards.md|Pod Security Standards 配置参考]]
- [[domain-18-manifests-patterns/YAML参考/28-poddisruptionbudget-reference.md|PodDisruptionBudget 配置参考]]

### 相关域资料

- [[domain-08-release-change-management/GitOps/99-argo-cd-gitops-guide.md|ArgoCD GitOps 指南]]
- [[domain-08-release-change-management/GitOps/99-helm-production-guide.md|Helm 生产化指南]]
- [[domain-05-security-compliance/策略治理/99-kyverno-policy-guide.md|Kyverno 策略指南]]
- [[domain-05-security-compliance/策略治理/99-opa-gatekeeper-policy-guide.md|OPA Gatekeeper 策略指南]]
- [[domain-06-observability/指标/10-monitoring-metrics-prometheus.md|Prometheus 监控指标实践]]
- [[domain-09-reliability-engineering/03-slo-sli-guide.md|SLO/SLI 指南]]
- [[domain-11-production-operations/01-production-sre-daily-ops.md|SRE 日常运维手册]]
- [[domain-02-workloads-applications/核心工作负载/12-advanced-pod-patterns.md|高级 Pod 模式]]

---

*本指南为 domain-18-manifests-patterns 的生产就绪入口文档，建议每季度结合集群升级与业务变更进行一次基线复核。*


<!-- risk-assessed -->
