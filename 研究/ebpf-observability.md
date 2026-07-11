---
title: eBPF 可观测性深度应用研究
summary: 深入研究 eBPF 在系统可观测性中的深度应用，覆盖网络可观测性、安全审计、性能 Profiling 和协议解析。
category: research
tags:
- research
- ebpf
- observability
- profiling
- security-audit
- cilium
- hubble
- parca
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# eBPF 可观测性深度应用研究

## 研究背景

eBPF 不仅是网络数据平面的革命，更是系统可观测性的范式转移。传统可观测性依赖应用埋点（SDK/APM Agent），存在侵入性、语言绑定、更新成本高等问题。eBPF 在内核态运行，提供零侵入、全语言通用、实时高效的可观测能力。

## 核心问题

1. eBPF 如何在不修改应用代码的情况下实现 HTTP/gRPC/DNS 协议解析？
2. Cilium Hubble 的网络可观测性能力边界在哪里？
3. eBPF Profiling（Parca/Inspektor Gadget）如何替代传统 pprof？
4. eBPF 安全审计（Tetragon/Falco）在运行时安全中的角色？

## 调研发现

### 发现一：eBPF 可观测性四大维度

| 维度 | eBPF 工具 | 对应传统方案 | 优势 |
|------|----------|-------------|------|
| **网络** | Cilium Hubble | Istio sidecar | 无 sidecar，全协议 |
| **安全** | Tetragon/Falco | Audit log + SIEM | 实时内核事件，零延迟 |
| **性能** | Parca/Inspektor | pprof + APM Agent | 全语言通用，持续采样 |
| **协议** | Pixie | SDK + APM | 零代码修改，L7 解析 |

### 发现二：Cilium Hubble 可观测能力

```bash
# 🟢 实时流量可视化
hubble observe --follow                     # 全集群实时流量
hubble observe --namespace prod             # 指定命名空间
hubble observe --pod web-app               # 指定 Pod

# 🟢 L7 协议级观测
hubble observe --type l7                    # HTTP/gRPC/Kafka
hubble observe --protocol http --verdict DROPPED  # 被 L7 策略丢弃的 HTTP

# 🟢 DNS 解析追踪
hubble observe --protocol dns               # DNS 查询和响应

# 🟢 网络策略审计
hubble observe --verdict DROPPED           # 被网络策略阻断的连接
hubble observe --from-pod checkout --to-pod payment  # 特定路径

# 🟢 流量指标（Prometheus 兼容）
hubble observe -o json --since 5m          # JSON 格式，适合 SIEM 消费
```

### 发现三：Tetragon 运行时安全审计

Tetragon 基于 eBPF 提供实时安全策略执行：

```yaml
# 检测并阻断可疑行为
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: detect-reverse-shell
spec:
  kprobes:
  - call: "sys_connectat"
    syscall: true
    args:
    - index: 0
      type: "int"
    - index: 1
      type: "sockaddr"
      sizeArgIndex: 3
    selectors:
    - matchBinaries:
      - operator: "In"
        values: ["/bin/bash", "/bin/sh"]    # shell 进程
      matchArgs:
      - index: 1
        operator: "DAddr"
        values: ["10.0.0.0/8"]              # 连接到内网
      matchActions:
      - action: Sigkill                      # 直接杀掉进程
```

### 发现四：eBPF Profiling 革命

Parca（基于 eBPF 的持续 Profiling）解决了传统 pprof 的三大痛点：

| 传统 pprof | eBPF Profiling (Parca) |
|-----------|----------------------|
| 需要在应用中 import pprof | 零代码修改 |
| 手动触发采样 | 持续后台采样 |
| 每种语言 API 不同 | 全语言通用（内核级采样） |
| 影响应用性能（~1-3% CPU） | 极低开销（<0.1% CPU） |

### 发现五：Pixie 零侵入 APM

Pixie 使用 eBPF 实现 APM 级别的应用可观测性：

```
传统 APM:
  应用代码 → SDK 埋点 → APM Agent → Collector → Backend
  问题: 代码侵入、语言绑定、版本耦合

Pixie (eBPF):
  内核 → eBPF probe → Pixie Agent → Backend
  优势: 零修改应用、自动发现、语言无关

能力:
  → HTTP 请求追踪（自动捕获 Request/Response）
  → 数据库查询分析（SQL 语句级别）
  → JVM/GC 事件（Java 应用无需 JMX）
  → 消息队列延迟（Kafka/Redis 命令级别）
```

## 结论与建议

1. **eBPF 可观测性是零侵入的未来**：不需要应用埋点，不需要 sidecar，不需要 SDK。
2. **Cilium Hubble 是网络可观测的标配**：与 NetworkPolicy 无缝集成，实时流量可视化。
3. **Tetragon 是运行时安全的首选**：比 Falco 更强的策略执行能力（不仅检测，还能阻断）。
4. **Parca 应纳入持续 Profiling 体系**：与 Grafana 集成，CPU/内存火焰图持续采集。
5. **Pixie 适合快速 APM 落地**：不需要修改代码即可获得应用级可观测性。

## 参考资料

- Cilium Hubble: https://docs.cilium.io/en/stable/observability/hubble/
- Tetragon: https://tetragon.io/
- Parca: https://www.parca.dev/
- Pixie: https://pixielabs.ai/
- [[可观测性/index.md|可观测性目录]]
- [[研究/ebpf-networking-revolution.md|eBPF 网络革命]]
- [[研究/zero-trust-k8s-security.md|零信任安全架构]]

## Related

- [[综合/ebpf-observability.md|eBPF × 可观测性]]
- [[研究/observability-evolution.md|可观测性体系演进]]
