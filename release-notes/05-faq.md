---
title: 发布会 FAQ
description: '**A**: K8s 官方文档是 API 参考和功能说明。kudig-database 是**生产运维实战知识**, 包含:'
category: general
tags:
- k8s
- etcd
- ingress
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布会 FAQ 是什么
- 如何 发布会 FAQ
trigger_keywords:
- 发布会
- FAQ
prerequisites:
- kubectl-basics
- etcd-basics
---

# 发布会 FAQ

> 常见问题与标准回答

---

## Q1: kudig-database 和 [[entities/kubernetes|k8s]] 官方文档有什么区别?

**A**: K8s 官方文档是 API 参考和功能说明。kudig-database 是**生产运维实战知识**, 包含:
- 97 个行业架构方案 (官方文档没有)
- 18 个可执行的问题排查 SOP (官方文档没有)
- 77 个故障树分析 (官方文档没有)
- 生产级 YAML 配置 (官方示例是玩具级)
- 多云对照方案 (官方文档不涉及)

简单说: 官方告诉你"是什么", 我们告诉你"怎么用"。

---

## Q2: 它和网上搜到的 K8s 博客有什么区别?

**A**: 三个核心区别:
1. **结构化**: 每篇文档有标准 YAML front matter (title/difficulty/audience/cross_refs), Agent 可以精准检索; 博客是散文式, Agent 难以理解
2. **系统性**: 40 个知识域形成完整知识图谱, 有依赖关系和学习路径; 博客是碎片化的
3. **可执行**: 18 个 SOP 配套 17 个 shell 脚本, Agent 可以直接调用; 博客只有文字描述

---

## Q3: 怎么接入我的 Agent / RAG 系统?

**A**: 提供 4 种接入方式:
1. **RAG Profile**: 我们提供了 4 套 YAML 配置 (SRE Agent / 学习助手 / 全量语料 / NotebookLM), 直接导入
2. **NotebookLM**: 拖拽 Markdown 文件到 Google NotebookLM 即可
3. **LangChain/LlamaIndex**: 按 RAG 分块策略指南配置 MarkdownHeaderTextSplitter
4. **自定义**: 98% 文件有标准 front matter, 可按 category/tags/difficulty 过滤导入

---

## Q4: 知识库会持续更新吗?

**A**: 是的。更新策略:
- **K8s 版本跟进**: 每个新版本发布后 2 周内更新相关文档
- **CNCF 项目跟进**: 新项目毕业/孵化时补充文档
- **行业场景扩展**: 持续新增行业架构方案
- **QA 对扩充**: 基于用户反馈持续补充真实问答

---

## Q5: 它支持哪些语言?

**A**: 主要是**中文**, 但:
- 所有技术术语保留英文原文 (etcd, Pod, Service, Ingress...)
- 207 个术语词典有 title_en 英文标题
- 代码示例和 YAML 配置是英文
- 适合中文技术团队使用, 英文团队可作为补充参考

---

## Q6: 数据量多大? 对 RAG 系统有什么要求?

**A**:
- **总大小**: 2.8 GB (纯 Markdown 文本)
- **Token 估算**: 约 500M tokens (中文约 1.5 字/token)
- **分块后**: 约 50,000 个 chunk (按 H2 标题分块, chunk_size=2000)
- **推荐**: 先导入核心语料 (SRE Profile 约 5M tokens), 再按需扩展

---

## Q7: 它适合什么规模的团队?

**A**:
- **3 人小团队**: 导入 SRE Profile, 作为问题排查参考
- **10 人运维团队**: 导入全量语料, 构建内部 K8s 知识助手
- **100 人平台部门**: 基于 kudig-database 构建企业级智能运维 Agent
- **云厂商/ISV**: 作为 K8s 技术支持的知识底座

---

## Q8: 和商业 K8s 知识库相比有什么优势?

**A**:
1. **开源免费**: MIT License, 无商业限制
2. **行业最全**: 97 个行业场景 + 218 个 CNCF 项目, 覆盖度远超商业产品
3. **Agent 原生**: 从第一天就为 AI Agent 设计, 不是事后改造
4. **方法论独创**: FTA 故障树 + FEBM 取证循证, 行业独有
5. **可执行**: 18 个 SOP 配套脚本, 不只是文档

---

## Q9: 有成功案例吗?

**A**: 目前已在以下场景验证:
- **K8s 运维 Agent**: 基于 SRE Profile 构建的故障诊断 Agent, 准确率 85%+
- **技术培训**: 28 天培训体系已在内部团队使用
- **架构评审**: 97 个行业场景用于技术方案参考
- **新员工 onboarding**: 4 周学习路径 + 自测题

---

## Q10: 我想贡献内容, 怎么参与?

**A**:
- **提 Issue**: 报告错误或建议新增主题
- **提 PR**: 按 templates/ 目录下的模板编写文档
- **补充 QA**: 为你的专业领域补充真实问答
- **反馈使用体验**: 告诉我们 Agent 在哪些场景命中率低
