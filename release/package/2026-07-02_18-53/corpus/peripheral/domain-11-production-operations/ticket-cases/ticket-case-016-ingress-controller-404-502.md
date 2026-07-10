---
title: Ingress 访问异常：控制器 Pod 重启导致 404/502
description: 专有云 ACK 集群因 Ingress 控制器 Pod 异常重启、配置重载失败导致业务入口大量返回 404/502 的工单闭环样本。
summary: 专有云 ACK 集群因 Ingress 控制器 Pod 异常重启、配置重载失败导致业务入口大量返回 404/502 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- ingress
- nginx-ingress
- '404'
- '502'
- p0
- network
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:45:00+08:00'
incident_id: INC-2026-ACK-016
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-04
affected_namespace: kube-system
ticket_type: 入口流量异常
skill_ref:
- Ingress 故障排查
- HTTP 状态码诊断树
fta_ref:
- 'FTA: Ingress 返回 404/502'
last_updated: 2026-06-26 16:45:00+08:00
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
- Ingress 访问异常：控制器 Pod 重启导致 404/502 如何处理
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

客户通过业务监控发现大量用户反馈 Web 端无法访问，CDN 回源返回 502 Bad Gateway，部分静态资源请求返回 404 Not Found。客户登录 ACK 控制台后发现 Ingress 控制器 Pod 状态异常，描述如下：

> “生产环境 ACK 集群 ack-zyy-prod-04 里，nginx-ingress-controller 有 Pod 在反复重启，业务域名访问一会儿 502 一会儿 404。kubectl get pod -n kube-system 看到 nginx-ingress-controller-xxxxx 状态是 CrashLoopBackOff。我们已经确认后端 Service 和 Pod 都是 Running 的，但流量就是过不去。麻烦尽快处理。”

受影响的主要命名空间包括 `ecommerce-web`、`payment-gateway` 与 `kube-system`，受影响域名为 `api.zyy-prod.example.com` 与 `www.zyy-prod.example.com`。当前正值促销活动高峰期，每秒 QPS 约为 12,000，失败请求数持续上升。

## 分类与优先级判定

- **工单类型**：入口流量异常 / Ingress 控制器故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境入口层故障，导致外部用户无法正常访问业务，直接造成收入损失。
2. 报错集中在 502/404，且后端服务本身正常，问题定位在 Ingress 控制器层。
3. 处于促销高峰，需在 15 分钟内完成止血并恢复主要业务域名访问。

## 诊断步骤

按“先 Pod 状态、再控制器日志、再配置一致性”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Ingress 控制器 Pod 状态与重启次数
kubectl get pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller -o wide

# 2. 查看异常 Pod 的事件与重启原因
kubectl describe pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller | tail -80

# 3. 采集控制器日志，关注配置重载与 Lua 模板错误
kubectl logs -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller --tail=500 --previous | grep -iE "error|fatal|reload|template|lua|panic"

# 4. 检查 Ingress 资源配置是否包含非法字符或重复路径
kubectl get ingress --all-namespaces -o yaml > /tmp/ingress-backup-016.yaml
kubectl get ingress -A -o json | jq '.items[] | {name: .metadata.name, ns: .metadata.namespace, rules: .spec.rules}'

# 5. 检查 ConfigMap 中 nginx 模板与自定义配置是否冲突
kubectl get configmap -n kube-system nginx-configuration -o yaml
kubectl get configmap -n kube-system custom-template -o yaml

# 6. 检查 IngressClass 与控制器关联关系
kubectl get ingressclass
kubectl describe ingressclass nginx

# 7. 通过 ACK 控制台查看 SLB 监听状态与后端健康检查
aliyun slb DescribeLoadBalancerHTTPListenerAttribute \
  --LoadBalancerId lb-8vbdummyprod04 \
  --ListenerPort 443 \
  --RegionId cn-beijing

# 8. 临时绕过 Ingress，直接访问 NodePort 验证后端服务是否正常
kubectl get svc -n ecommerce-web
kubectl get svc -n payment-gateway
```
## 根因分析

经过排查，发现 `nginx-ingress-controller-6f8b9c7d4-xk2z9` Pod 处于 `CrashLoopBackOff` 状态，日志中出现以下关键错误：

```
[emerg] 1234#1234: duplicate location "/api/v1/pay" in /etc/nginx/nginx.conf:1892
nginx: [emerg] duplicate location "/api/v1/pay" in /etc/nginx/nginx.conf:1892
```

根本原因为：业务团队在 `payment-gateway` 命名空间内同时创建了两个 Ingress 资源，均配置了相同 host `api.zyy-prod.example.com` 与相同路径 `/api/v1/pay`，但指向不同的 Service（`payment-service-v1` 与 `payment-service-v2`）。nginx-ingress-controller 在合并所有 Ingress 规则生成 `nginx.conf` 时，检测到重复的 location 块，导致配置重载失败并退出进程。控制器 Pod 因此进入 CrashLoopBackOff 循环，无法继续同步任何新的 Ingress 规则，所有业务域名均受影响。

此外，控制器重启期间，旧 nginx worker 进程仍持有部分连接，但无法处理新的 Ingress 变更，导致部分请求路由到已删除的后端或返回 404；而新请求因 worker 不足返回 502。

## 修复命令

**第一步：快速止血，临时通过 ACK 控制台将 SLB 流量切换到备用 Ingress 控制器 Deployment（如存在灰度控制器）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若存在灰度控制器，临时切换 IngressClass 默认注解
kubectl annotate ingressclass nginx ingressclass.kubernetes.io/is-default-class- -n kube-system
kubectl annotate ingressclass nginx-canary ingressclass.kubernetes.io/is-default-class=true -n kube-system
```
**第二步：定位并删除冲突的重复 Ingress 规则（保留正确的版本）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找所有包含 api.zyy-prod.example.com 与 /api/v1/pay 的 Ingress
kubectl get ingress -A --field-selector metadata.namespace=payment-gateway -o json | \
  jq '.items[] | select(.spec.rules[].host=="api.zyy-prod.example.com" and .spec.rules[].http.paths[].path=="/api/v1/pay") | {name: .metadata.name, service: .spec.rules[].http.paths[].backend.service.name}'

# 确认后删除重复版本（示例：删除 payment-api-v2，保留 payment-api-v1）
kubectl delete ingress payment-api-v2 -n payment-gateway
```
**第三步：修复后重启 Ingress 控制器 Pod 以强制重新加载配置**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment nginx-ingress-controller -n kube-system
kubectl rollout status deployment nginx-ingress-controller -n kube-system --timeout=180s
```
**第四步：若仍无法启动，临时清空控制器缓存并重新同步**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
kubectl delete pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller --grace-period=0 --force  # ⚠️ 跳过优雅终止，可能丢数据
kubectl wait --for=condition=Ready pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller --timeout=120s
```
**第五步：恢复默认 IngressClass（若第一步做了切换）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl annotate ingressclass nginx-canary ingressclass.kubernetes.io/is-default-class- -n kube-system
kubectl annotate ingressclass nginx ingressclass.kubernetes.io/is-default-class=true -n kube-system
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Ingress 控制器 Pod 全部 Running 且重启次数不再增加
kubectl get pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller -o wide

# 2. 检查控制器日志无 emerg 错误
kubectl logs -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller --tail=100 | grep -iE "error|emerg|fatal" || echo "no error logs"

# 3. 验证 nginx.conf 已重新生成且语法正确
kubectl exec -n kube-system -it $(kubectl get pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller -o jsonpath='{.items[0].metadata.name}') -- nginx -t

# 4. 验证业务域名访问正常
for host in api.zyy-prod.example.com www.zyy-prod.example.com; do
  curl -s -o /dev/null -w "%{http_code}\n" https://$host/healthz
done

# 5. 支付接口路径不再冲突
kubectl get ingress -n payment-gateway payment-api-v1 -o jsonpath='{.spec.rules[*].http.paths[*].path}'

# 6. 监控 5xx/4xx 比例下降
kubectl top pod -n kube-system -l app.kubernetes.io/name=nginx-ingress-controller
```
## 回复客户话术

> 您好，经排查，本次 Ingress 访问异常的根因是 **payment-gateway 命名空间内存在两条重复的 Ingress 规则**，均声明了相同的 host `api.zyy-prod.example.com` 与路径 `/api/v1/pay`，导致 nginx-ingress-controller 合并配置时生成重复的 location 块，配置重载失败并触发 Pod 反复重启。我们已完成以下处置：
>
> 1. 临时确认后端 Service 与 Pod 状态正常，排除业务层故障；
> 2. 删除重复的 `payment-api-v2` Ingress，保留正确的 `payment-api-v1`；
> 3. 重启并滚动更新 nginx-ingress-controller，确认配置语法测试通过；
> 4. 验证业务域名 `api.zyy-prod.example.com` 与 `www.zyy-prod.example.com` 访问正常，5xx/4xx 比例已回落。
>
> 当前入口流量已恢复。建议后续：
> - 在 CI/CD 或 GitOps 流程中增加 Ingress 规则冲突检测，禁止同一命名空间内出现相同 host+path 组合；
> - 配置 Ingress 5xx 告警 与控制器 Pod 重启告警；
> - 使用 `kubectl diff` 或 ACK 控制台预览 Ingress 变更，避免重复规则上线。
>
> 如有新异常，请随时联系。

## 复盘与沉淀

本次故障暴露出两个关键问题：第一，Ingress 规则缺乏前置校验，同一团队在短时间内上线了两套相似规则而未发现冲突；第二，Ingress 控制器作为集群入口单点，其配置失败会导致全局性影响，缺乏灰度入口做快速切换。

nginx-ingress-controller 的核心工作是将所有 Ingress、Service、Endpoint 信息合并为一份 nginx 配置。任何导致 nginx 语法错误的规则都会使控制器无法启动，因此需要在发布前做配置校验。建议在 GitLab CI 或 Argo CD 中集成 `kubectl apply --dry-run=server` 与 `nginx -t` 等价校验，必要时使用 `ingress-nginx` 提供的 admission controller 在 API Server 层拦截非法规则。

针对高可用入口架构，建议：
1. 部署多副本 Ingress 控制器，并配置 PodDisruptionBudget 与 HPA；
2. 使用两套独立的 IngressClass（如 `nginx` 与 `nginx-canary`），在主线控制器故障时可快速切换 DNS 或 SLB 权重；
3. 将关键业务域名的 Ingress 分散到不同控制器实例组，避免单点配置错误影响所有域名。

后续 SOP 更新要点：
1. 新增 Ingress 时必须检查同 host+path 是否已存在；
2. 促销高峰期间禁止非紧急的 Ingress 变更；
3. 在 Prometheus 中配置告警：`nginx_ingress_controller_nginx_reload_errors_total` 或 Pod 重启次数 > 3 次/5 分钟触发 P1 告警；
4. 将本案例写入 Ingress 404/502 回复模板，缩短后续同类工单响应时间。

最后，建议在每次重大变更后执行 `curl` 基线探测，并记录 RTO（恢复时间目标）与受影响请求数，用于后续 SRE 复盘与容量规划。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若后续出现配置持续同步失败或控制器版本 Bug，需升级至 **ACK 产品支持** 与 **网络基础设施团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-016`
  - 根因：`payment-gateway` 命名空间内重复 Ingress 规则导致 nginx 配置重载失败
  - 影响集群：`ack-zyy-prod-04`
  - 影响域名：`api.zyy-prod.example.com`、`www.zyy-prod.example.com`
  - 临时修复：删除重复 Ingress 并重启控制器
  - 长期方案：CI 增加 Ingress 冲突校验、部署灰度入口、配置入口层监控告警
  - 待跟进：确认 CI 校验规则落地、更新 SOP 与入口架构评审

## Related

- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
