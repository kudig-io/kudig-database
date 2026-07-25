---
title: KUDIG-DATABASE Manpages
description: '├── man8/           # 系统管理命令 (System Administration)'
summary: '├── man8/           # 系统管理命令 (System Administration)'
category: general
tags:
- k8s
- etcd
- prometheus
- istio
- cilium
- helm
- argocd
- containerd
- ebpf
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG-DATABASE Manpages 是什么
- 如何 KUDIG-DATABASE Manpages
trigger_keywords:
- KUDIG-DATABASE
- Manpages
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- gpu-scheduling-basics
- tls-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG-DATABASE Manpages

> 核心开源产品和项目脚本的 manpage 参考文档

## 目录结构

```
# 🟢 低风险：只读/信息收集，通常无副作用
man/
├── man1/           # 用户命令 (User Commands)
│   ├── kudig-stats.1
│   ├── kudig-quality.1
│   ├── kudig-validate.1
│   └── kudig-fta-viz.1
├── man8/           # 系统管理命令 (System Administration)
│   ├── kubernetes.8
│   ├── prometheus.8
│   ├── etcd.8
│   ├── containerd.8
│   ├── cilium.8
│   ├── helm.8
│   ├── argocd.8
│   ├── istio.8
│   ├── velero.8
│   └── cert-manager.8
└── README.md       # 本文件
```
## 使用方法

### 方式一：直接查看

```bash
# 查看项目脚本帮助
man ./man/man1/kudig-stats.1
man ./man/man1/kudig-quality.1

# 查看核心开源产品帮助
man ./man/man8/kubernetes.8
man ./man/man8/prometheus.8
```

### 方式二：安装到系统

#### Linux

```bash
# 复制到系统 man 目录
sudo cp -r man/man1/* /usr/local/share/man/man1/
sudo cp -r man/man8/* /usr/local/share/man/man8/

# 更新 man 数据库
sudo mandb

# 现在可以直接使用
man kudig-stats
man kubernetes
```

#### macOS

```bash
# 复制到系统 man 目录
sudo cp -r man/man1/* /usr/local/share/man/man1/
sudo cp -r man/man8/* /usr/local/share/man/man8/

# 使用
man kudig-stats
man kubernetes
```

### 方式三：添加到 MANPATH

```bash
# 临时添加（当前会话）
export MANPATH="$MANPATH:$(pwd)/man"
man kudig-stats

# 永久添加（添加到 ~/.bashrc 或 ~/.zshrc）
echo 'export MANPATH="$MANPATH:/path/to/kudig-database/man"' >> ~/.bashrc
```

## Manpage 清单

### Section 1 - 用户命令 (KUDIG 项目脚本)

| 命令 | 描述 | 对应脚本 |
|:---|:---|:---|
| `kudig-stats` | README 数字指标自动统计 | `scripts/generate-readme-stats.sh` |
| `kudig-quality` | 知识库全面质量检查 | `scripts/comprehensive-quality-check.sh` |
| `kudig-validate` | 代码示例语法校验 | `scripts/code-example-validation.sh` |
| `kudig-fta-viz` | FTA 故障树可视化 | `scripts/fta_tree_visualization.py` |

### Section 8 - 系统管理 (CNCF 核心开源产品)

| 产品 | 描述 | 文档位置 |
|:---|:---|:---|
| `kubernetes` | 容器编排平台 | `集群基础/` |
| `prometheus` | 监控和告警系统 | `可观测性/` |
| `etcd` | 分布式键值存储 | `集群基础/` |
| `containerd` | 容器运行时 | `集群基础/` |
| `cilium` | eBPF 网络和安全 | `网络/` |
| `helm` | Kubernetes 包管理器 | `专项技术/` |
| `argocd` | GitOps 持续交付 | `平台工程/` |
| `istio` | 服务网格平台 | `网络/` |
| `velero` | 备份和灾难恢复 | `可靠性/` |
| `cert-manager` | 证书管理自动化 | `平台工程/` |

## 文档标准

本项目的 manpage 遵循以下标准：

1. **格式标准**: 使用传统 Unix man 宏格式 (man 7 man)
2. **章节结构**:
   - NAME - 名称和简要描述
   - SYNOPSIS - 命令语法
   - DESCRIPTION - 详细描述
   - OPTIONS - 命令选项
   - EXAMPLES - 使用示例
   - SEE ALSO - 相关文档
   - AUTHOR - 作者信息
   - COPYRIGHT - 许可证信息

3. **交叉引用**: 每个 manpage 都链接到 KUDIG-DATABASE 的相关文档

## 更新和维护

当添加新的核心开源产品或项目脚本时，请同步创建对应的 manpage：

```bash
# 创建新的 manpage
touch man/man1/<command>.1   # 用户命令
touch man/man8/<product>.8   # 系统管理命令
```

使用现有的 manpage 作为模板，确保格式一致性。

## 故障排查

### man 命令找不到页面

```bash
# 检查文件是否存在
ls -la man/man1/kudig-stats.1

# 检查 MANPATH
man -w kudig-stats

# 手动指定路径
man ./man/man1/kudig-stats.1
```

### 格式显示问题

```bash
# 确保使用正确的编码
export LC_ALL=en_US.UTF-8
man ./man/man1/kudig-stats.1
```

## 相关资源

- [KUDIG-DATABASE 主文档](../README.md)
- [项目脚本](../脚本/README.md)
- [CNCF 项目库](../生态参考/)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[10-平台工程/02-运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[23-实体/15-参考与索引/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
