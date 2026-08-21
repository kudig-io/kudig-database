---
title: Limit Ranges（限制范围）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Limit Ranges（限制范围） 是什么
- 如何 Limit Ranges（限制范围）
trigger_keywords:
- Limit
- Ranges
- 限制范围
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Limit Ranges（限制范围）

## 概述

LimitRange 是 [[kubernetes|Kubernetes]] 中的一种策略对象，用于约束在命名空间中可为每种适用对象类型（如 Pod 或 PersistentVolumeClaim）指定的资源分配（limits 和 requests）。默认情况下，容器在集群中以无限制的_compute resources_运行，LimitRange 能够防止单个对象垄断命名空间内的所有可用资源。

## 核心概念/原理

- **命名空间级策略**：LimitRange 仅在单个命名空间内生效，当该命名空间中至少存在一个 LimitRange 对象时，Kubernetes 就会对资源分配进行约束。
- **Admission Controller 机制**：LimitRange 通过准入控制器在 Pod 准入阶段工作，而不是在运行时持续监控。
- **约束类型**：
  - 强制每个 Pod 或 Container 的最小和最大计算资源使用量（CPU、内存）。
  - 强制每个 PersistentVolumeClaim 的最小和最大存储请求。
  - 强制资源 request 与 limit 之间的比率。
  - 为命名空间设置默认的 request/limit，并在运行时自动注入到未显式声明资源需求的 Container 中。

## 关键机制或特性

1. **两阶段检查**：
   - **第一阶段**：为所有未设置计算资源需求的 Pod（及其容器）应用默认的 request 和 limit 值。
   - **第二阶段**：跟踪使用量，确保不超过任何 LimitRange 中定义的最小、最大和比率限制。
2. **违反约束**：若创建或更新对象时违反 LimitRange 约束，API 服务器将返回 HTTP `403 Forbidden` 并说明被违反的约束。
3. **仅影响准入阶段**：LimitRange 的验证仅在 Pod 准入阶段发生，对已运行的 Pod 不生效；新增或修改 LimitRange 不会影响已存在的 Pod。
4. **多 LimitRange 的不确定性**：若同一命名空间中存在两个或更多 LimitRange 对象，默认值的生效是不确定的。
5. **默认值一致性风险**：LimitRange 不会检查其应用默认值的一致性。例如，若 LimitRange 设置的默认 limit 小于客户端提交的 request，则最终 Pod 将无法调度（报 `Invalid value` 错误）。

## 使用场景

- **防止资源垄断**：在多用户共享的命名空间中，确保单个 Pod 或 PVC 不会占用过多资源。
- **自动注入默认值**：为开发团队提供“免配置”体验，自动为未声明资源需求的容器注入合理的 CPU/内存默认值。
- **存储范围控制**：限制 PVC 的存储请求在合理范围内，避免用户申请过大或过小的存储卷。

## 最佳实践/注意事项

- 若 LimitRange 适用于 `cpu` 和 `memory`，必须为 Pod 显式指定 requests 或 limits，否则系统可能拒绝 Pod 创建。
- 添加或修改 LimitRange 后，已存在的 Pod 不会受到影响，如有必要需手动重建。
- 尽量避免在同一命名空间中创建多个可能产生冲突默认值的 LimitRange。
- 设置默认值时，务必确保 `limit ≥ request`，否则 Pod 将因资源规格无效而无法调度。
- LimitRange 常与 ResourceQuota 配合使用：LimitRange 负责单个对象的资源范围约束，ResourceQuota 负责命名空间级别的总资源配额。

## 架构深度解析

### LimitRange 准入与默认值注入链路

```
┌──────────────────────────────────────────────────────────────┐
│  Pod 创建/更新请求                                             │
│   │  ① 认证 → 授权 → 基本校验                                   │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ API Server 准入链                                        │  │
│  │ └─ LimitRanger（limitranger admission）                  │  │
│  │    ├─ 阶段 1：默认值注入                                  │  │
│  │    │  ├─ 容器未声明 resources → 写默认 request/limit      │  │
│  │    │  └─ 仅声明 request → 补 limit（或反之）              │  │
│  │    ├─ 阶段 2：范围校验                                    │  │
│  │    │  ├─ min/max：request 与 limit 必须在 [min, max]      │  │
│  │    │  ├─ maxLimitRequestRatio：limit/request ≤ 比例       │  │
│  │    │  └─ 存储：PVC 容量在 min/max 之间                    │  │
│  │    └─ 违反 → HTTP 403 Forbidden + 违反原因                │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ② 通过 → 进入 ResourceQuota 校验 → etcd 持久化           │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 注意：LimitRange 变更不影响已运行 Pod                     │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| LimitRange 准入 | `plugin/pkg/admission/limitranger/` | 默认值注入与范围校验 |
| 校验逻辑 | `plugin/pkg/admission/limitranger/admission.go` | min/max/ratio 计算 |
| 存储限制 | `pkg/apis/core/validation/` | PVC 与容器资源校验 |
| 计算工具 | `pkg/api/v1/resource/helpers.go` | 资源量比较/合并 |

### 流程步骤

1. LimitRanger 对未声明资源的容器注入默认值（多个 LimitRange 时默认值不确定）。
2. 校验每个容器的 request/limit 满足 `min ≤ request ≤ limit ≤ max`。
3. 校验 `maxLimitRequestRatio`：`limit / request` 不得超过设定比例。
4. 对 PVC 校验 storage 容量范围（min/max）。
5. 校验通过后写入 etcd；已运行 Pod 不受后续 LimitRange 变更影响。

## 生产案例

### 案例 1：默认 limit 小于 request 导致新 Pod 全部无法调度

| 时间 | 事件 |
| --- | --- |
| T+0 | 团队为 namespace 添加 LimitRange，默认 limit 设为 512Mi |
| T+10min | 发布系统创建新 Deployment 时容器 request 为 1Gi |
| T+30min | 新 Pod 全部 Pending，报 `Invalid value: 1073741824: must be less than or equal to memory limit` |
| T+2h | 定位：LimitRange 只注入 limit（512Mi）但保留请求的 request（1Gi），违反 `limit ≥ request` |
| T+4h | 修正 LimitRange 默认 limit 为 2Gi，滚动发布恢复 |

- **根因分析**：LimitRange 不会校验注入默认值的一致性；当客户端显式声明 request 大于默认 limit 时，注入的 limit 与请求的 request 矛盾，Pod 在准入后无法调度。
- **修复命令**：
```bash
# 1. 查看命名空间 LimitRange（只读）
kubectl get limitrange -n app -o yaml
# 2. 修正默认 limit 并应用（🟡 中风险：影响后续创建）
kubectl apply -f - <<'EOF'
apiVersion: v1
kind: LimitRange
metadata:
  name: mem-lr
  namespace: app
spec:
  limits:
  - type: Container
    defaultRequest: { memory: 512Mi }
    default: { memory: 2Gi }
    max: { memory: 4Gi }
EOF
# 3. 验证新 Pod 可创建
kubectl run test --image=nginx --requests=memory=1Gi --limits=memory=1.5Gi -n app  # 🟢 验证后删除
```

### 案例 2：多个 LimitRange 默认值冲突导致行为不确定

| 时间 | 事件 |
| --- | --- |
| T+0 | 基础设施团队与业务团队各自创建了 LimitRange（默认值不同） |
| T+1d | 部分新 Pod 无资源限制、部分被注入 1Gi 限制，行为不一致 |
| T+2d | 定位：命名空间存在两个 LimitRange，默认值注入顺序不确定 |
| T+3d | 合并为单一 LimitRange，冲突消除 |

- **根因分析**：Kubernetes 明确规定同一命名空间多个 LimitRange 时默认值生效不确定（未定义顺序），生产中应保证单一来源。
- **修复命令**：
```bash
# 1. 列出全部 LimitRange（只读）
kubectl get limitrange -A
# 2. 删除冗余并合并（🟡 中风险）
kubectl delete limitrange team-lr -n app
kubectl apply -f merged-lr.yaml
```

## 对比评测

| 维度 | LimitRange | ResourceQuota | VPA | Karpenter 约束 |
| --- | --- | --- | --- | --- |
| 作用对象 | 单个 Pod/PVC | 命名空间总量 | 单工作负载 | 节点池 |
| 强制时机 | 准入时 | 准入时 | 运行期建议/自动 | 调度时 |
| 默认值注入 | 支持 | 不支持 | 支持（推荐值） | 不支持 |
| 典型场景 | 单对象资源边界 | 总量控制/公平性 | 自动伸缩 | 容量上限 |

**选型建议**：LimitRange 管"单个对象不超限"，ResourceQuota 管"命名空间总量"，两者组合是生产标配；自动调优叠加 VPA。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 403 且提示违反 min/max | request/limit 越界 | `kubectl get limitrange -n <ns> -o yaml` 对照修正 |
| Pod 无法调度（Invalid value） | 默认 limit < 请求 request | 调整 LimitRange 默认值或客户端 request |
| 默认值未生效 | 存在多个 LimitRange | 删除冗余 LimitRange 保留单一来源 |
| PVC 创建失败 | storage 容量越界 | 检查 LimitRange 的 `type: PersistentVolumeClaim` |
| 已运行 Pod 无限制 | LimitRange 变更不回溯 | 手动重建 Pod：`kubectl rollout restart deploy` |

## 生产部署清单

- [ ] 每个命名空间收敛为单一 LimitRange（避免默认值不确定性）
- [ ] 同时声明 default 与 defaultRequest，保证 `limit ≥ request` 恒成立
- [ ] CPU 与内存分别设置 maxLimitRequestRatio（如 10:1）
- [ ] PVC 设置 storage min/max 防止容量滥用
- [ ] 与 ResourceQuota 组合发布，先 Quota 后 LimitRange 观察

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | LimitRange 默认 limit < 业务常见 request | 立即修正默认值，阻断异常 Pod 创建 |
| P1 | 命名空间存在多个冲突 LimitRange | 合并为单一来源并验证注入行为 |
| P2 | 缺少 maxLimitRequestRatio 防突发 | 按业务画像设置合理比例 |

## 面试要点

1. **Q：LimitRange 与 ResourceQuota 的区别？**
   A：LimitRange 约束单个对象（容器/Pod/PVC）的资源上下限并可注入默认值；ResourceQuota 约束命名空间内全部对象的总量。两者都在准入阶段生效，配合使用实现"单对象有界、总量有控"。
2. **Q：为什么 LimitRange 默认值可能导致 Pod 无法调度？**
   A：LimitRange 注入默认 limit 时不校验与客户端请求 request 的一致性。若默认 limit < 显式 request，Pod 的 limit/request 规格自相矛盾，准入虽通过但调度器拒绝（Invalid value）。因此配置必须保证 default ≥ 常见 request。
3. **Q：修改 LimitRange 后存量 Pod 会变化吗？**
   A：不会。LimitRange 仅在准入阶段对创建/更新操作生效，已运行 Pod 的 spec 不回溯。若需让存量 Pod 遵循新边界，必须手动重建（如 `rollout restart`）。

## 运维要点

- 单一来源：每个命名空间只保留一个 LimitRange，杜绝默认值不确定性。
- 变更感知：修改 LimitRange 后观察 24h 内新 Pod 创建失败率。
- 监控：跟踪 Pod 创建 403（LimitRange 拒绝）事件，识别业务资源画像偏差。
- 容量规划：LimitRange 上限与节点池容量联动设计，避免"范围内仍无节点可调度"。
- 排障入口：`kubectl describe pod` 的 Events 会显示准入拒绝原因，直接对照 LimitRange 规则。

## 参考链接

- [Kubernetes 官方文档 - Limit Ranges](https://kubernetes.io/docs/concepts/policy/limit-range/)

## Related

- [[17-系统基础/06-知识字典/security/admission-controller.md|准入控制器]]
- [[17-系统基础/06-知识字典/security/application-security-checklist.md|应用安全清单]]
- [[17-系统基础/06-知识字典/security/athenz.md|Athenz 身份认证与授权]]


<!-- risk-assessed -->
