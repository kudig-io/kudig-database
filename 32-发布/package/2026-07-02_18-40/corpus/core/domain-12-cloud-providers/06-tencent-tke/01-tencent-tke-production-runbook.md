---
title: 腾讯云 TKE 生产运维 Runbook
description: 面向腾讯云 TKE 托管 Kubernetes 集群的全生命周期生产运维手册，覆盖集群创建、VPC-CNI 网络、CAM/TCM 工作负载身份、节点池、升级、灾备、CLS 可观测、成本治理与故障排查。
summary: 腾讯云 TKE 托管 Kubernetes 生产运维 Runbook，涵盖集群生命周期、网络、身份、节点池、升级、灾备、可观测与成本。
category: cloud-provider
tags:
- production
- best-practices
- playbook
- tencent
- tke
- cloud-provider
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
estimated_read_time: 25min
intent_queries:
- 腾讯云 TKE 生产运维 Runbook 是什么
- 如何运维 TKE 生产集群
- TKE 集群升级、灾备、网络、节点池最佳实践
trigger_keywords:
- TKE
- 腾讯云
- Tencent Kubernetes Engine
- 生产运维
- runbook
- CAM
- TCM
- CLS
prerequisites:
- kubectl-basics
- cloud-provider-basics
- tencent-cloud-cli-basics
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

# 腾讯云 TKE 生产运维 Runbook

> **适用版本**: TKE 托管集群 v1.28 - v1.33 | **适用地域**: 腾讯云中国大陆 / 国际站 | **目标角色**: SRE / 平台工程师 / 云架构师

本 Runbook 聚焦腾讯云 TKE（Tencent Kubernetes Engine）托管 Kubernetes 集群在生产环境中的全生命周期运维。内容覆盖集群创建与下线、VPC-CNI 网络、CAM 与 TCM 工作负载身份、节点池与扩缩容、版本升级、灾备与回滚、CLS 可观测接入、成本治理以及高频故障排查。对于跨云厂商的通用生产就绪检查，请先阅读 [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|云厂商托管 Kubernetes 生产就绪运维指南]]。

---

## 1. 适用场景与范围

本文档适用于以下场景：

- 新建 TKE 托管集群并需要满足生产可用要求。
- 已有 TKE 集群的日常巡检、变更、升级与灾备演练。
- 涉及 VPC-CNI、CAM 角色、节点池、CLS、COS 等云产品联动的故障排查。
- 多可用区部署、混合云专线、跨地域灾备的架构落地。

**不在本文范围**：自建 Kubernetes（TKE 独立部署模式）、非腾讯云环境、业务代码层排障。

---

## 2. 前置条件与工具

| 工具 | 用途 | 推荐版本 |
|---|---|---|
| `kubectl` | Kubernetes 集群操作 | v1.28+ |
| `tccli` / `tcloud` | 腾讯云 CLI | 最新版 |
| `helm` | 安装可观测 / 安全组件 | v3.13+ |
| `jq` | JSON 输出处理 | 任意 |
| `velero` | 集群资源备份恢复 | v1.13+ |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 配置 tccli 凭据与默认地域
tccli configure
# 验证身份
tccli sts GetCallerIdentity
# 配置 kubeconfig
tccli tke DescribeClusterSecurity --ClusterId tke-xxxxxx
tccli tke DescribeClusterKubeconfig --ClusterId tke-xxxxxx > ~/.kube/tke-prod
export KUBECONFIG=~/.kube/tke-prod
kubectl config current-context
```

---

## 3. 核心概念/架构

```
┌─────────────────────────────────────────────────────────────┐
│                      腾讯云 TKE 托管集群                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ 控制平面     │  │ VPC-CNI     │  │ 节点池 / NodePool   │  │
│  │ (Master HA) │  │ (Global Router│ │ (CVM / 黑石 / 竞价)  │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│         │                │                  │               │
│         ▼                ▼                  ▼               │
│  ┌─────────────────────────────────────────────────────────┐│
│  │  CAM / TCM 工作负载身份 + CLS / CM / COS / CBS         ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

- **控制平面**：托管版 Master 三可用区部署，建议开启审计日志与 KMS 加密。
- **VPC-CNI**：推荐 Global Router 模式，Pod 与 CVM 同 VPC 网段，注意子网 IP 容量规划。
- **CAM/TCM 工作负载身份**：通过 `tke.cloud.tencent.com/cam-role-name` 等注解为 Pod 授予 CAM 角色，避免节点密钥泛化。
- **节点池**：支持普通 CVM、黑石、竞价实例；建议关键业务使用普通节点池，可中断负载使用竞价节点池。
- **CLS**：日志服务统一采集容器标准输出、文件日志与审计日志。

---

## 4. 标准操作流程

### 4.1 集群创建与基线加固

> **🔴 高风险操作警告**
>
> 创建集群会初始化云资源并产生费用，执行前请确认：VPC/子网/CIDR 规划、访问策略、成本预算与责任人授权。

``` bash
# 🔴 高风险：会创建云资源并产生费用，执行前需审批与成本确认
# 创建托管集群示例（通过 tccli）
tccli tke CreateCluster \
  --ClusterName prod-tke-ap-guangzhou \
  --ClusterVersion 1.30.0 \
  --ClusterType MANAGED_CLUSTER \
  --Region ap-guangzhou \
  --VpcId vpc-xxxxxx \
  --SubnetIds '["subnet-xxxxxx"]' \
  --ClusterCIDR 172.16.0.0/16 \
  --MaxNodePodNum 64 \
  --MaxClusterServiceNum 1024 \
  --EnableClusterAudit True \
  --EnableEncryptionProtection True \
  --KmsKeyId kms-xxxxxx
```

创建后必须执行基线加固：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 为业务命名空间设置 PSA enforce=baseline
kubectl label ns production pod-security.kubernetes.io/enforce=baseline --overwrite

# 2. 部署默认拒绝 NetworkPolicy
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
EOF

# 3. 启用审计日志投递到 CLS
tccli tke ModifyClusterAttribute \
  --ClusterId tke-xxxxxx \
  --AuditLogEnabled True \
  --AuditLogTopicId xxxxxx
```

### 4.2 VPC-CNI 网络容量检查与扩容

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群网络模式与 CIDR
tccli tke DescribeCluster --ClusterId tke-xxxxxx | jq '.Clusters[0] | {ClusterCIDR, VpcId, SubnetIds}'

# 查看节点已分配 IP（tke-eniip 插件）
kubectl get pods -A -o wide

# 查看 ENI/IP 余量（需登录节点或查看 tke-cni 插件指标）
kubectl logs -n kube-system ds/tke-eniip -c tke-eniip | grep -i "available"
```

当子网可用 IP 低于 20% 时，应新增子网并关联到节点池：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建新子网并扩容节点池（控制台或 tccli）
tccli vpc CreateSubnet --VpcId vpc-xxxxxx --SubnetName tke-pod-subnet-02 --CidrBlock 172.16.128.0/20 --Zone ap-guangzhou-3
```

### 4.3 CAM/TCM 工作负载身份配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 CAM 角色并绑定到 ServiceAccount
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cos-access-sa
  namespace: production
  annotations:
    tke.cloud.tencent.com/cam-role-name: tke-cos-readonly-role
EOF

# 检查角色注解
kubectl get sa cos-access-sa -n production -o jsonpath='{.metadata.annotations.tke\.cloud\.tencent\.com/cam-role-name}'
```

### 4.4 节点池创建与扩缩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建多 AZ 节点池
tccli tke CreateClusterNodePool \
  --ClusterId tke-xxxxxx \
  --NodePoolName prod-general-ng \
  --LaunchConfigurePara '{"InstanceType":"S5.2XLARGE16","SystemDisk":{"DiskType":"CLOUD_SSD","DiskSize":100}}' \
  --SubnetIds '["subnet-az1-xxxxxx","subnet-az2-xxxxxx","subnet-az3-xxxxxx"]' \
  --AutoscalingGroupPara '{"MinSize":3,"MaxSize":20,"DesiredCapacity":3}'

# 手动扩容
kubectl scale --replicas=10 deployment/<workload> -n production
# 或调整节点池容量
tccli tke ModifyClusterNodePool --ClusterId tke-xxxxxx --NodePoolId np-xxxxxx --AutoscalingGroupPara '{"DesiredCapacity":10}'
```

### 4.5 集群升级

> **🔴 高风险操作警告**
>
> 升级涉及控制平面与节点池滚动替换，执行前请确认：变更窗口、应用兼容性、回滚方案、备份完成。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查询可升级版本
tccli tke DescribeAvailableClusterVersion --ClusterId tke-xxxxxx

# 查看节点版本分布
kubectl get nodes -o wide

# 🔴 高风险：可能造成服务中断，执行前需备份、变更审批与回滚方案
# 升级控制平面
tccli tke UpdateClusterVersion --ClusterId tke-xxxxxx --DstVersion 1.31.0 --MaxNotReadyPercent 20

# 升级节点池
tccli tke UpgradeClusterInstances --ClusterId tke-xxxxxx --NodePoolId np-xxxxxx --MaxNotReadyPercent 20
```

### 4.6 灾备与回滚

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Velero 备份关键命名空间
velero backup create tke-prod-backup-$(date +%Y%m%d) \
  --include-namespaces production,monitoring \
  --storage-location cos-default \
  --ttl 720h0m0s

# 跨地域复制 COS 备份桶（需预先配置跨地域复制规则）
tccli cos PutBucketReplication --Bucket prod-tke-backup-gz --ReplicationConfiguration file:///path/to/replication.json

# 灾备切换：在灾备地域恢复 Velero 备份
velero restore create --from-backup tke-prod-backup-20260701
```

### 4.7 CLS 可观测接入

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 TKE 日志采集组件（如未启用）
# 通过控制台开启容器日志采集到 CLS

# 验证采集器运行
kubectl get pods -n kube-system -l app=tke-log-agent

# 查看日志主题与机器组
tccli cls DescribeTopics --TopicId xxxxxx
tccli cls DescribeMachines --TopicId xxxxxx
```

### 4.8 成本治理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点规格与计费模式
kubectl get nodes -o custom-columns=NAME:.metadata.name,INSTANCE-TYPE:.metadata.labels.'node\.kubernetes\.io/instance-type',SPOT:.metadata.labels.'eks\.tencentcloud\.com/capacity-type'

# 查看 ResourceQuota 与 LimitRange
kubectl get resourcequota,limitrange -A
```

---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 / 方法 | 通过标准 |
|---|---|---|
| 控制平面多 AZ | `tccli tke DescribeCluster --ClusterId tke-xxxxxx` | Master 跨 ≥2 可用区 |
| 节点跨 AZ 分布 | `kubectl get nodes -L topology.kubernetes.io/zone` | 关键节点池覆盖 ≥3 AZ |
| VPC-CNI IP 余量 | 子网控制台 / `tccli vpc DescribeSubnets` | 可用 IP > 20% |
| CAM 身份绑定 | `kubectl get sa -A -o json \| jq` | 业务 SA 均绑定 CAM 角色 |
| Pod Security | `kubectl get ns production -o jsonpath='{.metadata.labels}'` | enforce=baseline/restricted |
| NetworkPolicy | `kubectl get networkpolicy -A` | 生产命名空间存在默认拒绝 |
| 审计日志 | `tccli tke DescribeClusterSecurity --ClusterId tke-xxxxxx` | Audit 已启用并投递 CLS |
| 备份成功率 | `velero backup get \| grep Completed` | 近 7 天备份均成功 |
| 证书有效期 | `kubeadm certs check-expiration`（独立模式）或查看托管告警 | > 90 天 |

---

## 6. 常见故障与 Remediation

| 现象 | 可能根因 | 确认命令 | 修复动作 |
|---|---|---|---|
| Pod 无法调度，事件提示 IP 耗尽 | VPC-CNI 子网 IP 不足 | `tccli vpc DescribeSubnets` | 扩容子网或新增节点池子网 |
| Pod 拉取镜像失败 | 镜像仓库鉴权、网络不通 | `kubectl describe pod <pod>` | 更新 imagePullSecret；检查 VPC 与 TCR 连通性 |
| 节点 NotReady | CVM 异常、kubelet 停止、磁盘压力 | `kubectl describe node` | 驱逐节点、重启 kubelet、替换故障 CVM |
| CAM 角色未生效 | 注解错误或 CAM 策略未授权 | `kubectl get sa -o yaml` | 修正注解；检查 CAM 角色策略与信任关系 |
| 跨节点 Pod 不通 | 安全组、NetworkPolicy、CNI 异常 | `kubectl get networkpolicy -n <ns>` | 放行安全组；检查 CNI Pod 日志 |
| API Server 响应慢 | etcd 延迟、控制平面规格不足 | `kubectl top pods -n kube-system` | 升配控制平面；限制 List 请求并发 |
| CLS 日志缺失 | 日志采集 Agent 未启动、主题配置错误 | `kubectl get pods -n kube-system -l app=tke-log-agent` | 重启 Agent；校验 TopicId 与权限 |
| 节点池扩缩容失败 | 配额不足、启动模板错误、ASG 异常 | `tccli tke DescribeClusterNodePoolDetail` | 检查配额与启动模板；重新关联子网 |

---

## 7. 风险与注意事项

1. **控制平面升级不可逆**：TKE 托管版控制平面升级后一般不支持版本回滚，升级前务必在 staging 验证。
2. **VPC-CNI IP 规划**：Pod CIDR 与 VPC 子网绑定后不可随意修改，建议按峰值 2 倍预留 IP。
3. **CAM 角色过度授权**：避免将高权限 CAM 角色绑定到 default ServiceAccount，遵循最小权限原则。
4. **节点池滚动替换**：升级或替换节点池会导致 Pod 驱逐，需确保 PDB 与 HPA 配置合理。
5. **CLS 成本**：全量审计日志与容器日志可能产生较高 CLS 费用，建议按命名空间配置采集策略与索引。
6. **跨地域灾备 RTO/RPO**：Velero + COS 跨地域复制可实现小时级 RPO，但应用层数据一致性仍需业务配合。

---

## 8. 相关 Runbook / 推荐阅读

### 本域必读

- [[domain-12-cloud-providers/腾讯云TKE/tencent-tke-overview.md|腾讯云 TKE 概述]] — TKE 架构与核心概念
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/06-tencent-tke/01-tke-networking-vpc-cni|TKE VPC-CNI 网络]] — 网络模式与 IPAM 细节
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/06-tencent-tke/03-tke-iam-cam-integration|TKE CAM 身份集成]] — CAM/TCM 工作负载身份
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/06-tencent-tke/04-tke-troubleshooting-playbook|TKE 故障排查手册]] — 专项排障

### 跨域参考

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|云厂商托管 Kubernetes 生产就绪运维指南]] — 跨云通用生产就绪检查
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|安全与合规生产就绪运维指南]] — 安全基线与审计
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|可观测性生产就绪运维指南]] — CLS / Prometheus / 告警体系
- [[domain-09-reliability-engineering/README.md|可靠性工程域]] — 灾备、备份演练与 SLO
- [[_reports/domain-content-gap-analysis-2026-07-01.md|Domain Content Gap Analysis 2026-07-01]]


<!-- risk-assessed -->
