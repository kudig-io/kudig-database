---
title: 技能库索引
description: KUDIG 运维技能库完整索引，按故障域/功能域分类，标注工单高频与最佳实践高频技能
summary: 技能库总索引，含高频标记（工单TOP/最佳实践TOP/产品高频），覆盖故障诊断、排障实战、运维操作、最佳实践、培训评估全域
category: index
tags:
- skills
- index
- fta
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

> **高频标记说明**
>
> | 标记 | 含义 |
> |:---|:---|
> | 🔴 `工单TOP` | 工单问题处理中最高频遇到 |
> | 🔵 `最佳实践TOP` | 最佳实践/配置中最高频使用 |
> | 🟢 `产品高频` | 产品日常使用中最高频操作 |

---

## 一、故障诊断（FTA 故障树 + 诊断技能）

### 1.1 工作负载故障诊断 `故障诊断-工作负载/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [pod/](故障诊断-工作负载/pod/) | CrashLoopBackOff、OOMKilled、Pending、ImagePullBackOff、Evicted | 🔴 工单TOP |
| [deployment/](故障诊断-工作负载/deployment/) | Deployment 滚动更新失败、副本异常、选择器冲突、发布策略 | 🟢 产品高频 |
| [statefulset/](故障诊断-工作负载/statefulset/) | StatefulSet 有序部署、持久卷绑定、网络标识、扩缩容 | |
| [daemonset/](故障诊断-工作负载/daemonset/) | DaemonSet 节点调度、污点容忍、滚动更新、资源竞争 | |
| [job-cronjob/](故障诊断-工作负载/job-cronjob/) | Job/CronJob 调度失败、并发控制、超时、完成策略 | |
| [hpa-vpa/](故障诊断-工作负载/hpa-vpa/) | HPA 扩缩容异常、VPA 推荐、PDB 中断预算 | 🟢 产品高频 |

### 1.2 网络故障诊断 `故障诊断-网络/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [dns/](故障诊断-网络/dns/) | CoreDNS 异常、NXDOMAIN、解析超时、缓存污染 | 🔴 工单TOP |
| [service/](故障诊断-网络/service/) | Service 不通、Endpoints 为空、kube-proxy 异常、会话亲和 | 🔴 工单TOP |
| [ingress/](故障诊断-网络/ingress/) | Ingress/Nginx/Higress 路由失败、TLS 终止、后端健康检查 | 🟢 产品高频 |
| [networkpolicy/](故障诊断-网络/networkpolicy/) | NetworkPolicy 入站/出站阻断、选择器错误、CNI 兼容 | |
| [cni/](故障诊断-网络/cni/) | Terway/Calico/Cilium/Flannel IP 分配、路由、安全组 | 🔴 工单TOP |
| [gateway-api/](故障诊断-网络/gateway-api/) | Gateway API/HTTPRoute/GRPCRoute/TLSRoute | |
| [service-mesh/](故障诊断-网络/service-mesh/) | Istio Sidecar 注入、流量管理、mTLS | |

### 1.3 控制面故障诊断 `故障诊断-控制面/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [apiserver/](故障诊断-控制面/apiserver/) | API Server 认证/授权/准入/限流/etcd 连接 | 🔴 工单TOP |
| [scheduler/](故障诊断-控制面/scheduler/) | 调度失败、过滤/打分异常、抢占、扩展点 | |
| [controller-manager/](故障诊断-控制面/controller-manager/) | Leader 选举、控制器同步、限速 | |
| [etcd/](故障诊断-控制面/etcd/) | etcd 集群/Raft/存储/快照/备份恢复 | 🔴 工单TOP |
| [crd-operator/](故障诊断-控制面/crd-operator/) | CRD 注册、Operator Reconcile 异常 | |

### 1.4 存储故障诊断 `故障诊断-存储/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [csi-storage/](故障诊断-存储/csi-storage/) | CSI 驱动异常、PV/PVC 挂载失败、存储后端、持久存储管理 | 🔴 工单TOP |

### 1.5 安全故障诊断 `故障诊断-安全/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [rbac/](故障诊断-安全/rbac/) | RBAC 权限不足、Role/Binding 配置、审计 | 🟢 产品高频 |
| [certificate/](故障诊断-安全/certificate/) | 证书过期、轮换失败、CA 链断裂、kubelet 证书 | 🔴 工单TOP |
| [webhook-admission/](故障诊断-安全/webhook-admission/) | Webhook 超时、TLS 错误、失败策略 | |
| [pod-security/](故障诊断-安全/pod-security/) | PSP/SCC/PSA 策略迁移、安全上下文 | |
| [resource-quota/](故障诊断-安全/resource-quota/) | ResourceQuota/LimitRange 配额超限 | |

### 1.6 节点故障诊断 `故障诊断-节点/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [node/](故障诊断-节点/node/) | NotReady、MemoryPressure、DiskPressure、kubelet 异常 | 🔴 工单TOP |
| [nodepool/](故障诊断-节点/nodepool/) | 节点池扩缩失败、配置漂移 | |
| [gpu/](故障诊断-节点/gpu/) | GPU 调度失败、设备插件异常、驱动兼容 | |

### 1.7 可观测性故障诊断 `故障诊断-可观测性/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [monitoring/](故障诊断-可观测性/monitoring/) | 监控指标缺失、告警规则异常、指标采集 | 🔵 最佳实践TOP |

### 1.8 集群运维故障诊断 `故障诊断-集群运维/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [cluster-upgrade/](故障诊断-集群运维/cluster-upgrade/) | 集群升级失败、版本兼容 | |
| [cluster-autoscaler/](故障诊断-集群运维/cluster-autoscaler/) | 自动扩缩容异常、节点池联动 | |
| [cloud-provider/](故障诊断-集群运维/cloud-provider/) | Cloud Controller 异常、SLB/ECS 联动 | |
| [gitops-argocd/](故障诊断-集群运维/gitops-argocd/) | ArgoCD 同步失败、Application 状态异常 | |
| [helm/](故障诊断-集群运维/helm/) | Helm Release 失败、Chart 渲染错误 | 🟢 产品高频 |
| [openkruise/](故障诊断-集群运维/openkruise/) | OpenKruise 高级工作负载异常 | |

---

## 二、排障实战（命令输出解读式）`排障实战/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [workloads/](排障实战/workloads/) | Pod/Deployment/StatefulSet 排障实战 | 🔴 工单TOP |
| [networking/](排障实战/networking/) | 网络连通性排障实战 | 🔴 工单TOP |
| [storage/](排障实战/storage/) | 存储挂载排障实战 | |
| [control-plane/](排障实战/control-plane/) | 控制面组件排障实战 | |
| [node-components/](排障实战/node-components/) | 节点组件排障实战 | 🔴 工单TOP |
| [security-auth/](排障实战/security-auth/) | 安全认证排障实战 | |
| [monitoring-observability/](排障实战/monitoring-observability/) | 监控可观测性排障实战 | |
| [ai-ml-workloads/](排障实战/ai-ml-workloads/) | AI/ML 工作负载排障 | |
| [cloud-provider/](排障实战/cloud-provider/) | 云厂商集成排障 | |
| [cluster-operations/](排障实战/cluster-operations/) | 集群运维操作排障 | |
| [gitops-devops/](排障实战/gitops-devops/) | GitOps/DevOps 流程排障 | |
| [resources-scheduling/](排障实战/resources-scheduling/) | 资源调度排障 | |
| [command-output/](排障实战/command-output/) | kubectl 命令输出解读 | 🟢 产品高频 |

---

## 三、运维操作 `运维操作/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [health-probes/](运维操作/health-probes/) | 健康探针配置（liveness/readiness/startup） | 🟢 产品高频 |
| [node-maintenance/](运维操作/node-maintenance/) | 节点排水、维护、驱逐机制 | 🔴 工单TOP |
| [deployment-operations/](运维操作/deployment-operations/) | 滚动更新、金丝雀、蓝绿发布操作 | 🟢 产品高频 |
| [crd-operator-dev/](运维操作/crd-operator-dev/) | CRD/Operator 开发 | |
| [kubeadm/](运维操作/kubeadm/) | 集群生命周期、HA 搭建、集群清理 | 🔵 最佳实践TOP |

---

## 四、FTA 方法论与诊断引擎 `fta-方法论/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [methodology/](fta-方法论/methodology/) | FTA 方法论与核心原则 | |
| [execution-engine/](fta-方法论/execution-engine/) | FTA 诊断执行引擎 | |
| [runbook-automation/](fta-方法论/runbook-automation/) | FTA 驱动的 Runbook 自动化 | |
| [top-events-index/](fta-方法论/top-events-index/) | Kubernetes FTA 顶层事件索引 | |
| [symptom-matching/](fta-方法论/symptom-matching/) | 症状向量匹配引擎 | |
| [diagnostic-overview/](fta-方法论/diagnostic-overview/) | Kubernetes 诊断技能总览 | |

---

## 五、Skill 参考资料 `skill-参考资料/`

| 技能文件夹 | 覆盖场景 |
|:---|:---|
| [diagnostic-workflow/](skill-参考资料/diagnostic-workflow/) | 标准诊断工作流 |
| [remediation-playbook/](skill-参考资料/remediation-playbook/) | 修复操作手册 |
| [root-cause-catalog/](skill-参考资料/root-cause-catalog/) | 根因分类目录 |
| [version-matrix/](skill-参考资料/version-matrix/) | K8s 版本兼容矩阵 |

---

## 六、最佳实践指南 `最佳实践指南/`

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [monitoring/](最佳实践指南/monitoring/) | 监控体系配置指南 | 🔵 最佳实践TOP |
| [networking/](最佳实践指南/networking/) | 网络配置指南 | 🔵 最佳实践TOP |
| [network-security/](最佳实践指南/network-security/) | 网络安全指南 | |
| [pod-security/](最佳实践指南/pod-security/) | Pod 安全指南 | |
| [storage/](最佳实践指南/storage/) | 存储配置指南 | 🔵 最佳实践TOP |
| [scaling/](最佳实践指南/scaling/) | 扩缩容指南 | 🔵 最佳实践TOP |
| [disaster-recovery/](最佳实践指南/disaster-recovery/) | 灾备恢复指南 | 🔵 最佳实践TOP |
| [deployment-strategies/](最佳实践指南/deployment-strategies/) | 发布策略指南 | 🟢 产品高频 |
| [distributed-tracing/](最佳实践指南/distributed-tracing/) | 分布式链路追踪指南 | |
| [logging/](最佳实践指南/logging/) | 日志管理指南 | 🔵 最佳实践TOP |
| [cluster-configuration/](最佳实践指南/cluster-configuration/) | 集群配置指南 | 🔵 最佳实践TOP |

---

## 七、培训学习 `培训学习/`

| 技能文件夹 | 覆盖场景 |
|:---|:---|
| [learning-path/](培训学习/learning-path/) | K8s 基础 15 课学习路径（Pod→Deployment→Service→Ingress→存储→HPA→调度） |
| [oncall-training/](培训学习/oncall-training/) | 新人 OnCall 培训（Day-1 清单、首单指南、交接流程） |
| [training-lecturer/](培训学习/training-lecturer/) | 内部培训讲师体系 |
| [training-public/](培训学习/training-public/) | 公开培训与学习路线图 |
| [tools/](培训学习/tools/) | 教学工具（类比词典、决策树、讲师人设） |

---

## 八、能力评估 `能力评估/`

| 技能文件夹 | 覆盖场景 |
|:---|:---|
| [daily-check-quiz/](能力评估/daily-check-quiz/) | 日常巡检知识测验 |
| [k8s-fundamentals-quiz/](能力评估/k8s-fundamentals-quiz/) | K8s 基础知识测验（含答案） |
| [troubleshooting-lab-exam/](能力评估/troubleshooting-lab-exam/) | 排障实操考核 |

---

## 九、Agent 编排 `agent-编排/`

| 技能文件夹 | 覆盖场景 |
|:---|:---|
| [orchestration-patterns/](agent-编排/orchestration-patterns/) | Agent 编排模式 |
| [specs-collection/](agent-编排/specs-collection/) | KUDIG Agent 规格集合 |
| [prompts-catalog/](agent-编排/prompts-catalog/) | 提示词目录 |

---

## 十、专项技能

| 技能文件夹 | 覆盖场景 | 高频标记 |
|:---|:---|:---|
| [skill-k8s-node-notready/](skill-k8s-node-notready/) | Node NotReady 完整诊断技能（SKILL + 使用指南 + 升级模板） | 🔴 工单TOP |
| [best-practices/](best-practices/) | 生产运维最佳实践合集（部署/迁移/场景/安全/可观测） | 🔵 最佳实践TOP |
| [技能建设最佳实践/](技能建设最佳实践/) | 技能文件编写规范与质量指南 | |

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
| L1 新人 | K8s 基础 15 课 + OnCall 培训 | [培训学习/learning-path/](培训学习/learning-path/) |
| L2 进阶 | 排障实战 + FTA 故障树 | [排障实战/](排障实战/) |
| L3 高级 | FTA 方法论 + 最佳实践指南 | [fta-方法论/](fta-方法论/) |
| L4 专家 | Agent 编排 + 技能建设 | [agent-编排/](agent-编排/) |

---

## 工单 TOP 10 高频技能速查

| # | 场景 | 对应技能 |
|:---|:---|:---|
| 1 | Pod CrashLoopBackOff / OOMKilled | [故障诊断-工作负载/pod/](故障诊断-工作负载/pod/) |
| 2 | Pod Pending（调度失败） | [故障诊断-工作负载/pod/](故障诊断-工作负载/pod/) |
| 3 | Node NotReady | [skill-k8s-node-notready/](skill-k8s-node-notready/) + [故障诊断-节点/node/](故障诊断-节点/node/) |
| 4 | DNS 解析失败 | [故障诊断-网络/dns/](故障诊断-网络/dns/) |
| 5 | Service 不通 | [故障诊断-网络/service/](故障诊断-网络/service/) |
| 6 | PVC 挂载失败 | [故障诊断-存储/csi-storage/](故障诊断-存储/csi-storage/) |
| 7 | 证书过期 | [故障诊断-安全/certificate/](故障诊断-安全/certificate/) |
| 8 | CNI/网络插件异常 | [故障诊断-网络/cni/](故障诊断-网络/cni/) |
| 9 | API Server 异常 | [故障诊断-控制面/apiserver/](故障诊断-控制面/apiserver/) |
| 10 | etcd 集群异常 | [故障诊断-控制面/etcd/](故障诊断-控制面/etcd/) |
