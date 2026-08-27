---
title: 变更日志
summary: 变更日志：本文件记录 KUDIG 云原生运维知识库的 notable 变更。遵循 Keep a Changelog 风格，按 git 历史归类。
category: meta
tags:
- meta
- visibility/public
- changelog
tier: supporting
sources:
- git-log
created: 2026-01-16
updated: 2026-08-25
last_updated: 2026-08-25
---



# 变更日志

本文件记录 KUDIG 云原生运维知识库的 notable 变更。遵循 [Keep a Changelog](https://keepachangelog.com/) 风格，按 git 历史归类。

## [Unreleased]

### 全量维护（2026-08）

#### 断链清零
- 重映射 109 个 broken wikilinks：26 个 index.md/导航页在子目录重新编号后仍使用旧编号链接（如 `09-可观测性` 的 `02-指标/` → `01-指标/`、各云厂商域文件号整体前移），按主题名精确重映射到真实文件
- 修复 1 个 macOS 重复后缀链接（`08-runtime-security-hardening 2.md` → `09-runtime-security-hardening.md`）

#### README / 规范一致性
- `readme-sync-check` 18 个问题 → 0：同步 `09-可观测性` 子目录编号（`00-总览/` 起）、修复 `22-概念` 2 个连字符文件名链接、`34-源码分析` 代码分析目录链接改指 `index.md`、根 README 目录图补录 `38-消化/` 与 `kubernetes-hardware/`
- `domain-mapping.md` 同步可观测性/平台工程/概念三个目录的实际二级结构（`00-总览` 编号方案）
- README / CONTRIBUTING 中旧 `scripts/`、`web/`、`_archives/` 路径统一为 `31-脚本/`、`30-站点/`、`37-归档/`

#### 质量门禁（本地全量复验）
- frontmatter 完整性（4491 文件）/ 标题层级（3972 文件）/ 断链 / README 一致性 / ruff lint 全部通过

### 生产级整改（2026-07）

#### 内容纠偏（P0）
- 修复 ValidatingAdmissionPolicy / MutatingAdmissionPolicy 版本声明矛盾：统一为 VAP GA=1.30、MAP GA=1.36（原多处误标 1.28/1.30 GA）
- 删除伪造的 Prometheus 配置项 `scrape.parallelism` / `scrape.compression`
- 全库替换 Alertmanager 自 0.22 起废弃的字段：`match_re` / `source_match` / `target_match` / `service_key` → `matchers` / `source_matchers` / `target_matchers` / `routing_key`（63 文件）
- 修复残留的 PodSecurityPolicy 失效诊断命令（改为 Pod Security Admission 标签检测）
- 刷新 Prometheus 镜像至 v3.2.1、Thanos 至 v0.37.0（原 v2.40 / v0.30 等过期版本）

#### 工程卫生（P1）
- 将误入库的 `.env` 移出版本控制并补强 `.gitignore`（密钥、IDE、个人开发环境产物）
- 取消追踪 `.claude/skills/` 死链（硬编码个人绝对路径）与 `.claude/scripts/output/` 构建产物
- 重写 `CONTRIBUTING.md`（原为 jsdiff 项目模板）与 `CHANGELOG.md`（原为 css-what 库日志）
- 删除已腐烂的 `comprehensive-quality-check.sh`（引用已废弃的 17 域旧布局）
- 新增 `.github/workflows/quality.yml`：ruff lint + frontmatter 完整性 + broken wikilink 检查（gating）
- `corpus-coverage-check.yml` 由 warn-only 改为阻断式（coverage 回归即失败）

#### 清理（P2）
- 批量清理 `#<!-- chunk:` 生成器残留标记
- frontmatter schema 归一（统一 `last_updated` / `intent_queries` 字段集）

## [0.3.0] - 2026-06

### 新增
- 部署链路从 mdBook 迁移至 Astro（`web/`，GitHub Pages）
- Pagefind 客户端搜索、Shiki 语法高亮、Mermaid 图表

### 变更
- 清理入口文档对 mkdocs / gitbook 的过期引用
- 移除 mkdocs 依赖与 gitbook / mkdocs 专用脚本及冗余追踪文件

## [0.2.0] - 2026-05

### 新增
- 全域深度研究：11 域 33 轮研究 + 31 页面 + 交叉链接
- 补充 domain-12（云厂商）/ domain-17（系统基础）/ domain-20（应用模式）研究
- 创建 `_archives`、`_meta`、`_reports`、`docs/agent-specs` 等目录索引

### 修复
- wiki-lint 全面健康审计：taxonomy 补全 + orphan 救援 + cross-linker + synthesis
- 删除 325 个重复文件 + 修复 327 个 wikilinks
- 修复 632 个路径前缀 broken links + 93 个概念/合成页 broken wikilinks
- 补全 46 个文件的缺失 frontmatter 字段（title / category / tags）
- 修复全部 fixable orphans（domain-11、skills scenarios 等）

## [0.1.0] - 2026-01 ~ 2026-04

### 新增
- 建立 20 个知识域（`domain-01` ~ `domain-20`）基础结构
- Java Spring 技术栈、Terway、PVC 内容与索引
- Web UI（d3.js 可视化）
- KUDIG 知识缺口全面修复、P0-P1 生产环境修复、P2 优化级修复
- 20 域 `98-merged-indexes` 全覆盖
- release-notes、assessments、ecosystem 索引与 orphan 修复
- `.gitattributes` 解决 GitHub ZIP 在 macOS 解压错误

---

> 本变更日志基于 git 提交历史人工整理。详细的逐提交记录请运行 `git log --oneline`。

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
