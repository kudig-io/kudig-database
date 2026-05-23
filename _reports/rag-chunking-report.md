---
title: RAG Chunking Report
description: '# RAG Chunking 优化报告'
category: references
tags:
- rag
- chunking
- report
- helm
- argocd
- networkpolicy
- ebpf
- wasm
- serverless
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- RAG Chunking Report 是什么
- 如何 RAG Chunking Report
trigger_keywords:
- RAG
- Chunking
- Report
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- ebpf-basics
created: "2026-05-23"
---

# RAG Chunking 优化报告

> 生成日期: 2026-05-20
> 最后更新: 2026-05-20 (第二轮改进)

## Chunking 标记

- 已添加 chunk 标记: **893 文件** (第一轮 375 + 第二轮 518)
- 标记位置:
  - 第一轮: domain-1 ~ domain-12 核心文档 (375 文件)
  - 第二轮: domain-13~40 + topic-* 长文档 (518 文件)
- 标记格式: `<!-- chunk: 章节标题 -->`

## 长文档报告

共 1103 篇文档超过 500 行，建议拆分:

| 行数 | 文件 |
|---|---|
| 6116 | domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-concepts-reference.md |
| 4638 | domain-17-system-foundation/topic-dictionary/multi-cloud/multi-cloud-operations.md |
| 4517 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.19.md |
| 4365 | domain-10-troubleshooting-diagnostics/topic-febm/08-febm-production-quick-start.md |
| 4324 | domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis.md |
| 4277 | domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md |
| 4204 | domain-03-networking-traffic/10-ebpf-security-applications.md |
| 4122 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.24.md |
| 4114 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.22.md |
| 4094 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.28.md |
| 4063 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.27.md |
| 4015 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.23.md |
| 3977 | domain-10-troubleshooting-diagnostics/topic-febm/06-febm-future-evolution.md |
| 3937 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.26.md |
| 3914 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.32.md |
| 3797 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.20.md |
| 3769 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.29.md |
| 3738 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.31.md |
| 3698 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.25.md |
| 3511 | domain-17-system-foundation/topic-dictionary/tooling/cli-commands.md |
| 3489 | domain-17-system-foundation/topic-dictionary/[[domain-17-system-foundation/topic-dictionary/security/cloud-native-security|cloud-native-security]]-practices.md |
| 3463 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.18.md |
| 3450 | domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md |
| 3420 | domain-18-manifests-patterns/35-advanced-pod-patterns.md |
| 3406 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.30.md |
| 3338 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.7.md |
| 3330 | domain-17-system-foundation/topic-dictionary/operations/incident-management-runbooks.md |
| 3319 | domain-18-manifests-patterns/34-component-configuration.md |
| 3304 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.21.md |
| 3230 | domain-10-troubleshooting-diagnostics/topic-febm/03-febm-best-practices.md |
| 3218 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.17.md |
| 3182 | domain-18-manifests-patterns/36-ecosystem-kustomize-helm-argocd.md |
| 3180 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.10.md |
| 3134 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.8.md |
| 3069 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.16.md |
| 3068 | domain-18-manifests-patterns/03-pod-specification-complete.md |
| 3054 | domain-17-system-foundation/topic-dictionary/operations/operations-best-practices.md |
| 3047 | domain-17-system-foundation/topic-dictionary/operations/performance-tuning-expert.md |
| 2948 | domain-15-specialized-tech/09-wasm-serverless.md |
| 2935 | domain-10-troubleshooting-diagnostics/topic-febm/05-febm-construction-methodology.md |
| 2929 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.6.md |
| 2911 | domain-15-specialized-tech/07-wasm-plugin-system.md |
| 2905 | domain-15-specialized-tech/10-wasm-security-sandbox.md |
| 2905 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.33.md |
| 2863 | domain-15-specialized-tech/08-wasm-ai-inference.md |
| 2840 | domain-14-ai-ml-infra/20-vector-database-rag.md |
| 2840 | domain-17-system-foundation/topic-dictionary/operations/production-troubleshooting-playbook.md |
| 2824 | domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.34.md |
| 2821 | domain-18-manifests-patterns/25-validatingadmissionpolicy.md |
| 2808 | domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md |
