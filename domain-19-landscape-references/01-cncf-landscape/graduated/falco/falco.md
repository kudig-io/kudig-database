---
title: Falco
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- helm
- opa
- falco
- elasticsearch
- webhook
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Falco 是什么
- 如何 Falco
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Falco
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- policy-basics
---

title: Falco
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- opa
- falco
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Falco 是什么
- 如何 Falco
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Falco
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Falco

> **成熟度**: Graduated | **加入时间**: 2018-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://falco.org |
| **GitHub** | https://github.com/falcosecurity/falco |
| **文档** | https://falco.org/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | C++ |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
Falco 是云原生运行时安全项目，通过检测异常行为和安全威胁来保护容器、Kubernetes 和云环境。

### 核心定位
Falco 作为运行时安全引擎，通过 eBPF 或内核模块监控系统调用，检测容器和主机上的异常活动、策略违规和安全威胁，是云原生安全的重要组成部分。

### 发展历程
- **2016**: Sysdig 创建 Falco 项目
- **2018-10**: 加入 CNCF 作为沙箱项目
- **2020-01**: 升级为 CNCF 孵化项目
- **2024-01**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **系统调用监控**: 内核级别的系统调用跟踪
- **规则引擎**: 灵活的安全检测规则
- **Kubernetes 集成**: K8s 审计日志分析
- **多数据源**: 支持云服务和应用日志
- **告警输出**: 多种告警输出渠道

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                       Data Sources                          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │  Syscalls   │ │   K8s Audit │ │    Cloud Logs           ││
│  │ (eBPF/kmod) │ │    Logs     │ │  (AWS/GCP/Azure)        ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Falco Engine                           │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    Rules Engine                         ││
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐   ││
│  │  │   Parser    │ │   Filter    │ │    Alerter      │   ││
│  │  └─────────────┘ └─────────────┘ └─────────────────┘   ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                         Outputs                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │   Stdout    │ │    File     │ │  Webhooks/SIEM          ││
│  │   Syslog    │ │   gRPC      │ │  Slack/PagerDuty        ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Falco | 主引擎 | 规则处理和告警生成 |
| Drivers | 数据采集 | eBPF 或内核模块 |
| Falcosidekick | 告警路由 | 多渠道告警转发 |
| Falcoctl | 管理工具 | 规则和驱动管理 |

### 工作原理
1. Driver（eBPF/内核模块）捕获系统调用
2. 事件传递给 Falco 引擎
3. 规则引擎评估事件
4. 匹配规则时生成告警
5. 告警输出到配置的渠道

---

## 使用场景

### 典型应用
- **入侵检测**: 检测容器中的可疑活动
- **合规审计**: 监控策略违规行为
- **异常检测**: 识别异常的系统行为
- **取证分析**: 安全事件调查

### 适用条件
- 需要运行时安全监控
- Kubernetes 环境安全
- 需要安全合规审计
- 需要威胁检测能力

### 不适用场景
- 静态代码分析
- 网络入侵检测（需配合其他工具）

---

## 快速开始

### 安装部署
```bash
# Helm 安装
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm install falco falcosecurity/falco --namespace falco --create-namespace

# 使用 falcoctl 安装驱动
falcoctl driver install
```

### 基础配置
```yaml
# falco.yaml
rules_file:
  - /etc/falco/falco_rules.yaml
  - /etc/falco/rules.d

json_output: true
json_include_output_property: true

stdout_output:
  enabled: true

webserver:
  enabled: true
  listen_port: 8765

grpc:
  enabled: true
  bind_address: "0.0.0.0:5060"
```

### 规则示例
```yaml
# 检测容器中运行的 shell
- rule: Terminal shell in container
  desc: A shell was spawned in a container
  condition: >
    spawned_process and
    container and
    shell_procs and
    proc.tty != 0
  output: >
    Shell spawned in container
    (user=%user.name container=%container.name shell=%proc.name
    parent=%proc.pname cmdline=%proc.cmdline)
  priority: WARNING
  tags: [container, shell, mitre_execution]
```

### 验证测试
```bash
# 查看 Falco 状态
kubectl get pods -n falco

# 查看告警日志
kubectl logs -n falco -l app.kubernetes.io/name=falco

# 触发测试告警（在容器中执行 shell）
kubectl exec -it <pod-name> -- /bin/sh
```

---

## 最佳实践

### 生产环境建议
- 使用 eBPF 驱动（推荐）
- 配置 Falcosidekick 告警路由
- 调优规则减少误报
- 集成 SIEM 系统

### 性能优化
- 优化规则复杂度
- 使用输出缓冲
- 合理配置采样率
- 监控 Falco 资源使用

### 安全加固
- 定期更新规则集
- 保护 Falco 配置
- 限制告警访问权限
- 审计 Falco 自身

---

## 生态集成

### 相关 CNCF 项目
- **Kubernetes**: K8s 审计日志集成
- **Prometheus**: 指标导出
- **OPA**: 策略集成

### 常见集成方案
- Falco + Falcosidekick + Slack/PagerDuty
- Falco + Prometheus + Grafana
- Falco + Elasticsearch/SIEM
- Falco + Security Response Automation

---

## 参考资源

- [官方文档](https://falco.org/docs)
- [GitHub Repo](https://github.com/falcosecurity/falco)
- [CNCF 项目页面](https://www.cncf.io/projects/falco/)
- [Falco 规则库](https://github.com/falcosecurity/rules)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/cloud-native-defense-in-depth|Cloud Native Defense in Depth]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[entities/trivy|Trivy]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/tetragon|Tetragon]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.43|falco v0.43 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.12|falco v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.26|falco v0.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.8|falco v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.36|falco v0.36 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.22|falco v0.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.16|falco v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.32|falco v0.32 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.23|falco v0.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.17|falco v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.33|falco v0.33 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.42|falco v0.42 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.13|falco v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.27|falco v0.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.9|falco v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.37|falco v0.37 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.18|falco v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.28|falco v0.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.38|falco v0.38 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.6|falco v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.29|falco v0.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.39|falco v0.39 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.7|falco v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.19|falco v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.5|falco v0.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.20|falco v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.14|falco v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.30|falco v0.30 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.10|falco v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.41|falco v0.41 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.24|falco v0.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.34|falco v0.34 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.11|falco v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.40|falco v0.40 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.25|falco v0.25 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.35|falco v0.35 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.21|falco v0.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.15|falco v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.31|falco v0.31 Release Notes]]
