---
title: Gitbook macOS 导出计划
description: '- [[entities/kudig-gitbook-system.md|kudig-gitbook-system]] — Gitbook
  本地文档浏览系统与构建指南'
summary: '- [[entities/kudig-gitbook-system.md|kudig-gitbook-system]] — Gitbook 本地文档浏览系统与构建指南'
category: reference
tags:
- k8s
- gitbook
- macos
- export
- plan
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gitbook macOS 导出计划 是什么
- 如何 Gitbook macOS 导出计划
trigger_keywords:
- Gitbook
- macOS
- 导出计划
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Gitbook macOS 导出计划

## 任务清单（全部已完成）

1. ✅ 核对 gitbook 文档与脚本，确认 Windows 与 macOS 的入口与差异
2. ✅ 在 mac 上安装或确认 mdBook 可用
3. ✅ 执行 export-static.sh 生成 dist/ 与可选 zip 产物
4. ✅ 验证 dist/index.html 可打开并检查页面与搜索可用性

## 用户需求

- 检查 Gitbook 在 Windows 与 macOS 上的运行与构建流程是否一致可用
- 在当前 mac 上以离线静态方式运行并输出可直接打开的本地页面
- 提供可验证的结果（输出目录与打开方式）

---

> 来源：.codebuddy/plans/gitbook-mac-export-run_8ca54509.md

## Related

- [[entities/kudig-gitbook-system.md|kudig-gitbook-system]] — Gitbook 本地文档浏览系统与构建指南
- [[INDEX]] — Wiki Index


<!-- risk-assessed -->
