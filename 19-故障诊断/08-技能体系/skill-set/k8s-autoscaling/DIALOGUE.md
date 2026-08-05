---
title: K8s Autoscaling Failure 远程顾问对话脚本
summary: 自动伸缩问题的远程顾问对话脚本，覆盖HPA、VPA、Cluster Autoscaler排查。
category: dialogue
tags:
- dialogue
- remote-advisor
- autoscaling
- hpa
- vpa
- cluster-autoscaler
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
relationships:
- target: '[[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[23-实体/08-交付与制品/helm.md]]'
  type: uses
- target: '[[23-实体/02-K8s核心组件/kubelet.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# K8s Autoscaling Failure 远程顾问对话脚本

> **角色设定**：你是部署在客户环境之外的远程顾问，无法直接连接集群。你只能通过对话指导现场工程师执行操作。
> **对话目标**：在 30 分钟内定位 HPA/VPA/ClusterAutoscaler 不工作的根因并给出修复方案。

---

## 对话入口

### 场景 A：工程师直接描述问题

工程师："HPA 不扩容了，CPU 飙高但是 Pod 数量没变"

顾问："收到。这是典型的自动扩容不工作问题。请确认三个基础信息：
1. 这个 HPA 所在的 **命名空间** 和 **HPA 名称** 是什么？
2. 这是 **HPA 不工作**，还是 **VPA/ClusterAutoscaler** 也不正常？
3. 最近 **1 小时内** 是否有过部署更新、配置变更或扩缩容操作？"

### 场景 B：工程师提供部分监控信息

工程师："Prometheus 告警 HPAReplicasMismatch"

顾问："告警已确认。在深入诊断之前，请确认三点：
1. 这个告警涉及 **几个 HPA/应用**？
2. 控制平面节点（master）和 metrics-server 是否正常？
3. 当前业务负载是否确实已经触发了扩容阈值？"

### 场景 C：工程师从控制台发现异常

工程师："控制台看到 HPA TARGET 显示 unknown"

顾问："HPA 指标获取异常已确认。请切换到命令行执行以下操作，或者把控制台看到的 HPA 状态截图发给我。如果控制台能显示更多细节（如 Events、Metrics 列），也请一并告知。"

### 场景 D：工程师描述 ClusterAutoscaler 问题

工程师："集群节点不够了，但是 ClusterAutoscaler 没有扩容节点"

顾问："Cluster Autoscaler 不工作，这可能导致业务无法获得足够资源。请确认：
1. 这是 **云厂商托管集群** 还是 **自建集群**？
2. 当前 Pod 是否处于 Pending 状态？有多少个？
3. 最近是否修改过节点组、伸缩组或 CA 的配置？"

---

## Round 1：快速状态确认

**顾问**：请执行以下命令，获取自动扩缩容组件的基本状态：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 HPA 状态
kubectl get hpa --all-namespaces

# 2. 查看 VPA 状态（如安装了 VPA）
kubectl get vpa --all-namespaces

# 3. 查看 Cluster Autoscaler Pod 状态
kubectl get pods -n kube-system | grep -E 'autoscaler|metrics-server'
```
> **如果无法执行 kubectl**：请通过集群管理控制台（如 Rancher、ACK Console、OpenShift Console）查看 HPA/VPA 列表和 autoscaler Pod 状态，或者告诉我你当前能访问的界面和看到的信息。
> 
> **如果无法查看所有命名空间**：请指定你怀疑出问题的命名空间执行：`kubectl get hpa -n <namespace>`

### 分支 1-A：HPA TARGET 显示 `<unknown>`

工程师：（提供 HPA 输出，TARGET 列为 unknown）

**顾问**：HPA 无法获取指标，这是最常见的根因。请继续执行以下命令确认指标来源：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 HPA 详情和事件
kubectl describe hpa <hpa-name> -n <namespace>

# 查看 metrics-server 状态
kubectl get pods -n kube-system | grep metrics-server
kubectl logs -n kube-system deployment/metrics-server --tail=50
```
> **如果无法执行 logs**：请尝试 `kubectl get deployment metrics-server -n kube-system`，确认 metrics-server 是否正常运行。如果连 deployment 也查不到，说明 metrics-server 可能未安装。
> 
> **如果无法查看 kube-system**：请确认你是否有查看 kube-system 命名空间的权限。如果没有，请告诉我集群是否使用外部监控系统（如 Prometheus + prometheus-adapter）提供指标。

**工程师回复选项**：
- **A1**：metrics-server Pod 不存在或状态异常（CrashLoopBackOff / Pending）
- **A2**：metrics-server 运行正常，但 HPA Events 显示 `unable to get metrics`
- **A3**：使用 prometheus-adapter，但 adapter Pod 异常或无 metrics 返回

### 分支 1-B：HPA TARGET 有数值但不扩缩容

工程师：（提供 HPA 输出，TARGET 有值但 CURRENT < TARGET 时未扩容，或 CURRENT < TARGET 时未缩容）

**顾问**：HPA 能获取指标但行为不符合预期。请执行以下命令确认配置：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 HPA 详细配置
kubectl get hpa <hpa-name> -n <namespace> -o yaml | grep -A 20 'metrics:'

# 查看关联 Deployment 的当前副本数
kubectl get deployment <deployment-name> -n <namespace>

# 查看 HPA Events 中的扩缩容记录
kubectl describe hpa <hpa-name> -n <namespace> | grep -A 5 'Events'
```
> **如果无法执行 `-o yaml`**：请使用 `kubectl edit hpa <hpa-name> -n <namespace>` 查看配置（不要保存），或者通过 Dashboard 查看 HPA 详情页的配置参数。
> 
> **如果无法查看 Events**：请告诉我 HPA 的 minReplicas、maxReplicas、targetCPUUtilizationPercentage（或 target.type/target.averageUtilization）的值是多少。

**工程师回复选项**：
- **B1**：CURRENT 远超 TARGET 但未扩容，Events 中有 `replica count is at max` 或权限拒绝
- **B2**：CURRENT 远低于 TARGET 但未缩容，Events 中有 `scale-down is disabled` 或稳定窗口提示
- **B3**：HPA 配置看起来正常，但 Deployment 的 Pod 状态异常（Pending/NotReady）导致无法扩容

### 分支 1-C：Cluster Autoscaler 未触发节点扩容

工程师：（提供 CA Pod 状态和相关 Pod 信息）

**顾问**：Cluster Autoscaler 不扩容节点。请执行以下命令确认状态：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CA Pod 状态和日志
kubectl get pods -n kube-system | grep cluster-autoscaler
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=100

# 查看 Pending Pod 列表
kubectl get pods --all-namespaces --field-selector status.phase=Pending

# 查看节点资源
kubectl top nodes
kubectl describe node <node-name> | grep -A 5 'Allocated resources'
```
> **如果无法执行 CA logs**：请确认 CA 的部署名称，可能是 `deployment/cluster-autoscaler` 或 `deployment/cluster-autoscaler-aws-cluster-autoscaler` 等。请执行 `kubectl get deployment -n kube-system` 列出所有 deployment。
> 
> **如果无法查看 Pending Pod**：请告诉我当前是否有 Pod 处于 Pending 状态，如果有，Pending 的原因是什么（资源不足、污点不匹配、节点选择器不匹配等）。

**工程师回复选项**：
- **C1**：CA Pod 不存在或状态异常
- **C2**：CA 日志显示 `not enough resources` 但节点组已达最大容量
- **C3**：CA 日志显示权限错误（如 `UnauthorizedOperation`、`AccessDenied`）
- **C4**：没有 Pending Pod，或 Pending Pod 的原因不是资源不足

### 分支 1-D：VPA recommendation 为空或不生效

工程师：（提供 VPA 输出，recommendation 为空）

**顾问**：VPA 推荐未生成。请执行以下命令确认：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VPA 详情
kubectl get vpa <vpa-name> -n <namespace> -o yaml

# 查看 VPA admission-controller Pod
kubectl get pods -n kube-system | grep vpa
kubectl logs -n kube-system deployment/vpa-admission-controller --tail=50
```
> **如果无法查看 VPA 详情**：请确认 VPA 是否已正确安装。VPA 需要三个组件：admission-controller、recommender、updater。请执行 `kubectl get pods -n kube-system | grep vpa` 查看是否都有运行。
> 
> **如果 VPA 未安装**：请告诉我你们的集群是否计划使用 VPA，或者当前是否仅使用了 HPA。

**工程师回复选项**：
- **D1**：VPA admission-controller Pod 不存在或异常
- **D2**：VPA 已创建但 recommendation 字段为空（未收集足够数据）
- **D3**：VPA 有 recommendation 但 Pod 资源未更新

---

## Round 2：根因定位

### 2-A 分支：metrics-server 异常（分支 1-A 的 A1）

**顾问**：metrics-server 是 HPA 获取指标的关键依赖。请执行以下命令诊断：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 metrics-server Pod 详情
kubectl describe pod -n kube-system -l k8s-app=metrics-server

# 查看 metrics-server 部署配置
kubectl get deployment metrics-server -n kube-system -o yaml | grep -A 10 'args:'
```
> **如果无法查看 Pod 详情**：请告诉我 metrics-server Pod 的状态（Running / Pending / CrashLoopBackOff）。如果是 Pending，可能是节点资源不足或调度约束导致。
> 
> **如果无法查看 deployment 配置**：请通过 Dashboard 查看 metrics-server 的启动参数，特别关注 `--[[23-实体/02-K8s核心组件/kubelet.md|kubelet]]-preferred-address-types` 和 `--kubelet-insecure-tls` 等参数。

**工程师回复选项**：
- **A1-1**：metrics-server Pod 为 Pending 状态，Events 显示无法调度
- **A1-2**：metrics-server Pod 为 CrashLoopBackOff，日志显示证书或连接 kubelet 失败
- **A1-3**：metrics-server 运行正常但 `kubectl top nodes` 也报错

### 2-B 分支：HPA 配置错误（分支 1-B 的 B1/B2）

**顾问**：HPA 配置可能有误。请执行以下命令确认配置细节：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取完整 HPA 配置
kubectl get hpa <hpa-name> -n <namespace> -o yaml

# 确认 Deployment 的 resources.requests 是否设置
kubectl get deployment <deployment-name> -n <namespace> -o yaml | grep -A 10 'resources:'

# 确认当前 Pod 的 CPU/内存实际使用
kubectl top pods -n <namespace> -l app=<app-label>
```
> **如果无法执行 `kubectl top pods`**：请检查 `kubectl top nodes` 是否正常。如果 top 命令全部不可用，说明 metrics-server 有问题，回到 2-A 分支诊断。
> 
> **如果无法查看 Deployment 配置**：请确认 Deployment 的 Pod template 中是否设置了 `resources.requests.cpu` 和 `resources.requests.memory`。HPA 基于 CPU 扩缩容时，**必须设置 requests.cpu**，否则 HPA 无法计算利用率。

**工程师回复选项**：
- **B1-1**：Deployment 没有设置 resources.requests.cpu → HPA 无法计算利用率
- **B1-2**：HPA 的 scaleTargetRef 指向了错误的 APIVersion/Kind/Name
- **B1-3**：HPA 使用了自定义指标（如 prometheus-adapter），但指标查询返回空值或错误

### 2-C 分支：Cluster Autoscaler 权限/配置错误（分支 1-C 的 C2/C3）

**顾问**：CA 无法操作云资源。请执行以下命令确认：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CA 的 ServiceAccount 和权限
kubectl get serviceaccount -n kube-system | grep autoscaler
kubectl get clusterrolebinding | grep autoscaler
kubectl describe clusterrole cluster-autoscaler

# 查看 CA 启动参数
kubectl get deployment cluster-autoscaler -n kube-system -o yaml | grep -A 20 'args:'
```
> **如果无法查看 clusterrole**：请确认你是否有查看 RBAC 配置的权限。如果没有，请告诉我你们使用的集群类型（EKS/AKS/GKE/ACK/自建），以及 CA 的安装方式（[[23-实体/08-交付与制品/helm.md|helm]] chart / 云厂商插件 / 手动部署）。
> 
> **如果 CA 部署在命名空间之外**：请确认 CA 的实际部署位置和配置方式。有些集群的 CA 作为云厂商的托管组件运行，不在 kube-system 中。

**工程师回复选项**：
- **C2-1**：CA 启动参数中 `--node-group-auto-discovery` 或 `--nodes` 配置错误
- **C2-2**：CA 的 ServiceAccount 缺少 `autoscaling` 相关权限（AWS）或 RBAC 权限不足
- **C2-3**：节点组（ASG/MIG/VMSS）已达到 maxSize，无法继续扩容
- **C2-4**：CA 的 `--cloud-provider` 参数与实际环境不匹配

---

### 2-ACK-C 分支：阿里云ACK节点池弹性伸缩排查（ACK特有）

**顾问**：阿里云 ACK 的弹性伸缩通过 **节点池（NodePool）** 和 **Cluster Autoscaler（CA）** 配合实现，与自建集群有很大差异。请按以下步骤排查：

**步骤 1：ACK节点池状态检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看ACK节点池列表和状态
kubectl get nodes -o custom-columns=NAME:.metadata.name,POOL:.metadata.labels["alibabacloud.com/nodepool-id"],ROLE:.metadata.labels["node-role.kubernetes.io/worker"]

# 查看节点池伸缩组活动（通过ack-node-problem-detector或控制台）
kubectl get events --field-selector reason=ScaleGroupActivity --sort-by='.lastTimestamp' | tail -20
```
> **如果无法执行 kubectl**：请登录 **ACK 控制台 > 集群 > 节点管理 > 节点池**，告诉我：
> 1. 涉及节点池的 **当前节点数** / **期望节点数** 是否一致？
> 2. 节点池的 **弹性伸缩** 是否已启用？
> 3. 节点池的 **最小节点数** 和 **最大节点数** 配置是多少？当前是否已达上限？
> 4. 节点池的 **伸缩活动** 中是否有失败记录？失败原因是什么？

**步骤 2：ACK Cluster Autoscaler组件状态**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看ACK CA Pod状态（ACK托管版中CA由阿里云托管，但Pro版可见）
kubectl get pods -n kube-system | grep -E 'cluster-autoscaler|autoscaler'

# 查看CA日志中的伸缩决策
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=100 | grep -i "scale up|scale down|node pool|nodepool"
```
> **如果无法执行**：请登录 **ACK 控制台 > 集群 > 组件管理**，确认：
> 1. **cluster-autoscaler** 组件（如可见）是否正常运行？
> 2. 如果是 **ACK托管标准版**，CA 由阿里云托管，请检查 **节点池** 的弹性伸缩配置。
> 3. 如果是 **ACK Pro版**，请确认 CA 组件是否已启用且版本兼容。

**步骤 3：阿里云ESS伸缩组状态检查**
```bash
# 通过aliyun CLI查询伸缩组状态
aliyun ess DescribeScalingGroups --RegionId <region-id> --ScalingGroupIds '["<scaling-group-id>"]'

# 查询伸缩组活动记录
aliyun ess DescribeScalingActivities --RegionId <region-id> --ScalingGroupId <scaling-group-id> --PageSize 10
```
> **如果无法执行 aliyun CLI**：请登录 **阿里云控制台 > 弹性伸缩 ESS**，告诉我：
> 1. 伸缩组状态是否为 **活跃（Active）**？
> 2. 伸缩组中 **ECS实例数量** 是否已达 **最大实例数**？
> 3. **伸缩活动** 中最近是否有 **失败** 记录？失败原因（如库存不足、起账失败、用户配额不足）？
> 4. **伸缩配置** 中的实例规格在目标可用区是否有库存？

**步骤 4：ACK节点自动伸缩特有配置检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池的label和taint配置
kubectl get nodepool -n kube-system 2>/dev/null || echo "ACK NodePool CRD未安装"

# 查看Pod的节点亲和性/反亲和性是否与节点池匹配
kubectl get pod <pending-pod> -n <ns> -o yaml | grep -A 10 "nodeAffinity|nodeSelector"
```
> **如果无法执行**：请登录 **ACK 控制台 > 集群 > 节点管理 > 节点池**，确认：
> 1. 节点池的 **节点标签（Labels）** 是否与 Pending Pod 的 nodeSelector/亲和性匹配？
> 2. 节点池的 **污点（Taints）** 是否被 Pod 的容忍度（Tolerations）覆盖？
> 3. 节点池的 **实例规格** 是否满足 Pod 的 resources.requests？

**阿里云ACK弹性伸缩特有诊断矩阵**：

| ACK特有场景 | 诊断方法 | 修复方案 |
|:---|:---|:---|
| 节点池弹性伸缩未启用 | ACK控制台查看节点池"弹性伸缩"开关 | 启用节点池弹性伸缩，配置最小/最大节点数 |
| 伸缩组已达最大实例数 | ESS控制台查看伸缩组最大实例数 | 调整节点池最大节点数，或释放其他节点池节点 |
| ECS库存不足导致扩容失败 | ESS伸缩活动记录显示`StockError` | 更换伸缩配置中的实例规格，或切换至其他可用区 |
| RAM角色权限不足（ESS操作被拒绝） | CA日志显示`Forbidden.RAM` / ESS事件 | 为集群Worker RAM角色添加`AliyunESSFullAccess`权限 |
| 节点池标签与Pod调度约束不匹配 | `kubectl describe pod`显示`node affinity`不匹配 | 修正节点池标签，或调整Pod的nodeSelector/亲和性 |
| ACK Pro版CA组件异常 | ACK控制台组件管理页面 | 重启或升级CA组件 |
| 专有云中Apsara AutoScaling异常 | 专有云ASO/天基控制台查看自动伸缩服务 | 联系阿里云驻场工程师修复底座弹性伸缩服务 |
| 节点池缩容保护导致无法缩容 | `kubectl get node <node> -o yaml`查看注解 | 移除节点的`cluster-autoscaler.kubernetes.io/scale-down-disabled`注解 |
| 竞价实例被回收导致节点丢失 | 控制台查看实例计费方式和回收事件 | 切换节点池为按量付费实例，或调整应用副本分布策略 |

> **远程顾问无法直连时的阿里云控制台排查**：
> 1. **ACK 控制台 > 集群 > 节点管理 > 节点池**：查看节点池状态、伸缩活动记录、失败原因
> 2. **阿里云控制台 > 弹性伸缩 ESS**：查看伸缩组状态、伸缩活动、伸缩配置
> 3. **阿里云控制台 > 云监控 > 弹性伸缩**：查看伸缩活动趋势和异常告警
> 4. **ACK 控制台 > 集群 > 运维管理 > 自动伸缩诊断**：使用 ACK 内置自动伸缩诊断工具
> 5. **RAM 控制台 > 角色**：检查集群使用的 RAM 角色是否有 ESS 和 ECS 操作权限
> 6. 如果是 **专有云**，请通过 **ASO/天基控制台** 查看 **Apsara AutoScaling** 服务状态

**分支决策**：
- **ACK-A1**：节点池伸缩配置问题（未启用/已达上限）→ 调整节点池最小/最大节点数
- **ACK-A2**：ESS伸缩活动失败（库存/权限/起账）→ 更换实例规格或修复RAM权限
- **ACK-A3**：Pod调度约束与节点池不匹配 → 修正节点池标签或Pod调度配置
- **ACK-A4**：ACK Pro CA组件异常 → 重启或升级CA组件
- **ACK-A5**：专有云平台底座弹性伸缩异常 → 升级至阿里云TAM/驻场工程师

### 2-D 分支：资源配额或节点池限制（分支 1-B 的 B3 / 1-C 的 C4）

**顾问**：HPA/CA 行为受限可能是资源上限导致。请执行以下命令：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看命名空间资源配额
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota -n <namespace>

# 查看 LimitRange
kubectl get limitrange -n <namespace>

# 查看节点可分配资源
kubectl describe node <node-name> | grep -A 10 'Allocated resources'
```
> **如果无法查看 resourcequota**：请确认该命名空间是否设置了 ResourceQuota。HPA 扩容时如果会超出 ResourceQuota 的 limits，扩容请求会被拒绝。
> 
> **如果节点资源充足但 Pod 仍 Pending**：请检查 Pod 的亲和性/反亲和性规则、污点容忍度、节点选择器等调度约束。

**工程师回复选项**：
- **D1**：ResourceQuota 的 pods 或 CPU/memory limits 已达到上限
- **D2**：节点有污点或 Pod 没有匹配的容忍度
- **D3**：Pod 的 nodeSelector 或亲和性规则导致无法调度到新节点

---

## Round 3：修复方案与执行

### 3-A 分支：修复 metrics-server

**顾问**：根因已定位到 metrics-server。请按以下步骤修复：

**步骤 1**：确认 metrics-server 的部署方式
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deployment metrics-server -n kube-system -o yaml > /tmp/metrics-server-backup.yaml
```
> **如果无法备份**：请确保你有修改 kube-system 命名空间资源的权限。如果没有，请告诉我，我需要提供无需修改 kube-system 的替代方案。

**步骤 2**：如果是证书问题（常见于自签证书集群），添加 insecure-tls 参数：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment metrics-server -n kube-system --type='json' -p='[
  {"op": "add", "path": "/spec/template/spec/containers/0/args/-", "value": "--kubelet-insecure-tls"}
]'
```
> **如果无法执行 patch**：请使用 `kubectl edit deployment metrics-server -n kube-system` 手动在 args 中添加 `--kubelet-insecure-tls`，保存后退出。
> 
> **如果 edit 也无法使用**：请准备修改后的 YAML 文件，执行 `kubectl apply -f <fixed-metrics-server.yaml>`。如果你不确定如何修改，请把当前的 metrics-server deployment YAML 发给我，我帮你生成正确的版本。

**步骤 3**：验证修复
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout status deployment/metrics-server -n kube-system
kubectl top nodes
kubectl get hpa -n <namespace>
```
> **如果 rollout status 卡住**：请执行 `kubectl get pods -n kube-system -l k8s-app=metrics-server` 查看新 Pod 状态。如果新 Pod 无法启动，请执行 `kubectl rollout undo deployment/metrics-server -n kube-system` 回滚。

### 3-B 分支：修正 HPA 配置

**顾问**：根因是 HPA 或 Deployment 配置有误。请按以下步骤修复：

**步骤 1**：如果 Deployment 缺少 resources.requests，请添加：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment <deployment-name> -n <namespace> --type='json' -p='[
  {"op": "add", "path": "/spec/template/spec/containers/0/resources", "value": {"requests": {"cpu": "100m", "memory": "128Mi"}}}
]'
```
> **如果无法执行 patch**：请使用 `kubectl edit deployment <deployment-name> -n <namespace>` 在 `containers[].resources` 下添加 `requests: {cpu: "100m", memory: "128Mi"}`。请根据应用实际需求调整 CPU 和内存值。
> 
> **如果 edit 也无法使用**：请告诉我当前的 Deployment YAML 内容，或准备一个新的 YAML 文件执行 `kubectl apply`。

**步骤 2**：如果 HPA 的 scaleTargetRef 错误，请修正：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch hpa <hpa-name> -n <namespace> --type='merge' -p='{
  "spec": {
    "scaleTargetRef": {
      "apiVersion": "apps/v1",
      "kind": "Deployment",
      "name": "<correct-deployment-name>"
    }
  }
}'
```
> **如果无法确定正确的 target**：请执行 `kubectl get deployment -n <namespace>` 列出所有 Deployment，告诉我目标应用的 Deployment 名称。

**步骤 3**：验证 HPA 开始工作
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa -n <namespace> -w
# 等待 1-2 分钟，确认 TARGET 列从 unknown 变为正常百分比
```
> **如果 TARGET 仍然 unknown**：请检查 metrics-server 是否正常工作（回到 3-A 分支），或检查是否有自定义指标适配器异常。

### 3-C 分支：修复 Cluster Autoscaler 配置/权限

**顾问**：根因是 CA 配置或权限问题。请按以下步骤修复：

**步骤 1**：检查并修正 CA 启动参数
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前参数
kubectl get deployment cluster-autoscaler -n kube-system -o yaml | grep 'args:' -A 30
```
> **如果 CA 不在 kube-system**：请告诉我 CA 的实际部署位置和名称。
> 
> **如果无法查看 deployment**：请通过集群控制台或云厂商管理界面查看 CA 的配置参数。

**步骤 2**：根据集群类型修复权限

- **AWS EKS**：确认 CA ServiceAccount 已关联正确的 IAM Role（IRSA）：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get sa cluster-autoscaler -n kube-system -o yaml | grep 'eks.amazonaws.com/role-arn'
```
> **如果无法执行**：请通过 AWS Console 确认 IAM Role 的 Trust Policy 和 Autoscaling 权限是否正确。

- **阿里云 ACK**：确认 CA 已启用并配置正确的伸缩组 ID：
> **如果无法通过 kubectl 确认**：请登录 ACK 控制台，进入"节点池"页面确认弹性伸缩配置。

- **GKE**：CA 通常为托管组件，请确认集群启用了 autoscaling：
> **如果无法确认**：请通过 GCP Console 检查集群的 Node Pool autoscaling 配置。

**步骤 3**：修正后重启 CA

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/cluster-autoscaler -n kube-system
kubectl rollout status deployment/cluster-autoscaler -n kube-system
```
> **如果无法 restart**：请执行 `kubectl delete pod -n kube-system -l app=cluster-autoscaler` 让 Deployment 自动重建 Pod。

### 3-D 分支：调整资源配额或节点限制

**顾问**：根因是资源上限阻止了扩缩容。请按以下步骤处理：

**步骤 1**：如果 ResourceQuota 不足，请调整：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前配额
kubectl get resourcequota <quota-name> -n <namespace> -o yaml

# 增加 pods 或 CPU/memory limits（根据实际需要调整）
kubectl patch resourcequota <quota-name> -n <namespace> --type='merge' -p='{
  "spec": {"hard": {"pods": "100", "requests.cpu": "50", "requests.memory": "100Gi"}}
}'
```
> **如果无法修改 ResourceQuota**：请确认你是否有修改 ResourceQuota 的权限。如果没有，请联系集群管理员调整配额，或临时将部分非关键应用的副本数减少以释放配额。
> 
> **如果不确定应该设置多少**：请告诉我当前命名空间下的 Pod 总数、CPU 和内存请求总量，以及你希望扩容到的目标副本数，我来帮你计算合适的配额。

**步骤 2**：如果节点组已达上限，请扩容节点组：

- **云厂商托管集群**：通过云厂商控制台增加节点组的 MaxSize
- **自建集群**：如果使用了 Cluster Autoscaler，请修改 CA 的 `--nodes` 参数增加最大节点数

> **如果无法修改节点组**：请考虑临时降低非关键应用的副本数以释放资源，或手动添加新节点到集群。

**步骤 3**：验证扩容成功
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> -w
kubectl get nodes -w
# 观察新 Pod 是否被创建并调度到新节点
```
---

## Round 4：验证修复与升级决策

**顾问**：修复已执行。现在请验证修复是否生效。请执行以下检查：

### 检查清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认 HPA 状态正常
kubectl get hpa -n <namespace>
```
> **如果无法执行**：请通过 Dashboard 或其他运维平台确认 HPA 状态，TARGET 列应显示为百分比而非 unknown。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 2. 确认 metrics-server 正常工作
kubectl top nodes
kubectl top pods -n <namespace>
```
> **如果无法执行 top**：请等待 1-2 分钟后再次尝试。metrics-server 收集指标需要一定时间。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 3. 对应用施加负载测试，观察 HPA 是否扩容
kubectl get hpa -n <namespace> -w
# 同时观察 Pod 数量变化
kubectl get pods -n <namespace> -w
```
> **如果无法施加负载**：请告诉我当前业务是否已经有自然流量。如果有，请观察 5-10 分钟看 HPA 是否有扩缩容动作。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 4. 确认 Cluster Autoscaler 正常（如涉及）
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=50 | grep -i 'scale up|scale down|failed'
```
> **如果无法查看 CA 日志**：请观察节点数量是否在负载增加后增长，在负载降低后减少。

---

### 升级决策点

**顾问**：根据修复结果，请告诉我当前状态，我将决定是否升级或结束：

#### 决策 4-A：修复成功，所有检查通过

**顾问**：很好，修复已验证成功。请继续保持观察 **5-10 分钟**，如果 HPA 能正常随负载变化扩缩容，可以认为问题已解决。

> **后续建议**：
> 1. 如果之前缺少 resources.requests，建议对所有生产应用统一补充
> 2. 如果 metrics-server 使用了 `--kubelet-insecure-tls`，建议在测试环境验证正式证书方案
> 3. 建议配置 HPA/VPA 的告警规则，提前发现扩缩容异常

**对话结束** ✅

#### 决策 4-B：修复后 HPA 仍然显示 unknown

**顾问**：修复未生效，HPA 仍无法获取指标。这说明根因可能比我初步判断的更复杂。接下来有以下选择：

> **升级路径选择**：
> 1. **继续深入排查** → 需要检查 prometheus-adapter 或自定义指标适配器配置
> 2. **升级至监控专家** → 如果涉及 Prometheus/Thanos/Grafana 等监控系统，转接至 [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|SKILL]]-MON-001
> 3. **升级至控制平面专家** → 如果怀疑 apiserver 或 kubelet 指标接口异常，转接至 SKILL-CONT-001
> 4. **升级至平台工程专家** → 如果涉及多租户配额策略或复杂的资源调度策略

请告诉我你的选择，或者告诉我当前 HPA describe 的最新 Events 信息。

#### 决策 4-C：HPA 工作正常但 CA 不扩容节点

**顾问**：HPA 能扩容 Pod，但 CA 无法扩容节点，导致 Pod 处于 Pending 状态。这通常暗示：

> 1. **云厂商 API 限制**：CA 调用云厂商 API 被限流或拒绝 → 需要检查云厂商控制台的事件和配额
> 2. **节点镜像/启动脚本问题**：新节点启动失败导致 CA 扩容后节点无法加入 → 需要检查节点启动日志
> 3. **CA 配置与节点组不匹配**：CA 监控的节点组不包含实际需要的节点池

请告诉我 Pending Pod 的 Events 信息，以及云厂商控制台是否有节点创建失败的记录。

#### 决策 4-D：修复后出现新的问题（如缩容过于激进）

**顾问**：修复后出现新的问题，可能是修复操作带来的副作用。请执行：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe hpa <hpa-name> -n <namespace> | grep -i "Events" -A 20
kubectl get events -n <namespace> --field-selector reason=ScalingReplicaSet
```
> **如果无法执行**：请把你能看到的最新 HPA 行为描述给我。

**顾问判断**：
- 如果是 HPA 缩容过于激进导致服务抖动 → 需要调整 `behavior.scaleDown.stabilizationWindowSeconds`
- 如果是扩容后节点资源不足导致 Pod 被驱逐 → 需要调整 CA 的扩容策略或增加节点资源
- 如果是 metrics-server 重启后指标波动导致频繁扩缩容 → 需要调整 HPA 的 `--horizontal-pod-autoscaler-tolerance` 参数

---

## 附录：常用命令速查

> 以下命令供现场工程师快速复制使用，顾问可根据实际情况选择性提供。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 快速查看所有 HPA 状态
kubectl get hpa --all-namespaces

# 查看 HPA 详情和事件
kubectl describe hpa <hpa-name> -n <namespace>

# 查看 HPA 完整配置
kubectl get hpa <hpa-name> -n <namespace> -o yaml

# 查看 metrics-server 状态
kubectl get pods -n kube-system -l k8s-app=metrics-server
kubectl logs -n kube-system deployment/metrics-server --tail=100

# 查看节点和 Pod 资源使用
kubectl top nodes
kubectl top pods --all-namespaces

# 查看 Cluster Autoscaler 日志
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=100

# 查看 Pending Pod
kubectl get pods --all-namespaces --field-selector status.phase=Pending

# 查看命名空间资源配额
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota -n <namespace>

# 查看节点可分配资源
kubectl describe node <node-name> | grep -A 10 "Allocated resources"

# 对 Deployment 施加负载测试（示例）
kubectl run load-generator --rm -i --restart=Never --image=busybox -- /bin/sh -c "while true; do wget -q -O- http://<service-name>; done"

# 查看 VPA 推荐
kubectl get vpa <vpa-name> -n <namespace> -o yaml | grep -A 20 "recommendation"
```
---

## 对话结束语

**顾问**：感谢你的配合。如果问题已解决，请记录本次问题的根因和修复方案，便于后续复盘。特别建议：
1. 将 HPA/CA 的关键配置纳入 GitOps 管理，避免手动修改
2. 配置 HPA 无法获取指标、CA 扩容失败等告警规则
3. 定期验证自动扩缩容策略是否符合业务需求

如果问题仍未解决，请告诉我当前状态，我们继续排查。

> **重要提醒**：本对话脚本覆盖 HPA/VPA/ClusterAutoscaler 的常见问题场景。对于涉及自定义指标（如 Prometheus 外部指标）、多集群联邦 HPA、或云厂商特定的节点自动伸缩问题，请随时要求升级至更专业的 Skill 处理。

## 相关案例

- [[22-概念/14-案例研究/2026-02-18-hpa-thrashing.md|2026-02-18-hpa-thrashing]]
- [[22-概念/14-案例研究/2026-08-18-cluster-autoscaler-scale-down-delay.md|2026-08-18-cluster-autoscaler-scale-down-delay]]
## Related

- [[23-实体/02-K8s核心组件/deployment.md|Deployment]]
- [[17-系统基础/06-知识字典/fundamentals/nodes.md|Nodes（节点）]]


<!-- risk-assessed -->
