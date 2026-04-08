# FTA 故障树清单索引

> **文档数量**: 36 个故障树 | **总大小**: ~1.2 MB | **最后更新**: 2026-03-02

---

## 概述

本目录包含 Kubernetes 生产环境各组件的故障树分析（FTA）文档。每个 FTA 文件提供：
- 完整的 Mermaid 故障树图（OR/AND 门结构）
- 底事件详细定义（severity/probability/MTTR/detection/remediation）
- JSON 工作流（支持 Agent 自动化遍历）
- K8s 版本兼容说明（1.19–1.30）

---

## 文件大小分布

| 分类 | 文件数 | 大小范围 |
|:---|:---:|:---|
| 大型 (>40 KB) | 8 | 44.0 KB – 58.8 KB |
| 中型 (25–40 KB) | 15 | 25.9 KB – 38.9 KB |
| 标准 (20–25 KB) | 9 | 20.3 KB – 24.9 KB |
| 紧凑 (<20 KB) | 4 | 14.8 KB – 18.9 KB |

---

## 按领域分类索引

### 1. 核心工作负载

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [pod-fta.md](pod-fta.md) | 58.8 KB | Pod 全生命周期异常（调度/镜像/运行时/健康检查/网络/存储/安全/节点/控制面） | ~80 |
| [deployment-fta.md](deployment-fta.md) | 21.4 KB | Deployment 滚动更新/副本管理/选择器/镜像拉取 | ~25 |
| [statefulset-fta.md](statefulset-fta.md) | 20.8 KB | StatefulSet 有序部署/持久卷/网络标识/扩缩容 | ~24 |
| [daemonset-fta.md](daemonset-fta.md) | 29.9 KB | DaemonSet 节点调度/污点容忍/滚动更新/资源竞争 | ~35 |
| [job-cronjob-fta.md](job-cronjob-fta.md) | 28.8 KB | Job/CronJob 调度/并发/完成策略/超时/时区 | ~32 |

### 2. 网络与流量

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [dns-fta.md](dns-fta.md) | 24.2 KB | CoreDNS/集群 DNS/外部 DNS 解析/缓存/NXDOMAIN | ~28 |
| [service-fta.md](service-fta.md) | 25.9 KB | Service 类型/Endpoints/kube-proxy/负载均衡/会话亲和 | ~30 |
| [ingress-fta.md](ingress-fta.md) | 26.3 KB | Ingress Controller/TLS 终止/路由/后端健康/注解 | ~30 |
| [networkpolicy-fta.md](networkpolicy-fta.md) | 21.7 KB | NetworkPolicy 入站/出站/选择器/CNI 支持/调试 | ~25 |
| [gateway-api-fta.md](gateway-api-fta.md) | 24.1 KB | Gateway API/HTTPRoute/GRPCRoute/TLSRoute/ReferenceGrant | ~28 |
| [terway-fta.md](terway-fta.md) | 16.8 KB | Terway ENI/IP 池/VPC 路由/安全组/控制面依赖 | ~20 |

### 3. 控制面组件

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [apiserver-fta.md](apiserver-fta.md) | 36.1 KB | API Server 认证/授权/准入/etcd 连接/限流/审计 | ~42 |
| [scheduler-fta.md](scheduler-fta.md) | 30.3 KB | Scheduler 过滤/打分/抢占/亲和性/资源/扩展点 | ~35 |
| [controller-manager-fta.md](controller-manager-fta.md) | 29.4 KB | Controller Manager Leader 选举/控制器/同步/限速 | ~34 |
| [etcd-fta.md](etcd-fta.md) | 27.4 KB | etcd 集群/Raft/存储/快照/认证/性能 | ~32 |

### 4. 存储

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [csi-fta.md](csi-fta.md) | 18.9 KB | CSI Controller/Node Plugin/卷挂载/性能/认证/后端 | ~22 |

### 5. 安全与准入

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [rbac-fta.md](rbac-fta.md) | 24.2 KB | RBAC Role/ClusterRole/Binding/ServiceAccount/权限不足 | ~28 |
| [certificate-fta.md](certificate-fta.md) | 52.6 KB | 证书签发/轮换/过期/CA 链/cert-manager/TLS | ~60 |
| [webhook-admission-fta.md](webhook-admission-fta.md) | 50.5 KB | Webhook 超时/TLS/失败策略/副作用/匹配规则 | ~58 |
| [psp-scc-fta.md](psp-scc-fta.md) | 44.0 KB | PSP/SCC/PSA 策略迁移/安全上下文/特权容器 | ~50 |
| [resource-quota-fta.md](resource-quota-fta.md) | 38.9 KB | ResourceQuota/LimitRange/配额计算/命名空间限制 | ~45 |

### 6. 节点与基础设施

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [node-fta.md](node-fta.md) | 27.4 KB | 节点状态/kubelet/容器运行时/磁盘/内存/网络 | ~32 |
| [nodepool-fta.md](nodepool-fta.md) | 20.3 KB | 节点池扩缩/标签/污点/机器配置/云提供商 | ~24 |
| [gpu-fta.md](gpu-fta.md) | 31.3 KB | GPU 驱动/设备插件/调度/CUDA/内存/多卡 | ~36 |

### 7. 扩缩容与可用性

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [hpa-fta.md](hpa-fta.md) | 24.9 KB | HPA 指标/扩缩算法/冷却/自定义指标/稳定窗口 | ~29 |
| [vpa-fta.md](vpa-fta.md) | 29.8 KB | VPA 推荐/更新模式/OOM 保护/资源请求 | ~34 |
| [pdb-fta.md](pdb-fta.md) | 28.3 KB | PDB 中断预算/驱逐保护/更新阻塞/选择器 | ~32 |
| [cluster-autoscaler-fta.md](cluster-autoscaler-fta.md) | 49.5 KB | CA 扩容/缩容/节点组/优先级/安全缩容 | ~56 |

### 8. 集群运维

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [cluster-upgrade-fta.md](cluster-upgrade-fta.md) | 52.0 KB | 升级前检查/控制面/工作节点/API 废弃/回滚 | ~60 |
| [backup-restore-fta.md](backup-restore-fta.md) | 33.4 KB | etcd 备份/Velero/恢复验证/数据一致性 | ~38 |

### 9. 应用交付与扩展

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [helm-fta.md](helm-fta.md) | 14.8 KB | Helm Chart/Hook/API 兼容/RBAC/状态/回滚 | ~18 |
| [crd-operator-fta.md](crd-operator-fta.md) | 35.9 KB | CRD 定义/Operator/控制器/Finalizer/版本转换 | ~42 |
| [gitops-argocd-fta.md](gitops-argocd-fta.md) | 30.4 KB | ArgoCD 同步/漂移/Git 仓库/健康检查/RBAC | ~35 |

### 10. 可观测性与平台

| 文件 | 大小 | 覆盖范围 | 底事件数 |
|:---|---:|:---|:---:|
| [monitoring-fta.md](monitoring-fta.md) | 34.0 KB | Prometheus/Alertmanager/Grafana/指标/告警 | ~40 |
| [service-mesh-istio-fta.md](service-mesh-istio-fta.md) | 32.8 KB | Istio Sidecar/流量/mTLS/控制面/配置下发 | ~38 |
| [cloud-provider-fta.md](cloud-provider-fta.md) | 28.4 KB | 云控制器/负载均衡/存储/IAM/API 限流 | ~32 |

---

## 文件完整性评估

### 大型文件 (>40 KB) - 覆盖全面

| 文件 | 状态 | 说明 |
|:---|:---:|:---|
| pod-fta.md | ✅ | 13 个主类别，80+ 底事件，作为参考基准 |
| certificate-fta.md | ✅ | 证书全生命周期，含 cert-manager 集成 |
| cluster-upgrade-fta.md | ✅ | 升级全流程，多版本兼容矩阵 |
| webhook-admission-fta.md | ✅ | Webhook 全场景覆盖，含调试指南 |
| cluster-autoscaler-fta.md | ✅ | CA 扩缩容全路径，云提供商特定场景 |
| psp-scc-fta.md | ✅ | PSP→PSA 迁移，多版本兼容 |
| resource-quota-fta.md | ✅ | 配额计算全场景，LimitRange 集成 |
| apiserver-fta.md | ✅ | API Server 全链路，认证/授权/准入 |

### 紧凑型文件 (<20 KB) - 评估结论

| 文件 | 大小 | 状态 | 评估说明 |
|:---|---:|:---:|:---|
| helm-fta.md | 14.8 KB | ✅ | 领域聚焦（Helm 工具），5 主类别足够覆盖 Chart/Hook/API/RBAC/State |
| terway-fta.md | 16.8 KB | ✅ | 阿里云特定 CNI，6 主类别覆盖 ENI/IP/CNI/网络/安全组/控制面 |
| csi-fta.md | 18.9 KB | ✅ | CSI 接口层，6 主类别覆盖 Controller/Node/Volume/性能/认证/后端 |

**评估结论**：这三个文件的大小与其领域范围匹配。它们覆盖了各自领域的主要故障模式，不需要强制扩展。Pod FTA 作为最大文件是因为 Pod 是 K8s 最基础的抽象，涉及面最广。

---

## 优先级建议

### 生产环境必读 (P0)

1. **[pod-fta.md](pod-fta.md)** - Pod 是所有工作负载的基础
2. **[node-fta.md](node-fta.md)** - 节点异常影响范围大
3. **[apiserver-fta.md](apiserver-fta.md)** - 控制面核心
4. **[dns-fta.md](dns-fta.md)** - DNS 是最常见的网络故障源

### 日常运维高频 (P1)

5. **[deployment-fta.md](deployment-fta.md)** - 最常用的工作负载类型
6. **[service-fta.md](service-fta.md)** - 服务发现与负载均衡
7. **[ingress-fta.md](ingress-fta.md)** - 外部流量入口
8. **[hpa-fta.md](hpa-fta.md)** - 自动扩缩容问题
9. **[certificate-fta.md](certificate-fta.md)** - TLS 证书问题频发

### 进阶场景 (P2)

10. **[cluster-upgrade-fta.md](cluster-upgrade-fta.md)** - 升级前必读
11. **[webhook-admission-fta.md](webhook-admission-fta.md)** - 准入控制调试
12. **[service-mesh-istio-fta.md](service-mesh-istio-fta.md)** - Service Mesh 环境
13. **[csi-fta.md](csi-fta.md)** - 存储问题排查
14. **[helm-fta.md](helm-fta.md)** - Helm 部署问题

---

## 快速查找

### 按故障现象

| 现象 | 相关 FTA |
|:---|:---|
| Pod Pending/调度失败 | pod-fta, scheduler-fta, node-fta |
| Pod CrashLoopBackOff | pod-fta, deployment-fta |
| 网络不通/超时 | dns-fta, service-fta, networkpolicy-fta, terway-fta |
| 证书过期/TLS 错误 | certificate-fta, webhook-admission-fta |
| 存储挂载失败 | csi-fta, pod-fta |
| HPA 不生效 | hpa-fta, monitoring-fta |
| 升级失败/回滚 | cluster-upgrade-fta, helm-fta |
| 权限拒绝 | rbac-fta, psp-scc-fta |

### 按组件

| 组件 | 相关 FTA |
|:---|:---|
| CoreDNS | dns-fta |
| kube-proxy | service-fta |
| Nginx Ingress | ingress-fta |
| cert-manager | certificate-fta |
| Prometheus | monitoring-fta |
| Istio | service-mesh-istio-fta |
| ArgoCD | gitops-argocd-fta |
| Velero | backup-restore-fta |

---

## 使用指南

### 人工排查

1. 根据故障现象找到对应 FTA 文件
2. 查看 Mermaid 图了解故障树结构
3. 按 OR/AND 门逻辑逐层排查
4. 参考底事件的 detection（检测方法）和 remediation（修复步骤）

### Agent 自动化

1. 解析 JSON 工作流中的 `flow_steps`
2. 根据 `gate_type` 决定遍历策略（OR=并行探测，AND=全条件检查）
3. 使用 `detection.events/metrics/logs` 进行证据收集
4. 执行 `remediation.auto_actions` 或展示 `manual_steps`

---

## 统计摘要

| 指标 | 数值 |
|:---|---:|
| FTA 文件总数 | 36 |
| 底事件总数 | ~1,200+ |
| 覆盖 K8s 组件 | 40+ |
| 支持 K8s 版本 | 1.19–1.30 |
| 最大文件 | pod-fta.md (58.8 KB) |
| 最小文件 | helm-fta.md (14.8 KB) |
| 平均文件大小 | ~30 KB |

---

## 相关资源

- [topic-fta README](../README.md) - FTA 方法论主页
- [kubernetes-fta-full-analysis.md](../kubernetes-fta-full-analysis.md) - K8s 全量故障树概览
- [23-fta-production-quick-start.md](../23-fta-production-quick-start.md) - 生产环境快速启动指南
