---
title: topic-functions MOC
description: topic-functions 专题导航页，覆盖 82 篇文档
summary: topic-functions 专题导航页，覆盖 82 篇文档
category: moc
tags:
- k8s
- moc
- reference
- etcd
- apiserver
- kubelet
- scheduler
- rbac
- webhook
- rag
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- topic-functions MOC 是什么
- 如何 topic-functions MOC
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- topic-functions
- MOC
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-functions [[MOC|MOC]]

> **[[MOC]] 版本**: 1.0
> **专题**: topic-functions
> **文档数量**: 82 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

函数 — 运维脚本常用函数库

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-functions |
| **文档数量** | 82 篇（展示前 50 篇） |
| **难度分布** | 入门 0 / 进阶 1 / 高级 6 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[10-平台工程/06-代码分析/functions-cluster-cert/01-pki-architecture.md|Kubernetes 集群 PKI 架构总览]] |  | reference, architecture |  |
| 2 | [[10-平台工程/06-代码分析/functions-cluster-cert/02-ca-generation.md|CA 证书生成源码分析]] |  | reference |  |
| 3 | [[10-平台工程/06-代码分析/functions-cluster-cert/03-apiserver-cert.md|API Server 证书生成源码分析]] |  | reference |  |
| 4 | [[10-平台工程/06-代码分析/functions-cluster-cert/04-etcd-cert.md|etcd 证书体系源码分析]] |  | reference |  |
| 5 | [[10-平台工程/06-代码分析/functions-cluster-cert/05-kubelet-cert.md|kubelet 证书与 CSR 机制源码分析]] |  | reference |  |
| 6 | [[10-平台工程/06-代码分析/functions-cluster-cert/06-cert-rotation.md|证书轮换机制源码分析]] |  | reference |  |
| 7 | [[10-平台工程/06-代码分析/functions-cluster-cert/07-service-account-keys.md|ServiceAccount 密钥对源码分析]] |  | reference |  |
| 8 | [[10-平台工程/06-代码分析/functions-cluster-cert/08-rbac-mapping.md|证书身份到 RBAC 的映射关系]] |  | reference, rbac |  |
| 9 | [[10-平台工程/06-代码分析/functions-cluster-cert/09-join-cert-flow.md|kubeadm join 证书分发流程]] |  | reference |  |
| 10 | [[10-平台工程/06-代码分析/functions-cluster-cert/10-front-proxy-workflow.md|Front Proxy 聚合层证书工作流]] |  | reference |  |
| 11 | [[10-平台工程/06-代码分析/functions-cluster-cert/11-apiserver-cert-flags.md|API Server 证书相关启动参数汇总]] |  | reference |  |
| 12 | [[10-平台工程/06-代码分析/functions-cluster-cert/12-kubeconfig-certs.md|kubeconfig 中的证书嵌入逻辑]] |  | reference, configuration |  |
| 13 | [[10-平台工程/06-代码分析/functions-cluster-cert/13-cert-config.md|kubeadm 配置对证书生成的影响]] |  | reference, configuration |  |
| 14 | [[10-平台工程/06-代码分析/functions-cluster-cert/14-admission-webhook-certs.md|Admission Webhook 证书体系]] |  | reference |  |
| 15 | [[10-平台工程/06-代码分析/functions-cluster-cert/15-cert-format-encoding.md|证书格式与编码详解]] |  | reference |  |
| 16 | [[10-平台工程/06-代码分析/functions-cluster-cert/16-openssl-cookbook.md|OpenSSL 证书操作速查手册]] |  | reference |  |
| 17 | [[10-平台工程/06-代码分析/functions-cluster-cert/17-pki-security-best-practices.md|Kubernetes PKI 安全最佳实践]] |  | reference, security, best-practice |  |
| 18 | [[10-平台工程/06-代码分析/cluster-create/01-overview.md|kubeadm init 集群初始化概览]] |  | reference, deep-dive |  |
| 19 | [[10-平台工程/06-代码分析/cluster-create/02-preflight.md|预检流程 (kubeadm preflight)]] |  | reference |  |
| 20 | [[10-平台工程/06-代码分析/cluster-create/03-certs.md|证书管理 (PKI Infrastructure)]] |  | reference |  |
| 21 | [[10-平台工程/06-代码分析/cluster-create/04-kubeconfig.md|kubeconfig 阶段 — Kubeconfig Generation 源码分析]] |  | reference, configuration |  |
| 22 | [[10-平台工程/06-代码分析/cluster-create/05-control-plane.md|控制面组件部署 (Static Pod Manifests)]] |  | reference |  |
| 23 | [[10-平台工程/06-代码分析/cluster-create/06-join.md|节点加入流程 (kubeadm join)]] |  | reference |  |
| 24 | [[10-平台工程/06-代码分析/cluster-create/07-etcd.md|etcd 静态 Pod 管理]] |  | reference |  |
| 25 | [[10-平台工程/06-代码分析/cluster-create/08-ha.md|高可用控制面搭建 — 源码分析]] |  | reference |  |
| 26 | [[10-平台工程/06-代码分析/cluster-create/09-upgrade.md|集群升级流程 (kubeadm upgrade)]] |  | reference, upgrade |  |
| 27 | [[10-平台工程/06-代码分析/cluster-create/10-cloud-comparison.md|云厂商方案与 kubeadm 对比]] |  | reference |  |
| 28 | [[10-平台工程/06-代码分析/cluster-create/11-advanced.md|集群新建进阶: 关键机制详解]] |  | reference |  |
| 29 | [[10-平台工程/06-代码分析/cluster-create/12-join-advanced.md|节点加入进阶: Discovery 与 TLS Bootstrap 详解]] |  | reference |  |
| 30 | [[10-平台工程/06-代码分析/cluster-create/13-etcd-advanced.md|etcd 进阶: HA 集群管理与性能调优]] |  | reference |  |
| 31 | [[10-平台工程/06-代码分析/cluster-create/14-ha-advanced.md|高可用进阶: 负载均衡与证书分发]] |  | reference |  |
| 32 | [[10-平台工程/06-代码分析/cluster-create/15-upgrade-advanced.md|集群升级进阶: 滚动升级与回滚策略]] |  | reference, upgrade |  |
| 33 | [[10-平台工程/06-代码分析/cluster-create/16-security.md|安全机制: ServiceAccount Token 与 Audit]] |  | reference, security |  |
| 34 | [[10-平台工程/06-代码分析/cluster-create/17-init-phases.md|init 阶段详解: mark-control-plane 与 upload-config]] |  | reference |  |
| 35 | [[10-平台工程/06-代码分析/cluster-create/18-cri-runtime.md|CRI 运行时管理 (Container Runtime Interface)]] |  | reference |  |
| 36 | [[10-平台工程/06-代码分析/cluster-create/19-cni-networking.md|CNI 网络插件与集群网络]] |  | reference, networking |  |
| 37 | [[10-平台工程/06-代码分析/cluster-create/20-node-registration.md|Node 注册与 kubeadm token 详解]] |  | reference |  |
| 38 | [[10-平台工程/06-代码分析/cluster-create/21-kube-proxy.md|kube-proxy 与 Service 负载均衡]] |  | reference |  |
| 39 | [[10-平台工程/06-代码分析/cluster-create/22-storage-volumes.md|存储与卷管理]] |  | reference, storage |  |
| 40 | [[10-平台工程/06-代码分析/cluster-create/23-scheduler.md|kube-scheduler 调度详解]] |  | reference |  |
| 41 | [[10-平台工程/06-代码分析/cluster-create/24-what-kubeadm-does-not-install.md|kubeadm 不安装的组件 (What kubeadm Does Not Install)]] |  | reference, configuration |  |
| 42 | [[10-平台工程/06-代码分析/cluster-create/25-resource-management.md|资源管理与配额控制 (Resource Management)]] |  | reference |  |
| 43 | [[10-平台工程/06-代码分析/functions-cluster-delete/01-overview.md|Kubernetes 集群删除逻辑 — 基于官方代码分析]] |  | reference, deep-dive |  |
| 44 | [[10-平台工程/06-代码分析/functions-cluster-delete/02-reset.md|kubeadm reset 源码分析]] |  | reference |  |
| 45 | [[10-平台工程/06-代码分析/functions-cluster-delete/03-delete-node.md|节点删除流程 — kubectl delete node 源码分析]] |  | reference |  |
| 46 | [[10-平台工程/06-代码分析/functions-cluster-delete/04-cleanup.md|节点清理机制 — cleanup-node 源码分析]] |  | reference |  |
| 47 | [[10-平台工程/06-代码分析/functions-cluster-delete/05-etcd-cleanup.md|etcd 数据清理与成员移除 — 源码分析]] |  | reference |  |
| 48 | [[10-平台工程/06-代码分析/functions-cluster-delete/06-force-delete.md|强制删除与异常场景处理]] |  | reference |  |
| 49 | [[10-平台工程/06-代码分析/functions-cluster-delete/07-ha-delete.md|HA 集群删除注意事项]] |  | reference |  |
| 50 | [[10-平台工程/06-代码分析/functions-cluster-delete/08-cloud-delete.md|云厂商集群删除方案对比]] |  | reference |  |
| ... | 共 82 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 82 |

---

*本文档由 scripts/generate-[[MOC]]s.py 自动生成，最后更新 2026-05-21。*

## 快速导航

### 按主题分类

| 主题 | 文档数 | 入口 |
|------|--------|------|
| 集群证书 (PKI) | 17 | [[10-平台工程/06-代码分析/functions-cluster-cert/01-pki-architecture.md|PKI 架构总览]] |
| 集群创建 (kubeadm init) | 25 | [[10-平台工程/06-代码分析/cluster-create/01-overview.md|集群初始化概览]] |
| 集群删除 (kubeadm reset) | 8 | [[10-平台工程/06-代码分析/functions-cluster-delete/01-overview.md|集群删除逻辑]] |
| 集群升级 (kubeadm upgrade) | 6 | [[10-平台工程/06-代码分析/cluster-create/09-upgrade.md|升级流程]] |
| 节点管理 | 10 | [[10-平台工程/06-代码分析/cluster-create/20-node-registration.md|Node 注册]] |
| 存储与网络 | 8 | [[10-平台工程/06-代码分析/cluster-create/19-cni-networking.md|CNI 网络]] |
| 调度与资源 | 8 | [[10-平台工程/06-代码分析/cluster-create/23-scheduler.md|调度详解]] |

### 按难度分类

| 难度 | 文档 | 适用场景 |
|------|------|----------|
| 入门 | 概览、基础配置 | 初次接触 kubeadm 源码 |
| 进阶 | 高级机制、HA、升级 | 生产环境运维 |
| 专家 | 安全、性能调优 | 平台工程师 |

### 学习路径推荐

```
新手入门:
01-overview → 02-preflight → 03-certs → 05-control-plane → 06-join

进阶运维:
08-ha → 09-upgrade → 13-etcd-advanced → 15-upgrade-advanced

安全专家:
16-security → functions-cluster-cert/17-pki-security-best-practices
```

## 常见问题

### Q: kubeadm init 做了哪些事情？

A: 主要流程：
1. **预检** (preflight) — 检查系统要求
2. **证书生成** (certs) — 创建 PKI 体系
3. **kubeconfig** — 生成各组件配置
4. **控制面** — 部署 Static Pod
5. **etcd** — 初始化 etcd 集群
6. **上传配置** — 存储集群配置到 ConfigMap

详见: [[10-平台工程/06-代码分析/cluster-create/01-overview.md|集群初始化概览]]

### Q: 如何安全地删除节点？

A: 标准流程：
```bash
# 1. 驱逐 Pod
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 2. 删除节点对象
kubectl delete node <node>

# 3. 在节点上执行 reset
kubeadm reset -f

# 4. 清理网络/存储
rm -rf /var/lib/cni /etc/cni/net.d
iptables -F && iptables -t nat -F
```

详见: [[10-平台工程/06-代码分析/functions-cluster-delete/03-delete-node.md|节点删除流程]]

### Q: 集群升级的最佳实践？

A: 滚动升级策略：
1. 先升级控制面（一个节点一个节点）
2. 再升级工作节点（分批进行）
3. 每次升级后验证集群健康
4. 保留回滚能力

详见: [[10-平台工程/06-代码分析/cluster-create/15-upgrade-advanced.md|升级进阶]]

## 相关工具

| 工具 | 用途 | 安装 |
|------|------|------|
| kubeadm | 集群生命周期管理 | `apt install kubeadm` |
| kubectl | 集群操作 | `apt install kubectl` |
| etcdctl | etcd 管理 | `apt install etcd-client` |
| crictl | 容器运行时调试 | `apt install cri-tools` |

## 源码阅读指南

### kubeadm 代码结构

```
kubernetes/cmd/kubeadm/
├── app/                    # 主入口
│   ├── cmd/               # 命令定义
│   │   ├── init.go        # kubeadm init
│   │   ├── join.go        # kubeadm join
│   │   ├── reset.go       # kubeadm reset
│   │   └── upgrade.go     # kubeadm upgrade
│   └── phases/            # 阶段实现
│       ├── preflight/     # 预检
│       ├── certs/         # 证书
│       ├── kubeconfig/    # 配置
│       ├── controlplane/  # 控制面
│       └── etcd/          # etcd
└── apis/                  # API 定义
    └── kubeadm/           # KubeadmConfig
```

### 关键函数索引

| 函数 | 文件 | 功能 |
|------|------|------|
| `RunInit` | `app/cmd/init.go` | init 主流程 |
| `RunJoin` | `app/cmd/join.go` | join 主流程 |
| `RunReset` | `app/cmd/reset.go` | reset 主流程 |
| `CreatePKIAssets` | `app/phases/certs/` | 证书生成 |
| `CreateKubeConfigFiles` | `app/phases/kubeconfig/` | kubeconfig 生成 |
| `CreateStaticPodFiles` | `app/phases/controlplane/` | Static Pod 创建 |

### 调试技巧

```bash
# 1. 查看详细日志
kubeadm init -v=5

# 2. 干跑模式（不实际执行）
kubeadm init --dry-run

# 3. 跳过预检（仅测试）
kubeadm init --ignore-preflight-errors=all

# 4. 查看生成的配置
kubectl get configmap kubeadm-config -n kube-system -o yaml

# 5. 查看集群状态
kubectl get componentstatuses
kubectl get nodes
kubectl get pods -n kube-system
```

## 版本兼容性

| kubeadm | Kubernetes | etcd | 状态 |
|---------|------------|------|------|
| v1.33 | v1.33.x | 3.5.x | 当前稳定 |
| v1.32 | v1.32.x | 3.5.x | 支持中 |
| v1.31 | v1.31.x | 3.5.x | 支持中 |
| v1.30 | v1.30.x | 3.5.x | EOL |

> **注意**: kubeadm 版本必须与目标 Kubernetes 版本匹配，不支持跨大版本升级。

## 参考资料

- [kubeadm 官方文档](https://kubernetes.io/docs/reference/setup-tools/kubeadm/)
- [kubeadm 源码](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm)
- [Kubernetes 版本发布](https://kubernetes.io/releases/)
- [etcd 运维指南](https://etcd.io/docs/v3.5/op-guide/)

## 贡献指南

### 添加新文档

1. 在对应子目录创建 Markdown 文件
2. 遵循命名规范: `NN-主题名称.md`
3. 添加完整的 frontmatter 元数据
4. 更新本 MOC 的文档清单
5. 运行 `scripts/generate-MOCs.py` 重新生成

### 文档模板

```markdown
---
title: 文档标题
description: 一句话描述
category: reference
tags:
- tag1
- tag2
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: platform-engineering
---

# 文档标题

> 一句话摘要

## 概述

...

## 详细内容

...

## Related

- [[相关文档]]
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[22-概念/07-调度与资源/resource-management.md|resource-management]]
- [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]]
- [[23-实体/02-K8s核心组件/cni.md|cni]]
- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]]

- [[MOC]]
- [[MOC]]
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 网络 MOC — Cross-reference
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[01-集群基础/05-kubectl/02-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[01-集群基础/01-架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[06-存储/01-K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[06-存储/01-K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference


<!-- risk-assessed -->
