---
title: 运行时安全
description: '# 运行时安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
- falco
- rbac
- networkpolicy
- operator
- webhook
- ebpf
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 运行时安全 是什么
- 如何 运行时安全
trigger_keywords:
- 运行时安全
- dictionary
title_en: Runtime Security
---


# 运行时安全

## 概述

**运行时安全（Runtime Security）** 关注的是容器和 Pod 在集群中实际运行时的威胁检测与防护。即使镜像通过了漏洞扫描和签名验证，运行时仍可能遭遇零日漏洞利用、配置漂移、内部威胁或供应链后门的激活。2026 年的最佳实践强调通过 **eBPF 技术**实现内核级、零侵入的运行时安全监控和实时响应。

## 核心概念/原理

### 1. 运行时威胁类型

Kubernetes 运行时面临的主要威胁包括：
- **容器逃逸**：攻击者突破容器边界获取宿主机权限
- **异常进程执行**：容器内启动未预期的 shell、挖矿程序或扫描工具
- **敏感文件访问**：读取 `/etc/shadow`、Kubernetes ServiceAccount Token 或宿主机路径
- **横向移动**：攻击者在一个 Pod 内获取凭证后尝试访问其他服务或节点
- **权限提升**：利用 SUID 程序或内核漏洞提升权限

### 2. eBPF 与运行时安全

**eBPF** 允许在内核中安全地运行沙箱程序，是运行时安全监控的理想技术：
- **系统调用追踪**：监控所有 `execve`、`open`、`connect` 等系统调用
- **网络包分析**：捕获 Pod 间的异常网络连接
- **文件访问审计**：记录敏感文件和配置文件的读写操作
- **零性能开销**：相比传统审计机制，eBPF 对应用性能影响极小

### 3. Falco

**Falco** 是 CNCF 孵化项目，由 Sysdig 创建，是 Kubernetes 运行时安全的行业标杆工具：
- **规则引擎**：使用类 SQL 的语法定义异常行为规则
- **eBPF 驱动**：默认使用 eBPF probe 进行事件采集，也可使用内核模块
- **事件输出**：支持将告警发送到 stdout、文件、Syslog、Webhook、Slack、PagerDuty 等

```yaml
# Falco 规则示例：检测容器内运行 shell
- rule: Terminal shell in container
  desc: Detect shell execution inside a container
  condition: spawned_process and container and shell_procs
  output: "Shell executed in container (user=%user.name container=%container.name)"
  priority: WARNING
```

### 4. KubeArmor

**KubeArmor** 是 CNCF 沙箱项目，专注于**运行时策略执行**（而不仅是检测）：
- 基于 eBPF/LSM（Linux Security Modules）强制限制容器的文件访问、进程执行和网络行为
- 可阻止容器运行未授权的进程（Deny-by-Default）
- 支持自动发现应用正常行为并生成最小权限策略

### 5. Trivy Operator

虽然主要用于漏洞扫描，**Trivy Operator** 也在运行时持续监控：
- 扫描运行中的容器镜像是否出现新的 CVE
- 检测 Kubernetes 配置漂移（如 RBAC 过度授权、Secret 暴露）
- 生成 Cluster-wide 的安全报告

## 关键机制或特性

| 工具 | 核心能力 | 技术基础 | 响应方式 |
|------|----------|----------|----------|
| **Falco** | 检测异常行为 | eBPF / Kernel Module | 告警（Alert） |
| **KubeArmor** | 强制执行最小权限 | eBPF / LSM | 阻止（Block） |
| **Tetragon** | 进程/网络执行追踪 | eBPF | 告警 +  Kill |
| **Trivy Operator** | 持续 CVE 和配置扫描 | 漏洞数据库 | 报告 + 告警 |

### Falco 告警响应闭环

```
Falco 检测到异常
    ↓
发送到 Falco Sidekick / NATS
    ↓
触发 Kubernetes Response Engine
    ↓
自动执行响应：隔离 Pod / 添加 NetworkPolicy / 通知 SOC
```

## 使用场景

1. **容器逃逸检测**：Falco 检测到容器内进程尝试访问 `/proc/1/environ` 或挂载宿主机目录时立即告警
2. **挖矿程序防护**：KubeArmor 阻止容器内执行任何不在白名单中的二进制文件，防止加密货币挖矿
3. **敏感数据访问审计**：监控所有对 Kubernetes Secret 文件路径的读取操作，发现异常访问模式
4. **零日漏洞响应**：即使镜像扫描时无已知 CVE，运行时行为异常仍可被 eBPF 探针捕获
5. **合规审计**：为金融、医疗等行业提供详细的运行时操作日志，满足 SOC2 / HIPAA 审计要求

## 最佳实践/注意事项

- **先检测后阻断**：初次部署时先用 Falco 观察环境，了解正常行为后再用 KubeArmor 实施阻断策略
- **规则调优避免告警疲劳**：默认规则集可能产生大量噪音，需根据实际业务场景精简和定制规则
- **结合 SOAR/SIEM**：将 Falco 告警集成到企业的 SIEM（如 Splunk、Elastic Security）中，实现统一安全运营
- **最小权限容器设计**：结合 Seccomp、AppArmor、User Namespaces 和 KubeArmor 构建多层防御
- **监控 Falco 自身健康**：Falco Agent 是安全基础设施，其自身可用性必须被监控和告警
- **事件上下文丰富化**：告警中应包含 Pod 名称、Namespace、镜像版本、节点名和 Kubernetes 标签，便于快速定位
- **定期红队演练**：通过模拟攻击验证运行时安全规则的覆盖度和响应速度

## 参考链接

- [Falco Documentation](https://falco.org/docs/)
- [KubeArmor Documentation](https://kubearmor.io/)
- [Tetragon Documentation](https://tetragon.io/)
- [Trivy Operator](https://aquasecurity.github.io/trivy-operator/)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)
