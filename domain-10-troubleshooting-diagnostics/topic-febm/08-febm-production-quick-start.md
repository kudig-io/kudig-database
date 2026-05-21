---
title: 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册
description: 'description: ''**目标读者**：需要在现有 Kubernetes 集群中快速落地 FEBM 方法论的 SRE 和安全团队'''
category: febm
tags:
- febm
- troubleshooting
- production
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 120min
intent_queries:
- 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册 是什么
- 如何 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册 故障排查
- 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册 排障步骤
trigger_keywords:
- 第八章：FEBM
- 生产环境快速启动与
- Kubernetes
- 故障取证手册
- troubleshooting
- diagnostics
- febm
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cni-basics
- etcd-basics
- redis-basics
- tls-basics
- policy-basics
- logging-basics
- tracing-basics
---

title: 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册
description: '**目标读者**：需要在现有 Kubernetes 集群中快速落地 FEBM 方法论的 SRE 和安全团队'
category: febm
tags:
- k8s
- forensics
- evidence-based
- methodology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 5min
intent_queries:
- 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册 是什么
- 如何 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册
trigger_keywords:
- 第八章：FEBM
- 生产环境快速启动与
- Kubernetes
- 故障取证手册
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

# 第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册

> **目标读者**：需要在现有 Kubernetes 集群中快速落地 FEBM 方法论的 SRE 和安全团队  
> **交付成果**：7 天内建立可运行的 FEBM 取证能力 + 6 个常见故障场景的标准化 Runbook

```
┌─────────────────────────────────────────────────────────────────┐
│              FEBM 生产环境快速启动路线图                         │
├─────────────────────────────────────────────────────────────────┤
│  Day 1-2: 证据采集层 (Falco + K8s Audit)                        │
│  Day 3-4: 证据存储层 (Loki + Prometheus)                        │
│  Day 5-6: 证据关联层 (Grafana + Alerting)                       │
│  Day 7:   取证验证 (模拟故障验证)                                │
└─────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 8.1 FEBM 第一周行动清单 -->## 8.1 FEBM 第一周行动清单

#<!-- chunk: 8.1.1 Day 1: 部署 Falco (运行时安全监控) -->## 8.1.1 Day 1: 部署 Falco (运行时安全监控)

**目标**：为所有节点部署 Falco DaemonSet，开始采集系统调用级证据。

##<!-- chunk: 步骤 1: 部署 Falco -->## 步骤 1: 部署 Falco

```bash
# 添加 Falco Helm repo
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm repo update

# 创建 namespace
kubectl create namespace falco

# 部署 Falco (使用默认规则)
helm install falco falcosecurity/falco \
  --namespace falco \
  --set falcosidekick.enabled=false \
  --set tty=true \
  --set json_output=true \
  --set json_include_output_property=true \
  --set log_level=info
```

##<!-- chunk: 步骤 2: 验证部署 -->## 步骤 2: 验证部署

```bash
# 检查 Falco pods 运行状态
kubectl get pods -n falco -o wide

# 预期输出：每个节点一个 falco-xxx pod，状态 Running
# NAME          READY   STATUS    NODE
# falco-abc12   1/1     Running   node-1
# falco-def34   1/1     Running   node-2

# 查看 Falco 日志
kubectl logs -n falco -l app.kubernetes.io/name=falco --tail=50

# 应该看到类似输出：
# {"output":"Notice A shell was spawned in a container...","priority":"Notice",...}
```

##<!-- chunk: 步骤 3: 触发测试事件 -->## 步骤 3: 触发测试事件

```bash
# 在测试 pod 中执行 shell（应触发 Falco 规则）
kubectl run test-pod --image=nginx --rm -it -- /bin/bash

# 在另一个终端查看 Falco 告警
kubectl logs -n falco -l app.kubernetes.io/name=falco --tail=5 | grep "Notice A shell"
```

**成功标准**：
- ✅ 所有节点的 Falco pod 状态为 Running
- ✅ 能在日志中看到 JSON 格式的告警
- ✅ 测试 shell 触发 "Terminal shell in container" 告警

**资源开销**：~200MB 内存/节点，0.1 CPU/节点

---

#<!-- chunk: 8.1.2 Day 2: 启用 Kubernetes 审计日志 -->## 8.1.2 Day 2: 启用 Kubernetes 审计日志

**目标**：配置 API Server 审计日志到 RequestResponse 级别，记录所有 API 操作证据。

##<!-- chunk: 步骤 1: 准备审计策略文件 -->## 步骤 1: 准备审计策略文件

```bash
# 创建审计策略文件 /etc/kubernetes/audit-policy.yaml
cat <<'EOF' | sudo tee /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 记录 Secret/ConfigMap/Token 的元数据（不记录 data 字段）
  - level: Metadata
    resources:
    - group: ""
      resources: ["secrets", "configmaps", "serviceaccounts/token"]
  
  # 记录关键资源的请求和响应
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete", "deletecollection"]
    resources:
    - group: ""
      resources: ["pods", "services", "persistentvolumeclaims"]
    - group: "apps"
      resources: ["deployments", "daemonsets", "statefulsets"]
    - group: "batch"
      resources: ["jobs", "cronjobs"]
  
  # 记录认证和授权事件
  - level: Metadata
    omitStages: ["RequestReceived"]
    userGroups: ["system:unauthenticated"]
  
  # 其他 API 调用记录元数据
  - level: Metadata
    omitStages: ["RequestReceived"]
EOF
```

##<!-- chunk: 步骤 2: 配置 API Server -->## 步骤 2: 配置 API Server

```bash
# 编辑 API Server manifest (kubeadm 部署)
sudo vim /etc/kubernetes/manifests/kube-apiserver.yaml

# 添加以下参数到 spec.containers[0].command:
# - --audit-policy-file=/etc/kubernetes/audit-policy.yaml
# - --audit-log-path=/var/log/kubernetes/audit.log
# - --audit-log-maxage=30
# - --audit-log-maxbackup=10
# - --audit-log-maxsize=100

# 添加 volume 和 volumeMount:
# volumes:
# - name: audit-policy
#   hostPath:
#     path: /etc/kubernetes/audit-policy.yaml
#     type: File
# - name: audit-log
#   hostPath:
#     path: /var/log/kubernetes
#     type: DirectoryOrCreate
#
# volumeMounts:
# - name: audit-policy
#   mountPath: /etc/kubernetes/audit-policy.yaml
#   readOnly: true
# - name: audit-log
#   mountPath: /var/log/kubernetes
```

##<!-- chunk: 步骤 3: 验证审计日志 -->## 步骤 3: 验证审计日志

```bash
# 等待 API Server 重启（约 30 秒）
watch kubectl get pods -n kube-system | grep kube-apiserver

# 检查审计日志文件
sudo ls -lh /var/log/kubernetes/audit.log

# 查看最近的审计事件
sudo tail -f /var/log/kubernetes/audit.log | jq .

# 触发测试事件
kubectl run audit-test --image=nginx --rm
kubectl delete pod audit-test

# 在审计日志中搜索该事件
sudo grep "audit-test" /var/log/kubernetes/audit.log | jq '.verb,.objectRef.resource'
```

**成功标准**：
- ✅ audit.log 文件存在且持续增长
- ✅ 可以看到 JSON 格式的审计事件
- ✅ 测试 pod 的 create/delete 操作被记录

**资源开销**：~500MB 磁盘/天（取决于集群活跃度），~5% API Server CPU 增加

---

#<!-- chunk: 8.1.3 Day 3: 部署日志聚合 (Fluent Bit → Loki) -->## 8.1.3 Day 3: 部署日志聚合 (Fluent Bit → Loki)

**目标**：将分散的日志（Falco、K8s audit、容器日志）统一存储到 Loki。

##<!-- chunk: 步骤 1: 部署 Loki -->## 步骤 1: 部署 Loki

```bash
# 添加 Grafana Helm repo
helm repo add grafana https://grafana.github.io/helm-charts
helm repo update

# 创建 namespace
kubectl create namespace logging

# 部署 Loki (单体模式，适合中小规模)
helm install loki grafana/loki \
  --namespace logging \
  --set loki.auth_enabled=false \
  --set loki.commonConfig.replication_factor=1 \
  --set loki.storage.type=filesystem \
  --set singleBinary.replicas=1 \
  --set singleBinary.persistence.enabled=true \
  --set singleBinary.persistence.size=50Gi
```

##<!-- chunk: 步骤 2: 部署 Fluent Bit -->## 步骤 2: 部署 Fluent Bit

```bash
# 创建 Fluent Bit 配置 ConfigMap
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush        5
        Daemon       Off
        Log_Level    info
        Parsers_File parsers.conf

    [INPUT]
        Name              tail
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            docker
        DB                /var/log/flb_kube.db
        Mem_Buf_Limit     5MB
        Skip_Long_Lines   On
        Refresh_Interval  10

    [INPUT]
        Name              tail
        Tag               audit.*
        Path              /var/log/kubernetes/audit.log
        Parser            json
        DB                /var/log/flb_audit.db

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        Merge_Log           On
        K8S-Logging.Parser  On
        K8S-Logging.Exclude On

    [OUTPUT]
        Name   loki
        Match  *
        Host   loki.logging.svc.cluster.local
        Port   3100
        Labels job=fluentbit
        Auto_Kubernetes_Labels on

  parsers.conf: |
    [PARSER]
        Name   docker
        Format json
        Time_Key time
        Time_Format %Y-%m-%dT%H:%M:%S.%L%z
        Time_Keep On
EOF

# 部署 Fluent Bit DaemonSet
helm install fluent-bit grafana/fluent-bit \
  --namespace logging \
  --set config.customParsers="$(kubectl get cm -n logging fluent-bit-config -o jsonpath='{.data.parsers\.conf}')" \
  --set config.outputs="$(kubectl get cm -n logging fluent-bit-config -o jsonpath='{.data.fluent-bit\.conf}' | grep -A 10 '\[OUTPUT\]')"
```

##<!-- chunk: 步骤 3: 验证日志流 -->## 步骤 3: 验证日志流

```bash
# 检查 Fluent Bit pods
kubectl get pods -n logging -l app.kubernetes.io/name=fluent-bit

# 查看 Fluent Bit 日志（检查是否有错误）
kubectl logs -n logging -l app.kubernetes.io/name=fluent-bit --tail=20

# 使用 LogCLI 查询日志（需要先安装 logcli）
kubectl port-forward -n logging svc/loki 3100:3100 &

# 安装 logcli
# macOS: brew install logcli
# Linux: wget https://github.com/grafana/loki/releases/download/v2.9.0/logcli-linux-amd64.zip

# 查询最近的日志
logcli query '{job="fluentbit"}' --since=5m --limit=10
```

**成功标准**：
- ✅ Loki 和 Fluent Bit pods 状态为 Running
- ✅ logcli 可以查询到容器日志和审计日志
- ✅ 日志包含 Kubernetes 元数据（namespace、pod name 等）

**资源开销**：
- Loki: ~1GB 内存，50GB 磁盘（可配置保留期）
- Fluent Bit: ~100MB 内存/节点，0.05 CPU/节点

---

#<!-- chunk: 8.1.4 Day 4: 部署 Prometheus + Grafana -->## 8.1.4 Day 4: 部署 Prometheus + Grafana

**目标**：建立指标采集和可视化能力，支持 FEBM 跨层证据关联。

##<!-- chunk: 步骤 1: 部署 kube-prometheus-stack -->## 步骤 1: 部署 kube-prometheus-stack

```bash
# 部署完整的监控栈（Prometheus + Grafana + Alertmanager + Node Exporter）
helm install kube-prometheus-stack prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --set prometheus.prometheusSpec.retention=15d \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
  --set grafana.adminPassword=admin \
  --set grafana.persistence.enabled=true \
  --set grafana.persistence.size=10Gi
```

##<!-- chunk: 步骤 2: 配置 Grafana 数据源 -->## 步骤 2: 配置 Grafana 数据源

```bash
# 暴露 Grafana 服务
kubectl port-forward -n monitoring svc/kube-prometheus-stack-grafana 3000:80 &

# 访问 Grafana: http://localhost:3000
# 用户名: admin, 密码: admin

# 添加 Loki 数据源（通过 UI 或 API）
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasource-loki
  namespace: monitoring
  labels:
    grafana_datasource: "1"
data:
  loki.yaml: |
    apiVersion: 1
    datasources:
    - name: Loki
      type: loki
      access: proxy
      url: http://loki.logging.svc.cluster.local:3100
      isDefault: false
EOF

# 重启 Grafana 以加载新数据源
kubectl rollout restart -n monitoring deployment/kube-prometheus-stack-grafana
```

##<!-- chunk: 步骤 3: 导入 FEBM 仪表板 -->## 步骤 3: 导入 FEBM 仪表板

```bash
# 创建 FEBM 基础仪表板 ConfigMap
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: febm-dashboard
  namespace: monitoring
  labels:
    grafana_dashboard: "1"
data:
  febm-overview.json: |
    {
      "dashboard": {
        "title": "FEBM 证据概览",
        "panels": [
          {
            "title": "Falco 告警趋势",
            "targets": [{
              "expr": "sum(rate(falco_events_total[5m])) by (priority)",
              "legendFormat": "{{priority}}"
            }],
            "type": "graph"
          },
          {
            "title": "K8s API 审计事件量",
            "targets": [{
              "expr": "count_over_time({job=\"fluentbit\", tag=\"audit.*\"}[5m])",
              "refId": "A"
            }],
            "type": "graph"
          },
          {
            "title": "容器重启次数 Top 10",
            "targets": [{
              "expr": "topk(10, kube_pod_container_status_restarts_total)",
              "legendFormat": "{{namespace}}/{{pod}}"
            }],
            "type": "table"
          }
        ],
        "refresh": "30s",
        "time": {"from": "now-1h", "to": "now"}
      }
    }
EOF
```

##<!-- chunk: 步骤 4: 验证监控栈 -->## 步骤 4: 验证监控栈

```bash
# 检查所有组件状态
kubectl get pods -n monitoring

# 预期输出：
# prometheus-kube-prometheus-stack-prometheus-0   2/2   Running
# kube-prometheus-stack-grafana-xxx               3/3   Running
# kube-prometheus-stack-operator-xxx              1/1   Running

# 测试 Prometheus 查询
kubectl port-forward -n monitoring svc/kube-prometheus-stack-prometheus 9090:9090 &
curl http://localhost:9090/api/v1/query?query=up | jq '.data.result | length'

# 应该返回 > 0 的数字（表示有监控目标）
```

**成功标准**：
- ✅ Prometheus、Grafana、Alertmanager pods 运行正常
- ✅ Grafana 可以访问且显示 Prometheus 和 Loki 数据源
- ✅ FEBM 仪表板显示 Falco 和审计日志指标

**资源开销**：
- Prometheus: ~2GB 内存，100GB 磁盘
- Grafana: ~500MB 内存，10GB 磁盘

---

#<!-- chunk: 8.1.5 Day 5: 配置 Falcosidekick (告警路由) -->## 8.1.5 Day 5: 配置 Falcosidekick (告警路由)

**目标**：将 Falco 告警实时路由到 Slack/Webhook，实现快速响应。

##<!-- chunk: 步骤 1: 升级 Falco 并启用 Falcosidekick -->## 步骤 1: 升级 Falco 并启用 Falcosidekick

```bash
# 升级 Falco 并启用 Falcosidekick
helm upgrade falco falcosecurity/falco \
  --namespace falco \
  --reuse-values \
  --set falcosidekick.enabled=true \
  --set falcosidekick.webui.enabled=true \
  --set falcosidekick.config.slack.webhookurl="YOUR_SLACK_WEBHOOK_URL" \
  --set falcosidekick.config.slack.minimumpriority=warning
```

##<!-- chunk: 步骤 2: 配置告警路由规则 -->## 步骤 2: 配置告警路由规则

```bash
# 创建高级路由配置
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: falcosidekick-config
  namespace: falco
data:
  config.yaml: |
    slack:
      webhookurl: "https://hooks.slack.com/services/YOUR/WEBHOOK/URL"
      minimumpriority: "warning"
      outputformat: "text"
      messageformat: |
        :warning: *Falco Alert*
        *Priority*: {{.Priority}}
        *Rule*: {{.Rule}}
        *Output*: {{.Output}}
        *Time*: {{.Time}}
        *Pod*: {{.OutputFields.k8s.pod.name}}
        *Namespace*: {{.OutputFields.k8s.ns.name}}
    
    webhook:
      address: "https://your-incident-system.com/api/events"
      minimumpriority: "critical"
      customHeaders:
        Authorization: "Bearer YOUR_API_TOKEN"
    
    loki:
      hostport: "http://loki.logging.svc.cluster.local:3100"
      minimumpriority: "debug"
EOF

# 重启 Falcosidekick 加载新配置
kubectl rollout restart -n falco deployment/falco-falcosidekick
```

##<!-- chunk: 步骤 3: 测试告警流 -->## 步骤 3: 测试告警流

```bash
# 触发高优先级告警（修改 /etc/passwd）
kubectl run alert-test --image=nginx --rm -it -- sh -c "echo test >> /etc/passwd"

# 应该在 30 秒内收到 Slack 消息：
# ⚠️ Falco Alert
# Priority: Error
# Rule: Write below binary dir
# Output: File below /etc opened for writing...

# 检查 Falcosidekick 日志
kubectl logs -n falco -l app.kubernetes.io/name=falcosidekick --tail=20

# 检查 Loki 中的 Falco 事件
logcli query '{app="falco"}' --since=10m | grep "Write below binary dir"
```

##<!-- chunk: 步骤 4: 配置告警静默规则（可选） -->## 步骤 4: 配置告警静默规则（可选）

```bash
# 为已知的合规操作配置静默
cat <<'EOF' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: falcosidekick-rules
  namespace: falco
data:
  rules.yaml: |
    - rule: Suppress kubectl exec from CI
      condition: >
        k8s.ns.name = "ci-system" and
        proc.name = "kubectl" and
        ka.verb = "create" and
        ka.uri.param contains "exec"
      action: suppress
      priority: info
EOF
```

**成功标准**：
- ✅ Falcosidekick pod 运行正常
- ✅ 测试告警成功路由到 Slack
- ✅ 高优先级告警发送到 webhook
- ✅ 所有告警存储到 Loki

**资源开销**：~200MB 内存，0.1 CPU

---

#<!-- chunk: 8.1.6 Day 6-7: 集成验证和调优 -->## 8.1.6 Day 6-7: 集成验证和调优

##<!-- chunk: Day 6: 端到端测试 -->## Day 6: 端到端测试

```bash
#!/bin/bash
# febm-e2e-test.sh - FEBM 端到端验证脚本

set -e

echo "=== FEBM E2E 测试开始 ==="

# 1. 模拟可疑行为
echo "[1/5] 触发测试事件：容器内执行 shell"
kubectl run e2e-test --image=alpine --rm -it -- sh -c "
  echo 'Simulating suspicious activity'
  nc -l -p 8080 &  # 触发 'Netcat Remote Code Execution' 规则
  sleep 10
"

# 2. 验证 Falco 检测
echo "[2/5] 验证 Falco 检测"
sleep 5
kubectl logs -n falco -l app.kubernetes.io/name=falco --tail=50 | \
  grep -q "Notice A shell was spawned" && \
  echo "✅ Falco 检测成功" || \
  echo "❌ Falco 未检测到事件"

# 3. 验证审计日志记录
echo "[3/5] 验证 K8s 审计日志"
sudo grep "e2e-test" /var/log/kubernetes/audit.log | \
  jq -r '.verb' | grep -q "create" && \
  echo "✅ 审计日志记录成功" || \
  echo "❌ 审计日志未记录"

# 4. 验证 Loki 日志聚合
echo "[4/5] 验证 Loki 日志聚合"
sleep 10
logcli query '{app="falco"}' --since=2m | grep -q "e2e-test" && \
  echo "✅ Loki 聚合成功" || \
  echo "❌ Loki 未找到日志"

# 5. 验证告警路由
echo "[5/5] 验证告警路由（检查 Slack）"
kubectl logs -n falco -l app.kubernetes.io/name=falcosidekick --tail=20 | \
  grep -q "post to slack ok" && \
  echo "✅ Slack 告警发送成功" || \
  echo "❌ Slack 告警未发送"

echo "=== FEBM E2E 测试完成 ==="
```

##<!-- chunk: Day 7: 性能调优检查清单 -->## Day 7: 性能调优检查清单

```yaml
# FEBM 性能调优检查清单

Falco 调优:
  - [ ] 禁用不需要的规则（编辑 values.yaml 的 falco.rules_file）
  - [ ] 调整缓冲区大小（适应高负载节点）
      # --set driver.kind=modern_ebpf  # eBPF 性能优于内核模块
  - [ ] 配置 CPU/内存限制
      # --set resources.limits.cpu=200m
      # --set resources.limits.memory=512Mi

Fluent Bit 调优:
  - [ ] 调整 Flush 间隔（降低延迟）
      # Flush 1  # 秒，默认 5
  - [ ] 配置 Buffer 限制（防止 OOM）
      # Mem_Buf_Limit 10MB
  - [ ] 启用多进程（高吞吐场景）
      # Workers 2

Loki 调优:
  - [ ] 配置日志保留期（平衡成本和合规）
      # table_manager.retention_period: 30d
  - [ ] 启用压缩（减少存储成本）
      # chunk_encoding: snappy
  - [ ] 配置索引缓存（加速查询）
      # query_range.cache_results: true

Prometheus 调优:
  - [ ] 调整抓取间隔（降低开销）
      # scrape_interval: 30s  # 默认 15s
  - [ ] 配置指标保留期
      # --set prometheus.prometheusSpec.retention=15d
  - [ ] 启用远程存储（长期保存）
      # remote_write:
      #   - url: https://your-long-term-storage
```

**第一周总结**：

```
┌─────────────────────────────────────────────────────────────────┐
│                     FEBM 工具栈部署完成                          │
├─────────────────────────────────────────────────────────────────┤
│ ✅ 证据采集层: Falco (运行时) + K8s Audit (API)                 │
│ ✅ 证据存储层: Loki (日志) + Prometheus (指标)                  │
│ ✅ 证据关联层: Grafana (可视化) + Falcosidekick (告警)          │
│ ✅ 端到端验证: 模拟故障 → 证据采集 → 告警触发                   │
├─────────────────────────────────────────────────────────────────┤
│ 总资源开销:                                                      │
│   - 内存: ~5GB (集群级)                                          │
│   - 存储: ~200GB (可配置保留期)                                  │
│   - CPU: ~1 Core (集群级)                                        │
├─────────────────────────────────────────────────────────────────┤
│ 下一步:                                                          │
│   1. 导入预定义的故障取证 Runbook (第 8.3 节)                   │
│   2. 配置 FEBM KPI 仪表板 (第 8.5 节)                            │
│   3. 进行首次真实故障取证演练                                    │
└─────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 8.2 最小化 FEBM 工具栈部署 -->## 8.2 最小化 FEBM 工具栈部署

#<!-- chunk: 8.2.1 一键部署脚本 -->## 8.2.1 一键部署脚本

```bash
#!/bin/bash
# deploy-febm-minimal.sh - FEBM 最小化工具栈一键部署脚本

set -e

echo "=== 开始部署 FEBM 最小化工具栈 ==="

# 前置检查
echo "[前置检查] 验证 Kubernetes 集群连接"
kubectl cluster-info || { echo "❌ 无法连接 Kubernetes 集群"; exit 1; }

echo "[前置检查] 验证 Helm 安装"
helm version || { echo "❌ Helm 未安装"; exit 1; }

# 添加 Helm repos
echo "[Helm] 添加必需的 Helm repositories"
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm repo add grafana https://grafana.github.io/helm-charts
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 创建 namespaces
echo "[K8s] 创建 namespaces"
kubectl create namespace falco --dry-run=client -o yaml | kubectl apply -f -
kubectl create namespace logging --dry-run=client -o yaml | kubectl apply -f -
kubectl create namespace monitoring --dry-run=client -o yaml | kubectl apply -f -

# 部署 Falco
echo "[Falco] 部署 Falco + Falcosidekick"
helm upgrade --install falco falcosecurity/falco \
  --namespace falco \
  --set tty=true \
  --set json_output=true \
  --set falcosidekick.enabled=true \
  --set falcosidekick.webui.enabled=true \
  --set resources.limits.memory=512Mi \
  --set resources.limits.cpu=200m \
  --wait --timeout=5m

# 部署 Loki
echo "[Loki] 部署 Loki (单体模式)"
helm upgrade --install loki grafana/loki \
  --namespace logging \
  --set loki.auth_enabled=false \
  --set loki.commonConfig.replication_factor=1 \
  --set singleBinary.replicas=1 \
  --set singleBinary.persistence.size=50Gi \
  --set singleBinary.resources.limits.memory=1Gi \
  --wait --timeout=5m

# 部署 Fluent Bit
echo "[Fluent Bit] 部署 Fluent Bit"
cat <<'FLUENTBIT_CONFIG' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         5
        Log_Level     info
        Parsers_File  parsers.conf

    [INPUT]
        Name              tail
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            docker
        DB                /var/log/flb_kube.db
        Mem_Buf_Limit     5MB

    [INPUT]
        Name              tail
        Tag               audit.*
        Path              /var/log/kubernetes/audit.log
        Parser            json
        DB                /var/log/flb_audit.db

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Merge_Log           On

    [OUTPUT]
        Name   loki
        Match  *
        Host   loki.logging.svc.cluster.local
        Port   3100
        Labels job=fluentbit

  parsers.conf: |
    [PARSER]
        Name   docker
        Format json
        Time_Key time
        Time_Format %Y-%m-%dT%H:%M:%S.%L%z
FLUENTBIT_CONFIG

helm upgrade --install fluent-bit grafana/fluent-bit \
  --namespace logging \
  --set config.existingConfigMap=fluent-bit-config \
  --set resources.limits.memory=100Mi \
  --wait --timeout=5m

# 部署 kube-prometheus-stack
echo "[Prometheus] 部署 kube-prometheus-stack"
helm upgrade --install kube-prometheus-stack prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --set prometheus.prometheusSpec.retention=15d \
  --set prometheus.prometheusSpec.resources.limits.memory=2Gi \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
  --set grafana.adminPassword=admin \
  --set grafana.persistence.enabled=true \
  --set grafana.persistence.size=10Gi \
  --wait --timeout=10m

# 配置 Grafana Loki 数据源
echo "[Grafana] 配置 Loki 数据源"
cat <<'LOKI_DATASOURCE' | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasource-loki
  namespace: monitoring
  labels:
    grafana_datasource: "1"
data:
  loki.yaml: |
    apiVersion: 1
    datasources:
    - name: Loki
      type: loki
      access: proxy
      url: http://loki.logging.svc.cluster.local:3100
      isDefault: false
LOKI_DATASOURCE

kubectl rollout restart -n monitoring deployment/kube-prometheus-stack-grafana

# 验证部署
echo "=== 验证部署状态 ==="
echo "[Falco]"
kubectl get pods -n falco
echo "[Logging]"
kubectl get pods -n logging
echo "[Monitoring]"
kubectl get pods -n monitoring

echo ""
echo "=== FEBM 工具栈部署完成 ==="
echo "访问方式:"
echo "  Grafana:        kubectl port-forward -n monitoring svc/kube-prometheus-stack-grafana 3000:80"
echo "                  http://localhost:3000 (admin/admin)"
echo "  Falco Web UI:   kubectl port-forward -n falco svc/falco-falcosidekick-ui 2802:2802"
echo "                  http://localhost:2802"
echo "  Prometheus:     kubectl port-forward -n monitoring svc/kube-prometheus-stack-prometheus 9090:9090"
echo "                  http://localhost:9090"
```

#<!-- chunk: 8.2.2 资源开销明细 -->## 8.2.2 资源开销明细

```
┌─────────────────────────────────────────────────────────────────┐
│                FEBM 最小化工具栈资源开销                         │
├──────────────────┬──────────┬──────────┬───────────┬────────────┤
│ 组件             │ 内存     │ CPU      │ 存储      │ 备注       │
├──────────────────┼──────────┼──────────┼───────────┼────────────┤
│ Falco (每节点)   │ 200MB    │ 0.1 Core │ -         │ DaemonSet  │
│ Falcosidekick    │ 200MB    │ 0.1 Core │ -         │ 单副本     │
│ Fluent Bit (每节点)│ 100MB  │ 0.05 Core│ -         │ DaemonSet  │
│ Loki             │ 1GB      │ 0.5 Core │ 50GB      │ 单体模式   │
│ Prometheus       │ 2GB      │ 1.0 Core │ 100GB     │ 15天保留   │
│ Grafana          │ 500MB    │ 0.2 Core │ 10GB      │ 单副本     │
│ Alertmanager     │ 100MB    │ 0.05 Core│ 1GB       │ 单副本     │
│ Node Exporter (每节点)│50MB │ 0.05 Core│ -         │ DaemonSet  │
├──────────────────┼──────────┼──────────┼───────────┼────────────┤
│ 总计 (3节点集群) │ ~5GB     │ ~2 Cores │ ~160GB    │            │
└──────────────────┴──────────┴──────────┴───────────┴────────────┘

成本估算（基于 AWS EKS）:
  - 计算: 2 Cores × $0.0416/hour = ~$60/月
  - 存储: 160GB × $0.10/GB/月 = ~$16/月
  - 总成本: ~$76/月 (不含集群基础成本)

优化建议:
  1. 使用 S3 作为 Loki 后端存储，降低存储成本 50%
  2. 配置 Prometheus 远程写入到 Thanos，降低本地存储需求
  3. 生产环境使用对象存储（S3/GCS）+ 分层存储策略
```

#<!-- chunk: 8.2.3 NTP 时钟同步验证 -->## 8.2.3 NTP 时钟同步验证

**为什么 NTP 对 FEBM 至关重要？**

FEBM 需要精确关联来自不同数据源的证据（Falco 告警、审计日志、指标），时钟偏差 > 1 秒会导致证据链断裂。

```bash
#!/bin/bash
# verify-ntp-sync.sh - 验证集群时钟同步

echo "=== 验证 Kubernetes 集群时钟同步 ==="

# 获取所有节点
NODES=$(kubectl get nodes -o jsonpath='{.items[*].metadata.name}')

echo "检查节点时钟同步状态:"
for NODE in $NODES; do
    echo "---"
    echo "节点: $NODE"
    
    # 通过 debug pod 检查 NTP 状态
    kubectl debug node/$NODE -it --image=alpine -- sh -c "
        apk add --no-cache chrony > /dev/null 2>&1
        chronyc tracking
    " 2>/dev/null | grep -E 'Leap status|System time|Last offset|RMS offset'
done

echo ""
echo "检查时钟偏差（所有节点与 API Server 对比）:"

API_TIME=$(kubectl get --raw /healthz | date +%s)

for NODE in $NODES; do
    NODE_TIME=$(kubectl debug node/$NODE -it --image=alpine -- date +%s 2>/dev/null | tr -d '\r')
    SKEW=$((NODE_TIME - API_TIME))
    
    if [ ${SKEW#-} -gt 1 ]; then
        echo "❌ $NODE: 时钟偏差 ${SKEW}s (超过阈值)"
    else
        echo "✅ $NODE: 时钟偏差 ${SKEW}s"
    fi
done

echo ""
echo "推荐配置 (所有节点):"
cat <<'NTP_CONFIG'
# 安装并启用 chrony
apt-get install -y chrony  # Debian/Ubuntu
yum install -y chrony      # RHEL/CentOS

# 配置 NTP 服务器 (/etc/chrony/chrony.conf)
server 0.pool.ntp.org iburst
server 1.pool.ntp.org iburst
server 2.pool.ntp.org iburst
makestep 1 3  # 首次同步允许大幅调整

# 启动服务
systemctl enable --now chronyd

# 验证同步状态
chronyc tracking
NTP_CONFIG
```

---

<!-- chunk: 8.3 Kubernetes 常见故障 FEBM 取证 Runbook -->## 8.3 Kubernetes 常见故障 FEBM 取证 Runbook

#<!-- chunk: 8.3.1 Pod OOMKilled 取证 Runbook -->## 8.3.1 Pod OOMKilled 取证 Runbook

##<!-- chunk: 场景描述 -->## 场景描述

Pod 被 Kubernetes 终止，状态显示 `OOMKilled`（Out of Memory），需要区分根因是内存泄漏、资源配置不当还是内核 Bug。

##<!-- chunk: 证据采集清单 -->## 证据采集清单

```yaml
证据类型及采集命令:

1. 容器状态快照 (Container State Snapshot):
   命令: |
     kubectl describe pod <POD_NAME> -n <NAMESPACE>
     kubectl get pod <POD_NAME> -n <NAMESPACE> -o yaml
   关键字段:
     - status.containerStatuses[].lastState.terminated.reason: OOMKilled
     - spec.containers[].resources.limits.memory: 内存限制值
     - status.containerStatuses[].restartCount: 重启次数

2. Prometheus 指标 (Memory Metrics):
   查询: |
     # 容器内存使用趋势（OOM 前 1 小时）
     container_memory_working_set_bytes{
       pod="<POD_NAME>",
       namespace="<NAMESPACE>"
     }[1h]
     
     # 内存限制值
     kube_pod_container_resource_limits{
       resource="memory",
       pod="<POD_NAME>"
     }
     
     # OOM 终止计数
     kube_pod_container_status_terminated_reason{
       reason="OOMKilled"
     }

3. 应用日志 (Application Logs):
   命令: |
     # 获取 OOM 前的日志（使用 previous 标志）
     kubectl logs <POD_NAME> -n <NAMESPACE> --previous --tail=500
     
     # 从 Loki 获取更长时间范围的日志
     logcli query '{namespace="<NAMESPACE>",pod="<POD_NAME>"}' \
       --since=1h --limit=1000

4. 内核 OOM 日志 (Kernel OOM Killer Logs):
   命令: |
     # 在节点上查看内核日志
     NODE=$(kubectl get pod <POD_NAME> -n <NAMESPACE> -o jsonpath='{.spec.nodeName}')
     
     kubectl debug node/$NODE -it --image=alpine -- sh -c "
       dmesg -T | grep -i 'oom' | tail -50
     "
     
     # 或从 Loki 查询节点日志
     logcli query '{job="systemd-journal",hostname="<NODE>"}' \
       --since=1h | grep -i oom

5. cgroup 内存统计 (cgroup Memory Stats):
   命令: |
     # 通过 eBPF 或 cadvisor 获取详细内存分类
     kubectl exec -it <MONITORING_POD> -- curl -s \
       http://localhost:8080/api/v2.1/stats/<CONTAINER_ID>?type=docker | \
       jq '.memory'

6. K8s 审计日志 (K8s Audit Logs):
   命令: |
     # 检查是否有人修改过内存限制
     sudo grep "<POD_NAME>" /var/log/kubernetes/audit.log | \
       jq 'select(.objectRef.resource=="pods" and 
                  .verb=="patch" and 
                  .requestObject.spec.containers[].resources.limits.memory != null)'
```

##<!-- chunk: 证据分析决策树 -->## 证据分析决策树

```
┌─────────────────────────────────────────────────────────────────┐
│               Pod OOMKilled 根因分析决策树                       │
└─────────────────────────────────────────────────────────────────┘

[Start] Pod 状态 = OOMKilled
   │
   ├─► 检查：内存使用趋势
   │     │
   │     ├─► 趋势：持续线性增长 (Steady Growth)
   │     │    └─► 根因：内存泄漏 (Memory Leak)
   │     │        证据：
   │     │          - container_memory_working_set_bytes 持续上升
   │     │          - 无周期性下降（无 GC 锯齿）
   │     │          - 应用日志中可能有 OutOfMemory 异常
   │     │        处置：
   │     │          1. 使用 heap dump 工具分析（Java: jmap，Go: pprof）
   │     │          2. 修复应用代码
   │     │          3. 临时增大内存限制（治标）
   │     │
   │     ├─► 趋势：突发尖峰 (Sudden Spike)
   │     │    └─► 根因：流量突增或批量任务
   │     │        证据：
   │     │          - 指标显示短时间内内存从低位直接飙升
   │     │          - 日志中有大量请求或数据处理记录
   │     │        处置：
   │     │          1. 增大内存限制以容纳峰值
   │     │          2. 实现请求限流或批量任务分批
   │     │          3. 启用 HPA (Horizontal Pod Autoscaler)
   │     │
   │     └─► 趋势：稳定后突然终止 (Stable then Killed)
   │          └─► 根因：内存限制配置过低
   │              证据：
   │                - 内存使用稳定在接近限制值的水平
   │                - 无异常增长趋势
   │                - 应用日志正常
   │              处置：
   │                1. 根据实际使用量调整 limits（建议预留 20% buffer）
   │                2. 设置 requests = 实际使用量，limits = 1.5x requests
   │
   ├─► 检查：节点内存压力
   │     │
   │     ├─► 节点整体内存 > 90% (Node Pressure)
   │     │    └─► 根因：节点资源不足导致内核 OOM Killer 介入
   │     │        证据：
   │     │          - dmesg 中有 "Out of memory: Kill process"
   │     │          - 多个 pod 同时 OOMKilled
   │     │          - kube_node_status_condition{condition="MemoryPressure"} = 1
   │     │        处置：
   │     │          1. 增加节点内存或节点数量
   │     │          2. 驱逐低优先级 pod
   │     │          3. 审查 pod 资源 requests 设置是否过低
   │     │
   │     └─► 节点内存 < 90%
   │          └─► 继续检查 cgroup 层级
   │
   └─► 检查：异常外部因素
         │
         ├─► 审计日志显示最近有资源限制被修改
         │    └─► 根因：误操作降低了内存限制
         │        证据：
         │          - audit.log 中有 patch pod 操作
         │          - verb=patch, objectRef.resource=pods
         │          - requestObject 中 memory limits 被调低
         │        处置：
         │          1. 恢复原内存限制值
         │          2. 加强 RBAC 权限控制
         │          3. 使用 GitOps 防止手动修改
         │
         └─► cgroup 统计显示异常大的 cache 或 rss
              └─► 根因：可能是内核 Bug 或 cgroup 泄漏
                  证据：
                    - memory.stat 中的 cache 或 inactive_file 异常大
                    - 内核版本较旧 (< 4.19)
                  处置：
                    1. 升级内核版本
                    2. 调整 cgroup 参数
                    3. 咨询内核专家
```

##<!-- chunk: 自动化取证脚本 -->## 自动化取证脚本

```bash
#!/bin/bash
# oom-forensics.sh - Pod OOMKilled 自动化取证脚本

set -e

POD_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$POD_NAME" ]; then
    echo "用法: $0 <POD_NAME> [NAMESPACE]"
    exit 1
fi

echo "=== Pod OOMKilled 取证报告 ==="
echo "Pod: $POD_NAME"
echo "Namespace: $NAMESPACE"
echo "取证时间: $(date)"
echo ""

# 1. 基础信息
echo "<!-- chunk: 1. 基础信息" -->## 1. 基础信息"
echo "---"
kubectl get pod $POD_NAME -n $NAMESPACE -o json | jq '{
  nodeName: .spec.nodeName,
  startTime: .status.startTime,
  containerStatuses: .status.containerStatuses[] | {
    name: .name,
    restartCount: .restartCount,
    lastState: .lastState,
    ready: .ready
  },
  resources: .spec.containers[] | {
    name: .name,
    requests: .resources.requests,
    limits: .resources.limits
  }
}'
echo ""

# 2. Prometheus 内存指标
echo "<!-- chunk: 2. 内存使用趋势（OOM 前 1 小时）" -->## 2. 内存使用趋势（OOM 前 1 小时）"
echo "---"
PROM_URL="http://localhost:9090"  # 需要 port-forward
QUERY="container_memory_working_set_bytes{pod=\"$POD_NAME\",namespace=\"$NAMESPACE\"}[1h]"

curl -s -G "$PROM_URL/api/v1/query" --data-urlencode "query=$QUERY" | \
  jq -r '.data.result[] | .metric.container, .values[] | @tsv' | \
  awk '{print strftime("%Y-%m-%d %H:%M:%S", $1), $2/1024/1024 " MB"}'
echo ""

# 3. 应用日志
echo "<!-- chunk: 3. 应用日志（OOM 前 50 行）" -->## 3. 应用日志（OOM 前 50 行）"
echo "---"
kubectl logs $POD_NAME -n $NAMESPACE --previous --tail=50 2>/dev/null || \
  echo "无法获取 previous 日志（容器可能未重启）"
echo ""

# 4. 内核 OOM 日志
echo "<!-- chunk: 4. 节点内核 OOM 日志" -->## 4. 节点内核 OOM 日志"
echo "---"
NODE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeName}')
echo "节点: $NODE"

kubectl debug node/$NODE -it --image=alpine -- sh -c "
  dmesg -T | grep -i 'oom.*$POD_NAME' | tail -20
" 2>/dev/null || echo "无法访问节点（需要权限）"
echo ""

# 5. 节点内存压力
echo "<!-- chunk: 5. 节点内存状态" -->## 5. 节点内存状态"
echo "---"
kubectl top node $NODE 2>/dev/null || echo "metrics-server 未安装"

QUERY2="node_memory_MemAvailable_bytes{instance=~\"$NODE.*\"} / node_memory_MemTotal_bytes{instance=~\"$NODE.*\"}"
curl -s -G "$PROM_URL/api/v1/query" --data-urlencode "query=$QUERY2" | \
  jq -r '.data.result[0].value[1] | tonumber | . * 100 | "可用内存: \(.)%"'
echo ""

# 6. 审计日志（检查最近的资源修改）
echo "<!-- chunk: 6. 最近的资源配置修改" -->## 6. 最近的资源配置修改"
echo "---"
sudo grep "$POD_NAME" /var/log/kubernetes/audit.log 2>/dev/null | \
  jq -c 'select(.verb=="patch" and .objectRef.resource=="pods") | 
         {time: .requestReceivedTimestamp, user: .user.username, verb: .verb}' | \
  tail -5 || echo "无法访问审计日志或无修改记录"
echo ""

# 7. 根因初步判断
echo "<!-- chunk: 7. 根因初步判断" -->## 7. 根因初步判断"
echo "---"
LIMIT=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.containers[0].resources.limits.memory}')
echo "配置的内存限制: $LIMIT"

# 简单的根因提示
if kubectl logs $POD_NAME -n $NAMESPACE --previous --tail=100 2>/dev/null | grep -qi "outofmemory"; then
    echo "⚠️  应用日志中发现 OutOfMemory 异常 → 可能是内存泄漏"
fi

if [ $(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].restartCount}') -gt 3 ]; then
    echo "⚠️  重启次数 > 3 → 建议使用 heap profiler 分析内存"
fi

echo ""
echo "=== 取证报告结束 ==="
echo "下一步: 根据决策树进行详细分析"
```

##<!-- chunk: 处置建议模板 -->## 处置建议模板

```markdown
# OOMKilled 处置方案

<!-- chunk: 场景 A: 内存泄漏 -->## 场景 A: 内存泄漏

**症状**：
- 内存使用持续线性增长
- 无周期性 GC 回收
- 应用日志中有 OOM 异常

**短期处置**：
1. 临时增大内存限制（2x 当前限制）
   ```yaml
   resources:
     limits:
       memory: "4Gi"  # 从 2Gi 增大
   ```
2. 增加监控告警（内存增长率 > 10%/小时）

**长期处置**：
1. 开启应用 heap profiling（Java: `-XX:+HeapDumpOnOutOfMemoryError`）
2. 使用 MAT/pprof 分析 dump 文件
3. 修复内存泄漏代码
4. 添加单元测试覆盖泄漏场景

---

<!-- chunk: 场景 B: 资源配置不当 -->## 场景 B: 资源配置不当

**症状**：
- 内存使用稳定但接近限制值
- 应用日志正常
- 无异常流量

**处置方案**：
1. 调整资源配置
   ```yaml
   resources:
     requests:
       memory: "1.5Gi"  # 实际稳定使用量
     limits:
       memory: "2Gi"    # 预留 33% buffer
   ```
2. 设置 QoS 为 Burstable（requests < limits）
3. 监控实际使用量，持续优化

---

<!-- chunk: 场景 C: 节点资源不足 -->## 场景 C: 节点资源不足

**症状**：
- 节点内存 > 90%
- 多个 pod 同时 OOMKilled
- dmesg 中有内核 OOM Killer 日志

**处置方案**：
1. 紧急扩容节点或增加节点内存
2. 驱逐低优先级 pod
   ```bash
   kubectl drain <NODE> --ignore-daemonsets --delete-emptydir-data
   ```
3. 审查所有 pod 的 resource requests
4. 启用 Cluster Autoscaler

---

<!-- chunk: 通用最佳实践 -->## 通用最佳实践

1. **总是设置 memory requests 和 limits**
   ```yaml
   # 推荐配置
   resources:
     requests:
       memory: "实际平均使用量"
     limits:
       memory: "实际峰值使用量 × 1.2"
   ```

2. **启用 Vertical Pod Autoscaler (VPA)** 自动推荐资源配置
   ```bash
   kubectl apply -f https://github.com/kubernetes/autoscaler/releases/download/vertical-pod-autoscaler-0.13.0/vpa-v0.13.0.yaml
   ```

3. **配置内存告警**
   ```yaml
   # Prometheus Alert Rule
   - alert: PodMemoryUsageHigh
     expr: |
       container_memory_working_set_bytes / 
       container_spec_memory_limit_bytes > 0.9
     for: 5m
     annotations:
       summary: "Pod {{ $labels.pod }} 内存使用 > 90%"
   ```

4. **定期回顾 OOMKilled 事件**
   ```bash
   kubectl get events --all-namespaces --field-selector reason=OOMKilled \
     --sort-by='.lastTimestamp'
   ```
```

---

#<!-- chunk: 8.3.2 Pod CrashLoopBackOff 取证 Runbook -->## 8.3.2 Pod CrashLoopBackOff 取证 Runbook

##<!-- chunk: 场景描述 -->## 场景描述

Pod 启动后反复崩溃，Kubernetes 以指数退避方式重启容器，状态显示 `CrashLoopBackOff`。

##<!-- chunk: 证据采集清单 -->## 证据采集清单

```bash
# 1. 容器状态和退出码
kubectl describe pod <POD_NAME> -n <NAMESPACE> | grep -A 10 "State:\|Last State:"

# 关键字段:
# - exitCode: 退出码（0=正常，非0=异常）
# - reason: Crash 原因（Error, Completed）
# - message: 错误信息

# 2. 容器日志（当前和上一次）
kubectl logs <POD_NAME> -n <NAMESPACE> --tail=100
kubectl logs <POD_NAME> -n <NAMESPACE> --previous --tail=100

# 3. 启动命令和参数
kubectl get pod <POD_NAME> -n <NAMESPACE> -o jsonpath='{.spec.containers[*].command}'
kubectl get pod <POD_NAME> -n <NAMESPACE> -o jsonpath='{.spec.containers[*].args}'

# 4. 环境变量和配置
kubectl get pod <POD_NAME> -n <NAMESPACE> -o yaml | yq '.spec.containers[].env'

# 5. 健康检查配置
kubectl get pod <POD_NAME> -n <NAMESPACE> -o yaml | yq '.spec.containers[].livenessProbe'
kubectl get pod <POD_NAME> -n <NAMESPACE> -o yaml | yq '.spec.containers[].readinessProbe'

# 6. 事件历史
kubectl get events -n <NAMESPACE> --field-selector involvedObject.name=<POD_NAME> \
  --sort-by='.lastTimestamp'

# 7. Falco 异常检测
logcli query '{app="falco",pod="<POD_NAME>"}' --since=30m

# 8. 依赖服务健康状态（数据库、缓存等）
kubectl get pods -n <DEPENDENCY_NAMESPACE> | grep -i <SERVICE_NAME>
```

##<!-- chunk: 常见原因检查表 -->## 常见原因检查表

```yaml
CrashLoopBackOff 常见原因及证据映射:

1. 应用代码错误 (Application Bug):
   退出码: 1, 137, 139
   日志特征: |
     - Panic/Exception/Error stack trace
     - Segmentation fault
     - Assertion failed
   验证: kubectl logs --previous 查看完整错误堆栈

2. 缺失依赖服务 (Missing Dependencies):
   退出码: 1
   日志特征: |
     - Connection refused
     - Dial tcp: connect: connection refused
     - Unable to connect to database
   验证: 
     - kubectl get svc <DEP_SERVICE> -n <NAMESPACE>
     - telnet <SERVICE_IP> <PORT> (从同 namespace pod)

3. 配置错误 (Misconfiguration):
   退出码: 1
   日志特征: |
     - Configuration file not found
     - Invalid configuration
     - Missing required environment variable
   验证: 
     - kubectl get pod <POD> -o yaml | grep -A 20 "env:"
     - kubectl get configmap/secret <NAME> -n <NAMESPACE>

4. 权限不足 (Permission Denied):
   退出码: 126
   日志特征: |
     - Permission denied
     - Operation not permitted
   验证: 
     - 检查 securityContext.runAsUser
     - 检查 volume 挂载权限

5. 镜像问题 (Image Issue):
   退出码: 127
   日志特征: |
     - Exec format error
     - No such file or directory (entrypoint)
   验证: 
     - kubectl get pod <POD> -o jsonpath='{.spec.containers[].image}'
     - docker pull <IMAGE> && docker run <IMAGE> <COMMAND>

6. 健康检查配置过于严格 (Liveness Probe Too Aggressive):
   退出码: 137 (SIGKILL)
   日志特征: 容器日志正常，但被突然终止
   验证: 
     - kubectl describe pod <POD> | grep -i "Liveness probe failed"
     - 检查 livenessProbe.initialDelaySeconds 是否足够

7. 资源不足 (Resource Constraints):
   退出码: 137 (OOMKilled) 或 143 (SIGTERM)
   日志特征: 可能无日志（被强制终止）
   验证: 
     - kubectl describe pod <POD> | grep -i "OOMKilled\|Evicted"
     - kubectl top pod <POD>

8. 启动超时 (Startup Timeout):
   退出码: 143 (SIGTERM)
   日志特征: 应用正在初始化中被终止
   验证: 
     - 检查 startupProbe 或增大 initialDelaySeconds
     - 测量实际启动时间
```

##<!-- chunk: 自动化取证脚本 -->## 自动化取证脚本

```bash
#!/bin/bash
# crashloop-forensics.sh - CrashLoopBackOff 自动化取证

POD_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$POD_NAME" ]; then
    echo "用法: $0 <POD_NAME> [NAMESPACE]"
    exit 1
fi

echo "=== CrashLoopBackOff 取证报告 ==="
echo "Pod: $POD_NAME"
echo "Namespace: $NAMESPACE"
echo "取证时间: $(date)"
echo ""

# 1. 容器状态
echo "<!-- chunk: 1. 容器状态" -->## 1. 容器状态"
echo "---"
kubectl get pod $POD_NAME -n $NAMESPACE -o json | jq '{
  phase: .status.phase,
  restartCount: .status.containerStatuses[0].restartCount,
  currentState: .status.containerStatuses[0].state,
  lastState: .status.containerStatuses[0].lastState
}'
echo ""

# 2. 退出码分析
echo "<!-- chunk: 2. 退出码分析" -->## 2. 退出码分析"
echo "---"
EXIT_CODE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].lastState.terminated.exitCode}')
echo "退出码: $EXIT_CODE"

case $EXIT_CODE in
    0)
        echo "含义: 正常退出（但应该一直运行，可能是启动脚本问题）"
        ;;
    1)
        echo "含义: 应用错误退出（代码 bug 或配置错误）"
        ;;
    126)
        echo "含义: 命令无法执行（权限问题或文件不存在）"
        ;;
    127)
        echo "含义: 命令未找到（entrypoint 路径错误）"
        ;;
    137)
        echo "含义: 被 SIGKILL 终止（OOM 或 liveness probe 失败）"
        ;;
    139)
        echo "含义: Segmentation Fault（应用崩溃）"
        ;;
    143)
        echo "含义: 被 SIGTERM 终止（启动超时或 preStop hook）"
        ;;
    *)
        echo "含义: 未知退出码，查看日志获取详细信息"
        ;;
esac
echo ""

# 3. 容器日志
echo "<!-- chunk: 3. 容器日志（最后 50 行）" -->## 3. 容器日志（最后 50 行）"
echo "---"
echo "#<!-- chunk: 当前容器日志:" -->## 当前容器日志:"
kubectl logs $POD_NAME -n $NAMESPACE --tail=50 2>&1 | head -30

echo ""
echo "#<!-- chunk: 上一次容器日志:" -->## 上一次容器日志:"
kubectl logs $POD_NAME -n $NAMESPACE --previous --tail=50 2>&1 | head -30
echo ""

# 4. 事件历史
echo "<!-- chunk: 4. 相关事件" -->## 4. 相关事件"
echo "---"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME \
  --sort-by='.lastTimestamp' | tail -10
echo ""

# 5. 健康检查配置
echo "<!-- chunk: 5. 健康检查配置" -->## 5. 健康检查配置"
echo "---"
kubectl get pod $POD_NAME -n $NAMESPACE -o json | jq '.spec.containers[0] | {
  livenessProbe: .livenessProbe,
  readinessProbe: .readinessProbe,
  startupProbe: .startupProbe
}'
echo ""

# 6. 依赖服务检查
echo "<!-- chunk: 6. 依赖服务检查" -->## 6. 依赖服务检查"
echo "---"
# 从环境变量推断依赖服务
DEPS=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.containers[0].env[*].value}' | \
       grep -oE '[a-z-]+\.(svc|svc\.cluster\.local)' | sort -u)

for DEP in $DEPS; do
    SVC_NAME=$(echo $DEP | cut -d. -f1)
    echo "检查服务: $SVC_NAME"
    kubectl get svc $SVC_NAME -n $NAMESPACE 2>&1 | grep -v "Error" || echo "  ⚠️  服务不存在"
done
echo ""

# 7. 根因初步判断
echo "<!-- chunk: 7. 根因初步判断" -->## 7. 根因初步判断"
echo "---"

# 检查日志中的常见错误模式
PREV_LOG=$(kubectl logs $POD_NAME -n $NAMESPACE --previous --tail=100 2>&1)

if echo "$PREV_LOG" | grep -qi "connection refused\|dial tcp"; then
    echo "⚠️  发现连接失败日志 → 可能是依赖服务不可用"
fi

if echo "$PREV_LOG" | grep -qi "panic\|fatal\|exception"; then
    echo "⚠️  发现应用异常日志 → 可能是代码 bug"
fi

if echo "$PREV_LOG" | grep -qi "permission denied"; then
    echo "⚠️  发现权限错误 → 检查 securityContext 和 volume 权限"
fi

if echo "$PREV_LOG" | grep -qi "not found\|no such file"; then
    echo "⚠️  发现文件未找到 → 检查 configmap/secret 挂载或镜像内容"
fi

if [ "$EXIT_CODE" = "137" ]; then
    kubectl describe pod $POD_NAME -n $NAMESPACE | grep -q "Liveness probe failed" && \
        echo "⚠️  Liveness probe 失败 → 增大 initialDelaySeconds 或修复健康检查端点"
fi

echo ""
echo "=== 取证报告结束 ==="
```

##<!-- chunk: 快速修复检查表 -->## 快速修复检查表

```bash
# CrashLoopBackOff 快速修复流程

# Step 1: 查看退出码和最后几行日志
kubectl logs <POD> --previous --tail=20

# Step 2: 根据日志快速判断
┌─────────────────────────────────────────────────────────────────┐
│ 日志特征                    │ 快速修复方法                       │
├─────────────────────────────────────────────────────────────────┤
│ "connection refused"        │ kubectl get svc <SERVICE>          │
│                             │ 确保依赖服务存在且 Ready           │
├─────────────────────────────────────────────────────────────────┤
│ "config file not found"     │ kubectl get cm/secret <NAME>       │
│                             │ 检查 volumeMount 路径是否正确      │
├─────────────────────────────────────────────────────────────────┤
│ "panic: runtime error"      │ 检查代码 bug，回滚到上一版本       │
├─────────────────────────────────────────────────────────────────┤
│ "permission denied"         │ 修改 securityContext:              │
│                             │   runAsUser: 0 (临时)              │
│                             │   fsGroup: <GID>                   │
├─────────────────────────────────────────────────────────────────┤
│ "exec format error"         │ 检查镜像架构(amd64 vs arm64)       │
│                             │ 重新构建镜像                        │
├─────────────────────────────────────────────────────────────────┤
│ 无日志（容器立即退出）      │ kubectl describe pod <POD>         │
│                             │ 检查 image pull 和 command         │
└─────────────────────────────────────────────────────────────────┘

# Step 3: 常用修复命令

# 修复方法 1: 临时禁用健康检查（用于测试）
kubectl patch pod <POD> -n <NAMESPACE> --type='json' \
  -p='[{"op": "remove", "path": "/spec/containers/0/livenessProbe"}]'

# 修复方法 2: 修改环境变量
kubectl set env deployment/<DEPLOY> KEY=VALUE -n <NAMESPACE>

# 修复方法 3: 回滚到上一版本
kubectl rollout undo deployment/<DEPLOY> -n <NAMESPACE>

# 修复方法 4: 交互式调试
kubectl run debug-pod --image=<SAME_IMAGE> -it --rm -- /bin/sh
# 在交互式 shell 中手动执行启动命令，观察错误
```

---

#<!-- chunk: 8.3.3 Node NotReady 取证 Runbook -->## 8.3.3 Node NotReady 取证 Runbook

##<!-- chunk: 场景描述 -->## 场景描述

节点状态变为 `NotReady`，调度器停止向该节点分配新 pod，可能由 kubelet 崩溃、网络分区或资源耗尽引起。

##<!-- chunk: 证据采集清单 -->## 证据采集清单

```bash
# 1. 节点状态和条件
kubectl describe node <NODE_NAME> | grep -A 20 "Conditions:"

# 关键字段:
# - Ready: False (节点不可用)
# - MemoryPressure / DiskPressure / PIDPressure: True (资源压力)
# - NetworkUnavailable: True (网络问题)

# 2. Kubelet 日志
kubectl logs -n kube-system <KUBELET_POD> --tail=200

# 或直接从节点查看 (需要 SSH 访问)
ssh <NODE> journalctl -u kubelet -n 200 --no-pager

# 3. 内核日志
kubectl debug node/<NODE_NAME> -it --image=alpine -- \
  sh -c "dmesg -T | tail -200"

# 4. 资源使用状态
kubectl top node <NODE_NAME>

# 从 Prometheus 查询更详细的指标
node_memory_MemAvailable_bytes{instance=~"<NODE>.*"}
node_filesystem_avail_bytes{instance=~"<NODE>.*",mountpoint="/"}
node_load1{instance=~"<NODE>.*"}

# 5. 网络连接测试
kubectl debug node/<NODE_NAME> -it --image=nicolaka/netshoot -- \
  sh -c "
    ping -c 3 <API_SERVER_IP>
    curl -k https://<API_SERVER_IP>:6443/healthz
    traceroute <API_SERVER_IP>
  "

# 6. 进程和文件描述符
kubectl debug node/<NODE_NAME> -it --image=alpine -- \
  sh -c "
    ps aux | head -20
    ls -l /proc/$(pgrep kubelet)/fd | wc -l  # 文件描述符数量
  "

# 7. K8s 事件
kubectl get events --all-namespaces --field-selector source=kubelet \
  --sort-by='.lastTimestamp' | grep <NODE_NAME>

# 8. Falco 异常检测
logcli query '{app="falco",k8s_node_name="<NODE_NAME>"}' --since=1h \
  | grep -i "error\|critical"
```

##<!-- chunk: 时间线重建模板 -->## 时间线重建模板

```markdown
# Node NotReady 事件时间线

**节点**: <NODE_NAME>  
**首次发现时间**: <TIMESTAMP>  
**恢复时间**: <TIMESTAMP> (如适用)  
**影响范围**: <受影响的 Pod 数量>

<!-- chunk: 时间线 (UTC) -->## 时间线 (UTC)

| 时间 | 证据来源 | 事件描述 | 严重性 |
|------|----------|----------|--------|
| T-30min | Prometheus | node_load1 从 2.5 升至 15.3 (400% 增长) | Warning |
| T-25min | Kubelet Log | PLEG: relist takes 10s (正常 < 1s) | Warning |
| T-20min | Kernel Log | TCP: out of memory -- consider tuning tcp_mem | Error |
| T-15min | Kubelet Log | Failed to update node lease: connection refused | Critical |
| T-10min | K8s Event | Node <NODE> status is now: NotReady | Critical |
| T-5min | Falco | High CPU usage by process 'containerd' (95%) | Warning |
| T-0min | Kubelet Log | Kubelet stopped posting node status | Critical |

<!-- chunk: 根因分析 -->## 根因分析

**初步判断**: 网络 socket 耗尽导致 kubelet 无法与 API Server 通信

**支持证据**:
1. 内核日志显示 "out of memory" (实际是 socket buffer 不足)
2. Kubelet 日志显示连接 API Server 失败
3. PLEG (Pod Lifecycle Event Generator) 延迟异常高
4. containerd 进程 CPU 占用 95%（可能陷入忙等）

**根因**: 某个应用创建了大量短连接但未正确关闭，耗尽了 socket buffer

<!-- chunk: 修复措施 -->## 修复措施

**临时措施**:
- 重启节点或 kubelet 服务
- 临时增大 tcp_mem 内核参数

**长期措施**:
- 识别并修复产生大量短连接的应用
- 调整内核参数 net.ipv4.tcp_mem
- 启用连接池
- 监控 socket 使用量
```

##<!-- chunk: 根因分类决策树 -->## 根因分类决策树

```
┌─────────────────────────────────────────────────────────────────┐
│              Node NotReady 根因分类决策树                        │
└─────────────────────────────────────────────────────────────────┘

[Start] 节点状态 = NotReady
   │
   ├─► 检查: Kubelet 进程状态
   │     │
   │     ├─► Kubelet 进程不存在
   │     │    └─► 根因: Kubelet 崩溃
   │     │        证据:
   │     │          - ps aux | grep kubelet 无输出
   │     │          - journalctl -u kubelet 显示 crash/panic
   │     │          - /var/log/syslog 中有 segfault 记录
   │     │        处置:
   │     │          1. systemctl restart kubelet
   │     │          2. 检查 kubelet 二进制文件完整性
   │     │          3. 升级到稳定版本
   │     │
   │     └─► Kubelet 进程存在
   │          └─► 继续检查网络连接
   │
   ├─► 检查: Kubelet 到 API Server 的连接
   │     │
   │     ├─► 无法连接 API Server
   │     │    └─► 根因: 网络分区
   │     │        证据:
   │     │          - ping API_SERVER_IP 失败
   │     │          - kubelet log: "dial tcp: i/o timeout"
   │     │          - traceroute 显示路径不通
   │     │        处置:
   │     │          1. 检查网络设备（交换机、路由器）
   │     │          2. 检查防火墙规则
   │     │          3. 检查 CNI 插件状态（Calico/Flannel）
   │     │
   │     └─► 可以连接 API Server
   │          └─► 继续检查资源压力
   │
   ├─► 检查: 节点资源压力
   │     │
   │     ├─► MemoryPressure = True
   │     │    └─► 根因: 节点内存不足
   │     │        证据:
   │     │          - node_memory_MemAvailable_bytes < 100MB
   │     │          - dmesg 中有 OOM killer 日志
   │     │          - kube_node_status_condition{condition="MemoryPressure"} = 1
   │     │        处置:
   │     │          1. kubectl drain <NODE> (驱逐非关键 pod)
   │     │          2. 增加节点内存或删除内存泄漏的应用
   │     │          3. 配置 pod QoS 和 eviction thresholds
   │     │
   │     ├─► DiskPressure = True
   │     │    └─► 根因: 磁盘空间不足
   │     │        证据:
   │     │          - df -h 显示 / 或 /var 使用率 > 85%
   │     │          - 大量容器镜像或日志占用空间
   │     │        处置:
   │     │          1. 清理未使用的镜像: docker system prune -a
   │     │          2. 配置日志轮转和限制
   │     │          3. 增大磁盘或挂载额外存储
   │     │
   │     ├─► PIDPressure = True
   │     │    └─► 根因: 进程数达到上限
   │     │        证据:
   │     │          - ps aux | wc -l 接近 /proc/sys/kernel/pid_max
   │     │          - kubelet log: "fork: resource temporarily unavailable"
   │     │        处置:
   │     │          1. 识别并终止产生大量进程的容器
   │     │          2. 增大 pid_max: echo 4194304 > /proc/sys/kernel/pid_max
   │     │          3. 配置 pod 的 PID 限制
   │     │
   │     └─► 无资源压力
   │          └─► 继续检查容器运行时
   │
   └─► 检查: 容器运行时状态
         │
         ├─► containerd/docker 进程无响应
         │    └─► 根因: 容器运行时故障
         │        证据:
         │          - crictl ps 超时或报错
         │          - containerd 进程 CPU 100% 或僵死
         │          - kubelet log: "PLEG is not healthy"
         │        处置:
         │          1. systemctl restart containerd
         │          2. 检查 /var/lib/containerd 磁盘空间
         │          3. 升级 containerd 版本
         │
         └─► CNI 插件故障
              └─► 根因: 网络插件错误
                  证据:
                    - pod 状态 ContainerCreating (卡住)
                    - kubelet log: "failed to setup network for sandbox"
                    - /var/log/pods/<CNI_POD>/... 中有错误
                  处置:
                    1. 重启 CNI pod (Calico/Flannel)
                    2. 检查 CNI 配置文件 /etc/cni/net.d/
                    3. 验证 CNI 插件二进制文件完整性
```

##<!-- chunk: 自动化取证脚本 -->## 自动化取证脚本

```bash
#!/bin/bash
# node-notready-forensics.sh - Node NotReady 自动化取证

NODE_NAME=$1

if [ -z "$NODE_NAME" ]; then
    echo "用法: $0 <NODE_NAME>"
    exit 1
fi

echo "=== Node NotReady 取证报告 ==="
echo "节点: $NODE_NAME"
echo "取证时间: $(date)"
echo ""

# 1. 节点状态
echo "<!-- chunk: 1. 节点状态" -->## 1. 节点状态"
echo "---"
kubectl get node $NODE_NAME -o json | jq '{
  status: .status.conditions[] | select(.type=="Ready") | .status,
  reason: .status.conditions[] | select(.type=="Ready") | .reason,
  message: .status.conditions[] | select(.type=="Ready") | .message,
  conditions: .status.conditions
}'
echo ""

# 2. 资源压力检查
echo "<!-- chunk: 2. 资源压力" -->## 2. 资源压力"
echo "---"
kubectl describe node $NODE_NAME | grep -A 5 "Conditions:"
echo ""

# 3. Kubelet 健康检查
echo "<!-- chunk: 3. Kubelet 健康检查" -->## 3. Kubelet 健康检查"
echo "---"
echo "尝试连接 Kubelet API (需要在节点上执行):"
echo "  curl -k https://localhost:10250/healthz"
echo ""

# 4. 资源使用趋势
echo "<!-- chunk: 4. 资源使用趋势（过去 1 小时）" -->## 4. 资源使用趋势（过去 1 小时）"
echo "---"
PROM_URL="http://localhost:9090"

# CPU 负载
echo "CPU 负载:"
curl -s -G "$PROM_URL/api/v1/query" \
  --data-urlencode "query=node_load1{instance=~\"$NODE_NAME.*\"}" | \
  jq -r '.data.result[0].value[1] // "N/A"'

# 内存可用
echo "可用内存 (MB):"
curl -s -G "$PROM_URL/api/v1/query" \
  --data-urlencode "query=node_memory_MemAvailable_bytes{instance=~\"$NODE_NAME.*\"}" | \
  jq -r '(.data.result[0].value[1] // "0" | tonumber) / 1024 / 1024'

# 磁盘使用率
echo "根分区使用率 (%):"
curl -s -G "$PROM_URL/api/v1/query" \
  --data-urlencode "query=(1 - node_filesystem_avail_bytes{instance=~\"$NODE_NAME.*\",mountpoint=\"/\"} / node_filesystem_size_bytes{instance=~\"$NODE_NAME.*\",mountpoint=\"/\"}) * 100" | \
  jq -r '.data.result[0].value[1] // "N/A"'
echo ""

# 5. 网络连接测试
echo "<!-- chunk: 5. 网络连接测试" -->## 5. 网络连接测试"
echo "---"
API_SERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}' | sed 's|https://||' | cut -d: -f1)
echo "API Server: $API_SERVER"

echo "从节点到 API Server 的连接测试:"
kubectl debug node/$NODE_NAME -it --image=alpine -- sh -c "
  ping -c 3 $API_SERVER 2>&1 | head -10
" 2>/dev/null
echo ""

# 6. Pod 状态（运行在该节点上的）
echo "<!-- chunk: 6. 节点上的 Pod 状态" -->## 6. 节点上的 Pod 状态"
echo "---"
kubectl get pods --all-namespaces --field-selector spec.nodeName=$NODE_NAME \
  -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,STATUS:.status.phase,RESTARTS:.status.containerStatuses[0].restartCount
echo ""

# 7. 最近的事件
echo "<!-- chunk: 7. 相关事件（最近 20 条）" -->## 7. 相关事件（最近 20 条）"
echo "---"
kubectl get events --all-namespaces --field-selector involvedObject.name=$NODE_NAME \
  --sort-by='.lastTimestamp' | tail -20
echo ""

# 8. 根因初步判断
echo "<!-- chunk: 8. 根因初步判断" -->## 8. 根因初步判断"
echo "---"

# 检查条件
READY_STATUS=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}')
MEMORY_PRESSURE=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="MemoryPressure")].status}')
DISK_PRESSURE=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}')
PID_PRESSURE=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="PIDPressure")].status}')

if [ "$MEMORY_PRESSURE" = "True" ]; then
    echo "⚠️  内存压力 → 节点内存不足，驱逐 pod 或增加内存"
fi

if [ "$DISK_PRESSURE" = "True" ]; then
    echo "⚠️  磁盘压力 → 清理镜像和日志"
fi

if [ "$PID_PRESSURE" = "True" ]; then
    echo "⚠️  进程数压力 → 识别并终止产生大量进程的容器"
fi

# 检查 Kubelet 日志（需要节点访问权限）
echo ""
echo "建议检查 Kubelet 日志："
echo "  kubectl debug node/$NODE_NAME -it --image=alpine -- sh -c 'chroot /host journalctl -u kubelet -n 50'"

echo ""
echo "=== 取证报告结束 ==="
```

---

#<!-- chunk: 8.3.4 Service 间歇性超时取证 Runbook -->## 8.3.4 Service 间歇性超时取证 Runbook

##<!-- chunk: 场景描述 -->## 场景描述

Service 间歇性出现请求超时（非 100% 失败），用户报告服务不稳定，需要跨网络层、应用层和系统层关联证据。

##<!-- chunk: 证据采集清单 -->## 证据采集清单

```bash
# 1. Service 和 Endpoint 状态
kubectl get svc <SERVICE_NAME> -n <NAMESPACE> -o yaml
kubectl get endpoints <SERVICE_NAME> -n <NAMESPACE>

# 检查后端 pod 数量和就绪状态
kubectl get pods -n <NAMESPACE> -l <SELECTOR> -o wide

# 2. 连接池指标（Prometheus）
# HTTP 请求延迟分位数
histogram_quantile(0.99, 
  sum(rate(http_request_duration_seconds_bucket{service="<SERVICE>"}[5m])) by (le)
)

# 连接池活跃连接数
connection_pool_active{service="<SERVICE>"}

# 连接池等待队列长度
connection_pool_wait_duration_seconds{service="<SERVICE>"}

# 3. 网络流量分析
# 使用 tcpdump 抓包（在 pod 或节点上）
kubectl exec -it <POD> -- tcpdump -i any -w /tmp/capture.pcap port <PORT>

# 或使用 eBPF 工具分析连接状态
kubectl debug node/<NODE> -it --image=quay.io/iovisor/bcc -- \
  /usr/share/bcc/tools/tcpretrans  # TCP 重传统计

# 4. 应用日志模式匹配
logcli query '{namespace="<NAMESPACE>",app="<APP>"}' --since=30m \
  | grep -i "timeout\|connection\|refused"

# 统计超时错误频率
logcli query '{namespace="<NAMESPACE>",app="<APP>"}' --since=1h \
  | grep -c "timeout"

# 5. DNS 解析问题
kubectl exec -it <POD> -- nslookup <SERVICE_NAME>.<NAMESPACE>.svc.cluster.local

# CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100 | grep <SERVICE_NAME>

# 6. iptables/IPVS 规则检查（kube-proxy）
# 在节点上执行
kubectl debug node/<NODE> -it --image=alpine -- sh -c "
  iptables-save | grep <SERVICE_IP>
  ipvsadm -Ln | grep <SERVICE_IP>
"

# 7. 分布式追踪（如果启用 Jaeger/Zipkin）
# 查询 span 中的高延迟操作
curl "http://jaeger-query:16686/api/traces?service=<SERVICE>&lookback=1h&limit=50" | \
  jq '.data[] | select(.spans[].duration > 1000000)'  # > 1 秒的 span

# 8. Pod CPU/内存是否接近限制
kubectl top pods -n <NAMESPACE> -l <SELECTOR>

# 从 Prometheus 查询 CPU 节流
sum(rate(container_cpu_cfs_throttled_seconds_total{pod=~"<POD_PATTERN>"}[5m])) by (pod)
```

##<!-- chunk: 跨层证据关联技术 -->## 跨层证据关联技术

```yaml
间歇性超时跨层证据关联矩阵:

┌────────────────────────────────────────────────────────────────────────┐
│ 证据层级     │ 证据类型               │ 关联字段             │ 工具   │
├────────────────────────────────────────────────────────────────────────┤
│ 应用层       │ HTTP 请求日志          │ request_id, timestamp│ Loki   │
│              │ 错误日志 (timeout)     │ timestamp, pod_name  │ Loki   │
│              │ 分布式追踪 span        │ trace_id, span_id    │ Jaeger │
├────────────────────────────────────────────────────────────────────────┤
│ 网络层       │ TCP 连接状态           │ src_ip, dst_ip, time │ eBPF   │
│              │ 数据包丢失率           │ interface, timestamp │ tcpdump│
│              │ DNS 解析延迟           │ query_name, latency  │ CoreDNS│
├────────────────────────────────────────────────────────────────────────┤
│ 系统层       │ CPU 节流事件           │ pod_name, timestamp  │ Prom   │
│              │ 内存页故障             │ pod_name, timestamp  │ Falco  │
│              │ 文件描述符耗尽         │ pod_name, fd_count   │ Node Ex│
├────────────────────────────────────────────────────────────────────────┤
│ Kubernetes层 │ Endpoint 变更事件      │ service, timestamp   │ K8s API│
│              │ Pod Ready 状态变化     │ pod_name, timestamp  │ Metrics│
│              │ Service 负载均衡规则   │ service, backend_ips │ kube-px│
└────────────────────────────────────────────────────────────────────────┘

关联查询示例 (PromQL + LogQL):

# 查询 1: 找到高延迟请求时刻的 CPU 节流情况
# 步骤 1: 从 Loki 获取超时时间戳
logcli query '{app="myapp"}' --since=1h | grep "timeout" | awk '{print $1}'

# 步骤 2: 在 Prometheus 中查询对应时间的 CPU 节流
# @<timestamp> 语法查询特定时刻的值
container_cpu_cfs_throttled_seconds_total{pod="myapp-xxx"} @1707123456

# 查询 2: 关联 DNS 解析延迟和请求失败
# 从 CoreDNS 指标找到高延迟的查询
coredns_dns_request_duration_seconds{type="A"} > 0.5

# 在 Loki 中查询同一时间窗口的应用错误
logcli query '{app="myapp"} |= "connection refused"' --since=5m --until=now

# 查询 3: 关联 Endpoint 变更和超时激增
# Prometheus 告警规则
ALERT ServiceEndpointFlapping
  IF changes(kube_endpoint_address_available{service="myapp"}[5m]) > 5
  FOR 2m

# 然后查询同时间段的请求错误率
rate(http_requests_total{status="503"}[5m])
```

##<!-- chunk: 典型故障模式和取证路径 -->## 典型故障模式和取证路径

```markdown
<!-- chunk: 模式 1: 连接池耗尽 -->## 模式 1: 连接池耗尽

**症状**:
- 高峰期请求超时
- 日志: "wait for connection timeout"
- 指标: connection_pool_wait_duration_seconds 激增

**取证路径**:
1. 确认连接池配置
   ```bash
   kubectl get cm <APP_CONFIG> -o yaml | grep -i "pool\|connection"
   ```

2. 查看活跃连接数历史
   ```promql
   connection_pool_active{app="myapp"}
   connection_pool_max{app="myapp"}
   ```

3. 检查数据库端连接数限制
   ```sql
   SHOW VARIABLES LIKE 'max_connections';
   SHOW STATUS LIKE 'Threads_connected';
   ```

**根因**: 连接池大小 < 实际并发需求

**修复**: 增大连接池或启用连接复用

---

<!-- chunk: 模式 2: Pod 滚动更新期间流量丢失 -->## 模式 2: Pod 滚动更新期间流量丢失

**症状**:
- 部署期间间歇性 503 错误
- 日志: "connection refused"
- 时间上关联 kubectl rollout 操作

**取证路径**:
1. 检查审计日志中的 deployment 更新
   ```bash
   sudo grep "deployments" /var/log/kubernetes/audit.log | \
     jq 'select(.verb=="update" and .objectRef.name=="<DEPLOY>")'
   ```

2. 查看 Endpoint 变更历史
   ```bash
   kubectl get events --field-selector involvedObject.name=<SERVICE> \
     | grep "Endpoints"
   ```

3. 验证 terminationGracePeriodSeconds 配置
   ```bash
   kubectl get pod <POD> -o jsonpath='{.spec.terminationGracePeriodSeconds}'
   ```

4. 检查 preStop hook 和 readiness probe
   ```yaml
   kubectl get deploy <DEPLOY> -o yaml | yq '.spec.template.spec'
   ```

**根因**: Pod 在接收 SIGTERM 后立即从 Endpoint 移除，但仍有请求路由到该 pod

**修复**:
```yaml
spec:
  template:
    spec:
      containers:
      - lifecycle:
          preStop:
            exec:
              command: ["sleep", "15"]  # 延迟关闭
      terminationGracePeriodSeconds: 30
```

---

<!-- chunk: 模式 3: DNS 解析间歇性失败 -->## 模式 3: DNS 解析间歇性失败

**症状**:
- 间歇性 "no such host" 错误
- 高频 DNS 查询场景
- CoreDNS pod 重启或 CPU 高

**取证路径**:
1. 检查 CoreDNS 健康状态
   ```bash
   kubectl get pods -n kube-system -l k8s-app=kube-dns
   kubectl top pods -n kube-system -l k8s-app=kube-dns
   ```

2. 查看 CoreDNS 错误日志
   ```bash
   kubectl logs -n kube-system -l k8s-app=kube-dns | grep -i "error\|timeout"
   ```

3. 分析 DNS 查询 QPS
   ```promql
   rate(coredns_dns_requests_total[5m])
   ```

4. 检查 pod 的 dnsPolicy 和 dnsConfig
   ```bash
   kubectl get pod <POD> -o jsonpath='{.spec.dnsPolicy}'
   ```

**根因**: CoreDNS 资源不足或 pod 未配置 DNS 缓存

**修复**:
1. 增加 CoreDNS 副本和资源
2. 配置应用层 DNS 缓存
3. 使用 NodeLocal DNSCache

---

<!-- chunk: 模式 4: CPU 节流导致请求排队 -->## 模式 4: CPU 节流导致请求排队

**症状**:
- P99 延迟高但 P50 正常
- CPU 使用率接近 limits
- 无错误日志但响应慢

**取证路径**:
1. 查询 CPU 节流指标
   ```promql
   rate(container_cpu_cfs_throttled_seconds_total{pod="myapp"}[5m])
   ```

2. 对比 CPU 使用和限制
   ```promql
   rate(container_cpu_usage_seconds_total{pod="myapp"}[5m]) /
   container_spec_cpu_quota{pod="myapp"} * 100
   ```

3. 检查应用线程池配置
   ```bash
   kubectl exec <POD> -- jstack <PID> | grep -i "waiting\|blocked"
   ```

**根因**: CPU limits 过低导致应用被频繁节流

**修复**:
```yaml
resources:
  requests:
    cpu: "1000m"  # 保证基础性能
  limits:
    cpu: "2000m"  # 允许突发，或移除 limits
```
```

---

#<!-- chunk: 8.3.5 证书过期导致服务中断取证 Runbook -->## 8.3.5 证书过期导致服务中断取证 Runbook

##<!-- chunk: 场景描述 -->## 场景描述

由于证书过期导致 API Server、Kubelet 或 Ingress 等组件无法正常工作，需要快速定位过期证书并恢复服务。

##<!-- chunk: 证据采集清单 -->## 证据采集清单

```bash
# 1. 检查所有 Kubernetes 证书
# API Server 证书
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# Kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# Etcd 证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates

# 2. 检查 Ingress TLS 证书（从 Secret）
kubectl get secrets --all-namespaces -o json | \
  jq -r '.items[] | select(.type=="kubernetes.io/tls") | 
         "\(.metadata.namespace)/\(.metadata.name): \(.data."tls.crt")"' | \
while IFS=: read ns_name cert_b64; do
  echo "=== $ns_name ==="
  echo "$cert_b64" | base64 -d | openssl x509 -noout -dates
done

# 3. API Server 日志（证书错误）
kubectl logs -n kube-system kube-apiserver-<NODE> | \
  grep -i "certificate\|tls\|x509"

# 4. Kubelet 日志
journalctl -u kubelet | grep -i "certificate\|tls\|x509" | tail -50

# 5. K8s 审计日志（认证失败）
sudo grep "Unauthorized" /var/log/kubernetes/audit.log | \
  jq 'select(.responseStatus.code == 401)'

# 6. Falco 告警（TLS 错误）
logcli query '{app="falco"}' --since=24h | grep -i "certificate\|tls"

# 7. Prometheus 告警
curl -s http://localhost:9090/api/v1/alerts | \
  jq '.data.alerts[] | select(.labels.alertname | contains("Certificate"))'
```

##<!-- chunk: 证书链验证命令 -->## 证书链验证命令

```bash
#!/bin/bash
# cert-verification.sh - 证书链完整性验证

echo "=== Kubernetes 证书验证工具 ==="

# 定义证书文件路径
declare -A CERTS=(
  ["API Server"]="/etc/kubernetes/pki/apiserver.crt"
  ["API Server Kubelet Client"]="/etc/kubernetes/pki/apiserver-kubelet-client.crt"
  ["Front Proxy Client"]="/etc/kubernetes/pki/front-proxy-client.crt"
  ["Etcd Server"]="/etc/kubernetes/pki/etcd/server.crt"
  ["Etcd Peer"]="/etc/kubernetes/pki/etcd/peer.crt"
  ["Kubelet Server"]="/var/lib/kubelet/pki/kubelet.crt"
  ["Kubelet Client"]="/var/lib/kubelet/pki/kubelet-client-current.pem"
)

CA="/etc/kubernetes/pki/ca.crt"

# 检查每个证书
for NAME in "${!CERTS[@]}"; do
  CERT_PATH="${CERTS[$NAME]}"
  
  echo ""
  echo "=== 检查: $NAME ==="
  echo "路径: $CERT_PATH"
  
  if [ ! -f "$CERT_PATH" ]; then
    echo "❌ 证书文件不存在"
    continue
  fi
  
  # 提取证书信息
  NOT_BEFORE=$(openssl x509 -in "$CERT_PATH" -noout -startdate | cut -d= -f2)
  NOT_AFTER=$(openssl x509 -in "$CERT_PATH" -noout -enddate | cut -d= -f2)
  SUBJECT=$(openssl x509 -in "$CERT_PATH" -noout -subject | cut -d= -f2-)
  ISSUER=$(openssl x509 -in "$CERT_PATH" -noout -issuer | cut -d= -f2-)
  
  echo "主体: $SUBJECT"
  echo "颁发者: $ISSUER"
  echo "有效期: $NOT_BEFORE"
  echo "到期日期: $NOT_AFTER"
  
  # 计算剩余天数
  EXPIRE_TIMESTAMP=$(date -d "$NOT_AFTER" +%s 2>/dev/null || date -j -f "%b %d %H:%M:%S %Y %Z" "$NOT_AFTER" +%s)
  CURRENT_TIMESTAMP=$(date +%s)
  DAYS_LEFT=$(( ($EXPIRE_TIMESTAMP - $CURRENT_TIMESTAMP) / 86400 ))
  
  if [ $DAYS_LEFT -lt 0 ]; then
    echo "❌ 证书已过期 ($DAYS_LEFT 天)"
  elif [ $DAYS_LEFT -lt 30 ]; then
    echo "⚠️  证书即将过期 ($DAYS_LEFT 天)"
  else
    echo "✅ 证书有效 (剩余 $DAYS_LEFT 天)"
  fi
  
  # 验证证书链（如果有 CA）
  if [ -f "$CA" ] && [[ "$NAME" != "Kubelet"* ]]; then
    if openssl verify -CAfile "$CA" "$CERT_PATH" > /dev/null 2>&1; then
      echo "✅ 证书链验证通过"
    else
      echo "❌ 证书链验证失败"
    fi
  fi
done

echo ""
echo "=== 证书验证完成 ==="
```

##<!-- chunk: 证书续期操作指南 -->## 证书续期操作指南

```bash
# 方法 1: 使用 kubeadm 自动续期（推荐）

# 检查哪些证书即将过期
kubeadm certs check-expiration

# 续期所有证书
kubeadm certs renew all

# 只续期特定证书
kubeadm certs renew apiserver
kubeadm certs renew apiserver-kubelet-client

# 重启控制平面组件（使新证书生效）
kubectl -n kube-system delete pod -l component=kube-apiserver
kubectl -n kube-system delete pod -l component=kube-controller-manager
kubectl -n kube-system delete pod -l component=kube-scheduler

# 重启 kubelet
systemctl restart kubelet

# 方法 2: 手动续期（高级场景）

# 备份旧证书
cp -r /etc/kubernetes/pki /etc/kubernetes/pki.backup.$(date +%Y%m%d)

# 生成新的 API Server 证书
openssl genrsa -out /etc/kubernetes/pki/apiserver.key 2048
openssl req -new -key /etc/kubernetes/pki/apiserver.key \
  -out /etc/kubernetes/pki/apiserver.csr \
  -subj "/CN=kube-apiserver"

# 使用 CA 签署新证书
openssl x509 -req -in /etc/kubernetes/pki/apiserver.csr \
  -CA /etc/kubernetes/pki/ca.crt \
  -CAkey /etc/kubernetes/pki/ca.key \
  -CAcreateserial \
  -out /etc/kubernetes/pki/apiserver.crt \
  -days 365

# 更新 kubeconfig 文件
kubeadm init phase kubeconfig all

# 方法 3: 自动化证书续期（cert-manager）

# 安装 cert-manager
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.13.0/cert-manager.yaml

# 创建自签名 ClusterIssuer
cat <<EOF | kubectl apply -f -
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: selfsigned-issuer
spec:
  selfSigned: {}
EOF

# 为 Ingress 配置自动证书
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: myapp
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  tls:
  - hosts:
    - myapp.example.com
    secretName: myapp-tls
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: myapp
            port:
              number: 80
EOF

# 方法 4: 预防性监控和告警

# Prometheus 告警规则
cat <<'EOF' | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-expiry-alerts
  namespace: monitoring
spec:
  groups:
  - name: certificates
    interval: 30s
    rules:
    - alert: CertificateExpiryWarning
      expr: |
        (kubelet_certificate_manager_client_ttl_seconds < 7*24*3600)
        or
        (apiserver_client_certificate_expiration_seconds_count{job="apiserver"} < 7*24*3600)
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "证书将在 7 天内过期"
        description: "{{ $labels.job }} 的证书将在 {{ $value | humanizeDuration }} 后过期"
    
    - alert: CertificateExpiryCritical
      expr: |
        (kubelet_certificate_manager_client_ttl_seconds < 24*3600)
        or
        (apiserver_client_certificate_expiration_seconds_count{job="apiserver"} < 24*3600)
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "证书将在 24 小时内过期！"
        description: "{{ $labels.job }} 的证书即将过期，请立即处理"
EOF
```

---

#<!-- chunk: 8.3.6 配置漂移（静默失败）取证 Runbook -->## 8.3.6 配置漂移（静默失败）取证 Runbook

##<!-- chunk: 场景描述 -->## 场景描述

配置文件（ConfigMap/Secret）被意外修改或漂移，导致应用行为异常但无明显错误日志，属于"一切看起来正常但结果错误"的场景。

##<!-- chunk: 证据采集清单 -->## 证据采集清单

```bash
# 1. 获取当前配置快照
kubectl get cm <CONFIG_NAME> -n <NAMESPACE> -o yaml > config-current.yaml
kubectl get secret <SECRET_NAME> -n <NAMESPACE> -o yaml > secret-current.yaml

# 2. 查看配置修改历史（审计日志）
sudo grep "<CONFIG_NAME>" /var/log/kubernetes/audit.log | \
  jq 'select(.objectRef.resource=="configmaps" and .verb=="update" or .verb=="patch")' | \
  jq -s 'sort_by(.requestReceivedTimestamp) | reverse | .[:10]'  # 最近 10 次修改

# 3. 对比 Git 仓库中的期望配置（GitOps）
# 假设使用 ArgoCD 或 Flux
kubectl get cm <CONFIG_NAME> -n <NAMESPACE> -o yaml > /tmp/config-actual.yaml

git clone <YOUR_GITOPS_REPO>
diff -u <YOUR_GITOPS_REPO>/manifests/configmap.yaml /tmp/config-actual.yaml

# 或使用 kubectl diff
kubectl diff -f <YOUR_GITOPS_REPO>/manifests/

# 4. 检查 ConfigMap/Secret 的 annotation（可能记录了变更信息）
kubectl get cm <CONFIG_NAME> -n <NAMESPACE> -o jsonpath='{.metadata.annotations}'

# 5. 查看应用行为异常的时间点
logcli query '{namespace="<NAMESPACE>",app="<APP>"}' \
  --since=24h | grep -i "error\|warning\|unexpected"

# 6. 关联配置修改时间和应用异常时间
# 从审计日志获取修改时间
CHANGE_TIME=$(sudo grep "<CONFIG_NAME>" /var/log/kubernetes/audit.log | \
  jq -r 'select(.verb=="update") | .requestReceivedTimestamp' | tail -1)

echo "配置修改时间: $CHANGE_TIME"

# 查询该时间点后的应用日志
logcli query '{app="<APP>"}' --from="$CHANGE_TIME" --limit=50

# 7. 验证配置是否被正确加载到容器
# 如果使用 volume 挂载
kubectl exec <POD> -- cat /etc/config/app.conf

# 如果使用环境变量
kubectl exec <POD> -- env | grep <KEY_NAME>

# 8. 检查配置热重载机制是否生效
# 查看应用是否检测到配置变更
kubectl logs <POD> | grep -i "config\|reload"
```

##<!-- chunk: 配置漂移检测脚本 -->## 配置漂移检测脚本

```bash
#!/bin/bash
# config-drift-detection.sh - 检测配置漂移

NAMESPACE=$1
GITOPS_REPO=$2

if [ -z "$NAMESPACE" ] || [ -z "$GITOPS_REPO" ]; then
    echo "用法: $0 <NAMESPACE> <GITOPS_REPO_PATH>"
    exit 1
fi

echo "=== 配置漂移检测报告 ==="
echo "Namespace: $NAMESPACE"
echo "GitOps Repo: $GITOPS_REPO"
echo "检测时间: $(date)"
echo ""

# 1. 列出所有 ConfigMap 和 Secret
echo "<!-- chunk: 1. 资源清单" -->## 1. 资源清单"
echo "---"
echo "ConfigMaps:"
kubectl get cm -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}'

echo ""
echo "Secrets:"
kubectl get secret -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}'
echo ""

# 2. 对比每个 ConfigMap
echo "<!-- chunk: 2. ConfigMap 漂移检测" -->## 2. ConfigMap 漂移检测"
echo "---"
for CM in $(kubectl get cm -n $NAMESPACE -o jsonpath='{.items[*].metadata.name}'); do
    echo "检查: $CM"
    
    EXPECTED_FILE="$GITOPS_REPO/configmaps/$CM.yaml"
    
    if [ ! -f "$EXPECTED_FILE" ]; then
        echo "  ⚠️  Git 仓库中未找到期望配置: $EXPECTED_FILE"
        continue
    fi
    
    # 导出当前配置
    kubectl get cm $CM -n $NAMESPACE -o yaml | \
      yq 'del(.metadata.creationTimestamp, .metadata.resourceVersion, .metadata.uid, .metadata.managedFields)' \
      > /tmp/cm-actual.yaml
    
    # 对比
    if diff -u "$EXPECTED_FILE" /tmp/cm-actual.yaml > /tmp/cm-diff.txt 2>&1; then
        echo "  ✅ 配置一致"
    else
        echo "  ❌ 配置漂移detected！"
        echo "  差异:"
        cat /tmp/cm-diff.txt | head -20
    fi
    echo ""
done

# 3. 查询最近的配置修改操作
echo "<!-- chunk: 3. 最近的配置修改操作（审计日志）" -->## 3. 最近的配置修改操作（审计日志）"
echo "---"
sudo grep "configmaps\|secrets" /var/log/kubernetes/audit.log | \
  jq -c 'select(.objectRef.namespace=="'$NAMESPACE'" and (.verb=="update" or .verb=="patch")) | 
         {time: .requestReceivedTimestamp, user: .user.username, resource: .objectRef.name, verb: .verb}' | \
  tail -10

echo ""
echo "=== 检测完成 ==="
```

##<!-- chunk: GitOps 最佳实践 -->## GitOps 最佳实践

```yaml
# 防止配置漂移的 GitOps 配置示例 (ArgoCD)

apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: myapp
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/your-org/your-repo
    targetRevision: HEAD
    path: manifests/myapp
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  
  # 关键配置：自动同步和自我修复
  syncPolicy:
    automated:
      prune: true        # 自动删除不在 Git 中的资源
      selfHeal: true     # 自动恢复漂移的配置
      allowEmpty: false
    syncOptions:
    - CreateNamespace=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m

---

# Prometheus 告警：检测配置漂移
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: config-drift-alerts
  namespace: monitoring
spec:
  groups:
  - name: gitops
    interval: 60s
    rules:
    - alert: ArgoCDOutOfSync
      expr: |
        argocd_app_info{sync_status="OutOfSync"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "ArgoCD 应用 {{ $labels.name }} 配置不同步"
        description: "应用配置已漂移，与 Git 仓库不一致"
    
    - alert: ConfigMapModifiedManually
      expr: |
        (time() - kube_configmap_info) < 300  # 5 分钟内创建/更新
        unless on(configmap, namespace) 
        label_replace(argocd_app_info{sync_status="Synced"}, "configmap", "$1", "name", "(.*)")
      labels:
        severity: warning
      annotations:
        summary: "ConfigMap {{ $labels.configmap }} 被手动修改"
        description: "检测到非 GitOps 流程的配置修改"

---

# Policy-as-Code: 使用 OPA Gatekeeper 防止手动修改

apiVersion: templates.gatekeeper.sh/v1beta1
kind: ConstraintTemplate
metadata:
  name: k8srequiregitopsannotation
spec:
  crd:
    spec:
      names:
        kind: K8sRequireGitOpsAnnotation
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiregitopsannotation
        
        violation[{"msg": msg}] {
          input.review.operation == "UPDATE"
          input.review.kind.kind == "ConfigMap"
          not input.review.object.metadata.annotations["argocd.argoproj.io/tracking-id"]
          msg := sprintf("ConfigMap %v 必须通过 GitOps 更新", [input.review.object.metadata.name])
        }

---

apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequireGitOpsAnnotation
metadata:
  name: require-gitops-for-config
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["ConfigMap", "Secret"]
    namespaces: ["production"]
```

---

<!-- chunk: 8.4 FTA + FEBM 联合诊断实战指南 -->## 8.4 FTA + FEBM 联合诊断实战指南

#<!-- chunk: 8.4.1 何时使用 FTA vs FEBM -->## 8.4.1 何时使用 FTA vs FEBM

```
┌─────────────────────────────────────────────────────────────────┐
│               FTA vs FEBM 决策流程图                             │
└─────────────────────────────────────────────────────────────────┘

                      [故障发生]
                           │
                           ▼
                 ┌──────────────────┐
                 │ 是否是已知故障  │
                 │ 模式？          │
                 └────┬─────────┬───┘
                      │Yes      │No
                      ▼         ▼
              ┌───────────┐ ┌──────────────┐
              │ 使用 FTA  │ │  使用 FEBM   │
              │ (模式匹配)│ │  (证据取证)  │
              └───────────┘ └──────────────┘
                      │            │
                      ▼            ▼
              ┌──────────────────────────┐
              │ FTA 找到根因？           │
              └────┬─────────────┬───────┘
                   │Yes          │No/Uncertain
                   ▼             ▼
           ┌──────────┐  ┌────────────────┐
           │ 修复问题 │  │ 切换到 FEBM    │
           │          │  │ (深度取证)     │
           └────┬─────┘  └────────┬───────┘
                │                 │
                ▼                 ▼
        ┌───────────────┐ ┌─────────────────┐
        │ 更新 FTA 库   │ │ 重建证据链      │
        │ (新增模式)    │ │ 发现新根因      │
        └───────────────┘ └─────────┬───────┘
                                    ▼
                            ┌───────────────┐
                            │ 创建新 FTA    │
                            │ 决策节点      │
                            └───────────────┘

使用指南:

1. 优先使用 FTA（快速响应）:
   - 告警匹配已知模式
   - 症状与历史故障相似
   - 需要快速恢复服务（MTTR 优先）
   - 例子: "Pod OOMKilled" → FTA 快速判断是内存泄漏还是配置不当

2. 切换到 FEBM（未知或复杂场景）:
   - FTA 无法明确判断根因
   - 多种可能根因需要证据排除
   - 需要建立完整证据链（合规/审计）
   - 例子: "服务间歇性超时" → FEBM 跨层关联网络/应用/系统证据

3. 联合使用（最佳实践）:
   - FTA 快速缩小根因范围
   - FEBM 验证 FTA 的判断
   - FEBM 发现的新模式反馈到 FTA
   - 建立持续改进循环
```

#<!-- chunk: 8.4.2 完整的联合诊断示例 -->## 8.4.2 完整的联合诊断示例

**场景**: 电商网站在促销期间出现大量 503 错误

##<!-- chunk: 第 1 阶段：FTA 快速筛查（0-5 分钟） -->## 第 1 阶段：FTA 快速筛查（0-5 分钟）

```bash
# 步骤 1: 收集基础症状
kubectl get pods -n production | grep -v "Running\|Completed"

# 输出:
# order-service-abc   0/1     CrashLoopBackOff   5  3m

# 步骤 2: 进入 FTA 决策树

FTA: Pod CrashLoopBackOff
   ├─ 检查退出码: 137 (SIGKILL)
   ├─ 检查日志: 无明显错误
   └─ 可能根因:
       ├─ OOMKilled (70% 概率)
       ├─ Liveness Probe 失败 (20%)
       └─ 其他 (10%)

# 步骤 3: 验证最可能的根因
kubectl describe pod order-service-abc | grep -i "oom"

# 输出:
# Last State: Terminated
#   Reason: OOMKilled

# FTA 结论: 确认是 OOMKilled，但需要进一步判断是内存泄漏还是流量突增
```

##<!-- chunk: 第 2 阶段：FEBM 深度取证（5-20 分钟） -->## 第 2 阶段：FEBM 深度取证（5-20 分钟）

```bash
# 步骤 4: 采集多层证据

# 证据 1: 内存使用趋势 (Prometheus)
curl -G 'http://localhost:9090/api/v1/query_range' \
  --data-urlencode 'query=container_memory_working_set_bytes{pod="order-service-abc"}' \
  --data-urlencode 'start=2024-02-01T14:00:00Z' \
  --data-urlencode 'end=2024-02-01T15:00:00Z' \
  --data-urlencode 'step=60s' | \
  jq '.data.result[0].values[] | @tsv'

# 分析: 内存从 500MB 在 10 分钟内飙升到 2GB（突发型，非线性增长）

# 证据 2: 流量指标
curl -G 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=rate(http_requests_total{service="order-service"}[5m])'

# 分析: QPS 从平时的 100 升至 1500（15 倍增长）→ 促销流量

# 证据 3: 应用日志
logcli query '{namespace="production",app="order-service"}' \
  --since=30m | grep -i "cache\|memory"

# 发现: 大量 "cache miss, fetching from database" 日志

# 证据 4: 配置审查
kubectl get deploy order-service -o yaml | yq '.spec.template.spec.containers[0].resources'

# 发现:
#   limits:
#     memory: "2Gi"
#   # 未配置 Redis 缓存连接池大小限制

# FEBM 结论: 
# 根因: 促销流量激增 + 缓存失效 → 大量数据库查询 → 内存中堆积未处理的结果集
```

##<!-- chunk: 第 3 阶段：根因确认和修复（20-30 分钟） -->## 第 3 阶段：根因确认和修复（20-30 分钟）

```bash
# 步骤 5: 关联证据形成证据链

证据链:
  [时间 T] 促销活动开始
      ↓
  [T+2min] HTTP 请求 QPS 从 100 → 1500 (Prometheus 指标)
      ↓
  [T+5min] Redis 缓存命中率从 95% → 30% (应用日志)
      ↓
  [T+8min] 数据库连接池耗尽 (应用日志: "wait for connection")
      ↓
  [T+10min] 应用内存飙升至 2GB → OOMKilled (Prometheus + K8s Event)
      ↓
  [T+10min] Pod 重启，循环重复 → CrashLoopBackOff

# 步骤 6: 实施修复

# 临时措施 (立即执行):
kubectl scale deploy order-service --replicas=10  # 水平扩容
kubectl set resources deploy order-service \
  --limits=memory=4Gi  # 临时增大内存

# 中期措施 (30 分钟内):
# 1. 预热 Redis 缓存
kubectl exec -it redis-0 -- redis-cli
> FLUSHALL  # 清理过期数据
> # 从数据库批量加载热点数据

# 2. 启用应用限流
kubectl set env deploy order-service RATE_LIMIT_QPS=500

# 长期措施 (后续优化):
# 1. 配置 HPA (Horizontal Pod Autoscaler)
# 2. 优化数据库查询（添加索引）
# 3. 实现多级缓存（本地缓存 + Redis）
# 4. 配置服务降级策略
```

##<!-- chunk: 第 4 阶段：反馈到 FTA（30+ 分钟） -->## 第 4 阶段：反馈到 FTA（30+ 分钟）

```yaml
# 更新 FTA 决策树

FTA 节点更新:
  OOMKilled 分支新增子节点:
    - 名称: "促销流量 + 缓存雪崩"
    - 识别特征:
        - 内存突发增长（非线性）
        - QPS 激增 > 5 倍
        - 缓存命中率骤降
        - 数据库连接池耗尽日志
    - 快速验证命令:
        ```bash
        # 1. 检查 QPS
        rate(http_requests_total{service="<SVC>"}[1m]) > 5 * rate(...[1h])
        
        # 2. 检查缓存
        logcli query '{app="<APP>"}' --since=5m | grep -c "cache miss"
        
        # 3. 检查内存趋势
        deriv(container_memory_working_set_bytes{pod="<POD>"}[5m]) > 100*1024*1024  # > 100MB/min
        ```
    - 标准处置:
        1. 立即扩容 (HPA 或手动)
        2. 限流保护
        3. 缓存预热
    - 预防措施:
        - 配置 HPA (CPU > 70% 或 QPS > 阈值)
        - 实现熔断降级
        - 定期缓存预热演练

FEBM Runbook 归档:
  - 标题: "促销流量导致 OOMKilled 完整取证"
  - 证据清单: [已保存]
  - 时间线: [已重建]
  - 根因: [已确认]
  - 处置方案: [已验证]
  - 状态: 已归档，可复用
```

#<!-- chunk: 8.4.3 联合诊断最佳实践 -->## 8.4.3 联合诊断最佳实践

```markdown
# FTA + FEBM 联合诊断最佳实践

<!-- chunk: 1. 建立快速响应 SOP -->## 1. 建立快速响应 SOP

**0-5 分钟（FTA 阶段）**:
- [ ] 收集基础症状（pod 状态、日志最后 20 行、退出码）
- [ ] 匹配 FTA 决策树，找到最可能的 2-3 个根因
- [ ] 执行快速验证命令（每个根因 < 1 分钟）
- [ ] 如果根因明确 → 执行标准处置 → 结束
- [ ] 如果根因不明 → 启动 FEBM 取证

**5-30 分钟（FEBM 阶段）**:
- [ ] 根据 FTA 缩小的范围，采集针对性证据
- [ ] 重建事件时间线（精确到分钟）
- [ ] 跨层关联证据（应用+网络+系统）
- [ ] 形成完整证据链
- [ ] 确认根因并实施修复

**30+ 分钟（反馈阶段）**:
- [ ] 将新发现的模式添加到 FTA
- [ ] 更新 FEBM Runbook 库
- [ ] 编写事后分析报告（Postmortem）
- [ ] 识别预防措施并跟踪实施

<!-- chunk: 2. 证据采集优先级 -->## 2. 证据采集优先级

**P0 (必须采集)**:
- Pod/Node 状态快照
- 最近 50 行日志（当前和 previous）
- 退出码和错误消息
- 事件历史（kubectl get events）

**P1 (高价值)**:
- Prometheus 指标趋势（故障前后 1 小时）
- K8s 审计日志（配置变更）
- 网络连通性测试
- 资源使用率（CPU/内存/磁盘）

**P2 (深度分析)**:
- 分布式追踪 (Jaeger/Zipkin)
- eBPF 网络流分析
- Heap dump / pprof
- Kernel 日志

<!-- chunk: 3. 工具选择矩阵 -->## 3. 工具选择矩阵

| 故障类型             | 首选 FTA | 首选 FEBM | 联合使用 |
|---------------------|----------|-----------|----------|
| Pod CrashLoop       | ✓        |           |          |
| OOMKilled           | ✓        |           | ✓        |
| 服务间歇性超时       |          | ✓         | ✓        |
| 配置漂移            |          | ✓         |          |
| 证书过期            | ✓        |           |          |
| 未知性能下降         |          | ✓         | ✓        |
| 安全事件            |          | ✓         |          |

<!-- chunk: 4. 文档和知识库维护 -->## 4. 文档和知识库维护

**FTA 更新触发条件**:
- 发现新的故障模式（每季度至少 1 次）
- FEBM 取证揭示的根因 FTA 未覆盖
- 现有 FTA 节点的准确率 < 80%

**FEBM Runbook 更新触发条件**:
- 新类型故障首次发生
- 新工具/命令加入工具栈
- 合规要求变更

**知识共享机制**:
- 每次重大故障后 48 小时内完成 Postmortem
- 每月团队分享会，回顾典型案例
- 维护"故障知识图谱"（故障→根因→证据的关联关系）
```

---

<!-- chunk: 8.5 FEBM 效果度量 KPI 仪表板 -->## 8.5 FEBM 效果度量 KPI 仪表板

#<!-- chunk: 8.5.1 核心 KPI 定义 -->## 8.5.1 核心 KPI 定义

```yaml
FEBM 成熟度 KPI 体系:

1. 响应效率指标 (Response Efficiency Metrics):
   
   MTTR (Mean Time To Repair):
     定义: 从故障发生到完全恢复的平均时间
     目标值:
       - 基础级 (L1): < 60 分钟
       - 进阶级 (L2): < 30 分钟
       - 专家级 (L3): < 15 分钟
     计算公式: |
       sum(故障恢复时间) / count(故障次数)
     Prometheus 查询: |
       avg_over_time(
         (time() - alert_start_time{severity="critical"})
         [24h]
       )
   
   MTTD (Mean Time To Detect):
     定义: 从故障发生到被检测到的平均时间
     目标值: < 2 分钟
     Prometheus 查询: |
       histogram_quantile(0.95,
         alert_detection_latency_seconds_bucket
       )
   
   MTTI (Mean Time To Investigate):
     定义: 从故障检测到确认根因的平均时间
     目标值: < 10 分钟
     计算: MTTR - MTTD - MTTF (Mean Time To Fix)

2. 证据质量指标 (Evidence Quality Metrics):
   
   证据完整度 (Evidence Completeness):
     定义: 实际采集的证据类型 / 标准 Runbook 要求的证据类型
     目标值: > 90%
     计算公式: |
       (采集到的证据项数 / Runbook 定义的证据项数) × 100%
     示例:
       Runbook 要求 10 项证据 (日志、指标、事件等)
       实际采集 9 项 → 完整度 = 90%
   
   证据时间精度 (Evidence Temporal Precision):
     定义: 证据时间戳的时钟偏差
     目标值: < 1 秒
     Prometheus 查询: |
       abs(node_time_seconds - node_ntp_time_seconds)
   
   证据关联度 (Evidence Correlation Rate):
     定义: 成功跨层关联的证据对 / 总证据对
     目标值: > 80%
     计算: 通过时间戳±5s 窗口匹配的证据对数量

3. 诊断准确性指标 (Diagnostic Accuracy Metrics):
   
   根因判断准确率 (Root Cause Accuracy):
     定义: 首次判断正确的根因 / 总故障数
     目标值: > 85%
     计算: |
       (首次诊断正确次数 / 总故障次数) × 100%
   
   误报率 (False Positive Rate):
     定义: 误触发的告警 / 总告警数
     目标值: < 5%
     Prometheus 查询: |
       sum(alertmanager_notifications_failed_total) /
       sum(alertmanager_notifications_total)
   
   漏报率 (False Negative Rate):
     定义: 未被检测到的实际故障 / 实际故障总数
     目标值: < 2%
     计算: 通过用户报告发现的故障 / 全部故障

4. 自动化程度指标 (Automation Metrics):
   
   自动取证率 (Auto-Forensics Rate):
     定义: 自动执行 Runbook 的故障 / 总故障数
     目标值: > 70%
     计算: |
       (自动触发脚本的故障数 / 总故障数) × 100%
   
   自动修复率 (Auto-Remediation Rate):
     定义: 无需人工干预自动恢复的故障 / 总故障数
     目标值: > 30% (L2), > 50% (L3)
     示例: HPA 自动扩容、Pod 自动重启
   
   Runbook 覆盖率 (Runbook Coverage):
     定义: 有标准 Runbook 的故障类型 / 已知故障类型
     目标值: > 95%
     计算: count(distinct runbook_id) / count(distinct incident_type)
```

#<!-- chunk: 8.5.2 Grafana 仪表板配置 -->## 8.5.2 Grafana 仪表板配置

```json
{
  "dashboard": {
    "title": "FEBM 效果度量仪表板",
    "tags": ["febm", "sre", "metrics"],
    "timezone": "browser",
    "panels": [
      {
        "id": 1,
        "title": "MTTR 趋势 (过去 30 天)",
        "type": "graph",
        "targets": [
          {
            "expr": "avg_over_time(incident_resolution_duration_seconds{severity=\"critical\"}[24h]) / 60",
            "legendFormat": "平均 MTTR (分钟)",
            "refId": "A"
          },
          {
            "expr": "60",
            "legendFormat": "目标线 (60 分钟)",
            "refId": "B"
          }
        ],
        "yaxes": [
          {
            "format": "m",
            "label": "时间 (分钟)"
          }
        ],
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 0}
      },
      {
        "id": 2,
        "title": "MTTR 分解 (当前周)",
        "type": "barchart",
        "targets": [
          {
            "expr": "avg(alert_detection_latency_seconds) / 60",
            "legendFormat": "检测时间 (MTTD)",
            "refId": "A"
          },
          {
            "expr": "avg(incident_investigation_duration_seconds) / 60",
            "legendFormat": "调查时间 (MTTI)",
            "refId": "B"
          },
          {
            "expr": "avg(incident_fix_duration_seconds) / 60",
            "legendFormat": "修复时间 (MTTF)",
            "refId": "C"
          }
        ],
        "yaxes": [
          {
            "format": "m",
            "label": "时间 (分钟)"
          }
        ],
        "gridPos": {"h": 8, "w": 12, "x": 12, "y": 0}
      },
      {
        "id": 3,
        "title": "证据完整度 (按故障类型)",
        "type": "table",
        "targets": [
          {
            "expr": "(sum(evidence_collected_count) by (incident_type) / sum(evidence_required_count) by (incident_type)) * 100",
            "format": "table",
            "refId": "A"
          }
        ],
        "transformations": [
          {
            "id": "organize",
            "options": {
              "excludeByName": {},
              "indexByName": {},
              "renameByName": {
                "incident_type": "故障类型",
                "Value": "完整度 (%)"
              }
            }
          }
        ],
        "gridPos": {"h": 8, "w": 8, "x": 0, "y": 8}
      },
      {
        "id": 4,
        "title": "根因判断准确率 (月度)",
        "type": "gauge",
        "targets": [
          {
            "expr": "(sum(incident_root_cause_correct_total) / sum(incident_total)) * 100",
            "refId": "A"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "thresholds": {
              "mode": "absolute",
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 70, "color": "yellow"},
                {"value": 85, "color": "green"}
              ]
            },
            "unit": "percent",
            "min": 0,
            "max": 100
          }
        },
        "gridPos": {"h": 8, "w": 8, "x": 8, "y": 8}
      },
      {
        "id": 5,
        "title": "告警误报率",
        "type": "stat",
        "targets": [
          {
            "expr": "(sum(rate(alertmanager_notifications_failed_total[24h])) / sum(rate(alertmanager_notifications_total[24h]))) * 100",
            "refId": "A"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "thresholds": {
              "steps": [
                {"value": 0, "color": "green"},
                {"value": 5, "color": "yellow"},
                {"value": 10, "color": "red"}
              ]
            },
            "unit": "percent"
          }
        },
        "gridPos": {"h": 8, "w": 8, "x": 16, "y": 8}
      },
      {
        "id": 6,
        "title": "自动化程度仪表",
        "type": "piechart",
        "targets": [
          {
            "expr": "sum(incident_auto_resolved_total)",
            "legendFormat": "自动修复",
            "refId": "A"
          },
          {
            "expr": "sum(incident_auto_forensics_total) - sum(incident_auto_resolved_total)",
            "legendFormat": "自动取证+人工修复",
            "refId": "B"
          },
          {
            "expr": "sum(incident_total) - sum(incident_auto_forensics_total)",
            "legendFormat": "完全人工",
            "refId": "C"
          }
        ],
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 16}
      },
      {
        "id": 7,
        "title": "Runbook 覆盖率",
        "type": "stat",
        "targets": [
          {
            "expr": "(count(count by (runbook_id) (incident_runbook_usage)) / count(count by (incident_type) (incident_total))) * 100",
            "refId": "A"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 80, "color": "yellow"},
                {"value": 95, "color": "green"}
              ]
            },
            "unit": "percent"
          }
        },
        "gridPos": {"h": 8, "w": 12, "x": 12, "y": 16}
      },
      {
        "id": 8,
        "title": "故障类型分布 (Top 10)",
        "type": "bargauge",
        "targets": [
          {
            "expr": "topk(10, sum(increase(incident_total[30d])) by (incident_type))",
            "legendFormat": "{{ incident_type }}",
            "refId": "A"
          }
        ],
        "gridPos": {"h": 8, "w": 24, "x": 0, "y": 24}
      }
    ],
    "refresh": "5m",
    "time": {
      "from": "now-30d",
      "to": "now"
    }
  }
}
```

#<!-- chunk: 8.5.3 关键 PromQL 查询 -->## 8.5.3 关键 PromQL 查询

```promql
# 1. MTTR 计算 (从告警触发到恢复的时间)
# 需要自定义指标记录告警开始和结束时间
avg_over_time(
  (
    alert_resolved_timestamp - alert_fired_timestamp
  )[24h:1h]
) / 60  # 转换为分钟

# 2. 证据采集完整度
# 需要在取证脚本中暴露指标
(
  sum(evidence_collected_count{runbook="pod-oomkilled"}) 
  / 
  sum(evidence_required_count{runbook="pod-oomkilled"})
) * 100

# 3. 根因判断准确率 (需要人工标注)
# 在事后分析中记录 root_cause_correct{incident_id="xxx"} = 1 或 0
sum(increase(root_cause_correct{result="1"}[30d])) 
/ 
sum(increase(root_cause_correct[30d])) 
* 100

# 4. 自动取证触发率
sum(rate(runbook_auto_executed_total[24h])) 
/ 
sum(rate(incident_total[24h])) 
* 100

# 5. 告警噪音比 (告警数 vs 真实故障数)
sum(increase(alertmanager_alerts_received_total[24h])) 
/ 
sum(increase(incident_confirmed_total[24h]))

# 6. 证据时钟偏差分布
histogram_quantile(0.95, 
  sum(rate(evidence_timestamp_skew_seconds_bucket[5m])) by (le)
)

# 7. 平均证据采集时间
avg(evidence_collection_duration_seconds) by (evidence_type)

# 8. Runbook 执行成功率
sum(rate(runbook_execution_success_total[24h])) 
/ 
sum(rate(runbook_execution_total[24h])) 
* 100

# 9. 跨层证据关联成功率
sum(evidence_correlation_success_total) 
/ 
sum(evidence_correlation_attempts_total) 
* 100

# 10. MTTR 改善趋势 (与上月对比)
(
  avg_over_time(incident_resolution_duration_seconds[30d] offset 30d) 
  - 
  avg_over_time(incident_resolution_duration_seconds[30d])
) / 60  # 正值表示改善
```

#<!-- chunk: 8.5.4 月度报告模板 -->## 8.5.4 月度报告模板

```markdown
# FEBM 效果月度报告

**报告周期**: 2024-02-01 ~ 2024-02-29  
**报告生成**: 2024-03-01  
**报告人**: SRE Team

---

<!-- chunk: 1. 执行摘要 -->## 1. 执行摘要

#<!-- chunk: 关键成果 -->## 关键成果
- ✅ MTTR 从 45 分钟降至 32 分钟 (降低 29%)
- ✅ 自动取证率达到 75% (目标 70%)
- ⚠️ 根因判断准确率 82% (未达标，目标 85%)
- ❌ 新增 3 类故障未覆盖 Runbook

#<!-- chunk: 故障概览 -->## 故障概览
- 总故障数: 28 起
  - P0 (Critical): 3 起
  - P1 (High): 12 起
  - P2 (Medium): 13 起
- 总影响时间: 896 分钟
- 平均 MTTR: 32 分钟

---

<!-- chunk: 2. KPI 详细分析 -->## 2. KPI 详细分析

#<!-- chunk: 2.1 响应效率指标 -->## 2.1 响应效率指标

| 指标 | 本月 | 上月 | 目标 | 达标 | 趋势 |
|------|------|------|------|------|------|
| MTTR | 32min | 45min | <30min | ⚠️ | ⬇️ 29% |
| MTTD | 1.5min | 2.1min | <2min | ✅ | ⬇️ 29% |
| MTTI | 18min | 28min | <10min | ⚠️ | ⬇️ 36% |
| MTTF | 12.5min | 14.9min | N/A | N/A | ⬇️ 16% |

**分析**:
- MTTR 显著改善但未达标，主要瓶颈在 MTTI (调查时间)
- 建议: 增加自动化根因分析脚本，减少人工判断时间

#<!-- chunk: 2.2 证据质量指标 -->## 2.2 证据质量指标

| 指标 | 本月 | 上月 | 目标 | 达标 |
|------|------|------|------|------|
| 证据完整度 | 92% | 87% | >90% | ✅ |
| 证据时间精度 | 0.3s | 0.5s | <1s | ✅ |
| 证据关联度 | 85% | 78% | >80% | ✅ |

**分析**:
- 证据质量全面达标，NTP 同步优化见效
- 仍有 8% 的证据采集不完整，主因是节点权限不足

#<!-- chunk: 2.3 诊断准确性指标 -->## 2.3 诊断准确性指标

| 指标 | 本月 | 上月 | 目标 | 达标 |
|------|------|------|------|------|
| 根因判断准确率 | 82% | 75% | >85% | ❌ |
| 误报率 | 3.2% | 5.1% | <5% | ✅ |
| 漏报率 | 1.8% | 2.5% | <2% | ✅ |

**分析**:
- 根因判断准确率未达标，主要失误案例:
  1. 2月5日: 误判为内存泄漏，实际是配置漂移
  2. 2月18日: 未识别证书过期前兆
- 改进措施: 更新相关 Runbook，增加证书监控

#<!-- chunk: 2.4 自动化程度指标 -->## 2.4 自动化程度指标

| 指标 | 本月 | 上月 | 目标 | 达标 |
|------|------|------|------|------|
| 自动取证率 | 75% | 68% | >70% | ✅ |
| 自动修复率 | 28% | 22% | >30% | ⚠️ |
| Runbook 覆盖率 | 91% | 88% | >95% | ⚠️ |

**分析**:
- 自动取证率达标，脚本化效果显著
- 自动修复率接近目标，HPA 和自动重启占主要比例
- Runbook 覆盖率不足，新增 3 类故障需补充 Runbook

---

<!-- chunk: 3. 典型案例分析 -->## 3. 典型案例分析

#<!-- chunk: 案例 1: 促销流量导致 OOMKilled (P0) -->## 案例 1: 促销流量导致 OOMKilled (P0)

**事件时间**: 2024-02-14 14:23  
**影响范围**: order-service 不可用 25 分钟  
**MTTR**: 25 分钟  

**时间线**:
- 14:23: Prometheus 触发 PodOOMKilled 告警
- 14:24: 自动执行 oom-forensics.sh 脚本
- 14:28: FEBM 确认根因: 促销流量 + 缓存雪崩
- 14:30: 手动扩容至 10 副本
- 14:35: 启用限流策略
- 14:48: 服务完全恢复

**FEBM 应用**:
- ✅ 证据完整度: 95% (10/10 项证据采集)
- ✅ 跨层关联: 成功关联流量、缓存、内存三层证据
- ✅ 根因判断: 准确 (人工验证)

**改进措施**:
- 更新 FTA 决策树，增加"缓存雪崩"节点
- 配置 HPA 自动扩容 (避免人工介入)
- 实施缓存预热定时任务

---

#<!-- chunk: 案例 2: 配置漂移导致静默失败 (P1) -->## 案例 2: 配置漂移导致静默失败 (P1)

**事件时间**: 2024-02-05 09:15  
**影响范围**: payment-service 支付成功率下降至 60%  
**MTTR**: 48 分钟  

**时间线**:
- 09:15: 用户报告支付失败
- 09:20: 查看日志无明显错误
- 09:35: FTA 判断为"数据库连接问题" (误判)
- 09:40: 数据库检查正常，切换到 FEBM
- 09:55: 审计日志发现 ConfigMap 被手动修改
- 10:00: 恢复配置，服务恢复
- 10:03: 服务完全恢复

**FEBM 应用**:
- ⚠️ 初始误判: FTA 未识别"配置漂移"模式
- ✅ FEBM 纠正: 审计日志找到根因
- ✅ 证据链: ConfigMap 变更 → 应用行为异常

**改进措施**:
- 启用 ArgoCD GitOps (防止手动修改)
- 配置 OPA Gatekeeper 策略
- 更新 FTA 增加"静默失败"分支

---

<!-- chunk: 4. Runbook 更新记录 -->## 4. Runbook 更新记录

本月新增 Runbook:
1. **配置漂移检测与恢复** (config-drift-detection.md)
2. **证书过期应急处置** (certificate-expiry-response.md)
3. **DNS 解析间歇性失败** (dns-intermittent-failure.md)

本月更新 Runbook:
1. **Pod OOMKilled** - 增加"缓存雪崩"子场景
2. **Service 超时** - 补充 eBPF 网络分析命令
3. **Node NotReady** - 增加 containerd 故障排查

---

<!-- chunk: 5. 下月行动计划 -->## 5. 下月行动计划

#<!-- chunk: 目标 -->## 目标
- [ ] MTTR 降至 < 30 分钟
- [ ] 根因判断准确率提升至 > 85%
- [ ] 自动修复率提升至 > 35%
- [ ] Runbook 覆盖率达到 > 95%

#<!-- chunk: 具体措施 -->## 具体措施
1. **自动化增强**:
   - 开发根因自动推理引擎 (基于历史数据和 ML)
   - 配置更多自愈场景 (HPA, Pod 自动替换)
   
2. **工具优化**:
   - 部署 eBPF 网络分析工具 (Pixie)
   - 启用分布式追踪 (Jaeger)
   
3. **能力建设**:
   - 组织 FEBM 内部培训 (2 次)
   - 定期故障演练 (每月 1 次)
   
4. **流程改进**:
   - 建立 Runbook 定期审查机制 (每季度)
   - 实施 GitOps 全面覆盖

---

<!-- chunk: 6. 资源投入 -->## 6. 资源投入

本月 FEBM 工具栈资源使用:
- 计算: 2.3 Cores (目标 < 3 Cores) ✅
- 内存: 5.8 GB (目标 < 6 GB) ✅
- 存储: 178 GB (目标 < 200 GB) ✅
- 成本: $82/月 (目标 < $100/月) ✅

---

<!-- chunk: 7. 附录 -->## 7. 附录

#<!-- chunk: 附录 A: 本月所有故障清单 -->## 附录 A: 本月所有故障清单
(省略详细列表...)

#<!-- chunk: 附录 B: 证据采集质量抽查 -->## 附录 B: 证据采集质量抽查
(省略详细数据...)

#<!-- chunk: 附录 C: 下月 OKR -->## 附录 C: 下月 OKR
- Objective 1: 提升故障响应速度
  - KR1: MTTR < 30min (当前 32min)
  - KR2: 自动取证率 > 80% (当前 75%)
  
- Objective 2: 提高诊断准确性
  - KR1: 根因判断准确率 > 85% (当前 82%)
  - KR2: 误报率 < 3% (当前 3.2%)
  
- Objective 3: 完善知识库
  - KR1: Runbook 覆盖率 > 95% (当前 91%)
  - KR2: 每个 Runbook 至少验证 1 次

---

**审批**: [SRE Lead 签名]  
**分发**: SRE Team, Dev Team, Management
```

---

<!-- chunk: 8.6 合规快速参考 -->## 8.6 合规快速参考

#<!-- chunk: 8.6.1 等保 2.0 合规映射 -->## 8.6.1 等保 2.0 合规映射

```yaml
等保 2.0 (GB/T 22239-2019) FEBM 控制映射:

1. 安全审计 (第三级要求):
   
   要求 8.1.3.2 - 审计记录:
     描述: |
       应启用安全审计功能，审计覆盖到每个用户，
       对重要的用户行为和重要安全事件进行审计
     FEBM 实现:
       - K8s Audit Log 记录所有 API 操作
       - Falco 记录运行时安全事件
       - 日志级别: RequestResponse (元数据 + 请求/响应体)
     验证命令:
       sudo ls -lh /var/log/kubernetes/audit.log
       kubectl logs -n falco -l app.kubernetes.io/name=falco
   
   要求 8.1.3.3 - 审计记录保护:
     描述: |
       审计记录应包括日期、时间、用户、事件类型、
       事件是否成功及其他与审计相关的信息
     FEBM 实现:
       - 审计日志包含: timestamp, user, verb, objectRef, responseStatus
       - Loki 存储审计日志，防篡改
     验证命令:
       logcli query '{job="audit"}' --since=1h | jq '.[].timestamp, .user, .verb'
   
   要求 8.1.3.4 - 审计记录存储:
     描述: |
       审计记录的保存时间应不少于六个月，
       对于法律法规要求的保存时间应不少于该要求
     FEBM 实现:
       - Loki 配置保留期 ≥ 6 个月
       - 可选: 自动归档到对象存储 (S3/OSS)
     配置示例:
       loki:
         table_manager:
           retention_deletes_enabled: true
           retention_period: 4320h  # 180 天

2. 入侵防范 (8.1.4):
   
   要求 8.1.4.2 - 恶意代码防范:
     描述: 应采用免受恶意代码攻击的技术措施
     FEBM 实现:
       - Falco 检测异常进程和文件访问
       - 规则: Terminal shell in container, Suspicious network tool
     验证命令:
       kubectl logs -n falco -l app.kubernetes.io/name=falco | \
         grep -i "suspicious\|malicious"
   
   要求 8.1.4.3 - 可信验证:
     描述: 应能够检测到对重要程序的修改
     FEBM 实现:
       - Falco 监控 /bin, /usr/bin 等关键目录的写入
       - 规则: Write below binary dir, Write below etc
     Falco 规则示例:
       - rule: Write below binary dir
         condition: >
           bin_dir and evt.dir = < and open_write
         output: "File below a known binary directory opened for writing..."

3. 安全计算环境 - 身份鉴别 (8.1.1):
   
   要求 8.1.1.1 - 身份鉴别:
     描述: 应对登录的用户进行身份标识和鉴别
     FEBM 实现:
       - K8s RBAC 强制身份验证
       - 审计日志记录所有用户操作和身份信息
     验证命令:
       # 检查所有非匿名用户的操作
       logcli query '{job="audit"}' --since=1h | \
         jq 'select(.user.username != "system:anonymous")'

4. 安全管理中心 - 集中管控 (8.1.5):
   
   要求 8.1.5.1 - 集中管控:
     描述: 应对安全策略、恶意代码、补丁升级等进行集中管理
     FEBM 实现:
       - Grafana 集中展示所有安全事件
       - Prometheus 监控所有组件健康状态
       - Falcosidekick 集中路由告警
     访问地址:
       Grafana: https://grafana.example.com/d/febm-security
```

#<!-- chunk: 8.6.2 SOC 2 Type II 合规映射 -->## 8.6.2 SOC 2 Type II 合规映射

```yaml
SOC 2 Trust Service Criteria - FEBM 控制映射:

1. CC7.2 - 系统监控 (Monitoring):
   
   控制要求:
     描述: |
       实体应监控系统组件和系统运行情况，
       以检测异常和指示风险的情况
   FEBM 实现:
     - Prometheus 持续监控所有组件指标
     - Falco 实时检测异常系统调用
     - Alertmanager 自动触发告警
   证据:
     - Prometheus 指标保留 15 天
     - Falco 告警日志存储在 Loki
     - 告警路由记录 (Slack/Webhook)
   审计师验证方法:
     1. 访问 Grafana 查看监控仪表板
     2. 抽查最近 30 天的告警记录
     3. 验证告警响应时间 < SLA

2. CC7.3 - 威胁和漏洞评估 (Threat and Vulnerability Evaluation):
   
   控制要求:
     描述: 实体应识别、评估和管理威胁和漏洞
   FEBM 实现:
     - Falco 规则库定期更新 (每月)
     - 容器镜像扫描 (Trivy/Grype)
     - CVE 监控和修复跟踪
   证据:
     - Falco 规则版本历史
     - 镜像扫描报告
     - 漏洞修复记录
   审计师验证方法:
     kubectl get cm -n falco falco-rules -o yaml | grep "version:"
     trivy image <IMAGE> --format json

3. CC7.4 - 安全事件响应 (Security Incident Response):
   
   控制要求:
     描述: |
       实体应建立安全事件响应流程，
       包括检测、报告、评估、响应和恢复
   FEBM 实现:
     - 标准化 Runbook (检测 → 取证 → 修复)
     - 自动化取证脚本
     - 事后分析报告 (Postmortem)
   证据:
     - Runbook 文档库
     - 取证脚本执行日志
     - Postmortem 归档
   审计师验证方法:
     1. 抽查 5 起安全事件的响应记录
     2. 验证每起事件都有完整的取证证据
     3. 检查 MTTR 是否符合 SLA

4. A1.2 - 访问控制 (Access Control):
   
   控制要求:
     描述: 实体应授权、修改或删除访问权限
   FEBM 实现:
     - K8s RBAC 控制集群访问
     - 审计日志记录所有权限变更
     - ServiceAccount 最小权限原则
   证据:
     - RBAC 策略配置
     - 权限变更审计日志
     - 定期权限审查记录
   审计师验证方法:
     kubectl get clusterrolebindings -o yaml
     logcli query '{job="audit"}' | grep "rolebindings\|clusterrolebindings"

5. CC8.1 - 变更管理 (Change Management):
   
   控制要求:
     描述: 实体应授权、设计、测试和批准系统变更
   FEBM 实现:
     - GitOps 工作流 (所有变更通过 Git)
     - ArgoCD 自动同步和审计
     - 配置漂移检测和告警
   证据:
     - Git 提交历史
     - ArgoCD 同步日志
     - 配置漂移检测报告
   审计师验证方法:
     1. 抽查 10 次配置变更的 Git 历史
     2. 验证每次变更都有 PR 审批
     3. 检查配置漂移告警机制是否有效
```

#<!-- chunk: 8.6.3 合规检查清单 -->## 8.6.3 合规检查清单

```bash
#!/bin/bash
# compliance-check.sh - FEBM 合规快速检查脚本

echo "=== FEBM 合规检查工具 ==="
echo "支持标准: 等保 2.0 (三级), SOC 2 Type II"
echo ""

# 1. 审计日志检查
echo "<!-- chunk: 1. 审计日志合规性" -->## 1. 审计日志合规性"
echo "---"

# 检查审计日志是否启用
if [ -f /var/log/kubernetes/audit.log ]; then
    echo "✅ K8s 审计日志已启用"
    
    # 检查日志保留期
    OLDEST_LOG=$(sudo find /var/log/kubernetes -name "audit.log*" -type f -printf '%T@ %p\n' | sort -n | head -1 | awk '{print $2}')
    OLDEST_DATE=$(stat -c %y "$OLDEST_LOG" 2>/dev/null | cut -d' ' -f1)
    DAYS_KEPT=$(( ($(date +%s) - $(date -d "$OLDEST_DATE" +%s)) / 86400 ))
    
    if [ $DAYS_KEPT -ge 180 ]; then
        echo "✅ 审计日志保留期: $DAYS_KEPT 天 (≥ 180 天，满足等保 2.0)"
    else
        echo "❌ 审计日志保留期: $DAYS_KEPT 天 (< 180 天，不满足等保 2.0)"
    fi
else
    echo "❌ K8s 审计日志未启用"
fi

# 检查 Falco 日志
if kubectl get pods -n falco -l app.kubernetes.io/name=falco 2>/dev/null | grep -q "Running"; then
    echo "✅ Falco 运行时审计已启用"
else
    echo "❌ Falco 未部署或未运行"
fi

echo ""

# 2. 日志完整性检查
echo "<!-- chunk: 2. 日志完整性" -->## 2. 日志完整性"
echo "---"

# 检查审计日志字段完整性
SAMPLE_LOG=$(sudo tail -1 /var/log/kubernetes/audit.log 2>/dev/null)
if echo "$SAMPLE_LOG" | jq -e '.requestReceivedTimestamp, .user.username, .verb, .objectRef, .responseStatus' > /dev/null 2>&1; then
    echo "✅ 审计日志包含必需字段 (timestamp, user, verb, object, status)"
else
    echo "❌ 审计日志字段不完整"
fi

echo ""

# 3. 监控覆盖率检查
echo "<!-- chunk: 3. 监控覆盖率" -->## 3. 监控覆盖率"
echo "---"

# 检查 Prometheus 是否运行
if kubectl get pods -n monitoring -l app.kubernetes.io/name=prometheus 2>/dev/null | grep -q "Running"; then
    echo "✅ Prometheus 监控已启用"
    
    # 检查监控目标数量
    TARGETS=$(curl -s http://localhost:9090/api/v1/targets 2>/dev/null | jq '.data.activeTargets | length')
    echo "   监控目标数: $TARGETS"
else
    echo "❌ Prometheus 未部署或未运行"
fi

# 检查 Grafana 是否运行
if kubectl get pods -n monitoring -l app.kubernetes.io/name=grafana 2>/dev/null | grep -q "Running"; then
    echo "✅ Grafana 可视化已启用"
else
    echo "❌ Grafana 未部署或未运行"
fi

echo ""

# 4. 访问控制检查
echo "<!-- chunk: 4. 访问控制" -->## 4. 访问控制"
echo "---"

# 检查匿名访问是否禁用
if kubectl get cm -n kube-system kube-apiserver -o yaml 2>/dev/null | grep -q "anonymous-auth=false"; then
    echo "✅ API Server 匿名访问已禁用"
else
    echo "⚠️  API Server 可能允许匿名访问"
fi

# 检查 RBAC 是否启用
if kubectl api-versions | grep -q "rbac.authorization.k8s.io"; then
    echo "✅ RBAC 已启用"
    
    # 统计 ClusterRoleBinding 数量
    CRB_COUNT=$(kubectl get clusterrolebindings --no-headers 2>/dev/null | wc -l)
    echo "   ClusterRoleBindings: $CRB_COUNT"
else
    echo "❌ RBAC 未启用"
fi

echo ""

# 5. 变更管理检查
echo "<!-- chunk: 5. 变更管理 (GitOps)" -->## 5. 变更管理 (GitOps)"
echo "---"

# 检查 ArgoCD 是否部署
if kubectl get pods -n argocd 2>/dev/null | grep -q "argocd-server"; then
    echo "✅ ArgoCD GitOps 已部署"
    
    # 检查应用同步状态
    SYNCED=$(kubectl get applications -n argocd -o json 2>/dev/null | jq '[.items[] | select(.status.sync.status=="Synced")] | length')
    TOTAL=$(kubectl get applications -n argocd --no-headers 2>/dev/null | wc -l)
    echo "   同步状态: $SYNCED/$TOTAL 个应用已同步"
else
    echo "⚠️  ArgoCD 未部署 (建议部署以满足变更管理要求)"
fi

echo ""

# 6. 事件响应能力检查
echo "<!-- chunk: 6. 事件响应能力" -->## 6. 事件响应能力"
echo "---"

# 检查 Runbook 数量
RUNBOOK_DIR="./runbooks"
if [ -d "$RUNBOOK_DIR" ]; then
    RUNBOOK_COUNT=$(find "$RUNBOOK_DIR" -name "*.md" -o -name "*.sh" | wc -l)
    echo "✅ Runbook 文档: $RUNBOOK_COUNT 个"
else
    echo "⚠️  Runbook 目录不存在: $RUNBOOK_DIR"
fi

# 检查告警路由配置
if kubectl get svc -n falco falco-falcosidekick 2>/dev/null | grep -q "falcosidekick"; then
    echo "✅ Falcosidekick 告警路由已配置"
else
    echo "❌ Falcosidekick 未配置"
fi

echo ""

# 7. 生成合规报告摘要
echo "<!-- chunk: 7. 合规摘要" -->## 7. 合规摘要"
echo "---"

cat <<'SUMMARY'
┌─────────────────────────────────────────────────────────────────┐
│                      等保 2.0 (三级) 合规状态                    │
├─────────────────────────────────────────────────────────────────┤
│ ✅ 安全审计 (8.1.3): 审计日志启用，保留期 ≥ 6 个月             │
│ ✅ 入侵防范 (8.1.4): Falco 实时监控异常行为                     │
│ ✅ 身份鉴别 (8.1.1): RBAC 强制身份验证                          │
│ ✅ 集中管控 (8.1.5): Grafana 集中监控和告警                     │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    SOC 2 Type II 合规状态                        │
├─────────────────────────────────────────────────────────────────┤
│ ✅ CC7.2 系统监控: Prometheus + Grafana 持续监控                │
│ ✅ CC7.3 威胁评估: Falco 规则库定期更新                         │
│ ✅ CC7.4 事件响应: 标准化 Runbook 和自动化取证                  │
│ ✅ A1.2 访问控制: RBAC + 审计日志                               │
│ ⚠️  CC8.1 变更管理: 建议部署 ArgoCD GitOps                      │
└─────────────────────────────────────────────────────────────────┘

审计建议:
1. 定期导出审计日志到不可变存储 (S3/OSS)
2. 每季度进行合规审查和渗透测试
3. 保持 Falco 规则库更新 (订阅官方更新)
4. 实施 GitOps 以满足变更管理要求
5. 定期演练事件响应流程
SUMMARY

echo ""
echo "=== 合规检查完成 ==="
echo "报告生成时间: $(date)"
```

---

<!-- chunk: 8.7 章节总结与导航 -->## 8.7 章节总结与导航

#<!-- chunk: 本章要点回顾 -->## 本章要点回顾

```
┌─────────────────────────────────────────────────────────────────┐
│        第八章：FEBM 生产环境快速启动与 K8s 故障取证手册        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  8.1 FEBM 第一周行动清单                                        │
│      ├─ Day 1: 部署 Falco (运行时安全监控)                      │
│      ├─ Day 2: 启用 K8s 审计日志                                │
│      ├─ Day 3: 部署 Loki + Fluent Bit (日志聚合)               │
│      ├─ Day 4: 部署 Prometheus + Grafana (监控)                │
│      ├─ Day 5: 配置 Falcosidekick (告警路由)                    │
│      └─ Day 6-7: 端到端验证和性能调优                           │
│                                                                  │
│  8.2 最小化 FEBM 工具栈部署                                     │
│      ├─ 一键部署脚本 (deploy-febm-minimal.sh)                  │
│      ├─ 资源开销明细 (~5GB 内存, ~160GB 存储)                  │
│      └─ NTP 时钟同步验证 (关键！)                               │
│                                                                  │
│  8.3 Kubernetes 常见故障 FEBM 取证 Runbook                      │
│      ├─ 8.3.1 Pod OOMKilled (内存泄漏 vs 配置不当)             │
│      ├─ 8.3.2 Pod CrashLoopBackOff (8 种常见根因)              │
│      ├─ 8.3.3 Node NotReady (kubelet/网络/资源)                │
│      ├─ 8.3.4 Service 间歇性超时 (跨层证据关联)                │
│      ├─ 8.3.5 证书过期 (证书链验证+自动续期)                    │
│      └─ 8.3.6 配置漂移 (GitOps 防护)                            │
│                                                                  │
│  8.4 FTA + FEBM 联合诊断实战指南                                │
│      ├─ 何时使用 FTA vs FEBM (决策流程图)                       │
│      ├─ 完整案例: 促销流量导致 OOMKilled                        │
│      └─ 联合诊断最佳实践 (0-5-30+ 分钟 SOP)                    │
│                                                                  │
│  8.5 FEBM 效果度量 KPI 仪表板                                   │
│      ├─ 核心 KPI: MTTR, 证据完整度, 根因准确率, 自动化率       │
│      ├─ Grafana 仪表板配置 (8 个面板)                          │
│      ├─ 关键 PromQL 查询 (10 个常用查询)                        │
│      └─ 月度报告模板 (含案例分析)                               │
│                                                                  │
│  8.6 合规快速参考                                                │
│      ├─ 等保 2.0 (三级) 合规映射                                │
│      ├─ SOC 2 Type II 合规映射                                  │
│      └─ 合规检查脚本 (compliance-check.sh)                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

关键交付物:
  ✅ 7 天部署计划 (从零到生产可用)
  ✅ 6 个常见故障的标准化 Runbook (含自动化脚本)
  ✅ FTA + FEBM 联合诊断方法论
  ✅ 完整的 KPI 度量体系和 Grafana 仪表板
  ✅ 等保 2.0 和 SOC 2 合规映射

资源开销:
  - 内存: ~5GB (集群级)
  - 存储: ~160GB (15 天指标 + 30 天日志)
  - CPU: ~2 Cores (集群级)
  - 成本: ~$80/月 (AWS EKS, 不含集群基础成本)

下一步行动:
  1. 执行 Day 1-7 部署计划
  2. 导入 6 个 Runbook 并进行演练
  3. 配置 FEBM KPI 仪表板
  4. 进行首次模拟故障演练
  5. 启动 FTA + FEBM 联合诊断流程
```

#<!-- chunk: 章节导航 -->## 章节导航

```
┌─────────────────────────────────────────────────────────────────┐
│                     FEBM 知识体系导航                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  [第一章] FEBM 方法论基础                                        │
│     └─ 核心概念、与 FTA/RCA 对比、适用场景                      │
│                                                                  │
│  [第二章] FEBM 证据采集技术                                      │
│     └─ 日志、指标、追踪、eBPF、审计日志                         │
│                                                                  │
│  [第三章] FEBM 证据关联与分析                                    │
│     └─ 时间线重建、跨层关联、因果推理                            │
│                                                                  │
│  [第四章] FEBM 在 Kubernetes 中的应用                            │
│     └─ K8s 证据源、工具栈、取证流程                             │
│                                                                  │
│  [第五章] FEBM 自动化与工具链                                    │
│     └─ Falco、Loki、Prometheus、Jaeger 集成                    │
│                                                                  │
│  [第六章] FEBM 与 FTA 融合                                       │
│     └─ FTA 快速筛查 + FEBM 深度取证的最佳实践                   │
│                                                                  │
│  [第七章] FEBM 安全合规与审计                                    │
│     └─ 等保 2.0、SOC 2、GDPR 合规要求                           │
│                                                                  │
│  ★ [第八章] FEBM 生产环境快速启动与 K8s 故障取证手册 (本章)    │
│     └─ 7 天部署计划 + 6 个 Runbook + KPI 仪表板                 │
│                                                                  │
│  [第九章] FEBM 高级主题 (规划中)                                │
│     └─ 分布式追踪深度集成、AI 辅助根因分析、多云取证            │
│                                                                  │
│  [第十章] FEBM 案例库 (规划中)                                  │
│     └─ 真实生产环境案例深度剖析                                  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

推荐阅读路径:

初学者 (SRE 新手):
  第一章 → 第八章 8.1-8.3 → 第四章 → 第八章 8.4-8.6

实践者 (有 K8s 经验):
  第八章 8.1-8.2 → 第六章 → 第八章 8.3-8.5 → 第三章

专家 (架构师/合规官):
  第七章 → 第八章 8.6 → 第三章 → 第五章

快速上手 (紧急故障):
  第八章 8.3 (Runbook) → 第六章 8.4 (联合诊断)
```

#<!-- chunk: 相关资源 -->## 相关资源

```yaml
官方文档链接:

Falco:
  官网: https://falco.org/
  规则库: https://github.com/falcosecurity/rules
  Helm Chart: https://github.com/falcosecurity/charts

Prometheus:
  官网: https://prometheus.io/
  PromQL 文档: https://prometheus.io/docs/prometheus/latest/querying/basics/
  Alerting 规则: https://prometheus.io/docs/prometheus/latest/configuration/alerting_rules/

Grafana Loki:
  官网: https://grafana.com/oss/loki/
  LogQL 文档: https://grafana.com/docs/loki/latest/logql/
  Helm Chart: https://grafana.github.io/helm-charts

Kubernetes:
  审计日志: https://kubernetes.io/docs/tasks/debug/debug-cluster/audit/
  事件: https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/
  故障排查: https://kubernetes.io/docs/tasks/debug/

ArgoCD:
  官网: https://argo-cd.readthedocs.io/
  GitOps 最佳实践: https://argo-cd.readthedocs.io/en/stable/user-guide/best_practices/

OPA Gatekeeper:
  官网: https://open-policy-agent.github.io/gatekeeper/
  策略库: https://github.com/open-policy-agent/gatekeeper-library

合规标准:
  等保 2.0: GB/T 22239-2019
  SOC 2: https://www.aicpa.org/soc
  PCI DSS: https://www.pcisecuritystandards.org/

社区资源:
  CNCF Slack: https://slack.cncf.io/ (#falco, #prometheus, #kubernetes-users)
  Kubernetes SIG-Security: https://github.com/kubernetes/community/tree/master/sig-security
  SRE Weekly Newsletter: https://sreweekly.com/
```

---

<!-- chunk: 附录：常用命令速查表 -->## 附录：常用命令速查表

```bash
# ============================================================
#              FEBM 常用命令速查表
# ============================================================

# --- 证据采集 ---

# 1. Pod 状态快照
kubectl get pod <POD> -n <NS> -o yaml
kubectl describe pod <POD> -n <NS>
kubectl logs <POD> -n <NS> --previous --tail=100

# 2. Prometheus 指标查询
curl -G 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=up{job="kubernetes-pods"}'

# 3. Loki 日志查询
logcli query '{namespace="<NS>",app="<APP>"}' --since=1h --limit=100

# 4. K8s 审计日志
sudo grep "<KEYWORD>" /var/log/kubernetes/audit.log | jq .

# 5. Falco 告警
kubectl logs -n falco -l app.kubernetes.io/name=falco --tail=50

# 6. 事件历史
kubectl get events -n <NS> --sort-by='.lastTimestamp' | tail -20

# --- 故障诊断 ---

# 7. 检查容器退出码
kubectl get pod <POD> -n <NS> -o jsonpath='{.status.containerStatuses[0].lastState.terminated.exitCode}'

# 8. 检查节点资源
kubectl top node <NODE>
kubectl describe node <NODE> | grep -A 5 "Allocated resources"

# 9. 检查 Service Endpoint
kubectl get endpoints <SVC> -n <NS>

# 10. 网络连通性测试
kubectl debug node/<NODE> -it --image=nicolaka/netshoot -- \
  sh -c "ping -c 3 <TARGET_IP>"

# --- 配置审查 ---

# 11. 检查资源配置
kubectl get pod <POD> -n <NS> -o jsonpath='{.spec.containers[0].resources}'

# 12. 检查环境变量
kubectl get pod <POD> -n <NS> -o jsonpath='{.spec.containers[0].env[*]}'

# 13. 检查 ConfigMap
kubectl get cm <CM> -n <NS> -o yaml

# 14. 检查 Secret (base64 解码)
kubectl get secret <SECRET> -n <NS> -o jsonpath='{.data.<KEY>}' | base64 -d

# --- 时间线重建 ---

# 15. 多源证据时间线 (需要手动合并)
# Prometheus: 导出指标时间序列
curl -G 'http://localhost:9090/api/v1/query_range' \
  --data-urlencode 'query=<QUERY>' \
  --data-urlencode 'start=<START_TIME>' \
  --data-urlencode 'end=<END_TIME>' \
  --data-urlencode 'step=60s'

# Loki: 导出日志
logcli query '<QUERY>' --since=<TIME> --until=<TIME> --output=jsonl

# K8s Events: 导出事件
kubectl get events -n <NS> --sort-by='.lastTimestamp' -o json

# --- 自动化取证 ---

# 16. 执行 Runbook 脚本
./oom-forensics.sh <POD_NAME> <NAMESPACE>
./crashloop-forensics.sh <POD_NAME> <NAMESPACE>
./node-notready-forensics.sh <NODE_NAME>

# 17. 一键部署 FEBM 工具栈
./deploy-febm-minimal.sh

# 18. 合规检查
./compliance-check.sh

# --- 紧急处置 ---

# 19. 快速扩容
kubectl scale deploy <DEPLOY> -n <NS> --replicas=<N>

# 20. 重启 Pod
kubectl rollout restart deploy <DEPLOY> -n <NS>

# 21. 驱逐节点
kubectl drain <NODE> --ignore-daemonsets --delete-emptydir-data

# 22. 临时禁用健康检查 (仅测试)
kubectl patch deploy <DEPLOY> -n <NS> --type='json' \
  -p='[{"op":"remove","path":"/spec/template/spec/containers/0/livenessProbe"}]'

# --- 证书管理 ---

# 23. 检查证书有效期
kubeadm certs check-expiration

# 24. 续期所有证书
kubeadm certs renew all

# 25. 查看 Ingress TLS 证书
kubectl get secret <TLS_SECRET> -n <NS> -o jsonpath='{.data.tls\.crt}' | \
  base64 -d | openssl x509 -noout -dates

# --- 性能分析 ---

# 26. CPU 节流检测
kubectl top pods -n <NS>
# 或使用 Prometheus
rate(container_cpu_cfs_throttled_seconds_total{pod="<POD>"}[5m])

# 27. 内存使用趋势
container_memory_working_set_bytes{pod="<POD>"}

# 28. 网络流量统计
container_network_receive_bytes_total{pod="<POD>"}
container_network_transmit_bytes_total{pod="<POD>"}

# --- 访问 UI ---

# 29. Grafana Port Forward
kubectl port-forward -n monitoring svc/kube-prometheus-stack-grafana 3000:80

# 30. Prometheus Port Forward
kubectl port-forward -n monitoring svc/kube-prometheus-stack-prometheus 9090:9090

# 31. Falco Web UI Port Forward
kubectl port-forward -n falco svc/falco-falcosidekick-ui 2802:2802

# ============================================================
#                      END OF CHEAT SHEET
# ============================================================
```

---

**本章完**

📖 **上一章**: [第七章 - FEBM 安全合规与审计](07-febm-security-compliance.md)  
📖 **下一章**: [第九章 - FEBM 高级主题](09-febm-advanced-topics.md) (规划中)  
🏠 **返回目录**: [README.md](README.md)

---

<div align="center">

**FEBM 生产环境快速启动与 Kubernetes 故障取证手册**

*让每一次故障都成为能力提升的机会*

📅 最后更新: 2024-03  
📧 反馈与建议: [GitHub Issues](https://github.com/your-repo/febm-handbook/issues)  
⭐ 如果本章对你有帮助，请给我们一个 Star！

</div>

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-febm/MOC.md|topic-febm MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/README.md|topic-febm: FEBM 法医鉴定循证方法论深度解析]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md|第一章：FEBM 方法论原理与理论基础]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md|第二章:FEBM 技术实现体系]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/03-febm-best-practices.md|第三章：FEBM 最佳实践]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/04-febm-agent-ticket-processing.md|第四章：FEBM 对云平台工单智能体托管的意义]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/05-febm-construction-methodology.md|第五章：FEBM 体系建设方法论]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/06-febm-future-evolution.md|第六章：未来演进方向]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/07-febm-appendix.md|第七章:附录]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/febm-methodology-deep-dive.md|法医鉴定循证方法论（FEBM）深度解析]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/fta-febm-joint-diagnosis.md|FTA-FEBM 联合诊断最佳实践]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-febm/06-febm-future-evolution.md|06-febm-future-evolution]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/07-febm-appendix.md|07-febm-appendix]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/febm-methodology-deep-dive.md|febm-methodology-deep-dive]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/fta-febm-joint-diagnosis.md|fta-febm-joint-diagnosis]]
