---
title: AWS EKS 生产运行手册
description: 面向 SRE 的 AWS EKS 集群全生命周期生产运维、IRSA、VPC CNI、升级、灾备、可观测性、成本治理与故障排查 Runbook
summary: AWS EKS 生产运行手册，覆盖集群创建与基线加固、IRSA/EKS Pod Identity、VPC CNI、控制平面与节点组升级、灾备、可观测性、成本治理与常见故障 remediation。
category: cloud-provider
tags:
- production
- best-practices
- playbook
- cloud-provider
- aws-eks
- eks
- irsa
- vpc-cni
- disaster-recovery
- cost-governance
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
estimated_read_time: 30min
intent_queries:
- AWS EKS 生产运行手册是什么
- 如何运维 AWS EKS 生产集群
- EKS IRSA VPC CNI 升级与灾备怎么做
- EKS 成本治理与故障排查
trigger_keywords:
- AWS EKS
- EKS
- IRSA
- VPC CNI
- eksctl
- 灾备
- 成本治理
- managed nodegroup
prerequisites:
- kubectl-basics
- aws-cli-basics
- eksctl-basics
- networking-basics
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


# AWS EKS 生产运行手册

> **适用范围**: AWS EKS 上运行生产负载的集群，覆盖集群搭建、身份管理、网络、升级、灾备、可观测性、成本与排障。  
> **目标读者**: SRE、平台工程师、AWS 云架构师。  
> **最后更新**: 2026-07-01

本手册聚焦 AWS EKS 生产运维的可执行命令与标准流程。建议与 [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|云厂商托管 Kubernetes 生产就绪运维指南]] 配套使用。

---

## 1. 适用场景与范围

- **适用场景**:
  - EKS 生产集群创建与基线加固（私有子网、KMS 加密、审计日志、OIDC）。
  - IRSA / EKS Pod Identity 配置、轮换与审计。
  - VPC CNI 扩容、升级、自定义网络与 IP 管理。
  - 控制平面与托管/自管节点组升级。
  - 灾备、备份、跨区域恢复与 RTO/RPO 验证。
  - CloudWatch / Prometheus / Grafana / AMP 监控与成本治理。
- **不适用场景**: 本地数据中心或其他云厂商 Kubernetes；Fargate-only 工作负载可参考但部分节点命令不适用。

---

## 2. 前置条件与工具

| 工具/资源 | 版本/要求 | 用途 |
|---|---|---|
| AWS CLI | ≥ v2 | IAM、EC2、EKS、S3 管理 |
| eksctl | ≥ 0.190 | EKS 集群与节点组管理 |
| kubectl | 与 EKS 版本匹配 | K8s 资源管理 |
| Helm 3 | ≥ 3.12 | 组件部署 |
| Velero | 已部署 | 跨集群备份恢复 |
| CloudWatch / AMP / S3 | 已接入 | 日志、指标、备份存储 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证工具版本
aws --version
eksctl version
kubectl version --client

# 验证当前默认凭证与区域
aws sts get-caller-identity
aws configure get region
```
---

## 3. 核心概念/架构

### 3.1 EKS 责任共担模型

- AWS 负责控制平面、etcd、API Server 高可用与底层控制平面补丁。
- 用户负责节点池、工作负载、网络策略、IAM、安全加固、升级与备份。

### 3.2 身份与访问

- **IRSA (IAM Roles for Service Accounts)**: 为 ServiceAccount 绑定 IAM Role，实现 Pod 级最小权限；依赖 OIDC Provider。
- **EKS Pod Identity**: 新一代方案，无需 OIDC Provider，使用 EKS Auth API，支持跨命名空间复用 Role，推荐新集群优先采用。

### 3.3 VPC CNI IP 管理

- VPC CNI 为 Pod 分配 VPC 子网 IP（ENI 辅助 IP）。
- 必须监控子网可用 IP，避免 `FailedCreatePodSandBox`。
- 在大规模集群中启用 `ENABLE_PREFIX_DELEGATION` 可显著提升 Pod 密度。
- 自定义网络（Custom Networking）可将 Pod IP 与节点 IP 分离，适用于 IP 规划受限的场景。

### 3.4 EKS Addons

- **vpc-cni**: 网络插件，需与 Kubernetes 版本匹配。
- **coredns**: 集群 DNS，升级时注意 DNS 中断窗口。
- **kube-proxy**: 服务代理，升级前确认 iptables/ipvs 模式。

---

## 4. 标准操作流程

### 4.1 集群创建（eksctl 生产模板）

推荐使用配置文件管理集群，便于 GitOps 与版本控制：

```yaml
# prod-eks.yaml
apiVersion: eksctl.io/v1alpha5
kind: ClusterConfig
metadata:
  name: prod-eks-tokyo
  region: ap-northeast-1
  version: "1.32"
  tags:
    Environment: production
    Team: platform
    CostCenter: platform-engineering
vpc:
  id: vpc-xxxx
  subnets:
    private:
      ap-northeast-1a: { id: subnet-xxx1 }
      ap-northeast-1c: { id: subnet-xxx2 }
      ap-northeast-1d: { id: subnet-xxx3 }
kubernetesNetworkConfig:
  ipFamily: IPv4
managedNodeGroups:
  - name: ng-general-1-32
    instanceType: m6i.xlarge
    desiredCapacity: 3
    minSize: 3
    maxSize: 20
    privateNetworking: true
    volumeSize: 100
    volumeType: gp3
    labels:
      workload-type: general
    tags:
      CostCenter: platform-engineering
addons:
  - name: vpc-cni
    version: latest
    configurationValues: '{"env":{"ENABLE_PREFIX_DELEGATION":"true"}}'
  - name: coredns
    version: latest
  - name: kube-proxy
    version: latest
cloudWatch:
  clusterLogging:
    enableTypes: ["api", "audit", "authenticator"]
secretsEncryption:
  keyARN: arn:aws:kms:ap-northeast-1:<account>:key/<key-id>
iam:
  withOIDC: true
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建集群
eksctl create cluster -f prod-eks.yaml

# 创建后验证
eksctl get cluster --name prod-eks-tokyo
kubectl get nodes -o wide
```
### 4.2 IRSA 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 创建 IAM OIDC Provider（若创建集群时已 --with-oidc 可省略）
eksctl utils associate-iam-oidc-provider --cluster prod-eks-tokyo --approve

# 2. 创建 IAM Policy 与 Role 并绑定 ServiceAccount
eksctl create iamserviceaccount \
  --name app-sa \
  --namespace production \
  --cluster prod-eks-tokyo \
  --attach-policy-arn arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess \
  --approve

# 3. 验证 ServiceAccount 注解
kubectl get sa app-sa -n production -o jsonpath='{.metadata.annotations.eks\.amazonaws\.com/role-arn}'
```
### 4.3 EKS Pod Identity 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 创建 Pod Identity 关联（无需 OIDC）
eksctl create podidentityassociation \
  --cluster prod-eks-tokyo \
  --namespace production \
  --service-account-name app-sa-v2 \
  --role-arn arn:aws:iam::<account>:role/EKSAppRole

# 2. 验证
aws eks list-pod-identity-associations --cluster-name prod-eks-tokyo
kubectl get sa app-sa-v2 -n production -o yaml
```
### 4.4 VPC CNI 升级与 IP 监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VPC CNI 版本
kubectl describe daemonset aws-node -n kube-system | grep Image

# 升级 VPC CNI（通过 eksctl addon）
eksctl update addon \
  --name vpc-cni \
  --cluster prod-eks-tokyo \
  --force

# 监控子网可用 IP（各子网保留 >20%）
aws ec2 describe-subnets \
  --subnet-ids subnet-xxx1 subnet-xxx2 subnet-xxx3 \
  --query 'Subnets[*].{ID:SubnetId,Available:AvailableIpAddressCount}'

# 查看节点可分配 IP 数
kubectl get node -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.vpc\.amazonaws\.com/PrivateIPv4Address}{"\n"}{end}'
```
### 4.5 节点池升级（蓝绿方式）

```bash
# 1. 查看可用 Kubernetes 版本
eksctl upgrade cluster --name prod-eks-tokyo --dry-run

# 2. 升级控制平面（不可逆， staging 验证后再执行）
eksctl upgrade cluster --name prod-eks-tokyo --version 1.33 --approve

# 3. 创建新版托管节点组
eksctl create nodegroup \
  --cluster prod-eks-tokyo \
  --name ng-1-33 \
  --node-type m6i.xlarge \
  --nodes-min 2 \
  --nodes-max 20 \
  --node-private-networking \
  --managed \
  --node-labels workload-type=general

# 4. 确认新节点 Ready 后，对旧节点组排水并删除
eksctl drain nodegroup --cluster prod-eks-tokyo --name ng-1-32 --approve
eksctl delete nodegroup --cluster prod-eks-tokyo --name ng-1-32 --approve
```

### 4.6 灾备与备份

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Velero 安装（使用 IRSA）
helm upgrade --install velero vmware-tanzu/velero \
  --namespace velero --create-namespace \
  --set configuration.provider=aws \
  --set configuration.backupStorageLocation.bucket=kudig-velero-backups \
  --set configuration.backupStorageLocation.region=ap-northeast-1 \
  --set configuration.backupStorageLocation.config.region=ap-northeast-1 \
  --set serviceAccount.server.name=velero \
  --set serviceAccount.server.create=false

# 2. 创建定时备份（每日 02:00，保留 30 天）
velero schedule create prod-daily \
  --schedule="0 2 * * *" \
  --include-namespaces production,monitoring \
  --ttl 720h0m0s

# 3. 异地复制备份（跨区域 S3 复制策略）
aws s3api put-bucket-replication \
  --bucket kudig-velero-backups \
  --replication-configuration file://replication.json

# 4. 恢复演练（映射到临时命名空间，避免覆盖生产）
velero restore create --from-backup prod-daily-$(date +%Y%m%d) \
  --include-namespaces production \
  --namespace-mappings production:production-drill
```
### 4.7 可观测性配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# CloudWatch Container Insights
eksctl utils update-cluster-logging --cluster prod-eks-tokyo --approve

# Prometheus 抓取 EKS 控制平面指标
kubectl apply -f - <<EOF
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: eks-control-plane
  namespace: monitoring
spec:
  endpoints:
  - port: https
    scheme: https
    tlsConfig:
      insecureSkipVerify: true
    bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
  selector: {}
  namespaceSelector:
    matchNames:
    - default
EOF

# Fluent Bit 日志转发 CloudWatch（建议通过 eksctl 启用）
eksctl utils update-cluster-logging \
  --cluster prod-eks-tokyo \
  --types api,audit,authenticator,controllerManager,scheduler \
  --enable-types all \
  --approve
```
### 4.8 成本治理

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看按标签分组的 EC2 成本（需启用 Cost Allocation Tags）
aws ce get-cost-and-usage \
  --time-period Start=$(date -d -7days +%Y-%m-%d),End=$(date +%Y-%m-%d) \
  --granularity DAILY \
  --metrics BlendedCost \
  --group-by Type=TAG,Key=Team

# 标记节点成本属性
kubectl label nodes -l nodegroup=spot-ng cost-center=platform team=backend env=prod

# 查看各命名空间资源申请
echo "=== CPU/Memory 申请 Top 20 ==="
kubectl top pods -A --containers | sort -k4 -nr | head -n 20

# 查看 Spot 与按需节点分布
kubectl get nodes -L eks.amazonaws.com/capacityType -o custom-columns='NAME:.metadata.name,TYPE:.metadata.labels.eks\.amazonaws\.com/capacityType'
```
### 4.9 安全加固要点

- 启用控制平面私有端点（private endpoint），限制公共访问。
- 使用 KMS 加密 etcd 与 EBS 卷。
- 节点组仅部署在私有子网，禁止直接公网 IP。
- 使用 Security Group 最小化节点间与外部访问。
- 开启 CloudTrail 与 EKS 审计日志。

### 4.10 升级前检查清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认所有工作负载 API 兼容性
kubectl get pods -A
kubectl get apiservices | grep -i false

# 确认 addon 版本兼容性
eksctl get addons --cluster prod-eks-tokyo

# 确认节点组可升级
eksctl upgrade nodegroup --cluster prod-eks-tokyo --name ng-general-1-32 --dry-run

# 确认备份可用
velero backup get | head -5
```
---

## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| 集群版本 | `eksctl get cluster --name prod-eks-tokyo` | 版本在 AWS 支持窗口内 |
| 节点就绪 | `kubectl get nodes -o wide` | 所有节点 Ready，版本一致 |
| IRSA 配置 | `kubectl get sa -A -o json \| jq '.items[].metadata.annotations'` | Pod 使用独立 SA 与 IAM Role |
| Pod Identity 关联 | `aws eks list-pod-identity-associations --cluster-name prod-eks-tokyo` | 关联存在且角色正确 |
| VPC CNI 健康 | `kubectl get pods -n kube-system -l app.kubernetes.io/name=aws-node` | 全节点 Running |
| 子网 IP 余量 | `aws ec2 describe-subnets ...` | 各子网可用 IP > 20% |
| 备份成功 | `velero backup get` | 最近 24h 备份 Complete |
| 审计日志 | `aws logs describe-log-groups --log-group-name-prefix /aws/eks/prod-eks-tokyo` | 日志组存在并写入 |
| 成本标签 | `kubectl get nodes --show-labels` | 关键标签已打 |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 处于 `Pending` | 子网 IP 耗尽或节点池无容量 | `aws ec2 describe-subnets` / `kubectl describe pod` | 扩容子网、启用 Prefix Delegation 或新增节点池 |
| `ImagePullBackOff` | ECR 权限或镜像不存在 | `kubectl describe pod` / `aws ecr describe-images` | 更新 IRSA/ECR policy；校验镜像 tag |
| 应用无法访问 AWS 服务 | IRSA Role 权限不足或 Trust Policy 错误 | `aws sts assume-role-with-web-identity` | 修复 IAM Role Trust Policy；更新 Policy |
| 节点 `NotReady` | kubelet/EC2 实例问题 | `kubectl describe node` / AWS Console | 替换节点；检查系统日志 |
| VPC CNI Pod CrashLoop | 版本不兼容或 IAM 权限不足 | `kubectl logs -n kube-system aws-node-xxx` | 升级 addon；附加正确 IAM policy |
| API Server 响应慢 | 控制平面负载高或大量 List | CloudWatch `APIServerRequestCount` | 限制客户端并发；升级控制平面 |
| 跨区域恢复失败 | 备份未跨区域复制或版本不兼容 | `velero backup get` / `velero restore logs` | 启用 S3 跨区域复制；验证版本兼容 |
| Pod Identity 鉴权失败 | 关联未同步或 Role 无权限 | `aws eks describe-pod-identity-association` | 重新创建关联；检查 Role policy |

---

## 7. 风险与注意事项

1. **控制平面升级不可回滚**: AWS EKS 控制平面升级后无法降级，升级前务必在 staging 验证所有工作负载 API 兼容性。
2. **IRSA Trust Policy**: 确保 `Federated` 与 `StringEquals` 条件准确，防止 Role 被其他集群冒用；新环境优先使用 Pod Identity。
3. **VPC CNI IP 规划**: 子网 CIDR 需预留 6 个月以上增长，大规模集群启用 Prefix Delegation。
4. **Spot 实例风险**: 关键控制面或数据库类负载避免使用 Spot 节点；为 Spot 工作负载配置 Pod 中断预算。
5. **Velero 备份加密**: S3 备份桶应启用服务端加密、版本控制与跨区复制，限制访问权限。
6. **成本标签一致性**: 在集群创建时就打上团队/环境/成本中心标签，避免后续补打困难。
7. **审计日志成本**: 开启全部控制平面日志会显著增加 CloudWatch 费用，根据合规需求选择类型。


## 4.11 多集群与 GitOps 管理

对于拥有多个 EKS 集群的组织，建议使用 Argo CD ApplicationSet 或 Flux 统一管理集群基线配置，建立集群版本矩阵并统一规划升级窗口。多集群场景下，应使用 AWS Organizations 与 IAM 角色链实现跨账号管理，避免在每个账号中重复配置 IAM Role。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出所有 EKS 集群
aws eks list-clusters --region ap-northeast-1

# 使用 eksctl 获取多个集群信息
for cluster in $(aws eks list-clusters --query 'clusters[]' --output text); do
  eksctl get cluster --name "$cluster"
done
```
---

## 8. 相关 Runbook / 推荐阅读

### 本域资料

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|云厂商托管 Kubernetes 生产就绪运维指南]]
- [[domain-12-cloud-providers/AWS-EKS/aws-eks-overview.md|AWS EKS 概述]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/02-aws-eks/01-eks-cluster-lifecycle-management|EKS 集群生命周期管理]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/02-aws-eks/04-eks-iam-irsa-pod-identity|EKS IAM/IRSA 与 Pod Identity]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/02-aws-eks/02-eks-networking-vpc-cni|EKS 网络与 VPC CNI]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/02-aws-eks/03-eks-storage-efs-fsx|EKS 存储 EFS/FSx]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-12-cloud-providers/02-aws-eks/05-eks-troubleshooting-playbook|EKS 故障排查手册]]

### 跨域参考

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|安全与合规生产就绪运维指南]] — IAM、NetworkPolicy、Secret 管理
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|可观测性生产就绪运维指南]] — CloudWatch、Prometheus、SLO
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|可靠性工程生产就绪运维指南]] — 灾备、RTO/RPO
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|生产运维生产就绪运维指南]] — 事件响应与变更管理

---

*AWS EKS 生产运维需要结合 AWS 服务与 Kubernetes 原生能力。建议将本手册中的命令封装为脚本或 Runbook Job，并在每次大版本升级前进行完整演练。*


<!-- risk-assessed -->
