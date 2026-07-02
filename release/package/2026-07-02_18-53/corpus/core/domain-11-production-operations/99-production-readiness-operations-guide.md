---
title: 生产运维域生产就绪运维指南
description: 面向 Kubernetes 生产运维域（domain-11）的生产就绪门控与持续运维手册，覆盖检查清单、风险缓解、日常操作、故障速查与跨域协作边界。
summary: 面向 Kubernetes 生产运维域的生产就绪门控与持续运维手册，覆盖检查清单、风险缓解、日常操作、故障速查与跨域协作边界。
category: production-operations
tags:
- production
- best-practices
- operations
- production-operations
- finops
- governance
- incident-response
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
- 生产运维域生产就绪运维指南是什么
- 如何按生产环境要求运维 Kubernetes 生产运维域
trigger_keywords:
- 生产就绪
- 运维指南
- 生产运维
- FinOps
- 事件响应
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


# 生产运维域生产就绪运维指南

本指南为 **domain-11-production-operations** 提供生产就绪（Production Readiness）门控框架与持续运维动作集，适用于集群上线前评审、重大变更前的状态确认，以及日常 SRE 值班。生产就绪不是一次性验收，而是贯穿容量、成本、安全、可观测性与事件响应的持续状态管理。它不会替代 [[domain-11-production-operations/01-production-sre-daily-ops.md|日常巡检与值班手册]] 或 [[domain-11-production-operations/02-change-management-guide.md|变更管理指南]]，而是将这些实践汇总为可执行的生产就绪标准，并在 gap 分析推荐的专题缺失时给出最小可行方案。

## 1. 生产环境检查清单

在将生产运维域的服务或集群宣布为“生产就绪”前，必须逐项确认以下 12 项。任何未通过项都应记录在 readiness tracker 中，并在上线前修复或取得风险接受（risk acceptance）。

| 检查项 | 验证命令/配置 | 通过标准 |
|---|---|---|
| 1. 节点与计算余量 | `kubectl top nodes` / `kubectl describe nodes` | CPU 峰值 < 70%，内存 < 80%，保留 20% 突发余量 |
| 2. 命名空间资源配额 | `kubectl get resourcequota -A` | 所有业务命名空间已配置 ResourceQuota 与 LimitRange |
| 3. 关键服务 PDB | `kubectl get pdb -A` | 核心工作负载 100% 配置 PodDisruptionBudget，minAvailable ≥ 1 |
| 4. 证书有效期 | `kubeadm certs check-expiration` / `kubectl get certificates -A` | 所有控制面与入口证书剩余有效期 > 30 天 |
| 5. 备份可恢复性 | `etcdctl snapshot status` / `velero backup get` | 最近 24h 内 etcd 与关键命名空间备份成功，且恢复演练通过 |
| 6. 监控与告警覆盖 | `kubectl get prometheusrules,alertmanagerconfigs -A` | 核心服务 RED/USE 指标、节点压力、证书过期均有可告警规则 |
| 7. SLO/SLI 基线 | 查看 Grafana SLO dashboard / Alertmanager 路由 | 关键服务已定义 SLI、SLO，错误预算消耗趋势可见 |
| 8. 安全基线 | `kubectl get clusterpolicies` / `kubectl auth can-i --list` | PSA/PSS 已启用，特权容器受限，RBAC 遵循最小权限 |
| 9. 审计与日志策略 | 检查 audit policy ConfigMap / 日志采集 DaemonSet | Kubernetes audit 已开启，日志保留 ≥ 30 天，可追溯管理员操作 |
| 10. FinOps 成本基线 | 云厂商账单标签 / `kubectl get nodes -L team,env,cost-center` | 节点与工作负载已打成本标签，月度预算告警已配置 |
| 11. 事件响应与值班 | 检查 on-call 排班表与 escalation 路径 | P0/P1 升级路径可达，事故 Commander 与通信模板已就绪 |
| 12. 变更回滚预案 | `helm history` / Git 版本记录 | 最近 3 个版本可回滚，回滚命令已在非生产环境验证 |

建议将上述检查清单固化为每次上线或重大变更前的 Readiness Review（PRR）会议议程，由 SRE、开发负责人与安全代表共同签字确认。任何未通过项都应记录在 readiness tracker 中，标注责任人、修复截止日期与风险接受级别。对于多集群或联邦场景，还需额外确认 多集群运维（待补充） 中定义的全局负载均衡、Secret 同步与舰队策略。

## 2. 关键风险与缓解措施

以下 5 项是生产运维域最常见的高影响风险，每项均给出可直接执行的命令或配置样例。

### 2.1 容量耗尽导致调度失败或服务雪崩

**风险**：业务突增或资源请求配置不合理时，节点池快速耗尽，Pod 进入 Pending，进而触发 HPA 循环扩容失败。

**缓解措施**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查节点池整体余量
kubectl top nodes
kubectl describe nodes | grep -A 5 "Allocated resources"

# 2. 识别过度请求的工作负载
kubectl top pods -A --sort-by=cpu
kubectl get pods -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,REQ_CPU:.spec.containers[*].resources.requests.cpu

# 3. 开启 cluster-autoscaler 并设置扩容优先级
helm upgrade cluster-autoscaler autoscaler/cluster-autoscaler \
  --set autoDiscovery.clusterName=<CLUSTER_NAME> \
  --set extraArgs.balance-similar-node-groups=true
```
同时建议将节点池目标利用率控制在 70% 以下，并对关键命名空间设置 `ResourceQuota`。对于不可预测的流量高峰，应提前在测试环境压测并验证 cluster-autoscaler 的扩容延迟，确保从 Pending 到 Ready 的时间满足业务容忍度。若使用抢占式实例或 Spot 节点，还需为关键工作负载配置反亲和性，避免同时被回收导致服务中断。

### 2.2 证书过期造成控制面或入口中断

**风险**：kubeadm 内部 CA、ingress TLS 或 cert-manager 证书过期未及时发现，导致 API server 或 HTTPS 入口不可用。

**缓解措施**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 kubeadm 证书
kubeadm certs check-expiration

# 检查 cert-manager 证书状态
kubectl get certificates -A
kubectl get certificaterequests -A
kubectl get challenges -A    # 确认 ACME 挑战无失败

# 设置 PrometheusRule 提前 30 天告警（示例）
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cert-expiry-alert
  namespace: monitoring
spec:
  groups:
  - name: certs
    rules:
    - alert: K8sCertificateExpiringSoon
      expr: |
        (
          apiserver_client_certificate_expiration_seconds_count{job="kubernetes-apiserver"} > 0
          and
          apiserver_client_certificate_expiration_seconds_sum{job="kubernetes-apiserver"}
          / apiserver_client_certificate_expiration_seconds_count{job="kubernetes-apiserver"} < 30 * 86400
        )
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Kubernetes 证书将在 30 天内过期"
EOF
```
### 2.3 配置漂移或未授权变更

**风险**：人工 `kubectl edit` 或节点本地修改导致 GitOps 仓库与实际状态不一致，回滚时丢失关键上下文。

**缓解措施**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 kubectl diff 在应用前预览变更
kubectl diff -f manifests/

# 对于 Helm，先 dry-run 并记录版本
helm upgrade myapp ./chart --namespace prod --dry-run --debug
helm upgrade myapp ./chart --namespace prod --atomic --history-max 10

# 定期检测漂移（以 ArgoCD 为例）
argocd app diff myapp --local ./chart
```
强制要求所有生产变更通过 GitOps/Helm 执行，禁止在集群内直接修改关键资源。同时建议在 Git 仓库中设置分支保护、CODEOWNERS 与 CI 准入检查，确保只有经过评审的清单才能同步到生产集群。对于必须执行的紧急热修复，应在修复后 30 分钟内将变更回写到仓库，并记录 incident 编号以便追溯。

### 2.4 备份失效导致 RPO/RTO 无法达成

**风险**：etcd 快照或 Velero 备份看似成功，但恢复时因版本不兼容、加密密钥缺失或存储不可达而失败。

**缓解措施**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 etcd 快照完整性
etcdctl snapshot status /backup/etcd-$(date +%F).db

# 每月执行一次恢复演练（在隔离环境）
etcdctl snapshot restore /backup/etcd-latest.db \
  --data-dir=/var/lib/etcd-restored \
  --initial-cluster-token=prod-drill

# 验证 Velero 备份可恢复
velero backup get
velero restore create --from-backup prod-ns-daily --include-namespaces demo-restore
```
建议定义并测试 RPO ≤ 1h、RTO ≤ 30min 的恢复目标。备份文件应存放在与生产集群不同的区域或对象存储桶，并启用版本控制与加密。恢复演练不应只在测试环境进行，还应定期演练控制面重建、持久化数据挂载与入口 DNS 切换，确保灾难发生时值班工程师熟悉每一步命令与验证点。

### 2.5 安全事件导致横向移动

**风险**：容器逃逸、凭据泄露或恶意镜像被部署后，攻击者在集群内部横向移动。

**缓解措施**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启用 Pod Security Admission
kubectl label --overwrite ns production pod-security.kubernetes.io/enforce=restricted

# 禁止特权容器（Kyverno 策略示例）
kubectl apply -f - <<EOF
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged
spec:
  validationFailureAction: Enforce
  rules:
  - name: privileged-containers
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "生产环境禁止特权容器"
      pattern:
        spec:
          containers:
          - securityContext:
              allowPrivilegeEscalation: "false"
              privileged: "false"
EOF
```
此外，建议将容器运行时威胁检测（Falco/Tetragon）与 SIEM 对接，对异常进程、反向 shell 与敏感目录挂载进行实时告警。定期轮换 ServiceAccount token、镜像仓库凭据与 etcd 加密密钥，并将轮换操作纳入变更日历，避免在业务高峰期执行。

## 3. 日常运维操作

日常运维的目标是在用户感知之前发现并消除异常。以下操作按频率分层：晨检聚焦健康状态，周审视聚焦容量与成本，变更执行聚焦可控性，备份与证书巡检聚焦恢复能力。所有命令均应在值班日志中保留输出或截图，便于事后审计与根因分析。

### 3.1 晨检（5 分钟）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes -o wide
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded
kubectl get events -A --sort-by='.lastTimestamp' | tail -n 20
```
### 3.2 容量与成本审视

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl top nodes
kubectl top pods -A --sort-by=memory
# 阿里云 ACK：查看节点池与实例计费类型
aliyun cs GET /clusters/<cluster-id>/nodes
```
### 3.3 变更执行

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 预览变更
kubectl diff -f manifests/
# 原子化升级并保留回滚历史
helm upgrade myapp ./chart -n prod --atomic --cleanup-on-fail --history-max 10
# 监控滚动更新状态
kubectl rollout status deployment/myapp -n prod --timeout=300s
```
### 3.4 证书与备份巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubeadm certs check-expiration
kubectl get certificates -A
velero backup get | head
```
### 3.5 事件响应启动

当监控触发 P0/P1 告警时，立即按 [[domain-11-production-operations/03-on-call-playbook.md|值班手册]] 启动升级，并按 [[domain-11-production-operations/04-incident-response-template.md|事故响应模板]] 记录时间线。

## 4. 故障排查速查

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| 大量 Pod 处于 Pending | 资源不足、污点/亲和性不匹配、调度约束 | `kubectl describe pod <pod> -n <ns>`、`kubectl describe nodes` | 扩容节点池、调整 requests/affinity、删除误配污点 |
| 核心服务 5xx/延迟高 | HPA 未触发、PDB 阻塞滚动、CPU 节流 | `kubectl top pods -n prod`、`kubectl get hpa -n prod`、`kubectl get pdb -n prod` | 调整 HPA 阈值、临时扩容副本、检查 PDB minAvailable |
| 节点 NotReady | kubelet/PLEG 异常、磁盘压力、CNI 故障 | `kubectl describe node <node>`、`journalctl -u kubelet -n 100` | 驱逐节点上的 Pod、修复磁盘/CNI、cordon 后维修 |
| 证书告警触发 | cert-manager 续期失败或 kubeadm 证书临近过期 | `kubectl get certificaterequests -A`、`kubeadm certs check-expiration` | 修复 issuer/Challenge、执行 `kubeadm certs renew all` |
| 月度云成本突增 | 资源泄漏、Spot 实例被回收、存储快照膨胀 | 云厂商账单标签分析、`kubectl get pods -A -o wide` | 回收闲置资源、调整预留实例策略、优化快照保留期 |
| NetworkPolicy 阻断流量 | 策略选择器或端口配置错误 | `kubectl get networkpolicies -A -o yaml`、`kubectl run -it --rm debug --image=nicolaka/netshoot` | 修正 NetworkPolicy 规则、临时添加 allow 策略验证 |

## 5. 与其他域的协作边界

生产运维域不是孤立存在的，它需要把平台、安全、可观测性、可靠性、发布管理和云服务商的能力组合成可执行的运行状态。明确协作边界可避免重复建设与责任真空：生产运维域关注“是否按生产标准运行”，而其他域关注“如何设计与建设”。以下边界可确保问题被正确路由到对应领域：

- **可靠性工程**：SLO/SLI 定义、容灾架构、故障演练与 postmortem 由 [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] 主导，生产运维域负责落地值班、告警响应与执行回滚。
- **平台工程**：自助服务平台、租户 onboarding、配额模板与 GitOps 基线由 [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] 提供，生产运维域负责监控这些策略在生产环境中的实际效果。
- **安全合规**：PSP→PSS 迁移、CIS 加固、Secret 轮换与 RBAC 审计由 [[domain-05-security-compliance/README.md|domain-05-security-compliance]] 主导，生产运维域负责将安全基线纳入变更窗口与巡检。
- **可观测性**：指标、日志、链路追踪与 SLO dashboard 由 [[domain-06-observability/README.md|domain-06-observability]] 建设，生产运维域使用这些工具进行告警与事故定位。
- **发布与变更管理**：GitOps 工作流、金丝雀/蓝绿发布策略由 [[domain-08-release-change-management/README.md|domain-08-release-change-management]] 定义，生产运维域执行发布窗口管理与回滚操作。
- **故障排查与诊断**：复杂故障的根因定位与 FTA 由 [[domain-10-troubleshooting-diagnostics/README.md|domain-10-troubleshooting-diagnostics]] 支持，生产运维域提供一线排查数据与变更上下文。
- **云服务商**：ACK/EKS/GKE 等 provider 特有升级、DR、配额与账单由 [[domain-12-cloud-providers/README.md|domain-12-cloud-providers]] 覆盖，生产运维域将 provider 操作纳入统一 runbook。

## 6. 推荐阅读

### 本域核心资料

- [[domain-11-production-operations/01-production-sre-daily-ops.md|生产环境日常巡检与值班手册]]
- [[domain-11-production-operations/02-change-management-guide.md|变更管理指南]]
- [[domain-11-production-operations/03-on-call-playbook.md|值班手册与告警响应规范]]
- [[domain-11-production-operations/04-incident-response-template.md|事故响应模板与流程规范]]
- [[domain-11-production-operations/02-governance/14-resource-quota-management.md|资源配额管理]]
- [[domain-11-production-operations/01-finops/13-kubernetes-cost-governance.md|Kubernetes 成本治理]]

### 本域待补齐专题（gap 分析推荐）

- 容量规划与生产就绪（待补充）
- 多集群与舰队运维（待补充）
- 灾备与备份恢复运维（待补充）
- [[domain-11-production-operations/08-security-operations-runbook.md|生产安全运维手册]]（待补充）
- 集群升级运维手册（待补充）
- [[domain-11-production-operations/10-observability-operations.md|可观测性运维]]（待补充）
- GitOps 运维（待补充）
- 自动化修复运维（待补充）
- 节点与容器运行时运维（待补充）

### 相关域入口

- [[domain-09-reliability-engineering/README.md|可靠性工程]]
- [[domain-07-platform-engineering/README.md|平台工程]]
- [[domain-05-security-compliance/README.md|安全合规]]
- [[domain-06-observability/README.md|可观测性]]
- [[domain-08-release-change-management/README.md|发布与变更管理]]
- [[domain-10-troubleshooting-diagnostics/README.md|故障排查与诊断]]
- [[domain-12-cloud-providers/README.md|云服务商]]

---

生产就绪不是一份文档就能解决的问题，而是需要通过反复演练、持续度量和跨团队协作形成肌肉记忆。建议每季度对本指南进行一次回顾，更新检查项、风险列表与推荐阅读，确保其与集群实际状态和组织成熟度保持一致。


<!-- risk-assessed -->
