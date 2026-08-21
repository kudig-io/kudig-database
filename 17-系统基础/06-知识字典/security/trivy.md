---
title: Trivy
description: Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏...
summary: Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏...
category: dictionary
tags:
- k8s
- glossary
- trivy
- security
- scanning
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Trivy 是什么
- Trivy 详解
trigger_keywords:
- Trivy
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Trivy

> **英文名**: Trivy

## 概述

Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏感信息，是 Kubernetes 安全扫描的事实标准工具。

## 核心概念/原理

### 扫描能力

| 扫描目标 | 内容 |
|----------|------|
| Container Image | CVE 漏洞、OS 包、应用依赖 |
| Filesystem | IaC 错误配置、密钥泄露 |
| Git Repository | 代码中的安全问题和密钥 |
| Kubernetes Cluster | 集群配置错误和权限风险 |
| SBOM | 软件物料清单生成 |

### 支持的漏洞数据库

NVD、Alpine、Debian、Ubuntu、Red Hat、Amazon Linux、GitHub Advisory 等。

## 关键机制或特性

- **Trivy Operator**：Kubernetes 原生部署，自动扫描集群中的镜像和配置。
- **CI/CD 集成**：作为 GitHub Action、GitLab CI 步骤扫描镜像。
- **SBOM 生成**：输出 SPDX/CycloneDX 格式的软件物料清单。
- **Misconfiguration**：扫描 Terraform、Kubernetes YAML、Dockerfile。
- 支持 JSON/Table/SARIF 多种输出格式。

## 使用场景与最佳实践

- CI/CD 流水线中集成 `trivy image` 扫描构建的镜像。
- 部署 Trivy Operator 持续扫描集群中的运行镜像。
- 使用 `trivy config` 检查 Kubernetes YAML 的安全配置。
- 将 Trivy 结果集成到 GitHub Security Advisory。
- 定期生成 SBOM 满足合规要求。

## 架构深度解析

### Trivy 扫描引擎架构

```
┌──────────────────────────────────────────────────────────────┐
│  扫描入口                                                     │
│  ├─ CLI：trivy image/fs/config/repo/sbom                     │
│  ├─ CI/CD：流水线门禁（exit code 判定）                       │
│  ├─ Trivy Operator：集群内定时扫描（CRD）                     │
│  └─ Server 模式：多客户端共享扫描缓存                         │
│   │                                                           │
│   ▼ 镜像扫描流水线                                            │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 1. 镜像解析（registry 认证 / OCI layout）                │  │
│  │ 2. 层展开分析：扫描文件系统（含已删除层）                 │  │
│  │ 3. 组件识别：                                            
│  │    ├─ OS 包：apk/dpkg/rpm 数据库 + 指纹                   │
│  │    ├─ 语言依赖：lock 文件 / SBOM 内嵌                     │
│  │    └─ 二进制指纹：go/node 二进制识别                      │
│  │ 4. 漏洞匹配：组件版本 × CVE 数据库（NVD/GHSA/厂商）       │
│  │ 5. 分级：CRITICAL/HIGH/MEDIUM/LOW/UNKNOWN                 │
│  └─────────────────────────────────────────────────────────┘  │
│   │                                                           │
│   ▼ 其他模式                                                  │
│  ├─ config：IaC/K8s YAML/容器配置（misconfig）                │
│  ├─ fs/repo：目录与仓库扫描（含 secret 检测）                 │
│  └─ sbom：生成 SPDX/CycloneDX 清单                            │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（aquasecurity/trivy）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| CLI 入口 | `pkg/commands/` | image/fs/config 子命令 |
| 扫描器 | `pkg/scanner/` | 镜像/文件系统扫描编排 |
| 漏洞检测 | `pkg/detector/` | OS/语言漏洞匹配逻辑 |
| 数据源 | `pkg/db/` | 漏洞库下载与缓存（trivy-db） |

### 流程步骤

1. 拉取/定位镜像，解析 manifest 与各层（支持 OCI 与 Docker 格式）。
2. 逐层合并文件系统视图，识别操作系统与包管理器类型。
3. 提取包版本清单（OS 包 + 语言依赖 + 二进制指纹），生成组件图谱。
4. 与本地漏洞数据库（默认 trivy-db，可离线）按 CPE/包名-版本匹配。
5. 输出结果并按严重级别汇总；CI 模式按 `--severity` + `--exit-code` 门禁。

## 生产案例

### 案例 1：CI 扫描门禁误杀导致发布阻塞

| 时间 | 事件 |
| --- | --- |
| T+0 | 团队升级 Trivy 数据库后，存量镜像突然报出大量 HIGH 漏洞 |
| T+2h | CI 流水线因 `--exit-code 1` 门禁全部阻塞，发布停滞 |
| T+4h | 定位：数据库更新导致漏洞基线变化（新增 CVE），且门禁策略过严（HIGH 即阻断） |
| T+8h | 调整门禁：仅 CRITICAL 阻断，HIGH 预警；为存量镜像建立豁免清单 |
| T+1d | 流水线恢复，漏洞修复排入迭代计划 |

- **根因分析**：漏洞库是动态的，门禁策略必须区分"新增基线 vs 存量债务"；全量 HIGH 阻断会把发布系统变成"不可用即安全"。
- **修复命令**：
```bash
# 1. 本地查看结果分级（只读）
trivy image --severity CRITICAL,HIGH --ignore-unfixed --exit-code 0 myapp:v1.2
# 2. 更新门禁配置：仅 CRITICAL 阻断（🟡 中风险）
trivy image --severity CRITICAL --ignore-unfixed --exit-code 1 myapp:$TAG
# 3. 豁免已知可接受风险（依赖 .trivyignore 文件）
echo "CVE-2024-XXXXX" >> .trivyignore
```

### 案例 2：离线环境扫描数据库过期导致漏报

| 时间 | 事件 |
| --- | --- |
| T+0 | 内网集群 Trivy Operator 完成部署，扫描结果全部为 0 漏洞 |
| T+1w | 渗透测试发现运行镜像存在已公开 3 个月的严重漏洞 |
| T+2d | 定位：离线环境 trivy-db 停留在部署时的旧版本，从未更新 |
| T+3d | 建立离线数据库同步通道（镜像内网同步 + 定时更新），重新扫描 |
| T+5d | 扫描出 40+ 高危漏洞，逐批修复 |

- **根因分析**：Trivy 的检测能力依赖漏洞数据库时效性，离线环境必须建立 trivy-db 的定期同步机制（air-gap 更新）。
- **修复命令**：
```bash
# 1. 检查数据库版本（只读）
trivy image --version
# 2. 离线更新：在有网环境下载并导入（🟡 中风险）
trivy image --download-db-only --db-repository registry.example.com/trivy-db
# 3. 定时任务同步（cron）
0 2 * * * trivy image --download-db-only --db-repository registry.example.com/trivy-db
```

## 对比评测

| 维度 | Trivy | Grype | Clair | Snyk |
| --- | --- | --- | --- | --- |
| 扫描模式 | image/fs/config/repo/sbom | image/dir | 镜像仓库 API | image + IaC + 依赖 |
| 语言生态 | 广泛（OS + 语言 + 二进制） | 广泛 | 中 | 广泛 |
| 误报率 | 低（含 ignore-unfixed） | 中 | 中 | 低（人工研判） |
| 离线能力 | 好（db 导入） | 好 | 中 | 差（SaaS） |
| 商业化 | Apache-2.0 | Apache-2.0 | Apache-2.0 | 商业 |

**选型建议**：开源优先 Trivy（功能全、CI/Operator 一体）；需要商业级漏洞情报与 SCA 治理选 Snyk；已有 Harbor 仓库体系可叠加 Clair。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 扫描超时 | 大镜像/网络慢 | `trivy image --timeout 10m`；启用 server 模式共享缓存 |
| 结果为空 | 数据库过期/组件识别失败 | 更新 db；检查基础镜像 OS 识别 |
| 401 拉取失败 | registry 认证缺失 | 配置 `--registry.token` 或 docker login |
| 误报固定版本 | 无 fixed 版本信息 | `--ignore-unfixed` 过滤 |
| Operator 不扫描 | CRD 权限/命名空间选择器 | 检查 `kubectl get vulnerabilityreports` 与日志 |

## 生产部署清单

- [ ] 漏洞数据库更新自动化（在线定时 / 离线同步通道）
- [ ] CI 门禁分级：CRITICAL 阻断、HIGH 预警、MEDIUM 记录
- [ ] 部署 Trivy Operator 覆盖运行态镜像持续扫描
- [ ] 建立 `.trivyignore` 豁免流程（安全团队审批）
- [ ] SBOM 生成纳入发布制品（`trivy sbom --format cyclonedx`）

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 数据库超过 7 天未更新且扫描显示 0 漏洞 | 立即同步数据库并全量重扫 |
| P1 | 门禁策略全量阻断导致发布瘫痪 | 改为分级门禁 + 豁免清单 |
| P2 | 运行态镜像无持续扫描 | 部署 Trivy Operator |

## 面试要点

1. **Q：Trivy 如何识别镜像中的组件？**
   A：多层机制：解析 OS 包管理器数据库（apk/dpkg/rpm）+ 扫描 lock 文件（package-lock.json、go.sum 等）+ 二进制指纹识别（编译产物中的 Go module 信息）；对无法直接读取的层做文件系统合并分析。识别后按"包名-版本"与 CVE 数据库匹配。
2. **Q：CI 中如何设置合理的漏洞门禁？**
   A：分级策略：CRITICAL 阻断（`--exit-code 1`）、HIGH 预警（`--exit-code 0` + 报告）、MEDIUM/LOW 记录；结合 `--ignore-unfixed` 区分"可修复债务"与"上游未修复"；建立豁免清单与修复时限（SLA），避免全量阻断导致发布瘫痪。
3. **Q：离线环境如何保证扫描有效性？**
   A：两条链路：一是 trivy-db 定期从有网环境下载并导入内网镜像仓库，定时任务更新；二是 Server 模式部署在内网统一提供缓存与数据库。关键是建立数据库时效性监控（比对 DB 发布时间），防止"扫描了但数据过期"的假安全。

## 运维要点

- 数据库时效：监控 trivy-db 发布时间，超期告警并触发同步。
- 性能调优：大规模扫描用 Trivy Server 共享缓存与并发控制。
- 报告归档：每次发布的漏洞报告归档，支撑合规审计与趋势分析。
- 与修复联动：扫描结果（vulnerabilityreports）对接工单系统，按 SLA 闭环。
- 排障入口：先验证数据库版本与镜像可拉取性，再分析组件识别与匹配结果。

## 参考链接

- [Trivy Official](https://aquasecurity.github.io/trivy/)

## Related

- [[17-系统基础/06-知识字典/security/pod-security-policy.md|Pod Security Policy]]
- [[17-系统基础/06-知识字典/security/security-context.md|Security Context]]
- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
- [[17-系统基础/06-知识字典/security/certificate.md|Certificate]]
- [[17-系统基础/06-知识字典/security/admission-controller.md|Admission Controller]]


<!-- risk-assessed -->
