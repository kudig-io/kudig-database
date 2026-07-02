---
title: 云厂商托管 Kubernetes 生产就绪运维指南
description: 覆盖 AWS EKS、GKE、AKS、阿里云 ACK、腾讯云 TKE、华为云 CCE 及多云场景的生产就绪检查、风险缓解、日常运维与故障排查的 SRE 级操作指南。
summary: 云厂商托管 Kubernetes 生产就绪检查、风险缓解、日常运维与故障排查的 SRE 级操作指南。
category: cloud-provider
tags:
- production
- best-practices
- cloud-provider
- operations
- multi-cloud
- eks
- gke
- aks
- ack
- tke
- cce
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 云架构师
estimated_read_time: 20min
intent_queries:
- 云厂商托管 Kubernetes 生产就绪运维指南是什么
- 如何按生产环境要求运维 AWS EKS / GKE / AKS / ACK
- 多云 Kubernetes 生产就绪检查清单有哪些
trigger_keywords:
- 生产就绪
- 运维指南
- 云厂商
- 托管 Kubernetes
- EKS
- GKE
- AKS
- ACK
- TKE
- CCE
- 多云
prerequisites:
- kubectl-basics
- cloud-provider-basics
- troubleshooting-methodology
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


# 云厂商托管 Kubernetes 生产就绪运维指南

本指南面向在 AWS EKS、Google GKE、Azure AKS、阿里云 ACK、腾讯云 TKE、华为云 CCE 等主流云厂商托管 Kubernetes 服务上运行生产负载的 SRE 与平台工程师。内容聚焦于跨云厂商通用的生产就绪检查、高风险点、日常运维命令、故障排查速查以及与其他域的协作边界。具体厂商的架构细节可参考各厂商专题页面。

> **适用范围**: 托管 Kubernetes v1.28-v1.33 | **维护状态**: 持续更新 | **风险等级**: 高 — 涉及生产控制平面与网络变更

---

## 1. 生产环境检查清单

在将任一云厂商托管集群标记为 **Production Ready** 之前，必须逐项确认以下检查点。建议将清单固化到集群交付流水线或 GitOps 仓库中，作为集群上线前的强制 Gate。清单分为控制平面、数据平面、网络、安全、可观测性、灾备、成本与治理七大维度，覆盖从集群创建到持续运营的全生命周期。

| 序号 | 检查项 | 验收标准 | 推荐命令 / 配置 |
|---|---|---|---|
| 1 | **控制平面高可用** | 多可用区部署、API Server 无单点、版本在厂商支持窗口内 | `kubectl version` 核对 Server 版本；控制台确认 Master 多 AZ |
| 2 | **节点池多 AZ 与反亲和** | 关键业务 Pod 跨可用区分布 | `kubectl get nodes -L topology.kubernetes.io/zone` |
| 3 | **网络 CNI 与 IPAM 容量** | Pod CIDR 与 VPC 子网剩余 IP 满足未来 6 个月增长 | ACK Terway 检查 `kube-system/terway-daemon` ENI 余量；EKS 检查 `kubectl get eniconfigs` |
| 4 | **负载均衡与 Ingress 出口稳定** | SLB/ALB/NLB/CLB 健康检查、证书、后端目标组一致 | `kubectl get svc,ingress -A`；云控制台核对目标组后端 |
| 5 | **IAM /  workload identity 启用** | 禁止节点长期密钥，Pod 使用 IRSA(EKS)、Workload Identity(GKE/AKS)、RRSA(ACK) | `kubectl get sa -n prod -o yaml` 确认 `eks.amazonaws.com/role-arn` 或等价注解 |
| 6 | **存储 CSI 与快照策略** | ESSD/EBS/Azure Disk/Cloud Disk 已部署 CSI，VolumeSnapshotClass 就绪 | `kubectl get sc,volumesnapshotclass` |
| 7 | **可观测性三支柱覆盖** | Metrics / Logs / Traces 采集到统一平台，核心告警规则生效 | `kubectl get prometheusrules,servicemonitors -A` |
| 8 | **备份与灾难恢复** | etcd/集群配置、PersistentVolume、关键 Secret 定期备份，RTO/RPO 明确 | ACK 启用自动备份；EKS 使用 Velero + S3；GKE Backup for GKE |
| 9 | **安全加固与合规基线** | PSP/PSS 限制特权容器、NetworkPolicy 默认拒绝、镜像扫描与准入 | `kubectl get networkpolicies -A`；`kubectl get psp` 或 `kubectl label ns prod pod-security.kubernetes.io/enforce=restricted` |
| 10 | **成本与配额治理** | 命名空间 ResourceQuota、LimitRange、Spot/抢占式实例标签与污点隔离 | `kubectl get resourcequota,limitrange -A` |
| 11 | **升级与回滚计划** | 已制定控制平面与节点池升级窗口、版本偏斜检查、回滚命令 | `kubectl get nodes -o wide` 核对 kubelet 版本；厂商 CLI 查询可用升级版本 |
| 12 | **灾难演练与混沌工程** | 已完成 AZ 故障切换、节点故障、控制平面断网演练并记录 RTO | 参考 [[domain-09-reliability-engineering/05-chaos-engineering/01-chaos-engineering-overview.md|混沌工程概述]] |

> 检查清单应随每次大版本升级或新环境交付重新执行，并将结果归档到变更管理记录中。建议由平台工程师、SRE 和安全工程师三方会签，缺一不可。对于多集群场景，应在每个区域或每个云账号单独执行，避免“一套环境通过、其余环境漏检”的隐患。

---

## 2. 关键风险与缓解措施

### 2.1 云厂商控制平面升级导致 API 变更或中断

**风险**: 托管 Kubernetes 控制平面由云厂商自动维护，升级窗口可能引入废弃 API、 admission webhook 兼容性问题或短暂 API Server 不可用。

**缓解措施**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 升级前扫描废弃 API
kubectl get --raw /apis | jq -r '.groups[].name'
# 或使用厂商工具：EKS
eksctl utils update-kube-proxy --cluster prod --approve --dry-run
# GKE
gcloud container clusters describe prod --zone asia-east1-a --format='table(currentMasterVersion,currentNodeVersion,availableNodeVersionCount)'
```
升级前在非生产环境复现完整升级路径，确认关键 CRD、Webhook、Policy 无异常。

### 2.2 节点池 / 可用区故障导致服务降级

**风险**: 单可用区节点池、Spot 实例被回收、或厂商侧 ECS/EC2 故障引发大规模 Pod 驱逐。

**缓解措施**:

```yaml
# PodDisruptionBudget 保证最小可用副本
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-app-pdb
  namespace: prod
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: critical-app
---
# 拓扑分布约束强制跨 AZ
apiVersion: apps/v1
kind: Deployment
metadata:
  name: critical-app
  namespace: prod
spec:
  replicas: 6
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: critical-app
              topologyKey: topology.kubernetes.io/zone
```

### 2.3 网络 IP 耗尽导致 Pod 无法调度

**风险**: VPC 子网、Pod CIDR 或 ENI 辅助 IP 耗尽，触发大量 `FailedCreatePodSandBox` 事件。

**缓解措施**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ACK Terway 检查 ENI/IP 余量
kubectl exec -n kube-system daemonset/terway-daemon -- terway-cli show
# EKS 检查子网可用 IP
aws ec2 describe-subnets --subnet-ids subnet-xxxx --query 'Subnets[*].AvailableIpAddressCount'
# GKE 检查 Pod CIDR
kubectl get nodes -o jsonpath='{.items[*].spec.podCIDR}'
```
当余量低于 20% 时，应触发网络扩容变更单，扩展子网或新增节点池。

### 2.4 工作负载身份泄露或过度授权

**风险**: 使用节点实例角色导致所有 Pod 共享云权限，或 ServiceAccount 绑定过高权限 RAM/IAM Role。

**缓解措施**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# EKS IRSA 注解检查
kubectl get sa -A -o jsonpath='{range .items[*]}{@.metadata.namespace}{"\t"}{@.metadata.name}{"\t"}{@.metadata.annotations.eks\.amazonaws\.com/role-arn}{"\n"}{end}'
# ACK RRSA 检查
kubectl get sa -A -o jsonpath='{range .items[*]}{@.metadata.namespace}{"\t"}{@.metadata.name}{"\t"}{@.metadata.annotations.ram\.aliyun\.com/role-arn}{"\n"}{end}'
```
强制每个应用使用独立 ServiceAccount，IAM Role 遵循最小权限，并启用 CloudTrail / ActionTrail 审计。

### 2.5 存储快照与跨可用区恢复失败

**风险**: CSI Snapshot 未验证恢复流程，灾难发生时无法在规定时间内挂载历史数据。

**缓解措施**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建测试快照并恢复验证（以 ACK 为例）
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: test-snapshot
  namespace: prod
spec:
  volumeSnapshotClassName: alibabacloud-disk-snapshot
  source:
    persistentVolumeClaimName: prod-data-pvc
EOF

# 验证快照可用
kubectl get volumesnapshot test-snapshot -n prod -o jsonpath='{.status.readyToUse}'
```
每月执行一次恢复演练，记录 RTO/RPO 实测值。

---

## 3. 日常运维操作

### 3.1 集群状态巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 多集群上下文快速切换与巡检
kubectl config use-context prod-eks-ap-northeast-1
kubectl get nodes -o wide
kubectl top nodes
kubectl get pods -A -o wide | grep -v Running
kubectl get events -A --sort-by=.lastTimestamp | tail -n 50

# 检查核心系统组件（各厂商 kube-system 组件）
kubectl get pods -n kube-system
kubectl get daemonset -n kube-system
```
### 3.2 节点池扩缩容

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# EKS 使用 eksctl 扩容节点池
eksctl scale nodegroup --cluster prod --name spot-ng --nodes 5 --nodes-min 3 --nodes-max 20

# ACK 使用 aliyun CLI 调整节点池
aliyun cs POST /clusters/<cluster-id>/nodepools/<np-id> \
  --body '{"auto_scaling":{"enable":true,"min_instances":3,"max_instances":20}}'

# GKE
gcloud container clusters resize prod --node-pool default-pool --num-nodes 5 --zone us-central1-a
```
### 3.3 证书与 Secret 轮换

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查证书过期时间（托管版通常由厂商自动维护，专有版需关注）
kubectl get secret -n kube-system
for cert in $(kubectl get secret -n kube-system -o jsonpath='{.items[*].metadata.name}'); do
  echo "$cert: $(kubectl get secret -n kube-system "$cert" -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates 2>/dev/null | grep notAfter)"
done

# 云厂商托管集群通常自动轮换，但需监控告警
```
### 3.4 备份作业验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Velero 备份示例（EKS / 多云通用）
velero backup create prod-daily-$(date +%Y%m%d) \
  --include-namespaces prod,monitoring \
  --ttl 720h0m0s \
  --storage-location aws-s3

velero backup get | head
velero backup logs prod-daily-$(date +%Y%m%d) | tail -n 30
```
### 3.5 成本与资源治理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看各命名空间资源使用与申请差异
kubectl top pods -A --containers | sort -k4 -nr | head -n 20
kubectl describe resourcequota -n prod

# 标记低利用率工作负载进行优化
kubectl get deployments -A -o jsonpath='{range .items[*]}{@.metadata.namespace}{"/"}{@.metadata.name}{"\n"}{end}' | \
  xargs -I {} sh -c 'echo "--- {} ---"; kubectl top pods -n $(echo {} | cut -d/ -f1) -l app=$(echo {} | cut -d/ -f2)'
```
---

## 4. 故障排查速查

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 处于 `Pending` | 资源不足、污点不匹配、调度约束冲突 | `kubectl describe pod <pod> -n <ns>`<br>`kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints` | 扩容节点池、调整 toleration/affinity、检查 ResourceQuota |
| Pod 反复 `CrashLoopBackOff` | 应用启动失败、健康检查配置错误、环境变量缺失 | `kubectl logs <pod> -n <ns> --previous`<br>`kubectl describe pod <pod>` | 修复镜像或配置、调整 probe 参数、补齐 Secret/ConfigMap |
| 节点 `NotReady` | kubelet 异常、磁盘压力、网络分区、实例被回收 | `kubectl describe node <node>`<br>`journalctl -u kubelet -n 200` | 驱逐节点 Pod、重启 kubelet、清理磁盘、替换故障实例 |
| 服务访问超时 / DNS 解析失败 | CoreDNS 负载高、SLB 后端健康检查失败、安全组拦截 | `kubectl get svc,endpoints -n <ns>`<br>`kubectl logs -n kube-system -l k8s-app=kube-dns` | 扩容 CoreDNS、检查 Endpoint 与目标组、修正安全组/NetworkPolicy |
| PVC 无法绑定 | StorageClass 不存在、可用区不匹配、磁盘配额耗尽 | `kubectl get pvc,pv,sc`<br>`kubectl describe pvc <pvc> -n <ns>` | 创建正确 StorageClass、确保节点与存储同 AZ、申请配额扩容 |
| 镜像拉取失败 `ImagePullBackOff` | 镜像不存在、仓库权限、网络不通 | `kubectl describe pod <pod>`<br>`kubectl get secret -n <ns>` | 校验镜像 tag、更新 imagePullSecret、检查仓库网络连通性 |
| API Server 响应缓慢 | etcd 延迟高、控制平面资源不足、大量 List 请求 | `kubectl top pods -n kube-system`<br>`kubectl logs -n kube-system -l component=kube-apiserver` | 限制客户端 List 并发、升级控制平面规格、检查 etcd 磁盘延迟 |
| 跨集群服务发现失败 | 多集群 DNS/ServiceExport 配置错误、网络隧道中断 | `kubectl get serviceexport -A`<br>`kubectl get gateway -A` | 检查 MCS/Gateway API 配置、验证跨集群 VPN/专线连通性 |

---

## 5. 生产就绪评审流程

将检查清单转化为可落地的 PRR（Production Readiness Review）流程，可显著降低上线后的事故率。推荐以下四步评审法：

**Step 1：自检与数据采集**
由集群交付负责人依据第 1 节清单逐项自检，采集命令输出、截图或日志，形成《生产就绪自检报告》。自检应在预发布环境完成后、生产环境交付前进行。

**Step 2：跨团队评审会**
组织 SRE、安全、网络、应用架构四方评审，重点审查：
- 高可用设计是否覆盖控制平面、节点池、网络、存储四层；
- IAM 与 workload identity 是否存在过度授权；
- 灾备方案是否经过实际恢复演练；
- 变更窗口、回滚命令与升级路径是否文档化。

**Step 3：遗留风险登记**
对于未达标的检查项，必须登记为“生产就绪遗留风险”，明确风险等级、责任人、预计修复时间与临时缓解措施。P0 级遗留风险不得上线。

**Step 4：上线后持续验证**
生产环境上线首月内，每周复核一次关键指标：节点就绪率、Pod 调度成功率、API Server 延迟、证书有效期、备份成功率、告警误报率。首月通过后转入标准运维节奏。所有复核结果应写入变更管理系统的附件，便于后续审计与复盘。

---

## 6. 与其他域的协作边界

云厂商托管 Kubernetes 并非孤立存在，生产就绪工作必须与以下域紧密协作：

- **[[domain-01-cluster-fundamentals/README.md|集群基础架构]]** — 控制平面高可用、版本偏斜、API 废弃、节点生命周期由本域落地，但架构设计原则由该域定义。
- **[[domain-03-networking-traffic/README.md|网络与流量]]** — VPC/CNI、负载均衡、Ingress、Service Mesh、跨云网络互联依赖该域的网络模型与排障方法。
- **[[domain-05-security-compliance/README.md|安全合规]]** — IAM/workload identity、NetworkPolicy、Pod Security Standards、镜像签名与审计在该域与云 IAM 之间形成交叉责任面。
- **[[domain-06-observability/README.md|可观测性]]** — Metrics/Logs/Traces 采集、告警体系、SLI/SLO 定义由该域提供方法论，本域负责对接云厂商监控服务（CloudWatch、SLS、Azure Monitor）。
- **[[domain-07-platform-engineering/README.md|平台工程]]** — GitOps、多集群 fleet 管理、标准化交付模板、FinOps 由该域主导，本域提供各云厂商的具体实现参数。
- **[[domain-09-reliability-engineering/README.md|可靠性工程]]** — 灾难恢复、备份演练、混沌工程、SLO/SLA 由该域统筹，本域补充厂商特定的 RTO/RPO 方案。
- **[[domain-11-production-operations/README.md|生产运维]]** — 值班、变更管理、事件响应流程由该域定义，本域提供云厂商相关的 runbook 与回滚命令。

---

## 7. 推荐阅读

### 本域必读

- [[domain-12-cloud-providers/05-alicloud-ack/alicloud-ack-overview.md|阿里云 ACK 概述]] — ACK 托管版/专有版架构、Terway 网络、RRSA 身份与安全加固。
- [[domain-12-cloud-providers/08-multi-cloud/00-multi-cloud-hybrid-deployment-strategy.md|多云混合部署策略]] — 主备/主主模式、跨云数据同步、故障切换与统一监控。
- [[domain-12-cloud-providers/02-aws-eks/aws-eks-overview.md|AWS EKS 概述]] — EKS 架构、IRSA、托管节点组与 Fargate 模式。
- [[domain-12-cloud-providers/03-google-cloud-gke/google-cloud-gke-overview.md|Google GKE 概述]] — GKE Autopilot/Standard、Workload Identity、Backup for GKE。
- [[domain-12-cloud-providers/04-azure-aks/azure-aks-overview.md|Azure AKS 概述]] — AKS 网络、Azure AD 集成、托管 Prometheus/Grafana。

### 跨域参考

- [[domain-01-cluster-fundamentals/99-kubernetes-production-architecture-blueprint.md|Kubernetes 生产架构蓝图]] — 生产集群整体架构设计原则。
- [[domain-05-security-compliance/README.md|安全合规域]] — RBAC、NetworkPolicy、Pod Security、镜像安全与审计。
- [[domain-09-reliability-engineering/README.md|可靠性工程域]] — 灾备、备份演练、SLO/SLA 与混沌工程。

---

*本指南作为 domain-12-cloud-providers 的统一生产就绪入口，后续将逐步补充各云厂商专属的升级 runbook、灾难恢复 runbook 与可观测性配置 runbook。当前若需具体厂商操作细节，请优先参考上述推荐阅读中的专题页面。*


<!-- risk-assessed -->
