---
title: '场景: 安全加固'
description: Kubernetes 安全加固，覆盖 RBAC、网络策略、Pod 安全、证书管理
summary: Kubernetes 安全加固，覆盖 RBAC、网络策略、Pod 安全、证书管理
category: scenario
tags:
- k8s
- scenario
- security
- rbac
- networkpolicy
- rag
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-20'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '场景: 安全加固 是什么'
- '如何 场景: 安全加固'
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- '场景:'
- 安全加固
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 场景: 安全加固

> **场景 ID**: SC-05
> **英文**: Security Hardening
> **最后更新**: 2026-05-20

---

## 场景概述

安全加固是生产环境的基础要求。

---

## 快速决策树

```mermaid
graph TD
    A["安全加固"] --> B{"问题确认"}
    B -->|"已知问题"| C["参考相关文档"]
    B -->|"未知问题"| D{"组件定位"}
    D -->|"控制平面"| E["参考 集群基础"]
    D -->|"工作负载"| F["参考 工作负载"]
    D -->|"网络"| G["参考 网络"]
    D -->|"存储"| H["参考 存储"]
    D -->|"安全"| I["参考 安全"]

    C --> J["执行修复"]
    E --> J
    F --> J
    G --> J
    H --> J
    I --> J

    J --> K{"验证"}
    K -->|"已解决"| L["记录关闭"]
    K -->|"未解决"| M["升级到专家"]

    style A fill:#ef4444,stroke:#b91c1c,color:#fff
    style L fill:#22c55e,stroke:#166534,color:#fff
    style M fill:#f59e0b,stroke:#b45309,color:#fff
```

---

## 相关文档

- [[08-安全/README.md|README]]
- [[08-安全/README.md|README]]
- supply-chain-security/README.md]]


---

## FTA 故障树

- [[19-故障诊断/06-FTA故障树/list/rbac-fta.md|rbac fta]]
- [[19-故障诊断/06-FTA故障树/list/certificate-fta.md|certificate fta]]
- [[19-故障诊断/06-FTA故障树/list/networkpolicy-fta.md|networkpolicy fta]]


---

## 操作技能

暂无专项技能卡片


---

## 关联场景

| 关联场景 | 说明 |
|---|---|

## 生产案例

### 案例1：匿名访问未禁用导致数据泄露

| 时间 | 事件 |
|---|---|
| 安全扫描 | 发现 API Server 允许匿名访问 |
| 评估 | 任何人可读取集群敏感信息 |
| 修复 | 禁用匿名访问 + 启用 RBAC |
| 验证 | 扫描确认无未授权访问 |

**根因**：初始化配置未禁用 anonymous-auth。

**修复**：
```bash
# 🟡 禁用匿名访问
# kube-apiserver 参数: --anonymous-auth=false
# 🟢 检查 RBAC 配置
kubectl get clusterrolebindings -o wide
kubectl auth can-i --list --as=system:anonymous
# 🟡 启用审计日志
# --audit-policy-file=/etc/kubernetes/audit-policy.yaml
```

### 案例2：Secret 明文存储被窃取

- **现象**：发现 Git 仓库中包含明文 Secret
- **诊断**：未启用 etcd 加密，Secret 以 base64 存储
- **修复**：启用 EncryptionConfiguration + 轮转所有 Secret + 清理 Git 历史

## 面试要点

1. **Q：K8s 安全加固的核心措施？**
   A：禁用匿名访问、RBAC 最小权限、Pod Security Standards、网络策略、Secret 加密、审计日志、镜像安全、定期扫描。

2. **Q：零信任架构在 K8s 中的实现？**
   A：mTLS(服务间加密)、身份验证(OIDC)、授权(RBAC/OPA)、网络微分段、持续验证、最小权限。

3. **Q：如何检测集群安全威胁？**
   A：Falco 运行时检测、审计日志分析、异常行为监控、定期漏洞扫描、镜像签名验证、网络流量分析。

## Related

- [[23-实体/15-参考与索引/kudig-metadata-index.md|README]].md|README]]
- [[26-技能/07-安全/certificate/certificate-fta.md|certificate-fta]]
- [[17-系统基础/06-知识字典/security/cloud-native-security.md|cloud-native-security]]


<!-- risk-assessed -->
