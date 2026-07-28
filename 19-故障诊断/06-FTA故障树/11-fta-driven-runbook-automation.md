---
title: 第十一章：FTA 驱动的 Runbook 自动化
description: '**所属部分**: 第三部分 - FTA 在 AI Agent 智能运维中的应用'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- etcd
- rag
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
- 第十一章：FTA 驱动的 Runbook 自动化 是什么
- 如何 第十一章：FTA 驱动的 Runbook 自动化
- 第十一章：FTA 驱动的 Runbook 自动化 根因分析
- 第十一章：FTA 驱动的 Runbook 自动化 故障树
trigger_keywords:
- 第十一章：FTA
- 驱动的
- Runbook
- 自动化
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
tier: supporting
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 第十一章：FTA 驱动的 Runbook 自动化

> **所属部分**: 第三部分 - FTA 在 AI Agent 智能运维中的应用  
> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: [第十章：Agent 编排模式与 FTA 逻辑门映射](./10-agent-orchestration-patterns.md)  
> **下一章**: [第十二章：FTA 与 AIOps 平台集成架构](./12-fta-aiops-integration.md)

---

## 11.1 从 FTA 底事件自动生成 Runbook

传统 Runbook 由运维工程师手工编写并维护。FTA 的结构化底事件天然包含生成 Runbook 所需的全部信息。

**自动生成算法**：

```python
def generate_runbook(basic_event):
    """从 FTA 底事件自动生成 Runbook"""
    runbook = {
        "id": f"RB-{basic_event.id}",
        "name": f"Runbook: {basic_event.name}",
        "trigger": basic_event.alert_rules,
        "steps": []
    }
    
    # 1. 诊断步骤 (来自 FTA 诊断命令)
    for i, cmd in enumerate(basic_event.diagnosis_commands):
        runbook["steps"].append({
            "step": i + 1,
            "type": "diagnosis",
            "name": f"确认 {basic_event.name}",
            "command": cmd,
            "expected": basic_event.observable_criteria
        })
    
    # 2. 修复步骤 (来自 FTA 修复动作,按风险排序)
    for action in sorted(basic_event.healing_actions,
                         key=lambda a: a.risk_level):
        runbook["steps"].append({
            "type": "healing",
            "name": action.name,
            "command": action.command,
            "risk_level": action.risk_level,
            "auto_executable": action.auto_healable,
            "requires_approval": action.risk_level == "high",
            "timeout": action.timeout
        })
    
    # 3. 验证步骤
    for verification in basic_event.verification_commands:
        runbook["steps"].append({
            "type": "verification",
            "name": "验证恢复",
            "command": verification.command,
            "expected": verification.expected_result
        })
    
    # 4. 回滚步骤
    if basic_event.rollback_action:
        runbook["rollback"] = basic_event.rollback_action
    
    return runbook
```

## 11.2 结构化 Runbook 示例

**示例：etcd 集群问题 Runbook**（从 FTA BE-1.2 自动生成）：

```yaml
runbook:
  id: RB-BE-1.2
  name: "Runbook: etcd 集群故障诊断与修复"
  version: "2.1.0"
  owner: "platform-sre-team"
  fta_event: BE-1.2
  
  trigger:
    alerts:
      - name: etcdNoLeader
        expr: "etcd_server_has_leader == 0"
      - name: etcdHighDiskUsage  
        expr: "etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes > 0.8"
  
  healing_actions:
    - id: HA-1.2.1
      name: "etcd 数据碎片整理"
      risk_level: medium
      auto_healable: true
      requires_approval: false
      steps:
        - name: "检查 etcd 数据库大小"
          cmd: "kubectl exec -n kube-system etcd-master-1 -- etcdctl endpoint status --cluster -w table"
          timeout: 10s
        - name: "执行碎片整理"
          cmd: "kubectl exec -n kube-system etcd-master-1 -- etcdctl defrag --cluster"
          timeout: 120s
        - name: "解除告警"
          cmd: "kubectl exec -n kube-system etcd-master-1 -- etcdctl alarm disarm"
          timeout: 10s
      verification:
        - cmd: "kubectl exec -n kube-system etcd-master-1 -- etcdctl endpoint health --cluster"
          expected: "所有 endpoint is healthy"
        - cmd: "kubectl get componentstatus"
          expected: "etcd-0 Healthy"
      rollback:
        description: "如果碎片整理失败，恢复 etcd 快照"
        cmd: "etcdctl snapshot restore /backup/etcd-latest.snap"

    - id: HA-1.2.2
      name: "etcd leader 重新选举"
      risk_level: high
      auto_healable: true
      requires_approval: true  # 高风险，需要人工确认
      steps:
        - name: "检查网络连通性"
          cmd: |
            for member in master-1 master-2 master-3; do
              kubectl exec -n kube-system etcd-$member -- ping -c 3 etcd-${member}
            done
          timeout: 30s
        - name: "如果网络正常，重启异常 member"
          cmd: "kubectl delete [[22-概念/02-工作负载/pod-lifecycle|pod]] -n kube-system etcd-master-1"
          timeout: 60s
      verification:
        - cmd: "kubectl exec -n kube-system etcd-master-1 -- etcdctl endpoint status --cluster -w table"
          expected: "存在一个 IS LEADER = true 的节点"

    - id: HA-1.2.3
      name: "清理 etcd 磁盘空间"
      risk_level: low
      auto_healable: true
      requires_approval: false
      steps:
        - name: "清理旧快照"
          cmd: "kubectl exec -n kube-system etcd-master-1 -- find /var/lib/etcd/member/snap -name '*.snap' -mtime +7 -delete"
          timeout: 30s
        - name: "清理 WAL 日志"
          cmd: "kubectl exec -n kube-system etcd-master-1 -- find /var/lib/etcd/member/wal -name '*.wal' -mtime +3 -delete"
          timeout: 30s
      verification:
        - cmd: "kubectl exec -n kube-system etcd-master-1 -- df -h /var/lib/etcd"
          expected: "磁盘使用率 < 80%"
```

## 11.3 Runbook 与 Agent 的集成

```
传统 Runbook 执行:
  人工 → 阅读文档 → 逐步执行命令 → 判断结果 → 决定下一步
  问题: 慢、易出错、无法 7×24

FTA-Agent Runbook 执行:
  告警 → Agent 自动加载 Runbook → 自动执行 → 自动判断 → 自动决策
  优势: 快、一致性高、全天候

集成架构:
  ┌────────────┐     ┌────────────┐     ┌────────────┐
  │ FTA 知识库  │────►│ Runbook    │────►│ Agent      │
  │            │     │ 自动生成器  │     │ 执行引擎   │
  │ 底事件     │     │            │     │            │
  │ 诊断命令   │     │ 结构化     │     │ 自动执行   │
  │ 修复动作   │     │ Runbook    │     │ 结果判定   │
  │ 验证条件   │     │ 库         │     │ 异常处理   │
  └────────────┘     └────────────┘     └────────────┘

关键设计:
  1. Runbook 以 YAML/JSON 存储 (机器可读)
  2. 每个步骤有明确的成功/失败判定条件
  3. 失败时有回滚机制
  4. 高风险步骤标记需人工审批
  5. 执行日志完整记录，用于 Postmortem
```

---

> **导航**: [<< 上一章 - Agent 编排模式与 FTA 逻辑门映射](./10-agent-orchestration-patterns.md) | [下一章 - FTA 与 AIOps 平台集成架构 >>](./12-fta-aiops-integration.md)


<!-- risk-assessed -->
