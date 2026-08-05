---
title: Readme
summary: '![Deploy to GitHub Pages](https://github.com/kudig-io/kudig-database/actions/workflows/deploy-pages.yml)'
category: general
tags:
- readme
tier: supporting
created: '2026-07-01'
---

# KUDIG Database

> 面向生产环境的 Kubernetes + AI Infrastructure 运维全域知识库。
> 既是人类可读的运维手册，也是 AI Agent 的 RAG 语料来源。

[![Deploy to GitHub Pages](https://github.com/kudig-io/kudig-database/actions/workflows/deploy-pages.yml/badge.svg)](https://github.com/kudig-io/kudig-database/actions/workflows/deploy-pages.yml)

## 这是什么

KUDIG Database 是一个**双层结构**的云原生运维知识库：

- **提炼知识层**（`22-概念/` `23-实体/` `26-技能/` `24-综合/`）—— Agent 优先读取，Token 效率高，frontmatter 元数据丰富
- **源文档层**（`01-集群基础/` … `21-生态参考/` `29-文档/`）—— 原始深度技术文档，供深度查询兜底

覆盖 **20 个核心知识域**：集群基础、工作负载、网络、存储、安全、可观测性、平台工程、发布变更、可靠性工程、故障诊断、生产运维、云厂商（13+ 家）、容器运行时、AI/ML 基础设施、专项技术、数据库中间件、系统基础、清单模式、生态参考、应用模式。

## 生产就绪快速入口

每个编号 Domain 均提供入口级生产就绪运维指南，另有关键跨域 Runbook：

- **Per-Domain 生产就绪指南**：各知识域目录下的 `99-production-readiness-operations-guide.md`
- **证书 / PKI 生命周期 Runbook**：`01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook.md`
- **集群升级 Runbook**：`01-集群基础/03-控制平面/39-cluster-upgrade-runbook.md`
- **灾难恢复与业务连续性 Runbook**：`12-可靠性/02-灾难恢复/25-disaster-recovery-bc-runbook-v2.md`
- **Fleet GitOps 操作指南**：`11-发布变更/01-GitOps/10-fleet-gitops-operations-guide.md`
- **事件响应 Runbook 模板**：`13-生产运维/03-事件响应/11-incident-response-runbook-template.md`
- **FinOps 成本治理 Runbook**：`13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md`
- **AI/ML 运维 Runbook**：`15-AI基础设施/01-基础设施/38-ai-ml-ops-runbook.md`
- **边缘生产运维 Runbook**：`16-专项技术/01-边缘计算/12-edge-production-runbook.md`

详细说明与缺口分析参见 `36-报告/assessments/domain-production-readiness-content-push-2026-07-01.md` 与 `36-报告/assessments/domain-content-gap-analysis-2026-07-01.md`。

## 快速开始

### 浏览在线站点

访问 GitHub Pages 自动部署的站点：<https://kudig-io.github.io/kudig-database/>

### 本地开发

本项目使用 [Astro](https://astro.build/) 构建静态站点。

```bash
# 进入站点目录
cd 30-站点

# 安装依赖
npm install

# 启动开发服务器（热重载，默认 http://localhost:4321）
npm run dev

# 构建生产静态产物（输出到 ../site）
npm run build

# 本地预览构建产物
npm run preview
```

### 仅阅读 Markdown 源文件

知识内容全部是纯 Markdown，可直接在任何编辑器（推荐 [Obsidian](https://obsidian.md/)）中阅读，wikilink `[[...]]` 可被 Obsidian 原生解析。

## 目录结构

```
.
├── 01-集群基础/ … 21-生态参考/   # 源文档：21 个技术域深度文档（NN-中文目录名）
├── 22-概念/         # 提炼知识：核心概念、架构模式、综合分析
├── 23-实体/         # 提炼知识：组件实体、CNCF 工具、云产品、术语词典
├── 24-综合/         # 提炼知识：跨领域综合分析
├── 25-研究/         # 研究资料
├── 26-技能/         # 提炼知识：诊断排障、最佳实践、培训体系、FTA 方法
├── 27-标签/         # 标签索引
├── 28-资产/         # 图片、图表、附件
├── 29-文档/         # 源文档：映射与规范文档
├── 30-站点/         # Astro 静态站点项目（.gitignore 忽略）
├── 31-脚本/         # 自动化脚本、模板、提示词
├── 32-发布/         # 发布产物（语料导出 corpus、metadata、qa，冻结）
├── 33-源码/         # vendor 源码树（Kubernetes、terway 等，.gitignore 忽略）
├── 34-源码分析/     # 源码分析笔记
├── 35-元数据/       # 元数据、语料配置（taxonomy、schema、corpus-config、journal）
├── 36-报告/         # 质量报告与评估、发布素材（冻结）
└── 37-归档/         # Wiki 归档快照（重建/恢复用，冻结）
```

完整目录映射与命名规范详见 [`35-元数据/metadata/domain-mapping.md`](35-元数据/metadata/domain-mapping.md)。

## 部署

推送到 `main` 分支会自动触发 GitHub Actions（`.github/workflows/deploy-pages.yml`）：执行 `npm ci && npm run build` 后部署到 GitHub Pages。

## 贡献

欢迎提交 Issue 和 PR。贡献前请阅读 [`35-元数据/metadata/domain-mapping.md`](35-元数据/metadata/domain-mapping.md) 与 [`35-元数据/metadata/schema.md`](35-元数据/metadata/schema.md) 了解目录约定与 frontmatter 规范。

## License

详见 [`LICENSE`](LICENSE)。
