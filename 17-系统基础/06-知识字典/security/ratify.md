---
title: Ratify 准入验证
description: Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM
  和漏洞扫...
summary: Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM
  和漏洞扫...
category: dictionary
tags:
- k8s
- glossary
- security
- admission
- supply-chain
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ratify 准入验证 是什么
- Ratify 详解
trigger_keywords:
- Ratify 准入验证
- Ratify
- dictionary
prerequisites:
- kubernetes
---



# Ratify 准入验证（Ratify）

## 概述

Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM 和漏洞扫描结果等供应链元数据。

## 核心概念/原理

- **准入验证**：作为 External Data Provider 为 Gatekeeper 提供验证数据
- **多验证器**：支持 Notary 签名、Cosign 签名、SBOM 验证、漏洞扫描验证
- **可扩展**：插件式验证器架构
- **Azure 背景**：微软主导，与 Azure 生态深度集成

## 关键机制或特性

- 与 OPA Gatekeeper 的 External Data 集成
- Notation / Cosign 签名验证
- SBOM 存在性和格式验证
- 漏洞扫描结果验证（Trivy/Grype）
- Certificate Store 管理签名证书
- VerificationResult 标准化输出

## 使用场景与最佳实践

- 生产集群的镜像签名强制验证
- CI/CD 中的供应链安全检查门控
- 合规要求下的 SBOM 验证
- 多来源镜像的统一准入策略
- 与 Kyverno/Gatekeeper 配合的策略引擎

## 架构深度解析

### Ratify 镜像验证架构

```
┌──────────────────────────────────────────────────────────────┐
│  Kubernetes API Server                                       │
│   │  Pod 创建（引用镜像）                                     │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Gatekeeper / Kyverno（策略引擎）                         │  │
│  │ ├─ 检测到含 ratify 注解/约束的资源                       │  │
│  │ └─ 调用 Ratify（external data provider）                 │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 验证请求                       │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Ratify（Deployment + Server 模式）                      │  │
│  │ ├─ Verifiers：cosign/notation/licensechecker 等         │  │
│  │ ├─ Referrers 解析：OCI 1.1 referrers API / fallback     │  │
│  │ └─ 输出 VerificationResult（pass/fail + 详情）           │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 拉取 referrer artifacts       │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ OCI Registry（镜像 + 签名/SBOM/漏洞报告）                │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（ratify-project/ratify）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| 验证器框架 | pkg/verifier/ | 多验证器注册与调度 |
| 引用解析 | pkg/referrerstore/ | OCI referrers API 与 artifact 解析 |
| 策略引擎 | pkg/policyprovider/ | 验证策略（anyOf/allOf）定义 |
| 插件体系 | pkg/plugins/ | cosign/notation/sbom 等插件 |
| API 服务 | httpserver/ | external data provider HTTP 接口 |

### 流程步骤

1. 镜像推送时附加 referrer artifacts（cosign 签名、SBOM、漏洞报告），registry 记录引用关系。
2. 用户创建 Pod 引用镜像，Gatekeeper/Kyverno 策略匹配后向 Ratify 发起验证请求。
3. Ratify 通过 OCI Referrers API 获取镜像的全部引用制品。
4. 按配置的验证器链（签名验证、SBOM 完整性、漏洞阈值）逐项评估。
5. 汇总 VerificationResult 返回策略引擎，决定允许/拒绝创建。

## 生产案例

### 案例 1：镜像签名验证灰度上线误拒存量镜像（2024 年供应链门禁）

| 时间 | 事件 |
|---|---|
| T+0 | 平台启用 Ratify + cosign 强制验证，覆盖全部命名空间 |
| T+30min | 大量存量未签名镜像被拒，发布流水线大面积失败 |
| T+2h | 调整为白名单模式（先覆盖新建命名空间），存量分批补签 |
| T+1w | 存量镜像全部补签完成，验证范围扩展到全部命名空间 |

- **根因**：未评估存量镜像签名覆盖率直接全量强制；无灰度发布机制。
- **修复命令**（灰度切换）：
```bash
# 🟢 查看验证结果（通过/拒绝统计）
kubectl get verificationresults -A
# 🟡 调整策略为白名单命名空间范围，灰度扩大
kubectl edit k8sconstrainedratifypolicy ratify-policy
```

### 案例 2：Referrers 解析失败导致误拒合法镜像

- **现象**：部分合法签名镜像验证失败：`no referrers found`。
- **诊断**：registry 不支持 OCI 1.1 Referrers API 且未配置 fallback；tag 方案引用索引未刷新。
- **修复**：升级 registry（支持 referrers API）或配置 ORAS artifact 兼容层；验证 pipeline 增加 referrers 健康检查。

## 对比评测

| 维度 | Ratify | cosign 单独验证 | Connaisseur |
|---|---|---|---|
| 架构 | 插件化验证框架 | CLI/库 | 自研 webhook |
| 引用类型 | 签名+SBOM+漏洞报告 | 签名 | 签名 |
| 策略引擎 | 内置 + 外部（Gatekeeper/Kyverno） | 需自建 | 内置 |
| 扩展性 | 插件体系丰富 | 有限 | 有限 |
| 生态 | CNCF 项目 | Sigstore 生态 | 小众 |

- **选型建议**：需要多类型制品验证（签名+SBOM）选 Ratify；仅签名验证且希望轻量选 cosign；已在 Kyverno/Gatekeeper 体系内选 Ratify 作为 external data provider。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| no referrers found | registry 不支持 Referrers API | 检查 registry 版本/配置 fallback |
| 签名验证失败 | 公钥/信任根未配置 | `ratify config show`、检查 certConfig |
| 验证超时 | 网络/registry 慢 | 检查 Ratify 日志与网络延迟 |
| 误拒镜像 | 策略过严（allOf） | 调整策略为 anyOf 或补充验证器 |
| 插件加载失败 | 插件版本不兼容 | `kubectl logs ratify` 查看插件错误 |

## 生产部署清单

- [ ] Registry 确认支持 OCI Referrers API（或配置 ORAS fallback）
- [ ] 信任根管理：cosign 公钥/notation 证书纳入统一 PKI 与轮换
- [ ] 验证策略灰度：先审计模式（report-only）评估覆盖率再强制
- [ ] 插件清单与版本锁定，升级走测试环境验证
- [ ] 监控验证 QPS、失败率、referrers 解析成功率并告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 验证器误拒大量生产镜像 | 立即回退策略为 report-only，恢复发布后修复 |
| P1 | 信任根/签名体系轮换 | 新旧信任并存过渡期，灰度验证后切换 |
| P2 | Ratify 版本升级 | 测试环境验证插件兼容性后滚动升级 |

## 面试要点

> 以下 Q&A 覆盖 Ratify 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Ratify 在镜像验证链中的角色是什么？**
   A：Ratify 是"验证编排框架"：它不定义安全策略，而是把多种验证器（cosign 签名、notation 证书、SBOM 完整性、漏洞扫描报告）插件化编排，通过 OCI Referrers API 获取镜像关联制品并逐项验证，最后输出标准化 VerificationResult，供 Gatekeeper/Kyverno 做准入决策，实现"验证逻辑与策略逻辑解耦"。

2. **Q：OCI Referrers API 与传统 tag 方案的区别？**
   A：传统方案用约定 tag（如 `image:sha256-xxx.sig`）关联签名，存在 tag 可变、竞态与查询困难问题；Referrers API 通过 manifest 的 subject 字段记录引用关系，registry 提供标准查询接口，不可变且可枚举全部关联制品（签名/SBOM/漏洞报告），是 Ratify 可靠解析的基础。

3. **Q：Ratify 生产落地的关键成功要素？**
   A：① 基础设施先行：registry 支持 Referrers API、信任根统一管理；② 灰度策略：先 report-only 审计存量覆盖率，补签/整改后再强制；③ 插件治理：版本锁定与兼容性测试；④ 监控闭环：验证失败率、误拒率、referrers 解析成功率，异常时一键回退审计模式。

## 运维要点

- 信任根：cosign/notation 信任根统一管理，轮换走双信任过渡期。
- 插件治理：插件镜像版本锁定，升级先在测试集群验证。
- 容量：Ratify 多副本 + 缓存（验证结果 TTL），高峰期观察 QPS 与延迟。
- 排障入口：VerificationResult CRD → Ratify 日志 → registry referrers 健康。
- 告警：验证失败率、referrers 解析失败、插件错误、策略拒绝率突增。

## 参考链接

- https://ratify.dev/
- https://github.com/ratify-project/ratify

## Related

- [[17-系统基础/06-知识字典/security/notary-project.md|Notary Project]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA Gatekeeper]]
- [[17-系统基础/06-知识字典/security/kyverno.md|Kyverno]]
