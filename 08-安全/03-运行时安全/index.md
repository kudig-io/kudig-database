---
title: Runtime Security
description: 运行时安全知识域 — Falco 威胁检测、容器沙箱(gVisor/Kata)、运行时防御、企业容器安全平台
category: subdomain
tags:
- falco
- runtime-security
- gvisor
- kata-containers
- threat-detection
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 运行时安全 Runtime Security

> 容器运行时的威胁检测、行为分析与实时防御。

## 运行时安全工具对比

| 工具 | 技术 | 能力 | 性能影响 |
|------|------|------|----------|
| Falco | eBPF/内核模块 | 系统调用监控、规则告警 | 低 |
| Sysdig | eBPF | 深度可视化 + 安全 | 中 |
| Aqua Security | Agent | 全生命周期安全 | 中 |
| gVisor | 用户态内核 | 系统调用拦截/沙箱 | 中-高 |
| Kata Containers | 轻量 VM | 硬件级隔离 | 高 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[08-安全/03-运行时安全/01-falco-cloud-native-security.md\|Falco 云原生安全]] | 规则编写/部署/响应 | advanced |
| [[08-安全/03-运行时安全/02-sysdig-enterprise-container-security.md\|Sysdig 容器安全]] | 企业级容器安全平台 | advanced |
| [[08-安全/03-运行时安全/03-aqua-enterprise-container-security.md\|Aqua 容器安全]] | 全生命周期安全防护 | advanced |
| [[08-安全/03-运行时安全/04-runtime-security-defense.md\|运行时防御]] | 防御架构与策略 | intermediate |
| [[08-安全/03-运行时安全/05-runtime-security-detection.md\|运行时检测]] | 异常行为检测与响应 | advanced |
| [[08-安全/03-运行时安全/06-gvisor-container-sandbox.md\|gVisor 沙箱]] | 用户态内核沙箱实践 | advanced |
| [[08-安全/03-运行时安全/07-security-context-fields-reference.md\|Security Context 字段参考]] | 全字段汇总/PSS 映射/LSM | advanced |
| [[08-安全/03-运行时安全/08-falco-runtime-security-guide.md\|Falco 指南]] | 生产环境完整指南 | advanced |

## 运行时安全检查清单

- [ ] 部署 Falco 并启用默认规则集
- [ ] 自定义规则覆盖业务特定场景
- [ ] 告警集成到 SIEM/SOAR 平台
- [ ] 高安全负载使用 gVisor/Kata 沙箱
- [ ] 定期更新规则库（跟踪 CVE）
- [ ] 建立运行时事件响应流程

## Related

- [[08-安全/02-网络安全/index.md|网络安全]]
- [[08-安全/06-合规审计/index.md|合规审计]]
- [[14-容器运行时/index.md|容器运行时]]
