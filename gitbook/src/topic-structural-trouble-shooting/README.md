# Kubernetes 结构化故障排查知识库

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **文档数量**: 40篇

本目录包含 Kubernetes 各组件的全面故障排查指南，每篇文档均基于生产环境真实案例编写，提供：
- **系统性排查方法**：从现象到根因的完整排查路径
- **实战经验总结**：来自大型互联网公司的运维实践
- **风险控制指导**：安全生产的操作规范和应急预案
- **性能优化建议**：高负载场景下的调优方案
- **自动化工具**：减少人工干预的运维脚本集合

---

## 目录结构

### 01-control-plane（控制平面组件）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-apiserver-troubleshooting.md](01-control-plane/01-apiserver-troubleshooting.md) | API Server 故障排查 | kubectl 无法连接、API 响应慢、认证授权错误 |
| [02-etcd-troubleshooting.md](01-control-plane/02-etcd-troubleshooting.md) | etcd 故障排查 | etcd 不可用、数据损坏、性能问题、备份恢复 |
| [03-scheduler-troubleshooting.md](01-control-plane/03-scheduler-troubleshooting.md) | Scheduler 故障排查 | Pod Pending、调度失败、调度策略问题 |
| [04-controller-manager-troubleshooting.md](01-control-plane/04-controller-manager-troubleshooting.md) | Controller Manager 故障排查 | 控制器异常、资源不同步、Endpoints 问题 |
| [05-webhook-admission-troubleshooting.md](01-control-plane/05-webhook-admission-troubleshooting.md) | Webhook/准入控制故障排查 | Webhook 超时、资源被拒绝、准入控制器问题 |
| [06-apf-troubleshooting.md](01-control-plane/06-apf-troubleshooting.md) | API 优先级与公平性故障排查 | 请求限流 (429)、API 延迟、FlowSchema 配置 |

### 02-node-components（节点组件）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-kubelet-troubleshooting.md](02-node-components/01-kubelet-troubleshooting.md) | kubelet 故障排查 | 节点 NotReady、Pod 创建失败、镜像拉取问题 |
| [02-kube-proxy-troubleshooting.md](02-node-components/02-kube-proxy-troubleshooting.md) | kube-proxy 故障排查 | Service 不可达、iptables/IPVS 规则问题 |
| [03-container-runtime-troubleshooting.md](02-node-components/03-container-runtime-troubleshooting.md) | 容器运行时故障排查 | containerd/Docker 故障、容器创建失败 |
| [04-node-troubleshooting.md](02-node-components/04-node-troubleshooting.md) | 节点故障专项排查 | 节点压力、污点容忍、亲和性、资源驱逐 |
| [05-image-registry-troubleshooting.md](02-node-components/05-image-registry-troubleshooting.md) | 镜像与镜像仓库故障排查 | 镜像拉取失败、认证问题、TLS 错误、限流 |
| [06-gpu-device-plugin-troubleshooting.md](02-node-components/06-gpu-device-plugin-troubleshooting.md) | GPU/设备插件故障排查 | GPU 不可见、设备分配失败、CUDA 兼容性、MIG 配置 |

### 03-networking（网络）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-cni-troubleshooting.md](03-networking/01-cni-troubleshooting.md) | CNI 网络插件故障排查 | Pod 网络不通、跨节点通信失败、IP 分配问题 |
| [02-dns-troubleshooting.md](03-networking/02-dns-troubleshooting.md) | CoreDNS/DNS 故障排查 | DNS 解析失败、服务发现异常、DNS 性能问题 |
| [03-service-ingress-troubleshooting.md](03-networking/03-service-ingress-troubleshooting.md) | Service/Ingress 故障排查 | Service 不可达、Ingress 路由问题、TLS 证书错误 |
| [04-networkpolicy-troubleshooting.md](03-networking/04-networkpolicy-troubleshooting.md) | NetworkPolicy 故障排查 | 网络策略不生效、流量被误拦截、策略配置问题 |
| [05-service-mesh-istio-troubleshooting.md](03-networking/05-service-mesh-istio-troubleshooting.md) | Service Mesh (Istio) 故障排查 | Sidecar 注入失败、mTLS 问题、流量路由异常、Gateway 不可用 |
| [06-gateway-api-troubleshooting.md](03-networking/06-gateway-api-troubleshooting.md) | Gateway API 故障排查 | GatewayClass/Gateway/HTTPRoute 配置、跨 namespace 路由、TLS 配置 |

### 04-storage（存储）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-pv-pvc-troubleshooting.md](04-storage/01-pv-pvc-troubleshooting.md) | PV/PVC 存储故障排查 | PVC Pending、卷挂载失败、存储类问题 |
| [02-csi-troubleshooting.md](04-storage/02-csi-troubleshooting.md) | CSI 存储驱动故障排查 | CSI 驱动故障、卷创建/挂载/扩容问题 |

### 05-workloads（工作负载）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-pod-troubleshooting.md](05-workloads/01-pod-troubleshooting.md) | Pod 故障排查 | Pod Pending/CrashLoopBackOff/OOMKilled、镜像拉取失败 |
| [02-deployment-troubleshooting.md](05-workloads/02-deployment-troubleshooting.md) | Deployment 故障排查 | 滚动更新卡住、副本数不足、回滚问题 |
| [03-statefulset-troubleshooting.md](05-workloads/03-statefulset-troubleshooting.md) | StatefulSet 故障排查 | 有序部署问题、PVC 绑定失败、网络标识异常 |
| [04-daemonset-troubleshooting.md](05-workloads/04-daemonset-troubleshooting.md) | DaemonSet 故障排查 | 节点污点、Pod 未调度、系统组件故障 |
| [05-job-cronjob-troubleshooting.md](05-workloads/05-job-cronjob-troubleshooting.md) | Job/CronJob 故障排查 | 任务失败、定时任务不触发、并行执行问题 |
| [06-configmap-secret-troubleshooting.md](05-workloads/06-configmap-secret-troubleshooting.md) | ConfigMap/Secret 故障排查 | 配置注入失败、热更新问题、编码问题 |

### 06-security-auth（安全与认证）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-rbac-troubleshooting.md](06-security-auth/01-rbac-troubleshooting.md) | RBAC 与认证故障排查 | 权限不足、认证失败、ServiceAccount 问题 |
| [02-certificate-troubleshooting.md](06-security-auth/02-certificate-troubleshooting.md) | 证书故障排查 | 证书过期、CA 不信任、TLS 握手失败、kubeconfig 失效 |
| [03-pod-security-troubleshooting.md](06-security-auth/03-pod-security-troubleshooting.md) | Pod 安全故障排查 | PSA 策略拒绝、SecurityContext 问题、权限不足 |
| [04-audit-logging-troubleshooting.md](06-security-auth/04-audit-logging-troubleshooting.md) | 审计日志故障排查 | 审计日志配置、Webhook 发送失败、日志分析、敏感信息保护 |

### 07-resources-scheduling（资源与调度）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-resources-quota-troubleshooting.md](07-resources-scheduling/01-resources-quota-troubleshooting.md) | 资源与配额故障排查 | 资源配额超限、OOM、调度失败 |
| [02-autoscaling-troubleshooting.md](07-resources-scheduling/02-autoscaling-troubleshooting.md) | HPA/VPA 自动扩缩容故障排查 | 自动扩缩不生效、metrics-server 故障、扩缩容振荡 |
| [03-cluster-autoscaler-troubleshooting.md](07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md) | Cluster Autoscaler 故障排查 | 节点不扩容/不缩容、云 API 错误、扩容延迟 |
| [04-pdb-troubleshooting.md](07-resources-scheduling/04-pdb-troubleshooting.md) | PodDisruptionBudget 故障排查 | drain 卡住、缩容阻塞、PDB 配置问题 |

### 08-cluster-operations（集群运维）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-cluster-maintenance-troubleshooting.md](08-cluster-operations/01-cluster-maintenance-troubleshooting.md) | 集群运维故障排查 | 集群升级、节点维护、版本兼容 |
| [02-logging-monitoring-troubleshooting.md](08-cluster-operations/02-logging-monitoring-troubleshooting.md) | 日志与监控故障排查 | 日志丢失、Prometheus 故障、告警问题、Grafana 异常 |
| [03-helm-troubleshooting.md](08-cluster-operations/03-helm-troubleshooting.md) | Helm 部署故障排查 | Release 失败、模板错误、升级回滚问题 |
| [04-ha-disaster-recovery-troubleshooting.md](08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md) | 高可用与灾备故障排查 | 控制平面故障、etcd 恢复、备份还原、灾难恢复 |
| [05-crd-operator-troubleshooting.md](08-cluster-operations/05-crd-operator-troubleshooting.md) | CRD/Operator 故障排查 | CRD 版本冲突、Operator 崩溃、Reconcile 失败、Finalizer 阻塞 |
| [06-kustomize-troubleshooting.md](08-cluster-operations/06-kustomize-troubleshooting.md) | Kustomize 部署故障排查 | 构建失败、Patch 不生效、多环境配置、镜像替换问题 |

---

## 使用方式与前置

- **面向读者**：初学者可先按“按错误现象查找”快速定位，再跳转具体文档；资深工程师可直接定位到组件章节，结合监控/日志做横向对比。
- **建议工具**：kubectl + stern/tail、kubectl-debug/ephemeral container、kubectl-trace、eBPF 观测工具 (bcc/bpftrace/inspektor-gadget)、perf/flamegraph、sysdig/ksniff、tcpdump/wireshark。
- **排查前置**：记录变更窗口、确认影响范围、备份关键配置/证书/etcd、准备回滚方案；生产环境操作优先在低峰执行并预留隔离窗口。
- **数据留存**：操作前后收集 `kubectl get/describe/logs`, 关键组件 (kube-apiserver/kubelet/etcd/controller-manager/scheduler/coredns/ingress) 日志与指标快照，必要时保留 pprof/heapdump。
- **安全提示**：涉及证书/密钥/审计日志时注意脱敏；对 Webhook、PSA、NetworkPolicy、PDB 等变更先在灰度/测试环境验证。

## 快速定位指南

### 按错误现象查找

| 错误现象 | 推荐文档 |
|----------|----------|
| kubectl 连接失败 | API Server、证书、高可用 |
| 节点 NotReady | kubelet、容器运行时、节点故障专项 |
| 节点资源压力 | 节点故障专项、资源配额 |
| Pod Pending | Scheduler、资源配额、PV/PVC、节点故障 |
| Pod CrashLoopBackOff | Pod 故障排查 |
| Pod OOMKilled | 资源配额 |
| Service 不可达 | kube-proxy、Service/Ingress |
| DNS 解析失败 | DNS 故障排查 |
| 镜像拉取失败 | kubelet、容器运行时、ConfigMap/Secret |
| 卷挂载失败 | PV/PVC、CSI 存储驱动 |
| 权限不足 (403) | RBAC、Pod 安全 |
| 证书过期/TLS 错误 | 证书故障排查 |
| Webhook 拒绝请求 | Webhook/准入控制 |
| HPA 不扩容 | HPA/VPA 自动扩缩容 |
| 日志/指标缺失 | 日志与监控 |
| 网络策略阻断 | NetworkPolicy |
| Deployment 更新卡住 | Deployment 故障排查 |
| StatefulSet Pod 不创建 | StatefulSet 故障排查 |
| CronJob 未执行 | Job/CronJob 故障排查 |
| ConfigMap/Secret 不生效 | ConfigMap/Secret 故障排查 |
| Helm 安装/升级失败 | Helm 部署故障排查 |
| etcd 集群故障 | etcd、高可用与灾备 |
| PSA 拒绝 Pod | Pod 安全故障排查 |
| GPU Pod 调度失败 | GPU/设备插件故障排查 |
| 镜像拉取认证失败 | 镜像与镜像仓库故障排查 |
| Istio Sidecar 问题 | Service Mesh (Istio) 故障排查 |
| CRD/CR 操作失败 | CRD/Operator 故障排查 |
| Operator 无法调谐 | CRD/Operator 故障排查 |
| Finalizer 阻塞删除 | CRD/Operator 故障排查 |
| API 请求限流 (429) | API 优先级与公平性故障排查 |
| kubectl drain 卡住 | PodDisruptionBudget 故障排查 |
| 节点不扩容/不缩容 | Cluster Autoscaler 故障排查 |
| Gateway API 路由不生效 | Gateway API 故障排查 |
| Kustomize 构建失败 | Kustomize 部署故障排查 |
| 审计日志缺失 | 审计日志故障排查 |

### 按组件查找

| 组件 | 推荐文档 |
|------|----------|
| kube-apiserver | 01-control-plane/01-apiserver-troubleshooting.md |
| etcd | 01-control-plane/02-etcd-troubleshooting.md |
| kube-scheduler | 01-control-plane/03-scheduler-troubleshooting.md |
| kube-controller-manager | 01-control-plane/04-controller-manager-troubleshooting.md |
| Admission Webhook | 01-control-plane/05-webhook-admission-troubleshooting.md |
| kubelet | 02-node-components/01-kubelet-troubleshooting.md |
| kube-proxy | 02-node-components/02-kube-proxy-troubleshooting.md |
| containerd/Docker | 02-node-components/03-container-runtime-troubleshooting.md |
| Node (节点) | 02-node-components/04-node-troubleshooting.md |
| Image Registry | 02-node-components/05-image-registry-troubleshooting.md |
| GPU/Device Plugin | 02-node-components/06-gpu-device-plugin-troubleshooting.md |
| CoreDNS | 03-networking/02-dns-troubleshooting.md |
| CNI (Calico/Flannel/Cilium) | 03-networking/01-cni-troubleshooting.md |
| Ingress Controller | 03-networking/03-service-ingress-troubleshooting.md |
| NetworkPolicy | 03-networking/04-networkpolicy-troubleshooting.md |
| Istio/Service Mesh | 03-networking/05-service-mesh-istio-troubleshooting.md |
| PV/PVC | 04-storage/01-pv-pvc-troubleshooting.md |
| CSI Driver | 04-storage/02-csi-troubleshooting.md |
| Deployment | 05-workloads/02-deployment-troubleshooting.md |
| StatefulSet | 05-workloads/03-statefulset-troubleshooting.md |
| DaemonSet | 05-workloads/04-daemonset-troubleshooting.md |
| Job/CronJob | 05-workloads/05-job-cronjob-troubleshooting.md |
| ConfigMap/Secret | 05-workloads/06-configmap-secret-troubleshooting.md |
| HPA/VPA | 07-resources-scheduling/02-autoscaling-troubleshooting.md |
| metrics-server | 07-resources-scheduling/02-autoscaling-troubleshooting.md |
| Prometheus | 08-cluster-operations/02-logging-monitoring-troubleshooting.md |
| Fluentd/Fluent Bit | 08-cluster-operations/02-logging-monitoring-troubleshooting.md |
| Helm | 08-cluster-operations/03-helm-troubleshooting.md |
| cert-manager | 06-security-auth/02-certificate-troubleshooting.md |
| CRD/Operator | 08-cluster-operations/05-crd-operator-troubleshooting.md |
| Kustomize | 08-cluster-operations/06-kustomize-troubleshooting.md |
| Gateway API | 03-networking/06-gateway-api-troubleshooting.md |
| Cluster Autoscaler | 07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md |
| PodDisruptionBudget | 07-resources-scheduling/04-pdb-troubleshooting.md |
| APF (FlowSchema) | 01-control-plane/06-apf-troubleshooting.md |
| Audit Logging | 06-security-auth/04-audit-logging-troubleshooting.md |

---

## 通用排查流程

```
问题发生
    │
    ├─► 确认影响范围
    │       │
    │       ├─► 单个 Pod ──► Pod 故障排查
    │       ├─► 单个节点 ──► 节点故障专项/kubelet/容器运行时
    │       ├─► 多个节点 ──► 控制平面组件
    │       └─► 整个集群 ──► API Server/etcd/高可用
    │
    ├─► 收集信息
    │       │
    │       ├─► kubectl describe
    │       ├─► kubectl logs
    │       ├─► journalctl
    │       └─► 监控系统
    │
    ├─► 分析原因
    │       │
    │       ├─► 查看 Events
    │       ├─► 查看日志
    │       └─► 检查配置
    │
    └─► 执行修复
            │
            ├─► 评估风险
            ├─► 准备回滚
            └─► 验证恢复
```

---

## 文档统计

| 类别 | 文档数 | 覆盖内容 | 生产环境重点 |
|------|--------|----------|--------------|
| 控制平面 | 6 | API Server、etcd、Scheduler、Controller Manager、Webhook、APF | ⭐⭐⭐ 集群核心组件 |
| 节点组件 | 6 | kubelet、kube-proxy、容器运行时、节点故障专项、镜像仓库、GPU/设备插件 | ⭐⭐⭐ 节点稳定性保障 |
| 网络 | 6 | CNI、DNS、Service/Ingress、NetworkPolicy、Service Mesh、Gateway API | ⭐⭐ 网络连通性保障 |
| 存储 | 2 | PV/PVC、CSI 驱动 | ⭐⭐ 数据持久化保障 |
| 工作负载 | 6 | Pod、Deployment、StatefulSet、DaemonSet、Job/CronJob、ConfigMap/Secret | ⭐⭐⭐ 业务应用保障 |
| 安全认证 | 4 | RBAC、证书、Pod 安全、审计日志 | ⭐⭐⭐ 安全合规保障 |
| 资源调度 | 4 | 资源配额、HPA/VPA、Cluster Autoscaler、PDB | ⭐⭐ 性能优化保障 |
| 集群运维 | 6 | 维护升级、日志监控、Helm、高可用灾备、CRD/Operator、Kustomize | ⭐⭐⭐ 运维效率提升 |
| **总计** | **40** | | |

---

## 紧急联系人

在遇到以下情况时，建议立即升级处理：
- etcd 数据损坏或不可用
- 多数控制平面节点故障
- 大规模节点 NotReady
- 证书全部过期导致集群不可用
- 安全相关的紧急事件
- 需要从备份恢复集群

---

## 贡献指南

欢迎补充和完善故障排查文档，请遵循以下格式：

1. **问题现象与影响分析**
   - 常见问题现象表格
   - 报错查看方式汇总
   - 影响面分析（直接/间接影响）

2. **排查方法与步骤**
   - 排查原理说明
   - 排查逻辑决策树
   - 具体排查命令

3. **解决方案与风险控制**
   - 解决步骤（含具体命令）
   - 执行风险评估
   - 安全生产风险提示

---

## 📝 更新日志

### 2026-01 (最新)
- ✨ 丰富所有文档内容，添加生产环境实战经验和最佳实践
- ✨ 增加典型故障场景分析和预防措施
- ✨ 补充自动化运维脚本和监控告警配置
- ✨ 优化文档结构，提升可读性和实用性
- ✨ 更新目录命名规范，统一为结构化命名

### 2025-12
- 🎉 初始化完整的故障排查知识库框架
- 🎉 建立标准化文档模板和内容结构