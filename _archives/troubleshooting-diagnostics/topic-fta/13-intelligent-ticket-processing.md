---
title: 第十三章：智能工单处理的 AI Agent 架构
description: '# 第十三章：智能工单处理的 AI Agent 架构'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- etcd
- prometheus
- grafana
- mysql
- ingress
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 第十三章：智能工单处理的 AI Agent 架构 是什么
- 如何 第十三章：智能工单处理的 AI Agent 架构
- 第十三章：智能工单处理的 AI Agent 架构 根因分析
- 第十三章：智能工单处理的 AI Agent 架构 故障树
trigger_keywords:
- 第十三章：智能工单处理的
- AI
- Agent
- 架构
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- monitoring-basics
- etcd-basics
- mysql-basics
- logging-basics
---

# 第十三章：智能工单处理的 AI Agent 架构

> **所属部分**: 第三部分 - FTA 在 AI Agent 智能运维中的应用  
> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: [第十二章：FTA 与 AIOps 平台集成架构](./12-fta-aiops-integration.md)  
> **下一章**: [第十四章：构建 FTA 系统的工程化方法](./14-fta-system-engineering.md)

---

## 13.1 工单生命周期全自动化

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    智能工单处理全生命周期                                  │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  用户报障          FTA匹配          Agent诊断         自动修复           │
│  ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐          │
│  │ 工单提交 │────►│ NLP理解 │────►│ 树遍历  │────►│ 执行修复 │          │
│  │ 或告警   │     │ 意图识别│     │ 根因定位│     │ 验证恢复│          │
│  └─────────┘     └─────────┘     └─────────┘     └────┬────┘          │
│       │               │               │                │               │
│       ▼               ▼               ▼                ▼               │
│  ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐          │
│  │ 信息提取 │     │ 顶事件  │     │ 证据收集 │     │ 效果验证 │          │
│  │ 关键词   │     │ 映射    │     │ 概率推理 │     │ 工单关闭 │          │
│  │ 实体识别 │     │ 优先级  │     │ 方案选择 │     │ 学习反馈 │          │
│  └─────────┘     └─────────┘     └─────────┘     └─────────┘          │
│                                                                          │
│  MTTD: < 1min    匹配: < 5s      诊断: < 3min    修复: < 5min          │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

## 13.2 NLP 意图识别与 FTA 映射

```python
class TicketToFTAMapper:
    """将工单描述映射到 FTA 顶事件和可能的底事件"""
    
    # 关键词 → FTA 事件映射规则
    KEYWORD_RULES = {
        # 网络相关
        "dns": ["TE-4", "BE-4.1"],
        "timeout": ["TE-4", "TE-2"],
        "connection refused": ["TE-4", "TE-2"],
        "502": ["TE-2", "IE-2.3"],
        "503": ["TE-2", "IE-2.2"],
        "network unreachable": ["TE-4", "IE-4.1"],
        
        # [[concepts/pod-lifecycle|pod]] 相关
        "crashloopbackoff": ["TE-3", "BE-2.1"],
        "oomkilled": ["TE-3", "BE-2.3"],
        "imagepullbackoff": ["TE-3", "BE-3.5"],
        "pending": ["TE-3", "IE-3.1"],
        "evicted": ["TE-3", "BE-2.4"],
        
        # 存储相关
        "pvc": ["TE-5", "IE-5.1"],
        "mount": ["TE-5", "BE-5.3"],
        "disk full": ["TE-5", "BE-5.7"],
        
        # 控制平面
        "kubectl": ["TE-1", "IE-1.1"],
        "api server": ["TE-1", "BE-1.1"],
        "etcd": ["TE-1", "BE-1.2"],
        
        # 安全
        "certificate": ["TE-7", "BE-7.1"],
        "unauthorized": ["TE-7", "BE-7.3"],
        "forbidden": ["TE-7", "BE-7.3"],
    }
    
    def map_ticket(self, ticket_text):
        """
        输入: 工单描述文本
        输出: FTA 事件匹配列表 (按置信度排序)
        """
        text_lower = ticket_text.lower()
        matches = {}
        
        for keyword, fta_events in self.KEYWORD_RULES.items():
            if keyword in text_lower:
                for event_id in fta_events:
                    if event_id not in matches:
                        matches[event_id] = 0
                    matches[event_id] += 1  # 匹配次数越多，置信度越高
        
        # 按匹配次数排序
        ranked = sorted(matches.items(), key=lambda x: x[1], reverse=True)
        
        return [
            {"fta_event": event_id, "confidence": min(count * 0.3, 0.95)}
            for event_id, count in ranked
        ]
```

**工单处理示例**：

```
输入工单:
  "生产环境 order-service 频繁报 502 错误，日志里看到大量 OOM 错误，
   Pod 一直在重启，已经影响线上用户下单了"

NLP 提取:
  关键词: ["502", "oom", "pod重启", "生产环境"]
  实体: [service=order-service, env=production]
  情感: 紧急

FTA 映射:
  ┌──────────┬──────────┬────────────────────────────────┐
  │ FTA事件   │ 置信度   │ 匹配依据                       │
  ├──────────┼──────────┼────────────────────────────────┤
  │ TE-2     │ 0.95    │ 502 + 影响用户 = 服务不可用     │
  │ BE-2.3   │ 0.90    │ OOM + Pod重启 = OOMKilled       │
  │ TE-3     │ 0.60    │ Pod重启 = Pod启动失败           │
  │ BE-2.1   │ 0.55    │ Pod重启 = CrashLoopBackOff      │
  └──────────┴──────────┴────────────────────────────────┘

Agent 决策:
  → 优先诊断 TE-2 → BE-2.3 (OOMKilled) 路径
  → 置信度 0.90，直接跳过中间层探索
```

## 13.3 人机协同分级模型

| 问题级别 | FTA 特征 | Agent 角色 | 人类角色 | 协同方式 | 自动化率 |
|---------|---------|-----------|---------|---------|---------|
| **常见问题** | FTA 路径明确，置信度 > 0.9 | 全自动处理 | 事后审计 | Agent 独立闭环 | 95% |
| **普通问题** | FTA 路径存在，置信度 0.7-0.9 | 诊断+方案推荐 | 确认方案后执行 | Agent 主导，人工确认 | 70% |
| **复杂问题** | FTA 多条路径候选，置信度 < 0.7 | 数据采集+分析 | 决策和执行 | 人工主导，Agent 辅助 | 30% |
| **未知问题** | FTA 无匹配路径 | 尽力收集信息 | 全程主导 | 人工诊断，Agent 学习 | 5% |

**升级机制**：

```
自动化处理失败 → 自动升级到人工

升级条件:
  1. Agent 修复执行失败 (连续 2 次)
  2. 修复后验证未通过
  3. 高风险操作需要人工审批
  4. FTA 置信度 < 0.5 (无法确认根因)
  5. 顶事件为 P0 且 5 分钟内未恢复

升级流程:
  Agent → ChatOps 消息 → On-Call SRE → (必要时) → Team Lead → Director

ChatOps 升级消息模板:
  ──────────────────────────────────────
  [P0 升级] order-service 服务不可用
  
  Agent 诊断结论:
  - 疑似根因: BE-2.3 OOMKilled (置信度: 87%)
  - 诊断路径: TE-2 → IE-2.1 → BE-2.3
  
  已尝试修复:
  - HA-2.3.1: 增加内存limit → 失败 (Pod仍然OOM)
  - HA-2.3.2: 重启Pod → 失败 (启动后立即OOM)
  
  建议人工介入:
  - 可能存在严重内存泄漏，需要开发团队排查
  - 考虑回滚到上一个稳定版本
  
  相关信息:
  - 工单: INC-2026-0225-042
  - Grafana: https://grafana.example.com/d/xxx
  - 日志: https://loki.example.com/explore?...
  ──────────────────────────────────────
```

## 13.4 完整工单自动处理案例

**场景**：用户工单 "数据库连接超时，应用无法正常工作"

```
═══════════════════════════════════════════════════════════════
  工单自动处理全流程
═══════════════════════════════════════════════════════════════

[T+0s] 工单接收
  ┌─────────────────────────────────────────────────────┐
  │ Ticket ID:    INC-2026-0225-088                     │
  │ Reporter:     developer@example.com                 │
  │ Description:  "生产环境 payment-service 报错        │
  │               connection timeout to mysql-primary,   │
  │               大量用户支付失败"                       │
  │ Priority:     P1                                    │
  └─────────────────────────────────────────────────────┘

[T+3s] NLP 意图识别
  关键词: ["connection timeout", "mysql", "支付失败"]
  FTA 映射:
    TE-2 应用服务不可用 (置信度: 0.88)
    TE-4 网络通信异常 (置信度: 0.72)
    BE-4.9 防火墙/NetworkPolicy阻止 (置信度: 0.45)

[T+5s] Agent 开始 FTA 导航 (从 TE-2 开始)
  
  TE-2 [OR门] → 并行检查 3 个中间事件:
  
  Agent-A: 检查 IE-2.1 Pod运行异常
    → kubectl get pods -n production -l app=payment-service
    → 3/3 Running, 但 RESTARTS: 5 (最近1小时)
    → 结论: Pod 在运行但不稳定
  
  Agent-B: 检查 IE-2.2 Service/Endpoint异常
    → kubectl get ep payment-service -n production
    → 3 个 Endpoint 正常
    → 结论: Service 层正常
    
  Agent-C: 检查 IE-2.3 Ingress异常
    → kubectl describe ingress payment-ingress -n production
    → Ingress 配置正常
    → 结论: Ingress 层正常

[T+15s] 深入 IE-2.1 Pod运行异常
  
  → kubectl logs payment-service-xxx --tail=100
  发现大量错误:
    ERROR: com.zaxxer.hikari.pool.HikariPool - Connection not available
    ERROR: java.sql.SQLException: Connection timed out to mysql-primary:3306
    WARN: HikariPool-1 - Connection pool exhausted (max=10, active=10, idle=0)

[T+20s] 切换到 TE-4 网络通信异常路径

  Agent-D: 检查 mysql-primary 可达性
    → kubectl exec payment-service-xxx -- nc -zv mysql-primary 3306
    → Connection timed out
    
  Agent-E: 检查 mysql-primary 本身状态
    → kubectl get pods -n database -l app=mysql,role=primary
    → mysql-primary-0: 1/1 Running
    
  Agent-F: 检查网络策略
    → kubectl get networkpolicy -n database
    → 发现 NetworkPolicy "db-access-policy" 最近被修改!
    
    → kubectl describe networkpolicy db-access-policy -n database
    发现:
      ingress:
        - from:
          - namespaceSelector:
              matchLabels:
                access: database  ← payment namespace 缺少此标签!
    
    → kubectl get namespace production --show-labels
      NAME        STATUS   AGE   LABELS
      production  Active   180d  team=platform  ← 缺少 access=database 标签

[T+40s] Agent 确认根因
  根因: NetworkPolicy 配置变更导致 payment-service 无法访问 MySQL
  FTA 路径: TE-4 → IE-4.2 → BE-4.6 (NetworkPolicy 阻止)
  置信度: 0.95
  
  根因分析:
    - 近期有人修改了 db-access-policy NetworkPolicy
    - 新增了 namespaceSelector 要求 access=database 标签
    - production namespace 没有这个标签
    - 导致 payment-service 的 MySQL 连接被 NetworkPolicy 拦截

[T+45s] 执行修复
  修复方案: 为 production namespace 添加缺失的标签
  
  → kubectl label namespace production access=database
  
  → 等待网络策略生效 (5s)
  
  → 验证: kubectl exec payment-service-xxx -- nc -zv mysql-primary 3306
  → mysql-primary [10.0.5.100] 3306: Connection succeeded

[T+65s] 验证恢复
  → 检查 Pod 日志: 无新的连接错误
  → 检查 Prometheus: payment_service_request_success_rate > 99.9%
  → 检查 HikariPool: active_connections < max_connections

[T+70s] 自动更新工单
  ┌─────────────────────────────────────────────────────┐
  │ Ticket ID:    INC-2026-0225-088                     │
  │ Status:       Resolved                              │
  │ Resolution:                                         │
  │   根因: NetworkPolicy db-access-policy 配置变更     │
  │   导致 production namespace 被拦截访问 MySQL        │
  │                                                     │
  │   修复: kubectl label namespace production          │
  │         access=database                              │
  │                                                     │
  │   MTTR: 70 秒                                       │
  │   FTA 路径: TE-4 → IE-4.2 → BE-4.6                │
  │                                                     │
  │ Follow-up:                                          │
  │   - 建议将 NetworkPolicy 变更纳入 GitOps 审批流程   │
  │   - 建议添加 NetworkPolicy 变更告警                  │
  └─────────────────────────────────────────────────────┘

[T+75s] 学习反馈
  → 新增 FTA 知识: NetworkPolicy 变更是网络问题的常见触发源
  → BE-4.6 概率上调 (本月第 2 次)
  → 建议: 将 NetworkPolicy 变更纳入变更管理系统
```

---

> **导航**: [<< 上一章 - FTA 与 AIOps 平台集成架构](./12-fta-aiops-integration.md) | [下一章 - 构建 FTA 系统的工程化方法 >>](./14-fta-system-engineering.md)
