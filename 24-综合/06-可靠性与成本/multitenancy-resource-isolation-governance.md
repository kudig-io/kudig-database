---
title: "多租户 × 资源隔离 × 治理交叉"
summary: "Kubernetes 多租户场景下资源隔离（Namespace/vCluster/硬隔离）与治理策略（配额/准入/审计）的交叉设计"
category: synthesis
tags:
- multitenancy
- resource-isolation
- governance
- quota
- admission-control
- rbac
tier: supporting
sources:
- 平台工程/治理/17-multi-tenant-management.md
- 平台工程/治理/18-gpu-cluster-governance-ai-platform.md
- 综合/rbac-multitenancy.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# 多租户 × 资源隔离 × 治理交叉

## The Connection

多租户是 Kubernetes 平台工程的核心挑战：如何在共享集群上为多个团队/项目/客户提供安全隔离、公平资源和可审计的使用体验。资源隔离是多租户的技术基础（"你不能看到/影响别人的资源"），治理是多租户的管理框架（"你只能用分配给你的资源，且使用方式合规"）。两者缺一不可：没有隔离的治理是空中楼阁（无法强制执行），没有治理的隔离是资源浪费（无法公平分配）。

在 Kubernetes 中，多租户隔离有三个层次：
1. **软隔离（Namespace + RBAC + NetworkPolicy）**：逻辑隔离，共享内核和节点，成本最低但隔离最弱
2. **中隔离（vCluster / Kata Containers）**：独立控制面或独立内核，隔离增强但开销增大
3. **硬隔离（独立集群 / 独立节点池）**：物理隔离，最强安全但成本最高

治理策略贯穿所有层次：ResourceQuota 限制资源用量、LimitRange 设置默认值、ValidatingAdmissionPolicy 强制合规、PriorityClass 管理优先级、审计日志追踪操作。GPU 集群的治理尤为复杂，因为 GPU 是整数资源（不可分数分配）、成本极高（需要精细分账）、且存在多种共享模式（MIG/MPS/时间分片）。^[inferred]

## Where They Co-occur

- **Namespace 作为租户边界**：每个团队一个 Namespace（或一组 Namespace），RBAC 限制跨 Namespace 访问，ResourceQuota 限制资源上限，NetworkPolicy 隔离网络流量。这是最基础也最常用的多租户模式。

- **GPU 配额与优先级**：GPU 是稀缺资源，配额管理尤为关键。组织级配额（64 GPU）→ 团队级配额（16 GPU）→ 项目级配额（8 GPU）三层嵌套。PriorityClass 确保推理（P=1000000）可抢占训练（P=500000），训练可抢占实验（P=100000）。

- **准入控制强制合规**：ValidatingAdmissionPolicy 在 API Server 层面拦截不合规请求：GPU Pod 必须设置 limits、必须标注 cost-center、单 Pod 不超过 8 GPU、镜像必须来自内部 Registry。这比"文档要求"有效得多——不合规的请求直接被拒绝。

- **成本归因与 Showback**：OpenCost 按 Namespace/Label 归因 GPU 成本，每周向团队发送使用报告。成本透明是治理的基础——团队看到自己的 GPU 账单后，会主动优化使用效率。

- **vCluster 隔离控制面**：对于需要独立 API Server 的租户（如外部客户），vCluster 在共享节点上运行轻量 K8s 控制面，租户拥有独立的 RBAC、CRD 和 Secret 空间，但共享底层计算资源。

- **Kata Containers 内核隔离**：对于安全要求极高的租户（金融/政务），Kata Containers 为每个 Pod 提供独立内核（轻量 VM），防止容器逃逸影响其他租户。代价是启动时间增加（~1s）和资源开销（~128MB/Pod）。

- **审计与合规**：K8s Audit Log 记录所有 API 操作，配合 Falco/Tetragon 运行时检测，形成"事前准入 + 事中监控 + 事后审计"的完整治理闭环。GPU 操作（MIG 切分、驱动升级）需要额外的变更审批流程。

- **弹性配额与借用**：Volcano/Koordinator 的弹性配额允许团队在空闲时借用其他团队的 GPU 配额，提高整体利用率。但借用有优先级：配额所有者的任务可以隨時召回借出的资源。^[inferred]

## Cross-cutting Insight

多租户治理的本质是**在共享与隔离之间找到平衡点**。完全隔离（每团队独立集群）安全但昂贵——GPU 利用率低（每团队需要预留峰值容量）、运维成本高（N 个集群 × 升级/监控/备份）。完全共享（所有团队一个 Namespace）便宜但危险——一个团队的 bug 可能影响所有人。

最佳实践是**分层隔离 + 统一治理**：
- 计算层：共享节点池 + 优先级调度（软隔离），GPU 节点按团队 Taint（中隔离）
- 网络层：NetworkPolicy 隔离（软隔离），敏感服务用独立 VPC（硬隔离）
- 存储层：Namespace 级 PVC 配额（软隔离），敏感数据用独立存储集群（硬隔离）
- 控制面：共享 API Server（内部团队），vCluster（外部客户/高安全需求）
- 治理层：统一的准入策略、配额体系、成本归因和审计日志

GPU 多租户的特殊性在于：GPU 不可分数分配（整数约束）、成本极高（需要精细分账）、共享模式多样（MIG/MPS/时间分片各有隔离级别）。因此 GPU 治理需要在标准多租户框架上增加：GPU 专属配额、MIG 实例管理、GPU 利用率监控和空闲回收机制。^[inferred]

## Tensions and Trade-offs

| 张力 | 选择 A | 选择 B | 建议 |
|------|--------|--------|------|
| 隔离级别 | 软隔离（Namespace） | 硬隔离（独立集群） | 内部团队软隔离，外部客户硬隔离 |
| 配额刚性 | 硬配额（不可超） | 弹性配额（可借用） | 生产硬配额，开发弹性配额 |
| 准入严格度 | 严格（拒绝不合规） | 宽松（告警不拒绝） | 生产严格，开发宽松 |
| GPU 共享 | 独占（安全） | MIG/时间分片（高效） | 训练独占，推理 MIG |
| 成本模型 | Showback（展示） | Chargeback（收费） | 先 Showback 培养意识，再 Chargeback |
| 审计粒度 | 全量（合规） | 采样（性能） | 生产全量，开发采样 |

## Practical Patterns

```yaml
# 🟢 低风险：多租户治理状态检查
# 1. 各租户配额使用率
kubectl get resourcequota -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,\
GPU-USED:.status.used.nvidia\\.com/gpu,GPU-LIMIT:.status.hard.nvidia\\.com/gpu,\
CPU-USED:.status.used.requests\\.cpu,CPU-LIMIT:.status.hard.requests\\.cpu

# 2. 准入策略状态
kubectl get validatingadmissionpolicies
kubectl get validatingadmissionpolicybindings

# 3. NetworkPolicy 覆盖率
kubectl get networkpolicy -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,PODS:.spec.podSelector.matchLabels

# 4. 优先级类分布
kubectl get pods -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,PRIORITY:.spec.priorityClassName | \
  sort -k3

# 5. 审计日志查询（最近 GPU 相关操作）
kubectl logs -n kube-system -l component=kube-apiserver --tail=1000 | \
  grep -i "nvidia\|gpu" | tail -20

# 6. 成本归因（OpenCost）
curl -s "http://opencost.cost-management:9003/allocation/compute?window=7d&aggregate=namespace" | \
  jq '.data[0] | to_entries | sort_by(-.value.totalCost) | .[:10]'
```

## Related

- [[10-平台工程/03-治理/17-multi-tenant-management|多租户管理]]
- [[10-平台工程/03-治理/18-gpu-cluster-governance-ai-platform|GPU 集群治理]]
- [[24-综合/04-安全与合规/rbac-multitenancy|RBAC × 多租户]]
- [[24-综合/01-AI与机器学习/gpu-scheduling-cost|GPU Scheduling × Cost Optimization]]
- [[24-综合/01-AI与机器学习/training-inference-data-lifecycle|训练 × 推理 × 数据生命周期]]
- [[24-综合/01-AI与机器学习/gpu-operator-device-plugin-ecosystem|GPU Operator × Device Plugin × CDI]]
