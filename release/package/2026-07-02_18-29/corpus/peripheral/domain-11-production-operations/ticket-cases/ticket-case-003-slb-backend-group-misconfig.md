---
title: Service 无法访问：专有云 SLB 后端服务器组配置异常
description: 专有云 ACK 集群 LoadBalancer 类型 Service 因 SLB 后端服务器组配置异常导致外部无法访问的工单闭环样本。
summary: 专有云 ACK 集群 LoadBalancer 类型 Service 因 SLB 后端服务器组配置异常导致外部无法访问的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- slb
- loadbalancer
- service
- network
- p1
tier: peripheral
created: '2026-06-26T10:00:00+08:00'
updated: '2026-06-26T11:20:00+08:00'
incident_id: INC-2026-ACK-003
priority: P1
severity: high
affected_cluster: ack-zyy-prod-03
affected_namespace: default
ticket_type: 网络故障
skill_ref:
- SLB 问题排查
- K8s LoadBalancer Service
fta_ref:
- 'FTA: Service 无法访问-SLB 后端异常'
last_updated: 2026-06-26 11:20:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Service 无法访问：专有云 SLB 后端服务器组配置异常 如何处理
trigger_keywords:
- Service
prerequisites:
- kubectl-basics
- k8s-networking
- alicloud-basics
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
- target: '[[concepts/service.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-044-kubeproxy-service-unreachable.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户报告通过公网域名 `api.zyy-prod.example.com` 访问订单网关时返回 502/504，ACK 集群内通过 Service ClusterIP 访问正常。客户描述：

> “从外网访问我们的 API 网关一直 502，但是集群里面 curl service 是好的。SLB 控制台看监听端口正常，后端服务器组也加了 ECS，但健康检查显示异常。是不是 SLB 后端配置被改掉了？”

受影响集群 `ack-zyy-prod-03`，命名空间 `default`，Service 类型为 LoadBalancer，SLB 实例 ID `lb-8vbdummy03`，监听端口 443/80。

## 分类与优先级判定

- **工单类型**：网络故障 / 入口流量异常。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境入口流量异常，外网用户访问失败，但集群内部正常，说明 K8s Service 与后端 Pod 本身健康。
2. 问题集中在 SLB 后端服务器组配置，属于可控的云资源层面问题。
3. 需在 15 分钟内完成定位并修复，无需立即进行集群级变更。

## 诊断步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Service 状态与 External IP
kubectl get svc api-gateway -n default -o wide
kubectl describe svc api-gateway -n default

# 2. 查看 Endpoint 与 Pod 健康
kubectl get endpoints api-gateway -n default
kubectl get pod -n default -l app=api-gateway -o wide

# 3. 检查 Cloud Controller Manager 日志中 SLB 相关事件
kubectl logs -n kube-system -l app=cloud-controller-manager --tail=200 | grep -i "api-gateway|backend|VServerGroup"

# 4. 查询 SLB 监听与后端服务器组
aliyun slb DescribeLoadBalancerAttribute --LoadBalancerId lb-8vbdummy03 --RegionId cn-zhangjiakou
aliyun slb DescribeVServerGroups --LoadBalancerId lb-8vbdummy03 --RegionId cn-zhangjiakou

# 5. 查看后端服务器健康检查状态
aliyun slb DescribeVServerGroupBackendServers \
  --VServerGroupId rsp-8vbdummy03 \
  --RegionId cn-zhangjiakou \
  --output cols=ServerId,Port,Weight,Description rows=BackendServers.BackendServer[]

# 6. 检查监听配置
aliyun slb DescribeLoadBalancerHTTPListenerAttribute --LoadBalancerId lb-8vbdummy03 --ListenerPort 80 --RegionId cn-zhangjiakou
aliyun slb DescribeLoadBalancerHTTPSListenerAttribute --LoadBalancerId lb-8vbdummy03 --ListenerPort 443 --RegionId cn-zhangjiakou
```
## 根因分析

在 ACK 控制台操作记录中发现，前一天运维同学手动在 SLB 控制台修改了 `VServerGroupId` 为 `rsp-8vbdummy03-old` 的后端服务器组，将部分 ECS 端口从 `NodePort 30080` 改为了 `30443`，但对应监听仍为 80/443。Cloud Controller Manager 虽然周期性同步，但 Service 注解中未配置强制覆盖，导致监听 80 的后端端口实际是 30443，健康检查失败，外网流量全部返回 502。

根因可归纳为：
1. 手动修改 SLB 后端服务器组端口，与 K8s Service 期望的 NodePort 不一致；
2. Cloud Controller Manager 的同步策略未开启 `service.beta.kubernetes.io/alicloud-loadbalancer-force-override-listeners: "true"`；
3. 缺少 SLB 配置漂移告警。

## 修复命令

**第一步：备份当前 SLB 配置**

```bash
aliyun slb DescribeVServerGroupBackendServers --VServerGroupId rsp-8vbdummy03 --RegionId cn-zhangjiakou > /tmp/slb-api-gateway-backup.json
```

**第二步：在 Service 中声明强制覆盖，确保 CCM 重新同步后端组**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl annotate svc api-gateway -n default \
  service.beta.kubernetes.io/alicloud-loadbalancer-force-override-listeners=true \
  service.beta.kubernetes.io/alicloud-loadbalancer-backend-type=eni \
  --overwrite
```
**第三步：修正 Service 的 target-port 与 NodePort 映射**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch svc api-gateway -n default --type='json' -p='[
  {"op": "replace", "path": "/spec/ports/0/targetPort", "value": 8080},
  {"op": "replace", "path": "/spec/ports/0/nodePort", "value": 30080},
  {"op": "replace", "path": "/spec/ports/1/targetPort", "value": 8443},
  {"op": "replace", "path": "/spec/ports/1/nodePort", "value": 30443}
]'
```
**第四步：触发 CCM 重新同步**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl delete pod -n kube-system -l app=cloud-controller-manager
kubectl rollout status deployment/cloud-controller-manager -n kube-system --timeout=120s
```
**第五步：若 CCM 同步后仍异常，手动修正 VServerGroup 端口**

```bash
aliyun slb SetVServerGroupAttribute \
  --VServerGroupId rsp-8vbdummy03 \
  --RegionId cn-zhangjiakou \
  --BackendServers '[{"ServerId":"i-8vbdummy03a","Port":30080,"Weight":100,"Type":"ecs"},{"ServerId":"i-8vbdummy03b","Port":30080,"Weight":100,"Type":"ecs"}]'
```

## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Service External IP 与端口正常
kubectl get svc api-gateway -n default

# 2. 后端服务器组健康检查通过
aliyun slb DescribeHealthStatus --LoadBalancerId lb-8vbdummy03 --ListenerPort 80 --RegionId cn-zhangjiakou
aliyun slb DescribeHealthStatus --LoadBalancerId lb-8vbdummy03 --ListenerPort 443 --RegionId cn-zhangjiakou

# 3. 从外部探测
curl -I https://api.zyy-prod.example.com/health
curl -I http://api.zyy-prod.example.com/health

# 4. CCM 无报错
kubectl logs -n kube-system -l app=cloud-controller-manager --tail=100 | grep api-gateway
```
## 回复客户话术

> 您好，外网访问 502 的根因已定位：**SLB 后端服务器组端口被手动修改，与 ACK Service 期望的 NodePort 不一致，导致健康检查失败**。
>
> 已执行修复：
>
> - 为 Service 添加强制覆盖注解，确保 Cloud Controller Manager 自动同步 SLB 配置；
> - 修正 Service 的 target-port 与 NodePort 映射；
> - 重启 CCM 触发同步，并验证后端健康检查全部通过。
>
> 当前 `https://api.zyy-prod.example.com/health` 已返回 200。后续建议：
>
> - 禁止直接登录 SLB 控制台修改由 K8s 管理的后端组；
> - 在 Service 中显式声明 `force-override-listeners: "true"`；
> - 配置 SLB 配置漂移告警，防止手动变更未被回滚。

## 复盘与沉淀

专有云 ACK 使用 LoadBalancer 类型 Service 时，SLB 资源的生命周期由 Cloud Controller Manager（CCM）管理，但在实际运维中经常出现“双头管理”问题：一部分配置由 K8s 注解驱动，另一部分由运维同学在 SLB 控制台手动调整。当两者不一致时，CCM 的同步行为取决于注解 `force-override-listeners` 与 `force-override-listener` 是否开启。若未开启，CCM 会采取保守策略，不会回滚手动修改，从而导致后端端口、权重、健康检查模板等关键参数漂移。

本例中，监听 80 的后端组端口被改为 30443，流量到达后端节点后实际访问的是 HTTPS NodePort，协议不匹配，健康检查自然失败。类似的陷阱还包括：监听协议与后端端口协议不一致、会话保持配置被手动关闭、证书被替换但 Service 注解未同步、虚拟服务器组权重被手动调整后导致流量不均。

为彻底避免此类问题，建议：
1. 在 Service 模板中固化所有 SLB 相关注解，禁止在 SLB 控制台进行任何修改；
2. 对 SLB 资源启用配置漂移检测，定期比对 CCM 期望状态与 SLB 实际状态；
3. 在 CI/CD 流水线中对 Service YAML 进行校验，确保 `nodePort`、`targetPort`、`protocol` 三者一致；
4. 配置 SLB 后端健康检查失败告警，在健康检查异常率达到阈值时第一时间触发。

对于使用 `backend-type=eni` 的场景，后端直接挂载 Pod ENI IP，不经过 NodePort，此时更需关注 Pod 就绪状态与 ENI 附属关系。若后续切换为 ENI 直连模式，需同步调整监听健康检查路径，避免误报。

后续 SOP 更新要点：
1. 建立 SLB 控制台只读权限矩阵，仅平台账号保留写权限；
2. 每月执行一次 `aliyun slb DescribeVServerGroupAttribute` 与 Service YAML 的 diff 检查；
3. 将强制覆盖注解加入所有生产 LoadBalancer Service 的准入策略；
4. 把本案例写入 SLB 后端配置异常回复模板。

此外，建议在 GitOps 仓库中为每个 LoadBalancer Service 维护一份“期望状态快照”，并通过定时任务比对 SLB 控制台实际配置。一旦出现漂移，立即触发告警并自动回滚。该机制可与 配置漂移治理 流程联动，确保所有入口流量的变更是可审计、可回滚的。

同时，可在 CI/CD 中集成 `kube-linter` 或 `datree` 策略，强制校验 LoadBalancer Service 必须包含 `force-override-listeners` 注解，避免新服务上线时遗漏。该策略可作为 策略即代码 的一部分统一管控。

最后，建议在变更窗口后执行一次全链路压测，验证 SLB 后端组在 CCM 重启、节点替换、Service 更新等场景下仍能保持一致。

该机制同样适用于 Ingress、NLB 等其他入口资源，确保所有南北向流量配置都纳入统一的漂移治理范围。

## 是否需要升级及交接信息

- **是否升级**：已修复，无需升级；若后续多次出现 SLB 配置被手动篡改，需升级至 **安全合规团队** 审计账号权限与变更流程。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-003`
  - 根因：`SLB VServerGroup 后端端口与 Service NodePort 不一致`
  - 影响 Service：`default/api-gateway`
  - 修复方式：Service 注解强制覆盖 + CCM 同步 + 手动修正
  - 待跟进：纳入变更审计，关闭 SLB 控制台写权限给非平台账号

## Related

- Service
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- kube-proxy 异常导致 Service 不通


<!-- risk-assessed -->
