---
title: Ingress 控制器 Pod 异常导致 404/502
description: 专有云 ACK 集群因 ingress-nginx 控制器配置重载失败导致部分域名返回 502/404 的工单闭环样本。
summary: 专有云 ACK 集群因 ingress-nginx 控制器配置重载失败导致部分域名返回 502/404 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- ingress
- nginx-ingress
- '502'
- '404'
- p0
tier: core
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:15:00+08:00'
incident_id: INC-2026-ACK-041
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-04
affected_namespace: ingress-nginx
ticket_type: 应用入口故障
skill_ref:
- '[[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-03-networking-traffic/03-api-gateway/01-api-gateway-production-operations|API
  网关生产运维]]'
- '[[domain-10-troubleshooting-diagnostics/FTA故障树/list/ingress-fta.md|Ingress 异常故障树分析]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/FTA故障树/list/nginx-ingress-fta.md|FTA:
  Nginx Ingress 异常]]'
last_updated: 2026-06-26 16:15:00+08:00
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
- target: '[[concepts/ingress.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈其部署在专有云 ACK 集群 `ack-zyy-prod-04` 的电商导购入口，从 14:10 开始部分用户访问 `https://mall.example.com` 时出现 502 Bad Gateway，少量路径返回 404 Not Found。客户描述如下：

> “我们前端 SLB 健康检查正常，但业务方说很多页面打不开。kubectl 看 ingress-nginx namespace 里的 Pod 有 Restart，describe ingress 看到 backend 是空的。用的是 nginx-ingress-controller，麻烦尽快看一下是不是控制器挂了。”

受影响命名空间主要为 `ingress-nginx` 与 `mall-prod`，当前正值大促预热期，入口流量较高。

## 分类与优先级判定

- **工单类型**：应用入口故障 / Ingress 控制器故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境业务入口返回 502/404，直接影响终端用户访问。
2. 控制器 Pod 重启且配置重载异常，故障面覆盖所有依赖该控制器的 Ingress。
3. 处于大促预热期，需在 15 分钟内止血并恢复入口可用。

## 诊断步骤

按“先入口状态、后控制器日志、再配置一致性”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 ingress-nginx 控制器 Pod 状态
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o wide
kubectl describe pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# 2. 查看控制器事件与重启原因
kubectl get events -n ingress-nginx --sort-by='.lastTimestamp' | tail -50

# 3. 检查 nginx 配置重载是否失败
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -c controller --tail=200 | grep -iE "error|reload|fail|invalid"

# 4. 检查 Ingress 资源 backend 与路径配置
kubectl get ingress -n mall-prod -o wide
kubectl describe ingress mall-frontend -n mall-prod

# 5. 验证 upstream Service Endpoints 是否为空
kubectl get endpoints mall-api -n mall-prod
kubectl get pod -n mall-prod -l app=mall-api -o wide

# 6. 进入控制器容器查看当前 nginx.conf 语法
kubectl exec -n ingress-nginx -it deploy/ingress-nginx-controller -- nginx -t

# 7. 通过 ACK 控制台查看 SLB 监听与后端服务器组状态
ack-cli ingress status --cluster ack-zyy-prod-04 --namespace mall-prod
```
## 根因分析

控制器 Pod `ingress-nginx-controller-7c9d4b8f5-x2k4p` 在 14:08 因 OOM 被 Kill 后重启。重启后持续出现配置重载失败：

```
nginx: [emerg] duplicate location "/api/v1/promo" in /etc/nginx/nginx.conf:1274
Error: exit status 1
Unexpected error reloading NGINX: exit status 1
```

根本原因是 `mall-prod` 命名空间内两个 Ingress 资源 `mall-frontend` 与 `mall-backend` 均配置了相同 host `mall.example.com` 且路径 `/api/v1/promo` 冲突。此前控制器缓存了旧配置，在 Pod 重启后重新聚合生成 nginx.conf 时触发语法错误，导致所有 Ingress 配置无法重载，控制器持续返回旧 snapshot 或 502/404。这种错误在 nginx-ingress-controller 的 admission webhook 未启用或版本较旧时尤为常见，因为 API Server 不会主动校验不同 Ingress 对象之间的路径冲突。

## 修复命令

**第一步：隔离问题配置，临时恢复控制器重载**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 备份并删除冲突的重复路径 Ingress
cd /tmp && kubectl get ingress mall-backend -n mall-prod -o yaml > mall-backend-ingress-backup.yaml
kubectl annotate ingress mall-backend -n mall-prod kubectl.kubernetes.io/last-applied-configuration-
kubectl delete ingress mall-backend -n mall-prod
```
**第二步：强制触发控制器配置重载**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/ingress-nginx-controller -n ingress-nginx
kubectl rollout status deployment/ingress-nginx-controller -n ingress-nginx --timeout=120s
```
**第三步：将冲突路径合并到单一 Ingress 并指定优先级**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -n mall-prod -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mall-merged
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  ingressClassName: nginx
  rules:
  - host: mall.example.com
    http:
      paths:
      - path: /api/v1/promo
        pathType: Prefix
        backend:
          service:
            name: mall-promo-api
            port:
              number: 8080
      - path: /
        pathType: Prefix
        backend:
          service:
            name: mall-frontend
            port:
              number: 80
EOF
```
**第四步：验证 nginx 语法与控制器状态**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -n ingress-nginx -it deploy/ingress-nginx-controller -- nginx -t
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -c controller --tail=50 | grep -i "reload"
```
若合并后的 Ingress 仍无法通过校验，可立即回滚到备份的 `mall-backend-ingress-backup.yaml`，并临时将流量切换到备用集群或维护页面，避免大促期间入口长时间不可用。

## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 控制器 Pod 全部 Running 且 Ready
kubectl get pod -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'

# 2. 配置重载成功且无 emerg 错误
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -c controller --tail=100 | grep -iE "reload|emerg|error" | tail -20

# 3. 业务入口返回 200
kubectl run ingress-test --image=registry.aliyuncs.com/acs/busybox --restart=Never -n default --rm -it -- wget -qO- --timeout=10 http://mall-frontend.mall-prod.svc.cluster.local/api/v1/promo/health

# 4. 外部域名验证
curl -I -m 10 https://mall.example.com/api/v1/promo/health

# 5. mall-prod 业务 Pod 全部 Running
kubectl get pod -n mall-prod -o wide | grep -v Running
```
验证过程中需特别关注控制器是否仍输出 `nginx: [emerg]` 错误。若存在，则说明仍有其他未被发现的 Ingress 冲突，需要继续扫描所有 Ingress 的 host 与 path 组合。同时建议保留问题发生时的控制器日志与 nginx.conf 备份，便于后续复盘与故障定责。

## 回复客户话术

> 您好，经排查，本次入口 502/404 的根因是 **ingress-nginx 控制器配置重载失败**。`mall-prod` 命名空间内存在两条 Ingress 配置了重复的 host 与路径 `/api/v1/promo`，控制器重启后生成 nginx.conf 时报 `duplicate location` 语法错误，导致所有 Ingress 无法生效。我们已完成以下处置：
>
> 1. 删除冲突的重复 Ingress 并备份配置；
> 2. 重启控制器使其重新加载配置；
> 3. 合并路径到单一 Ingress，避免再次冲突。
>
> 当前控制器 Pod 全部 Running，nginx 配置测试通过，业务域名返回 200。建议后续：
> - 统一收口 Ingress 变更，避免同一 host/path 重复定义；
> - 为控制器配置合理的 memory limit，避免 OOM 导致重启；
> - 配置 Ingress 配置重载失败告警。
>
> 如有新异常请随时联系。

## 复盘与沉淀

本次故障说明 Ingress 控制器的健壮性不仅取决于 Pod 是否 Running，更取决于配置重载是否成功。即使控制器进程存活，错误的 Ingress 对象也会通过聚合配置影响整个入口平面。在专有云 ACK 中，nginx-ingress-controller 通常以 Deployment 形式部署多个副本，但所有副本共享同一套聚合配置，因此单个命名空间的错误配置可能引发全局入口异常。

排障时应优先检查控制器日志中的 `nginx: [emerg]` 与 `exit status 1`，这是定位配置冲突的最直接证据。同时需要关注控制器是否因 OOM 重启，OOM 本身会触发配置的重新加载，从而暴露历史遗留的配置冲突问题。建议将控制器内存 limit 从默认 512Mi 提升到至少 1Gi（视 Ingress 数量而定），并配置 HPA 以应对大促流量。

对于配置冲突的识别，可以编写脚本定期扫描同一 IngressClass 下相同 host 与 path 的组合，或在 admission webhook 中实现重复路径检测。另外，建议将控制器的 `publish-service` 与 `election-id` 配置与 SLB 健康检查联动，当所有控制器副本均无法重载配置时，SLB 应自动将流量切换到备用集群或返回维护页面，避免用户持续遭遇 502/404。

后续 SOP 更新要点：
1. 在 CI/CD 中增加 Ingress 配置校验，使用 `kubectl apply --dry-run=server` 与 `ingress-nginx` 的 `admission controller` 拦截重复路径；
2. 在 Prometheus 中监控 `nginx_ingress_controller_config_last_reload_successful` 指标，值为 0 时触发 P1 告警；
3. 大促前对控制器进行压力测试，验证在 10 万级 Ingress 规则下的重载耗时与内存占用；
4. 将本案例写入 Ingress 502 回复模板，缩短后续响应时间。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若类似配置冲突多次出现，需升级至 **平台工程团队** 推进 Ingress 变更规范与 admission 校验。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-041`
  - 根因：Ingress 路径重复导致 nginx-ingress-controller 配置重载失败
  - 影响命名空间：`mall-prod`、`ingress-nginx`
  - 临时修复：删除冲突 Ingress 并合并路径
  - 长期方案：Ingress 变更校验 + 控制器内存优化 + 配置重载监控
  - 待跟进：确认大促期间控制器资源充足，更新 SOP 与告警规则

## Related

- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
