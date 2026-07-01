---
title: Ingress 控制器 Pod 异常导致业务访问 404/502
description: 专有云 ACK 集群 Nginx Ingress Controller Pod 异常重启，导致外部流量出现 404/502 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- ingress
- nginx-ingress
- '404'
- '502'
- p1
- application-failure
incident_id: INC-2026-ACK-011
priority: P1
severity: high
affected_cluster: ack-zyy-prod-03
affected_namespace: ingress-nginx
ticket_type: 应用访问异常
skill_ref:
- Ingress Nginx 排障指南
- Service 不可达 FTA
fta_ref:
- 'FTA: Ingress 返回 404/502'
created: '2026-06-26T09:15:00+08:00'
updated: '2026-06-26T11:40:00+08:00'
last_updated: 2026-06-26T11:40:00+08:00
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
- Ingress 控制器 Pod 异常导致业务访问 404/502 如何处理
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
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-046-ingress-controller-404-502.md]]"
  type: related_to
- target: "[[concepts/ingress.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]"
  type: related_to
---

# 工单描述

客户反馈生产环境 ACK 集群 `ack-zyy-prod-03` 的线上业务入口出现大量 404 与 502 错误，监控显示 Nginx Ingress Controller Pod 在 `ingress-nginx` 命名空间内频繁重启。客户原始描述如下：

> “从 9 点开始，我们的商城首页和下单接口断断续续报 404 和 502，CDN 回源到 SLB，SLB 后端健康检查正常。进到集群里看 ingress-nginx 的几个 Pod 一直在 CrashLoopBackOff，describe 看到 OOMKilled。麻烦尽快处理，现在还在影响用户下单。”

影响范围为 `mall-web` 与 `mall-order` 两个命名空间，对应 Ingress 资源 `mall-web-ingress` 与 `mall-order-ingress`，外部流量经 SLB 进入 Ingress Controller 后转发至后端 Service。

## 分类与优先级判定

- **工单类型**：应用访问异常 / Ingress 控制器故障。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境业务入口持续出现 4xx/5xx，服务明显降级。
2. 根因指向 Ingress Controller Pod 异常，影响所有依赖该控制器的 Ingress 规则。
3. 业务仍在受损，需在 15 分钟内完成止血并给出修复方案。

## 诊断步骤

按“先控制器状态、再资源配置、后流量链路”的顺序排查：

```bash
# 1. 确认 Ingress Controller Pod 状态与重启原因
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o wide
kubectl describe pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx | grep -A 20 "Last State"

# 2. 查看控制器日志，定位 404/502 与重启原因
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --previous --tail=300 | grep -iE "oom|out of memory|reload|nginx: configuration|error"

# 3. 检查 Ingress 资源与后端 Service 映射
kubectl get ingress -A
kubectl describe ingress mall-web-ingress -n mall-web
kubectl describe ingress mall-order-ingress -n mall-order

# 4. 核对 Ingress Controller 的 Deployment 资源限制
kubectl get deployment ingress-nginx-controller -n ingress-nginx -o yaml | grep -A 10 resources

# 5. 通过 ACK 控制台或 SLB 查看后端健康检查状态
aliyun slb DescribeLoadBalancerHTTPListenerAttribute --LoadBalancerId lb-8vbdummymall --ListenerPort 443

# 6. 检查 ASO/天基侧 Pod 事件与节点资源
kubectl get events -n ingress-nginx --sort-by='.lastTimestamp' | tail -50
```

## 根因分析

`ingress-nginx-controller` Deployment 的内存限制为 `limit.memory=512Mi`，而实际业务 Ingress 规则数量较多，且部分规则使用了复杂的 rewrite 注解与 Lua 插件。Nginx 在 reload 配置时会短暂占用双倍内存，导致 Pod 频繁触发 OOMKilled：

```
Last State: Terminated
  Reason:    OOMKilled
  Exit Code: 137
```

控制器 Pod 反复重启期间，新旧 Nginx worker 进程无法正常交接，部分连接被异常关闭，外部表现为 502；同时由于配置 reload 未完成，部分 Ingress location 规则未加载，表现为 404。根本原因是内存 limit 设置过低，无法承载当前配置规模与连接数。

## 修复命令

**第一步：临时扩容控制器副本数并添加节点亲和，分散压力**

```bash
kubectl scale deployment ingress-nginx-controller -n ingress-nginx --replicas=4
```

**第二步：调整控制器内存限制至合理值**

```bash
kubectl set resources deployment ingress-nginx-controller -n ingress-nginx \
  --limits=memory=2Gi,cpu=2000m \
  --requests=memory=1Gi,cpu=1000m
```

**第三步：检查并清理无效或冗余的 Ingress 注解，降低 Nginx 配置复杂度**

```bash
kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\n"}{range .metadata.annotations}{@key}{": "}{@value}{"\n"}{end}{end}' | grep -E "rewrite|lua|configuration-snippet"
```

**第四步：为关键业务 Ingress 配置会话保持与健康检查，减少异常重试**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
kubectl annotate ingress mall-web-ingress -n mall-web nginx.ingress.kubernetes.io/session-cookie-name=route --overwrite
kubectl annotate ingress mall-order-ingress -n mall-order nginx.ingress.kubernetes.io/session-cookie-name=route --overwrite
```

**第五步：滚动重启控制器使资源限制生效**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment ingress-nginx-controller -n ingress-nginx
kubectl rollout status deployment ingress-nginx-controller -n ingress-nginx --timeout=300s
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 控制器 Pod 全部 Running 且没有 OOMKilled
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o wide
kubectl describe pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx | grep -A 5 "Last State"

# 2. 业务 Ingress 后端可达
kubectl get svc -n mall-web mall-web-svc
kubectl get svc -n mall-order mall-order-svc
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s -o /dev/null -w "%{http_code}" http://mall-web-svc.mall-web.svc.cluster.local/health

# 3. 外部访问验证
curl -I https://mall.example.com/home
curl -I https://order.example.com/api/health

# 4. 控制器内存使用趋势
kubectl top pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx
```

## 回复客户话术

> 您好，经排查，本次业务入口 404/502 的根因是 **Ingress Controller 内存限制过低导致频繁 OOM 重启**。控制器在处理大量 Ingress 规则与 reload 配置时超出 512Mi 内存限制，Pod 反复重启期间出现配置未加载与连接异常关闭，从而表现为 404 与 502。我们已完成以下处置：
>
> 1. 将控制器副本数临时扩容至 4，缓解单点压力；
> 2. 调整控制器内存 limit 至 2Gi、request 至 1Gi；
> 3. 滚动重启控制器并确认所有 Pod 稳定 Running；
> 4. 检查并标注了复杂的 rewrite 与 Lua 注解，建议后续清理。
>
> 当前外部访问已恢复正常，业务健康检查返回 200。建议后续：
> - 根据 Ingress 规则数量与流量规模，建立控制器资源基线；
> - 配置 Ingress Controller 内存使用率告警；
> - 对非必要的高级注解进行治理，降低配置复杂度；
> - 定期检查控制器版本，及时升级以修复已知内存泄漏与性能问题。
>
> 如有波动，请随时联系。

## 复盘与沉淀

本次故障说明 Ingress Controller 作为集群流量入口，其稳定性对业务影响极大。很多团队在部署时沿用默认的 512Mi 内存限制，随着 Ingress 规则增加、注解复杂度提升，很容易触发 OOM。reload 操作会短暂占用双倍内存，因此在设置 limit 时需要预留至少 2 倍日常峰值余量。

排障过程中应优先确认 Pod 重启原因。`OOMKilled` 的 Exit Code 为 137，容易被误认为是普通重启。若仅关注 404/502 日志而忽视控制器状态，会浪费大量时间在后端 Service 上排查。正确的顺序是：先看 Ingress Controller 是否健康，再看 Ingress 规则是否正确，最后看后端 Service 与 Pod。在专有云 ACK 环境中，还需关注 SLB 与 Ingress Controller 之间的健康检查超时时间。如果控制器重启过于频繁，SLB 可能会将短暂不可用的后端标记为异常，进一步放大业务访问失败率。

建议在 `ingress-nginx` 命名空间部署专用告警：
- Pod 重启次数 > 3 次/10 分钟触发 P2 告警；
- 内存使用率 > 80% 触发 P3 告警；
- 控制器 P99 延迟 > 500ms 触发 P2 告警；
- 5xx 错误率 > 1% 持续 2 分钟触发 P1 告警。

同时，可参考 Ingress 容量规划 对控制器副本数、节点分布、资源限制进行年度评估，避免业务扩容后再次出现入口层瓶颈。对于金融、电商等高并发场景，建议将 Ingress Controller 部署为独占节点池，避免与业务 Pod 争用 CPU 与内存。此外，可以在 SLB 层配置多个后端服务器组，将流量分散到多个控制器实例，提升入口层整体可用性。在业务高峰期前，建议进行 Ingress Controller 的压力测试，验证单实例最大连接数与内存消耗，避免因突发流量导致控制器资源耗尽。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若调整资源后仍频繁 OOM，需升级至 **ACK 网络产品支持** 与 **容器平台架构团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-011`
  - 根因：`Ingress Controller 内存限制过低，OOM 重启导致配置未加载与连接异常`
  - 影响集群：`ack-zyy-prod-03`
  - 影响命名空间：`mall-web`、`mall-order`
  - 临时修复：扩容副本 + 提升内存 limit
  - 长期方案：建立 Ingress Controller 资源基线与容量评估机制
  - 待跟进：清理冗余 Ingress 注解，更新监控告警规则

## Related

- Ingress 控制器 Pod 异常导致业务访问 404/502
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
