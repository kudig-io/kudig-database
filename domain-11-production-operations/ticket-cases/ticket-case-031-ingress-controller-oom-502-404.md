---
title: Ingress 控制器 Pod 异常导致 404/502
description: 专有云 ACK 集群 Nginx Ingress Controller 因内存限制过低触发 OOMKilled，导致七层入口流量返回 502/404
  的工单闭环样本。
category: production-operations
tags:
- ack
- zyy
- nginx-ingress
- oom
- '502'
- '404'
- ingress
- p1
incident_id: TC-2026-031
priority: P1
severity: high
affected_cluster: ack-zyy-prod-01
affected_namespace: ingress-nginx
ticket_type: 应用入口访问故障
skill_ref:
- '[[domain-03-networking-traffic/00-core-k8s-networking/21-nginx-ingress-complete-guide.md|Nginx
  Ingress 完全指南]]'
- '[[domain-03-networking-traffic/00-core-k8s-networking/25-ingress-monitoring-troubleshooting.md|Ingress
  监控与排障]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/nginx-ingress-fta.md|FTA: Nginx
  Ingress 故障]]'
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md|FTA: Ingress
  访问异常]]'
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
last_updated: 2026-06-26T16:30:00+08:00
duplicate_of: TC-2026-021
status: duplicate
duplication_reason: 与 TC-2026-021 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Ingress 控制器 Pod 异常导致 404/502 如何处理
trigger_keywords:
- ack
- zyy
- nginx-ingress
- oom
- '502'
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
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-041-ingress-controller-502.md]]"
  type: related_to
- target: "[[concepts/ingress.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]"
  type: related_to
---

# 工单描述

客户反馈生产环境通过域名 `api.zyy-prod.example.com` 访问订单中心接口时，间歇性出现 502 Bad Gateway 与 404 Not Found，刷新后时而正常、时而失败。客户描述如下：

> “今天下午把 Nginx Ingress 的版本升到 1.11 之后，入口流量就不太稳。kubectl 看 ingress-nginx namespace 里有一个 controller Pod 一直在 CrashLoopBackOff。curl 集群内 Service 直接访问后端是正常的，说明业务 Pod 没问题，应该就是 Ingress 这层出问题了。麻烦尽快定位，很多外部渠道在调这个域名。”

该集群为专有云 `ack-zyy-prod-01`，Ingress 部署在 `ingress-nginx` 命名空间，受影响业务命名空间包括 `order-service`、`payment-service` 与 `user-service`，当前为交易高峰时段。

## 分类与优先级判定

- **工单类型**：应用入口访问故障 / Ingress 控制器异常。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境七层入口流量异常，外部渠道调用失败，影响面跨多个业务命名空间。
2. Ingress Controller Pod 处于 CrashLoopBackOff，说明控制面本身不稳定，随时可能完全中断。
3. 业务后端 Service 与 Pod 正常，根因集中在 Ingress 层，具备快速止血条件，需在 15 分钟内给出方案。

## 诊断步骤

按“先 Pod 状态、后 Controller 日志、再配置与资源”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 确认 Ingress Controller Pod 状态与资源使用
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o wide
kubectl describe pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# 2. 查看 Controller 事件，关注 OOMKilled、CrashLoopBackOff
kubectl get events -n ingress-nginx --sort-by='.lastTimestamp' | tail -50

# 3. 采集异常 Pod 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=300 --previous

# 4. 检查 Ingress 规则与 Endpoint 状态
kubectl get ingress -A
kubectl get endpoints -n order-service
kubectl get endpoints -n payment-service

# 5. 进入正常 Controller Pod 检查 nginx 配置合法性
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -t

# 6. 检查 Controller Deployment 资源限制与副本数
kubectl get deploy ingress-nginx-controller -n ingress-nginx -o yaml | grep -A 10 resources

# 7. 使用 ACK 诊断工具扫描 Ingress 路径
ack-cli ingress diagnose --cluster ack-zyy-prod-01 --namespace order-service

# 8. 检查 ASO/天基侧 Ingress 相关组件事件
kubectl get event -n kube-system --field-selector reason=IngressControllerDown --sort-by='.lastTimestamp'
```

## 根因分析

综合 Pod 状态、日志与配置校验，判定根因为 **Nginx Ingress Controller 内存限制过低，配置重载时触发 OOMKilled**，置信度 **高**。

1. **资源限制缺陷**：当前 Deployment 中 Controller 容器 `memory.limit=512Mi`。在升级至 1.11 后，lua 模板、SSL 证书缓存与后端 Endpoint 列表占用内存增加，启动时首次 `nginx -s reload` 即超过 512Mi，被 kubelet OOMKilled。
2. **副本数与 PDB 不足**：Deployment 仅设置 `replicas=1`，Pod 崩溃期间无可用控制器，所有 Ingress 流量既无法被正确路由，也回退到 default backend，表现为 404；部分已建立长连接被异常重置，表现为 502。
3. **业务后端正常**：直接访问后端 ClusterIP 返回 200，说明问题不在业务 Pod，而在七层代理层。

## 修复命令

**第一步：临时扩容 Controller 副本，保证至少一只能在旧配置下运行**

```bash
kubectl scale deploy ingress-nginx-controller --replicas=2 -n ingress-nginx
```

**第二步：调大 Controller 内存限制并增加 HPA 最小副本**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch deploy ingress-nginx-controller -n ingress-nginx --type=strategic --patch='{
  "spec": {
    "template": {
      "spec": {
        "containers": [
          {
            "name": "controller",
            "resources": {
              "limits": {"memory": "1Gi", "cpu": "1000m"},
              "requests": {"memory": "512Mi", "cpu": "250m"}
            }
          }
        ]
      }
    }
  }
}'
```

**第三步：重新滚动部署，使新资源限制生效**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deploy ingress-nginx-controller -n ingress-nginx
kubectl rollout status deploy ingress-nginx-controller -n ingress-nginx --timeout=300s
```

**第四步：配置 PodDisruptionBudget，保证升级期间入口可用**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  minAvailable: 1
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
EOF
```

**第五步：在 ACK 控制台确认 SLB 监听后端全部 Controller Pod 健康**

```bash
aliyun slb DescribeLoadBalancerAttribute \
  --LoadBalancerId lb-8vbdummynginx \
  --output cols=ListenerPorts rows=ListenerPorts.ListenerPort[]
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. Controller Pod 全部 Running 且资源限制已生效
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.spec.containers[0].resources.limits.memory}{"\n"}{end}'

# 2. nginx 配置校验通过
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -t

# 3. 从集群内模拟七层访问
kubectl run ingress-test --rm -it --restart=Never -n default --image=registry-vpc.cn-zhangjiakou.aliyuncs.com/acs/busybox:latest -- \
  wget -qO- --header "Host: api.zyy-prod.example.com" http://ingress-nginx-controller.ingress-nginx.svc.cluster.local/health

# 4. 外部域名探测 30 次，确认无 502/404
for i in $(seq 1 30); do
  curl -s -o /dev/null -w "%{http_code}" https://api.zyy-prod.example.com/health
  echo ""
  sleep 1
done | sort | uniq -c

# 5. 确认 Pod 无 OOMKilled 事件
kubectl get events -n ingress-nginx --field-selector reason=OOMKilled --sort-by='.lastTimestamp' | tail -10
```

## 回复客户话术

> 您好，工单 TC-2026-031 已处理完成。
>
> **现象确认：** 通过 `api.zyy-prod.example.com` 访问订单中心接口时出现 502/404，集群内直接访问后端 Service 正常。
>
> **根因：** Nginx Ingress Controller 容器内存限制设置为 512Mi，升级到 1.11 后启动重载时内存不足，被 kubelet 反复 OOMKilled。由于副本数为 1，控制器不可用期间七层路由失败，外部流量返回 502/404。
>
> **已执行修复：**
> 1. 将 Controller 副本数临时扩到 2，并配置 `minAvailable=1` 的 PodDisruptionBudget；
> 2. 将内存限制上调至 1Gi，请求值保持 512Mi；
> 3. 滚动重启 Controller，确认 nginx 配置校验通过。
>
> **当前状态：** 外部域名 30 次探测全部返回 200，Controller Pod 全部 Running，无新 OOMKilled 事件。
>
> **后续建议：**
> - 为 Ingress Controller 开启 HPA，建议最小副本 2，最大 8，避免单点故障；
> - 参考 [[domain-03-networking-traffic/00-core-k8s-networking/25-ingress-monitoring-troubleshooting.md|Ingress 监控与排障]] 配置入口延迟、5xx 比例、Controller CPU/内存使用率告警；
> - 升级 Nginx Ingress 前在预发环境按实际证书与 Ingress 数量压测内存占用；
> - 将 Controller 接入 Prometheus，采集 `nginx_ingress_controller_requests` 与容器 OOM 指标。
>
> 如有异常请随时联系。

## 复盘与沉淀

本次故障说明 Ingress Controller 作为七层入口，其稳定性直接决定外部流量可用性。单副本部署在版本升级或配置重载时极易因瞬时内存冲高而中断服务。对于 Nginx Ingress 这类有状态缓存（SSL 证书、lua 共享内存、后端 Endpoint 列表）的控制器，不能简单按普通无状态服务配置 512Mi 内存。建议在版本升级前，使用 `helm template` 或 `kubectl diff` 对比新旧版本资源默认值，并在预发环境模拟真实证书数量和 Ingress 条目数进行压测。

同时，应将 Ingress Controller 纳入容量规划基线：最小副本 2，配置 PDB，启用 HPA（基于 CPU/内存/自定义 QPS 指标）。在 Prometheus 中重点监控 `container_memory_working_set_bytes`、`nginx_ingress_controller_nginx_process_connections_total` 以及 `nginx_ingress_controller_requests` 的 5xx 比例。对于专有云 ACK 客户，还需关注 SLB 后端健康检查状态与 Terway 网络路径，避免将控制器问题误判为业务问题。

此外，建议在升级变更窗口中预留回滚方案：保存旧版本 Deployment YAML，若新版本持续异常可立即 `kubectl rollout undo` 恢复。日常巡检中，应检查 Ingress Controller 的 OOM 事件数、重启次数与后端 Endpoint 变化频率，将入口可用性作为 SLO 核心指标之一。

告警规则示例：当 `nginx_ingress_controller_nginx_process_connections_total` 增长率异常，或容器 `restart_count` 在 5 分钟内大于 2 次时触发 P1 告警；当 5xx 比例超过 1% 持续 2 分钟时触发 P0 告警。通过这些前置监控，可以在入口完全中断前发现控制器异常。

## 是否需要升级及交接信息

- **是否升级**：已闭环，无需升级。若后续出现大量 5xx 且 Controller 资源正常，需升级至 **网络团队** 排查 SLB/Terway 路径。
- **是否需要变更审批**：是（修改生产环境核心入口 Deployment 资源限制，已登记变更台账）。
- **交接信息**：
  - 故障单号：`TC-2026-031`
  - 根因：`Nginx Ingress Controller 内存限制过低触发 OOMKilled，单副本导致入口不可用`
  - 影响集群：`ack-zyy-prod-01`
  - 修复动作：扩容副本 + 提升内存限制 + 配置 PDB
  - 待跟进：观察 24 小时内存使用曲线，评估是否需要进一步扩容到 2Gi 或启用 HPA

## Related

- Ingress 控制器 Pod 异常导致 404/502
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
