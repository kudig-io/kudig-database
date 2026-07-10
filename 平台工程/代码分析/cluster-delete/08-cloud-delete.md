---
title: 云厂商集群删除方案对比 (topic-code-analysis)
description: 'title: 云厂商集群删除方案对比'
summary: 'title: 云厂商集群删除方案对比'
category: general
tags:
- reference
- etcd
- ingress
- gateway
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 云厂商集群删除方案对比 是什么
- 如何 云厂商集群删除方案对比
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 云厂商集群删除方案对比
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 云厂商集群删除方案对比
category: cluster-delete
tags:
- cloud-provider
- eks
- aks
- gke
- ack
- tke
- cluster-deletion
- comparison
last_updated: 2026-05-18
description: 对比分析 AWS EKS、Azure AKS、GCP GKE、阿里云 ACK、腾讯云 TKE 以及 kubeadm 自建集群的删除/销毁方案差异，涵盖删除命令、控制面清理、etcd
  处理和需要手动清理的资源。
difficulty: intermediate
intent_queries:
- eksctl delete cluster vs kubeadm reset
- gcp gke cluster deletion cleanup
- aliyun ack cluster delete vs manual cleanup
- cloud provider managed kubernetes deletion comparison
- kubernetes cluster teardown cloud vs self-managed
trigger_keywords:
- eksctl delete cluster
- az aks delete
- gcloud container clusters delete
- aliyun cs DELETE cluster
- tencentcloud delete-cluster
- kubeadm reset comparison
- cloud provider managed etcd
- EKS AKS GKE deletion
- ACK TKE cluster delete
- hybrid cluster deletion
reading_level: intermediate
audience:
- platform-engineer
- devops-engineer
- cloud-engineer
estimated_read_time: 5min
related_domains:
- 集群基础
- 集群基础
related_topics:
- cluster-delete
- reset-phase-commands
- cleanup
- security-delete
- network-cleanup
domain_link: '[Installation](../集群基础/README.md)'
topic_link: '[Cluster Delete Overview](./01-overview.md)'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 云厂商集群删除方案对比

## 概述

`cluster-create/10-cloud-comparison.md` 分析了各云厂商的集群创建方案。本文档补充其**删除/销毁**侧的对比分析，涵盖 EKS、AKS、GKE、ACK、TKE 以及 kubeadm 自建集群的删除差异。

---

## 删除方式对比

| 方案 | 删除命令 | 控制面清理 | etcd 处理 | Worker 清理 |
|------|---------|-----------|----------|-------------|
| kubeadm | `kubeadm reset` + 手动清理 | 需手动逐节点 | 需手动移除成员 | 需手动 |
| EKS | `eksctl delete cluster` | AWS 自动 | 托管，无需处理 | ASG 自动回收 |
| AKS | `az aks delete` | Azure 自动 | 托管，无需处理 | VMSS 自动回收 |
| GKE | `gcloud container clusters delete` | Google 自动 | 托管，无需处理 | MIG 自动回收 |
| ACK | `aliyun cs DELETE /clusters/<id>` | 阿里云自动 | 托管，无需处理 | ECS 自动释放 |
| TKE | `tencentcloud cli delete-cluster` | 腾讯云自动 | 托管，无需处理 | CVM 自动回收 |

---

## kubeadm 删除流程（对比基准）

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
┌──────────────────────────────────────────────────────────────────┐
│  kubeadm 集群删除（全手动）                                       │
├──────────────────────────────────────────────────────────────────┤
│  1. kubectl drain <node>         ← 手动驱逐                      │
│  2. kubectl delete node <node>   ← 手动删除 Node 对象             │
│  3. kubeadm reset -f             ← 手动在每台节点执行             │  # ⚠️ 清理节点所有 K8s 配置
│  4. etcdctl member remove        ← 手动移除 etcd 成员             │  # ⚠️ 移除 etcd 成员，可能丢数据
│  5. iptables/ipvs 清理           ← 手动清理网络规则               │
│  6. CNI/数据目录清理             ← 手动清理                       │
│  7. LB/DNS 清理                  ← 手动清理                       │
│                                                                   │
│  ⚠️ 每一步都需要人工介入，容易遗漏                               │
└──────────────────────────────────────────────────────────────────┘
```
### 混合场景：云托管 + kubeadm Worker 节点

---

## EKS 集群删除

### eksctl 方式

```bash
eksctl delete cluster --name my-cluster --region us-west-2
```

**自动处理**:

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌──────────────────────────────────────────────────────────────┐
│  eksctl delete cluster 自动化                                 │
├──────────────────────────────────────────────────────────────┤
│  1. cordon + drain 所有节点                                   │
│  2. 删除 NodeGroup / Fargate Profile                          │
│  3. 删除 CloudFormation Stack                                 │  │
│  │   ├─ IAM Role / Policy                                    │
│  │   ├─ Security Group                                       │
│  │   ├─ VPC / Subnet（如果是 eksctl 创建的）                  │
│  │   └─ EBS Volume                                           │
│  4. 删除 EKS Control Plane（AWS 托管 etcd 自动清理）          │
│  5. 删除 kubeconfig 条目                                      │
└──────────────────────────────────────────────────────────────┘
```
### 需要手动清理的资源

| 资源 | 说明 |
|------|------|
| EBS CSI 持久卷 | `aws ec2 delete-volume` |
| NLB/ALB | `aws elbv2 delete-load-balancer` |
| ECR 镜像 | `aws ecr delete-repository` |
| CloudWatch Log Group | `aws logs delete-log-group` |
| EFS 文件系统 | `aws efs delete-file-system` |

### AWS Console 方式

```
EKS Console → 选择集群 → Delete → 输入集群名确认
```

**注意**: Console 删除只删除 EKS 集群本身，不删除 VPC/IAM 等关联资源。

---

## AKS 集群删除

### Azure CLI 方式

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
az aks delete --name my-cluster --resource-group my-rg --yes
```
**自动处理**:

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
┌──────────────────────────────────────────────────────────────┐
│  az aks delete 自动化                                         │
├──────────────────────────────────────────────────────────────┤
│  1. 删除所有 Node Pool (VMSS)                                 │
│  2. 删除托管 Control Plane                                    │
│  3. 删除托管 etcd                                             │
│  4. 清理 MC_* 资源组中的资源                                  │
│     ├─ Public IP                                             │
│     ├─ Load Balancer                                         │
│     ├─ Route Table                                           │
│     ├─ NSG (Network Security Group)                          │
│     └─ VMSS 实例                                             │
│  5. 删除 MC_* 资源组本身                                      │
└──────────────────────────────────────────────────────────────┘
```
### 需要手动清理的资源

| 资源 | 命令 |
|------|------|
| Azure Disk | `az disk delete` |
| Azure File | `az storage share delete` |
| ACR | `az acr delete` |
| Log Analytics | `az monitor log-analytics workspace delete` |

---

## GKE 集群删除

### gcloud 方式

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
gcloud container clusters delete my-cluster --zone=us-central1-a --quiet
```
**自动处理**:

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
┌──────────────────────────────────────────────────────────────┐
│  gcloud container clusters delete 自动化                      │
├──────────────────────────────────────────────────────────────┤
│  1. 删除所有 Node Pool (MIG)                                  │
│  2. 删除 Compute Engine 实例                                  │
│  3. 删除托管 Control Plane                                    │
│  4. 删除 etcd                                                │
│  5. 清理 Firewall Rules                                      │
│  6. 清理 Health Check                                        │
│  7. 清理 Backend Service                                     │
│  8. 删除 kubeconfig 条目                                      │
└──────────────────────────────────────────────────────────────┘
```
### 需要手动清理的资源

| 资源 | 命令 |
|------|------|
| Persistent Disk | `gcloud compute disks delete` |
| GCS Bucket | `gsutil rm -r gs://bucket` |
| GCR 镜像 | `gcloud container images delete` |
| Cloud Armor Policy | `gcloud compute security-policies delete` |

---

## ACK 集群删除（阿里云）

### aliyun CLI 方式

```bash
aliyun cs DELETE /clusters/<cluster-id> --body '{"retain_resources": false}'
```

**自动处理**:

```
┌──────────────────────────────────────────────────────────────┐
│  ACK 集群删除自动化                                           │
├──────────────────────────────────────────────────────────────┤
│  1. 驱逐所有节点上的 Pod                                      │
│  2. 释放所有 ECS Worker 节点                                  │
│  3. 删除托管 Control Plane                                    │
│  4. 清理 SLB 实例                                             │
│  5. 清理 NAT Gateway                                          │
│  6. 清理安全组                                                │
│  7. (可选) 释放 VPC/交换机                                    │
└──────────────────────────────────────────────────────────────┘
```

### 需要手动清理的资源

| 资源 | 命令 |
|------|------|
| 云盘 (EBS) | `aliyun ecs DeleteDisk` |
| NAS 文件系统 | `aliyun nas DeleteFileSystem` |
| ACR 镜像 | `aliyun cr DeleteRepository` |
| SLS Project | `aliyun sls DeleteProject` |
| EIP | `aliyun vpc ReleaseEipAddress` |

### ACK 专有集群 vs 托管集群

```
┌──────────────────────────────────────────────────────────────┐
│  托管集群 (Managed)                                            │
│  ├─ 控制面由阿里云管理，删除时自动清理                         │
│  └─ etcd 由阿里云管理，无需手动处理                            │
│                                                                │
│  专有集群 (Dedicated)                                          │
│  ├─ 控制面在用户 ECS 上运行                                    │
│  ├─ 删除时需要手动处理 etcd（类似 kubeadm）                    │
│  └─ 需要手动清理控制面 ECS 实例                                │
└──────────────────────────────────────────────────────────────┘
```

---

## TKE 集群删除（腾讯云）

### tencentcloud CLI 方式

```bash
tencentcloud cli cvm DeleteCluster --cluster-id cls-xxx
```

**自动处理**:

```
┌──────────────────────────────────────────────────────────────┐
│  TKE 集群删除自动化                                           │
├──────────────────────────────────────────────────────────────┤
│  1. 驱逐所有 Pod                                              │
│  2. 释放所有 CVM Worker 节点                                  │
│  3. 删除托管 Control Plane                                    │
│  4. 清理 CLB 实例                                             │
│  5. 清理安全组                                                │
│  6. 清理 VPC 路由                                             │
└──────────────────────────────────────────────────────────────┘
```

### 需要手动清理的资源

| 资源 | 命令 |
|------|------|
| CBS 云盘 | `tencentcloud cli cbs DeleteDisks` |
| COS 对象 | `coscli rm -r cos://bucket` |
| TCR 镜像 | `tencentcloud cli tcr DeleteRepository` |
| CLS 日志 | `tencentcloud cli cls DeleteLogset` |

---

## 删除耗时对比

| 方案 | 单节点 | 3 节点 HA | 全托管 |
|------|--------|----------|--------|
| kubeadm | 5-10 min/节点 | 30-60 min | — |
| EKS | — | — | 10-20 min |
| AKS | — | — | 5-15 min |
| GKE | — | — | 5-15 min |
| ACK | — | — | 10-20 min |
| TKE | — | — | 10-20 min |

**kubeadm 最慢的原因**: 每一步都需要人工介入，且需要等待 etcd 成员移除、Pod 驱逐等操作。

---

## 删除风险对比

| 风险 | kubeadm | 云托管 |
|------|---------|--------|
| etcd 数据丢失 | 手动操作容易出错 | 云厂商自动备份 |
| 残留资源 | 容易遗漏 iptables/CNI/数据 | 云厂商自动清理 |
| 网络规则残留 | 需手动清理 | 自动回收 |
| 云资源泄露 | N/A | 可能残留 EBS/Disk/LB |
| 数据不可恢复 | 无备份机制（除非手动备份） | 云厂商有快照功能 |

---

## 混合场景：云托管 + kubeadm Worker 节点

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
┌──────────────────────────────────────────────────────────────┐
│  混合集群删除流程                                              │
├──────────────────────────────────────────────────────────────┤
│                                                                │
│  Step 1: 删除 kubeadm 管理的 Worker 节点                      │
│    kubectl drain <self-managed-node>                           │
│    kubectl delete node <self-managed-node>                     │
│    ssh <node> "kubeadm reset -f"                              │  # ⚠️ 清理节点所有 K8s 配置
│    ssh <node> "iptables -F && rm -rf /etc/cni/net.d"         │  # ⚠️ 删除系统/数据文件
│                                                                │
│  Step 2: 删除云托管集群                                       │
│    eksctl delete cluster / az aks delete / ...                │
│                                                                │
│  ⚠️ 先删自管理节点，再删托管集群                               │
│  ⚠️ 自管理节点的网络/安全组需手动清理                          │
└──────────────────────────────────────────────────────────────┘
```
---

## 最佳实践

### kubeadm 自建集群删除检查清单

```
# 🟢 低风险：只读/信息收集，通常无副作用
□ 备份 etcd 数据（etcdctl snapshot save）
□ 备份关键应用数据（PV 快照）
□ 通知相关团队（集群即将下线）
□ drain 所有 Worker 节点
□ 逐个 reset Worker 节点
□ 逐个 reset 控制面节点（维护 etcd 仲裁）
□ 清理负载均衡器
□ 清理 DNS 记录
□ 清理防火墙规则
□ 清理监控/告警配置
□ 清理证书（如果使用了 Vault/CFSSL）
□ 确认云资源已全部释放
```
### 云托管集群删除检查清单

```
# 🟢 低风险：只读/信息收集，通常无副作用
□ 备份 Persistent Volume 数据
□ 导出集群配置（kubectl get all -A --dry-run -o yaml）
□ 删除非托管资源（Ingress LB、自定义 DNS）
□ 执行云厂商删除命令
□ 确认云资源已全部释放（检查账单）
□ 清理 kubeconfig
□ 清理 CI/CD 中的集群配置
```
---

## 参考

- [eksctl delete cluster](https://eksctl.io/usage/deleting-clusters/)
- [az aks delete](https://learn.microsoft.com/en-us/cli/azure/aks#az-aks-delete)
- [gcloud container clusters delete](https://cloud.google.com/sdk/gcloud/reference/container/clusters/delete)
- [ACK 集群删除](https://www.alibabacloud.com/help/zh/ack/)
- [TKE 集群删除](https://cloud.tencent.com/document/product/457/)

### 云厂商删除命令汇总

| 云厂商 | 命令 | 文档链接 |
|--------|------|----------|
| AWS EKS | `eksctl delete cluster --name <name> --region <region>` | [eksctl 文档](https://eksctl.io/usage/deleting-clusters/) |
| Azure AKS | `az aks delete --name <name> --resource-group <rg> --yes` | [Azure 文档](https://learn.microsoft.com/en-us/cli/azure/aks#az-aks-delete) |
| GCP GKE | `gcloud container clusters delete <name> --zone=<zone> --quiet` | [GCP 文档](https://cloud.google.com/sdk/gcloud/reference/container/clusters/delete) |
| 阿里云 ACK | `aliyun cs DELETE /clusters/<cluster-id>` | [ACK 文档](https://help.aliyun.com/zh/ack/ack-enterprise-user-guide/delete-a-cluster-1) |
| 腾讯云 TKE | `tencentcloud cli cvm DeleteCluster --cluster-id <id>` | [TKE 文档](https://cloud.tencent.com/document/product/457) |

### 删除失败常见原因与处理

| 场景 | 原因 | 处理 |
|------|------|------|
| 资源保护 | 云资源启用删除保护 | 禁用保护后重试 |
| 依赖资源 | LB 仍关联后端 | 先删除 LB |
| 权限不足 | IAM 权限不足 | 提升权限后重试 |
| 异步删除 | 删除操作异步执行 | 等待几分钟后重试 |
| 账单未结清 | 仍有未结清账单 | 结清账单后重试 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[README|README]]
- [[log|log]]
- [[scripts/man/INSTALL.md|INSTALL]]
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]


<!-- risk-assessed -->
