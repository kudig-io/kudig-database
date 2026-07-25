---
title: 团队入职指南模板
description: 新成员团队入职 Guide 标准模板
summary: 团队入职指南模板 — 新成员加入团队的结构化入职流程和资源清单
category: template
tags:
- onboarding
- template
- team
- documentation
- standard
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: beginner
reading_level: beginner
audience:
- 新成员
- 团队负责人
- Mentor
estimated_read_time: 5min
intent_queries:
- 入职指南模板 是什么
- 如何编写团队入职 Guide
- 新成员 onboarding 模板
- team onboarding guide template
trigger_keywords:
- 入职
- onboarding
- 团队
- 新成员
- 模板
- guide
prerequisites: []
---

> **生产环境安全提示**
>
> 本文档为模板，不包含可执行命令。

# 团队入职指南模板

> **模板版本**: 1.0 | **适用场景**: 新成员加入 SRE/平台/开发团队 | **使用方式**: 复制此模板，替换所有 `[PLACEHOLDER]`

## 模板使用说明

1. 团队负责人为新成员分配一名 Mentor
2. Mentor 基于此模板定制个人入职计划
3. 在第一天的 1:1 中 review 入职计划
4. 每周 1:1 跟踪进度
5. 试用期结束时 (通常 3 个月) 做完整 review

---

## 入职指南: [团队名称]

> **团队**: [团队名称，如: Platform Engineering / SRE]
> **入职人员**: [姓名]
> **入职日期**: [YYYY-MM-DD]
> **Mentor**: [姓名]
> **团队负责人**: [姓名]
> **计划周期**: [如: 12 周]

### 1. 团队介绍

#### 1.1 团队使命

[2-3 句话描述团队的使命和价值。例如: "平台工程团队负责构建和维护公司 Kubernetes 基础设施，为 50+ 业务团队提供稳定、高效、安全的容器运行平台。"]

#### 1.2 团队职责

- [职责 1: 如 — 集群生命周期管理]
- [职责 2: 如 — CI/CD 基础设施]
- [职责 3: 如 — 可观测性平台]
- [职责 4: 如 — 安全合规]

#### 1.3 团队成员

| 姓名 | 角色 | 负责领域 | 联系方式 |
|------|------|---------|---------|
| [姓名] | [角色] | [领域] | [企业微信/Slack] |
| [姓名] | [角色] | [领域] | [企业微信/Slack] |

#### 1.4 团队规范

- **工作时间**: [如: 10:00-19:00，弹性 1 小时]
- **站会**: [如: 每日 10:30，15 分钟]
- **周会**: [如: 每周二 14:00]
- **代码 Review**: [如: 所有 PR 至少 1 人 Review]
- **文档文化**: [如: 所有变更必须更新文档]
- **通信工具**: [如: 企业微信为主，Slack 备用]

### 2. 第一周: 环境搭建与基础知识

#### Day 1-2: 账号与权限

- [ ] 获取公司邮箱和工号
- [ ] 加入团队通信群组 ([企业微信/Slack 群])
- [ ] 获取 GitLab/GitHub 账号，加入团队组织
- [ ] 获取 VPN 访问权限
- [ ] 获取跳板机/Bastion 访问权限
- [ ] 获取 AWS/阿里云 Console 访问 (如需要)
- [ ] 配置 MFA (多因素认证)
- [ ] 获取 kubeconfig (只读权限 — staging 环境)

#### Day 3-4: 本地开发环境

```bash
# 🟢 安装必要工具 (示例)
brew install kubectl helm jq yq terraform  # macOS
# apt install kubectl helm jq yq terraform  # Linux

# 🟢 验证 kubectl 可连接 staging
kubectl get nodes --context staging-cluster

# 🟢 克隆团队代码仓库
git clone [repo-url]
```

- [ ] 安装 `kubectl` >= [版本]
- [ ] 安装 `helm` >= [版本]
- [ ] 安装 `terraform` >= [版本] (如使用)
- [ ] 配置 IDE ([VSCode/GoLand] + 插件)
- [ ] 配置 kubeconfig (staging 环境)
- [ ] 验证可以访问 staging 集群: `kubectl get nodes`

#### Day 5: 架构概览

阅读以下文档:
- [ ] [KUDIG 知识库 — 平台架构概览]
- [ ] [集群拓扑图]
- [ ] [网络架构文档]
- [ ] [安全模型文档]

**第一周结束时，新成员应能**:
- ✅ 访问所有必要的工具和系统
- ✅ 描述集群的大致架构
- ✅ 在 staging 环境执行基本 kubectl 命令

### 3. 第二至四周: 核心技能学习

#### 学习路径

| 周次 | 主题 | 学习资源 | 实践任务 | 完成标准 |
|------|------|---------|---------|---------|
| 第 2 周 | Kubernetes 基础 | [[13-生产运维/topic-learn/k8s-basics|K8s 基础]] | 在 staging 部署一个应用 | Pod 正常运行 |
| 第 2 周 | Helm 包管理 | [Helm 官方文档] | 用 Helm 部署一个 chart | Release 成功 |
| 第 3 周 | 监控与告警 | [[10-平台工程/02-运维/06-monitoring-alerting-system|监控体系]] | 配置一个告警规则 | 告警可触发 |
| 第 3 周 | CI/CD 流程 | [CI/CD 文档] | 走完一个完整 pipeline | 部署成功 |
| 第 4 周 | 安全合规 | [[17-系统基础/06-知识字典/security/pod-security-standards|PSS 标准]] | 审计一个命名空间 | 生成报告 |
| 第 4 周 | 故障排查 | [[19-故障诊断/06-FTA故障树/MOC|FTA 故障树]] | 模拟故障并排查 | 找到根因 |

#### 推荐阅读清单

- [ ] [[22-概念/10-最佳实践/bp-common-best-practices|Kubernetes 通用最佳实践]]
- [ ] [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management|GPU 调度与管理]] (如涉及 GPU)
- [ ] [[31-脚本/templates/runbook-template|Runbook 示例]] — 了解运维手册格式
- [ ] [Google SRE Book — 前三章]

### 4. 第五至八周: 实战演练

#### Oncall 准备

- [ ] Shadow oncall (跟随 Mentor 观察 1 周)
- [ ] 学习处理常见告警 (参考 [[31-脚本/templates/runbook-template|Runbook]])
- [ ] 了解升级路径和通知流程
- [ ] 模拟练习: 使用 [[31-脚本/automation/k8s-health-check|健康检查脚本]] 排查问题

#### 小项目 (建议)

分配一个适合新成员的小型改进项目:
- [项目示例: 优化某个 Runbook]
- [项目示例: 编写一个自动化脚本]
- [项目示例: 改进监控 Dashboard]

**项目验收标准**: [定义清晰的可交付成果]

### 5. 第九至十二周: 独立工作

- [ ] 独立处理 oncall 告警 (Mentor 作为 backup)
- [ ] 参与代码 Review (Review 他人代码)
- [ ] 独立完成一个中等复杂度的任务
- [ ] 参与团队架构设计讨论

**试用期结束目标**:
- ✅ 能独立处理 P2/P3 级别告警
- ✅ 熟悉团队所有核心系统的架构
- ✅ 完成 1 个以上独立项目
- ✅ 获得生产环境的操作权限

### 6. 权限提升路径

| 阶段 | 环境权限 | 时间 | 审批人 |
|------|---------|------|--------|
| 入职 | staging 只读 | Day 1 | Mentor |
| 第 2 周 | staging 读写 | Day 10 | Team Lead |
| 第 6 周 | production 只读 | Week 6 | Team Lead + SRE Lead |
| 第 12 周 | production 读写 (受控) | Week 12 | SRE Lead + Manager |

### 7. 资源清单

| 类别 | 资源 | 链接/位置 |
|------|------|---------|
| 知识库 | KUDIG Wiki | [Obsidian Vault] |
| 代码 | 团队仓库 | [GitLab Group] |
| 文档 | 架构文档 | [Confluence/Notion] |
| 监控 | Grafana | [Grafana URL] |
| 告警 | Alertmanager | [Alertmanager URL] |
| 日志 | ELK/Loki | [Kibana/Grafana Logs URL] |
| CI/CD | Pipeline | [GitLab CI / ArgoCD] |
| 紧急 | Oncall 交接 | [PagerDuty / 内部系统] |

### 8. 反馈机制

- **每周 1:1**: 每周五与 Mentor 进行 30 分钟 1:1，review 本周进度
- **月度 Review**: 每月与 Team Lead 进行入职进度 review
- **匿名反馈**: [如有匿名反馈渠道，让新成员可以坦诚反馈入职体验]
- **改进迭代**: 根据每位新成员的反馈，持续改进本入职指南

---

## 入职 Checklist (给 Mentor)

- [ ] 提前一天确认所有账号已创建
- [ ] 准备好新成员的工位/设备
- [ ] 第一天 10:00 接待新成员
- [ ] 介绍团队成员
- [ ] Review 入职计划和期望
- [ ] 分配第一周的学习任务
- [ ] 设置每周 1:1 日历邀请
- [ ] 每周跟踪进度，及时调整计划

---

## 版本历史

| 版本 | 日期 | 变更 | 作者 |
|------|------|------|------|
| v1.0 | [YYYY-MM-DD] | 初始版本 | [作者] |

<!-- risk-assessed -->
