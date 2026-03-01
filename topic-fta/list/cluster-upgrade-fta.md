# 集群升级异常 FTA 树

## 适用范围与说明
- **目标**：覆盖集群升级失败、版本不兼容与回滚失败的关键成因与路径。
- **范围**：控制面升级、节点升级、API 版本兼容、运行时/插件、证书与审计。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: 集群升级异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> CP[控制面升级异常]
  OR0 --> NODE[节点升级异常]
  OR0 --> API[API 版本不兼容]
  OR0 --> PLUG[插件/运行时异常]
  OR0 --> ROLLBACK[回滚失败]

  CP_OR{{OR}}
  CP --> CP_OR
  CP_OR --> CP1[组件版本不一致]
  CP_OR --> CP2[etcd 升级失败]

  NODE_OR{{OR}}
  NODE --> NODE_OR
  NODE_OR --> NODE1[kubelet 升级失败]
  NODE_OR --> NODE2[节点不可达]

  API_OR{{OR}}
  API --> API_OR
  API_OR --> API1[API 移除未迁移]
  API_OR --> API2[CRD 版本不兼容]

  PLUG_OR{{OR}}
  PLUG --> PLUG_OR
  PLUG_OR --> PLUG1[CNI/CSI 不兼容]
  PLUG_OR --> PLUG2[容器运行时不兼容]

  ROLLBACK_OR{{OR}}
  ROLLBACK --> ROLLBACK_OR
  ROLLBACK_OR --> RB1[回滚策略缺失]
  ROLLBACK_OR --> RB2[数据/状态不可逆]
```

---

## 生产级观测与证据
- **事件**：升级卡顿、组件重启、API 不可用。
- **关键指标**：控制面健康、节点就绪率、API 失败率。
- **关键日志**：控制面组件日志、升级工具日志。
- **配置核对**：升级计划、API 迁移清单、插件兼容矩阵。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    { "name": "开始", "action": "start", "step": "start_upgrade_fta", "next_step": "event_upgrade_abnormal" },
    { "name": "顶事件: 集群升级异常", "action": "event", "step": "event_upgrade_abnormal", "description": "升级失败/回滚失败", "next_step": "gate_root_or" },
    { "name": "根因 OR 门", "action": "gate_or", "step": "gate_root_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["cat_cp","cat_node","cat_api","cat_plug","cat_rb"] }
  ]
}
```

---

## 版本适配（1.19–1.30）
- **1.19–1.23**：重点关注 Ingress/CronJob 等 API 迁移与 dockershim 变更预案。
- **1.24–1.27**：PSP 移除与运行时切换为主要风险点。
- **1.28–1.30**：稳定 API 为主，插件兼容矩阵需更新到当前版本。
- **共性**：遵循 `fta_methodology_and_agentic_practices.md` 中的“版本适配基线”。
