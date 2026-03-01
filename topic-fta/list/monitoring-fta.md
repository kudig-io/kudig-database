# 监控与告警异常 FTA 树

## 适用范围与说明
- **目标**：覆盖监控采集失败、告警不触发与数据丢失的关键成因与路径。
- **范围**：Prometheus 采集、目标发现、告警规则、存储与远程写入。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: 监控/告警异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> SCRAPE[采集异常]
  OR0 --> DISC[服务发现异常]
  OR0 --> ALERT[告警规则异常]
  OR0 --> STORE[存储异常]
  OR0 --> REMOTE[远程写入异常]

  SCRAPE_OR{{OR}}
  SCRAPE --> SCRAPE_OR
  SCRAPE_OR --> S1[Target 不可达]
  SCRAPE_OR --> S2[采集超时]

  DISC_OR{{OR}}
  DISC --> DISC_OR
  DISC_OR --> D1[Service/Endpoint 发现失败]
  DISC_OR --> D2[RBAC 权限不足]

  ALERT_OR{{OR}}
  ALERT --> ALERT_OR
  ALERT_OR --> A1[规则语法错误]
  ALERT_OR --> A2[阈值配置错误]

  STORE_OR{{OR}}
  STORE --> STORE_OR
  STORE_OR --> ST1[存储空间不足]
  STORE_OR --> ST2[数据损坏]

  REMOTE_OR{{OR}}
  REMOTE --> REMOTE_OR
  REMOTE_OR --> R1[远端不可达]
  REMOTE_OR --> R2[鉴权失败]
```

---

## 生产级观测与证据
- **事件**：采集失败、告警未触发、指标缺失。
- **关键指标**：`prometheus_target_interval_length_seconds`、`prometheus_rule_evaluation_failures_total`。
- **关键日志**：Prometheus/Alertmanager 日志、远程存储日志。
- **配置核对**：Scrape 配置、告警规则、存储与远程写入配置。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    { "name": "开始", "action": "start", "step": "start_monitor_fta", "next_step": "event_monitor_abnormal" },
    { "name": "顶事件: 监控/告警异常", "action": "event", "step": "event_monitor_abnormal", "description": "采集失败/告警不触发", "next_step": "gate_root_or" },
    { "name": "根因 OR 门", "action": "gate_or", "step": "gate_root_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["cat_scrape","cat_disc","cat_alert","cat_store","cat_remote"] }
  ]
}
```

---

## 版本适配（1.19–1.30）
- **1.19–1.23**：服务发现与 API 版本兼容需校验，旧版目标发现可能缺失 EndpointSlice。
- **1.24–1.27**：PSP 移除后监控组件权限需调整。
- **1.28–1.30**：稳定 API 为主，审计链路与告警口径需统一。
- **共性**：遵循 `fta_methodology_and_agentic_practices.md` 中的“版本适配基线”。
