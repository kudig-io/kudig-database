---
title: 监控与日志系统集成源码分析
description: 基于 prometheus-3.13.0 与 metrics-server-0.8.1 本地源码的可观测性集成剖析：K8s 服务发现、抓取循环、资源指标管道与 EFK/OTel 日志链路
summary: 从 Prometheus 的 kubernetes SD 与 scrapeLoop 源码（行号实测）拆解监控系统如何 watch K8s 动态发现目标，剖析 metrics-server→HPA 资源指标管道、EFK 日志采集路径与 OTel 统一管道，给出可观测性链路排障方法。
category: source-analysis
tags:
- k8s
- source-code
- prometheus
- metrics-server
- grafana
- efk
- opentelemetry
- observability
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- Prometheus 如何发现 K8s 目标
- metrics-server 与 HPA 的关系
- K8s 日志采集链路
- ServiceMonitor 不生效怎么排查
trigger_keywords:
- Prometheus
- kubernetes_sd_configs
- ServiceMonitor
- metrics-server
- HPA
- EFK
- Fluent Bit
- OpenTelemetry
related_domains:
- 可观测性
- 集群基础
- 可靠性
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# 监控与日志系统集成源码分析

> **源码基线**：`33-源码/可观测性/{prometheus-3.13.0,metrics-server-0.8.1}/`（行号实测）；Grafana/EFK 侧为机制级分析（源码树待入库，见 [[33-源码/README.md|33-源码 待补充清单]]）
> 本篇属 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 一、两条独立的指标管道

K8s 集群里并存两条容易混淆的指标链路，集成点、协议、用途完全不同：

| | 监控管道（Prometheus） | 资源指标管道（metrics-server） |
|---|----------------------|------------------------------|
| 数据流 | exporter/应用 `/metrics` → Prometheus TSDB | kubelet `/metrics/resource` → 内存聚合 → Metrics API |
| 集成方式 | kubernetes_sd（watch apiserver 发现目标） | APIService 聚合（`metrics.k8s.io` 注册进 apiserver） |
| 消费者 | Grafana/Alertmanager/长期存储 | HPA、`kubectl top` |
| 保留 | 磁盘 TSDB，可远程写 | 只留最近一次，无历史 |

**HPA 不读 Prometheus**——它经 apiserver 聚合层调 Metrics API。要用业务指标扩缩容需再架一层 prometheus-adapter（或 KEDA）把 PromQL 翻译成 custom.metrics.k8s.io。

## 二、Prometheus 的 K8s 服务发现

```go
// prometheus-3.13.0/discovery/kubernetes/kubernetes.go（实测行号）
func New(l *slog.Logger, metrics discovery.DiscovererMetrics, conf *SDConfig)  // :284
```

kubernetes SD 就是一个消费级 informer 用户（[[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 篇]]机制的又一次外部复用）：按 role（node/service/pod/endpoints/endpointslice/ingress）建对应 informer，把对象增量翻译成 targetgroup 推给抓取层。每个对象的字段全部展开为 `__meta_kubernetes_*` 标签——**relabel_configs 的本质是在这批元标签上做筛选与改写**（keep 带注解的 Pod、把 label 映射为指标维度）。

```go
// prometheus-3.13.0/scrape/scrape.go（实测行号）
func newScrapePool(cfg *config.ScrapeConfig, app storage.Appendable, ...)  // :138  每个 job 一个池
func (sl *scrapeLoop) run(errc chan<- error)                              // :1263 每目标一个循环
```

`scrapePool`(:138) 响应 SD 的目标增删——Pod 滚动更新时旧 target 停、新 target 起，无需重启；每个 target 一个 `scrapeLoop.run`(:1263)，按 interval 定时 GET、解析、写入 TSDB，并附加 `up`/`scrape_duration_seconds` 等合成指标。**`up == 0` 是排障第一入口**：目标在列表里但抓不动（网络/端口/超时），与「目标根本没被发现」（SD/relabel 问题）是两类故障。

Prometheus Operator 的 ServiceMonitor/PodMonitor 是在此之上的声明式封装：Operator watch 这些 CRD → 生成 kubernetes_sd + relabel 配置 → 热加载 Prometheus。**ServiceMonitor 不生效的第一排查点永远是 label selector 三级匹配**：ServiceMonitor 选 Service 的 label、Service 选 Pod、Prometheus CR 的 serviceMonitorSelector 选 ServiceMonitor——任一级不匹配都静默无目标。

## 三、metrics-server：资源指标的极简管道

```go
// metrics-server-0.8.1（实测行号）
// pkg/scraper/scraper.go
func (c *scraper) Scrape(baseCtx context.Context) *storage.MetricsBatch   // :115  并发抓全部节点
func (c *scraper) collectNode(ctx, node)                                  // :186  单节点抓取
// pkg/scraper/client/resource/client.go
func (kc *kubeletClient) GetMetrics(ctx, node)                            // :83   GET kubelet /metrics/resource
```

链路：cAdvisor（内嵌 kubelet）统计 cgroup → kubelet `/metrics/resource` 端点 → metrics-server 每 15s `Scrape`(:115) 全节点 → 内存存储 → 经 APIService 聚合暴露为 `metrics.k8s.io`。

生产要点：

- **`kubectl top` 报 `Metrics API not available`** 三连查：metrics-server Pod 状态 → `kubectl get apiservice v1beta1.metrics.k8s.io`（False 时看 message）→ metrics-server 到 kubelet 10250 的连通性与证书（`--kubelet-insecure-tls` 是测试环境常见妥协）
- **HPA 抖动的根源常在此管道**：指标 15s 粒度 + HPA 15s 同步周期 + 容器刚启动无数据（HPA 视为缺失按保守处理）——`behavior.scaleDown.stabilizationWindowSeconds` 是正解而非调小周期
- kube-state-metrics 与 metrics-server 职责互补：前者把 K8s **对象状态**（副本数、Pod phase）转为 Prometheus 指标，不涉及资源用量

## 四、日志链路：EFK 与 OTel

K8s 本身不存日志——kubelet 只把容器 stdout/stderr 落盘到 `/var/log/pods/<ns>_<pod>_<uid>/<container>/*.log`（CRI 统一格式，`kubectl logs` 即 kubelet 读此文件）。采集体系全部构建在这个约定之上：

```
容器 stdout ─▶ /var/log/pods/...（kubelet+运行时落盘，containerd 配置轮转大小）
                  │ DaemonSet 采集器 tail + 按路径解析出 ns/pod/container 元数据
                  │ （Fluent Bit / Fluentd / Filebeat / vector / otel-collector filelog）
                  ▼
        富化（对 apiserver 查 Pod label/annotation 附加到日志条目）
                  ▼
        Elasticsearch / Loki / Kafka ─▶ Kibana / Grafana
```

- **采集器的 K8s 集成点有二**：①路径约定（文件名内嵌 ns/pod/uid，无需任何 API 即可归属）；②kubernetes filter（查 apiserver 富化 label——大集群下这一步的 apiserver 查询压力需要开元数据缓存）
- **日志丢失的经典根因**：容器写文件而非 stdout（采集不到）、日志轮转过快（tail 追不上）、Pod 删除后文件即清（缓冲未刷完）
- OTel Collector（`33-源码/可观测性/opentelemetry-collector-0.156.0`）正在统一三信号：filelog receiver 替代 Fluent Bit、prometheus receiver 兼容抓取、OTLP 统一外送——选型演进见 [[09-可观测性/03-日志/index.md|可观测性域：日志]]

## 五、生产排障速查

| 症状 | 链路定位 | 检查手段 |
|------|---------|---------|
| 目标未被发现 | SD/relabel | Prometheus UI Service Discovery 页看 dropped 原因、relabel 规则 |
| up == 0 | 抓取失败 | 目标端口连通性、NetworkPolicy、scrape_timeout |
| ServiceMonitor 不生效 | 三级 selector | ServiceMonitor↔Service↔Pod label 逐级核对、Prometheus CR selector |
| kubectl top 不可用 | Metrics API | APIService 状态、metrics-server 日志、kubelet 10250 证书 |
| HPA 不扩容 | 指标缺失/计算 | `kubectl describe hpa` conditions、Pod requests 是否设置（利用率=用量/request） |
| 日志缺失 | 采集链路 | 容器是否写 stdout、`/var/log/pods` 文件存在性、采集器背压指标 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|kubernetes-core 06 - 声明式 API 与 Informer 机制]]（SD 的机制基础）
- [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|kubernetes-core 02 - kube-apiserver 源码深度剖析]]（APIService 聚合层）
- [[09-可观测性/02-指标/index.md|可观测性域：指标]]
- [[09-可观测性/03-日志/index.md|可观测性域：日志]]
- [[09-可观测性/05-告警/index.md|可观测性域：告警]]
- [[12-可靠性/README.md|可靠性域]]（SLO 消费侧）
