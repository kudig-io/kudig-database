---
title: 阿里云专有云 NetworkPolicy 误拦截导致服务间调用 503
description: 新上线 NetworkPolicy 后微服务间调用出现 503，根因是策略缺少 namespaceSelector 与 podSelector
  放行规则，含诊断、修复与验证。
summary: 新上线 NetworkPolicy 后微服务间调用出现 503，根因是策略缺少 namespaceSelector 与 podSelector 放行规则，含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- networkpolicy
- network-policy
- calico
- terway
- 503
- service-mesh
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-010
priority: P1
severity: high
affected_cluster: ack-prod-vpc02
affected_namespace: risk-engine
ticket_type: 网络安全策略故障
skill_ref: NetworkPolicy 拦截诊断
fta_ref: 'FTA: 服务间调用 503'
last_updated: 2026-06-26
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 NetworkPolicy 误拦截导致服务间调用 503 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- networkpolicy
- network-policy
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 010：NetworkPolicy 误拦截导致服务间调用 503

## 1. 工单描述

**用户原始描述：**

> 今天下午 17:00 我们在 risk-engine 命名空间上线了一条 NetworkPolicy，想限制只有 api-gateway 能访问 risk-engine 里的 score-service。结果上线后，score-service 调用 rule-engine 一直 503，rule-engine 调用 config-service 也 503。score-service 到 api-gateway 的入口流量倒是正常的。我们用的是 ACK 专有云的 Terway 网络，Calico 模式。namespace 是 risk-engine。请帮忙看一下策略哪里配错了，业务风控链路已经受影响了。

## 2. 分类与优先级判定

- **任务类型：** 网络安全策略 / NetworkPolicy 误拦截 / 服务间调用失败
- **优先级：** P1（生产环境 + 服务间调用失败 + 风控链路受影响）
- **严重程度：** high
- **响应时限：** 15 分钟内给出修复方案
- **安全级别：** 中风险（修改网络安全策略，需确认最小权限原则）

## 3. 诊断步骤

### 3.1 确认 Pod 与 Service 503 现象

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get pod -n risk-engine
kubectl get svc -n risk-engine

# 进入 score-service Pod 测试到 rule-engine 的连通性
kubectl exec -it deploy/score-service -n risk-engine -- /bin/sh
wget -qO- http://rule-engine.risk-engine.svc.cluster.local:8080/health || echo "FAILED"
```
### 3.2 列出并检查 NetworkPolicy

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy -n risk-engine -o yaml
kubectl describe networkpolicy -n risk-engine
```
### 3.3 检查 CNI 与 NetworkPolicy 实现

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 ACK 集群网络插件为 Terway + Calico Policy Controller
kubectl get daemonset -n kube-system | grep -E "terway|calico"
kubectl get pod -n kube-system -l app=calico-policy-controller

# 查看 Calico 全局网络策略（如有）
kubectl get globalnetworkpolicy
```
### 3.4 检查 Pod 标签与选择器匹配

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod -n risk-engine --show-labels
kubectl get pod -n risk-engine -l app=rule-engine --show-labels
kubectl get pod -n risk-engine -l app=config-service --show-labels
```
### 3.5 抓包与 Calico 日志排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在 rule-engine Pod 所在节点上抓包
tcpdump -i any -nn host <rule-engine-pod-ip> and port 8080

# 查看 Calico Felix 日志
kubectl logs -n kube-system -l k8s-app=calico-node --tail=200 | grep -i "policy|deny|drop"
```
### 3.6 诊断过程补充说明

NetworkPolicy 的排障难点在于其默认行为与直观理解存在差异。很多人误以为只写 Ingress 规则就不会影响出向流量，但实际上一旦声明了某个 policyType 而无具体规则，该方向就会被默认拒绝。因此诊断时应先确认 `policyTypes` 字段，再看每个方向是否有明确的 allow 规则。

在 ACK 专有云 Terway + Calico 模式下，NetworkPolicy 由 Calico Felix 实际执行。如果 Calico 的 GlobalNetworkPolicy 与 namespace 级 NetworkPolicy 同时存在，两者会叠加生效，可能出现预期之外的拦截。诊断时可以通过 Calico 日志中的 `denied` 关键字快速定位被丢弃的数据包，并结合 tcpdump 确认是 SYN 被丢弃还是连接建立后异常。

## 4. 根因分析

综合 NetworkPolicy 规则、Pod 标签与 503 发生路径，判定根因为 **"新 NetworkPolicy 只定义了 Ingress 规则，未定义 Egress 规则，导致 Pod 出站请求被默认拒绝"**，置信度 **高**。

1. **策略缺陷：** 用户创建的 NetworkPolicy 仅包含 `policyTypes: ["Ingress"]`，未声明 `Egress`。在 Kubernetes NetworkPolicy 语义中，一旦某个 policyType 被声明但无规则，则该方向流量默认拒绝。因此 score-service 主动访问 rule-engine 的出站流量被丢弃。
2. **标签匹配问题：** Ingress 规则中 `podSelector` 使用了 `app: api-gateway`，但未限定 `namespaceSelector`，在单一名称空间场景下可工作，但若 api-gateway 在其他 namespace 则会同时失效。
3. **503 来源：** 出向连接被网络层丢弃后，客户端连接超时，服务框架（如 Spring Cloud LoadBalancer）返回 503 Service Unavailable。

### 4.1 风险与影响评估

- **业务影响：** risk-engine 为风控核心链路，score-service → rule-engine → config-service 调用链中断将直接影响风险决策与交易放行。
- **扩散风险：** 若该 NetworkPolicy 被复制到其他命名空间，可能引发更大范围的服务间调用故障。
- **数据风险：** 不涉及数据丢失，但连接超时可能导致业务重试、日志暴增与用户体验下降。

## 5. 修复命令

### 5.1 临时回滚（快速恢复业务）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若业务影响严重，先删除该策略恢复默认放行
kubectl delete networkpolicy score-service-ingress-only -n risk-engine

# 验证流量恢复
kubectl exec -it deploy/score-service -n risk-engine -- \
  wget -qO- http://rule-engine.risk-engine.svc.cluster.local:8080/health
```
### 5.2 编写最小权限 NetworkPolicy（推荐）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<'EOF' | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: risk-engine-default
  namespace: risk-engine
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    # 允许 api-gateway namespace 中 app=api-gateway 的 Pod 访问 risk-engine 所有服务
    - from:
        - namespaceSelector:
            matchLabels:
              name: api-gateway
        - podSelector:
            matchLabels:
              app: api-gateway
      ports:
        - protocol: TCP
          port: 8080
  egress:
    # 允许 risk-engine 内部所有 Pod 互访
    - to:
        - podSelector: {}
      ports:
        - protocol: TCP
          port: 8080
    # 允许访问 DNS（CoreDNS）
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
EOF
```
### 5.3 针对 score-service 单独放行的精细化策略

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<'EOF' | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-score-service-egress
  namespace: risk-engine
spec:
  podSelector:
    matchLabels:
      app: score-service
  policyTypes:
    - Egress
  egress:
    - to:
        - podSelector:
            matchLabels:
              app: rule-engine
      ports:
        - protocol: TCP
          port: 8080
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
EOF
```
### 5.4 验证策略生效

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Calico 策略是否同步
kubectl exec -it calico-node-xxxxx -n kube-system -- calicoctl get networkpolicy -n risk-engine

# 在 Terway 模式下，也可检查 Pod 网络标识
kubectl exec -it deploy/score-service -n risk-engine -- ip addr
```
## 6. 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 NetworkPolicy 已应用
kubectl get networkpolicy -n risk-engine -o yaml

# 2. 连续测试服务间调用，确认无 503
kubectl exec -it deploy/score-service -n risk-engine -- /bin/sh -c '
ok=0; fail=0
for i in $(seq 1 30); do
  code=$(wget -qO- --timeout=3 http://rule-engine.risk-engine.svc.cluster.local:8080/health >/dev/null 2>&1 && echo 200 || echo 503)
  if [ "$code" = "200" ]; then ok=$((ok+1)); else fail=$((fail+1)); fi
done
echo "OK: $ok, FAIL: $fail"
'

# 3. 测试 rule-engine 到 config-service
kubectl exec -it deploy/rule-engine -n risk-engine -- /bin/sh -c '
for i in $(seq 1 10); do
  wget -qO- --timeout=3 http://config-service.risk-engine.svc.cluster.local:8080/health && echo "" || echo "FAIL"
done
'

# 4. 确认 api-gateway 仍可访问 score-service
kubectl run api-gateway-test --rm -it --restart=Never -n api-gateway --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/busybox:latest -- \
  wget -qO- --timeout=5 http://score-service.risk-engine.svc.cluster.local:8080/health

# 5. 确认 Calico Felix 日志无新 drop
kubectl logs -n kube-system -l k8s-app=calico-node --tail=100 | grep -i "drop|deny" || echo "无拦截日志"
```
## 7. 回复客户话术

> 您好，工单 TC-2026-010 已处理完成。
>
> **现象确认：** risk-engine 命名空间 17:00 上线 NetworkPolicy 后，score-service → rule-engine、rule-engine → config-service 出现 503，入口流量正常。
>
> **根因：** 新 NetworkPolicy 仅声明了 `Ingress` 方向，未声明 `Egress`。在 Kubernetes 语义中，声明了 policyType 但无规则时，该方向默认拒绝，因此 score-service 主动访问 rule-engine 的出站流量被 Calico/Terway 丢弃，业务层表现为 503。
>
> **已执行修复：**
> 1. 临时删除问题策略恢复业务；
> 2. 重新下发最小权限 NetworkPolicy：
>    - Ingress：仅允许 api-gateway namespace 中 `app=api-gateway` 访问 risk-engine 8080 端口；
>    - Egress：允许 risk-engine 内部 Pod 互访，并放行 DNS 出站；
> 3. 对 score-service 单独补充精细化 Egress 规则。
>
> **当前状态：** 30 次服务间调用全部成功，api-gateway 入口访问正常，Calico 日志无新拦截记录。
>
> **后续建议：**
> - 上线 NetworkPolicy 前，务必在预发环境验证 Ingress 与 Egress 双向规则；
> - 建议逐步从 "默认放行" 过渡到 "默认拒绝 + 白名单"，避免一次性收紧导致误拦截；
> - 对关键服务可先用 `kubectl run` 做连通性基线测试，再应用策略；
- 建议使用 GitOps 管理 NetworkPolicy，变更前强制 peer review，并标注 policyTypes 与影响范围；
- 对 Terway/Calico 环境，建议在预发环境模拟 GlobalNetworkPolicy 与 namespace 级策略的叠加效果；
- 建议在风险引擎命名空间设置默认 deny-all Egress 策略，并逐项显式放行，避免新服务上线后默认无出站权限。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（已闭环）
- **是否需要变更审批：** 是（NetworkPolicy 为网络安全策略变更，已记录变更台账）
- **交接信息：**
  - 已将修复后的 NetworkPolicy 清单提交至 GitOps 仓库；
  - 建议安全团队复盘并补充 NetworkPolicy 变更评审 checklist；
  - 若 24 小时内 risk-engine 其他服务出现同类 503，需检查是否存在更细粒度策略冲突；
  - 本案例已沉淀至网络安全策略故障知识库，供后续参考；
- 建议安全团队本周内输出 NetworkPolicy 变更评审 checklist，并纳入 CI 门禁；
- 若 risk-engine 其他服务后续出现策略冲突，将升级为网络安全专项排查；
  - 建议对现有所有 NetworkPolicy 进行一致性审计，重点检查 policyTypes 与 egress 规则完整性；
  - 本次修复后的 NetworkPolicy 模板已提交至 GitOps，作为风险域命名空间安全基线参考。

---

*更新时间：2026-06-26 | 责任域：domain-11-production-operations/ticket-cases*


<!-- risk-assessed -->
