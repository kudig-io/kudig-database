---
title: 附录 C：参考文献 [故障诊断]
description: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
summary: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
category: fta
tags:
- fta
- troubleshooting
- reference
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 附录 C：参考文献 是什么
- 如何 附录 C：参考文献
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 附录 C：参考文献 故障排查
- 附录 C：参考文献 排障步骤
- 附录 C：参考文献 根因分析
trigger_keywords:
- 附录
- C：参考文献
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
fta_id: FTA-APPENDIX_C_REFERENCES-001
component: Appendix C References
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 附录 C：参考文献
description: '**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 附录 C：参考文献 是什么
- 如何 附录 C：参考文献
- 附录 C：参考文献 根因分析
- 附录 C：参考文献 故障树
trigger_keywords:
- 附录
- C：参考文献
- fta
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# 附录 C：参考文献

> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一附录**: 附录 B：工具与资源清单](./[[19-故障诊断/06-FTA故障树/appendix-b-tools-and-resources.md|appendix-b-tools-and-resources]].md)  
> **下一附录**: [附录 D：FTA 模板与检查表](./appendix-d-templates.md)

---

## 标准与规范

1. IEC 61025:2006 - Fault tree analysis (FTA)
2. IEC 61508:2010 - Functional safety of E/E/PE safety-related systems
3. ISO 26262:2018 - Road vehicles -- Functional safety
4. MIL-STD-1629A - Procedures for performing a FMECA
5. IEEE Std 352 - Reliability Analysis of Nuclear Power Generating Station Protection Systems
6. ARP 4761 - Guidelines for conducting the safety assessment process on civil airborne systems
7. NIST SP 800-30 - Guide for Conducting Risk Assessments

## 书籍

1. Kumamoto, H. & Henley, E.J. (1996). "Probabilistic Risk Assessment and Management for Engineers and Scientists"
2. Ericson, C.A. (2015). "Hazard Analysis Techniques for System Safety" (2nd Edition)
3. Stamatelatos, M. et al. (2002). "Fault Tree Handbook with Aerospace Applications" - NASA
4. Beeson, B. & Murphy, N. et al. (2016). "Site Reliability Engineering" - Google/O'Reilly
5. Beyer, B. et al. (2018). "The Site Reliability Workbook" - Google/O'Reilly

## 论文与白皮书

1. Watson, H.A. (1961). "Launch Control Safety Study" - Bell Telephone Laboratories
2. WASH-1400 (NUREG-75/014). "Reactor Safety Study" - U.S. Nuclear Regulatory Commission
3. Marzal, E. et al. (2020). "AIOps: Challenges and Trends" - IEEE ICSOC
4. Notaro, P. et al. (2021). "A Survey of AIOps Methods for Failure Management" - ACM Computing Surveys
5. CNCF Observability Whitepaper (2024)
6. CNCF Cloud Native Chaos Engineering Whitepaper (2024)

---

> **导航**: [<< 附录 B - 工具与资源清单](./appendix-b-tools-and-resources.md) | [附录 D - FTA 模板与检查表 >>](./appendix-d-templates.md)

---

## Obsidian 相关文档

- [[19-故障诊断/06-FTA故障树/MOC.md|topic-fta MOC]]
- [[19-故障诊断/06-FTA故障树/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[19-故障诊断/06-FTA故障树/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[19-故障诊断/06-FTA故障树/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[19-故障诊断/06-FTA故障树/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[19-故障诊断/06-FTA故障树/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[19-故障诊断/06-FTA故障树/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[19-故障诊断/06-FTA故障树/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[19-故障诊断/06-FTA故障树/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[19-故障诊断/06-FTA故障树/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[19-故障诊断/06-FTA故障树/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[19-故障诊断/06-FTA故障树/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|appendix-a-glossary]]
- [[19-故障诊断/06-FTA故障树/appendix-b-tools-and-resources.md|appendix-b-tools-and-resources]]
- [[19-故障诊断/06-FTA故障树/appendix-d-templates.md|appendix-d-templates]]
- [[19-故障诊断/06-FTA故障树/fta-diagnosis-improvement.md|fta-diagnosis-improvement]]

## Related

- [[reference|#reference Hub]] — tag hub


<!-- risk-assessed -->
