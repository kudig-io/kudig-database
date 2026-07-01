---
title: Ingress 控制器 Pod 异常导致业务访问 404/502
description: 专有云 ACK 集群 Nginx Ingress Controller Pod 重启后进入 CrashLoopBackOff，导致外部流量
  404/502，含诊断、修复与验证。
summary: 专有云 ACK 集群 Nginx Ingress Controller Pod 重启后进入 CrashLoopBackOff，导致外部流量 404/502，含诊断、修复与验证。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- ingress
- nginx-ingress-controller
- 404
- 502
- crashloopbackoff
- p0
tier: peripheral
created: '2026-06-26T09:15:00+08:00'
updated: '2026-06-26T11:40:00+08:00'
incident_id: TC-2026-036
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-03
affected_namespace: kube-system
ticket_type: 入口流量故障
skill_ref:
- Ingress 故障诊断
- Pod CrashLoopBackOff 排查
fta_ref:
- 'FTA: Ingress 404/502'
last_updated: 2026-06-26 11:40:00+08:00
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
- Ingress 控制器 Pod 异常导致业务访问 404/502 如何处理
trigger_keywords:
- ack
- zyy
- ingress
- nginx-ingress-controller
- 404
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
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-046-ingress-controller-404-502.md]]'
  type: related_to
- target: '[[concepts/ingress.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---



# 工单描述

客户通过云监控告警发现 ACK 专有云集群 `ack-zyy-prod-03` 的入口流量成功率骤降，大量外部请求返回 404 或 502。客户描述如下：

> “我们的电商大促活动页从 09:00 开始大量报 502，刷新后偶尔 404。域名是 `promo.example.com`，指向 ACK 的 Ingress。kubectl 看 `kube-system` 里的 nginx-ingress-controller Pod 好像在重启，状态是 CrashLoopBackOff。麻烦紧急处理，大促流量还在涨。”

受影响命名空间主要为 `promo-activity` 与 `kube-system`，核心活动页服务 `promo-web` 无法被外部访问。

## 分类与优先级判定

- **工单类型**：入口流量故障 / Ingress 控制器异常。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境集群入口流量异常，直接影响外部用户访问。
2. Ingress Controller Pod 进入 CrashLoopBackOff，属于控制面入口组件故障，影响所有依赖该控制器的域名。
3. 处于业务大促高峰期，需在 15 分钟内止血并恢复入口流量。

## 诊断步骤

按“先控制器状态、后配置、再后端服务”的顺序排查：

```bash
# 1. 确认 Ingress Controller Pod 状态
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o wide

# 2. 查看 Controller 重启原因与事件
kubectl describe pod -n kube-system -l app.kubernetes.io/name=ingress-nginx | grep -A 30 Events
kubectl get events -n kube-system --field-selector reason=BackOff --sort-by='.lastTimestamp' | tail -30

# 3. 采集 Controller 日志，定位启动失败原因
kubectl logs -n kube-system -l app.kubernetes.io/name=ingress-nginx --tail=200 --previous 2>/dev/null || \
kubectl logs -n kube-system -l app.kubernetes.io/name=ingress-nginx --tail=200

# 4. 检查 Ingress 资源配置与冲突
kubectl get ingress -A
kubectl get ingress -n promo-activity promo-web-ingress -o yaml

# 5. 验证后端 Service 与 Endpoints 状态
kubectl get svc -n promo-activity
kubectl get endpoints -n promo-activity promo-web

# 6. 检查节点资源是否导致 Controller 被驱逐
kubectl describe node $(kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].spec.nodeName}') | grep -A 10 Conditions

# 7. 通过 ASO/ACK 控制台确认 SLB 与 Ingress Controller 的关联是否正常
ack-cli ingress status --cluster ack-zyy-prod-03 --ingress-class nginx
```

## 根因分析

Nginx Ingress Controller Pod `nginx-ingress-controller-6f8d9c4b5-xk7m2` 因配置文件语法错误导致启动失败，进入 CrashLoopBackOff。具体根因为：

1. **配置冲突：** 运维人员前一天通过 ConfigMap `nginx-configuration` 新增了一条 `server-snippet` 配置，其中包含未闭合的 Lua 代码块，Nginx 在 reload 时解析失败。
2. **级联影响：** Controller 启动时加载所有 Ingress 与 ConfigMap 配置，语法错误导致进程直接退出，kubelet 反复重启，Controller 无法对外提供入口代理服务。
3. **404/502 来源：** 外部 SLB 健康检查探测 Controller 暴露的 80/443 端口失败，SLB 将后端节点标记为不健康，流量到达已 unhealthy 的节点后返回 502；部分请求因缺少有效 upstream 映射返回 404。

根因置信度：**高**（日志中明确出现 `nginx: [emerg] unexpected end of file, expecting "}"`）。

### 风险与影响评估

- **业务影响：** `promo.example.com` 为大促活动页入口，入口流量异常直接影响用户下单与活动参与，故障期间估算失败请求约 12 万条。
- **扩散风险：** Nginx Ingress Controller 为集群级组件，一个 ConfigMap 错误即可导致所有依赖该 Ingress Class 的域名受影响，而非单个业务。
- **数据风险：** 不涉及数据丢失，但可能导致用户请求重试、日志突增及后端服务空载。
- **恢复关键：** 快速定位并移除问题配置，比修复语法本身更重要，因此优先回滚再优化。

## 修复命令

**第一步：隔离问题配置，临时恢复默认 Nginx 配置**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 备份当前问题 ConfigMap
kubectl get configmap nginx-configuration -n kube-system -o yaml > /tmp/nginx-configuration-backup-$(date +%Y%m%d-%H%M%S).yaml

# 移除导致语法错误的 server-snippet 字段
kubectl patch configmap nginx-configuration -n kube-system --type=json -p='[
  {"op": "remove", "path": "/data/server-snippet"}
]'
```

**第二步：重启 Ingress Controller 以重新加载配置**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment nginx-ingress-controller -n kube-system
kubectl rollout status deployment nginx-ingress-controller -n kube-system --timeout=180s
```

**第三步：确认 Controller Pod 全部 Running**

```bash
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o wide
```

**第四步：修复 server-snippet 语法并重新应用（可选，变更窗口期执行）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-configuration
  namespace: kube-system
data:
  server-snippet: |
    location /healthz {
      access_log off;
      return 200 "healthy\n";
    }
EOF
kubectl rollout restart deployment nginx-ingress-controller -n kube-system
```

## 验证命令

```bash
# 1. Controller Pod 全部 Running 且重启次数不再增加
kubectl get pod -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.containerStatuses[0].restartCount}{"\n"}{end}'

# 2. 从集群内部测试 Ingress 入口连通性
kubectl run ingress-test --rm -it --restart=Never -n default --image=registry.aliyuncs.com/acs/busybox -- \
  wget -qO- --timeout=5 http://promo-web.promo-activity.svc.cluster.local:8080/health

# 3. 通过 Ingress 暴露的域名测试（在可访问公网的节点或堡垒机执行）
curl -I -H "Host: promo.example.com" http://<ingress-slb-ip>/api/health

# 4. 检查 SLB 后端健康状态
aliyun slb DescribeHealthStatus --LoadBalancerId lb-zyy-ingress-xxx --output cols=ListenerPort,BackendServers rows=BackendServers.BackendServer[]

# 5. 查看 Controller 日志无 emerg 错误
kubectl logs -n kube-system -l app.kubernetes.io/name=ingress-nginx --tail=100 | grep -i "emerg|error" || echo "无 emerg 错误"
```

## 回复客户话术

> 您好，工单 TC-2026-036 已处理完成。
>
> **现象确认：** 大促活动页 `promo.example.com` 09:00 起出现大量 404/502，Nginx Ingress Controller Pod 处于 CrashLoopBackOff。
>
> **根因：** `kube-system/nginx-configuration` ConfigMap 中新增的 `server-snippet` 存在语法错误（Lua 代码块未闭合），导致 Nginx 启动时解析失败，Controller 反复重启，无法代理入口流量。
>
> **已执行修复：**
> 1. 备份并移除问题 `server-snippet` 配置；
> 2. 滚动重启 Nginx Ingress Controller；
> 3. 确认所有 Controller Pod 恢复 Running，SLB 后端健康检查通过。
>
> **当前状态：** 入口流量成功率恢复正常，活动页访问正常，Controller 日志无 emerg 错误。
>
> **后续建议：**
> - 对 Nginx Ingress ConfigMap 变更增加语法校验，可在 CI 中集成 `nginx -t` 或 Helm lint；
> - 变更 `server-snippet` 等高级配置前，先在预发环境灰度验证；
> - 配置 Ingress Controller 多副本并设置 PodDisruptionBudget，提升入口层高可用；
> - 参考 Ingress 故障诊断 建立入口流量监控看板。
>
> 如有异常请随时联系。

## 复盘与沉淀

本次故障典型地反映了 Nginx Ingress Controller 作为集群入口控制面的“单点脆弱性”：一个 ConfigMap 的语法错误即可导致所有 Ingress 流量中断。在专有云 ACK 环境中，Nginx Ingress 通常以 Deployment 形式部署，并通过 SLB 暴露，虽然 Deployment 本身可以多副本，但只要 ConfigMap 错误，所有副本都会因配置解析失败而同时 CrashLoopBackOff。

排查过程中需要特别注意区分 404 与 502 的来源。404 通常意味着请求到达了 Controller，但找不到对应的 Ingress 规则或 upstream；502 通常意味着 Controller 本身不可用，或后端 Service 没有健康 Endpoints。本例中两者同时出现，说明 Controller 在重启过程中状态不稳定，SLB 健康检查在 healthy 与 unhealthy 之间切换。

建议在后续变更管理中建立以下机制：
1. **ConfigMap 变更前置校验：** 在 CI 或 GitOps 流程中对 `nginx-configuration` 变更进行 `nginx -t` 语法检查，尤其是包含 `server-snippet`、`http-snippet`、`location-snippet` 等高级字段时；
2. **灰度发布策略：** 对 Ingress Controller 配置变更，先在测试集群或一个副本上应用，观察无异常后再全量滚动；
3. **入口层监控：** 在 Prometheus 中配置 Ingress Controller 进程数、5xx 比例、upstream 健康状态、ConfigMap reload 失败次数等告警；
4. **回滚脚本化：** 将 ConfigMap 备份与快速回滚脚本沉淀到 Ingress 回滚模板，缩短后续同类故障恢复时间。

此外，建议在专有云 ACK 控制台或 ASO 侧关注 SLB 后端健康状态变化。Controller 重启期间，SLB 会逐个将后端节点置为 unhealthy，若健康检查阈值配置过严，可能在 Controller 尚未完全恢复时就将全部节点剔除，导致更长时间的流量中断。

## 是否需要升级及交接信息

- **是否升级**：否（已闭环）。若后续频繁出现 ConfigMap 配置导致 Controller 启动失败，需升级至 **平台工程团队**  review 配置变更流程。
- **交接信息**：
  - 故障单号：`TC-2026-036`
  - 根因：`nginx-configuration` ConfigMap 中 `server-snippet` 语法错误
  - 影响集群：`ack-zyy-prod-03`
  - 影响命名空间：`promo-activity`、`kube-system`
  - 临时修复：移除问题配置并滚动重启 Controller
  - 长期方案：建立 ConfigMap 变更语法校验与灰度机制
  - 待跟进：将修复后的 server-snippet 在变更窗口期重新下发并验证

## Related

- Ingress 控制器 Pod 异常导致业务访问 404/502
- Ingress
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
