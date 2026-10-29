---
title: RAG 分块策略指南与 Manpage 安装指南
description: '## RAG 分块策略指南'
summary: 'from langchain.text_splitter import MarkdownHeaderTextSplitter'
category: reference
tags:
- k8s
- rag
- chunking
- manpage
- installation
- etcd
- coredns
- helm
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- RAG 分块策略指南与 Manpage 安装指南 是什么
- 如何 RAG 分块策略指南与 Manpage 安装指南
trigger_keywords:
- RAG
- 分块策略指南与
- Manpage
- 安装指南
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# RAG 分块策略与 Manpage 安装

## RAG 分块策略指南

推荐策略：按 Markdown 标题分块

```python
from langchain.text_splitter import MarkdownHeaderTextSplitter

splitter = MarkdownHeaderTextSplitter(
    headers_to_split_on=[
        ('#', 'title'),
        ('##', 'section'),
    ]
)
```

最佳实践：
- 按 H1/H2 分块，保持知识完整性
- 块大小：200-800 tokens
- 重叠：50-100 tokens

## Manpage 安装指南

一键安装 KUDIG manpage 到系统：

```bash
# Linux
sudo bash -c '
  cd /path/to/kudig-database
  cp man/*.1 /usr/local/share/man/man1/
  mandb
'
```

支持的工具 manpage：kubectl、etcd、helm、containerd、coredns 等。

---

> 来源：corpus-config/*.md, man/*.md（共 4 篇）

## Related

- [[containerd]] — containerd
- [[helm]] — Helm
- [[coredns]] — CoreDNS
- [[etcd]] — etcd


<!-- risk-assessed -->
