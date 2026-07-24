---
title: 阿里云专有云 Deployment 滚动更新失败：ACR 镜像拉取超时
description: 业务发布时新 Pod 卡在 ImagePullBackOff，根因是专线到 ACR 专有镜像仓库网络抖动及 imagePullSecret
  配置不当，包含完整诊断、修复、验证与客诉话术。
summary: 业务发布时新 Pod 卡在 ImagePullBackOff，根因是专线到 ACR 专有镜像仓库网络抖动及 imagePullSecret 配置不当，包含完整诊断、修复、验证与客诉话术。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- acr
- image-pull
- imagepullbackoff
- deployment
- rollout
- network-timeout
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-006
priority: P1
severity: high
affected_cluster: ack-prod-vpc01
affected_namespace: trade-core
ticket_type: 发布变更故障
skill_ref: 镜像拉取异常诊断
fta_ref: 'FTA: Pod ImagePullBackOff'
last_updated: 2026-06-26
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 Deployment 滚动更新失败：ACR 镜像拉取超时 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- acr
- image-pull
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[实体/deployment.md]]'
  type: related_to
- target: '[[系统基础/知识字典/operations/rolling-update.md]]'
  type: related_to
- target: '[[技能/工作负载/deployment/deployment-rolling-update.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-009-etcd-disk-full-apiserver-slow.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 006：Deployment 滚动更新失败（ACR 镜像拉取超时）

## 1. 工单描述

**用户原始描述：**

> 下午 14:20 对 trade-core 命名空间的 order-service 做灰度发布，Deployment 从 v2.3.1 升到 v2.4.0。滚动更新到第 3 个 Pod 就卡住了，新 Pod 一直 `ImagePullBackOff`，老 Pod 还在跑，业务流量没切过去。已经持续 40 分钟。ACK 控制台看到事件提示 "Back-off pulling image \"registry-vpc.cn-shanghai.aliyuncs.com/trade/order-service:v2.4.0\""，namespace 是 trade-core。麻烦尽快处理，我们是双 11 备战核心链路，影响后续压测排期。

## 2. 分类与优先级判定

- **任务类型：** Pod 运行异常 / 镜像拉取失败 / 发布变更故障
- **优先级：** P1（生产环境 + 服务降级 + 影响发布窗口）
- **严重程度：** high
- **响应时限：** 15 分钟内给出可执行修复方案
- **安全级别：** 中风险（仅只读诊断先行，修改 Deployment 需用户确认）

## 3. 诊断步骤

### 3.1 快速确认异常 Pod 与事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 trade-core 命名空间内异常 Pod
kubectl get pod -n trade-core -l app=order-service \
  --field-selector=status.phase!=Running

# 查看具体 Pod 事件与状态
kubectl describe pod -n trade-core <stuck-pod-name>
```
### 3.2 检查 Deployment 滚动更新状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deploy order-service -n trade-core
kubectl rollout status deploy/order-service -n trade-core --timeout=30s
kubectl rollout history deploy/order-service -n trade-core
```
### 3.3 验证镜像仓库与 ACR 专线连通性

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在节点上测试 ACR VPC 域名解析与端口连通
kubectl run net-debug --rm -it --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/busybox:latest --restart=Never -- /bin/sh
# 容器内执行
nc -vz registry-vpc.cn-shanghai.aliyuncs.com 443
nslookup registry-vpc.cn-shanghai.aliyuncs.com
```
### 3.4 检查 imagePullSecret 与 ServiceAccount 权限

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get sa default -n trade-core -o yaml
kubectl get secret -n trade-core | grep acr
kubectl get secret <acr-secret-name> -n trade-core -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d
```
### 3.5 阿里云 ASO/ACK 控制台侧检查

```bash
# 登录到 ASO 运维门户查看当前告警：专有云平台 > 容器服务 ACK > 告警中心
# 检查 ACR 企业版实例健康状态：
aliyun cr GET /repos/{RepoNamespace}/{RepoName}/tags --RegionId cn-shanghai
```

### 3.6 诊断过程补充说明

在真实排障过程中，需要特别注意区分 "镜像不存在"、"认证失败" 与 "网络超时" 三类错误。镜像不存在通常返回 `ErrImagePull`，事件里会明确提示 manifest unknown 或 repository not found；认证失败则会在容器运行时日志里看到 unauthorized 或 denied；而网络超时表现为镜像拉取阶段长时间挂起，随后进入 `ImagePullBackOff`。通过 `kubectl describe pod` 中 `Failed to pull image` 后的具体错误码，可以快速把问题归类。

另外，ACK 专有云的节点通常部署在用户 VPC 内部，访问 ACR 企业版时推荐走 `registry-vpc.cn-*.aliyuncs.com` 域名。如果节点安全组、自定义路由或专线策略发生变更，即使 ACR 实例本身正常，也可能出现握手超时。因此诊断阶段务必在异常 Pod 所在节点上执行网络探测，而不是只在本地跳板机上测试。

## 4. 根因分析

综合事件日志与网络探测，判定根因为 **"ACR 专有镜像仓库拉取超时叠加 imagePullSecret 配置遗漏"**，置信度 **高**。

1. **网络层：** 专有云到 ACR 的专线在 14:10–14:45 出现偶发丢包，TCP 443 三次握手耗时超过 30s，导致 container runtime 判定拉取超时。
2. **配置层：** order-service v2.4.0 的 Deployment 在迁移到新 ACR 企业版实例后，未挂载 `acr-credential-trade` Secret，默认 `default` ServiceAccount 无拉取权限，错误被网络超时掩盖。
3. **Kubernetes 行为：** 镜像拉取失败触发 `ImagePullBackOff`，kubelet 按指数退避重试，Deployment `maxUnavailable` 和 `maxSurge` 设置保守，导致滚动更新停滞。

### 4.1 风险与影响评估

- **业务影响：** 新版本无法完成滚动更新，老版本 Pod 继续承担流量，存在功能未上线与潜在稳定性风险。
- **扩散风险：** 若同一命名空间其他 Deployment 也使用相同 ACR 实例且未配置 imagePullSecret，可能在后续发布中陆续触发同类问题。
- **数据风险：** 本次修复仅涉及 Secret 与 Deployment 策略调整，不触及业务数据与持久化存储。

## 5. 修复命令

### 5.1 创建/更新 imagePullSecret

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若 Secret 不存在，使用 ACR 企业版固定密码或临时凭证创建
kubectl create secret docker-registry acr-credential-trade \
  --docker-server=registry-vpc.cn-shanghai.aliyuncs.com \
  --docker-username=<acr-username> \
  --docker-password=<acr-password> \
  -n trade-core --dry-run=client -o yaml | kubectl apply -f -
```
### 5.2 修改 Deployment 挂载 imagePullSecret

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deploy order-service -n trade-core --type='json' -p='[
  {"op": "add", "path": "/spec/template/spec/imagePullSecrets", "value": [{"name": "acr-credential-trade"}]}
]'
```
### 5.3 调整滚动更新策略以加速恢复

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deploy order-service -n trade-core --type='merge' -p '
spec:
  strategy:
    rollingUpdate:
      maxSurge: 50%
      maxUnavailable: 0
    type: RollingUpdate
'
```
### 5.4 若 ACR 专线仍不稳定，切换至内网 VIP 或 OSS 中转

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 临时将镜像地址切到同 Region OSS 中转域名（需预先配置镜像同步）
kubectl set image deploy/order-service order-service=registry-vpc-internal.cn-shanghai.aliyuncs.com/trade/order-service:v2.4.0 -n trade-core
```
### 5.5 重启拉取以清除退避

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl delete pod -n trade-core -l app=order-service,version=v2.4.0 --field-selector=status.phase=Pending
```
### 5.6 回滚方案（如修复失败）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 若镜像问题无法快速解决，先回滚到上一版本恢复业务
kubectl rollout undo deploy/order-service -n trade-core
kubectl rollout status deploy/order-service -n trade-core --timeout=300s

# 回滚后再次确认老版本 Pod 镜像可正常拉取
kubectl get pod -n trade-core -l app=order-service -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[0].image}{"\n"}{end}'
```
## 6. 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 Deployment 滚动更新完成
kubectl rollout status deploy/order-service -n trade-core --timeout=300s

# 确认所有新 Pod 运行正常且镜像版本正确
kubectl get pod -n trade-core -l app=order-service -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[0].image}{"\t"}{.status.phase}{"\n"}{end}'

# 确认无 ImagePullBackOff 事件
kubectl get events -n trade-core --field-selector reason=ImagePullBackOff --sort-by='.lastTimestamp'

# 业务验证：调用健康检查接口
curl -s http://order-service.trade-core.svc.cluster.local:8080/actuator/health | jq .
```
## 7. 回复客户话术

> 您好，工单 TC-2026-006 已处理完成。
>
> **现象确认：** order-service v2.4.0 在 trade-core 命名空间滚动更新时，新 Pod 因镜像拉取失败进入 `ImagePullBackOff`。
>
> **根因：** 14:10–14:45 专有云到 ACR 的专线存在偶发丢包，且 v2.4.0 Deployment 未挂载 ACR 凭据 Secret，双重因素导致拉取超时。
>
> **已执行修复：**
> 1. 创建 `acr-credential-trade` imagePullSecret；
> 2. 为 order-service Deployment 挂载该 Secret；
> 3. 临时放宽滚动策略 `maxSurge=50% / maxUnavailable=0`；
> 4. 删除 Pending 状态 Pod 清除退避。
>
> **当前状态：** Deployment 已完成滚动更新，所有 Pod 均为 v2.4.0 且 Running。
>
> **后续建议：**
> - 与网络团队确认专线 SLA 及丢包根因；
> - 建议 ACR 实例开启就近访问与镜像缓存；
> - 发布前在预发环境验证 imagePullSecret 配置，避免生产重复踩坑；
- 建议在镜像仓库切换或升级时，维护一份命名空间级别的 imagePullSecret 清单，并在发布 checklist 中强制检查；
- 对于双 11 等高压场景，建议提前对 ACR 专线进行带宽评估，并配置镜像预热与本地缓存节点。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（本工单已闭环）
- **是否需要变更审批：** 是（Deployment 修改与 Secret 创建已走工单级变更授权）
- **交接信息：**
  - 若 24 小时内 trade-core 其他 Deployment 出现同类 ImagePullBackOff，自动升级为 P0；
  - 已通知网络值班同学跟进专线抖动；
  - 建议发布负责人 review 所有使用新 ACR 实例的 Deployment，统一补齐 imagePullSecret；
  - 建议在 CI/CD 流水线中增加发布前检查：若镜像域名含 `registry-vpc.cn-` 且未挂载对应 Secret，自动阻断发布；
  - 对高频发布的核心命名空间，建议配置命名空间默认 ServiceAccount 自动挂载 ACR 凭据，减少人工遗漏；
  - 相关命令与根因已记录至本页，可复用于同集群发布故障。

---

*更新时间：2026-06-26 | 责任域：生产运维/ticket-cases*

## Related

- Deployment
- 滚动更新
- [[技能/工作负载/deployment/deployment-rolling-update.md|Deployment 滚动更新策略]]
- 阿里云专有云 etcd 数据目录磁盘满导致 apiserver 响应慢
- 滚动更新
- [[技能/工作负载/deployment/deployment-rolling-update.md|Deployment 滚动更新策略]]
- 阿里云专有云 etcd 数据目录磁盘满导致 apiserver 响应慢


<!-- risk-assessed -->
