---
title: Gitbook 本地文档浏览系统与构建指南
description: '## 概述'
summary: '基于 [mdBook](https://rust-lang.github.io/mdBook/) 构建的本地知识库浏览系统，支持全文搜索、目录折叠导航。'
category: reference
tags:
- k8s
- gitbook
- mdbook
- documentation
- build
- export
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gitbook 本地文档浏览系统与构建指南 是什么
- 如何 Gitbook 本地文档浏览系统与构建指南
trigger_keywords:
- Gitbook
- 本地文档浏览系统与构建指南
prerequisites:
- kubectl-basics
---



# Gitbook 本地文档浏览系统

## 概述

基于 [mdBook](https://rust-lang.github.io/mdBook/) 构建的本地知识库浏览系统，支持全文搜索、目录折叠导航。

## 快速导出

**方式 1：双击运行**
```
双击文件：QUICK-BUILD.cmd
```

**方式 2：命令行**
```cmd
cd gitbook
QUICK-BUILD.cmd
```

- 构建时间：1-3 分钟
- 输出位置：`export/kudig-gitbook-YYYYMMDD-HHMMSS/`
- 自动生成 ZIP 压缩包
- UTF-8 编码，无乱码

## 目录结构

```
gitbook/
├── README.md              # 主入口
├── BUILD-README.md        # 构建说明
├── QUICK-BUILD.cmd        # Windows 一键构建
├── src/                   # mdBook 源文件
├── build-scripts/         # 构建脚本
├── documentation/         # 使用文档
└── export/                # 导出产物
```

## 平台兼容性

- Windows：QUICK-BUILD.cmd 双击运行
- macOS/Linux：shell 脚本执行
- 需要预装 mdBook（`cargo install mdbook`）

---

> 来源：gitbook/*.md（共 11 篇）

## Related

- [[kudig-templates-catalog]] — KUDIG Templates Catalog
- [[README]] — FTA 故障树清单索引
