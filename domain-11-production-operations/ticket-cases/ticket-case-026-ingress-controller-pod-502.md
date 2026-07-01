---
title: Ingress 控制器 Pod 异常导致 404/502
description: 专有云 ACK 集群 Ingress-Nginx 控制器因内存限制过小而 OOM，导致外部流量出现大量 404/502 的工单闭环样本。
summary: 专有云 ACK 集群 Ingress-Nginx 控制器因内存限制过小而 OOM，导致外部流量出现大量 404/502 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- ingress-nginx
- '502'
- '404'
- oom
- p0
- application-access
tier: peripheral
created: '2026-06-26T09:00:00+08:00'
updated: '2026-06-26T11:20:00+08:00'
incident_id: INC-2026-ACK-026
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-03
affected_namespace: kube-system
ticket_type: 应用访问异常
skill_ref:
- Ingress 访问异常诊断
- ACK Ingress-Nginx 运维
fta_ref:
- 'FTA: Ingress 404/502 根因分析'
last_updated: 2026-06-26 11:20:00+08:00
duplicate_of: TC-2026-021
status: duplicate
duplication_reason: 与 TC-2026-021 主题重复，内容角度相似，降低 RAG 权重
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Ingress 控制器 Pod 异常导致 404/502 如何处理
trigger_keywords:
- Ingress
prerequisites:
- kubectl-basics
- k8s-networking
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
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[concepts/ingress.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---



# 工单描述

客户反馈其部署在专有云 ACK 集群 `ack-zyy-prod-03` 的电商业务从外部访问出现大量 **404** 与 **502** 错误，受影响域名包括 `api.shop.example.com` 与 `checkout.shop.example.com`。客户通过阿里云 SLB 控制台看到后端健康检查全部失败，业务入口流量被 SLB 置为不可用。客户描述如下：

> “今天上午 10 点开始，我们的移动端和 PC 端下单页面都打不开，浏览器返回 502 Bad Gateway。kubectl 看业务 Pod 都是 Running，但 ingress-nginx 控制器好像有问题，Pod 一直在重启。请尽快处理。”

该集群命名空间主要为 `e-commerce`（业务）与 `kube-system`（Ingress 控制器），处于大促预热期，外部访问量是平时的 3 倍以上。

## 分类与优先级判定

- **工单类型**：应用访问异常 / Ingress 控制器故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境业务入口全部异常，外部用户无法访问，符合“服务不可用”标准。
2. 业务 Pod 本身正常，问题集中在 Ingress-Nginx 控制器层，影响面覆盖所有通过该控制器暴露的域名。
3. 大促预热期，需在 15 分钟内给出止血方案并恢复流量。

## 诊断步骤

按“先控制器状态、再 SLB 后端、最后配置与资源”的顺序排查：

```bash
# 1. 查看 Ingress-Nginx 控制器 Pod 状态
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o wide

# 2. 检查控制器重启原因与事件
kubectl describe pod -n kube-system -l app.kubernetes.io/name=ingress-nginx | grep -A 30 Events
kubectl get events -n kube-system --field-selector reason=OOMKilled --sort-by='.lastTimestamp'

# 3. 采集控制器日志，关注 nginx reload、SSL 与 upstream 报错
kubectl logs -n kube-system deployment/ingress-nginx-controller --tail=500 | grep -iE "oom|out of memory|nginx reload|upstream|error"

# 4. 检查业务 Ingress、Service、Endpoint 是否完整
kubectl get ingress -n e-commerce
kubectl get svc -n e-commerce
kubectl get endpoints -n e-commerce

# 5. 查看 SLB 后端服务器组健康状态
aliyun slb DescribeLoadBalancerAttribute \
  --LoadBalancerId lb-zyyprod03api \
  --output cols=VipAddress,LoadBalancerStatus rows=LoadBalancer

aliyun slb DescribeVServerGroups \
  --LoadBalancerId lb-zyyprod03api \
  --output cols=VServerGroupId,VServerGroupName rows=VServerGroups.VServerGroup[]

# 6. 检查控制器 Deployment 资源限制与副本数
kubectl get deployment ingress-nginx-controller -n kube-system -o yaml | grep -A 10 resources

# 7. 使用 ACK 控制台网络诊断或 ack-cli 检查 Ingress 路由
ack-cli ingress diagnose -n e-commerce --cluster ack-zyy-prod-03
```

## 根因分析

`ack-zyy-prod-03` 集群使用 ACK 托管版默认安装的 **Ingress-Nginx Controller**，以 Deployment 形式运行在 `kube-system` 命名空间，副本数为 2，默认内存限制为 **1Gi**。近期大促预热导致业务 Ingress 数量从 120 个增加到 380 个，SSL 证书数量也同步增加。Nginx 每次 reload 时需要将大量 Ingress 配置加载到内存，控制器实际内存占用峰值超过 1Gi，触发 OOMKilled：

```
State:          Waiting
Reason:         CrashLoopBackOff
Last State:     Terminated
Reason:         OOMKilled
Exit Code:      137
```

控制器 Pod 反复重启期间，Readiness 探针失败，Kubernetes Endpoint 中控制器 Pod 的 ready 状态变为 false。ACK 会自动将控制器 Service 后端从 SLB 虚拟服务器组中摘除，导致所有经过该 SLB 转发的流量返回 502；而部分请求在控制器重启瞬间命中无后端状态，SLB 返回 404。根本原因是 **Ingress 控制器内存 Limit 与业务规模不匹配**，而非业务应用本身故障。

## 修复命令

**第一步：隔离异常副本，临时扩容控制器副本数以快速恢复流量**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
# 临时扩容到 4 个副本，并滚动重启
kubectl scale deployment ingress-nginx-controller -n kube-system --replicas=4
kubectl rollout restart deployment ingress-nginx-controller -n kube-system
```

**第二步：调整控制器内存限制，从 1Gi 提升到 2Gi，并预留足够 request**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch deployment ingress-nginx-controller -n kube-system --type='merge' -p '{
  "spec": {
    "template": {
      "spec": {
        "containers": [
          {
            "name": "controller",
            "resources": {
              "requests": {"cpu": "500m", "memory": "1Gi"},
              "limits":   {"cpu": "2000m", "memory": "2Gi"}
            }
          }
        ]
      }
    }
  }
}'
```

**第三步：为控制器启用 HPA，根据 CPU/内存自动伸缩**

```bash
kubectl autoscale deployment ingress-nginx-controller -n kube-system \
  --min=4 --max=8 --cpu-percent=70 \
  --name=ingress-nginx-controller-hpa
```

**第四步：清理无效或过期 Ingress 规则，降低 nginx 配置规模**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 找出 30 天内没有 Endpoint 的 Ingress
kubectl get ingress -A -o json | jq -r '
  .items[] | select(.status.loadBalancer.ingress | length == 0) |
  "\(.metadata.namespace)/\(.metadata.name)"
' | head -20

# 在业务侧确认后删除废弃规则（示例）
# kubectl delete ingress old-campaign-2025 -n e-commerce
```

**第五步：等待滚动更新完成并观察内存使用**

```bash
kubectl rollout status deployment ingress-nginx-controller -n kube-system --timeout=300s
kubectl top pod -n kube-system -l app.kubernetes.io/name=ingress-nginx
```

## 验证命令

```bash
# 1. 控制器 Pod 全部 Running 且 Ready
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o wide

# 2. 内存使用在限制范围内
kubectl top pod -n kube-system -l app.kubernetes.io/name=ingress-nginx

# 3. SLB 后端服务器组恢复健康
aliyun slb DescribeHealthStatus \
  --LoadBalancerId lb-zyyprod03api \
  --output cols=ServerId,Status rows=BackendServers.BackendServer[]

# 4. 外部访问测试，预期返回 200
curl -I -H "Host: api.shop.example.com" http://lb-zyyprod03api.elb.aliyuncs.com/health

# 5. 业务核心接口返回正常
kubectl get ingress -n e-commerce -o json | jq '.items[].status.loadBalancer'

# 6. HPA 已生效
kubectl get hpa ingress-nginx-controller-hpa -n kube-system
```

## 回复客户话术

> 您好，经排查，本次业务入口 404/502 的根因是 **Ingress-Nginx 控制器 Pod 因内存限制不足触发 OOM 反复重启**。大促预热期间 Ingress 数量与 SSL 配置增加，控制器内存峰值超过默认 1Gi Limit，导致 Readiness 探针失败、SLB 后端被摘除。我们已完成以下处置：
>
> 1. 临时扩容控制器副本数至 4，并滚动重启恢复流量；
> 2. 将控制器内存 Limit 从 1Gi 提升至 2Gi，CPU Limit 同步上调；
> 3. 配置 HPA，使控制器能够根据负载自动伸缩；
> 4. 排查并清理无效 Ingress 规则，降低配置规模。
>
> 当前外部访问已恢复，SLB 后端健康检查全部通过。建议后续：
> - 在大促前基于 Ingress 数量评估控制器内存基线，参考 容量规划；
> - 配置 Ingress 控制器资源使用率告警；
> - 将业务域名按重要性分级，关键域名使用独立 Ingress Class 或多副本控制器。
>
> 如流量继续冲高，请随时联系。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若内存继续增长或单副本异常反复，需升级至 **ACK 产品支持** 与 **网络基础设施团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-026`
  - 根因：`Ingress-Nginx 控制器内存 Limit 不足导致 OOM 重启`
  - 影响集群：`ack-zyy-prod-03`
  - 影响命名空间：`kube-system`、`e-commerce`
  - 临时修复：扩容副本 + 提升内存 Limit + 启用 HPA
  - 长期方案：按业务规模设定控制器内存基线，清理废弃 Ingress，启用资源告警
  - 待跟进：观察 24 小时内存趋势，确认 HPA 触发阈值是否合理

## 复盘与沉淀

本次故障体现了 **Ingress 控制器资源与业务入口规模不匹配** 的典型风险。很多团队将注意力放在业务 Pod 的扩缩容上，却忽略了控制器作为流量网关的资源需求。当 Ingress 数量、SSL 证书数量或 location 块数量大幅增长时，Nginx 工作进程的内存占用会线性增加，默认 1Gi 的 Limit 很容易成为瓶颈。

复盘要点：
1. **容量规划应包含控制器维度**：在业务上线前，应评估 Ingress 数量、证书数量、QPS 峰值，据此调整控制器副本数与内存 Limit。参考 容量规划 SOP。
2. **可观测性覆盖控制器层**：除了业务 Pod 的 CPU/内存，还需监控 `ingress-nginx-controller` 的内存使用、reload 耗时、5xx 错误数、SLB 后端健康状态。建议配置 P2 告警：控制器内存使用率 > 80% 持续 5 分钟。
3. **避免无效 Ingress 堆积**：长期未清理的测试域名、过期活动页 Ingress 会增加 nginx 配置负担。建议每季度审计一次 Ingress 列表，删除无流量、无 Endpoint 的规则。
4. **高可用架构**：对于关键业务，可部署多套 Ingress Controller，使用不同 `ingressClassName` 隔离关键域名与普通域名，避免单点故障导致全部入口异常。

后续 SOP 更新要点：
- 将控制器默认内存 Limit 写入集群创建模板：Ingress 数量 > 200 时，Limit 不低于 2Gi；
- 在 Prometheus 中新增告警：`container_memory_working_set_bytes{container="controller"} / container_spec_memory_limit_bytes > 0.85`；
- 将本案例写入 Ingress 502 回复模板，缩短后续同类工单响应时间。

## Related

- Ingress 控制器 Pod 异常导致 404/502
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
