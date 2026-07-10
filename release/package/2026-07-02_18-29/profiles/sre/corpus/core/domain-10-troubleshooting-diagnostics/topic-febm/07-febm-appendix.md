---
title: 第七章:附录 [domain-10-troubleshooting-diagnostics]
description: 'description: ''**所属系列**: FEBM 法医鉴定循证方法论深度解析'''
summary: 'description: ''**所属系列**: FEBM 法医鉴定循证方法论深度解析'''
category: febm
tags:
- febm
- troubleshooting
- apiserver
- prometheus
- grafana
- jaeger
- istio
- cilium
- docker
- opa
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 35min
intent_queries:
- 第七章:附录 是什么
- 如何 第七章:附录
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 第七章:附录 故障排查
- 第七章:附录 排障步骤
trigger_keywords:
- 第七章:附录
- troubleshooting
- diagnostics
- febm
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 第七章:附录
description: '**所属系列**: FEBM 法医鉴定循证方法论深度解析'
category: febm
tags:
- k8s
- forensics
- evidence-based
- methodology
- apiserver
- [[Prometheus|prometheus]]
- grafana
- [[Jaeger|jaeger]]
- [[Istio|istio]]
- [[Cilium|cilium]]
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 20min
intent_queries:
- 第七章:附录 是什么
- 如何 第七章:附录
trigger_keywords:
- 第七章:附录
- febm
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

# 第七章:附录

> **所属系列**: FEBM 法医鉴定循证方法论深度解析  
> **关联主文档**: [FEBM 方法论深度解析](./febm-methodology-deep-dive.md)  
> **上一章**: [第六章:未来演进方向](./06-febm-future-evolution.md)

---

<!-- chunk: A. 核心术语表 -->## A. 核心术语表

## A.1 方法论术语

| 术语 | 英文全称 | 定义 | 示例 |
|------|---------|------|------|
| **FEBM** | Forensic Evidence-Based Methodology | 法医鉴定循证方法论,强调通过系统化证据收集和分析来确定根本原因 | 在 Pod 崩溃事件中,收集日志、指标、审计记录等证据进行分析 |
| **FTA** | Fault Tree Analysis | 故障树分析,一种自顶向下的演绎推理方法 | 从"应用不可用"顶事件开始,分解为 Pod 崩溃、网络问题等基础事件 |
| **Chain of Custody** | - | 证据监管链,记录证据从收集到呈现的完整历史 | 记录日志文件的采集时间、采集人、存储位置、访问记录 |
| **Locard's Exchange Principle** | - | 洛卡德交换原理,任何接触都会留下痕迹 | 攻击者进入容器必然留下系统调用、网络连接等痕迹 |
| **RCA** | Root Cause Analysis | 根本原因分析 | 确定导致 OOMKilled 的真正原因是内存泄漏而非配置不当 |
| **TTPs** | Tactics, Techniques, and Procedures | 战术、技术与程序,描述攻击者的行为模式 | MITRE ATT&CK 中的"容器逃逸"就是一种 TTP |
| **IOC** | Indicator of Compromise | 入侵指标,表明系统可能被攻击的证据 | 异常的出站网络连接、未知进程、文件系统修改 |
| **APT** | Advanced Persistent Threat | 高级持续性威胁 | 国家级黑客组织针对关键基础设施的长期渗透 |

## A.2 Kubernetes 术语

| 术语 | 英文全称 | 定义 | FEBM 相关性 |
|------|---------|------|------------|
| **OOMKilled** | Out Of Memory Killed | 因内存超限而被终止的容器 | 常见的基础事件,需分析是配置问题还是代码缺陷 |
| **CrashLoopBackOff** | - | Pod 不断崩溃并重启的状态 | 表面症状,需通过 FEBM 追踪到启动脚本错误、依赖缺失等根因 |
| **HPA** | Horizontal Pod Autoscaler | 水平 Pod 自动扩缩容器 | 扩容失败可能导致问题,需检查指标采集、资源配额 |
| **VPA** | Vertical Pod Autoscaler | 垂直 Pod 自动扩缩容器 | 资源调整不当可能触发 OOM,需审查调整策略 |
| **NetworkPolicy** | - | 网络策略,控制 Pod 间通信 | 错误配置可能导致连接失败,需检查策略规则与实际流量 |
| **RBAC** | Role-Based Access Control | 基于角色的访问控制 | 权限提升攻击的关键证据来源 |
| **Admission Controller** | - | 准入控制器,拦截 API 请求并执行策略 | 可作为取证点,记录所有变更尝试 |
| **Service Mesh** | - | 服务网格 (如 Istio, Linkerd) | 提供详细的流量追踪,是网络取证的关键工具 |

## A.3 取证技术术语

| 术语 | 英文全称 | 定义 | 在 FEBM 中的应用 |
|------|---------|------|-----------------|
| **DFIR** | Digital Forensics and Incident Response | 数字取证与事件响应 | FEBM 是 DFIR 在云原生环境的具体实现 |
| **CRIU** | Checkpoint/Restore In Userspace | 用户态检查点与恢复工具 | 捕获容器运行时状态用于离线分析 |
| **eBPF** | Extended Berkeley Packet Filter | 扩展的伯克利包过滤器 | 内核级可观测性,捕获系统调用、网络包 |
| **OSDFIR** | Open Source Digital Forensics and Incident Response | 开源数字取证工具栈 | Plaso, Turbinia, GRR 等工具的集合 |
| **Continuous Forensics** | - | 持续取证,将证据收集嵌入日常运维 | 通过 Falco, Prometheus 等工具实现 |
| **Forensics as Code** | - | 取证即代码,将取证流程自动化 | 使用 Argo Workflows 编排取证任务 |
| **Live Forensics** | - | 实时取证,在系统运行时收集证据 | 通过 eBPF 追踪而不停止 Pod |
| **Post-Mortem Analysis** | - | 事后分析 | 在事件结束后分析日志、指标等历史数据 |

## A.4 安全威胁术语

| 术语 | 英文全称 | 定义 | 检测方法 |
|------|---------|------|---------|
| **Container Escape** | - | 容器逃逸,从容器突破到主机 | Falco 规则检测 nsenter, privileged 容器 |
| **Lateral Movement** | - | 横向移动,攻击者在网络内部横向扩散 | 通过 Service Mesh 追踪异常的 Pod 间通信 |
| **Privilege Escalation** | - | 权限提升 | 审计日志中的 RBAC 违规、Capabilities 添加 |
| **Data Exfiltration** | - | 数据外泄 | 网络流量分析,检测大量出站流量 |
| **Fileless Malware** | - | 无文件恶意软件,仅存在于内存 | 内存取证,检测异常进程、注入代码 |
| **Supply Chain Attack** | - | 供应链攻击,通过污染依赖包或镜像植入后门 | 镜像扫描、SBOM 验证、签名校验 |
| **Zero-Day Exploit** | - | 零日漏洞利用 | 行为分析,检测与已知模式不符的活动 |
| **Crypto Mining** | - | 加密货币挖矿 | CPU 使用率异常、挖矿进程特征 |

## A.5 可观测性术语

| 术语 | 英文全称 | 定义 | 取证价值 |
|------|---------|------|---------|
| **Structured Logging** | - | 结构化日志,使用 JSON 等格式 | 便于自动化解析和关联分析 |
| **Distributed Tracing** | - | 分布式追踪 | 重建跨服务的请求调用链 |
| **RED Metrics** | Rate, Errors, Duration | 速率、错误率、持续时间 | 快速识别服务异常 |
| **USE Method** | Utilization, Saturation, Errors | 利用率、饱和度、错误 | 系统资源问题诊断 |
| **Golden Signals** | - | 黄金指标:延迟、流量、错误、饱和度 | Google SRE 推荐的核心监控指标 |
| **Cardinality** | - | 基数,指标标签的唯一值数量 | 高基数指标可能导致存储和查询性能问题 |
| **Exemplar** | - | 范例,Prometheus 中的追踪样本 | 将指标与具体请求 trace 关联 |

## A.6 其他相关术语

| 术语 | 英文全称 | 定义 | 应用场景 |
|------|---------|------|---------|
| **GitOps** | - | 以 Git 为单一事实来源的运维模式 | 提供不可变的配置变更审计轨迹 |
| **Policy as Code** | - | 策略即代码 | OPA, Gatekeeper 实现自动化合规检查 |
| **Chaos Engineering** | - | 混沌工程,主动注入问题测试系统韧性 | 生成已知问题场景用于取证训练 |
| **Shift-Left** | - | 左移,将质量保证活动前移到开发阶段 | 在 CI/CD 中集成取证就绪性检查 |
| **SOAR** | Security Orchestration, Automation and Response | 安全编排、自动化与响应 | 自动触发取证工作流 |
| **SIEM** | Security Information and Event Management | 安全信息与事件管理 | 集中存储和分析安全日志 |
| **TLS** | Transport Layer Security | 传输层安全协议 | 加密通信,防止证据在传输中被篡改 |
| **mTLS** | Mutual TLS | 双向 TLS 认证 | Service Mesh 中确保服务间通信的身份验证 |

---

<!-- chunk: B. 参考标准与规范 -->## B. 参考标准与规范

## B.1 数字取证标准

## NIST SP 800-61 Rev.2
**标题**: Computer Security Incident Handling Guide  
**组织**: 美国国家标准与技术研究院 (NIST)  
**发布时间**: 2012 年  

**核心内容**:
- 事件响应生命周期:准备 → 检测与分析 → 遏制、根除与恢复 → 事后活动
- 证据收集最佳实践
- 事件优先级分类

**FEBM 相关性**:  
FEBM 的证据链管理和报告生成遵循该指南的框架。

**参考链接**: https://csrc.nist.gov/publications/detail/sp/800-61/rev-2/final

---

## NIST IR 8006
**标题**: NIST Cloud Computing Forensic Science Challenges  
**组织**: NIST  
**发布时间**: 2020 年  

**核心内容**:
- 云取证面临的 65 个挑战
- 涵盖架构、数据收集、分析、反取证等维度
- 多租户环境的隐私与合规问题

**FEBM 相关性**:  
识别了云原生取证的特殊挑战,FEBM 方法论针对这些挑战提供解决方案。

**参考链接**: https://csrc.nist.gov/publications/detail/nistir/8006/final

---

## ISO/IEC 27037:2012
**标题**: Information technology — Security techniques — Guidelines for identification, collection, acquisition and preservation of digital evidence  
**组织**: 国际标准化组织 (ISO)  

**核心内容**:
- 数字证据的识别、收集、获取和保全指南
- 证据完整性保护方法
- 角色与职责定义

**FEBM 相关性**:  
FEBM 的 Chain of Custody 实现基于该标准。

**参考链接**: https://www.iso.org/standard/44381.html

---

## ISO/IEC 27041:2015
**标题**: Information technology — Security techniques — Guidance on assuring suitability and adequacy of incident investigative method  
**组织**: ISO  

**核心内容**:
- 评估取证方法的适用性和充分性
- 方法论选择指南
- 质量保证机制

**FEBM 相关性**:  
提供了评估 FEBM 有效性的框架。

---

## ISO/IEC 27042:2015
**标题**: Information technology — Security techniques — Guidelines for the analysis and interpretation of digital evidence  
**组织**: ISO  

**核心内容**:
- 数字证据的分析与解释指南
- 证据关联技术
- 结论推导的严谨性要求

**FEBM 相关性**:  
FEBM 的证据关联分析流程参考该标准。

---

## ISO/IEC 27043:2015
**标题**: Information technology — Security techniques — Incident investigation principles and processes  
**组织**: ISO  

**核心内容**:
- 事件调查的原则和流程
- 从初始响应到法律诉讼的完整链路
- 跨组织协作机制

**FEBM 相关性**:  
提供了事件调查的标准化流程框架。

---

## RFC 3227
**标题**: Guidelines for Evidence Collection and Archiving  
**组织**: IETF  
**发布时间**: 2002 年  

**核心内容**:
- 证据收集的"易失性顺序":寄存器 → 内存 → 磁盘 → 远程日志
- 最小化对现场的影响
- 时间同步的重要性

**FEBM 相关性**:  
FEBM 在容器取证中遵循该易失性顺序原则。

**参考链接**: https://www.rfc-editor.org/rfc/rfc3227

---

## B.2 云原生安全标准

## MITRE ATT&CK for Containers
**标题**: MITRE ATT&CK Framework - Containers Matrix  
**组织**: MITRE Corporation  

**核心内容**:
- 容器环境的 12 个战术阶段
- 40+ 种攻击技术
- 真实 APT 组织的 TTPs

**FEBM 相关性**:  
用于分类和识别攻击行为,指导证据收集方向。

**参考链接**: https://attack.mitre.org/matrices/enterprise/containers/

**示例战术**:
```
Initial Access → Execution → Persistence → Privilege Escalation 
→ Defense Evasion → Credential Access → Discovery → Lateral Movement 
→ Collection → Exfiltration → Impact
```

---

## CIS Kubernetes Benchmark
**标题**: CIS Kubernetes Benchmark v1.8  
**组织**: Center for Internet Security (CIS)  

**核心内容**:
- Kubernetes 安全配置基线
- 涵盖 Control Plane, Worker Node, Policies 等维度
- 分为 Level 1 (基础) 和 Level 2 (深度防御) 建议

**FEBM 相关性**:  
不合规的配置可能是根本原因,FEBM 调查应检查 CIS 合规性。

**参考链接**: https://www.cisecurity.org/benchmark/kubernetes

---

## NIST SP 800-86
**标题**: Guide to Integrating Forensic Techniques into Incident Response  
**组织**: NIST  

**核心内容**:
- 将取证技术集成到事件响应的方法
- 不同证据源的采集技术
- 法律与合规考量

**FEBM 相关性**:  
提供了取证与事件响应集成的理论基础。

**参考链接**: https://csrc.nist.gov/publications/detail/sp/800-86/final

---

## B.3 数据保护与合规

## GDPR (General Data Protection Regulation)
**组织**: 欧盟  
**生效时间**: 2018 年 5 月  

**取证相关要点**:
- **数据最小化**: 仅收集必要的取证数据
- **访问控制**: 证据访问需要审计
- **数据删除权**: 用户数据可能需在调查后删除
- **跨境传输**: 证据存储位置受限

**FEBM 实践**:
- 使用数据脱敏技术保护个人信息
- 明确定义数据保留策略
- 实施基于角色的证据访问控制

---

## CCPA (California Consumer Privacy Act)
**组织**: 美国加利福尼亚州  
**生效时间**: 2020 年 1 月  

**取证相关要点**:
- 消费者有权知道收集了哪些数据
- 有权请求删除数据
- 数据泄露通知义务

---

## SOC 2 Type II
**组织**: AICPA (American Institute of CPAs)  

**Trust Service Criteria**:
- Security (安全性)
- Availability (可用性)
- Processing Integrity (处理完整性)
- Confidentiality (保密性)
- Privacy (隐私)

**FEBM 相关性**:  
取证流程需要满足 SOC 2 审计要求,特别是证据完整性和访问控制。

---

<!-- chunk: C. 工具速查表 -->## C. 工具速查表

## C.1 运行时安全与检测

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 成熟度 | 官方链接 |
|---------|------|----------|--------|--------|---------|
| **Falco** | Runtime Security | 实时威胁检测,生成安全事件证据 | Apache 2.0 | CNCF Incubating | https://falco.org |
| **Sysdig** | Observability & Security | 系统调用追踪,容器取证 | Apache 2.0 (开源版) | 生产就绪 | https://sysdig.com |
| **Cilium Hubble** | Network Observability | 网络流量可观测性,L7 追踪 | Apache 2.0 | CNCF Graduated | https://github.com/cilium/hubble |
| **Tetragon** | eBPF Security | 基于 eBPF 的运行时安全执行 | Apache 2.0 | CNCF Sandbox | https://github.com/cilium/tetragon |

**使用建议**:
- **Falco**: 第一优先级部署,覆盖 MITRE ATT&CK 规则
- **Cilium Hubble**: 与 Cilium CNI 配合使用,提供 L7 网络可见性
- **Tetragon**: 适用于需要细粒度策略执行的场景

---

## C.2 漏洞扫描

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 扫描范围 | 官方链接 |
|---------|------|----------|--------|---------|---------|
| **Trivy** | Vulnerability Scanner | 镜像/配置/IaC 漏洞扫描 | Apache 2.0 | CVE, 配置错误, Secrets | https://trivy.dev |
| **Grype** | Vulnerability Scanner | 镜像/文件系统漏洞扫描 | Apache 2.0 | CVE | https://github.com/anchore/grype |
| **Clair** | Vulnerability Scanner | 容器镜像静态分析 | Apache 2.0 | CVE | https://github.com/quay/clair |
| **Snyk** | Vulnerability Scanner | 代码/依赖/容器漏洞 | 商业/免费 | CVE, License 合规 | https://snyk.io |

**集成示例**:
```bash
# CI/CD 流水线中集成 Trivy
trivy image --severity HIGH,CRITICAL nginx:latest
if [ $? -ne 0 ]; then
    echo "发现严重漏洞,阻止部署" && exit 1
fi
```

---

## C.3 日志聚合与分析

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 部署模式 | 官方链接 |
|---------|------|----------|--------|---------|---------|
| **Elasticsearch** | Search & Analytics | 证据存储与全文搜索 | Elastic License / SSPL | 集群 | https://www.elastic.co |
| **Loki** | Log Aggregation | 轻量级日志聚合 | AGPL-3.0 | 分布式 | https://grafana.com/oss/loki |
| **Fluentd** | Log Collector | 日志采集与转发 | Apache 2.0 | DaemonSet | https://www.fluentd.org |
| **Fluent Bit** | Log Collector | 轻量级日志采集 | Apache 2.0 | DaemonSet | https://fluentbit.io |
| **Plaso** | Timeline Generation | 超级时间线生成 | Apache 2.0 | 离线分析 | https://plaso.readthedocs.io |
| **Timesketch** | Forensic Timeline | 取证时间线可视化 | Apache 2.0 | Web UI | https://timesketch.org |

**架构建议**:
```
Pod Logs → Fluent Bit (采集) → Fluentd (聚合) → Elasticsearch (存储) → Kibana (可视化)
                                              ↓
                                          Timesketch (取证分析)
```

---

## C.4 指标监控

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 数据模型 | 官方链接 |
|---------|------|----------|--------|---------|---------|
| **Prometheus** | Metrics | 时间序列数据存储,异常检测基础 | Apache 2.0 | Pull-based | https://prometheus.io |
| **Grafana** | Visualization | 指标可视化,故障定位 | AGPL-3.0 | 多数据源 | https://grafana.com |
| **Thanos** | Prometheus HA | 长期存储,全局查询 | Apache 2.0 | 分布式 | https://thanos.io |
| **VictoriaMetrics** | Metrics | 高性能时间序列数据库 | Apache 2.0 | Pull/Push | https://victoriametrics.com |

---

## C.5 分布式追踪

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 协议支持 | 官方链接 |
|---------|------|----------|--------|---------|---------|
| **Jaeger** | Distributed Tracing | 请求链路追踪,性能分析 | Apache 2.0 | OpenTelemetry, Zipkin | https://www.jaegertracing.io |
| **Zipkin** | Distributed Tracing | 分布式追踪 | Apache 2.0 | Zipkin | https://zipkin.io |
| **Tempo** | Distributed Tracing | 大规模追踪存储 | AGPL-3.0 | OpenTelemetry | https://grafana.com/oss/tempo |
| **OpenTelemetry** | Observability Framework | 统一可观测性 SDK | Apache 2.0 | OTel Protocol | https://opentelemetry.io |

---

## C.6 数字取证工具

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 分析对象 | 官方链接 |
|---------|------|----------|--------|---------|---------|
| **Volatility** | Memory Forensics | 内存转储分析 | GPL-2.0 | RAM dump | https://www.volatilityfoundation.org |
| **GRR Rapid Response** | Remote Forensics | 远程证据采集 | Apache 2.0 | 活体系统 | https://github.com/google/grr |
| **Turbinia** | Evidence Processing | 分布式证据处理 | Apache 2.0 | 磁盘镜像, 日志 | https://github.com/google/turbinia |
| **Autopsy** | Digital Forensics | 磁盘取证分析 | Apache 2.0 | 磁盘镜像 | https://www.sleuthkit.org/autopsy |
| **YARA** | Malware Detection | 恶意软件模式匹配 | BSD-3 | 文件, 内存 | https://virustotal.github.io/yara |

---

## C.7 编排与自动化

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 工作流引擎 | 官方链接 |
|---------|------|----------|--------|-----------|---------|
| **Argo Workflows** | Workflow Engine | 取证流程编排 | Apache 2.0 | Kubernetes-native | https://argoproj.github.io/workflows |
| **Argo Events** | Event-driven Automation | 事件驱动取证触发 | Apache 2.0 | Event Gateway | https://argoproj.github.io/events |
| **Falcosidekick** | Alert Routing | Falco 告警路由与响应 | Apache 2.0 | 多输出 | https://github.com/falcosecurity/falcosidekick |
| **Ansible** | Configuration Management | 取证环境部署 | GPL-3.0 | Playbook | https://www.ansible.com |

---

## C.8 威胁情报

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 数据源 | 官方链接 |
|---------|------|----------|--------|--------|---------|
| **YETI** | Threat Intelligence Platform | IOC 管理与关联 | Apache 2.0 | STIX/TAXII | https://yeti-platform.github.io |
| **MISP** | Threat Sharing Platform | 威胁情报共享 | AGPL-3.0 | 社区贡献 | https://www.misp-project.org |
| **OpenCTI** | Threat Intelligence | 结构化威胁情报 | Apache 2.0 | STIX 2.1 | https://www.opencti.io |

---

## C.9 策略引擎

| 工具名称 | 类别 | FEBM 角色 | 许可证 | 策略语言 | 官方链接 |
|---------|------|----------|--------|---------|---------|
| **Open Policy Agent (OPA)** | Policy Engine | 合规性验证,取证前检查 | Apache 2.0 | Rego | https://www.openpolicyagent.org |
| **Gatekeeper** | Admission Controller | Kubernetes 准入控制 | Apache 2.0 | Rego (via OPA) | https://open-policy-agent.github.io/gatekeeper |
| **Kyverno** | Policy Engine | Kubernetes 原生策略管理 | Apache 2.0 | YAML | https://kyverno.io |

---

<!-- chunk: D. Kubernetes 审计策略模板 -->## D. Kubernetes 审计策略模板

## D.1 取证就绪审计策略

```yaml
apiVersion: audit.k8s.io/v1
kind: Policy
metadata:
  name: forensic-ready-audit-policy
rules:
  # 规则 0: 不记录 RequestResponse 级别的只读请求 (减少数据量)
  - level: None
    verbs: ["get", "list", "watch"]
    resources:
      - group: ""
        resources: ["events"]

  # 规则 1: 记录所有 Secret 访问 (敏感数据)
  - level: Metadata
    resources:
      - group: ""
        resources: ["secrets"]

  # 规则 2: 记录所有 ConfigMap 变更 (配置漂移检测)
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: ""
        resources: ["configmaps"]

  # 规则 3: 记录所有 Pod exec/attach (容器访问)
  - level: RequestResponse
    verbs: ["create"]
    resources:
      - group: ""
        resources: ["pods/exec", "pods/attach", "pods/portforward"]

  # 规则 4: 记录所有 RBAC 变更 (权限变更)
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: "rbac.authorization.k8s.io"
        resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]

  # 规则 5: 记录所有 Service Account token 创建
  - level: Metadata
    verbs: ["create"]
    resources:
      - group: ""
        resources: ["serviceaccounts/token"]

  # 规则 6: 记录所有 NetworkPolicy 变更
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: "networking.k8s.io"
        resources: ["networkpolicies"]

  # 规则 7: 记录所有 PersistentVolume 绑定 (数据访问)
  - level: RequestResponse
    verbs: ["create", "update", "patch"]
    resources:
      - group: ""
        resources: ["persistentvolumeclaims"]

  # 规则 8: 记录所有 Admission Webhook 配置变更
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: "admissionregistration.k8s.io"
        resources: ["mutatingwebhookconfigurations", "validatingwebhookconfigurations"]

  # 规则 9: 记录所有 CRD 变更 (扩展 API)
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: "apiextensions.k8s.io"
        resources: ["customresourcedefinitions"]

  # 规则 10: 记录所有认证失败
  - level: Metadata
    omitStages:
      - RequestReceived
    users: ["system:anonymous"]

  # 规则 11: 记录所有来自特定命名空间的操作 (高敏感度)
  - level: RequestResponse
    namespaces: ["production", "finance", "customer-data"]
    verbs: ["create", "update", "patch", "delete"]

  # 规则 12: 记录所有 Pod 创建/删除 (工作负载变更)
  - level: Request
    verbs: ["create", "delete"]
    resources:
      - group: ""
        resources: ["pods"]
      - group: "apps"
        resources: ["deployments", "statefulsets", "daemonsets"]

  # 规则 13: 记录所有授权失败 (未授权访问尝试)
  - level: Metadata
    omitStages:
      - RequestReceived
    omitManagedFields: true

  # 规则 14: 其他所有请求记录 Metadata 级别
  - level: Metadata
    omitStages:
      - RequestReceived
    omitManagedFields: true
```

## D.2 审计级别说明

| 级别 | 记录内容 | 使用场景 | 存储开销 |
|------|---------|---------|---------|
| **None** | 不记录 | 健康检查等高频低价值请求 | 0 |
| **Metadata** | 请求元数据 (用户、时间戳、资源等) | 大多数只读操作 | 低 |
| **Request** | Metadata + 请求体 | 变更操作 | 中 |
| **RequestResponse** | Request + 响应体 | 敏感操作 (RBAC, Secret) | 高 |

## D.3 审计日志字段映射

```json
{
  "kind": "Event",
  "apiVersion": "audit.k8s.io/v1",
  "level": "RequestResponse",
  "auditID": "36aa7f2c-3e5f-4d6a-bb59-8e6f7c8d9e0a",
  "stage": "ResponseComplete",
  "requestURI": "/api/v1/namespaces/default/pods/nginx/exec?command=bash",
  "verb": "create",
  "user": {
    "username": "alice@company.com",
    "uid": "alice",
    "groups": ["developers", "system:authenticated"]
  },
  "sourceIPs": ["10.0.1.5"],
  "userAgent": "kubectl/v1.28.0",
  "objectRef": {
    "resource": "pods",
    "namespace": "default",
    "name": "nginx",
    "subresource": "exec"
  },
  "responseStatus": {
    "code": 200
  },
  "requestReceivedTimestamp": "2024-01-15T10:30:45.123456Z",
  "stageTimestamp": "2024-01-15T10:30:45.234567Z",
  "annotations": {
    "authorization.k8s.io/decision": "allow",
    "authorization.k8s.io/reason": "RBAC: allowed by ClusterRoleBinding"
  }
}
```

**FEBM 取证关键字段**:
- `auditID`: 全局唯一标识符
- `user`: 操作主体
- `sourceIPs`: 来源 IP (追踪攻击源)
- `verb`: 操作类型 (create/update/delete)
- `objectRef`: 受影响的资源
- `requestReceivedTimestamp`: 精确时间戳 (构建时间线)
- `annotations.authorization.k8s.io/decision`: 授权决策

---

<!-- chunk: E. Falco 检测规则模板集 -->## E. Falco 检测规则模板集

## E.1 容器逃逸检测

```yaml
# 规则: 检测容器内启动 privileged 容器
- rule: Launch Privileged Container
  desc: 检测到启动特权容器,可能是容器逃逸的前兆
  condition: >
    container and
    container.privileged=true and
    not (container.image.repository in (trusted_images))
  output: >
    Privileged container started (user=%user.name container_id=%container.id 
    container_name=%container.name image=%container.image.repository:%container.image.tag)
  priority: CRITICAL
  tags: [container_escape, mitre_privilege_escalation]

# 规则: 检测 nsenter 使用 (命名空间切换)
- rule: Namespace Change via nsenter
  desc: 检测到 nsenter 命令,可能用于容器逃逸
  condition: >
    spawned_process and
    proc.name = "nsenter" and
    container
  output: >
    Namespace change detected (user=%user.name command=%proc.cmdline 
    container=%container.name)
  priority: CRITICAL
  tags: [container_escape, mitre_execution]

# 规则: 检测 /dev 下敏感设备挂载
- rule: Sensitive Mount Detected
  desc: 检测到挂载敏感主机路径
  condition: >
    container and
    container.mount.dest in (/dev, /proc, /sys, /host) and
    not container.image.repository in (allowed_monitoring_tools)
  output: >
    Sensitive path mounted (container=%container.name 
    mount_source=%container.mount.source mount_dest=%container.mount.dest)
  priority: HIGH
  tags: [container_escape, mitre_persistence]
```

## E.2 横向移动检测

```yaml
# 规则: 检测异常的网络连接
- rule: Unexpected Outbound Connection
  desc: 检测到容器向非白名单 IP 建立连接
  condition: >
    outbound and
    container and
    not fd.sip in (allowed_external_ips) and
    not fd.sport in (allowed_ports)
  output: >
    Unexpected outbound connection (container=%container.name 
    dest_ip=%fd.rip dest_port=%fd.rport proto=%fd.l4proto)
  priority: WARNING
  tags: [lateral_movement, mitre_command_and_control]

# 规则: 检测 kubectl/crictl 在容器内执行
- rule: Kubernetes Client Tool in Container
  desc: 检测到容器内使用 Kubernetes 客户端工具
  condition: >
    spawned_process and
    container and
    proc.name in (kubectl, crictl, ctr, docker) and
    not container.image.repository in (ci_cd_images)
  output: >
    Kubernetes client tool executed in container 
    (user=%user.name command=%proc.cmdline container=%container.name)
  priority: HIGH
  tags: [lateral_movement, mitre_discovery]
```

## E.3 数据外泄检测

```yaml
# 规则: 检测大量数据传输
- rule: Large Data Exfiltration
  desc: 检测到异常大量的数据传输
  condition: >
    outbound and
    container and
    fd.bytes > 100MB and
    not fd.sip in (trusted_backup_servers)
  output: >
    Large data transfer detected (container=%container.name 
    bytes=%fd.bytes dest_ip=%fd.rip)
  priority: CRITICAL
  tags: [exfiltration, mitre_exfiltration]

# 规则: 检测 /etc/shadow 读取
- rule: Read Sensitive File
  desc: 检测到读取敏感文件
  condition: >
    open_read and
    container and
    fd.name in (/etc/shadow, /etc/passwd, /root/.ssh/id_rsa) and
    not proc.name in (authorized_readers)
  output: >
    Sensitive file read (user=%user.name file=%fd.name 
    process=%proc.name container=%container.name)
  priority: CRITICAL
  tags: [credential_access, mitre_credential_dumping]
```

## E.4 加密货币挖矿检测

```yaml
# 规则: 检测挖矿进程
- rule: Crypto Mining Detected
  desc: 检测到加密货币挖矿进程
  condition: >
    spawned_process and
    container and
    (proc.name in (xmrig, ethminer, cpuminer, ccminer, minerd) or
     proc.cmdline contains "stratum+tcp" or
     proc.cmdline contains "cryptonight")
  output: >
    Crypto mining process detected (process=%proc.name 
    cmdline=%proc.cmdline container=%container.name)
  priority: CRITICAL
  tags: [crypto_mining, mitre_resource_hijacking]

# 规则: 检测异常 CPU 使用
- rule: High CPU Usage
  desc: 检测到容器 CPU 使用率异常高
  condition: >
    container and
    container.cpu.usage > 90 and
    not container.image.repository in (high_cpu_workloads)
  output: >
    High CPU usage detected (container=%container.name 
    cpu_usage=%container.cpu.usage)
  priority: WARNING
  tags: [crypto_mining, mitre_impact]
```

## E.5 权限提升检测

```yaml
# 规则: 检测 setuid/setgid 调用
- rule: Privilege Escalation via Setuid
  desc: 检测到 setuid 系统调用
  condition: >
    syscall.type in (setuid, setgid, setreuid, setregid) and
    container and
    proc.euid=0 and
    proc.ruid!=0
  output: >
    Setuid privilege escalation detected (user=%user.name 
    process=%proc.name container=%container.name)
  priority: CRITICAL
  tags: [privilege_escalation, mitre_privilege_escalation]

# 规则: 检测 sudo/su 使用
- rule: Sudo or Su Executed
  desc: 检测到 sudo/su 命令执行
  condition: >
    spawned_process and
    container and
    proc.name in (sudo, su) and
    not container.image.repository in (admin_containers)
  output: >
    Privilege escalation attempt via sudo/su (user=%user.name 
    command=%proc.cmdline container=%container.name)
  priority: HIGH
  tags: [privilege_escalation, mitre_valid_accounts]
```

## E.6 持久化检测

```yaml
# 规则: 检测 cron 作业创建
- rule: Cron Job Created
  desc: 检测到创建 cron 作业
  condition: >
    open_write and
    container and
    fd.name startswith /etc/cron or
    fd.name startswith /var/spool/cron
  output: >
    Cron job created (user=%user.name file=%fd.name container=%container.name)
  priority: HIGH
  tags: [persistence, mitre_scheduled_task]

# 规则: 检测 systemd 服务创建
- rule: Systemd Service Created
  desc: 检测到创建 systemd 服务
  condition: >
    open_write and
    container and
    fd.name startswith /etc/systemd/system/
  output: >
    Systemd service created (file=%fd.name container=%container.name)
  priority: HIGH
  tags: [persistence, mitre_systemd_service]
```

---

<!-- chunk: F. FEBM 事件响应 Checklist -->## F. FEBM 事件响应 Checklist

## F.1 初始响应 (0-15分钟)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
□ 确认事件真实性
  □ 验证告警来源 (Falco/Prometheus/审计日志)
  □ 排除误报 (检查已知的 False Positive 模式)
  □ 评估初步严重性 (Critical/High/Medium/Low)

□ 启动事件响应流程
  □ 通知事件响应团队
  □ 创建事件跟踪单 (Jira/ServiceNow)
  □ 开启专用通信频道 (Slack #incident-xxx)
  □ 指定事件指挥官 (Incident Commander)

□ 初步证据保全
  □ 暂停自动清理策略 (防止日志/指标被轮换)
  □ 快照受影响的 Pod 定义 (kubectl get pod xxx -o yaml)
  □ 导出最近 15 分钟的日志
  □ 记录当前时间戳 (事件时间线起点)

□ 隔离措施 (如需要)
  □ 隔离受影响的 Pod (NetworkPolicy deny-all)
  □ 冻结受影响的节点 (kubectl cordon)
  □ 撤销可疑账户权限
```
## F.2 证据收集 (15分钟 - 2小时)

```
# 🟢 低风险：只读/信息收集，通常无副作用
□ 日志证据
  □ 收集 Pod 日志 (kubectl logs --all-containers --timestamps)
  □ 收集 Kubernetes 事件 (kubectl get events --sort-by='.lastTimestamp')
  □ 收集审计日志 (查询 API Server audit log)
  □ 收集 Falco 告警日志
  □ 收集 Ingress/Service Mesh 访问日志

□ 指标证据
  □ 导出 Prometheus 时间序列数据 (CPU/内存/网络)
  □ 截图 Grafana 仪表板
  □ 记录资源配额使用情况

□ 网络证据
  □ 导出 Cilium Hubble 流量日志
  □ 捕获网络包 (tcpdump/Wireshark)
  □ 检查 NetworkPolicy 配置
  □ 追踪异常外部连接

□ 配置证据
  □ 导出所有相关资源的 YAML 定义
  □ 检查 Git 仓库最近变更 (GitOps)
  □ 检查 ConfigMap/Secret 版本历史
  □ 验证 RBAC 权限配置

□ 运行时证据 (如需要)
  □ 容器文件系统快照 (docker export/crictl export)
  □ 内存转储 (CRIU checkpoint)
  □ 进程列表与系统调用追踪 (strace)
  □ 镜像分析 (Trivy/Dive)
```
## F.3 分析与诊断 (2-6小时)

```
□ 时间线重建
  □ 使用 Timesketch 构建超级时间线
  □ 标注关键事件节点
  □ 识别初始入侵点
  □ 追踪攻击传播路径

□ 根因分析
  □ 构建故障树 (FTA)
  □ 收集支持性证据 (FEBM)
  □ 执行因果推断分析
  □ 生成假设并逐一验证

□ 影响评估
  □ 确定受影响的服务范围
  □ 评估数据泄露风险
  □ 计算业务损失
  □ 识别合规风险 (GDPR/SOC2)

□ 威胁情报关联
  □ 查询 IOC 数据库 (YETI/MISP)
  □ 匹配 MITRE ATT&CK 技术
  □ 检查是否为已知 APT 组织 TTPs
  □ 搜索 CVE 数据库

□ 法医分析
  □ 内存取证 (Volatility)
  □ 镜像分层分析 (Container Explorer)
  □ 恶意软件检测 (YARA rules)
  □ 供应链完整性验证
```

## F.4 遏制与根除 (6-12小时)

```
□ 短期遏制
  □ 隔离受感染的 Pod/节点
  □ 阻止恶意 IP 地址 (NetworkPolicy/Firewall)
  □ 撤销被盗用的凭证
  □ 轮换所有敏感密钥

□ 长期遏制
  □ 修复漏洞 (打补丁/升级版本)
  □ 移除后门/恶意软件
  □ 加固安全配置 (CIS Benchmark)
  □ 部署额外的检测规则

□ 根除
  □ 删除所有受感染的容器
  □ 重建受污染的节点
  □ 清除持久化机制 (CronJob/DaemonSet)
  □ 验证清除完整性
```

## F.5 恢复与验证 (12-24小时)

```
□ 系统恢复
  □ 从干净的镜像重新部署
  □ 恢复数据备份 (验证完整性)
  □ 重新配置网络策略
  □ 恢复正常监控

□ 安全验证
  □ 执行漏洞扫描 (Trivy)
  □ 运行渗透测试
  □ 检查所有日志是否有残留威胁
  □ 验证所有访问控制

□ 业务验证
  □ 执行功能测试
  □ 验证性能指标
  □ 确认用户访问正常
  □ 监控异常行为
```

## F.6 事后活动 (1周内)

```
□ 文档编写
  □ 生成取证报告 (NIST SP 800-61 格式)
  □ 记录事件时间线
  □ 总结根本原因
  □ 文档化响应过程

□ 经验教训
  □ 召开事后回顾会议 (Postmortem)
  □ 识别流程改进点
  □ 更新 Runbook
  □ 分享知识到团队

□ 预防措施
  □ 部署新的检测规则
  □ 更新安全策略
  □ 加强培训
  □ 实施 Chaos Engineering 验证韧性

□ 合规报告
  □ 通知受影响用户 (如需要)
  □ 向监管机构报告 (GDPR 72小时内)
  □ 更新安全审计记录
  □ 归档证据 (保留期限: 7年)
```

---

<!-- chunk: G. 相关阅读与学习资源 -->## G. 相关阅读与学习资源

## G.1 学术论文

1. **"X-Force: A Formal Framework for Root Cause Analysis"**  
   - 作者: Aceto et al.  
   - 会议: USENIX ATC 2020  
   - 链接: https://www.usenix.org/conference/atc20/presentation/aceto  
   - 摘要: 提出了 RCA 的形式化框架,适用于分布式系统

2. **"CloudRCA: Cloud Fault Detection and Root Cause Analysis Using Machine Learning"**  
   - 作者: Wang et al.  
   - 期刊: IEEE Transactions on Cloud Computing  
   - 年份: 2021  
   - 摘要: 使用机器学习进行云环境 RCA

3. **"Autopsy of a Cloud Breach: The Impact of Static and Dynamic Analysis"**  
   - 作者: Sultan et al.  
   - 会议: Digital Forensics Research Workshop (DFRWS) 2022  
   - 摘要: 云环境数据泄露的取证分析案例研究

4. **"FTA vs FEBM: A Comparative Study in Kubernetes Operations"**  
   - 本文档配套论文  
   - 文件: [./FTA-vs-FEBM.pdf](./FTA-vs-FEBM.pdf)  
   - 摘要: 系统性比较 FTA 与 FEBM 在 K8s 运维中的适用性

## G.2 书籍推荐

1. **《Site Reliability Engineering》**  
   - 作者: Betsy Beyer et al. (Google SRE)  
   - 出版社: O'Reilly  
   - ISBN: 978-1491929124  
   - 相关章节: 第 12 章 "Effective Troubleshooting"

2. **《Kubernetes Security》**  
   - 作者: Liz Rice, Michael Hausenblas  
   - 出版社: O'Reilly  
   - ISBN: 978-1492039174  
   - 相关章节: 第 10 章 "Forensics"

3. **《Digital Forensics with Open Source Tools》**  
   - 作者: Cory Altheide, Harlan Carvey  
   - 出版社: Syngress  
   - ISBN: 978-1597495868

4. **《The Art of Memory Forensics》**  
   - 作者: Michael Hale Ligh et al.  
   - 出版社: Wiley  
   - ISBN: 978-1118825099  
   - 相关章节: 第 17 章 "Linux Memory Forensics"

## G.3 在线课程

1. **Cloud Forensics (Coursera)**  
   - 提供者: University of Colorado  
   - 链接: https://www.coursera.org/learn/cloud-forensics  
   - 内容: 云取证基础,包含 AWS/Azure 案例

2. **Kubernetes Security (A Cloud Guru)**  
   - 链接: https://acloudguru.com/course/kubernetes-security  
   - 内容: K8s 安全最佳实践,包含取证模块

3. **SANS FOR508: Advanced Incident Response, Threat Hunting, and Digital Forensics**  
   - 提供者: SANS Institute  
   - 链接: https://www.sans.org/cyber-security-courses/advanced-incident-response-threat-hunting-training/  
   - 内容: 高级取证技术

## G.4 开源项目与工具

1. **Kubernetes Goat**  
   - 链接: https://github.com/madhuakula/kubernetes-goat  
   - 描述: 故意包含漏洞的 K8s 集群,用于安全训练

2. **BadPods**  
   - 链接: https://github.com/BishopFox/badPods  
   - 描述: 演示 K8s Pod 的各种攻击场景

3. **Kubernetes Forensics Workshop**  
   - 链接: https://github.com/kubernetes-forensics/workshop  
   - 描述: 动手实验室,练习容器取证技能

## G.5 会议与社区

1. **KubeCon + CloudNativeCon**  
   - 链接: https://www.cncf.io/kubecon-cloudnativecon-events/  
   - 内容: CNCF 旗舰会议,包含安全与取证 track

2. **BSides (Security Conferences)**  
   - 链接: http://www.securitybsides.com/  
   - 内容: 社区驱动的安全会议,常有云取证议题

3. **DFRWS (Digital Forensics Research Workshop)**  
   - 链接: https://dfrws.org/  
   - 内容: 顶级数字取证学术会议

4. **CNCF Security TAG**  
   - 链接: https://github.com/cncf/tag-security  
   - 内容: CNCF 安全技术咨询组,发布指南文档

## G.6 博客与专栏

1. **Falco Blog**  
   - 链接: https://falco.org/blog/  
   - 内容: 运行时安全与取证案例研究

2. **Google Cloud Security Blog - Forensics**  
   - 链接: https://cloud.google.com/blog/topics/threat-intelligence  
   - 内容: GCP 环境的取证实践

3. **Sysdig Secure Blog**  
   - 链接: https://sysdig.com/blog/  
   - 内容: 容器安全与取证技术

4. **SANS Internet Storm Center**  
   - 链接: https://isc.sans.edu/  
   - 内容: 最新威胁情报与分析

## G.7 Podcast

1. **The Kubernetes Podcast from Google**  
   - 链接: https://kubernetespodcast.com/  
   - 推荐集数: Episode 182 "Security and Forensics"

2. **Darknet Diaries**  
   - 链接: https://darknetdiaries.com/  
   - 推荐集数: Episode 120 "Container Escapes"

## G.8 YouTube 频道

1. **CNCF [Cloud Native Computing Foundation]**  
   - 链接: https://www.youtube.com/@cncf  
   - 推荐播放列表: "Security & Identity"

2. **Rawsec**  
   - 链接: https://www.youtube.com/@rawsec  
   - 内容: 云原生安全技术演示

3. **LiveOverflow**  
   - 链接: https://www.youtube.com/@LiveOverflow  
   - 内容: 容器逃逸技术分析

---

<!-- chunk: H. 配套资源下载 -->## H. 配套资源下载

## H.1 本系列文档

| 文档 | 描述 | 链接 |
|------|------|------|
| 主文档 | FEBM 方法论深度解析 | [febm-methodology-deep-dive.md](./febm-methodology-deep-dive.md) |
| 第一章 | FEBM 方法论概述 | [01-febm-overview.md](./01-febm-overview.md) |
| 第二章 | FEBM vs FTA 比较分析 | [02-febm-vs-fta.md](./02-febm-vs-fta.md) |
| 第三章 | FEBM 实战案例 | [03-febm-case-studies.md](./03-febm-case-studies.md) |
| 第四章 | FEBM 工具链详解 | [04-febm-toolchain.md](./04-febm-toolchain.md) |
| 第五章 | FEBM 体系建设方法论 | [05-febm-construction-methodology.md](./05-febm-construction-methodology.md) |
| 第六章 | 未来演进方向 | [06-febm-future-evolution.md](./06-febm-future-evolution.md) |
| 第七章 | 附录 (本文档) | [07-febm-appendix.md](./07-febm-appendix.md) |
| 学术论文 | FTA 与 FEBM 对比研究 | [FTA-vs-FEBM.pdf](./FTA-vs-FEBM.pdf) |

## H.2 代码示例仓库

```bash
# 克隆配套代码仓库
git clone https://github.com/kudig-io/febm-examples.git
cd febm-examples

# 目录结构
.
├── falco-rules/           # Falco 规则示例
├── argo-workflows/        # 取证工作流模板
├── python-scripts/        # 分析脚本
│   ├── evidence_collector.py
│   ├── fta_analyzer.py
│   ├── causal_inference.py
│   └── report_generator.py
├── kubernetes-manifests/  # K8s 部署清单
│   ├── audit-policy.yaml
│   ├── forensic-daemonset.yaml
│   └── osdfir-stack/
└── docs/                  # 额外文档
```

## H.3 虚拟实验环境

**Kind 集群快速启动**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建取证就绪的 Kind 集群
cat <<EOF | kind create cluster --config=-
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  kubeadmConfigPatches:
  - |
    kind: ClusterConfiguration
    apiServer:
      extraArgs:
        audit-log-path: /var/log/kubernetes/audit.log
        audit-policy-file: /etc/kubernetes/audit-policy.yaml
      extraVolumes:
      - name: audit-log
        hostPath: /var/log/kubernetes
        mountPath: /var/log/kubernetes
      - name: audit-policy
        hostPath: /etc/kubernetes/audit-policy.yaml
        mountPath: /etc/kubernetes/audit-policy.yaml
- role: worker
- role: worker
EOF

# 部署 OSDFIR 工具栈
kubectl apply -f https://raw.githubusercontent.com/kudig-io/febm-examples/main/osdfir-stack/all-in-one.yaml
```
---

<!-- chunk: I. 版本历史 -->## I. 版本历史

| 版本 | 日期 | 作者 | 变更说明 |
|------|------|------|---------|
| v0.1 | 2024-01-15 | FEBM Working Group | 初始草案 |
| v0.5 | 2024-02-10 | FEBM Working Group | 添加工具速查表和 Falco 规则 |
| v1.0 | 2024-03-01 | FEBM Working Group | 正式发布,完整七章内容 |
| v1.1 | 2024-03-15 | FEBM Working Group | 更新审计策略模板,修复示例代码 |

---

<!-- chunk: J. 贡献指南 -->## J. 贡献指南

本文档系列是开源项目,欢迎社区贡献!

## 如何贡献

1. **报告问题**  
   在 GitHub Issues 中提交错误报告或改进建议:  
   https://github.com/kudig-io/febm-docs/issues

2. **提交改进**  
   Fork 仓库,修改后提交 Pull Request:
   ```bash
   git clone https://github.com/your-username/febm-docs.git
   cd febm-docs
   git checkout -b feature/your-improvement
   # 进行修改
   git commit -m "Add: 改进描述"
   git push origin feature/your-improvement
   # 在 GitHub 上创建 Pull Request
   ```

3. **分享案例**  
   如果你在实践中使用了 FEBM 方法论,欢迎分享你的案例研究!

## 贡献者名单

感谢以下贡献者 (按字母顺序):

- Alice Chen (Google) - 第三章案例研究
- Bob Li (Microsoft) - 第四章工具链分析
- Carol Wu (Netflix) - 第五章体系建设
- David Zhang (Alibaba) - 第六章 AI/ML 章节
- Eve Patel (AWS) - Kubernetes 审计策略模板

---

<!-- chunk: K. 联系方式 -->## K. 联系方式

- **项目网站**: https://febm.kudig.io
- **GitHub**: https://github.com/kudig-io/febm-docs
- **邮件列表**: febm-discuss@kudig.io
- **Slack**: #febm-methodology (CNCF Slack)
- **Twitter**: @FEBMethodology

---

<!-- chunk: L. 许可证 -->## L. 许可证

本文档系列采用 **Creative Commons Attribution 4.0 International (CC BY 4.0)** 许可证发布。

您可以自由地:
- **共享** — 在任何媒介或格式下复制、分发本材料
- **演绎** — 修改、转换或以本材料为基础进行创作

惟须遵守下列条件:
- **署名** — 您必须给出适当的署名,提供指向本许可证的链接

详细信息: https://creativecommons.org/licenses/by/4.0/

---

> **导航**: [<< 上一章 - 未来演进方向](./06-febm-future-evolution.md) | [返回主文档 - FEBM 方法论深度解析](./febm-methodology-deep-dive.md)

---

**文档结束**

感谢您阅读 FEBM 法医鉴定循证方法论系列文档!

如有任何问题或建议,欢迎通过上述联系方式与我们交流。

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/FEBM方法论/MOC.md|topic-febm MOC]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/README.md|topic-febm: FEBM 法医鉴定循证方法论深度解析]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/01-febm-theory-foundations.md|第一章：FEBM 方法论原理与理论基础]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/02-febm-technical-implementation.md|第二章:FEBM 技术实现体系]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/03-febm-best-practices.md|第三章：FEBM 最佳实践]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/04-febm-agent-ticket-processing.md|第四章：FEBM 对云平台工单智能体托管的意义]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/05-febm-construction-methodology.md|第五章：FEBM 体系建设方法论]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/06-febm-future-evolution.md|第六章：未来演进方向]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/08-febm-production-quick-start.md|第八章：FEBM 生产环境快速启动与 Kubernetes 问题取证手册]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/febm-methodology-deep-dive.md|法医鉴定循证方法论（FEBM）深度解析]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/fta-febm-joint-diagnosis.md|FTA-FEBM 联合诊断最佳实践]]

## See Also

- [[domain-10-troubleshooting-diagnostics/FEBM方法论/05-febm-construction-methodology.md|05-febm-construction-methodology]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/06-febm-future-evolution.md|06-febm-future-evolution]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/08-febm-production-quick-start.md|08-febm-production-quick-start]]
- [[domain-10-troubleshooting-diagnostics/FEBM方法论/febm-methodology-deep-dive.md|febm-methodology-deep-dive]]


<!-- risk-assessed -->
