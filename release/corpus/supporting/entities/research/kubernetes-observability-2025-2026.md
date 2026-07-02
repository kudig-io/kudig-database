---
title: Kubernetes Observability 2025 2026
summary: 'First major release in 7 years. Key features:'
category: entities
tags:
- kubernetes-observability-2025-2026
tier: supporting
created: '2026-07-01'
---

# Kubernetes Observability 2025-2026: Research Findings

## 1. OpenTelemetry (OTel) Evolution

### CNCF Graduated Project (2026)
- OpenTelemetry officially became a CNCF Graduated project in early 2026
- Source: https://opentelemetry.io/blog/2026/otel-graduates/

### Signal Status (2025-2026)
- **Traces**: Stable (GA since 2023)
- **Metrics**: Stable (GA)
- **Logs**: Stable (GA) — log collection and OTel-native logs are production-ready
- **Profiling**: NEW signal type announced in 2024, active development with Elastic contributing their profiling agent
- **Events**: Part of the logs signal, gaining prominence for Kubernetes event collection
- Source: https://opentelemetry.io/status/

### Collector & Operator
- Collector roadmap toward v1 published in 2024; declarative configuration support added
- Container log parser introduced (2024) for Kubernetes container log collection
- OTel Operator Q&A published (2024) — the Operator handles auto-instrumentation injection, Collector lifecycle, and bridge CRDs
- Collector Survey (2024) showed massive adoption growth
- Skyscanner case study (2026): managing Collectors across 24 production K8s clusters
- Source: https://opentelemetry.io/blog/2024/collector-roadmap/
- Source: https://opentelemetry.io/blog/2024/otel-operator-q-and-a/
- Source: https://opentelemetry.io/blog/2026/devex-skyscanner/

### GenAI Observability
- OpenTelemetry added GenAI/LLM observability support (2024-2026), instrumenting LLM calls with traces and metrics
- Source: https://opentelemetry.io/blog/2024/llm-observability/
- Source: https://opentelemetry.io/blog/2026/genai-observability/

### OpenTracing Deprecation
- OpenTracing compatibility requirements deprecated in 2026, completing the migration path
- Source: https://opentelemetry.io/blog/2026/deprecating-opentracing-compatibility/

---

## 2. Prometheus Ecosystem

### Prometheus 3.0 (Released Nov 14, 2024)
First major release in 7 years. Key features:
- **New UI**: Completely rewritten with modern stack, PromLens-style tree view, enabled by default
- **Remote Write 2.0**: Native support for metadata, exemplars, created timestamps, native histograms; string interning for reduced payload
- **UTF-8 Support**: All valid UTF-8 characters in metric and label names; no more dots-to-underscores mangling
- **OTLP Ingestion**: Native OTLP metrics receiver at `/api/v1/otlp/v1/metrics`; experimental translation strategies for seamless OpenTelemetry interop
- **Native Histograms**: Experimental (opt-in via `--enable-feature=native-histograms`); exponential bucket boundaries replace manual bucket selection
- **Performance**: Significant CPU and memory improvements over v2.0 and v2.18
- **Breaking Changes**: Minor; migration from v2.55 recommended before upgrading to v3.0
- **Roadmap**: OpenMetrics 2.0 (under Prometheus governance), more OTel features, native histogram stability with custom buckets
- Source: https://prometheus.io/blog/2024/11/14/prometheus-3-0/

### Thanos
- Continues as the de facto Prometheus long-term storage and federation solution
- PromLens-style features now in Prometheus 3.0 reduce some Thanos UI dependency
- Pyrra (SLO tool) explicitly supports Thanos with downsampling headers and partial response disabling

### Cortex / Grafana Mimir
- **Cortex**: Largely superseded by Grafana Mimir for new deployments; Cortex project still maintained but Mimir is the recommended path
- **Grafana Mimir**: Scalable, performant Prometheus-compatible metrics backend
  - Multi-tenant, horizontally scalable
  - Native Prometheus remote write and OTLP ingestion
  - Part of Grafana's LGTM stack (Loki, Grafana, Tempo, Mimir)
  - Source: https://grafana.com/oss/mimir/

---

## 3. Grafana Ecosystem

### Grafana 11+ and Grafana 13 (2025-2026)
- Grafana 13 announced at GrafanaCON 2026 (April 2026)
- Continued focus on AI-powered observability, unified dashboards, and correlation
- Source: https://grafana.com/blog/grafanacon-2026-announcements/

### Grafana Loki (Log Aggregation)
- Loki 3.4+ released in 2025
- Multi-tenant log aggregation system designed for cost efficiency
- Native OTel log ingestion support
- "Like Prometheus, but for logs" — label-based indexing, not full-text
- Source: https://grafana.com/oss/loki/

### Grafana Tempo (Distributed Tracing)
- High-scale distributed tracing backend
- Native OpenTelemetry protocol support (OTLP)
- TraceQL query language for searching traces
- Source: https://grafana.com/oss/tempo/

### Grafana Mimir (Metrics)
- Successor to Cortex for Prometheus-compatible metrics at scale
- Multi-tenant, long-term storage, global view
- Source: https://grafana.com/oss/mimir/

### Grafana Pyroscope (Profiling)
- Scalable continuous profiling (absorbed the Phlare project)
- Complements traces/metrics/logs with the 4th pillar: profiles
- Source: https://grafana.com/oss/pyroscope/

### Grafana Alloy (Collector)
- OpenTelemetry Collector distribution with native Prometheus pipeline support
- Replaces Grafana Agent as the recommended telemetry collector
- eBPF auto-instrumentation via Beyla integration
- Source: https://grafana.com/oss/alloy-opentelemetry-collector/

### Grafana Beyla (eBPF)
- eBPF-based auto-instrumentation for application observability
- Zero-code instrumentation for HTTP/gRPC/SQL services
- Source: https://grafana.com/oss/beyla-ebpf/

### Full LGTM Stack
The recommended Grafana observability stack:
- **L**oki (logs) + **G**rafana (visualization) + **T**empo (traces) + **M**imir (metrics)
- Plus Pyroscope (profiles), Alloy (collector), Beyla (eBPF), Faro (frontend RUM)

---

## 4. Distributed Tracing

### Jaeger
- Jaeger v2 released (2024) based on OpenTelemetry Collector architecture
- Fully embraces OTLP as native protocol; Jaeger exporter in OTel Collector deprecated in favor of OTLP
- Source: https://opentelemetry.io/blog/2023/jaeger-exporter-collector-migration/

### Zipkin
- Still maintained but adoption declining relative to OTel-native backends
- OTel Collector can export to Zipkin for backward compatibility

### OpenTelemetry as Standard
- OTLP is now the de facto standard for trace propagation
- W3C Trace Context and B3 propagation formats supported
- OTel SDKs (Go, Java, Python, .NET, JS, etc.) all support stable trace APIs
- Auto-instrumentation available for most languages/frameworks

---

## 5. eBPF Observability

### Cilium Hubble
- Network observability for Cilium CNI
- Provides L3/L4/L7 flow visibility, DNS monitoring, network policy enforcement visualization
- Deep integration with Kubernetes service mesh and networking
- Source: https://docs.cilium.io/en/stable/observability/

### Cilium Tetragon (v1.0+)
- eBPF-based security observability and runtime enforcement
- Process execution, file access, network activity monitoring without sidecar containers
- Kubernetes-aware: maps events to pods, namespaces, and labels
- Can enforce security policies in real-time (not just observe)
- Source: https://isovalent.com/blog/post/cilium-tetragon-1-0/

### Pixie (CNCF Sandbox)
- Auto-instrumented Kubernetes observability using eBPF
- Captures HTTP, gRPC, DNS, MySQL, PostgreSQL traffic without code changes
- In-cluster edge computing model (query data in the cluster, no external storage required)
- Note: Pixie project activity has slowed; Grafana Beyla is emerging as an alternative for eBPF auto-instrumentation

### Grafana Beyla
- eBPF-based automatic instrumentation producing OpenTelemetry-compatible metrics and traces
- Zero-code instrumentation for HTTP, gRPC, SQL, Redis, and more
- Integrates directly with Grafana Alloy/OTel Collector pipelines

---

## 6. Kubernetes Events and Audit Logging

### Kubernetes Events
- Native Kubernetes events (watchable via `kubectl get events`) provide pod lifecycle, scheduling, and health information
- OTel Collector's `k8s_events` receiver can collect and export these as structured logs/traces
- kubeletstats receiver provides node/pod/container metrics (CPU, memory, disk, network)
  - Note: metric naming transition in 2025 from `.cpu.utilization` to `.cpu.usage` pattern
  - Source: https://opentelemetry.io/blog/2025/kubeletstats-receiver-metrics-deprecation/

### Kubernetes Audit Logging
- K8s audit logs capture API server requests (who did what, when, to which resource)
- Can be collected via OTel Collector or fluentd/fluentbit pipelines
- Critical for security observability and compliance
- Correlation with Tetragon for runtime security observability

### Best Practices (2025)
- Use OTel Collector as DaemonSet for node-level log/metric collection
- Use OTel Operator for auto-instrumentation injection
- Collect K8s events, audit logs, and container logs through unified pipeline
- Correlate with traces using trace context propagation through K8s metadata

---

## 7. SLO Monitoring

### Pyrra (CNCF-adjacent, actively maintained)
- Making SLOs with Prometheus manageable, accessible, and easy to use
- Features:
  - Kubernetes Operator: watches SLO CRDs, generates Prometheus recording rules and alert rules
  - Filesystem-based operator for non-K8s deployments
  - Generates Multi Burn Rate Alerts (4 severity levels)
  - UI for SLO listing, error budget visualization, RED metrics
  - Grafana dashboard integration
  - Thanos support (downsampling, partial response handling)
  - Mimir support
- CRD-based SLO definitions (`pyrra.dev/v1alpha1/ServiceLevelObjective`)
- Supports ratio-based and latency-based SLOs
- Source: https://github.com/pyrra-dev/pyrra
- Demo: https://demo.pyrra.dev

### Sloth
- Simple SLO generator that creates Prometheus recording and alerting rules
- Plugin system for custom SLI sources
- Generates multi-window, multi-burn-rate alerts following Google SRE book methodology
- Works with Prometheus, Thanos, and Mimir
- Source: https://github.com/slok/sloth

### slo-libsonnet
- Jsonnet library for defining SLOs as code
- Source: https://github.com/metalmatze/slo-libsonnet

### Grafana SLO (Commercial)
- Built-in SLO management in Grafana Cloud
- Integrated with Grafana Incident Response & Management (IRM)

---

## Key Trends Summary (2025-2026)

1. **OTel is the standard**: OpenTelemetry graduated at CNCF, becoming the universal instrumentation framework. All major backends support OTLP natively.

2. **Prometheus 3.0 bridges OTel gap**: Native OTLP ingestion + UTF-8 support eliminates friction between OTel and Prometheus.

3. **LGTM stack dominates self-hosted**: Grafana's Loki+Grafana+Tempo+Mimir+Pyroscope is the reference open-source observability stack.

4. **eBPF goes mainstream**: Tetragon, Beyla, and Hubble enable zero-code observability for K8s workloads. Beyla produces OTel-native telemetry.

5. **4 pillars + profiling**: Observability now includes metrics, logs, traces, AND continuous profiling (Pyroscope/Phlare).

6. **SLO tooling matures**: Pyrra and Sloth provide Kubernetes-native SLO management with Prometheus integration.

7. **GenAI observability**: OTel now supports instrumenting LLM/AI workloads for performance and cost monitoring.
