---
title: 阿里云专有云 HPA 未生效：metrics-server 异常与 Prometheus adapter 配置错误
description: 业务扩容时 HPA 无法获取 CPU/自定义指标，根因是 metrics-server 证书过期及 Prometheus adapter
  规则指向错误 ServiceMonitor，包含诊断、修复与验证。
summary: 业务扩容时 HPA 无法获取 CPU/自定义指标，根因是 metrics-server 证书过期及 Prometheus adapter 规则指向错误
  ServiceMonitor，包含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- hpa
- metrics-server
- prometheus-adapter
- autoscaling
- custom-metrics
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-007
priority: P1
severity: high
affected_cluster: ack-prod-vpc02
affected_namespace: pay-gateway
ticket_type: 容量弹性故障
skill_ref: HPA 未生效诊断
fta_ref: 'FTA: HPA 未触发扩容'
last_updated: 2026-06-26
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 HPA 未生效：metrics-server 异常与 Prometheus adapter 配置错误 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- hpa
- metrics-server
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
- target: '[[系统基础/知识字典/scheduling/hpa.md]]'
  type: related_to
- target: '[[entities/prometheus.md]]'
  type: related_to
- target: '[[concepts/autoscaling-strategies.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 007：HPA 未生效（metrics-server 异常 + Prometheus adapter 配置错误）

## 1. 工单描述

**用户原始描述：**

> 今天上午 10:00 开始，pay-gateway 服务的 HPA 突然不生效了。业务流量上涨，CPU 都飙到 80% 以上，Pod 数量还是 3 个没变化。`kubectl get hpa` 显示 `TARGETS` 是 `<unknown>/60%`，自定义指标 `gateway_qps` 显示 `<unknown>`。我们昨晚刚升级了 Prometheus operator，怀疑是升级导致的。namespace 是 pay-gateway。请尽快看一下，现在业务已经有点慢了。

## 2. 分类与优先级判定

- **任务类型：** 容量弹性 / HPA 指标获取失败 / 自动扩缩容异常
- **优先级：** P1（生产环境 + 服务降级 + 有扩容风险）
- **严重程度：** high
- **响应时限：** 15 分钟内定位根因并给出修复
- **安全级别：** 中风险（涉及 kube-system 组件，操作需变更授权）

## 3. 诊断步骤

### 3.1 查看 HPA 状态与事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa -n pay-gateway
kubectl describe hpa pay-gateway-hpa -n pay-gateway
kubectl get events -n pay-gateway --field-selector involvedObject.kind=HorizontalPodAutoscaler
```
### 3.2 检查 metrics-server 组件状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deploy metrics-server -n kube-system
kubectl get pod -n kube-system -l k8s-app=metrics-server
kubectl logs -n kube-system -l k8s-app=metrics-server --tail=200

# 验证 metrics-server API 聚合层可用性
kubectl get --raw /apis/metrics.k8s.io/v1beta1
```
### 3.3 检查 Prometheus adapter 与自定义指标 API

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deploy prometheus-adapter -n monitoring
kubectl get pod -n monitoring -l name=prometheus-adapter
kubectl logs -n monitoring -l name=prometheus-adapter --tail=300

# 查看自定义指标 API
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/namespaces/pay-gateway/services/*/gateway_qps
```
### 3.4 检查 Prometheus 目标与 ServiceMonitor

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get servicemonitor -n monitoring | grep gateway
kubectl get servicemonitor pay-gateway-sm -n monitoring -o yaml

# 在 Prometheus UI 上查询：up{job="pay-gateway"}
# 或在 promtool 中执行
kubectl exec -it prometheus-k8s-0 -n monitoring -- \
  wget -qO- 'http://localhost:9090/api/v1/query?query=gateway_qps'
```
### 3.5 检查证书与聚合层 APIService

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get apiservice | grep metrics
kubectl get apiservice v1beta1.metrics.k8s.io -o yaml
kubectl get apiservice v1beta1.custom.metrics.k8s.io -o yaml

# 检查 metrics-server 证书有效期
kubectl get secret metrics-server-certs -n kube-system -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
```
### 3.6 诊断过程补充说明

HPA 出现 `<unknown>` 时，首要任务是区分是 "资源指标（metrics.k8s.io）" 不可用，还是 "自定义指标（custom.metrics.k8s.io）" 不可用。前者通常由 metrics-server 负责，后者由 Prometheus adapter 负责。如果两者同时不可用，往往意味着 apiserver 聚合层或证书链出现了问题，需要优先检查 APIService 的 `Available` 条件。

在 ACK 专有云环境中，metrics-server 可能以托管插件形式存在，也可能由用户自行部署。若证书由集群 CA 自动轮转，理论上不会过期，但在某些老版本或自定义部署中，证书可能未纳入自动轮转。Prometheus operator 升级时，如果同时变更了 ServiceMonitor 标签规范（如从 `app` 迁移到 `app.kubernetes.io/name`），adapter 的 rules 必须同步更新，否则指标名虽然存在，但查询时无法匹配到正确的 Prometheus 时间序列。

## 4. 根因分析

综合 HPA 事件、组件日志与 Prometheus 目标状态，判定根因为 **"metrics-server 证书过期导致资源指标不可用，且 Prometheus adapter 升级后规则匹配了错误的 ServiceMonitor"**，置信度 **高**。

1. **metrics-server：** 昨晚 Prometheus operator 升级时触发节点滚动，metrics-server 的 serving 证书已于 3 天前过期，重启后 TLS 握手失败，APIService `v1beta1.metrics.k8s.io` 状态变为 `False`。
2. **Prometheus adapter：** 升级后 `configmap` 中 `rules` 引用的 ServiceMonitor 标签从 `app: pay-gateway` 变成了 `app.kubernetes.io/name: pay-gateway`，但用户 HPA 仍查询旧指标名 `gateway_qps`，导致自定义指标返回空值。
3. **HPA 行为：** 资源指标与自定义指标均 `<unknown>`，控制器无法计算副本数，扩容被抑制。

### 4.1 风险与影响评估

- **业务影响：** HPA 无法扩容，pay-gateway 在流量高峰下持续高负载，可能导致响应延迟增加、超时率上升，存在雪崩风险。
- **扩散风险：** 同一集群其他依赖 HPA 的服务也可能受 metrics-server 证书问题影响，需全局检查。
- **数据风险：** 本次修复不涉及业务数据，主要影响监控与控制面聚合层。

## 5. 修复命令

### 5.1 重新签发 metrics-server 证书

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 删除旧证书 Secret，让 metrics-server 自动重新生成
kubectl delete secret metrics-server-certs -n kube-system
kubectl rollout restart deploy metrics-server -n kube-system
kubectl rollout status deploy metrics-server -n kube-system --timeout=120s
```
### 5.2 刷新 APIService 状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl delete apiservice v1beta1.metrics.k8s.io
kubectl apply -f https://raw.githubusercontent.com/kubernetes-sigs/metrics-server/release-0.7/components.yaml
# 若使用 ACK 托管组件，改用：
# aliyun cs clusteraddon install metrics-server --cluster-id <ack-cluster-id>
```
### 5.3 修复 Prometheus adapter 规则

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 编辑 adapter ConfigMap，将规则中的标签选择器修正为新标签
kubectl get cm adapter-config -n monitoring -o yaml > /tmp/adapter-config.yaml

# 关键修改示例：将 labels.app: pay-gateway 改为 labels.app_kubernetes_io_name: pay-gateway
# 同时确认 metricsQuery 与 HPA 查询的指标名一致
kubectl apply -f /tmp/adapter-config.yaml
kubectl rollout restart deploy prometheus-adapter -n monitoring
```
### 5.4 验证自定义指标返回

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
sleep 30
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/namespaces/pay-gateway/services/*/gateway_qps
```
### 5.5 触发 HPA 重新评估

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动 annotate 强制 HPA 立即重新计算
kubectl annotate hpa pay-gateway-hpa -n pay-gateway kube-controller-manager.kubernetes.io/restart-
kubectl patch hpa pay-gateway-hpa -n pay-gateway -p '{"metadata":{"annotations":{"manual-trigger":"'$(date +%s)'"}}}'
```
### 5.6 回滚方案（如修复失败）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若 metrics-server 无法快速恢复，可临时手动扩容 pay-gateway 以缓解业务压力
kubectl scale deploy pay-gateway -n pay-gateway --replicas=10

# 若 Prometheus adapter 修复后仍无法获取自定义指标，可临时使用 CPU-only HPA
kubectl patch hpa pay-gateway-hpa -n pay-gateway --type='merge' -p '
spec:
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
'
```
## 6. 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 HPA 能读取到当前指标
kubectl get hpa pay-gateway-hpa -n pay-gateway -w

# 确认 metrics-server API 可用
kubectl get --raw /apis/metrics.k8s.io/v1beta1/pods | head -20

# 确认自定义指标 API 返回非空
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/namespaces/pay-gateway/services/*/gateway_qps | jq .

# 压测验证 HPA 是否扩容
kubectl run load-test --rm -i --restart=Never -n pay-gateway --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/fortio -- load -qps 500 -t 60s http://pay-gateway:8080/health

# 观察 Pod 数量变化
kubectl get deploy pay-gateway -n pay-gateway -w
```
## 7. 回复客户话术

> 您好，工单 TC-2026-007 已处理完成。
>
> **现象确认：** pay-gateway HPA 在 10:00 后无法获取指标，`TARGETS` 显示 `<unknown>/60%`，自定义指标 `gateway_qps` 同样不可用。
>
> **根因：**
> 1. metrics-server 的 serving 证书已过期，导致 kube-apiserver 无法通过聚合层获取 Pod CPU/内存指标；
> 2. 昨晚 Prometheus operator 升级后，adapter 规则中的 ServiceMonitor 标签未同步更新，自定义指标查询返回空。
>
> **已执行修复：**
> 1. 删除旧证书 Secret 并重启 metrics-server，使其自动重新签发证书；
> 2. 重新应用 APIService 配置；
> 3. 修正 Prometheus adapter 规则中的标签选择器并重启 adapter；
> 4. 手动触发 HPA 重新评估。
>
> **当前状态：** HPA 已能正常显示 CPU 与 gateway_qps 指标，压测下副本数已自动扩容。
>
> **后续建议：**
> - 将 metrics-server 与 Prometheus 证书过期监控加入告警；
> - 升级 Prometheus operator 前，先在预发环境验证 adapter 规则与 HPA 指标可用性；
> - 建议配置 HPA 行为参数（behavior.scaleUp/stabilizationWindowSeconds）防止抖动；
- 建议在 CI/CD 发布流程中增加 "HPA 指标可用性"  gates，发布后自动校验 TARGETS 是否可读取；
- 对关键业务 HPA，建议同时配置 CPU 与自定义指标的多指标策略，并设置 fallback 副本数，避免单一指标失效导致无法扩容；
- 建议在 ACK 控制台为 pay-gateway 开启 HPA 事件告警，当 TARGETS 持续 5 分钟为 unknown 时自动通知值班。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（已闭环）
- **是否需要变更审批：** 是（涉及 kube-system/monitoring 组件重启与 ConfigMap 修改）
- **交接信息：**
  - 已通知监控团队将 metrics-server 证书剩余 30 天告警纳入基线；
  - 若 7 天内再次出现 HPA `<unknown>`，自动升级为 P0 并启动 APIService 专项排查；
  - 本工单相关修复命令已沉淀至运维知识库，同类故障可快速复用；
- 建议对 ack-prod-vpc02 全集群做一次 HPA 健康巡检，确认所有自定义指标 API 均可正常返回；
- 若监控团队证书告警未在本周落地，将触发 P0 升级；
  - 建议将 Prometheus adapter rules 变更纳入 operator 升级 checklist，每次升级后进行 HPA 指标回归验证；
  - 本次修复命令与配置模板已同步至 GitOps 仓库，便于后续版本对比与回滚。

---

*更新时间：2026-06-26 | 责任域：生产运维/ticket-cases*

## Related

- 水平 Pod 自动扩缩容
- Metrics Server
- Prometheus (entities)
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]
- Metrics Server
- Prometheus (entities)
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]


<!-- risk-assessed -->
