---
title: 技能库索引
description: KUDIG 运维技能库完整索引，按 8 大技术专题分类，标注工单高频与最佳实践高频技能
summary: 技能库总索引，纯技术专题结构（工作负载/网络/控制面/存储/安全/节点/可观测性/集群运维），含高频标记（工单TOP/最佳实践TOP/产品高频）
category: index
tags:
- skills
- index
- troubleshooting
- best-practices
tier: core
created: '2026-07-23'
last_updated: '2026-07-23'
difficulty: intermediate
audience:
- 所有工程师
- SRE
- 技术支持
- 新人
estimated_read_time: 10min
---

# 技能库索引

> 本技能库按 **8 大技术专题** 组织。每个专题下按 **组件** 划分，组件内含内容型子层：
> `概念原理/ 诊断排障/ 工单案例/ 最佳实践/ 运维操作/ 培训/ 方法论/ reference/ 上游源码/`。

> **高频标记说明**
>
> | 标记 | 含义 |
> |:---|:---|
> | 🔴 `工单TOP` | 工单问题处理中最高频遇到 |
> | 🔵 `最佳实践TOP` | 最佳实践/配置中最高频使用 |
> | 🟢 `产品高频` | 产品日常使用中最高频操作 |

---

## 一、工作负载 `工作负载/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [pod/](工作负载/pod/) | CrashLoopBackOff、OOMKilled、Pending、ImagePullBackOff、Evicted；FTA 方法论、培训、测验 | 🔴 工单TOP |
| [deployment/](工作负载/deployment/) | 滚动更新失败、副本异常、选择器冲突、发布策略 | 🟢 产品高频 |
| [statefulset/](工作负载/statefulset/) | 有序部署、持久卷绑定、网络标识、扩缩容 | |
| [daemonset/](工作负载/daemonset/) | 节点调度、污点容忍、滚动更新、资源竞争 | |
| [job-cronjob/](工作负载/job-cronjob/) | 调度失败、并发控制、超时、完成策略 | |
| [hpa-vpa/](工作负载/hpa-vpa/) | HPA 扩缩容异常、VPA 推荐、PDB 中断预算、扩缩容最佳实践 | 🟢 产品高频 |

## 二、网络 `网络/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [dns/](网络/dns/) | CoreDNS 异常、NXDOMAIN、解析超时、缓存污染 | 🔴 工单TOP |
| [service/](网络/service/) | Service 不通、Endpoints 为空、kube-proxy 异常、会话亲和 | 🔴 工单TOP |
| [ingress/](网络/ingress/) | Ingress/Nginx/Higress 路由失败、TLS 终止、后端健康检查 | 🟢 产品高频 |
| [networkpolicy/](网络/networkpolicy/) | 入站/出站阻断、选择器错误、CNI 兼容、网络安全 | |
| [cni/](网络/cni/) | Terway/Calico/Cilium/Flannel IP 分配、路由、网络配置 | 🔴 工单TOP |
| [gateway-api/](网络/gateway-api/) | Gateway API/HTTPRoute/GRPCRoute/TLSRoute | |
| [service-mesh/](网络/service-mesh/) | Istio Sidecar 注入、流量管理、mTLS | |

## 三、控制面 `控制面/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [apiserver/](控制面/apiserver/) | 认证/授权/准入/限流/etcd 连接 | 🔴 工单TOP |
| [scheduler/](控制面/scheduler/) | 调度失败、过滤/打分异常、抢占、扩展点 | |
| [controller-manager/](控制面/controller-manager/) | Leader 选举、控制器同步、限速 | |
| [etcd/](控制面/etcd/) | etcd 集群/Raft/存储/快照/备份恢复、灾备 | 🔴 工单TOP |
| [crd-operator/](控制面/crd-operator/) | CRD 注册、Operator Reconcile、CRD/Operator 开发 | |

## 四、存储 `存储/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [csi-storage/](存储/csi-storage/) | CSI 驱动异常、PV/PVC 挂载失败、存储后端、持久存储管理、存储最佳实践 | 🔴 工单TOP |

## 五、安全 `安全/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [rbac/](安全/rbac/) | RBAC 权限不足、Role/Binding 配置、审计、密钥管理 | 🟢 产品高频 |
| [certificate/](安全/certificate/) | 证书过期、轮换失败、CA 链断裂、kubelet 证书 | 🔴 工单TOP |
| [webhook-admission/](安全/webhook-admission/) | Webhook 超时、TLS 错误、失败策略 | |
| [pod-security/](安全/pod-security/) | PSP/SCC/PSA 策略迁移、安全上下文、Pod 安全指南 | |
| [resource-quota/](安全/resource-quota/) | ResourceQuota/LimitRange 配额超限 | |

## 六、节点 `节点/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [node/](节点/node/) | NotReady、MemoryPressure、DiskPressure、kubelet 异常；节点排水维护、NotReady 完整诊断技能 | 🔴 工单TOP |
| [nodepool/](节点/nodepool/) | 节点池扩缩失败、配置漂移 | |
| [gpu/](节点/gpu/) | GPU 调度失败、设备插件异常、驱动兼容、AI/ML 工作负载 | |

## 七、可观测性 `可观测性/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [monitoring/](可观测性/monitoring/) | 监控指标缺失、告警规则异常、指标采集；监控/日志/链路追踪最佳实践 | 🔵 最佳实践TOP |

## 八、集群运维 `集群运维/`

| 组件 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [cluster-upgrade/](集群运维/cluster-upgrade/) | 集群升级失败、版本兼容、集群配置、最佳实践合集 | 🔵 最佳实践TOP |
| [cluster-autoscaler/](集群运维/cluster-autoscaler/) | 自动扩缩容异常、节点池联动 | |
| [cloud-provider/](集群运维/cloud-provider/) | Cloud Controller 异常、SLB/ECS 联动 | |
| [gitops-argocd/](集群运维/gitops-argocd/) | ArgoCD 同步失败、Application 状态异常、GitOps 流程 | |
| [helm/](集群运维/helm/) | Helm Release 失败、Chart 渲染错误 | 🟢 产品高频 |
| [openkruise/](集群运维/openkruise/) | OpenKruise 高级工作负载异常 | |
| [kubeadm/](集群运维/kubeadm/) | 集群生命周期、HA 搭建、集群清理 | 🔵 最佳实践TOP |
| [migration/](集群运维/migration/) | 集群/工作负载迁移方案与实战 | |
| [cluster-deployment/](集群运维/cluster-deployment/) | 本地/单机/开发/生产环境部署 | |
| [cluster/](集群运维/cluster/) | 集群级 SOP、控制面与自动扩缩容操作 | |

---

## 快速诊断入口

```bash
# 🟢 低风险：只读/信息收集
# Pod 异常
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded
kubectl describe pod <name> -n <ns> | tail -30
kubectl logs <pod> --previous -n <ns> --tail=50

# 节点异常
kubectl get nodes -o wide
kubectl describe node <name>
journalctl -u kubelet --since "10min ago"

# 网络诊断
kubectl exec <pod> -- nslookup kubernetes.default
kubectl get endpoints <svc> -n <ns>

# 存储诊断
kubectl get pv,pvc -A
kubectl describe pvc <name> -n <ns>
```

---

## 学习路径导航

| 阶段 | 内容 | 入口 |
|:---|:---|:---|
| L1 新人 | K8s 基础 15 课 + OnCall 培训（分散在各组件 `培训/`） | [工作负载/pod/培训/](工作负载/pod/培训/) |
| L2 进阶 | 各组件 `诊断排障/` + 工单案例 | [工作负载/pod/诊断排障/](工作负载/pod/诊断排障/) |
| L3 高级 | FTA 方法论与诊断引擎 + 各组件 `最佳实践/` | [工作负载/pod/方法论/](工作负载/pod/方法论/) |
| L4 专家 | Agent 编排 + 技能建设规范 | [工作负载/pod/方法论/agent/](工作负载/pod/方法论/agent/) |

---

## 工单 TOP 10 高频技能速查

| # | 场景 | 对应技能 |
|:---|:---|:---|
| 1 | Pod CrashLoopBackOff / OOMKilled | [工作负载/pod/](工作负载/pod/) |
| 2 | Pod Pending（调度失败） | [工作负载/pod/](工作负载/pod/) + [控制面/scheduler/](控制面/scheduler/) |
| 3 | Node NotReady | [节点/node/](节点/node/)（含 skill-notready/） |
| 4 | DNS 解析失败 | [网络/dns/](网络/dns/) |
| 5 | Service 不通 | [网络/service/](网络/service/) |
| 6 | PVC 挂载失败 | [存储/csi-storage/](存储/csi-storage/) |
| 7 | 证书过期 | [安全/certificate/](安全/certificate/) |
| 8 | CNI/网络插件异常 | [网络/cni/](网络/cni/) |
| 9 | API Server 异常 | [控制面/apiserver/](控制面/apiserver/) |
| 10 | etcd 集群异常 | [控制面/etcd/](控制面/etcd/) |
