# DNS 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 DNS 解析失败、延迟升高与解析不一致的关键成因与路径。
- **范围**：CoreDNS 部署、上游解析、网络策略、缓存与配置、资源压力。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: DNS 解析异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> CORE[CoreDNS 异常]
  OR0 --> UP[上游解析异常]
  OR0 --> NET[网络策略/连通性异常]
  OR0 --> CFG[配置与缓存异常]
  OR0 --> RES[资源与容量异常]

  CORE_OR{{OR}}
  CORE --> CORE_OR
  CORE_OR --> CORE1[Pod 异常/重启]
  CORE_OR --> CORE2[服务发现异常]

  UP_OR{{OR}}
  UP --> UP_OR
  UP_OR --> UP1[上游 DNS 不可达]
  UP_OR --> UP2[上游超时/丢包]

  NET_OR{{OR}}
  NET --> NET_OR
  NET_OR --> NET1[NetworkPolicy 阻断 DNS]
  NET_OR --> NET2[跨节点网络不通]

  CFG_OR{{OR}}
  CFG --> CFG_OR
  CFG_OR --> CFG1[CoreDNS 配置错误]
  CFG_OR --> CFG2[缓存污染/过期]

  RES_OR{{OR}}
  RES --> RES_OR
  RES_OR --> RES1[CPU/内存资源不足]
  RES_OR --> RES2[查询峰值过高]
```

---

## 生产级观测与证据
- **事件**：`SERVFAIL`、解析超时、`NXDOMAIN` 异常升高。
- **关键指标**：`coredns_dns_request_count_total`、`coredns_dns_request_duration_seconds`、`coredns_cache_hits_total`。
- **关键日志**：`coredns` 日志、网络插件日志。
- **配置核对**：CoreDNS `Corefile`、上游 DNS 地址、NetworkPolicy。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    { "name": "开始", "action": "start", "step": "start_dns_fta", "next_step": "event_dns_abnormal" },
    { "name": "顶事件: DNS 解析异常", "action": "event", "step": "event_dns_abnormal", "description": "解析超时/SERVFAIL", "next_step": "gate_root_or" },
    { "name": "根因 OR 门", "action": "gate_or", "step": "gate_root_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["cat_core","cat_up","cat_net","cat_cfg","cat_res"] }
  ]
}
```

---

## 版本适配（1.19–1.30）
- **1.19–1.23**：CoreDNS 版本差异较大，需关注缓存与插件兼容性。
- **1.24–1.27**：运行时切换后 coredns 日志路径与资源限制需校验。
- **1.28–1.30**：稳定 API 为主，DNS 观测信号应与审计链路一致。
- **共性**：遵循 `fta_methodology_and_agentic_practices.md` 中的“版本适配基线”。
