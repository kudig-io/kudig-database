---
title: 华为云 CCE 生产运行手册
description: 面向 SRE 的华为云 CCE 集群全生命周期生产运维、IAM 工作负载身份、VPC 网络、节点池、升级、灾备、AOM 可观测性、成本治理与故障排查 Runbook
summary: 华为云 CCE 生产运行手册，覆盖集群创建与基线加固、IAM/Agency 工作负载身份、VPC 网络、控制平面与节点池升级、灾备、AOM 可观测性、成本治理与常见故障 remediation。
category: cloud-provider
tags:
- production
- best-practices
- playbook
- cloud-provider
- huawei-cce
- cce
- iam
- vpc
- node-pool
- disaster-recovery
- aom
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
estimated_read_time: 25min
intent_queries:
- 华为云 CCE 生产运行手册是什么
- 如何运维华为云 CCE 生产集群
- CCE IAM 工作负载身份、VPC 网络、升级与灾备怎么做
- CCE AOM 可观测性与成本治理
trigger_keywords:
- 华为云 CCE
- CCE
- 运行手册
- IAM
- Agency
- VPC 网络
- 节点池
- 升级
- 灾备
- AOM
- 成本治理
prerequisites:
- kubectl-basics
- huawei-cloud-cli-basics
- networking-basics
- cce-overview
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


# 华为云 CCE 生产运行手册

> **适用范围**: 华为云 CCE 上运行生产负载的集群，覆盖集群搭建、身份管理、网络、升级、灾备、可观测性、成本与排障。  
> **目标读者**: SRE、平台工程师、华为云架构师。  
> **最后更新**: 2026-07-01

本手册聚焦华为云 CCE（Cloud Container Engine）生产运维的可执行命令与标准流程。CCE 作为华为云托管 Kubernetes 服务，在控制平面高可用、节点管理、网络、存储、安全与可观测性方面与开源 Kubernetes 有较大差异。建议与 [[domain-12-cloud-providers/99-production-readiness-operations-guide.md|云厂商托管 Kubernetes 生产就绪运维指南]] 配套使用，形成跨云一致的 SRE 操作基线。

---

## 1. 适用场景与范围

### 1.1 适用场景

- **生产集群创建与基线加固**: 私有子网、KMS Secret 加密、企业项目隔离、安全组、审计日志投递。
- **IAM / Agency 工作负载身份配置**: 将华为云 IAM 委托映射到 Kubernetes ServiceAccount，实现 Pod 级最小权限。
- **VPC 网络生命周期管理**: 容器隧道网络、VPC 网络、Cloud Native 2.0 的选择、IP 规划、路由、安全组与 NetworkPolicy。
- **节点池设计与管理**: 多 AZ 节点池、弹性伸缩、污点/标签、系统池与业务池分离、异构计算（鲲鹏 / 昇腾 / x86）。
- **控制平面与节点池升级**: 版本规划、版本偏斜检查、滚动升级、回滚预案。
- **灾备与备份**: etcd/集群配置备份、EVS 快照、跨 AZ/Region 恢复、RTO/RPO 验证。
- **AOM 可观测性**: 指标、日志、链路追踪、告警、仪表盘与 SLO 管理。
- **成本治理**: 标签分账、节点池右移、Spot/竞价实例、自动伸缩策略、FinOps 实践。
- **故障排查**: 节点 NotReady、Pod 调度失败、网络不通、存储挂载失败、Agency 授权失败等。

### 1.2 不适用场景

- 本地数据中心、私有云 HCS Online 或其他云厂商 Kubernetes，仅部分理念可参考。
- 云容器实例 CCI（Serverless 容器）的运维模型，本手册仅涉及 CCE 托管集群。
- 底层 IaaS（ECS、BMS、VPC、ELB）的通用运维，可参考华为云官方文档。

---

## 2. 前置条件与工具

| 工具/资源 | 版本/要求 | 用途 |
|---|---|---|
| 华为云 CLI (hcloud) | ≥ v1.0.0 | IAM、VPC、CCE、ECS、EVS、OBS 管理 |
| kubectl | 与 CCE 版本匹配 | K8s 资源管理 |
| Helm 3 | ≥ 3.12 | 组件部署 |
| obsutil / OBS Browser+ | 已配置 | 备份、日志归档、跨区复制 |
| AOM / APM / LTS | 已接入 | 指标、日志、链路、告警 |
| 华为云账号 | 具备 CCE 管理员、IAM 管理员、VPC 只读权限 | 资源操作与审计 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证工具版本
hcloud version
kubectl version --client
helm version

# 验证当前凭证、项目与区域
hcloud iam user show
hcloud configure get region
hcloud configure get project-id

# 获取集群凭证（推荐创建 RBAC 用户，避免使用 admin 证书长期落地）
hcloud cce cluster kubeconfig \
  --cluster <CLUSTER_ID> \
  --kb-cfg-path ~/.kube/cce-prod-config

export KUBECONFIG=~/.kube/cce-prod-config
kubectl config current-context
```

---

## 3. 核心概念/架构

### 3.1 CCE 责任共担模型

| 责任层 | 华为云负责 | 用户负责 |
|---|---|---|
| 控制平面 | 托管 Master、etcd、API Server、Scheduler、Controller Manager 高可用 | 升级窗口、版本选择、废弃 API 扫描 |
| 数据平面 | 节点镜像、底层 ECS/BMS 维护 | 节点池设计、扩缩容、污点标签、Pod 调度 |
| 网络 | VPC、子网、ELB、CNI 插件基础能力 | CIDR 规划、安全组、NetworkPolicy、ELB 监听 |
| 存储 | EVS/SFS/OBS 后端服务 | StorageClass、PVC、快照策略、备份 |
| 安全 | IAM 服务、KMS | Agency 授权、RBAC、Secret 加密、POD 安全标准 |
| 可观测性 | AOM/LTS/APM 平台 | Agent 部署、告警规则、仪表盘、SLO |

### 3.2 工作负载身份

CCE 推荐通过 **IAM Agency（委托）** 为 Pod 授予华为云资源权限，避免在容器中使用长期 AK/SK：

- **IAM Agency + 节点委托**: 在节点级别绑定 Agency，所有 Pod 共享节点权限。配置简单但权限粒度过粗，生产环境不推荐。
- **IAM Agency + ServiceAccount 映射**: 通过 `cce.io/agency-name` 等注解将 Namespace/ServiceAccount 与 Agency 绑定，实现 Pod 级最小权限。Cloud Native 2.0 与较新 CCE 版本支持更细粒度的映射。
- **IAM 用户 AK/SK**: 通过 Secret 挂载到 Pod，仅允许遗留系统临时过渡，必须定期轮换并设置过期提醒。

生产建议：所有新应用统一使用 Agency + ServiceAccount 映射；已有 AK/SK 应用制定迁移计划，逐步下线。

### 3.3 CCE 网络模型

| 网络模型 | 封装方式 | 性能 | 适用场景 |
|---|---|---|---|
| 容器隧道网络 | VXLAN 封装 | 中 | 通用场景、快速交付、对 VPC IP 无强需求 |
| VPC 网络 | VPC 路由 | 高 | 需要 Pod 与 VPC 原生互通、较高吞吐 |
| Cloud Native 2.0 | 独占弹性网卡（ENI） | 极高 | 低延迟、高吞吐、大规模、需要直连 ELB |

**关键约束**:

- 网络模型在集群创建后不可更改。
- 容器 CIDR（Pod CIDR）、Service CIDR、VPC CIDR 必须互不重叠。
- Cloud Native 2.0 对 VPC 子网 IP 数量要求更高，需要提前规划 ENI/辅助 IP。

### 3.4 CCE Add-on

- **Everest**: CCE 核心存储插件，对接 EVS/SFS/OBS，负责动态卷供给、挂载、扩容与快照。
- **CoreDNS**: 集群 DNS，升级时注意缓存 TTL 与解析中断窗口。
- **kube-proxy / CNI**: 与网络模型强相关，升级前必须确认 CCE 版本矩阵兼容性。
- **AOM / ICAgent / CCE 监控插件**: 负责指标、日志、链路数据采集。

---

## 4. 标准操作流程

### 4.1 集群创建（生产模板）

推荐通过 Terraform 或 hcloud CLI 管理集群，便于 GitOps 与版本控制：

``` bash
# 🟡 中风险：会创建/修改云资源，执行前请确认项目、计费与网络规划
hcloud cce cluster create \
  --name prod-cce-hk \
  --region ap-southeast-1 \
  --flavor cce.s2.small \
  --vpc-id <VPC_ID> \
  --subnet-id <SUBNET_ID> \
  --container-network-mode vpc-router \
  --container-cidr 172.16.0.0/16 \
  --service-cidr 192.168.0.0/16 \
  --kubernetes-version v1.30 \
  --cluster-type vm \
  --description "Production CCE cluster" \
  --enterprise-project-id <PROJECT_ID> \
  --tags env=production,team=platform
```

创建后必须立即加固：

``` bash
# 🟡 中风险：会修改集群访问策略与加密配置
# 开启审计日志并投递到 LTS
hcloud cce cluster update <CLUSTER_ID> \
  --extend-param '{"clusterCASpectrumAudit":{"auditType":"security"}}'

# 开启 Secret 加密（KMS）
hcloud cce cluster update <CLUSTER_ID> \
  --encryption-config '{"enable":true,"kmsKeyId":"<KMS_KEY_ID>"}'

# 配置 API Server 授权 IP 范围（若使用公网访问）
hcloud cce cluster update <CLUSTER_ID> \
  --spec '{"spec":{"publicAccess":{"cidrs":["<OFFICE_CIDR>/32"]}}}'
```

### 4.2 配置 IAM Agency 工作负载身份

``` bash
# 🟡 中风险：会修改 IAM 与 K8s 资源
# 1. 创建 Agency 并授权 OBS 只读
hcloud iam agency create \
  --name cce-app-obs-reader \
  --domain-id <DOMAIN_ID> \
  --trust-policy '{"trust_domain_id":"<DOMAIN_ID>","trust_policy":[]}'

hcloud iam agency policy attach \
  --agency-name cce-app-obs-reader \
  --role-name "Tenant Guest" \
  --scope project <PROJECT_ID>

# 2. 创建 ServiceAccount 并映射 Agency
kubectl create serviceaccount app-sa -n prod
kubectl annotate serviceaccount app-sa -n prod \
  cce.io/agency-name=cce-app-obs-reader

# 3. 部署示例 Pod 验证临时凭证
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: test-agency
  namespace: prod
spec:
  serviceAccountName: app-sa
  containers:
  - name: obsutil
    image: swr.ap-southeast-1.myhuaweicloud.com/library/obsutil:latest
    command: ["obsutil", "ls", "obs://<BUCKET>"]
  restartPolicy: Never
EOF

kubectl logs test-agency -n prod
```

### 4.3 节点池管理

``` bash
# 🟡 中风险：会修改集群节点数量与计费
# 创建多 AZ 通用节点池
hcloud cce nodepool create \
  --cluster-id <CLUSTER_ID> \
  --name general-pool \
  --node-flavor c7n.2xlarge.2 \
  --availability-zone ap-southeast-1a,ap-southeast-1b,ap-southeast-1c \
  --os euleros \
  --root-volume-size 100 \
  --root-volume-type SAS \
  --data-volumes '[{"size":200,"volumetype":"SSD"}]' \
  --initial-node-count 3 \
  --min-node-count 3 \
  --max-node-count 20 \
  --scale-down-cooldown 10 \
  --tags workload=general

# 标记 system 节点池污点，仅运行核心 Add-on
kubectl taint nodes -l nodepool=system-pool CriticalAddonsOnly=true:NoSchedule
```

节点池设计原则：

- **系统节点池**: 承载 kube-system、monitoring、ingress 等核心组件，设置 `CriticalAddonsOnly=true:NoSchedule` 污点。
- **业务节点池**: 按业务域、环境、硬件架构划分，例如 `general`、`spot`、`gpu-ascend`、`arm-kunpeng`。
- **多 AZ 分布**: 关键节点池必须跨至少两个可用区，避免单 AZ 故障。
- **弹性边界**: 设置 `min-node-count` 与 `max-node-count`，防止异常伸缩导致费用失控。

### 4.4 升级 CCE

CCE 控制平面升级不可逆，必须先在 staging 验证完整路径。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查询当前版本与可用升级版本
hcloud cce cluster show <CLUSTER_ID> --query 'spec.version'
hcloud cce cluster upgrade-info <CLUSTER_ID>

# 升级前置检查
kubectl get nodes -o wide
kubectl get pdb -A
pluto detect-helm --target-versions k8s=v1.31
kubectl get --raw /apis | jq -r '.groups[].name' | sort

# 🟡 中风险：会触发控制平面/节点重建
# 升级控制平面
hcloud cce cluster upgrade \
  --cluster-id <CLUSTER_ID> \
  --target-version v1.31 \
  --upgrade-strategy in-place

# 升级节点池
hcloud cce nodepool upgrade \
  --cluster-id <CLUSTER_ID> \
  --nodepool-id <POOL_ID> \
  --target-version v1.31
```

升级后验证：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl version
kubectl get nodes -o wide
kubectl get pods -A
kubectl get events --sort-by='.lastTimestamp' | tail -n 50
```

### 4.5 灾备与备份

CCE 生产环境必须同时覆盖控制平面配置、PersistentVolume 数据与关键 Secret 的备份：

``` bash
# 🟡 中风险：会创建快照、备份任务与跨区复制
# 1. 配置 EVS 快照 StorageClass
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: cce-evs-snapshot
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"
driver: csi.huaweicloud.com
deletionPolicy: Retain
parameters:
  csi.storage.k8s.io/snapshot-initial-time: "false"
EOF

# 2. 创建业务 PVC 快照示例
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: prod-db-snapshot-20260701
  namespace: prod
spec:
  volumeSnapshotClassName: cce-evs-snapshot
  source:
    persistentVolumeClaimName: prod-db-pvc
EOF

# 3. 跨 Region 复制关键 PVC 快照到 OBS
obsutil cp obs://<SRC_BUCKET>/pvc-snapshots/ obs://<DST_BUCKET>/pvc-snapshots/ -r -f

# 4. 定期导出集群关键资源配置
kubectl get all,configmap,secret,ingress,networkpolicy -A -o yaml > cce-prod-manifest-backup.yaml
obsutil cp cce-prod-manifest-backup.yaml obs://<BACKUP_BUCKET>/manifests/
```

灾备演练要求：每季度至少执行一次快照恢复演练，验证 RTO/RPO 是否满足业务 SLA。

### 4.6 AOM 可观测性接入

``` bash
# 🟡 中风险：会创建告警规则与仪表盘对象
# 1. 在 CCE 控制台确认已安装 AOM 插件；CLI 安装/升级示例
hcloud cce addon install \
  --cluster-id <CLUSTER_ID> \
  --addon-template-name aom-agent \
  --addon-version <VERSION>

# 2. 配置 ServiceMonitor（若使用 Prometheus Operator）
kubectl apply -f - <<EOF
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: prod-app-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: prod-app
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
EOF

# 3. 配置 LTS 日志采集（通过 CRD 或控制台）
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: lts-log-config
  namespace: kube-system
data:
  log-paths.json: |
    {
      "logs": [
        {"path": "/var/log/containers/*.log", "group": "cce-container"}
      ]
    }
EOF
```

核心告警建议：

| 告警名称 | 阈值建议 | 优先级 |
|---|---|---|
| 节点 NotReady | 任意节点持续 5 分钟 | P1 |
| Pod 重启次数 | 5 分钟内 > 3 次 | P2 |
| 节点磁盘使用率 | > 85% | P2 |
| Pod CPU 限流 | > 10% 持续 10 分钟 | P3 |
| API Server 延迟 | P99 > 1s 持续 5 分钟 | P1 |

### 4.7 成本治理

``` bash
# 🟢 低成本：只读/信息收集，通常无副作用
# 查询按标签分账的节点与 Pod 资源
kubectl get nodes --show-labels
kubectl top nodes
kubectl top pods -A
```

成本治理措施：

- **标签与项目隔离**: 集群、节点池、Namespace 统一打 `env`、`team`、`cost-center` 标签，结合企业项目做分账。
- **弹性伸缩**: 非核心负载使用竞价实例节点池；核心负载使用按需实例，配置 HPA + Cluster Autoscaler。
- **右移与清理**: 每月 review `kubectl top` 与 AOM 资源利用率报告，下调 request/limit；清理未使用 PVC、LoadBalancer、快照。
- **Spot 节点池示例**:

``` bash
hcloud cce nodepool create \
  --cluster-id <CLUSTER_ID> \
  --name spot-pool \
  --node-flavor c7n.2xlarge.2 \
  --billing-mode spot \
  --initial-node-count 0 \
  --min-node-count 0 \
  --max-node-count 50 \
  --tags workload=spot-batch
```

---

## 5. 关键检查点与验证命令

| 检查项 | 验收标准 | 推荐命令 |
|---|---|---|
| 控制平面高可用 | 多 AZ Master、版本在支持窗口 | `hcloud cce cluster show <ID>` |
| 节点池跨 AZ | 关键业务 Pod 跨可用区分布 | `kubectl get nodes -L topology.kubernetes.io/zone` |
| IP 余量 | Pod/Service CIDR 余量 ≥ 20% | `kubectl get nodes -o jsonpath='{.items[*].spec.podCIDR}'` |
| Agency 映射 | Pod 使用独立 Agency，无节点长期密钥 | `kubectl get sa -A -o yaml \| grep cce.io/agency-name` |
| 存储快照 | VolumeSnapshotClass 就绪、快照策略生效 | `kubectl get sc,volumesnapshotclass` |
| 可观测性 | AOM/LTS 接入、核心告警生效 | `kubectl get pods -n monitoring`；AOM 控制台告警列表 |
| 备份 | etcd/配置/关键 PVC 备份完成 | 控制台备份任务 + OBS 桶校验 |
| 成本标签 | 节点池/集群标签完整 | `kubectl get nodes --show-labels` |
| Secret 加密 | KMS 加密已启用 | `hcloud cce cluster show <ID> --query 'spec.encryptionConfig'` |
| NetworkPolicy | 核心命名空间已配置默认拒绝 | `kubectl get networkpolicies -A` |

---

## 6. 常见故障与 remediation

### 6.1 节点 NotReady

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes
kubectl describe node <NODE_NAME>
kubectl get events --field-selector reason=FailedMount

# 检查节点系统日志
hcloud cce node show --cluster-id <CLUSTER_ID> --node-id <NODE_ID>

# 若磁盘压力或 kubelet 异常，可重启节点
# 🟡 中风险：会重建节点
hcloud cce node reboot --cluster-id <CLUSTER_ID> --node-id <NODE_ID>
```

常见原因：ECS 底层故障、kubelet 证书过期、容器运行时异常、磁盘压力、网络插件异常。

### 6.2 Pod 无法调度（IP 耗尽）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get events --field-selector reason=FailedCreatePodSandBox
kubectl describe pod <POD> -n prod

# 检查子网可用 IP
hcloud vpc subnet show <SUBNET_ID>

# Cloud Native 2.0 场景下检查弹性网卡余量
kubectl exec -n kube-system ds/cce-cni-daemon -- /opt/cni/bin/show-eni
```

解决方案：扩展 VPC 子网、新增节点池、切换为更大 CIDR 的新集群。

### 6.3 Agency 授权失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get sa app-sa -n prod -o yaml
kubectl get pod <POD> -n prod -o jsonpath='{.spec.serviceAccountName}'
hcloud iam agency show --name cce-app-obs-reader
hcloud iam agency policy list --agency-name cce-app-obs-reader
```

排查顺序：Pod 是否使用正确 SA → SA 注解是否正确 → Agency 是否存在 → Agency 是否被授予正确 IAM 角色 → Agency 作用域是否覆盖目标资源。

### 6.4 存储挂载失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pvc -A
kubectl describe pvc <PVC> -n prod
kubectl logs -n kube-system -l app=everest-csi-driver
kubectl logs -n kube-system -l app=everest-csi-node
```

常见原因：EVS 盘未 detach 即被其他 Pod 复用、StorageClass 参数错误、节点与 EVS 可用区不一致、CSI 插件版本不兼容。

### 6.5 控制平面 API 延迟高

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get --raw /metrics | grep apiserver_request_duration_seconds
kubectl top nodes -l node-role.kubernetes.io/master=
hcloud cce cluster show <CLUSTER_ID>
```

缓解措施：限制大规模 List/Watch 客户端、调整 etcd 磁盘类型、分散大型 CronJob 调度时间。

---

## 7. 风险与注意事项

1. **网络模型不可变更**: 集群创建后无法从容器隧道网络切换为 Cloud Native 2.0，必须在创建前根据业务延迟、吞吐与规模选定。建议高性能生产场景直接选择 Cloud Native 2.0。
2. **版本升级不可逆**: 控制平面升级后无法回退到旧版本，务必在 staging 验证并保留应用级回滚能力（蓝绿/金丝雀）。
3. **IAM Agency 作用域**: Agency 可授权到项目或账号级别，生产环境严格限制到项目级最小权限，避免跨项目越权。
4. **KMS 密钥轮换**: 启用 Secret 加密后，KMS 密钥删除将导致 etcd 中 Secret 无法解密。须开启 KMS 自动轮换并备份密钥 ID，禁止手动删除生产 KMS 密钥。
5. **成本失控**: 未配置弹性伸缩上下限、未打标签的节点池容易造成费用黑洞。建议结合企业项目 + 资源标签做分账，并设置预算告警。
6. **AK/SK 禁用**: 禁止在容器镜像、ConfigMap、环境变量中存放长期 AK/SK；所有新应用必须使用 Agency 工作负载身份。
7. **安全组配置**: 节点安全组切勿开放 0.0.0.0/0 到 kubelet、etcd、Docker 端口；仅允许 VPC 内必要网段访问。
8. **审计日志保留**: 审计日志建议保留至少 180 天，并投递到 LTS 长期存储，满足合规与事后溯源需求。

---

## 8. 相关 Runbook / 推荐阅读

- [[domain-12-cloud-providers/99-production-readiness-operations-guide.md|云厂商托管 Kubernetes 生产就绪运维指南]]
- [[domain-12-cloud-providers/07-huawei-cce/huawei-cce-overview.md|华为云 CCE 企业级深度实战指南]]
- [[domain-12-cloud-providers/07-huawei-cce/02-cce-networking-vpc-router.md|CCE 网络模型与 VPC 路由深度解析]]
- [[domain-12-cloud-providers/07-huawei-cce/04-cce-iam-aad-integration.md|CCE 身份认证与 IAM 细粒度授权]]
- [[domain-12-cloud-providers/07-huawei-cce/03-cce-storage-evs-sfs.md|CCE 存储架构：EVS、SFS 与 OBS 集成]]
- [[domain-12-cloud-providers/07-huawei-cce/05-cce-troubleshooting-playbook.md|CCE 故障排查手册]]
- [[domain-09-reliability-engineering/02-disaster-recovery/18-cross-region-disaster-recovery.md|跨 Region 灾难恢复指南]]
- [[domain-06-observability/01-overview/01-observability-architecture-overview.md|可观测性架构概述]]
- [[domain-11-production-operations/01-finops/13-kubernetes-cost-governance.md|Kubernetes 成本治理]]
- [[domain-01-cluster-fundamentals/03-control-plane/35-cluster-upgrade-runbook.md|集群升级 Runbook]]

---

*本文件定义华为云 CCE 生产运行手册。修改本文件会影响 CCE 集群运维流程与决策逻辑。*

<!-- risk-assessed -->
