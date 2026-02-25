# 附录 A：FTA 术语表

> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta_methodology_and_agentic_practices.md)  
> **上一章**: [第二十二章：行业标准化建议](./22_industry_standardization.md)  
> **下一附录**: [附录 B：工具与资源清单](./appendix_b_tools_and_resources.md)

---

| 中文术语 | 英文术语 | 缩写 | 定义 |
|---------|---------|------|------|
| 故障树分析 | Fault Tree Analysis | FTA | 自顶向下的演绎式系统安全分析方法 |
| 顶事件 | Top Event | TE | 故障树最顶层的不期望事件 |
| 中间事件 | Intermediate Event | IE | 故障传播链中的中间层事件 |
| 底事件/基本事件 | Basic Event | BE | 不可再分解的最底层故障事件 |
| 或门 | OR Gate | - | 任一输入发生则输出发生 |
| 与门 | AND Gate | - | 全部输入发生则输出发生 |
| 最小割集 | Minimal Cut Set | MCS | 使顶事件发生的最小底事件集合 |
| 割集阶数 | Cut Set Order | - | 最小割集中底事件的数量 |
| 重要度 | Importance Measure | - | 底事件对顶事件的影响程度 |
| 平均故障间隔 | Mean Time Between Failures | MTBF | 系统两次故障之间的平均时间 |
| 平均修复时间 | Mean Time To Repair | MTTR | 从故障发生到恢复的平均时间 |
| 平均检测时间 | Mean Time To Detect | MTTD | 从故障发生到被检测到的平均时间 |
| 可用性 | Availability | A | 系统正常运行的时间比例 |
| 可靠度 | Reliability | R(t) | 系统在时间 t 内无故障运行的概率 |
| 故障率 | Failure Rate | λ | 单位时间内发生故障的概率 |
| 风险优先级数 | Risk Priority Number | RPN | 严重度 x 发生频率 x 可检测性 |
| 故障模式与影响分析 | Failure Mode and Effects Analysis | FMEA | 自底向上的归纳式分析方法 |
| 共因故障 | Common Cause Failure | CCF | 由同一根因导致的多个组件故障 |
| 外部事件/房屋事件 | House Event | HE | 正常预期会发生的事件 |
| 未展开事件 | Undeveloped Event | UE | 暂未分解到底的事件 |
| 投票门 | Voting Gate | k/n | n 个输入中至少 k 个发生 |
| 抑制门 | Inhibit Gate | - | 带条件约束的 AND 门 |
| 优先与门 | Priority AND Gate | PAND | 按时序发生的 AND 门 |
| 转移符号 | Transfer Symbol | - | 故障树跨页连接标记 |

---

> **导航**: [<< 第二十二章 - 行业标准化建议](./22_industry_standardization.md) | [附录 B - 工具与资源清单 >>](./appendix_b_tools_and_resources.md)
