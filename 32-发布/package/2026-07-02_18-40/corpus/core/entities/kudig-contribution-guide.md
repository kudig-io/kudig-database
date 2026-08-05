---
title: 贡献指南、项目概览与版本发布说明
description: '# 贡献指南、项目概览与版本发布说明'
summary: '# 贡献指南、项目概览与版本发布说明'
category: reference
tags:
- k8s
- contribution
- project-overview
- release-notes
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 贡献指南、项目概览与版本发布说明 是什么
- 如何 贡献指南、项目概览与版本发布说明
trigger_keywords:
- 贡献指南
- 项目概览与版本发布说明
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 贡献指南、项目概览与版本发布说明

## 快速开始

1. 克隆仓库：`git clone https://github.com/kudig-io/kudig-database.git`
2. 浏览文档：使用 Obsidian 或 VS Code 打开
3. Gitbook 离线浏览：`cd gitbook && QUICK-BUILD.cmd`
4. AI 语料库接入：参考 `corpus-config/` 目录

## 知识图谱与学习路径

推荐学习顺序：
1. Linux 基础 → Docker 容器 → K8s 架构
2. 核心组件 → API 对象 → 控制器模式
3. 网络 → 存储 → 安全
4. 生产运维 → 高级主题

## 贡献指南

- 文档规范：遵循 KUDIG Frontmatter 规范
- 命名规则：小写英文 + 连字符，序号前缀
- 提交流程：Fork → Branch → PR → Review → Merge
- 质量要求：YAML 示例经过验证，命令附带注释

## 版本发布说明

覆盖 Kubernetes v1.25-v1.33 版本特性，包括：
- API 版本变更
- 功能门控（Feature Gates）
- 废弃与移除项
- 升级注意事项

---

> 来源：.zread/wiki/drafts/2-*.md, 3-*.md, 4-*.md, 31-*.md

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/skills/training-lecturer/11-workloads/index|release-notes-cli-tools]] — 发布说明索引 — CLI 工具
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/skills/training-lecturer/11-workloads/index|release-notes-cicd-gitops]] — 发布说明索引 — CI/CD 与 GitOps
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/skills/training-lecturer/11-workloads/index|release-notes-reading-guide]] — 发布说明阅读指南
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
