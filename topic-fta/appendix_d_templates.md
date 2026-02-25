# 附录 D：FTA 模板与检查表

> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta_methodology_and_agentic_practices.md)  
> **上一附录**: [附录 C：参考文献](./appendix_c_references.md)

---

## D.1 FTA 顶事件定义模板

```yaml
# 顶事件定义模板
top_event:
  id: "TE-{序号}"
  name: "{描述性名称}"
  severity: "P{0-3}"
  description: "{详细描述}"
  
  slo_mapping:
    indicator: "{SLI 名称}"
    target: "{SLO 目标值}"
    consequence: "{SLO 违约后果}"
  
  impact:
    users: "{受影响用户范围}"
    services: "{受影响服务列表}"
    business: "{业务影响描述}"
  
  response:
    sla: "{响应时间要求}"
    notification: "{通知方式和对象}"
    escalation: "{升级路径}"
```

## D.2 底事件定义模板

```yaml
# 底事件定义模板
basic_event:
  id: "BE-{顶事件序号}.{底事件序号}"
  name: "{描述性名称}"
  description: "{详细描述}"
  
  observability:
    metrics:
      - expression: "{PromQL 表达式}"
        threshold: "{阈值}"
        severity: "{告警级别}"
    logs:
      - pattern: "{日志匹配模式}"
        component: "{来源组件}"
    events:
      - type: "{K8s Event 类型}"
        reason: "{Event Reason}"
  
  probability:
    annual_rate: {年故障率}
    mttr_minutes: {平均修复时间}
    data_source: "{数据来源}"
  
  root_causes:
    - "{可能原因 1}"
    - "{可能原因 2}"
  
  diagnosis_commands:
    - "{诊断命令 1}"
    - "{诊断命令 2}"
  
  healing_actions:
    - id: "HA-{关联底事件}.{序号}"
      name: "{动作名称}"
      risk_level: "{low|medium|high}"
      auto_healable: {true|false}
      command: "{执行命令}"
      verification: "{验证命令}"
```

## D.3 FTA 评审检查表

```
FTA 评审检查表 (Review Checklist)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

结构完整性:
  □ 顶事件定义清晰，与 SLO 关联
  □ 所有中间事件都有子事件
  □ 所有底事件都是叶子节点
  □ 没有悬挂的孤立事件
  □ 没有循环依赖

逻辑正确性:
  □ 逻辑门类型选择正确 (OR vs AND)
  □ 同一门下的子事件满足 MECE 原则
  □ 同一门下的子事件满足独立性原则
  □ 层数在 3-5 层之间

可观测性:
  □ 每个底事件至少有 1 个指标监控
  □ 每个底事件至少有 1 种诊断命令
  □ 每个底事件有明确的判定条件
  □ 告警规则与 FTA 事件正确关联

可维护性:
  □ 编号遵循规范 (TE-/IE-/BE- 前缀)
  □ 命名专业准确，无歧义
  □ 每个子树有明确的 Owner
  □ 概率数据有标注来源
  □ 修复动作有风险分级

Agent 友好性:
  □ 每个底事件有结构化的修复动作
  □ 修复动作标注了自动化程度
  □ 高风险操作有审批标记
  □ 验证条件可自动化判定
```

---

> **导航**: [<< 附录 C - 参考文献](./appendix_c_references.md) | [返回主文档](./fta_methodology_and_agentic_practices.md)
