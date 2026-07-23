---
title: Skills & Training
description: 技能知识域 — FTA 故障树索引、故障诊断技能体系、培训路径、最佳实践
summary: 技能知识域入口，涵盖 FTA 故障树分析索引、故障诊断技能树、培训体系、运维最佳实践
category: domain
tags:
- skills
- training
- fta
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

---

## 目录结构

```
技能/
├── index.md                     # 完整索引（含工单TOP/最佳实践TOP/产品高频标记）
├── README.md                    # 本文件
│
├── 故障诊断-工作负载/            # Pod/Deployment/StatefulSet/DaemonSet/Job/HPA
├── 故障诊断-网络/                # DNS/Service/Ingress/CNI/Gateway API/Mesh
├── 故障诊断-控制面/              # APIServer/Scheduler/KCM/etcd/CRD
├── 故障诊断-存储/                # CSI/PV/PVC
├── 故障诊断-安全/                # RBAC/证书/Webhook/PSA/Quota
├── 故障诊断-节点/                # Node/NodePool/GPU
├── 故障诊断-可观测性/            # 监控/日志/链路
├── 故障诊断-集群运维/            # 升级/Autoscaler/CloudProvider/GitOps/Helm
│
├── 排障实战/                     # ts-* 命令输出解读式排障（13 个子域）
├── 运维操作/                     # 可执行操作技能（探针/节点维护/发布/kubeadm）
├── fta-方法论/                   # FTA 方法论、诊断引擎、症状匹配
├── skill-参考资料/               # 诊断工作流/修复手册/根因目录/版本矩阵
│
├── 最佳实践指南/                 # k8s-*-guide 系列（11 个配置指南）
├── best-practices/              # 生产运维最佳实践合集（部署/迁移/场景）
├── 技能建设最佳实践/             # 技能文件编写规范
│
├── 培训学习/                     # 学习路径/OnCall培训/讲师/公开课
├── 能力评估/                     # 测验/考核
├── agent-编排/                   # Agent 编排模式/规格/提示词
└── skill-k8s-node-notready/     # Node NotReady 完整诊断技能
```

---

## 按领域分类索引

### 1. 核心工作负载

| 文件夹 | 覆盖范围 |
|:---|:---|
| [故障诊断-工作负载/pod/](故障诊断-工作负载/pod/) | Pod 全生命周期异常（~80 底事件） |
| [故障诊断-工作负载/deployment/](故障诊断-工作负载/deployment/) | Deployment 滚动更新/副本/选择器/发布策略 |
| [故障诊断-工作负载/statefulset/](故障诊断-工作负载/statefulset/) | StatefulSet 有序部署/持久卷/网络标识 |
| [故障诊断-工作负载/daemonset/](故障诊断-工作负载/daemonset/) | DaemonSet 节点调度/污点/滚动更新 |
| [故障诊断-工作负载/job-cronjob/](故障诊断-工作负载/job-cronjob/) | Job/CronJob 调度/并发/超时 |
| [故障诊断-工作负载/hpa-vpa/](故障诊断-工作负载/hpa-vpa/) | HPA/VPA/PDB 弹性伸缩 |

### 2. 网络与流量

| 文件夹 | 覆盖范围 |
|:---|:---|
| [故障诊断-网络/dns/](故障诊断-网络/dns/) | CoreDNS/集群DNS/外部解析/缓存 |
| [故障诊断-网络/service/](故障诊断-网络/service/) | Service/Endpoints/kube-proxy/负载均衡 |
| [故障诊断-网络/ingress/](故障诊断-网络/ingress/) | Ingress/Nginx/Higress/TLS/路由 |
| [故障诊断-网络/cni/](故障诊断-网络/cni/) | Terway/Calico/Cilium/Flannel |
| [故障诊断-网络/gateway-api/](故障诊断-网络/gateway-api/) | Gateway API/HTTPRoute/GRPCRoute |
| [故障诊断-网络/service-mesh/](故障诊断-网络/service-mesh/) | Istio Sidecar/流量管理/mTLS |

### 3. 控制面组件

| 文件夹 | 覆盖范围 |
|:---|:---|
| [故障诊断-控制面/apiserver/](故障诊断-控制面/apiserver/) | 认证/授权/准入/etcd连接/限流 |
| [故障诊断-控制面/scheduler/](故障诊断-控制面/scheduler/) | 过滤/打分/抢占/亲和性/扩展点 |
| [故障诊断-控制面/controller-manager/](故障诊断-控制面/controller-manager/) | Leader选举/控制器/同步/限速 |
| [故障诊断-控制面/etcd/](故障诊断-控制面/etcd/) | Raft/存储/快照/备份恢复 |

### 4. 存储 / 安全 / 节点

| 文件夹 | 覆盖范围 |
|:---|:---|
| [故障诊断-存储/csi-storage/](故障诊断-存储/csi-storage/) | CSI/PV/PVC/挂载/后端 |
| [故障诊断-安全/rbac/](故障诊断-安全/rbac/) | RBAC 权限/审计 |
| [故障诊断-安全/certificate/](故障诊断-安全/certificate/) | 证书签发/轮换/过期 |
| [故障诊断-节点/node/](故障诊断-节点/node/) | NotReady/资源压力/kubelet |

---

## 技能域使用指南

### 学习路径

| 阶段 | 内容 | 前置要求 |
|------|------|----------|
| L1 基础 | [培训学习/learning-path/](培训学习/learning-path/) — 15 课入门 | Linux 基础 |
| L2 进阶 | [排障实战/](排障实战/) — 命令输出解读式排障 | L1 + K8s 架构 |
| L3 高级 | [fta-方法论/](fta-方法论/) + [最佳实践指南/](最佳实践指南/) | L2 + 生产经验 |
| L4 专家 | [agent-编排/](agent-编排/) + 架构设计 | L3 + 多领域经验 |

### 文件分类统计

| 类型 | 目录 | 数量 | 用途 |
|------|------|------|------|
| FTA 故障树 | 故障诊断-*/ | 36 | 结构化故障诊断 |
| 排障实战 | 排障实战/ | 15 | 命令输出解读 |
| 操作技能 | 运维操作/ | 7 | 可执行运维操作 |
| 最佳实践 | 最佳实践指南/ + best-practices/ | 30+ | 配置和操作指南 |
| 培训材料 | 培训学习/ | 25+ | 培训课程 |
| 评估考核 | 能力评估/ | 4 | 能力评估 |

### 常用命令速查

```bash
# Pod 故障排查
kubectl get pods -A --field-selector=status.phase!=Running
kubectl describe pod <name> -n <ns>
kubectl logs <pod> --previous -n <ns>

# 节点故障排查
kubectl get nodes -o wide
kubectl describe node <name>
journalctl -u kubelet --since "10min ago"

# 网络诊断
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
