# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Stack

static HTML/CSS/JS — GTM 页面为自包含静态页面，存放于根目录 `GTM/`，无构建依赖，浏览器直接打开即可运行。不集成到 30-站点/ Astro 站点。

## Users

三类受众兼顾（用户确认）：
1. **K8s / 平台工程师**：在生产事故中需要结构化排障知识的人，日常查阅运维手册
2. **团队决策者（SRE Lead / 平台团队负责人）**：评估是否采用该知识库作为团队知识基座或 Agent RAG 语料源
3. **开源社区（GitHub / CNCF 生态）**：潜在 star、fork、贡献者

## Product Purpose

KUDIG Database 是面向生产环境的 Kubernetes + AI Infrastructure 运维全域知识库：
- 既是人类可读的运维手册，也是 AI Agent 的 RAG 语料来源（双读架构）
- ~4,750 篇活跃知识文档，21 个核心技术域 + 4 层 wiki 提炼知识（概念/实体/综合/技能）
- 附带 RAG 语料导出 pipeline，可直接供 AI Agent 检索增强
- GTM 页面是该知识库面向外部世界的旗舰营销页面，同时承担获客、策略展示、社区招募三重职能

## Positioning

结构化排障引擎是 KUDIG 独有机制（邻近产品无法复制）：
- FTA 故障树分析：16 个顶层故障事件的故障树 + 动态概率推理
- FEBM 法医证据方法：从症状特征向量匹配到根因确认的证据链
- 19+ 张诊断技能卡
- QA 语料生成：脚本化 I-O 配对，用于 Agent 训练与评估
- 双层架构：源文档层（深度查询兜底）+ 提炼知识层（Agent 优先，Token 高效）

## Operating Context

- 版本覆盖：Kubernetes v1.25 ~ v1.32+，含 ACK / Terway / ASM 阿里云扩展
- 云厂商覆盖：13 家（阿里云 / AWS / GCP / Azure / 腾讯云 / 华为云 / 多云混合）
- 发布渠道：GitHub（kudig-io/kudig-database）、GitHub Pages 站点
- License：Apache 2.0
- 仓库结构：中文编号目录体系（01-集群基础 ~ 39-百炼智能体）

## Capabilities and Constraints

- GTM 页面必须为纯静态，无 JS 框架、无构建步骤
- 页面语言：中文为主（仓库全中文），关键英文术语保留
- 内容必须基于仓库真实数据（4,750 篇、21 域、13 云厂商等），不可虚构用户评价、客户案例或未承诺的功能

## Brand Commitments

- 项目名：KUDIG Database（Kubernetes 全域知识库）
- 组织：kudig-io
- 已有站点部署于 GitHub Pages（kudig-io.github.io/kudig-database）

## Evidence on Hand

- README.md：完整项目概览、核心统计表、知识架构图、生产就绪入口表
- 30-站点/：Astro 站点（既有视觉参考）
- 仓库本身即产品：4,750 篇文档、故障诊断域 491 篇、系统基础域 646 篇
- CI badges：Deploy to GitHub Pages、Quality Check
- 无：付费客户案例、用户评价、benchmark 数据（不可虚构）

## Product Principles

1. 双读架构是核心叙事：人类手册 + AI Agent 语料，一页之内必须让两者都可见
2. 结构化排障引擎是差异化证明：FTA/FEBM 必须被演示而非仅声称
3. 规模即证据：4,750 篇 / 21 域 / 13 云厂商的数字本身就是说服力
4. 开源即渠道：Apache 2.0 + GitHub 生态是采纳摩擦为零的路径
5. 生产就绪导向：所有入口指向可执行的 Runbook，非概念罗列
