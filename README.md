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

- **提炼知识层**（`concepts/` `entities/` `skills/` `references/` `synthesis/`）—— Agent 优先读取，Token 效率高，frontmatter 元数据丰富
- **源文档层**（`domain-*/` `topic-*/` `docs/`）—— 原始深度技术文档，供深度查询兜底

覆盖 **20 个核心知识域**：集群基础、工作负载、网络、存储、安全、可观测性、平台工程、发布变更、可靠性工程、故障诊断、生产运维、云厂商（13+ 家）、容器运行时、AI/ML 基础设施、专项技术、数据库中间件、系统基础、清单模式、生态参考、应用模式。

## 生产就绪快速入口

每个编号 Domain 均提供入口级生产就绪运维指南，另有关键跨域 Runbook：

- **Per-Domain 生产就绪指南**：`domain-01..20/99-production-readiness-operations-guide.md`
- **证书 / PKI 生命周期 Runbook**：`domain-01-cluster-fundamentals/03-control-plane/34-certificate-pki-lifecycle-runbook.md`
- **集群升级 Runbook**：`domain-01-cluster-fundamentals/03-control-plane/35-cluster-upgrade-runbook.md`
- **灾难恢复与业务连续性 Runbook**：`domain-09-reliability-engineering/09-disaster-recovery-playbooks/03-disaster-recovery-bc-runbook.md`
- **Fleet GitOps 操作指南**：`domain-08-release-change-management/01-gitops/08-fleet-gitops-operations-guide.md`
- **事件响应 Runbook 模板**：`domain-11-production-operations/03-incident-response/24-incident-response-runbook-template.md`
- **FinOps 成本治理 Runbook**：`domain-11-production-operations/01-finops/14-finops-cost-governance-runbook.md`
- **AI/ML 运维 Runbook**：`domain-14-ai-ml-infra/01-ai-infra/45-ai-ml-ops-runbook.md`
- **边缘生产运维 Runbook**：`domain-15-specialized-tech/01-edge-computing/14-edge-production-runbook.md`

详细说明与缺口分析参见 `_reports/domain-production-readiness-content-push-2026-07-01.md` 与 `_reports/domain-content-gap-analysis-2026-07-01.md`。

## 快速开始

### 浏览在线站点

访问 GitHub Pages 自动部署的站点：<https://kudig-io.github.io/kudig-database/>

### 本地开发

本项目使用 [Astro](https://astro.build/) 构建静态站点。

```bash
# 进入 web 目录
cd web

# 安装依赖
npm install

# 启动开发服务器（热重载，默认 http://localhost:4321）
npm run dev

# 构建生产静态产物（输出到 ../site）
npm run build

# 本地预览构建产物
npm run preview
```

或使用项目封装的脚本：

```bash
bash scripts/start-web.sh            # Astro dev @ :4321（默认）
bash scripts/start-web.sh --preview  # 先构建再预览
bash scripts/start-web.sh --static   # 伺服 visualizations/ 等独立 HTML 工具 @ :8767
bash scripts/start-web.sh --stop     # 停止服务
```

### 仅阅读 Markdown 源文件

知识内容全部是纯 Markdown，可直接在任何编辑器（推荐 [Obsidian](https://obsidian.md/)）中阅读，wikilink `[[...]]` 可被 Obsidian 原生解析。

## 目录结构

```
.
├── concepts/        # 提炼知识：核心概念、架构模式
├── entities/        # 提炼知识：组件实体、CNCF 工具、云产品
├── skills/          # 提炼知识：诊断排障、最佳实践、FTA 方法
├── references/      # 提炼知识：术语词典、命令速查、规范
├── synthesis/       # 提炼知识：跨领域综合分析
├── domain-01..20/   # 源文档：20 个技术域深度文档
├── docs/            # 映射与规范文档
├── _meta/           # 元数据定义（taxonomy、schema）
├── _reports/        # 质量报告与评估
├── corpus-config/   # AI 语料配置（RAG profile、分块策略）
├── web/             # Astro 站点项目
├── scripts/         # 自动化脚本
└── STRUCTURE.md     # 完整目录结构规范
```

详见 [`STRUCTURE.md`](STRUCTURE.md)。

## 部署

推送到 `main` 分支会自动触发 GitHub Actions（`.github/workflows/deploy-pages.yml`）：执行 `npm ci && npm run build` 后部署到 GitHub Pages。

## 贡献

欢迎提交 Issue 和 PR。贡献前请阅读 [`STRUCTURE.md`](STRUCTURE.md) 了解目录约定与 frontmatter 规范。

## License

详见 [`LICENSE`](LICENSE)。
