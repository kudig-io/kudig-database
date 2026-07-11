---
title: 灾备自动化手册
description: 灾备切换的自动化与 runbook 化：Argo Workflow 编排、健康门控、DNS 自动切换
summary: Argo Workflow + 健康检查 + DNS failover 把灾备切换从 2 小时人工缩到 5 分钟自动化
category: reliability
tags:
- slo
- sli
- reliability
- disaster-recovery
- automation
- runbook
- argo
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 灾备自动化手册

> **核心原则**：灾备切换是"低频高紧张"操作——平时不练、出事才用，正是最不该靠人手记步骤的场景。**每个灾备动作都应是可执行代码**（Argo Workflow + 健康门控），人工只做"按按钮"和"确认结果"，绝不现场翻 wiki 想 `kubectl` 命令。自动化的目标是把 RTO 从 2 小时压到 5 分钟。

## 自动化目标：5 分钟切换

```
人工灾备（典型）：
  找 runbook(10m) → 改 DNS(5m) → 扩容备区(15m) → 验证(20m) = 50m+

自动化灾备：
  触发(10s) → 健康门控(30s) → Workflow 执行(3m) → 验证(1m) = 5m
```

## 自动化编排：Argo Workflow

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata: { name: dr-failover, namespace: dr }
spec:
  entrypoint: failover
  arguments:
    parameters:
    - { name: target_region, value: "region-b" }
    - { name: reason, value: "primary degraded" }
  templates:
  - name: failover
    steps:
    - - name: pre-checks                 # ① 前置门控
        template: pre-checks
    - - name: scale-up-standby            # ② 备区扩容
        template: scale-up
        arguments: { parameters: [{ name: region, value: "{{workflow.parameters.target_region}}" }] }
    - - name: wait-ready                  # ③ 等待就绪
        template: wait-ready
    - - name: verify-replication          # ④ 复制延迟门控
        template: replication-gate
    - - name: switch-dns                  # ⑤ 切流量
        template: dns-switch
    - - name: verify-slo                  # ⑥ 验证 SLO
        template: slo-verify
    - - name: notify                      # ⑦ 通知
        template: notify

  - name: pre-checks
    container:
      image: dr-tools:latest
      command: [sh, -c]
      args:
        - |
          # 备区健康 + 复制通道在线 + 没有正在进行的发布
          check-region-health --region region-b || exit 1
          check-replication-channel || exit 1
          check-no-active-deploy || exit 1

  - name: replication-gate
    container:
      image: dr-tools:latest
      command: [sh, -c]
      args:
        - |
          # 🔴 硬门控：复制延迟必须 < RPO 承诺才能切
          LAG=$(query-prometheus 'pg_replication_lag_seconds{region="primary"}')
          awk "BEGIN{exit !($LAG > 60)}" && { echo "FAIL lag=${LAG}s > 60s"; exit 1; }
```

## 健康门控（绝不跳过）

每个步骤必须 pass 才进下一步，**任一门控失败即中止 Workflow 并告警**：

| 门控 | 检查内容 | 失败动作 |
|------|---------|---------|
| 备区健康 | 控制面 + 关键服务 Running | 中止 |
| 复制延迟 | DB lag < RPO 承诺 | 中止 |
| 无活跃变更 | 无进行中发布/迁移 | 中止 |
| 资源充足 | 备区有足够配额扩容 | 中止 |
| SLO 绿区 | 切换后 5 分钟内 SLO 恢复 | 回滚 + 告警 |

## DNS 自动切换

```yaml
# dns-switch 模板
- name: dns-switch
  container:
    image: dr-tools:latest
    command: [sh, -c]
    args:
      - |
        # 🔴 高危：修改全局 DNS，影响所有用户
        # 双人审批通过 Workflow 的 manualApproval 步骤触发
        aws route53 change-resource-record-sets \
          --hosted-zone-id "$ZONE_ID" \
          --change-batch file://dns-{{workflow.parameters.target_region}}.json
        # 验证 DNS 生效
        sleep 30
        dig +short api.example.com @8.8.8.8 | grep "$REGION_B_IP"
```

🔴 **高危**：DNS 切换必须有人工审批节点（Argo `manualApproval`），全自动化无审批 = 自杀开关。

## 触发方式三选一

1. **手动触发**（推荐）：on-call 在事故中 `kubectl submit` Workflow，自动化执行细节。
2. **半自动触发**：Prometheus 检测到主区 Sev1，自动开 Incident + 准备好 Workflow 但等人按按钮。
3. **全自动触发**：仅对低风险服务（如静态站点），核心服务绝不全自动。

```bash
# 🟡 中危：触发灾备切换
argo submit dr-failover.yaml \
  -p target_region=region-b \
  -p reason="primary region network outage" \
  --namespace dr
```

## 切换后验证（SLO 门控）

```yaml
- name: slo-verify
  container:
    image: prometheus-checker:latest
    command: [sh, -c]
    args:
      - |
        # 切换后 5 分钟内 SLO 必须回绿，否则自动回滚 DNS
        for i in 1 2 3 4 5; do
          sleep 60
          verify-slo --service api --window 1m && exit 0
        done
        echo "FAIL: SLO 未恢复，触发回滚"
        kubectl create job --from=workflow/dns-rollback dns-rollback-$(date +%s)
        exit 1
```

## 灾备 runbook 模板（每服务一份）

```markdown
# DR Runbook: <服务名>
- RTO 承诺: 5 min
- RPO 承诺: 60 s
- 主区: region-a
- 备区: region-b
- 触发命令: argo submit dr-failover.yaml -p target_region=region-b
- 回滚命令: argo submit dr-failback.yaml -p target_region=region-a
- 复制延迟监控: dashboard db/replication-lag
- 负责人: @team-payment
- 上次演练: 2026-04-15 (通过)
```

## 常见陷阱

1. **全自动无审批**：DNS 自动切 = 一个误告警就能把全站切挂。核心服务必须人工按按钮。
2. **门控被"先切再说"绕过**：演练时图省事跳过复制延迟检查，事故时就敢真跳 → 数据丢失。
3. **没自动化回滚**：切过去发现没好，回不来。回滚 Workflow 必须和切换一起设计、一起演练。
4. **runbook 只在 wiki**：事故中 wiki 登不上、找不到。runbook 必须是可执行代码，不是文档。
5. **演练用假数据**：演练流量/数据与生产差异大，掩盖真实问题。定期做真实流量灰度切换。

## 相关

- [[可靠性/灾难恢复/01-multi-region-dr-architecture.md|01 multi region dr architecture]]
- [[可靠性/灾难恢复/20-automated-dr-patterns-2025.md|20 automated dr patterns 2025]]
- [[可靠性/灾难恢复/17-disaster-recovery-drills.md|17 disaster recovery drills]]
- [[可靠性/SRE实践/07-incident-command-field-guide.md|07 incident command field guide]]

<!-- risk-assessed -->
