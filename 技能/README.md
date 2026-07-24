---
title: Skills & Training
description: 技能知识域 — 按 8 大 Kubernetes 技术专题组织的诊断、最佳实践、运维与培训体系
summary: 技能知识域入口，涵盖工作负载/网络/控制面/存储/安全/节点/可观测性/集群运维 8 大技术专题，各专题按组件粒度组织内容
category: domain
tags:
- skills
- training
- troubleshooting
- best-practices
tier: core
created: '2026-05-23'
last_updated: '2026-07-23'
difficulty: intermediate
audience:
- 所有工程师
- SRE
- 新人
estimated_read_time: 15min
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 技能 Skills & Training

> 完整索引（含高频标记）请参阅 **[index.md](index.md)**

技能库按 **8 大 Kubernetes 技术专题** 组织，顶层只保留 8 个技术专题目录，无横向聚合文件夹。每个专题下按 **组件** 粒度组织；每个组件内部按内容型子层归档（`概念原理/` `诊断排障/` `工单案例/` `培训/` `最佳实践/` `运维操作/` `reference/` `上游源码/` `方法论/`）。

---

## 目录结构

```
技能/
├── index.md                     # 完整索引（含工单TOP/最佳实践TOP/产品高频标记）
├── README.md                    # 本文件
│
├── 工作负载/                     # pod deployment statefulset daemonset job-cronjob hpa-vpa
├── 网络/                         # dns service ingress cni networkpolicy gateway-api service-mesh
├── 控制面/                       # apiserver scheduler controller-manager etcd crd-operator
├── 存储/                         # csi-storage
├── 安全/                         # rbac certificate webhook-admission pod-security resource-quota
├── 节点/                         # node nodepool gpu
├── 可观测性/                     # monitoring
└── 集群运维/                     # cluster-upgrade cluster-autoscaler cloud-provider gitops-argocd
                                  #   helm openkruise kubeadm migration cluster-deployment cluster
```

---

## 按专题分类索引

### 1. 工作负载 `工作负载/`

| 组件 | 覆盖范围 |
|:---|:---|
| [工作负载/pod/](工作负载/pod/) | Pod 全生命周期异常、通用排障框架、FTA 方法论、培训与测验锚点 |
| [工作负载/deployment/](工作负载/deployment/) | Deployment 滚动更新/副本/选择器/发布策略 |
| [工作负载/statefulset/](工作负载/statefulset/) | StatefulSet 有序部署/持久卷/网络标识 |
| [工作负载/daemonset/](工作负载/daemonset/) | DaemonSet 节点调度/污点/滚动更新 |
| [工作负载/job-cronjob/](工作负载/job-cronjob/) | Job/CronJob 调度/并发/超时 |
| [工作负载/hpa-vpa/](工作负载/hpa-vpa/) | HPA/VPA/PDB 弹性伸缩、扩缩容最佳实践 |

### 2. 网络 `网络/`

| 组件 | 覆盖范围 |
|:---|:---|
| [网络/dns/](网络/dns/) | CoreDNS/集群DNS/外部解析/缓存 |
| [网络/service/](网络/service/) | Service/Endpoints/kube-proxy/负载均衡 |
| [网络/ingress/](网络/ingress/) | Ingress/Nginx/Higress/TLS/路由 |
| [网络/cni/](网络/cni/) | Terway/Calico/Cilium/Flannel、网络配置最佳实践 |
| [网络/networkpolicy/](网络/networkpolicy/) | NetworkPolicy 隔离策略、网络安全 |
| [网络/gateway-api/](网络/gateway-api/) | Gateway API/HTTPRoute/GRPCRoute |
| [网络/service-mesh/](网络/service-mesh/) | Istio Sidecar/流量管理/mTLS |

### 3. 控制面 `控制面/`

| 组件 | 覆盖范围 |
|:---|:---|
| [控制面/apiserver/](控制面/apiserver/) | 认证/授权/准入/etcd连接/限流 |
| [控制面/scheduler/](控制面/scheduler/) | 过滤/打分/抢占/亲和性/扩展点/资源调度 |
| [控制面/controller-manager/](控制面/controller-manager/) | Leader选举/控制器/同步/限速 |
| [控制面/etcd/](控制面/etcd/) | Raft/存储/快照/备份恢复/灾备 |
| [控制面/crd-operator/](控制面/crd-operator/) | CRD 注册/Operator Reconcile/CRD 开发 |

### 4. 存储 `存储/`

| 组件 | 覆盖范围 |
|:---|:---|
| [存储/csi-storage/](存储/csi-storage/) | CSI/PV/PVC/挂载/后端/持久存储管理/存储最佳实践 |

### 5. 安全 `安全/`

| 组件 | 覆盖范围 |
|:---|:---|
| [安全/rbac/](安全/rbac/) | RBAC 权限/审计/密钥管理 |
| [安全/certificate/](安全/certificate/) | 证书签发/轮换/过期/CA 链 |
| [安全/webhook-admission/](安全/webhook-admission/) | Webhook 超时/TLS/失败策略 |
| [安全/pod-security/](安全/pod-security/) | PSP/SCC/PSA 策略、安全上下文、Pod 安全指南 |
| [安全/resource-quota/](安全/resource-quota/) | ResourceQuota/LimitRange 配额 |

### 6. 节点 `节点/`

| 组件 | 覆盖范围 |
|:---|:---|
| [节点/node/](节点/node/) | NotReady/资源压力/kubelet、节点维护、NotReady 完整诊断技能 |
| [节点/nodepool/](节点/nodepool/) | 节点池扩缩/配置漂移 |
| [节点/gpu/](节点/gpu/) | GPU 调度/设备插件/驱动兼容/AI-ML 工作负载 |

### 7. 可观测性 `可观测性/`

| 组件 | 覆盖范围 |
|:---|:---|
| [可观测性/monitoring/](可观测性/monitoring/) | 监控指标/告警规则/采集、监控/日志/链路追踪最佳实践 |

### 8. 集群运维 `集群运维/`

| 组件 | 覆盖范围 |
|:---|:---|
| [集群运维/cluster-upgrade/](集群运维/cluster-upgrade/) | 集群升级/版本兼容/集群配置/最佳实践合集 |
| [集群运维/cluster-autoscaler/](集群运维/cluster-autoscaler/) | 自动扩缩容/节点池联动 |
| [集群运维/cloud-provider/](集群运维/cloud-provider/) | Cloud Controller/SLB/ECS 联动 |
| [集群运维/gitops-argocd/](集群运维/gitops-argocd/) | ArgoCD 同步/Application 状态/GitOps 流程 |
| [集群运维/helm/](集群运维/helm/) | Helm Release/Chart 渲染 |
| [集群运维/openkruise/](集群运维/openkruise/) | OpenKruise 高级工作负载 |
| [集群运维/kubeadm/](集群运维/kubeadm/) | 集群生命周期/HA 搭建/集群清理 |
| [集群运维/migration/](集群运维/migration/) | 集群/工作负载迁移方案与实战 |
| [集群运维/cluster-deployment/](集群运维/cluster-deployment/) | 本地/单机/开发/生产环境部署 |
| [集群运维/cluster/](集群运维/cluster/) | 集群级 SOP、控制面与自动扩缩容操作 |

---

## 技能域使用指南

### 学习路径

| 阶段 | 内容 | 前置要求 |
|------|------|----------|
| L1 基础 | [工作负载/pod/培训/](工作负载/pod/培训/) — K8s 入门 15 课 + OnCall 培训 | Linux 基础 |
| L2 进阶 | 各组件 [诊断排障/](工作负载/pod/诊断排障/) — 命令输出解读式排障 | L1 + K8s 架构 |
| L3 高级 | [工作负载/pod/方法论/](工作负载/pod/方法论/) + 各组件 `最佳实践/` | L2 + 生产经验 |
| L4 专家 | [工作负载/pod/方法论/agent/](工作负载/pod/方法论/agent/) + 架构设计 | L3 + 多领域经验 |

### 内容型子层说明

各组件按需包含以下内容型子层：

| 子层 | 用途 |
|------|------|
| `概念原理/` | 组件原理、架构、核心概念 |
| `诊断排障/` | 命令输出解读式排障（原 ts-* 系列） |
| `工单案例/` | 真实工单复盘 |
| `培训/` | 培训课程、讲师材料、测验 |
| `最佳实践/` | 配置与操作指南 |
| `运维操作/` | 可执行运维操作 |
| `reference/` | 参考资料、命令速查、版本矩阵 |
| `方法论/` | FTA 诊断框架、Agent 编排、技能建设规范（集中于 工作负载/pod/方法论/） |

### 常用命令速查

```bash
# Pod 故障排查（🟢 只读）
kubectl get pods -A --field-selector=status.phase!=Running
kubectl describe pod <name> -n <ns>
kubectl logs <pod> --previous -n <ns>

# 节点故障排查（🟢 只读）
kubectl get nodes -o wide
kubectl describe node <name>
journalctl -u kubelet --since "10min ago"

# 网络诊断（🟢 只读）
kubectl exec <pod> -- nslookup kubernetes.default
kubectl get endpoints <svc> -n <ns>
```

## 相关链接

- [[故障诊断/README.md|Domain-12 故障排查]]
- [[故障诊断/FTA故障树/README.md|topic-fta: 故障树分析方法论]]
- [[故障诊断/FEBM方法论/README.md|topic-febm: FEBM 循证方法论]]
- [[生产运维/README.md|Domain 11: 生产环境运维最佳实践]]
- [[AI基础设施/README.md|AI Agent 工程专题]]

<!-- risk-assessed -->
