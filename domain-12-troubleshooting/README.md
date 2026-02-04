# Domain-12 故障排查 (Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **文档数量**: 38篇 | **最后更新**: 2026-02 | **质量等级**: 专家级

---

## 文档目录

### 控制平面故障排查
| 编号 | 文档名称 | 关键内容 |
|:---:|:---|:---|
| 01 | [API Server故障排查](./01-control-plane-apiserver-troubleshooting.md) | API Server不可用、性能问题、认证授权故障 |
| 02 | [etcd故障排查](./02-control-plane-etcd-troubleshooting.md) | etcd集群不可用、数据一致性、性能优化 |
| 03 | [CNI网络故障排查](./03-networking-cni-troubleshooting.md) | Pod网络不通、DNS解析失败、跨节点通信 |
| 04 | [CSI存储故障排查](./04-storage-csi-troubleshooting.md) | 卷创建/挂载失败、存储性能问题、CSI组件故障 |

### 工作负载故障排查
| 编号 | 文档名称 | 关键内容 |
|:---:|:---|:---|
| 05 | [Pod Pending诊断](./05-pod-pending-diagnosis.md) | Pod调度失败深度诊断 |
| 06 | [Node NotReady诊断](./06-node-notready-diagnosis.md) | 节点异常深度诊断 |
| 07 | [OOM内存诊断](./07-oom-memory-diagnosis.md) | 内存溢出、驱逐问题排查 |
| 08 | [Pod综合故障排查](./08-pod-comprehensive-troubleshooting.md) | Pod全状态故障排查 |
| 09 | [Node综合故障排查](./09-node-comprehensive-troubleshooting.md) | Node全方位故障诊断 |
| 10 | [Service综合故障排查](./10-service-comprehensive-troubleshooting.md) | Service访问失败、Endpoints问题 |
| 11 | [Deployment故障排查](./11-deployment-comprehensive-troubleshooting.md) | Deployment滚动更新、回滚问题 |
| 12 | [RBAC/Quota故障排查](./12-rbac-quota-troubleshooting.md) | 权限不足、配额限制问题 |
| 13 | [证书故障排查](./13-certificate-troubleshooting.md) | 证书过期、轮换问题 |
| 14 | [PVC存储故障排查](./14-pvc-storage-troubleshooting.md) | PVC绑定、存储类问题 |

### 高级故障排查
| 编号 | 文档名称 | 关键内容 |
|:---:|:---|:---|
| 15 | [Ingress故障排查](./15-ingress-troubleshooting.md) | Ingress控制器、路由规则、TLS证书 |
| 16 | [NetworkPolicy故障排查](./16-networkpolicy-troubleshooting.md) | 网络策略、安全组、微隔离 |
| 17 | [HPA/VPA故障排查](./17-hpa-vpa-troubleshooting.md) | 自动扩缩容配置、指标监控 |
| 18 | [CronJob故障排查](./18-cronjob-troubleshooting.md) | 定时任务、并发控制、资源清理 |
| 19 | [ConfigMap/Secret故障排查](./19-configmap-secret-troubleshooting.md) | 配置注入、热更新、安全性 |
| 20 | [DaemonSet故障排查](./20-daemonset-troubleshooting.md) | 节点级服务、系统守护进程 |
| 21 | [StatefulSet故障排查](./21-statefulset-troubleshooting.md) | 有状态应用、持久化、有序部署 |
| 22 | [Job故障排查](./22-job-troubleshooting.md) | 批处理任务、并行执行、完成策略 |
| 23 | [Namespace故障排查](./23-namespace-troubleshooting.md) | 资源隔离、配额管理、生命周期 |
| 24 | [Quota/LimitRange故障排查](./24-quota-limitrange-troubleshooting.md) | 资源限制、配额超限、默认值配置 |
| 25 | [网络连通性故障排查](./25-network-connectivity-troubleshooting.md) | Pod通信、Service访问、DNS解析 |
| 26 | [DNS故障排查](./26-dns-troubleshooting.md) | CoreDNS配置、外部解析、缓存优化 |
| 27 | [镜像仓库故障排查](./27-image-registry-troubleshooting.md) | 镜像拉取、认证管理、网络代理 |
| 28 | [集群自动扩缩容故障排查](./28-cluster-autoscaler-troubleshooting.md) | 扩缩容策略、节点驱逐、云API集成 |
| 29 | [云提供商集成故障排查](./29-cloud-provider-troubleshooting.md) | 认证权限、LoadBalancer、存储卷 |
| 30 | [监控告警故障排查](./30-monitoring-alerting-troubleshooting.md) | Prometheus、Alertmanager、Grafana |
| 31 | [备份恢复故障排查](./31-backup-restore-troubleshooting.md) | Velero备份、etcd快照、灾难恢复 |
| 32 | [安全相关故障排查](./32-security-troubleshooting.md) | 认证授权、网络安全、镜像安全 |
| 33 | [性能瓶颈故障排查](./33-performance-bottleneck-troubleshooting.md) | CPU/内存/存储/I/O瓶颈分析 |
| 34 | [升级迁移故障排查](./34-upgrade-migration-troubleshooting.md) | 版本兼容性、滚动升级、回滚策略 |
| 35 | [节点组件故障排查](./35-node-component-troubleshooting.md) | kubelet、容器运行时、kube-proxy |
| 36 | [Helm Chart故障排查](./36-helm-chart-troubleshooting.md) | Chart渲染、依赖管理、Release状态 |
| 37 | [多集群管理故障排查](./37-multi-cluster-management-troubleshooting.md) | 联邦控制、跨集群网络、故障转移 |
| 38 | [GitOps/ArgoCD故障排查](./38-gitops-argocd-troubleshooting.md) | ArgoCD同步失败、应用状态、RBAC配置 |

---

## 使用指南

### 新手运维工程师
建议按以下顺序学习：
1. 基础故障排查文档 (01-14)
2. 工作负载故障排查 (15-22)
3. 高级运维文档 (23-38)

### 资深SRE工程师
可以直接查阅相关领域的专项故障排查文档，重点关注：
- 安全和监控相关文档
- 性能优化和升级迁移文档
- 自动化工具和最佳实践

### 企业架构师
建议重点关注：
- 整体架构设计和最佳实践
- 安全合规和灾备方案
- 性能优化和成本控制

---

## 统计信息

- **文档总数**: 38篇
- **总字数**: ~850,000字
- **命令示例**: 超过2000个可执行命令
- **脚本工具**: 提供100+自动化诊断脚本
- **配置模板**: 包含200+生产级配置示例

---
