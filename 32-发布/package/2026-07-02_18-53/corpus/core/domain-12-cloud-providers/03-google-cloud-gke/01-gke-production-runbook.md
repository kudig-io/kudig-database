---
title: GKE 生产环境运行手册
description: 面向 Google Kubernetes Engine（GKE）的生产运维运行手册，覆盖 Autopilot 与 Standard 模式选择、Workload Identity、VPC-native 网络、节点池、升级、备份/容灾、Cloud Monitoring、成本治理与故障排查。
summary: 面向 GKE 的生产运维运行手册，覆盖集群模式选择、身份与网络、升级、备份/容灾、监控、成本与故障排查。
category: cloud-providers
tags:
- production
- best-practices
- playbook
- cloud-providers
- gke
- google-cloud
- workload-identity
- vpc-native
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- GKE 生产环境如何运维
- GKE Autopilot 与 Standard 怎么选
- GKE Workload Identity 配置与排障
- GKE 升级与备份最佳实践
trigger_keywords:
- GKE
- Google Kubernetes Engine
- Autopilot
- Standard
- Workload Identity
- VPC-native
- Cloud Monitoring
prerequisites:
- kubectl-basics
- gcloud-cli
- gke-networking-basics
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


# GKE 生产环境运行手册

本手册面向已在 Google Cloud 上运行 GKE 的 SRE 与平台工程师，提供从集群选型、日常运维到故障响应的完整操作路径。GKE 托管了控制面，但节点池、网络、身份、升级与可观测性仍需要按生产标准进行设计与持续运营。手册中的命令可直接在配置了 `gcloud` 与 `kubectl` 的环境中执行，所有变更操作均建议在非生产环境验证后再应用到生产集群。通过遵循本手册，团队可以将 GKE 特定操作纳入组织统一的 [[32-发布/package/2026-07-02_18-53/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|生产就绪运维框架]]，降低人为失误并提升故障恢复速度。

## 1. 适用场景与范围

本手册适用于以下场景：

- 新建或接管 GKE Standard / Autopilot 生产集群，需要制定运维基线与变更流程。
- 执行节点池变更、控制面/节点升级、网络调整、安全加固等操作。
- 建立备份/容灾、监控告警、成本优化与安全加固流程。
- 排查 GKE 相关的 Pod 调度、网络、身份、节点异常与升级失败。
- 需要与云厂商无关的 Kubernetes 运维实践进行对照，理解 GKE 托管特性带来的差异与限制。

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装并初始化 gcloud（根据操作系统选择安装方式）
sudo apt-get install google-cloud-cli  # Debian/Ubuntu
# 或 macOS: brew install --cask google-cloud-sdk

gcloud auth login
gcloud config set project <PROJECT_ID>
gcloud components install gke-gcloud-auth-plugin

# 获取集群凭证并验证访问
gcloud container clusters get-credentials <CLUSTER_NAME> --region=<REGION>
kubectl get nodes -o wide

# 建议为每个环境单独配置 gcloud configuration，避免跨项目误操作
gcloud config configurations create prod
gcloud config set project prod-project-id
gcloud config set compute/region asia-east1
```
生产环境建议启用以下 API：`container.googleapis.com`、`compute.googleapis.com`、`iam.googleapis.com`、`monitoring.googleapis.com`、`logging.googleapis.com`、`gkebackup.googleapis.com`、`cloudkms.googleapis.com`。在共享项目中，应为 SRE 团队配置最小权限 IAM 角色，例如 `roles/container.admin`、`roles/compute.networkAdmin`、`roles/monitoring.editor`，避免使用 Owner 角色。所有生产变更都应通过 Infrastructure as Code（Terraform 或 Config Connector）管理，禁止在控制台进行未记录的手动修改。变更窗口、回滚方案与影响范围应在变更管理工具中登记，确保事后可审计。

## 3. 核心概念与架构

### 3.1 Autopilot vs Standard

GKE 提供两种主要集群模式，选择错误会导致后续运维成本显著增加。Autopilot 由 Google 管理节点基础设施，用户只需声明工作负载资源请求，按 Pod 实际请求计费；Standard 则需要用户自行管理节点池、机器类型、节点数量与升级节奏。

| 维度 | Autopilot | Standard |
|---|---|---|
| 节点管理 | Google 托管，按 Pod 计费 | 用户管理节点池与机器类型 |
| 配置灵活性 | 受限（仅指定 compute class） | 高（机器类型、磁盘、污点、标签） |
| 安全基线 | 强制启用 Workload Identity、VPC-native、GKE Sandbox 可选 | 需手动启用 |
| 适用场景 | 无状态微服务、快速上线、希望降低运维负担 | 有状态、GPU、自定义 OS、复杂调度 |
| 成本模型 | 按 Pod 资源请求计费 | 按节点实例计费 |

生产建议：优先使用 Autopilot 降低运维负担；当需要自定义节点、GPU、本地 SSD、特殊 taint/toleration 或必须控制节点级参数时选择 Standard。对于混合负载，可以在 Standard 集群中按工作负载类型划分多个节点池，例如 `system`、`general`、`spot`、`gpu`，并为每个节点池设置独立的资源上限与维护策略。无论选择哪种模式，都应通过发布通道（Release Channel）管理版本节奏：Regular 通道适合大多数生产环境，Stable 通道适合对稳定性要求极高且能接受版本延迟的场景，Rapid 通道仅建议用于开发测试环境。

### 3.2 VPC-native 与 Workload Identity

- **VPC-native（别名 IP）**：Pod 与 Service 使用 Google Cloud 子网中的 Secondary IP ranges，支持原生 Cloud Armor、Cloud NAT、Private Google Access 与 Network Policy。生产环境应始终启用 VPC-native，因为它提供了更好的网络性能、更精确的防火墙控制以及与 Google Cloud 服务的原生集成。
- **Workload Identity**：允许 GKE ServiceAccount 映射到 Google Cloud IAM ServiceAccount，避免在 Pod 内挂载 GCP Service Account JSON 密钥。这是生产环境访问 GCP 资源的标准方式，能够显著降低凭据泄露风险并简化密钥轮换。
- **Private Cluster**：控制面使用私有 endpoint，节点无公共 IP，通过 Cloud NAT 访问外部网络，显著提升安全基线。对于生产环境，Private Cluster 是推荐配置，但需要确保管理网络能够访问控制面 endpoint。

### 3.3 IP 地址规划

在生产环境中，IP 地址规划不足会导致 Pod 或 Service 无法分配地址，进而引发调度失败。建议在创建 VPC 与集群前完成以下规划：

- **节点子网**：根据节点数量与预留扩容空间选择合适掩码，例如 `/24` 支持约 250 个节点。
- **Pod secondary range**：根据最大 Pod 数量规划。每个节点默认可运行 110 个 Pod，若集群最大 100 个节点，则至少需要 `100 * 110 = 11000` 个 IP，建议使用 `/18` 或更大范围。
- **Service secondary range**：根据 Service 数量规划，通常 `/22` 或 `/20` 足够。
- **Master IPv4 CIDR**：用于控制面 endpoint，使用 `/28` 即可，需确保与现有网络不重叠。

### 3.4 节点池与计算设计

- 按工作负载类型拆分节点池：`system` 承载 kube-system、monitoring 等核心组件；`general` 承载通用业务；`workload` 承载特定业务；`gpu` 承载 AI/ML 工作负载（如需要）。
- 关键工作负载使用 `n2-standard` 系列保证稳定性能，可中断负载使用 `e2` 或 Spot 节点。
- 启用节点自动修复（node auto-repair）与自动升级（node auto-upgrade），但业务关键池应在变更窗口内手动控制升级节奏。
- 使用 Node Auto-provisioning 动态创建新节点池时，需定义资源限制（CPU、内存、GPU、磁盘）防止成本失控。
- 为所有节点打上 `team`、`env`、`cost-center` 等标签，便于成本分摊与资源归属追踪。

### 3.5 安全加固基线

生产 GKE 集群应至少满足以下安全基线：

- 启用 Shielded Nodes，包括 Secure Boot 与 Integrity Monitoring，防止启动级攻击与未授权镜像修改。
- 启用 Workload Identity，禁止在 Pod 中挂载 GCP Service Account JSON 密钥。
- 使用 Private Cluster 与 Private Endpoint，限制控制面暴露面。
- 启用 Binary Authorization（可选），仅允许签名镜像部署。
- 启用 VPC Service Controls，保护敏感 GCP 服务访问边界。
- 配置 least-privilege IAM，避免使用项目 Owner/Editor 角色。
- 使用 Cloud KMS 或 HashiCorp Vault 管理 Secret 与加密密钥。
- 启用 Audit Logs（Admin Activity、Data Access、System Event），并导入 SIEM 进行长期留存与分析。

这些基线并非一次性配置，而应在每次集群变更、节点池新增或工作负载上线时重复检查。建议将基线检查纳入 CI/CD 门禁与定期巡检。

## 4. 标准操作流程

### 4.1 创建生产集群（Standard 示例）

以下命令创建一个位于 asia-east1 的三可用区 Standard 集群，启用 VPC-native、Private Cluster、Workload Identity、Dataplane V2 与 Shielded Nodes。该配置适用于对安全性与网络可控性要求较高的生产场景。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
gcloud container clusters create prod-gke \
  --region=asia-east1 \
  --release-channel=regular \
  --network=prod-vpc \
  --subnetwork=prod-subnet \
  --enable-ip-alias \
  --cluster-secondary-range-name=pods \
  --services-secondary-range-name=services \
  --enable-private-nodes \
  --enable-private-endpoint \
  --master-ipv4-cidr=172.16.0.0/28 \
  --enable-workload-identity \
  --workload-pool=<PROJECT_ID>.svc.id.goog \
  --enable-dataplane-v2 \
  --enable-shielded-nodes \
  --shielded-secure-boot \
  --shielded-integrity-monitoring \
  --node-locations=asia-east1-a,asia-east1-b \
  --machine-type=n2-standard-4 \
  --num-nodes=2 \
  --node-labels=nodepool=system \
  --tags=prod-gke-node
```
创建后应立即配置授权网络（Authorized Networks）或 Private Endpoint 访问白名单，限制能够访问控制面的源地址。如果启用了 Private Endpoint，需要通过 Cloud IAP、Bastion Host 或同 VPC 的 CI/CD Agent 访问集群。同时，应为集群启用维护窗口（maintenance window），避免 Google 自动维护动作影响业务高峰。

### 4.2 配置 Workload Identity

Workload Identity 是 GKE 上工作负载访问 GCP 资源的安全方式，配置包含 GCP Service Account 创建、IAM 角色绑定、K8s ServiceAccount 注解与 IAM Workload Identity User 绑定四个步骤。任何一个步骤配置错误都会导致 Pod 无法获取 GCP 凭据。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 创建 GCP Service Account
gcloud iam service-accounts create gke-app-sa \
  --display-name="GKE App SA"

# 2. 为 GCP SA 绑定所需 IAM 角色（示例：Cloud Storage 只读）
gcloud projects add-iam-policy-binding <PROJECT_ID> \
  --member="serviceAccount:gke-app-sa@<PROJECT_ID>.iam.gserviceaccount.com" \
  --role="roles/storage.objectViewer"

# 3. 创建 K8s ServiceAccount 并添加注解
kubectl create serviceaccount app-sa -n prod
kubectl annotate serviceaccount app-sa -n prod \
  iam.gke.io/gcp-service-account=gke-app-sa@<PROJECT_ID>.iam.gserviceaccount.com

# 4. 允许 K8s SA 模拟 GCP SA
gcloud iam service-accounts add-iam-policy-binding \
  gke-app-sa@<PROJECT_ID>.iam.gserviceaccount.com \
  --role="roles/iam.workloadIdentityUser" \
  --member="serviceAccount:<PROJECT_ID>.svc.id.goog[prod/app-sa]"
```
验证：在 Pod 中执行 `curl -H "Metadata-Flavor: Google" http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token`，应返回有效访问令牌。如果返回 403，请依次检查：Pod 是否使用了正确的 ServiceAccount、ServiceAccount 注解是否正确、IAM Workload Identity User 绑定中的 member 是否与 K8s SA 完全匹配、GCP SA 是否拥有所需权限。

### 4.3 节点池管理

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
# 添加业务节点池
gcloud container node-pools create app-pool \
  --cluster=prod-gke \
  --region=asia-east1 \
  --machine-type=n2-standard-8 \
  --num-nodes=3 \
  --node-labels=workload=app \
  --node-taints=workload=app:NoSchedule \
  --enable-autoscaling \
  --min-nodes=3 \
  --max-nodes=20 \
  --disk-type=pd-ssd \
  --disk-size=200GB

# 使用 taints 与 tolerations 将工作负载绑定到指定节点池
kubectl taint nodes <node> workload=app:NoSchedule
```
节点池变更前，应确认目标节点池有足够容量接收被驱逐的 Pod，并为关键服务配置 PodDisruptionBudget。删除节点池是破坏性操作，必须先 drain 节点并验证业务无影响。对于系统节点池，建议固定机器类型与数量，避免因自动缩放导致核心组件资源不足。

### 4.4 控制面与节点升级

GKE 控制面升级不可逆，必须先在 staging 验证应用兼容性。节点升级会导致节点重建，因此需要确保工作负载能够平滑迁移。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看可用版本
gcloud container get-server-config --region=asia-east1

# 升级控制面
gcloud container clusters upgrade prod-gke --region=asia-east1 --master

# 升级指定节点池
gcloud container clusters upgrade prod-gke \
  --region=asia-east1 \
  --node-pool=general-pool \
  --cluster-version=<TARGET_VERSION>

# 监控节点状态
kubectl get nodes -o wide --watch
```
升级前检查清单：
- 所有关键工作负载配置 PodDisruptionBudget。
- 无单点 Pod 或无法迁移的本地存储 Pod。
- 业务低峰期执行，变更窗口已通知相关团队。
- 已准备回滚方案（节点池可重建，控制面不可回滚）。
- 已检查目标版本中的弃用 API 与已知问题。

### 4.5 备份与容灾

GKE 配置备份推荐使用 Backup for GKE，同时保留 Velero 作为跨云/灵活备份方案。Backup for GKE 支持备份整个集群状态、PVC 数据与 Secret，而 Velero 更适合命名空间级细粒度备份与跨云迁移。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 Backup for GKE API
gcloud services enable gkebackup.googleapis.com

# 创建备份计划
gcloud container backup-restore backup-plans create prod-backup-plan \
  --cluster=prod-gke \
  --location=asia-east1 \
  --all-namespaces \
  --include-secrets \
  --include-volume-data \
  --cron-schedule="0 2 * * *" \
  --retention-days=30

# 使用 Velero 备份关键命名空间
velero backup create prod-daily --include-namespaces prod,monitoring

# 跨区域容灾：在 asia-northeast1 建立灾难恢复集群并定期演练恢复
```
每季度至少执行一次恢复演练，验证备份数据的完整性与恢复时间目标（RTO）。演练应在隔离环境中进行，避免影响生产集群。对于关键有状态应用，还需单独制定应用级备份策略，例如数据库的物理备份与逻辑导出。

### 4.6 Cloud Monitoring 与日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 GKE 组件监控
gcloud container clusters update prod-gke \
  --region=asia-east1 \
  --monitoring=SYSTEM,API_SERVER,SCHEDULER,CONTROLLER_MANAGER

# 启用 Cloud Logging
gcloud container clusters update prod-gke \
  --region=asia-east1 \
  --logging=SYSTEM,WORKLOAD
```
关键告警规则（Prometheus/GMP 示例）：

```yaml
apiVersion: monitoring.googleapis.com/v1
kind: Rules
metadata:
  name: gke-prod-alerts
  namespace: monitoring
spec:
  groups:
  - name: gke
    rules:
    - alert: GkeNodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "GKE 节点 NotReady 超过 5 分钟"
    - alert: GkeCertificateExpiringSoon
      expr: |
        (
          apiserver_client_certificate_expiration_seconds_count{job="kubernetes-apiserver"} > 0
          and
          apiserver_client_certificate_expiration_seconds_sum{job="kubernetes-apiserver"}
          / apiserver_client_certificate_expiration_seconds_count{job="kubernetes-apiserver"}
          < 30 * 86400
        )
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "GKE 证书将在 30 天内过期"
```

### 4.7 网络策略示例

启用 Dataplane V2 后，可以使用原生 NetworkPolicy 实现命名空间隔离。以下示例为 `prod` 命名空间配置默认拒绝入站流量，但允许同命名空间内 Pod 互访。

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: prod
spec:
  podSelector: {}
  policyTypes:
  - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: prod
spec:
  podSelector: {}
  ingress:
  - from:
    - podSelector: {}
  policyTypes:
  - Ingress
```

### 4.8 GKE Sandbox 与不可信工作负载

对于运行不可信代码或多租户场景，建议启用 GKE Sandbox（gVisor）：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
gcloud container node-pools create sandbox-pool \
  --cluster=prod-gke \
  --region=asia-east1 \
  --machine-type=n2-standard-4 \
  --workload-metadata=GKE_METADATA \
  --sandbox type=gvisor
```
使用 Sandbox 会增加一定性能开销，但能显著提升隔离性，适合 CI/CD 作业、用户提交代码执行等场景。

### 4.9 成本治理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 Cluster Autoscaler 并选择优化利用率模式
gcloud container clusters update prod-gke \
  --region=asia-east1 \
  --autoscaling-profile=optimize-utilization

# 使用 Spot 节点池运行可中断工作负载
gcloud container node-pools create spot-pool \
  --cluster=prod-gke \
  --region=asia-east1 \
  --spot \
  --machine-type=e2-standard-4 \
  --num-nodes=0 \
  --enable-autoscaling \
  --min-nodes=0 \
  --max-nodes=10
```
成本优化建议：
- 为所有节点与工作负载打上 team/env/cost-center 标签。
- 设置预算告警，监控月度账单异常突增。
- 对开发/测试环境使用 Spot 与 e2 实例。
- 定期审查闲置 PVC、LoadBalancer 与未使用的 IP 地址。
- 使用 GKE 成本分配标签（cost allocation）将资源消耗关联到团队或项目。
- 分析 Cloud Billing 数据，识别长期低利用率节点池并调整机器类型。
- 对长期稳定负载考虑购买 Committed Use Discounts（CUD），降低计算成本。
- 设置 HPA 目标利用率在 60%-70%，避免过度配置同时保留突发余量。

### 4.10 证书与密钥生命周期

GKE 集群涉及多种证书：API server 证书、etcd 证书、kubelet 证书、 ingress TLS 证书与镜像签名证书。生产环境必须建立证书到期监控与自动轮换机制。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 GKE 托管证书状态
kubectl get certificates -A
kubectl describe certificate prod-tls -n prod

# 使用 cert-manager 自动管理 TLS 证书
kubectl apply -f - <<EOF
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: prod-tls
  namespace: prod
spec:
  secretName: prod-tls-secret
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - api.example.com
EOF

# 检查证书过期时间
kubectl get secrets prod-tls-secret -n prod -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
```
建议为所有证书设置 30 天内过期告警，对内部 CA 签发的证书建立年度轮换计划，并将轮换操作纳入变更管理流程。

## 5. 关键检查点与验证命令

### 5.1 日常巡检检查点

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| 集群状态 | `gcloud container clusters describe prod-gke --region=asia-east1` | status=RUNNING，版本在支持窗口内 |
| 节点健康 | `kubectl get nodes -o wide` | 所有节点 Ready，版本一致 |
| Workload Identity | Pod 内获取 metadata token | 成功返回 GCP 访问令牌 |
| 网络策略 | `kubectl get networkpolicies -A` | 核心命名空间存在默认拒绝策略 |
| 备份状态 | `gcloud container backup-restore backups list --location=asia-east1` | 最近 24h 有成功备份 |
| 证书过期 | `kubectl get certificates -A` | 所有证书有效期 > 30 天 |
| 成本标签 | `kubectl get nodes --show-labels` | 节点带有 team/env/cost-center 标签 |
| 组件监控 | `gcloud container clusters describe prod-gke --format='value(monitoringConfig)'` | 已启用 SYSTEM/API_SERVER 等 |

### 5.2 生产变更检查清单

在执行任何生产变更前，建议逐项确认以下事项，并将结果记录在变更管理工具中：

- [ ] 变更已在 staging 或预生产环境验证，回滚方案已准备就绪。
- [ ] 变更窗口已与业务方确认，关键服务已配置 PodDisruptionBudget。
- [ ] 当前集群与节点状态健康，无正在进行的升级或修复操作。
- [ ] 备份已完成且可恢复，关键命名空间数据已确认一致性。
- [ ] 监控告警已启用，值班人员已收到变更通知。
- [ ] 涉及 IAM、网络或安全策略的变更已通过安全团队评审。
- [ ] 变更脚本或命令已通过代码审查，禁止在生产环境直接手工编辑。
- [ ] 变更完成后将进行验证，包括功能测试、指标检查与日志审查。

### 5.3 推荐验证命令组合

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 快速健康检查
kubectl get nodes -o wide
gcloud container clusters describe prod-gke --region=asia-east1 --format='table(status,currentMasterVersion,currentNodeVersion)'

# 工作负载状态
kubectl get pods -A -o wide | grep -v Running | grep -v Completed
kubectl top nodes
kubectl top pods -A --sort-by=cpu

# 安全与网络检查
kubectl get networkpolicies -A
kubectl get serviceaccounts -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\n"}{end}' | grep -v default

# 备份与证书
velero backup get | head -5
gcloud container backup-restore backups list --location=asia-east1 --filter='state=SUCCESS' --limit=5
kubectl get certificates -A
```
## 6. 常见故障与 remediation

| 现象 | 根因 | 处理命令/步骤 |
|---|---|---|
| Pod 持续 Pending | 资源不足、Spot 中断、污点不匹配 | `kubectl describe pod <pod> -n <ns>`；检查节点池容量与 taint/toleration |
| Workload Identity 失败 | 注解错误、IAM 绑定缺失 | 确认 `iam.gke.io/gcp-service-account` 注解与 IAM binding 一致 |
| 节点 NotReady | 磁盘压力、PLEG 异常、网络中断 | `kubectl describe node <node>`；SSH 到节点查看 `journalctl -u kubelet` |
| 控制面 API 延迟高 | 证书过期、etcd 延迟、请求激增 | `kubeadm certs check-expiration`；查看 Cloud Monitoring API server 指标 |
| 出口流量失败 | Cloud NAT 配额耗尽、VPC 路由缺失 | 检查 Cloud NAT 状态与 VPC 路由表 |
| 升级失败/卡住 | PDB 阻塞 drain | `kubectl get pdb -A`；临时调整 minAvailable 或驱逐阻塞 Pod |
| 成本突增 | 节点池过度配置、Spot 被回收后扩容 | 分析 Cloud Billing 标签；调整 requests 与 HPA 阈值 |
| 网络策略不生效 | Dataplane V2 与 Calico 差异 | 确认 CNI 实现；使用 Dataplane V2 原生 NetworkPolicy 语义测试 |
| 应用无法访问 GCP API | Private Google Access 未启用 | 检查子网是否启用 Private Google Access |
| HPA 无法获取指标 | Managed Prometheus 未启用或 ServiceMonitor 配置错误 | 检查 monitoring config 与 PodMonitoring/ServiceMonitor 标签选择器 |
| DNS 解析失败 | kube-dns/CoreDNS 负载高或 VPC DNS 策略错误 | 检查 coredns Pod 资源与 DNS policy；考虑 NodeLocal DNSCache |
| 存储挂载失败 | CSI driver 未安装或 PVC 绑定异常 | `kubectl describe pvc`；检查 Compute Engine CSI driver 状态 |
| 镜像拉取失败 | Artifact Registry 权限或网络策略拦截 | 确认节点 SA 或 Workload Identity 具有 artifactregistry.reader 权限 |
| 证书过期导致 API 拒绝 | TLS Secret 未轮换或 cert-manager 异常 | 检查 Certificate 状态与 cert-manager Pod 日志 |

## 7. 风险与注意事项

1. **控制面升级不可逆**：GKE 控制面升级后无法回滚，务必先在 staging 验证。
2. **节点池删除会终止 Pod**：删除节点池前确认工作负载已迁移，或先 cordon/drain。
3. **Workload Identity 映射错误会导致权限泄露**：确保 IAM binding 的 member 精确到 namespace/serviceaccount。
4. **Private Cluster 访问受限**：管理端需与 master endpoint 同 VPC 或配置授权网络。
5. **Dataplane V2 与 Calico 策略兼容**：启用 Dataplane V2 后，NetworkPolicy 由 eBPF 实现，行为与 Calico 略有差异，需测试验证。
6. **备份不等于可恢复**：每季度执行一次恢复演练，验证 Backup for GKE 或 Velero 的 RTO/RPO。
7. **Spot 节点不适合有状态服务**：为关键 StatefulSet 配置反亲和性，避免同时被回收。
8. **IP 地址耗尽会导致无法调度**：定期监控 Pod/Service IP 使用率，提前扩容 secondary ranges。

## 8. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-12-cloud-providers/09-production-readiness-operations-guide|生产运维域生产就绪运维指南]]
- [[domain-12-cloud-providers/Google-GKE/google-cloud-gke-overview.md|GKE 概览]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-12-cloud-providers/03-google-cloud-gke/01-gke-autopilot-serverless|GKE Autopilot]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-12-cloud-providers/03-google-cloud-gke/02-gke-networking-dataplane-v2|GKE 网络与 Dataplane V2]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-12-cloud-providers/03-google-cloud-gke/04-gke-workload-identity-security|GKE Workload Identity 与安全]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-12-cloud-providers/03-google-cloud-gke/05-gke-troubleshooting-playbook|GKE 故障排查手册]]
- [[domain-05-security-compliance/README.md|安全合规域]]
- [[domain-06-observability/README.md|可观测性域]]


<!-- risk-assessed -->
