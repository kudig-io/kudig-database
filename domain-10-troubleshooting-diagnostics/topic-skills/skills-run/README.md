---
title: Skills Demo — 本地运行工单诊断技能
description: '| 04 | DNS 解析问题 | SKILL-NET-001 | CoreDNS 缩容 | LOW |'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- coredns
- docker
- hpa
- rbac
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Skills Demo — 本地运行工单诊断技能 是什么
- 如何 Skills Demo — 本地运行工单诊断技能
trigger_keywords:
- Skills
- Demo
- 本地运行工单诊断技能
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
skill_id: SKILL-README-001
skill_name: Skills Demo — 本地运行工单诊断技能
version: 1.0.0
created: "2026-05-23"
---

# Skills Demo — 本地运行工单诊断技能

> **目的**: 在本地 Kind 集群中实际运行 [[SKILL|Skill]] 执行闭环，体验从故障注入到修复验证的完整流程  
> **场景数量**: 10 个 (5 个基础 + 5 个扩展)  
> **耗时**: 环境搭建 ~5min + 每个场景 ~5min  
> **前置条件**: Docker Desktop + kind + kubectl

---

## 快速开始

```bash
# 1. 进入 demo 目录
cd domain-10-troubleshooting-diagnostics/topic-skills/skills-run

# 2. 创建本地多节点 Kind 集群 (1 control-plane + 2 workers)
bash setup-kind-cluster.sh

# 3. 运行交互式 demo（菜单选择场景）
bash run-skill-demo.sh

# 4. 或直接运行单个场景
bash run-skill-demo.sh 1    # 节点 Cordon
bash run-skill-demo.sh 2    # Pod CrashLoop
bash run-skill-demo.sh 3    # Pod Pending
bash run-skill-demo.sh 4    # DNS 问题
bash run-skill-demo.sh 5    # Service 无 Endpoints
bash run-skill-demo.sh 6    # PVC Pending
bash run-skill-demo.sh 7    # Deployment 卡住
bash run-skill-demo.sh 8    # RBAC 拒绝
bash run-skill-demo.sh 9    # HPA 不触发
bash run-skill-demo.sh 10   # 镜像拉取失败

# 5. 运行完毕后清理
bash teardown.sh
```

---

## 场景列表

### 基础场景 (1-5)

| # | 场景 | 对应 Skill | 根因 | 风险等级 |
|---|------|-----------|------|----------|
| 01 | 节点被 cordon | SKILL-NODE-001 | RC-012 | LOW |
| 02 | Pod CrashLoopBackOff | SKILL-POD-001 | 启动命令错误 | LOW |
| 03 | Pod Pending | SKILL-POD-002 | 资源超限 | LOW |
| 04 | DNS 解析问题 | SKILL-NET-001 | [[CoreDNS|CoreDNS]] 缩容 | LOW |
| 05 | Service 无 Endpoints | SKILL-NET-002 | Selector 不匹配 | LOW |

### 扩展场景 (6-10)

| # | 场景 | 对应 Skill | 根因 | 风险等级 |
|---|------|-----------|------|----------|
| 06 | PVC Pending | SKILL-STORE-001 | RC-001 (StorageClass 不存在) | LOW |
| 07 | Deployment rollout 卡住 | SKILL-WORK-001 | RC-002 (readinessProbe 失败) | LOW |
| 08 | RBAC 权限拒绝 | SKILL-SEC-002 | RC-001 (缺少 RBAC 权限) | MEDIUM |
| 09 | HPA 不触发扩容 | SKILL-SCALE-001 | RC-002 (未设置 resources.requests) | MEDIUM |
| 10 | 镜像拉取失败 | SKILL-IMAGE-001 | RC-001 (镜像不存在) | LOW |

---

## 每个场景的执行流程

每个 demo 场景严格按照 [skill-schema.md](../skill-schema.md) 定义的 Skill 执行闭环运行:

```
Phase 0: 故障注入 (Fault Injection)
    │
    ▼
Phase 1: 症状检测 (Symptom Detection)     ← Skill Section 2
    │   匹配 trigger_keywords / trigger_events
    │   置信度评估
    ▼
Phase 2: 快速分级 (Quick Triage)           ← Skill Section 3
    │   T1-T3 影响评估
    │   P0-P3 严重性分级
    ▼
Phase 3: 诊断工作流 (Diagnostic Workflow)  ← Skill Section 4
    │   D1.x 快速检查 (kubectl, 只读)
    │   证据收集与分析
    ▼
Phase 4: 根因确认 (Root Cause)             ← Skill Section 5
    │   匹配 root-cause-map.yaml
    │   FTA 映射确认
    ▼
Phase 5: 修复操作 (Remediation)            ← Skill Section 6
    │   前置检查 → 执行 → 后置验证
    │   风险门控 (LOW → MEDIUM → HIGH → CRITICAL)
    ▼
Phase 6: 验证确认 (Verification)           ← Skill Section 7
    │   V1-V4 即时验证
    └── ✅ 完成
```

---

## 目录结构

```
skills-run/
├── README.md                              # 本文件
├── setup-kind-cluster.sh                  # 创建多节点 Kind 集群
├── run-skill-demo.sh                      # 交互式 demo 运行器
├── teardown.sh                            # 清理集群
├── scenarios/
│   ├── 01-node-cordon-notready.sh        # SKILL-NODE-001 / RC-012
│   ├── 02-pod-crashloop.sh               # SKILL-POD-001
│   ├── 03-pod-pending.sh                 # SKILL-POD-002
│   ├── 04-dns-failure.sh                 # SKILL-NET-001
│   ├── 05-service-no-endpoints.sh        # SKILL-NET-002
│   ├── 06-pvc-pending.sh                 # SKILL-STORE-001 / RC-001
│   ├── 07-deployment-stuck.sh            # SKILL-WORK-001 / RC-002
│   ├── 08-rbac-denied.sh                 # SKILL-SEC-002 / RC-001
│   ├── 09-hpa-not-scaling.sh             # SKILL-SCALE-001 / RC-002
│   └── 10-image-pull-failure.sh          # SKILL-IMAGE-001 / RC-001
└── manifests/                             # (预留) YAML 清单
```

---

## 前置环境安装

### macOS (Homebrew)

```bash
# Docker Desktop
# 从 https://www.docker.com/products/docker-desktop/ 下载安装

# kind
brew install kind

# kubectl
brew install kubectl

# 验证
docker version
kind version
kubectl version --client
```

### Linux

```bash
# kind
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.25.0/kind-linux-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# kubectl
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
chmod +x kubectl
sudo mv kubectl /usr/local/bin/kubectl
```

---

## 与 Skill 文档的对应关系

每个 demo 场景的每一步都直接映射到 Skill 文档的章节:

| Demo Phase | Skill Schema Section | 关键文件 |
|-----------|---------------------|---------|
| Phase 0: 故障注入 | — (demo 特有) | scenarios/*.sh |
| Phase 1: 症状检测 | Section 2: 症状识别 | symptom-patterns.yaml |
| Phase 2: 快速分级 | Section 3: 快速分级 | SKILL.md |
| Phase 3: 诊断工作流 | Section 4: 诊断工作流 | diagnostic-workflow.md |
| Phase 4: 根因确认 | Section 5: 根因分类 | root-cause-map.yaml |
| Phase 5: 修复操作 | Section 6: 修复操作 | remediation-playbook.md |
| Phase 6: 验证确认 | Section 7: 验证确认 | verify-node.sh |

---

## 自定义与扩展

### 添加新场景

1. 在 `scenarios/` 下创建 `11-xxx.sh` (双位数编号)
2. 遵循现有场景的 6-Phase 结构
3. 在 `run-skill-demo.sh` 的菜单中添加入口
4. 更新本 README 的场景列表

### 修改集群配置

编辑 `setup-kind-cluster.sh` 中的 Kind 配置:
- 调整节点数量
- 修改 Kubernetes 版本 (`KIND_IMAGE`)
- 添加额外端口映射

### 环境变量

| 变量 | 默认值 | 说明 |
|------|-------|------|
| `CLUSTER_NAME` | `skill-demo` | Kind 集群名称 |
| `KIND_IMAGE` | `kindest/node:v1.31.4` | Kind 节点镜像 |

---

## 硬件资源要求

运行本 Demo 需要以下最低配置：

| 资源 | 最低要求 | 推荐配置 |
|------|---------|---------|
| CPU | 4 核 | 6 核 |
| 内存 | 8 GB | 12 GB |
| 磁盘 | 20 GB 可用空间 | 40 GB |
| Docker Desktop | 已安装并运行 | 已安装并运行 |

> **注意**: macOS Apple Silicon (M1/M2/M3) 用户需使用 `kindest/node` 的 arm64 镜像，或通过 Rosetta 运行 amd64 镜像。Kind v0.20+ 自动处理架构选择。

---

## 故障排查

### Kind 集群创建失败

**症状**: `kind create cluster` 报错或超时

**排查步骤**:
1. 检查 Docker 是否运行: `docker info`
2. 检查可用内存: Docker Desktop Preferences → Resources → Memory (建议 >= 8GB)
3. 检查端口冲突: `lsof -i :30000` 和 `lsof -i :30001`
4. 清理旧集群: `kind delete cluster --name skill-demo`
5. 手动重试并增加超时: `kind create cluster --config <config> --wait 300s`

### 节点长时间不 Ready

**症状**: `kubectl get nodes` 显示 NotReady

**排查步骤**:
1. 检查 Kind 容器状态: `docker ps -a | grep kind`
2. 查看 kubelet 日志: `docker exec kind-control-plane journalctl -u kubelet -n 50`
3. 检查 Docker 资源限制: 确保分配给 Docker 的内存 >= 6GB
4. 重启 Docker Desktop 后重试

### 场景脚本运行失败

**症状**: 场景脚本报错或中途退出

**排查步骤**:
1. 确认 kubectl 版本: `kubectl version --client` (建议 v1.28+)
2. 确认 context 正确: `kubectl config current-context` 应显示 `kind-skill-demo`
3. 检查 namespace 存在: `kubectl get namespace skill-demo`
4. 查看具体错误后手动清理: `kubectl delete namespace skill-demo` 然后重新运行

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
