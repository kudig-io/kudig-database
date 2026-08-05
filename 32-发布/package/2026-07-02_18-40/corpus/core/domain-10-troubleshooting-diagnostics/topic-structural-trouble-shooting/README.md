---
title: Kubernetes 结构化故障排查知识库 [domain-10-troubleshooting-diagnostics]
description: '# Kubernetes 结构化故障排查知识库'
summary: '本目录包含 Kubernetes 各组件的全面故障排查指南，每篇文档均基于生产环境真实案例编写，提供：'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- Kubernetes 结构化故障排查知识库 是什么
- 如何 Kubernetes 结构化故障排查知识库
- Kubernetes 结构化故障排查知识库 故障排查
- Kubernetes 结构化故障排查知识库 排障步骤
trigger_keywords:
- Kubernetes
- 结构化故障排查知识库
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
- tls-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 结构化故障排查知识库

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-04 | **文档数量**: 63篇

本目录包含 Kubernetes 各组件的全面故障排查指南，每篇文档均基于生产环境真实案例编写，提供：
- **系统性排查方法**：从现象到根因的完整排查路径
- **实战经验总结**：来自大型互联网公司的运维实践
- **风险控制指导**：安全生产的操作规范和应急预案
- **性能优化建议**：高负载场景下的调优方案
- **自动化工具**：减少人工干预的运维脚本集合

---

## 排查方法论

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [00-configuration-first-methodology.md](00-configuration-first-methodology.md) | **配置优先（Configuration-First）排查方法论** | 疑难问题的系统性排查，强调先检查配置文件再深入网络/系统排查，以 [[CoreDNS|CoreDNS]] 为完整示例 |

> **推荐**：遇到复杂疑难问题时，先阅读方法论文档确定排查策略，再进入具体组件的排查指南。

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
| [07-control-plane-security-troubleshooting.md](01-control-plane/07-control-plane-security-troubleshooting.md) | 控制平面安全故障排查 | PSA 策略冲突、安全上下文配置、准入控制绕过 |
| [08-control-plane-performance-troubleshooting.md](01-control-plane/08-control-plane-performance-troubleshooting.md) | 控制平面性能故障排查 | API Server 延迟、etcd 性能退化、大规模 LIST 请求 |
| [09-control-plane-ha-troubleshooting.md](01-control-plane/09-control-plane-ha-troubleshooting.md) | 控制平面高可用故障排查 | 多 master 节点问题、etcd 仲裁丢失、VIP 漂移 |
| [10-control-plane-upgrade-troubleshooting.md](01-control-plane/10-control-plane-upgrade-troubleshooting.md) | 控制平面升级故障排查 | 版本升级失败、API 废弃、 kubeadm 升级卡住 |

### 02-node-components（节点组件）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-kubelet-troubleshooting.md](02-node-components/01-kubelet-troubleshooting.md) | kubelet 故障排查 | 节点 NotReady、Pod 创建失败、镜像拉取问题 |
| [02-kube-proxy-troubleshooting.md](02-node-components/02-kube-proxy-troubleshooting.md) | kube-proxy 故障排查 | Service 不可达、iptables/IPVS 规则问题 |
| [03-container-runtime-troubleshooting.md](02-node-components/03-container-runtime-troubleshooting.md) | 容器运行时故障排查 | containerd/Docker 问题、容器创建失败 |
| [04-node-troubleshooting.md](02-node-components/04-node-troubleshooting.md) | 节点问题专项排查 | 节点压力、污点容忍、亲和性、资源驱逐 |
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
| [07-terway-troubleshooting.md](03-networking/07-terway-troubleshooting.md) | Terway（阿里云 CNI）故障排查 | ENI/IPVlan 模式、IPAM、安全组、跨节点通信、网络策略 |
| [08-flannel-troubleshooting.md](03-networking/08-flannel-troubleshooting.md) | Flannel [[skills/ts-networking.md|ts-networking]] | VXLAN/host-gw 模式、子网分配、跨节点通信、MTU、后端切换 |

### 04-storage（存储）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-pv-pvc-troubleshooting.md](04-storage/01-pv-pvc-troubleshooting.md) | PV/PVC [[skills/ts-storage.md|ts-storage]] | PVC Pending、卷挂载失败、存储类问题 |
| [02-csi-troubleshooting.md](04-storage/02-csi-troubleshooting.md) | CSI 存储驱动故障排查 | CSI 驱动问题、卷创建/挂载/扩容问题 |
| [03-snapshot-backup-troubleshooting.md](04-storage/03-snapshot-backup-troubleshooting.md) | CSI 快照与卷备份故障排查 | VolumeSnapshot、快照恢复、数据一致性 |
| [04-storage-performance-troubleshooting.md](04-storage/04-storage-performance-troubleshooting.md) | 存储 I/O 性能故障排查 | 高延迟 I/O、吞吐瓶颈、fio 基准测试 |
| [05-storageclass-troubleshooting.md](04-storage/05-storageclass-troubleshooting.md) | StorageClass 配置与动态供给故障排查 | StorageClass 参数、volumeBindingMode、拓扑约束、扩容、性能等级 |

### 05-workloads（工作负载）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-pod-troubleshooting.md](05-workloads/01-pod-troubleshooting.md) | Pod 故障排查 | Pod Pending/CrashLoopBackOff/OOMKilled、镜像拉取失败 |
| [02-deployment-troubleshooting.md](05-workloads/02-deployment-troubleshooting.md) | Deployment 故障排查 | 滚动更新卡住、副本数不足、回滚问题 |
| [03-statefulset-troubleshooting.md](05-workloads/03-statefulset-troubleshooting.md) | StatefulSet 故障排查 | 有序部署问题、PVC 绑定失败、网络标识异常 |
| [04-daemonset-troubleshooting.md](05-workloads/04-daemonset-troubleshooting.md) | DaemonSet 故障排查 | 节点污点、Pod 未调度、系统组件问题 |
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
| [02-autoscaling-troubleshooting.md](07-resources-scheduling/02-autoscaling-troubleshooting.md) | HPA/VPA 自动扩缩容故障排查 | 自动扩缩不生效、metrics-server 问题、扩缩容振荡 |
| [03-cluster-autoscaler-troubleshooting.md](07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md) | Cluster Autoscaler 故障排查 | 节点不扩容/不缩容、云 API 错误、扩容延迟 |
| [04-pdb-troubleshooting.md](07-resources-scheduling/04-pdb-troubleshooting.md) | PodDisruptionBudget 故障排查 | drain 卡住、缩容阻塞、PDB 配置问题 |

### 08-cluster-operations（集群运维）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-cluster-maintenance-troubleshooting.md](08-cluster-operations/01-cluster-maintenance-troubleshooting.md) | [[skills/ts-cluster-operations.md|ts-cluster-operations]] | 集群升级、节点维护、版本兼容 |
| [02-logging-monitoring-troubleshooting.md](08-cluster-operations/02-logging-monitoring-troubleshooting.md) | 日志与监控故障排查 | 日志丢失、Prometheus 问题、告警问题、Grafana 异常 |
| [03-helm-troubleshooting.md](08-cluster-operations/03-helm-troubleshooting.md) | Helm 部署故障排查 | Release 失败、模板错误、升级回滚问题 |
| [04-ha-disaster-recovery-troubleshooting.md](08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md) | 高可用与灾备故障排查 | 控制平面问题、etcd 恢复、备份还原、灾难恢复 |
| [05-crd-operator-troubleshooting.md](08-cluster-operations/05-crd-operator-troubleshooting.md) | CRD/Operator 故障排查 | CRD 版本冲突、Operator 崩溃、Reconcile 失败、Finalizer 阻塞 |
| [06-kustomize-troubleshooting.md](08-cluster-operations/06-kustomize-troubleshooting.md) | Kustomize 部署故障排查 | 构建失败、Patch 不生效、多环境配置、镜像替换问题 |

### 09-cloud-provider（云厂商集成）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-cloud-provider-integration-troubleshooting.md](09-cloud-provider/01-cloud-provider-integration-troubleshooting.md) | 云厂商集成故障排查 | CCM 认证失败、LoadBalancer 创建失败、云 API 限流 |
| [02-multi-cloud-networking-troubleshooting.md](09-cloud-provider/02-multi-cloud-networking-troubleshooting.md) | 多云/混合云网络故障排查 | 跨云 VPC Peering、VPN 隧道、集群网格互联 |
| [03-cloud-resource-quota-troubleshooting.md](09-cloud-provider/03-cloud-resource-quota-troubleshooting.md) | 云资源配额与 API 限流故障排查 | 配额耗尽、实例扩容失败、API Throttling |

### 10-ai-ml-workloads（AI/ML 工作负载）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-ai-ml-workloads-troubleshooting.md](12-ai-ml-workloads/01-ai-ml-workloads-troubleshooting.md) | AI/ML 工作负载通用故障排查 | GPU 调度、分布式训练、模型服务、数据处理 |
| [02-kubeflow-troubleshooting.md](12-ai-ml-workloads/02-kubeflow-troubleshooting.md) | Kubeflow 平台故障排查 | Pipeline 失败、Katib 实验、KServe 推理、Notebook |
| [03-mpi-operator-troubleshooting.md](12-ai-ml-workloads/03-mpi-operator-troubleshooting.md) | MPI Operator 与分布式训练故障排查 | MPIJob 启动、NCCL 通信、多节点 GPU 训练 |

### 11-gitops-devops（GitOps 与 DevOps）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-gitops-devops-troubleshooting.md](14-gitops-devops/01-gitops-devops-troubleshooting.md) | GitOps/DevOps 通用故障排查 | ArgoCD/Flux 同步、CI/CD 流水线、Secret 管理 |
| [02-tekton-troubleshooting.md](14-gitops-devops/02-tekton-troubleshooting.md) | Tekton CI/CD 流水线故障排查 | PipelineRun 失败、Workspace 问题、触发器异常 |
| [03-flux-image-automation-troubleshooting.md](14-gitops-devops/03-flux-image-automation-troubleshooting.md) | Flux 镜像自动化故障排查 | 镜像扫描失败、策略不匹配、Git 自动提交异常 |

### 12-monitoring-01-observability-architecture-overview（可观测性）

| 文档 | 说明 | 适用场景 |
|------|------|----------|
| [01-monitoring-observability-troubleshooting.md](15-monitoring-observability/01-monitoring-observability-troubleshooting.md) | 可观测性通用故障排查 | Prometheus/Grafana/Loki/Jaeger/AlertManager |
| [02-opentelemetry-troubleshooting.md](15-monitoring-observability/02-opentelemetry-troubleshooting.md) | OpenTelemetry Collector 故障排查 | OTLP 接收/导出失败、采样、数据丢失 |
| [03-ebpf-observability-troubleshooting.md](15-monitoring-observability/03-ebpf-observability-troubleshooting.md) | eBPF 可观测性故障排查 | Cilium Hubble、Tetragon、Pixie、eBPF 加载 |
| [04-finops-cost-optimization-troubleshooting.md](15-monitoring-observability/04-finops-cost-optimization-troubleshooting.md) | FinOps 成本优化故障排查 | 成本飙升、闲置资源、Spot 优化、预算告警 |

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
| 节点 NotReady | kubelet、容器运行时、节点问题专项 |
| 节点资源压力 | 节点问题专项、资源配额 |
| Pod Pending | Scheduler、资源配额、PV/PVC、节点问题 |
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
| etcd 集群问题 | etcd、高可用与灾备 |
| PSA 拒绝 Pod | Pod 安全故障排查 |
| GPU Pod 调度失败 | GPU/设备插件故障排查 |
| 镜像拉取认证失败 | 镜像与镜像仓库故障排查 |
| Istio Sidecar 问题 | Service Mesh (Istio) 故障排查 |
| CRD/CR 操作失败 | CRD/Operator 故障排查 |
| Operator 无法调谐 | CRD/Operator 故障排查 |
| Finalizer 阻塞删除 | CRD/Operator 故障排查 |
| API 请求限流 (429) | API 优先级与公平性故障排查 |
| 控制平面安全告警 | 控制平面安全故障排查 |
| API Server 响应缓慢 | 控制平面性能故障排查 |
| 多 master 节点问题 | 控制平面高可用故障排查 |
| 集群升级失败 | 控制平面升级故障排查 |
| kubectl drain 卡住 | PodDisruptionBudget 故障排查 |
| 节点不扩容/不缩容 | Cluster Autoscaler 故障排查 |
| Gateway API 路由不生效 | Gateway API 故障排查 |
| Kustomize 构建失败 | Kustomize 部署故障排查 |
| 审计日志缺失 | 审计日志故障排查 |
| 快照创建/恢复失败 | CSI 快照与卷备份故障排查 |
| 存储 I/O 性能差 | 存储 I/O 性能故障排查 |
| StorageClass 配置错误 | StorageClass 配置与动态供给故障排查 |
| PVC 扩容失败 | StorageClass 配置与动态供给故障排查 |
| 跨云网络不通 | 多云/混合云网络故障排查 |
| Terway Pod 无 IP | Terway（阿里云 CNI）故障排查 |
| Flannel 跨节点不通 | Flannel 网络故障排查 |
| 云配额超限无法扩容 | 云资源配额与 API 限流故障排查 |
| Kubeflow Pipeline 失败 | Kubeflow 平台故障排查 |
| MPI 分布式训练失败 | MPI Operator 故障排查 |
| Tekton 流水线失败 | Tekton CI/CD 流水线故障排查 |
| Flux 镜像未自动更新 | Flux 镜像自动化故障排查 |
| OpenTelemetry 数据丢失 | OpenTelemetry Collector 故障排查 |
| eBPF 程序加载失败 | eBPF 可观测性故障排查 |
| 云成本异常飙升 | FinOps 成本优化故障排查 |

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
| Terway (阿里云 CNI) | 03-networking/07-terway-troubleshooting.md |
| Flannel | 03-networking/08-flannel-troubleshooting.md |
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
| Pod | 05-workloads/01-pod-troubleshooting.md |
| Resource Quota | 07-resources-scheduling/01-resources-quota-troubleshooting.md |
| HPA/VPA | 07-resources-scheduling/02-autoscaling-troubleshooting.md |
| metrics-server | 07-resources-scheduling/02-autoscaling-troubleshooting.md |
| Prometheus | 08-cluster-operations/02-logging-monitoring-troubleshooting.md |
| Fluentd/Fluent Bit | 08-cluster-operations/02-logging-monitoring-troubleshooting.md |
| Helm | 08-cluster-operations/03-helm-troubleshooting.md |
| Cluster Maintenance | 08-cluster-operations/01-cluster-maintenance-troubleshooting.md |
| HA/Disaster Recovery | 08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md |
| cert-manager | 06-security-auth/02-certificate-troubleshooting.md |
| CRD/Operator | 08-cluster-operations/05-crd-operator-troubleshooting.md |
| Kustomize | 08-cluster-operations/06-kustomize-troubleshooting.md |
| Gateway API | 03-networking/06-gateway-api-troubleshooting.md |
| Cluster Autoscaler | 07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md |
| PodDisruptionBudget | 07-resources-scheduling/04-pdb-troubleshooting.md |
| APF (FlowSchema) | 01-control-plane/06-apf-troubleshooting.md |
| Control Plane Security | 01-control-plane/07-control-plane-security-troubleshooting.md |
| Control Plane Performance | 01-control-plane/08-control-plane-performance-troubleshooting.md |
| Control Plane HA | 01-control-plane/09-control-plane-ha-troubleshooting.md |
| Control Plane Upgrade | 01-control-plane/10-control-plane-upgrade-troubleshooting.md |
| Audit Logging | 06-security-auth/04-audit-logging-troubleshooting.md |
| CSI Snapshot/Backup | 04-storage/03-snapshot-backup-troubleshooting.md |
| Storage Performance | 04-storage/04-storage-performance-troubleshooting.md |
| StorageClass | 04-storage/05-storageclass-troubleshooting.md |
| Cloud Provider Integration | 09-cloud-provider/01-cloud-provider-integration-troubleshooting.md |
| Multi-Cloud Network | 09-cloud-provider/02-multi-cloud-networking-troubleshooting.md |
| Cloud Quota/API | 09-cloud-provider/03-cloud-resource-quota-troubleshooting.md |
| Kubeflow | 10-ai-ml-workloads/02-kubeflow-troubleshooting.md |
| MPI Operator | 10-ai-ml-workloads/03-mpi-operator-troubleshooting.md |
| Tekton | 11-gitops-devops/02-tekton-troubleshooting.md |
| Flux Image Automation | 11-gitops-devops/03-flux-image-automation-troubleshooting.md |
| OpenTelemetry | 12-monitoring-observability/02-opentelemetry-troubleshooting.md |
| eBPF Observability | 12-monitoring-observability/03-ebpf-observability-troubleshooting.md |
| FinOps/Cost | 12-monitoring-observability/04-finops-cost-optimization-troubleshooting.md |

---

## 通用排查流程

```
问题发生
    │
    ├─► 确认影响范围
    │       │
    │       ├─► 单个 Pod ──► Pod 故障排查
    │       ├─► 单个节点 ──► 节点问题专项/kubelet/容器运行时
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
| 控制平面 | 10 | API Server、etcd、Scheduler、Controller Manager、Webhook、APF、安全、性能、高可用、升级 | ⭐⭐⭐ 集群核心组件 |
| 节点组件 | 6 | kubelet、kube-proxy、容器运行时、节点问题专项、镜像仓库、GPU/设备插件 | ⭐⭐⭐ 节点稳定性保障 |
| 网络 | 8 | CNI、DNS、Service/Ingress、NetworkPolicy、Service Mesh、Gateway API、Terway、Flannel | ⭐⭐ 网络连通性保障 |
| 存储 | 5 | PV/PVC、CSI 驱动、快照备份、存储性能、StorageClass | ⭐⭐ 数据持久化保障 |
| 工作负载 | 6 | Pod、Deployment、StatefulSet、DaemonSet、Job/CronJob、ConfigMap/Secret | ⭐⭐⭐ 业务应用保障 |
| 安全认证 | 4 | RBAC、证书、Pod 安全、审计日志 | ⭐⭐⭐ 安全合规保障 |
| 资源调度 | 4 | 资源配额、HPA/VPA、Cluster Autoscaler、PDB | ⭐⭐ 性能优化保障 |
| 集群运维 | 6 | 维护升级、日志监控、Helm、高可用灾备、CRD/Operator、Kustomize | ⭐⭐⭐ 运维效率提升 |
| 云厂商集成 | 3 | 云厂商集成、多云网络、资源配额 | ⭐⭐ 多云环境保障 |
| AI/ML 工作负载 | 3 | AI/ML 通用、Kubeflow、MPI 分布式训练 | ⭐⭐ AI 基础设施保障 |
| GitOps/DevOps | 3 | GitOps 通用、Tekton 流水线、Flux 镜像自动化 | ⭐⭐ 交付效率保障 |
| 可观测性 | 4 | 可观测性通用、OpenTelemetry、eBPF、FinOps 成本 | ⭐⭐ 全链路可观测 |
| **总计** | **63** | | |

---

## 紧急联系人

在遇到以下情况时，建议立即升级处理：
- etcd 数据损坏或不可用
- 多数控制平面节点问题
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

### 2026-04 (最新)
- ✨ 全面加强 03-networking、04-storage 核心网络与存储内容
- ✨ 新增 3 篇专项排查文档：Terway（阿里云 CNI）深度排查、Flannel 专项排查、StorageClass 配置与动态供给专项排查
- ✨ 全面扩充 04-storage、09-cloud-provider、10-ai-ml-workloads、11-gitops-devops、12-monitoring-observability 目录
- ✨ 新增 11 篇高质量故障排查文档，覆盖 CSI 快照、存储性能、多云网络、云配额、Kubeflow、MPI Operator、Tekton、Flux 镜像自动化、OpenTelemetry、eBPF 可观测性、FinOps 成本优化
- ✨ 所有新文档严格遵循"四要素法"（问题现象、报错信息、排查方案、解决方案）
- ✨ 更新 [[README]] 目录结构、快速定位指南、文档统计，总计 63 篇故障排查文档（含 1 篇方法论）
- ✨ 补充 Prometheus 监控告警规则、自动化诊断脚本和风险控制矩阵

### 2026-01
- ✨ 丰富所有文档内容，添加生产环境实战经验和最佳实践
- ✨ 增加典型问题场景分析和预防措施
- ✨ 补充自动化运维脚本和监控告警配置
- ✨ 优化文档结构，提升可读性和实用性
- ✨ 更新目录命名规范，统一为结构化命名

### 2025-12
- 🎉 初始化完整的故障排查知识库框架
- 🎉 建立标准化文档模板和内容结构

## Related

- [[README|README]]

- [[scripts/templates/decision-tree-template.md|decision-tree-template]]
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


## 历史归档版本

> 以下为迁移前的历史版本，保留供参考和对比。

### 01 Control Plane

- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/01-apiserver-troubleshooting.md|01-apiserver-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/02-etcd-troubleshooting.md|02-etcd-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/03-scheduler-troubleshooting.md|03-scheduler-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/04-controller-manager-troubleshooting.md|04-controller-manager-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/05-webhook-admission-troubleshooting.md|05-webhook-admission-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/06-apf-troubleshooting.md|06-apf-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/07-control-plane-security-troubleshooting.md|07-control-plane-security-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/01-control-plane/08-control-plane-performance-troubleshooting.md|08-control-plane-performance-troubleshooting]]

### 02 Node Components

- [[_archives/troubleshooting-diagnostics/高级排障/02-node-components/01-kubelet-troubleshooting.md|01-kubelet-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/02-node-components/02-kube-proxy-troubleshooting.md|02-kube-proxy-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/02-node-components/03-container-runtime-troubleshooting.md|03-container-runtime-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/02-node-components/04-node-troubleshooting.md|04-node-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/02-node-components/05-image-registry-troubleshooting.md|05-image-registry-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|06-gpu-device-plugin-troubleshooting]]

### 03 Networking

- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/01-cni-troubleshooting.md|01-cni-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/02-dns-troubleshooting.md|02-dns-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/03-service-ingress-troubleshooting.md|03-service-ingress-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/04-networkpolicy-troubleshooting.md|04-networkpolicy-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/05-service-mesh-istio-troubleshooting.md|05-service-mesh-istio-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/06-gateway-api-troubleshooting.md|06-gateway-api-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/07-terway-troubleshooting.md|07-terway-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/03-networking/08-flannel-troubleshooting.md|08-flannel-troubleshooting]]

### 04 Storage

- [[_archives/troubleshooting-diagnostics/高级排障/04-storage/01-pv-pvc-troubleshooting.md|01-pv-pvc-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/04-storage/02-csi-troubleshooting.md|02-csi-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/04-storage/03-snapshot-backup-troubleshooting.md|03-snapshot-backup-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/04-storage/04-storage-performance-troubleshooting.md|04-storage-performance-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/04-storage/05-storageclass-troubleshooting.md|05-storageclass-troubleshooting]]

### 05 Workloads

- [[_archives/troubleshooting-diagnostics/高级排障/05-workloads/01-pod-troubleshooting.md|01-pod-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/05-workloads/02-deployment-troubleshooting.md|02-deployment-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/05-workloads/03-statefulset-troubleshooting.md|03-statefulset-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/05-workloads/04-daemonset-troubleshooting.md|04-daemonset-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/05-workloads/05-job-cronjob-troubleshooting.md|05-job-cronjob-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/05-workloads/06-configmap-secret-troubleshooting.md|06-configmap-secret-troubleshooting]]

### 06 Security Auth

- [[_archives/troubleshooting-diagnostics/高级排障/06-security-auth/01-rbac-troubleshooting.md|01-rbac-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/06-security-auth/02-certificate-troubleshooting.md|02-certificate-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/06-security-auth/03-pod-security-troubleshooting.md|03-pod-security-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/06-security-auth/04-audit-logging-troubleshooting.md|04-audit-logging-troubleshooting]]

### 07 Resources Scheduling

- [[_archives/troubleshooting-diagnostics/高级排障/07-resources-scheduling/01-resources-quota-troubleshooting.md|01-resources-quota-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|02-autoscaling-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|03-cluster-autoscaler-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/07-resources-scheduling/04-pdb-troubleshooting.md|04-pdb-troubleshooting]]

### 08 Cluster Operations

- [[_archives/troubleshooting-diagnostics/高级排障/08-cluster-operations/01-cluster-maintenance-troubleshooting.md|01-cluster-maintenance-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/08-cluster-operations/02-logging-monitoring-troubleshooting.md|02-logging-monitoring-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/08-cluster-operations/03-helm-troubleshooting.md|03-helm-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md|04-ha-disaster-recovery-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/08-cluster-operations/05-crd-operator-troubleshooting.md|05-crd-operator-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/08-cluster-operations/06-kustomize-troubleshooting.md|06-kustomize-troubleshooting]]

### 09 Cloud Provider

- [[_archives/troubleshooting-diagnostics/高级排障/09-cloud-provider/01-cloud-provider-integration-troubleshooting.md|01-cloud-provider-integration-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/09-cloud-provider/02-multi-cloud-networking-troubleshooting.md|02-multi-cloud-networking-troubleshooting]]
- [[_archives/troubleshooting-diagnostics/高级排障/09-cloud-provider/03-cloud-resource-quota-troubleshooting.md|03-cloud-resource-quota-troubleshooting]]

### 09 Command Output

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-command-output/00-command-output-root-cause-parser|00-command-output-root-cause-parser]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-command-output/01-kubectl-watch-output-parser|01-kubectl-watch-output-parser]]

### 10 Ai Ml Workloads

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|01-ai-ml-workloads-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/02-kubeflow-troubleshooting|02-kubeflow-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/03-mpi-operator-troubleshooting|03-mpi-operator-troubleshooting]]

### 11 Gitops Devops

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/01-gitops-devops-troubleshooting|01-gitops-devops-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/02-tekton-troubleshooting|02-tekton-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/03-flux-image-automation-troubleshooting|03-flux-image-automation-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/04-backup-restore-troubleshooting|04-backup-restore-troubleshooting]]

### 12 Monitoring Observability

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/01-monitoring-observability-troubleshooting|01-monitoring-observability-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/02-opentelemetry-troubleshooting|02-opentelemetry-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/03-ebpf-observability-troubleshooting|03-ebpf-observability-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/04-finops-cost-optimization-troubleshooting|04-finops-cost-optimization-troubleshooting]]



<!-- risk-assessed -->
