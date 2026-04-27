---
skill_id: "DEMO-GUIDE-001"
skill_name: "Skill 本地运行 Demo 指南 / Skill Local Demo Guide"
version: "1.0"
category: "demo"
k8s_versions:
  - "1.28.x"
  - "1.29.x"
  - "1.30.x"
  - "1.31.x"
  - "1.32.x"
tested_on:
  - "1.28.15"
  - "1.29.12"
  - "1.30.8"
  - "1.31.4"
  - "1.32.0"
k8s_version_notes:
  - "v1.28+: Kind v0.20+ supports node images for v1.28+"
  - "v1.29+: Kind v0.21+ recommended"
  - "v1.30+: Kind v0.22+ recommended"
  - "v1.31+: Kind v0.24+ recommended (kindest/node:v1.31.4 default)"
  - "v1.32+: Kind v0.26+ recommended"
last_updated: "2026-04-26"
---

# Skill 本地运行 Demo 指南

> **目的**: 在本地 Kind 集群中实际运行 Skill 执行闭环，验证和体验 topic-skills 中定义的诊断-修复最佳实践  
> **受众**: 运维工程师、SRE、AI Agent 开发者、希望理解 Skill 系统的贡献者  
> **耗时**: 环境搭建 ~5 min + 每个场景 ~5 min

---

## 1. 概述

topic-skills 定义了面向 AI Agent 的 Kubernetes 故障诊断技能库（详见 [README.md](./README.md)）。本 Demo 允许你在本地 Kind 集群中**实际运行**这些 Skill 的完整执行闭环：

```
故障注入 → 症状检测 → 快速分级 → 诊断工作流 → 根因确认 → 修复操作 → 验证确认
```

每个 demo 场景**严格映射**到 Skill 文档的 10 个 Section（参见 [skill-schema.md](./skill-schema.md)），让你看到 Agent 在生产中会如何使用这些 Skill。

---

## 2. 前置条件

### 2.1 必需工具

| 工具 | 版本要求 | 安装方式 (macOS) |
|------|---------|-----------------|
| Docker Desktop | 最新版 | [官网下载](https://www.docker.com/products/docker-desktop/) |
| kind | >= v0.20 | `brew install kind` |
| kubectl | >= v1.28 | `brew install kubectl` |

### 2.2 系统资源

| 资源 | 最低要求 | 推荐 |
|------|---------|------|
| CPU | 2 核 | 4 核 |
| 内存 | 4 GB | 8 GB |
| 磁盘 | 10 GB | 20 GB |

### 2.3 验证安装

```bash
docker version           # Docker 运行中
kind version             # kind 已安装
kubectl version --client # kubectl 已安装
```

---

## 3. 快速开始

### Step 1: 创建 Kind 集群

```bash
cd topic-skills/demo
bash setup-kind-cluster.sh
```

这会创建一个 **1 control-plane + 2 worker** 的多节点 Kind 集群：

```
┌──────────────────────────────────┐
│  skill-demo cluster              │
├──────────────────────────────────┤
│  control-plane    (1 node)       │
│  worker-1         (app label)    │
│  worker-2         (app label)    │
├──────────────────────────────────┤
│  namespace: skill-demo           │
│  demo-nginx deployment (3 pods)  │
└──────────────────────────────────┘
```

### Step 2: 运行 Demo

```bash
# 交互式菜单（推荐新手）
bash run-skill-demo.sh

# 直接运行指定场景
bash run-skill-demo.sh 1    # 节点 Cordon
bash run-skill-demo.sh 2    # Pod CrashLoop
bash run-skill-demo.sh 3    # Pod Pending
bash run-skill-demo.sh 4    # DNS 故障
bash run-skill-demo.sh 5    # Service 无 Endpoints

# 顺序运行所有场景
bash run-skill-demo.sh all
```

### Step 3: 清理

```bash
bash teardown.sh
```

---

## 4. Demo 场景详解

### 场景 01: 节点 Cordon → NotReady (SKILL-NODE-001 / RC-012)

**故障注入**: `kubectl cordon <worker-node>`

**Skill 执行流程**:

| Phase | Skill Section | 关键命令 | 判断依据 |
|-------|--------------|---------|---------|
| 症状检测 | Section 2 (S1) | `kubectl get nodes` | SchedulingDisabled |
| 快速分级 | Section 3 (T1-T3) | 计算异常节点比例 | 单节点 → P2 |
| 诊断 | Section 4 (D1.1-D1.5) | `describe node`, `get lease` | Taint + Lease 正常 |
| 根因 | Section 5 | 匹配 root-cause-map.yaml | RC-012 (手动 cordon) |
| 修复 | Section 6 (REM-001) | `kubectl uncordon` | 🟢 低风险 |
| 验证 | Section 7 (V1-V4) | `get node`, `get lease` | Ready + 无 taint |

**学习要点**: 
- RC-012 的 `is_fault: false` — 这不是故障，是人为操作
- 最简单的修复路径：从 cordon 到 uncordon 的闭环

---

### 场景 02: Pod CrashLoopBackOff (SKILL-POD-001)

**故障注入**: 部署启动命令为 `exit 1` 的 Pod

**Skill 执行流程**:

| Phase | 关键诊断点 |
|-------|-----------|
| 症状检测 | Pod 状态 CrashLoopBackOff, 重启次数 >= 3 |
| 诊断 | 容器日志 + Exit Code 分析 (1=应用错误, 137=OOMKilled) |
| 根因 | 启动命令错误 (排除 OOM、镜像、ConfigMap) |
| 修复 | patch deployment 修正 command |

**学习要点**: 
- Exit Code 是区分 CrashLoop 和 OOMKilled 的关键证据
- `kubectl logs --previous` 查看崩溃前日志

---

### 场景 03: Pod Pending (SKILL-POD-002)

**故障注入**: Pod 请求 100 CPU + 512Gi 内存

**Skill 执行流程**:

| Phase | 关键诊断点 |
|-------|-----------|
| 症状检测 | Pod Pending + FailedScheduling event |
| 诊断 | 比较 requests vs 集群 Allocatable |
| 根因 | 资源请求超出集群容量 (排除 affinity/taint/PVC) |
| 修复 | 重建 Pod 使用合理的 requests |

**学习要点**: 
- FailedScheduling 事件消息是定位调度失败原因的核心
- 常见排除项: nodeSelector, affinity, taint, PVC

---

### 场景 04: DNS 解析故障 (SKILL-NET-001)

**故障注入**: CoreDNS 缩容到 0 副本

**Skill 执行流程**:

| Phase | 关键诊断点 |
|-------|-----------|
| 症状检测 | Pod 内 nslookup 超时 + CoreDNS Pod 不存在 |
| 分级 | P0 — DNS 影响全集群 |
| 诊断 | CoreDNS Deployment replicas=0, Endpoints 为空 |
| 根因 | CoreDNS 被缩容 (排除 ConfigMap 错误、OOM、网络) |
| 修复 | scale CoreDNS 回原始副本数 |

**学习要点**: 
- DNS 故障爆炸半径最大，默认 P0
- kube-dns Endpoints 为空是快速定位的关键信号

---

### 场景 05: Service 无 Endpoints (SKILL-NET-002)

**故障注入**: Service selector 含 typo (backend-app-typo)

**Skill 执行流程**:

| Phase | 关键诊断点 |
|-------|-----------|
| 症状检测 | Endpoints `<none>` + Service 访问失败 |
| 诊断 | 对比 Service selector vs Pod labels |
| 根因 | Selector typo (backend-app-typo ≠ backend-app) |
| 修复 | patch Service 修正 selector |

**学习要点**: 
- `kubectl get endpoints` 是 Service 连通性问题的第一检查点
- Selector 不匹配是 Endpoints 为空的最常见原因

---

## 5. Skill 执行闭环与文档映射

Demo 的每一步都与 [skill-schema.md](./skill-schema.md) 的 10-Section 规范严格对应：

```
Demo Phase 0 → (故障注入，demo 特有)
Demo Phase 1 → Skill Section 2: 症状识别
                 └── symptom-patterns.yaml, trigger_keywords
Demo Phase 2 → Skill Section 3: 快速分级
                 └── T1-T3, P0-P3 分级
Demo Phase 3 → Skill Section 4: 诊断工作流
                 └── D1.x-D3.x, diagnostic-workflow.md
Demo Phase 4 → Skill Section 5: 根因分类
                 └── root-cause-map.yaml, FTA 映射
Demo Phase 5 → Skill Section 6: 修复操作
                 └── REM-*, 风险门控, remediation-playbook.md
Demo Phase 6 → Skill Section 7: 验证确认
                 └── V1-V6, verify-node.sh
```

---

## 6. 与 IDE Skill 目录的关联

Demo 的场景 01（节点 Cordon）完整映射到 [k8s-node-notready/](./skill-set/k8s-node-notready/) IDE Skill 目录：

| Demo 步骤 | k8s-node-notready 对应文件 |
|-----------|--------------------------|
| 症状检测 | `assets/symptom-patterns.yaml` |
| 快速分级 | `SKILL.md` Section 3 |
| 诊断工作流 | `scripts/diagnose-quick.sh` + `reference/diagnostic-workflow.md` |
| 根因确认 | `assets/root-cause-map.yaml` → RC-012 |
| FTA 映射 | `assets/skill-metadata.yaml` → `rc_to_fta_steps["RC-012"]` → `evt_cordon` |
| 修复操作 | `reference/remediation-playbook.md` → REM-001 |
| 验证确认 | `scripts/verify-node.sh` |

---

## 7. 常见问题

### Q: Kind 集群创建失败？

```bash
# 确认 Docker 正在运行
docker info

# 删除旧集群重试
kind delete cluster --name skill-demo
bash setup-kind-cluster.sh
```

### Q: Pod 镜像拉取慢？

Kind 集群需要从互联网拉取镜像。如果网络慢，可以：

```bash
# 预加载镜像到 Kind 节点
docker pull nginx:1.27-alpine
kind load docker-image nginx:1.27-alpine --name skill-demo

docker pull busybox:1.36
kind load docker-image busybox:1.36 --name skill-demo
```

### Q: 如何修改集群的 K8s 版本？

```bash
# 使用环境变量指定版本
KIND_IMAGE=kindest/node:v1.30.6 bash setup-kind-cluster.sh
```

可用版本列表: https://hub.docker.com/r/kindest/node/tags

---

## 8. 后续扩展

| 计划场景 | Skill | 注入方式 |
|---------|-------|---------|
| OOMKilled | SKILL-POD-001 | 部署内存泄漏容器 |
| 证书过期 | SKILL-SEC-001 | 修改 kubelet 证书有效期 |
| 节点磁盘压力 | SKILL-NODE-001 / RC-003 | 填充节点磁盘 |
| Ingress 路由失败 | SKILL-NET-003 | 错误的 Ingress 规则 |

---

## 9. 关联资源

| 资源 | 路径 | 说明 |
|------|------|------|
| Demo 脚本 | [skills-run/](./skills-run/) | 本地运行脚本和场景 |
| Skill Schema | [skill-schema.md](./skill-schema.md) | Skill 文档规范模板 |
| IDE Skill 示例 | [k8s-node-notready/](./skill-set/k8s-node-notready/) | 完整 IDE 格式 Skill |
| Skills 索引 | [README.md](./README.md) | Skill 库总索引 |
| FTA 故障树 | [../topic-fta/](../topic-fta/) | 故障分析模型 |
