# Falco 运行时安全监控部署指南

> **适用版本**: Falco v0.41.0 / Falco Sidekick v2.29  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、架构设计](#一架构设计)
- [二、Helm 部署](#二helm-部署)
- [三、规则定制](#三规则定制)
- [四、Falco Sidekick 响应](#四falco-sidekick-响应)
- [五、与 Prometheus/Grafana 集成](#五与-prometheusgrafana-集成)
- [六、与 K8s 审计日志集成](#六与-k8s-审计日志集成)
- [七、性能调优](#七性能调优)
- [八、常见问题](#八常见问题)

---

## 一、架构设计

```
┌──────────────────────────────────────────────┐
│                 K8s Node                      │
│  ┌────────────────────────────────────────┐  │
│  │  Falco (DaemonSet)                     │  │
│  │  ├── Kernel Module / eBPF Probe        │  │
│  │  ├── Userspace Engine (libsinsp)       │  │
│  │  └── Rule Engine (Falco Rules)         │  │
│  └────────────┬───────────────────────────┘  │
│               │ gRPC / HTTP                  │
│  ┌────────────┴───────────────────────────┐  │
│  │  Falco Sidekick (可选)                  │  │
│  │  ├── Slack / Teams / Discord           │  │
│  │  ├── PagerDuty / Opsgenie              │  │
│  │  ├── SQS / SNS / Pub/Sub               │  │
│  │  ├── Webhook / Lambda                  │  │
│  │  └── Loki / Elasticsearch              │  │
│  └────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

---

## 二、Helm 部署

### 2.1 生产级 values

```yaml
# values-falco-production.yaml
cat << 'EOF' > values-falco-production.yaml
driver:
  kind: ebpf  # 或 module (内核模块)
  # ebpf.leastPrivileged: true  # v0.40+ 最小权限模式

tty: false

collectors:
  enabled: true
  docker:
    enabled: false  # containerd 环境禁用
  containerd:
    enabled: true
    socket: /run/containerd/containerd.sock
  crio:
    enabled: false

falco:
  rules_file:
    - /etc/falco/falco_rules.yaml
    - /etc/falco/falco_rules.local.yaml
    - /etc/falco/rules.d

  json_output: true
  json_include_tags_property: true

  http_output:
    enabled: true
    url: "http://falco-falcosidekick:2801"
    # 或发送到远程 Falco Sidekick

  file_output:
    enabled: false

  stdout_output:
    enabled: true

  syslog_output:
    enabled: false

  program_output:
    enabled: false

  http_server:
    enabled: true
    listen_port: 8765
    k8s_healthz_endpoint: /healthz
    # 用于 Prometheus metrics

  grpc:
    enabled: true
    bind_address: "0.0.0.0"
    threadiness: 2

  grpc_output:
    enabled: true

resources:
  requests:
    cpu: 100m
    memory: 512Mi
  limits:
    cpu: 1000m
    memory: 1Gi

extra:
  env:
    # 设置 Falco 缓冲区大小
    - name: FALCO_BUFSIZE
      value: "8388608"
EOF

# 部署 Falco
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm repo update

helm install falco falcosecurity/falco \
  --namespace falco \
  --create-namespace \
  --values values-falco-production.yaml \
  --version 4.21.0

# 部署 Falco Sidekick
helm install falcosidekick falcosecurity/falcosidekick \
  --namespace falco \
  --set config.debug=false \
  --set config.slack.webhookurl="https://hooks.slack.com/services/XXX" \
  --set config.slack.minimumpriority="critical"
```

---

## 三、规则定制

### 3.1 自定义规则 ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-custom-rules
  namespace: falco
  labels:
    app.kubernetes.io/name: falco
    role: rules
data:
  custom_rules.yaml: |
    - rule: Privileged Container Started in Production
      desc: Detect privileged containers in production namespace
      condition: >
        spawned_process
        and container
        and container.privileged = true
        and k8s.ns.name in (production, api-production)
      output: >
        Privileged container started in production
        user=%user.name command=%proc.cmdline
        container=%container.name namespace=%k8s.ns.name pod=%k8s.pod.name
      priority: CRITICAL
      tags: [k8s, privilege_escalation, production]

    - rule: Sensitive File Access in Container
      desc: Detect access to sensitive files
      condition: >
        open_read
        and container
        and (fd.name contains "/etc/shadow"
             or fd.name contains "/etc/ssh/ssh_host_")
      output: >
        Sensitive file accessed
        user=%user.name file=%fd.name
        container=%container.name pod=%k8s.pod.name
      priority: WARNING
      tags: [filesystem, sensitive_data]

    - rule: Outbound Connection from Database Pod
      desc: Database pods should not make outbound connections
      condition: >
        outbound
        and k8s.pod.label.app in (postgres, mysql, redis)
        and not (fd.sip in (10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16))
      output: >
        Unexpected outbound connection from database
        connection=%fd.name pod=%k8s.pod.name namespace=%k8s.ns.name
      priority: NOTICE
      tags: [network, database, anomaly]

    # 白名单示例
    - macro: allowed_admin_containers
      condition: (container.image.repository in (gcr.io/company/admin-tool))

    - rule: Admin Shell Access
      desc: Detect shell access unless from admin containers
      condition: >
        spawned_process
        and shell_procs
        and not allowed_admin_containers
      output: >
        Shell spawned in non-admin container
        user=%user.name shell=%proc.name pod=%k8s.pod.name
      priority: NOTICE
      tags: [shell, compliance]
```

### 3.2 加载自定义规则

```yaml
# 在 values 中挂载
falco:
  rules_file:
    - /etc/falco/falco_rules.yaml
    - /etc/falco/falco_rules.local.yaml
    - /etc/falco/rules.d/custom_rules.yaml  # 自定义

extra:
  volumeMounts:
    - mountPath: /etc/falco/rules.d/custom_rules.yaml
      name: custom-rules
      subPath: custom_rules.yaml
  volumes:
    - name: custom-rules
      configMap:
        name: falco-custom-rules
```

---

## 四、Falco Sidekick 响应

### 4.1 多输出配置

```yaml
# values-falcosidekick.yaml
config:
  slack:
    webhookurl: "https://hooks.slack.com/services/XXX"
    minimumpriority: "critical"
    messageformat: "Falco Alert: {{ .Output }}"

  teams:
    webhookurl: "https://outlook.office.com/webhook/..."
    minimumpriority: "critical"

  pagerduty:
    routingkey: "<integration-key>"
    minimumpriority: "critical"

  aws:
    accesskeyid: ""
    secretaccesskey: ""
    region: "us-east-1"
    sqs:
      url: "https://sqs.us-east-1.amazonaws.com/123456789/falco-alerts"
      minimumpriority: "warning"

  webhook:
    address: "https://security-automation.example.com/falco"
    minimumpriority: "notice"
    checkcert: true

  loki:
    hostport: "http://loki.monitoring:3100"
    minimumpriority: ""

  elasticsearch:
    hostport: "https://es.example.com:9200"
    index: "falco-logs"
    type: "event"
    minimumpriority: ""
```

---

## 五、与 Prometheus/Grafana 集成

### 5.1 ServiceMonitor

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: falco-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: falco
  namespaceSelector:
    matchNames:
      - falco
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
```

### 5.2 Grafana Dashboard

- 官方 Dashboard ID: `11914` (Falco Overview)
- 导入: `grafana.com/dashboards/11914`

### 5.3 关键指标

| 指标 | 说明 |
|:---|:---|
| `falcosecurity_scap_events_total` | 捕获的系统调用总数 |
| `falcosecurity_falco_events_total` | 触发的规则事件数 |
| `falcosecurity_falco_drops_total` | 丢弃的事件数 (性能瓶颈) |

---

## 六、与 K8s 审计日志集成

```yaml
# values 中启用 K8s 审计
falco:
  webserver:
    enabled: true
    listen_port: 8765
    k8s_audit_endpoint: /k8s-audit

# K8s API Server 配置
# /etc/kubernetes/manifests/kube-apiserver.yaml
# --audit-policy-file=/etc/kubernetes/audit-policy.yaml
# --audit-webhook-config-file=/etc/kubernetes/audit-webhook.yaml

# audit-webhook.yaml
apiVersion: v1
kind: Config
clusters:
- name: falco
  cluster:
    server: http://falco.falco.svc.cluster.local:8765/k8s-audit
contexts:
- context:
    cluster: falco
    user: ""
  name: default-context
current-context: default-context
preferences: {}
users: []
```

---

## 七、性能调优

| 参数 | 默认值 | 调优建议 |
|:---|:---|:---|
| `FALCO_BUFSIZE` | 8MB | 高负载节点 32MB+ |
| `FALCO_MAX_READS_PER_CPU` | 无 | 限制单 CPU 读取速率 |
| `syscall_buf_size_preset` | 4 | 高负载调至 6-8 |
| eBPF vs Kernel Module | 自动选择 | 新内核优先 eBPF |
| `grpc.threadiness` | 2 | 高并发调高 |

### 7.2 减少误报

```yaml
# 使用白名单减少噪音
- list: trusted_images
  items: [gcr.io/company/base, registry.example.com/common]

- macro: trusted_container
  condition: (container.image.repository in (trusted_images))

# 在规则中排除
condition: >
  spawned_process
  and not trusted_container
```

---

## 八、常见问题

| 问题 | 原因 | 解决 |
|:---|:---|:---|
| Falco 无法启动 | 内核不支持 | 检查 `falco-driver-loader` 日志，尝试 eBPF |
| 事件丢失 (drops) | 缓冲区不足 | 增大 `FALCO_BUFSIZE`，降低规则复杂度 |
| 误报过多 | 规则过于宽泛 | 增加白名单，细化规则条件 |
| Sidekick 未收到事件 | 网络不通 / URL 错误 | 测试连通性，检查 Service DNS |
| 无 K8s 元数据 | 未启用 collectors | 启用 containerd/docker 收集器 |

---

## 参考链接

- [Falco 官方文档](https://falco.org/docs/)
- [Falco Sidekick 文档](https://github.com/falcosecurity/falcosidekick)
- [Falco Rules 指南](https://falco.org/docs/rules/)
- [eBPF 模式说明](https://falco.org/docs/event-sources/kernel/#ebpf-probe)
