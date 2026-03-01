# GitOps（ArgoCD）异常 FTA 树

## 适用范围与说明
- **目标**：覆盖同步失败、应用状态漂移与回滚失败的关键成因与路径。
- **范围**：Git 仓库访问、清单渲染、集群连接、权限与审计、回滚流程。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: GitOps 同步异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> REPO[仓库访问异常]
  OR0 --> MANI[清单渲染异常]
  OR0 --> CLUSTER[集群连接异常]
  OR0 --> RBAC[权限/准入异常]
  OR0 --> ROLLBACK[回滚异常]

  REPO_OR{{OR}}
  REPO --> REPO_OR
  REPO_OR --> R1[凭证过期]
  REPO_OR --> R2[网络不可达]

  MANI_OR{{OR}}
  MANI --> MANI_OR
  MANI_OR --> M1[模板渲染失败]
  MANI_OR --> M2[API 版本不兼容]

  CLUSTER_OR{{OR}}
  CLUSTER --> CLUSTER_OR
  CLUSTER_OR --> C1[API Server 不可达]
  CLUSTER_OR --> C2[证书失效]

  RBAC_OR{{OR}}
  RBAC --> RBAC_OR
  RBAC_OR --> RB1[RBAC 权限不足]
  RBAC_OR --> RB2[准入 Webhook 拒绝]

  ROLLBACK_OR{{OR}}
  ROLLBACK --> ROLLBACK_OR
  ROLLBACK_OR --> RBK1[回滚策略缺失]
  ROLLBACK_OR --> RBK2[状态不可逆]
```

---

## 生产级观测与证据
- **事件**：Sync 失败、应用状态漂移、回滚失败。
- **关键指标**：同步失败率、漂移次数、回滚成功率。
- **关键日志**：ArgoCD Controller 日志、`apiserver` 审计日志。
- **配置核对**：仓库凭证、应用清单、集群访问配置、RBAC。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    { "name": "开始", "action": "start", "step": "start_gitops_fta", "next_step": "event_gitops_abnormal" },
    { "name": "顶事件: GitOps 同步异常", "action": "event", "step": "event_gitops_abnormal", "description": "同步失败/漂移/回滚失败", "next_step": "gate_root_or" },
    { "name": "根因 OR 门", "action": "gate_or", "step": "gate_root_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["cat_repo","cat_mani","cat_cluster","cat_rbac","cat_rb"] }
  ]
}
```

---

## 版本适配（1.19–1.30）
- **1.19–1.23**：清单中旧版 API 需迁移；GitOps 工具需兼容旧 API。
- **1.24–1.27**：PSP 移除后 RBAC/准入策略需调整。
- **1.28–1.30**：稳定 API 为主，审计与回滚链路需一致。
- **共性**：遵循 `fta_methodology_and_agentic_practices.md` 中的“版本适配基线”。
