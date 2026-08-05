---
title: 云原生应用开发与交付规范
description: 面向开发者的云原生应用规范：容器化规范（15-factor 落地）、镜像构建规范、JVM/Go/Node 运行时容器适配、配置外部化、可观测性埋点、Helm/Kustomize 交付规范与应用接入检查清单
summary: 开发者视角的生产应用规范：容器化、运行时容器适配（CPU throttling/堆内存/GOMAXPROCS）、配置与日志、OTel 埋点、交付物结构与接入 checklist
category: references
tags:
- k8s
- cloud-native
- application
- developer-guide
- production
tier: core
created: '2026-08-04'
last_updated: '2026-08-04'
difficulty: intermediate
audience:
- 应用开发者
- SRE
estimated_read_time: 25min
---

# 云原生应用开发与交付规范

> [[03-workload|工作负载最佳实践]] 是**平台视角**（怎么管工作负载），本文是**开发者视角**（怎么写、怎么打包、怎么交付一个适合跑在 K8s 上的应用）。两文配合使用：平台定底线，应用守规范。

## 1. 容器化规范（15-factor 落地版）

在云原生语境下，经典 12-factor 需要按 K8s 现实修正为可执行检查项：

| 规范 | 要求 | 违反的典型后果 |
|---|---|---|
| 无状态 | 状态外置（DB/缓存/对象存储）；必须本地的状态用 PVC 并明确声明 | 扩容后数据不一致、会话丢失 |
| 配置外置 | 环境差异全部走 ConfigMap/Secret/环境变量，镜像与环境无关 | 一套镜像走天下变一环境一镜像 |
| 单一进程职责 | 一容器一主进程；sidecar 模式分离辅助职能 | 信号处理失效、僵尸进程、日志混乱 |
| 快速启动 | 启动时间有预算（常规 < 60s，弹性敏感 < 10s）；慢启动用 startupProbe | HPA 扩容救不了火、滚动发布超时 |
| 优雅停机 | 正确处理 SIGTERM：停止接新 → 完成存量 → 退出（见 [[03-workload#4. 健康检查与优雅上下线]]） | 发布/驱逐时请求被砍断 |
| 健康端点 | 提供 `/healthz`（liveness）与 `/readyz`（readiness，独立于下游依赖） | 故障实例持续接流或全副本误摘除 |
| 日志到 stdout | 结构化 JSON 写 stdout，不写容器内文件 | 日志丢失、采集成本翻倍 |
| 可移植 | 不依赖特定节点/内核特性/云厂商 API（必须用走 DRA/CSI 等标准接口） | 调度约束锁死、迁移成本爆炸 |

## 2. 镜像构建规范

1. **多阶段构建**：构建期依赖不进最终镜像；目标体积 < 500 MB
2. **基础镜像收敛**：统一企业基础镜像库（distroless/alpine/统一 JDK 镜像），禁止随机公网基础镜像（[[12-security-hardening-baseline#5. 供应链安全]]）
3. **非 root 运行**：Dockerfile 显式 `USER` 非 root；与 [[03-workload#6. 安全基线（工作负载侧）]] 对齐
4. **标签规范**：不可变 tag（语义化版本或 git commit），digest 固定；`latest` 禁止进生产
5. **元数据**：OCI 标准 label（source、revision、created）打进镜像，溯源必备
6. **SBOM**：CI 生成并归档（Syft），签名（cosign）——核心应用强制
7. **CI 门禁**：高危漏洞阻断构建；构建过程禁用特权模式等危险操作

## 3. 运行时容器适配（事故高发区）

### 3.1 JVM 应用

容器环境的 JVM 经典坑，逐条核对：

| 项 | 正确姿势 |
|---|---|
| 容器感知 | JDK 8u191+ / 11+ 默认识别 cgroup 限制；更老版本必须升级或显式 `-Xmx` |
| 堆内存 | **不要**用 `-Xmx` 硬编码，用 `-XX:MaxRAMPercentage=75.0` 随容器 limit 自适应；requests=limits 时堆 + 非堆 + 元空间须留足余量 |
| CPU 与 GC 线程 | JVM 按可用 CPU 核数定 GC/编译线程数；设置 CPU limit 后 JVM 能正确感知（8u191+），老版本会按节点核数起线程导致争抢 |
| CPU throttling | CPU limit 触发 CFS 节流是 Java 服务 P99 毛刺的头号元凶：对时延敏感服务**不设 CPU limit 或改用 static CPU 绑定**（requests==limits 且整数核），监控 `container_cpu_cfs_throttled_seconds_total` |
| OOM 行为 | `-XX:+ExitOnOutOfMemoryError` 让 JVM 挂掉而不是带病运行，交给 K8s 重启 |

### 3.2 Go 应用

- `GOMAXPROCS` 不会自动随 CPU limit 调整——不设 limit 或引入 `uber-go/automaxprocs`；否则调度器按节点核数并行，CFS 节流下延迟劣化
- 优雅停机：`http.Server.Shutdown(ctx)` 配合 SIGTERM 处理

### 3.3 Node.js / Python 应用

- Node：`--max-old-space-size` 与容器 limit 对齐（默认按物理内存计算，容器内会 OOMKill）
- Python（gunicorn/uwsgi）：worker 数按容器 CPU 配置而非节点核数；注意多 worker × 内存的总量
- 通用原则：**线程池/连接池大小必须按容器资源画像配置，禁止按宿主机自动探测**

## 4. 配置外部化与治理

- 分层：默认值打进应用 → ConfigMap 环境配置 → Secret 敏感配置 → 外部配置中心（Nacos/Apollo）动态配置
- ConfigMap 只放**环境差异配置**，不放业务规则；单 ConfigMap < 100 KB（大配置治理见 [[21-release-engineering#4. 配置变更治理]]）
- 敏感信息禁止：镜像内、代码库、日志输出、环境变量明文（优先文件挂载 + 外部密钥管理，见 [[12-security-hardening-baseline#4. OWASP Kubernetes Top 10（2025 版）]] K03）
- 配置变更支持热更新或滚动重启，二选一明确，不允许"改了没人知道什么时候生效"

## 5. 可观测性埋点规范

| 项 | 规范 |
|---|---|
| Metrics | 暴露 `/metrics`（Prometheus 格式）：RED 四件套（请求量/错误率/时延分布/饱和度）；禁止无界 label（用户 ID、URL 全路径） |
| Tracing | 接入 OTel SDK，traceparent 头全链路透传；入口服务 100% 采样，内部按比例 |
| 日志 | JSON 结构化，必带 `traceId`、`level`、`ts`；ERROR 日志带错误码便于聚合 |
| 健康端点 | `/healthz`、`/readyz` 语义区分（[[03-workload#4.1 探针规范]] 的反面教训：readyz 不查下游） |
| 版本信息 | `/version` 或启动日志输出版本/commit，故障定位时快速确认"线上跑的是哪版" |

## 6. 交付物规范（Helm/Kustomize）

```text
deploy/
├── base/                  # 环境无关基线（Kustomize base 或 Helm chart）
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── hpa.yaml
│   ├── pdb.yaml
│   └── networkpolicy.yaml
└── overlays/              # 环境差异
    ├── staging/
    └── prod/
```

- 交付物必含五件套：Deployment + Service + HPA + PDB + NetworkPolicy（缺 PDB/NP 的交付物不允许上生产）
- 环境差异只通过 overlay/values 表达，禁止复制整份 YAML 改着用
- CRD/Operator 引入需平台评审（CRD 是集群级资源，失控会影响整个集群）
- 所有交付物进 Git，经 GitOps 晋级发布（[[21-release-engineering#5. GitOps 多环境晋级]]）

## 7. 应用接入生产检查清单（开发者自测）

> 应用首次接入或重大改造上线前，开发者自测 + 平台复核。与平台侧 [[07-pre-production-checklist#8. 安全就绪（上线视角）]] 互补。

- [ ] 镜像：非 root、无 latest、高危漏洞扫描通过、体积 < 500 MB
- [ ] 资源：requests/limits 按压测设定，无 BestEffort（[[03-workload#1.2 QoS 分级与应用]]）
- [ ] 健康：`/healthz` `/readyz` 语义正确，startupProbe 覆盖慢启动
- [ ] 停机：SIGTERM 优雅处理实测（滚动发布期间无 5xx 尖峰）
- [ ] 运行时：JVM/Go/Node 容器适配项逐条过（本文第 3 节）
- [ ] 弹性：HPA 指标合理，副本 ≥ 2，PDB 已配
- [ ] 分布：topologySpreadConstraints 跨节点/AZ
- [ ] 观测：metrics/tracing/结构化日志接入，告警规则已配置
- [ ] 安全：SecurityContext 基线、SA token 按需、无敏感信息硬编码
- [ ] 交付：五件套齐备、全部进 Git、NetworkPolicy 明确出入规则

## Related

- [[03-workload|工作负载最佳实践（平台视角）]]
- [[21-release-engineering|发布工程与变更管理（交付与发布）]]
- [[12-security-hardening-baseline|安全加固基线（供应链）]]
- [[09-observability|可观测性体系（指标消费侧）]]
