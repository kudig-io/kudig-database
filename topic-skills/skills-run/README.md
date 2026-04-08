# Skills Demo — 本地运行工单诊断技能

> **目的**: 在本地 Kind 集群中实际运行 Skill 执行闭环，体验从故障注入到修复验证的完整流程  
> **场景数量**: 10 个 (5 个基础 + 5 个扩展)  
> **耗时**: 环境搭建 ~5min + 每个场景 ~5min  
> **前置条件**: Docker Desktop + kind + kubectl

---

## 快速开始

```bash
# 1. 进入 demo 目录
cd topic-skills/demo

# 2. 创建本地多节点 Kind 集群 (1 control-plane + 2 workers)
bash setup-kind-cluster.sh

# 3. 运行交互式 demo（菜单选择场景）
bash run-skill-demo.sh

# 4. 或直接运行单个场景
bash run-skill-demo.sh 1    # 节点 Cordon
bash run-skill-demo.sh 2    # Pod CrashLoop
bash run-skill-demo.sh 3    # Pod Pending
bash run-skill-demo.sh 4    # DNS 故障
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
| 01 | 节点被 cordon | SKILL-NODE-001 | RC-012 | 🟢 低 |
| 02 | Pod CrashLoopBackOff | SKILL-POD-001 | 启动命令错误 | 🟢 低 |
| 03 | Pod Pending | SKILL-POD-002 | 资源超限 | 🟢 低 |
| 04 | DNS 解析故障 | SKILL-NET-001 | CoreDNS 缩容 | 🟢 低 |
| 05 | Service 无 Endpoints | SKILL-NET-002 | Selector 不匹配 | 🟢 低 |

### 扩展场景 (6-10)

| # | 场景 | 对应 Skill | 根因 | 风险等级 |
|---|------|-----------|------|----------|
| 06 | PVC Pending | SKILL-STORE-001 | RC-001 (StorageClass 不存在) | 🟢 低 |
| 07 | Deployment rollout 卡住 | SKILL-WORK-001 | RC-002 (readinessProbe 失败) | 🟢 低 |
| 08 | RBAC 权限拒绝 | SKILL-SEC-002 | RC-001 (缺少 RBAC 权限) | 🟡 中 |
| 09 | HPA 不触发扩容 | SKILL-SCALE-001 | RC-002 (未设置 resources.requests) | 🟡 中 |
| 10 | 镜像拉取失败 | SKILL-IMAGE-001 | RC-001 (镜像不存在) | 🟢 低 |

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
    │   风险门控 (🟢🟡🔴⚫)
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
