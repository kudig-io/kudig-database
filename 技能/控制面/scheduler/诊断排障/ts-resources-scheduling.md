---
title: 资源调度故障排查
description: '# 资源调度故障排查'
summary: '1. **配额状态**：`kubectl get resourcequota -n <ns>`、`kubectl describe resourcequota <name>`。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- resources-scheduling
- kubelet
- helm
- hpa
- vpa
- pdb
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 资源调度故障排查 是什么
- 如何 资源调度故障排查
trigger_keywords:
- 资源调度故障排查
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 资源调度故障排查

### 01 Resources Quota Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **配额状态**：`kubectl get resourcequota -n <ns>`、`kubectl describe resourcequota <name>`。
2. **LimitRange 约束**：`kubectl describe limitrange -n <ns>`，确认默认/最大/最小限制。
3. **OOM 证据**：`kubectl describe pod <pod> | grep -i oom`，节点上 `dmesg | grep -i killed`。
4. **调度失败原因**：`kubectl describe pod <pod> | grep -A20 Events`，关注资源不足/污点/亲和性。
5. **资源可用性**：`kubectl top nodes/pods` 与 `kubectl describe nodes`。
6. **快速缓解**：
   - 临时提升配额或削减请求值。
   - 清理失败/完成 Job 与 Evicted Pod 释放配额。
7. **证据留存**：保存配额/LimitRange YAML、Pod 事件与节点资源快照。

#### 排查方法与步骤


#### ResourceQuota 排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1：查看命名空间配额使用情况
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota <quota-name> -n <namespace>

# 输出示例：
# Name:            compute-quota
# Namespace:       default
# Resource         Used    Hard
# --------         ----    ----
# limits.cpu       4       8
# limits.memory    4Gi     16Gi
# pods             10      20
# requests.cpu     2       4
# requests.memory  2Gi     8Gi

# 步骤 2：检查哪些资源占用了配额
kubectl get pods -n <namespace> -o json | \
  jq -r '.items[] | "\(.metadata.name): CPU=\(.spec.containers[].resources.requests.cpu // "none"), MEM=\(.spec.containers[].resources.requests.memory // "none")"'

# 步骤 3：检查 LimitRange 约束
kubectl get limitrange -n <namespace>
kubectl describe limitrange <lr-name> -n <namespace>

# 步骤 4：计算当前使用量
kubectl get pods -n <namespace> -o json | \
  jq '[.items[].spec.containers[].resources.requests.memory // "0" | gsub("Mi"; "") | gsub("Gi"; "000") | tonumber] | add'
```
---

### 02 Autoscaling Troubleshooting

#### 0. 10 分钟快速诊断

1. **指标可用性**：`kubectl top nodes/pods` 与 `kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes`。
2. **HPA 状态**：`kubectl describe hpa <name>`，查看 `ScalingActive` 与事件。
3. **VPA 推荐**：`kubectl describe vpa <name>`，确认 recommendation 是否生成。
4. **资源请求**：HPA 需 requests，检查目标工作负载 resources 配置。
5. **扩缩策略**：检查 `behavior.scaleUp/scaleDown` 与稳定窗口。
6. **快速缓解**：
   - metrics-server 问题：重启并调整证书/资源。
   - 扩缩振荡：收敛策略或提高稳定窗口。
7. **证据留存**：保存 HPA/VPA 描述、metrics-server 日志与 metrics API 输出。

#### 排查方法与步骤


#### 排查决策树

```
# 🟢 低风险：只读/信息收集，通常无副作用
HPA/VPA 问题
     │
     ├─── HPA 显示 `<unknown>`？
     │         │
     │         ├─ metrics-server 运行？ ──→ 检查 metrics-server Pod
     │         ├─ API 可用？ ──→ kubectl top nodes/pods
     │         └─ 目标有 resources.requests？ ──→ 添加资源请求
     │
     ├─── HPA 不扩容？
     │         │
     │         ├─ 当前指标低于阈值 ──→ 检查指标计算方式
     │         ├─ 已达到 maxReplicas ──→ 调整 max 或优化应用
     │         ├─ 扩容策略限制 ──→ 检查 behavior.scaleUp
     │         └─ ScalingActive=False ──→ 查看具体原因
     │
     ├─── HPA 不缩容？
     │         │
     │         ├─ 稳定窗口内 ──→ 等待稳定窗口过期
     │         ├─ 缩容策略限制 ──→ 检查 behavior.scaleDown
     │         └─ 指标仍高于目标 ──→ 验证指标准确性
     │
     ├─── VPA 不推荐/不更新？
     │         │
     │         ├─ Recommender 运行？ ──→ 检查 vpa-recommender Pod
     │         ├─ updateMode 配置 ──→ 检查是否为 "Off"
     │         ├─ 数据不足 ──→ 等待收集更多数据
     │         └─ Pod 控制器不支持 ──→ 检查 targetRef
     │
     └─── metrics-server 问题？
               │
               ├─ Pod 状态 ──→ kubectl get pods -n kube-system
               ├─ 证书问题 ──→ 检查 --kubelet-insecure-tls
               └─ 资源不足 ──→ 检查 metrics-server 资源使用
```
---

### 03 Cluster Autoscaler Troubleshooting

#### 0. 10 分钟快速诊断

1. **CA 存活**：`kubectl get pods -n kube-system | grep cluster-autoscaler`。
2. **Pending 原因**：`kubectl get pods -A --field-selector=status.phase=Pending`，确认是否资源不足。
3. **节点组状态**：`kubectl get cm -n kube-system cluster-autoscaler-status -o yaml`，查看 scale up/down 记录。
4. **云 API 错误**：查看 CA 日志中的 `authorization`/`quota`/`node group` 错误。
5. **扩缩容策略**：核对 `scale-down-delay-after-add`、`max-node-provision-time`。
6. **快速缓解**：
   - 达到配额：提升云配额或切换节点组。
   - 扩容慢：调整节点组规格或增加预热节点。
7. **证据留存**：保存 CA 日志、Pending Pod 事件、节点组状态。

#### 排查方法与步骤


#### 排查决策树

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
Cluster Autoscaler 问题
        │
        ▼
┌───────────────────────┐
│  问题类型是什么？      │
└───────────────────────┘
        │
        ├── 不扩容 ───────────────────────────────────────────┐
        │                                                      │
        │   ┌─────────────────────────────────────────┐       │
        │   │ 检查 CA Pod 是否运行                    │       │
        │   │ kubectl get pods -n kube-system | grep  │       │
        │   │ cluster-autoscaler                      │       │
        │   └─────────────────────────────────────────┘       │
        │                  │                                   │
        │                  ▼                                   │
        │   ┌─────────────────────────────────────────┐       │
        │   │ CA 运行正常?                            │       │
        │   └─────────────────────────────────────────┘       │
        │          │                │                          │
        │         否               是                          │
        │          │                │                          │
        │          ▼                ▼                          │
        │   ┌────────────┐   ┌────────────────┐               │
        │   │ 检查 CA    │   │ Pending Pod    │               │
        │   │ 部署和配置 │   │ 是否因资源不足 │               │
        │   └────────────┘   │ 无法调度?      │               │
        │                    └────────────────┘               │
        │                           │                  
...(截断)

---

### 04 Pdb Troubleshooting

#### 0. 10 分钟快速诊断

1. **PDB 状态**：`kubectl get pdb -A`，查看 `disruptionsAllowed` 是否为 0。
2. **匹配关系**：`kubectl describe pdb <name>`，确认 selector 是否匹配实际 Pod。
3. **驱逐阻塞**：`kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` 输出中定位被阻塞的 Pod。
4. **健康度**：确认 `currentHealthy/expectedPods` 是否满足 minAvailable/maxUnavailable。
5. **快速缓解**：
   - 临时放宽 PDB（调整 minAvailable/maxUnavailable）。
   - 启用 `unhealthyPodEvictionPolicy: AlwaysAllow`（v1.27+）。
6. **证据留存**：保存 PDB 描述、Pod 就绪状态与 drain 输出。

#### 排查方法与步骤


#### 排查决策树

```
PDB 问题
    │
    ▼
┌───────────────────────┐
│  问题类型是什么？      │
└───────────────────────┘
    │
    ├── drain/驱逐被阻止 ────────────────────────────────────┐
    │                                                         │
    │   ┌─────────────────────────────────────────┐          │
    │   │ 检查 PDB 状态                           │          │
    │   │ kubectl get pdb -A                      │          │
    │   └─────────────────────────────────────────┘          │
    │                  │                                      │
    │                  ▼                                      │
    │   ┌─────────────────────────────────────────┐          │
    │   │ disruptionsAllowed = 0?                 │          │
    │   └─────────────────────────────────────────┘          │
    │          │                │                             │
    │         是               否                             │
    │          │                │                             │
    │          ▼                ▼                             │
    │   ┌────────────┐   ┌────────────────┐                  │
    │   │ 检查 Pod   │   │ 检查其他阻止   │                  │
    │   │ 健康状态   │   │ 驱逐的原因     │                  │
    │   └────────────┘   └────────────────┘                  │
    │                                                         │
    ├── PDB 似乎无效 ────────────────────────────────────────┤
    │                                                         │
    │   ┌────────────────────────────
...(截断)

## 相关链接

- [[技能/可观测性/monitoring/monitor-kubernetes-metrics.md|K8s 监控指标]]
- [[技能/节点/node/诊断排障/troubleshoot-node-issues.md|节点故障排查]]

## Related

- [[csi-fta]] — CSI 存储异常故障树分析
- [[helm-fta]] — Helm 发布异常故障树分析
- [[技能/工作负载/pod/方法论/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[技能/工作负载/pod/reference/ts-command-output.md|ts-command-output]] — 命令输出根因解析
- [[实体/kubelet.md|kubelet]] — kubelet


<!-- risk-assessed -->
