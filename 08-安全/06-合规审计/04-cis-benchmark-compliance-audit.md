---
title: 08-CIS基准合规检查
description: 'description: ''<!-- chunk: 📋 概述'' -->## 📋 概述'''
summary: 'description: ''<!-- chunk: 📋 概述'' -->## 📋 概述'''
category: general
tags:
- k8s
- production
- best-practice
- performance
- compliance
- etcd
- apiserver
- kubelet
- prometheus
- opa
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- cis-benchmark-compliance-audit是什么？
- cis-benchmark-compliance-audit的使用方法
- cis-benchmark-compliance-audit的最佳实践
trigger_keywords:
- CIS基准合规检查
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- prometheus-basics
- etcd-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 08-CIS基准合规检查
description: '<!-- chunk: 📋 概述' -->## 📋 概述'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- [[prometheus|prometheus]]
- opa
- daemonset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- CIS基准合规检查 是什么
- 如何 CIS基准合规检查
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- CIS基准合规检查
- production
- operations
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

# 08-CIS基准合规检查

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 📋 概述 -->## 📋 概述

CIS (Center for Internet Security) 基准是Kubernetes安全配置的标准参考。本文档详细介绍如何实施CIS基准合规检查和持续监控。

<!-- chunk: 🎯 CIS基准概述 -->## 🎯 CIS基准概述

## 基准版本对应关系

| Kubernetes版本 | CIS基准版本 | 发布日期 |
|---------------|------------|----------|
| v1.25         | CIS 1.25   | 2022-11  |
| v1.26         | CIS 1.26   | 2023-02  |
| v1.27         | CIS 1.27   | 2023-05  |
| v1.28         | CIS 1.28   | 2023-08  |
| v1.29         | CIS 1.29   | 2023-11  |
| v1.30+        | CIS 1.30+  | 2024-02+ |

## 控制平面检查项 (Master Node)

## 1. Master节点安全配置
```yaml
# Master节点CIS检查配置
apiVersion: batch/v1
kind: Job
metadata:
  name: cis-master-check
  namespace: security-audit
spec:
  template:
    spec:
      hostPID: true
      hostNetwork: true
      containers:
      - name: kube-bench-master
        image: aquasec/kube-bench:latest
        command: ["kube-bench", "run", "--targets", "master"]
        volumeMounts:
        - name: var-lib-etcd
          mountPath: /var/lib/etcd
          readOnly: true
        - name: etc-kubernetes
          mountPath: /etc/kubernetes
          readOnly: true
        - name: etc-systemd
          mountPath: /etc/systemd
          readOnly: true
        - name: etc-default
          mountPath: /etc/default
          readOnly: true
      volumes:
      - name: var-lib-etcd
        hostPath:
          path: "/var/lib/etcd"
      - name: etc-kubernetes
        hostPath:
          path: "/etc/kubernetes"
      - name: etc-systemd
        hostPath:
          path: "/etc/systemd"
      - name: etc-default
        hostPath:
          path: "/etc/default"
      restartPolicy: Never
```

## 2. 关键检查项示例
```bash
#!/bin/bash
# Master节点CIS关键检查脚本

echo "=== CIS Master Node Checks ==="

# 1.1.1 - Ensure that the API server pod specification file permissions are set to 644 or more restrictive
API_SERVER_FILE="/etc/kubernetes/manifests/kube-apiserver.yaml"
if [ -f "$API_SERVER_FILE" ]; then
    PERMISSIONS=$(stat -c %a "$API_SERVER_FILE")
    if [ "$PERMISSIONS" -le 644 ]; then
        echo "✓ 1.1.1 API server file permissions OK: $PERMISSIONS"
    else
        echo "✗ 1.1.1 API server file permissions TOO PERMISSIVE: $PERMISSIONS"
    fi
fi

# 1.2.1 - Ensure that the --anonymous-auth argument is set to false
if grep -q "anonymous-auth=false" /etc/kubernetes/manifests/kube-apiserver.yaml; then
    echo "✓ 1.2.1 Anonymous auth disabled"
else
    echo "✗ 1.2.1 Anonymous auth should be disabled"
fi

# 1.2.2 - Ensure that the --token-auth-file parameter is not set
if ! grep -q "token-auth-file" /etc/kubernetes/manifests/kube-apiserver.yaml; then
    echo "✓ 1.2.2 Token auth file not configured"
else
    echo "✗ 1.2.2 Token auth file should not be used"
fi
```

## 工作节点检查项 (Worker Node)

## 1. Worker节点安全配置
```yaml
# Worker节点CIS检查配置
apiVersion: batch/v1
kind: DaemonSet
metadata:
  name: cis-worker-check
  namespace: security-audit
spec:
  selector:
    matchLabels:
      app: cis-worker-check
  template:
    metadata:
      labels:
        app: cis-worker-check
    spec:
      hostPID: true
      hostNetwork: true
      containers:
      - name: kube-bench-worker
        image: aquasec/kube-bench:latest
        command: ["kube-bench", "run", "--targets", "node"]
        volumeMounts:
        - name: etc-kubernetes
          mountPath: /etc/kubernetes
          readOnly: true
        - name: etc-systemd
          mountPath: /etc/systemd
          readOnly: true
        - name: etc-default
          mountPath: /etc/default
          readOnly: true
        - name: var-lib-kubelet
          mountPath: /var/lib/kubelet
          readOnly: true
      volumes:
      - name: etc-kubernetes
        hostPath:
          path: "/etc/kubernetes"
      - name: etc-systemd
        hostPath:
          path: "/etc/systemd"
      - name: etc-default
        hostPath:
          path: "/etc/default"
      - name: var-lib-kubelet
        hostPath:
          path: "/var/lib/kubelet"
```

## 2. 节点级检查项
```bash
#!/bin/bash
# Worker节点CIS检查脚本

echo "=== CIS Worker Node Checks ==="

# 4.1.1 - Ensure that the kubelet service file permissions are set to 644 or more restrictive
KUBELET_SERVICE="/etc/systemd/system/kubelet.service.d/10-kubeadm.conf"
if [ -f "$KUBELET_SERVICE" ]; then
    PERMISSIONS=$(stat -c %a "$KUBELET_SERVICE")
    if [ "$PERMISSIONS" -le 644 ]; then
        echo "✓ 4.1.1 Kubelet service file permissions OK: $PERMISSIONS"
    else
        echo "✗ 4.1.1 Kubelet service file permissions TOO PERMISSIVE: $PERMISSIONS"
    fi
fi

# 4.2.1 - Ensure that the --anonymous-auth argument is set to false
if ps aux | grep kubelet | grep -q "anonymous-auth=false"; then
    echo "✓ 4.2.1 Kubelet anonymous auth disabled"
else
    echo "✗ 4.2.1 Kubelet anonymous auth should be disabled"
fi

# 4.2.2 - Ensure that the --authorization-mode argument is not set to AlwaysAllow
if ps aux | grep kubelet | grep -q "authorization-mode.*AlwaysAllow"; then
    echo "✗ 4.2.2 Kubelet authorization mode should not be AlwaysAllow"
else
    echo "✓ 4.2.2 Kubelet authorization mode properly configured"
fi
```

<!-- chunk: 🛠️ 自动化合规检查 -->## 🛠️ 自动化合规检查

## 持续合规监控

## 1. 定期扫描配置
```yaml
# CIS基准定期扫描CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cis-compliance-scan
  namespace: security-audit
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 5
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: cis-scanner
          containers:
          - name: cis-scanner
            image: aquasec/kube-bench:latest
            command:
            - /bin/sh
            - -c
            - |
              kube-bench run --targets master,node,etcd --json > /results/cis-results-$(date +%Y%m%d-%H%M%S).json
              # 发送结果到安全平台
              curl -X POST -H "Content-Type: application/json" \
                -d @/results/latest-results.json \
                https://security-dashboard.example.com/api/cis-results
            volumeMounts:
            - name: results
              mountPath: /results
          volumes:
          - name: results
            emptyDir: {}
          restartPolicy: Never
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cis-scanner
  namespace: security-audit
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cis-scanner-role
rules:
- apiGroups: [""]
  resources: ["nodes", "pods", "namespaces"]
  verbs: ["get", "list"]
- apiGroups: ["policy"]
  resources: ["podsecuritypolicies"]
  verbs: ["use"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cis-scanner-binding
subjects:
- kind: ServiceAccount
  name: cis-scanner
  namespace: security-audit
roleRef:
  kind: ClusterRole
  name: cis-scanner-role
  apiGroup: rbac.authorization.k8s.io
```

## 2. 实时合规监控
```yaml
# 实时合规监控Operator
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cis-compliance-operator
  namespace: security-audit
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cis-compliance-operator
  template:
    metadata:
      labels:
        app: cis-compliance-operator
    spec:
      serviceAccountName: cis-operator
      containers:
      - name: operator
        image: custom/cis-compliance-operator:latest
        env:
        - name: SCAN_INTERVAL
          value: "300"  # 5分钟扫描间隔
        - name: ALERT_THRESHOLD
          value: "70"   # 70%合规率告警阈值
        - name: SLACK_WEBHOOK
          valueFrom:
            secretKeyRef:
              name: compliance-secrets
              key: slack-webhook
        volumeMounts:
        - name: config
          mountPath: /config
      volumes:
      - name: config
        configMap:
          name: cis-compliance-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: cis-compliance-config
  namespace: security-audit
data:
  compliance-rules.yaml: |
    rules:
    - id: "1.1.1"
      name: "API Server File Permissions"
      severity: "HIGH"
      remediation: "chmod 644 /etc/kubernetes/manifests/kube-apiserver.yaml"
      
    - id: "1.2.1"
      name: "Disable Anonymous Authentication"
      severity: "CRITICAL"
      remediation: "Add --anonymous-auth=false to API server arguments"
```

<!-- chunk: 📊 合规报告生成 -->## 📊 合规报告生成

## 详细报告模板

## 1. JSON格式报告
```json
{
  "scan_metadata": {
    "scan_id": "cis-20240115-143022",
    "timestamp": "2024-01-15T14:30:22Z",
    "kubernetes_version": "v1.28.2",
    "cis_benchmark_version": "1.28",
    "cluster_name": "production-cluster"
  },
  "summary": {
    "total_checks": 156,
    "passed": 134,
    "failed": 15,
    "skipped": 7,
    "compliance_score": 85.9
  },
  "failed_checks": [
    {
      "id": "1.2.3",
      "description": "Ensure that the --insecure-port argument is set to 0",
      "severity": "HIGH",
      "current_state": "--insecure-port=8080",
      "recommended_state": "--insecure-port=0",
      "remediation": "Edit the API server pod specification file /etc/kubernetes/manifests/kube-apiserver.yaml and set the --insecure-port parameter to 0"
    }
  ],
  "node_details": {
    "master_nodes": {
      "total": 3,
      "compliant": 2,
      "non_compliant": 1
    },
    "worker_nodes": {
      "total": 12,
      "compliant": 10,
      "non_compliant": 2
    }
  }
}
```

## 2. HTML可视化报告
```html
<!DOCTYPE html>
<html>
<head>
    <title>CIS Compliance Report</title>
    <style>
        .critical { color: #dc3545; }
        .high { color: #fd7e14; }
        .medium { color: #ffc107; }
        .low { color: #28a745; }
        .progress-bar {
            width: 100%;
            height: 20px;
            background-color: #e9ecef;
            border-radius: 10px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            transition: width 0.3s ease;
        }
    </style>
</head>
<body>
    <div class="report-header">
        <h1>CIS Kubernetes Benchmark Compliance Report</h1>
        <p>Generated: <span id="timestamp"></span></p>
        <p>Cluster: <span id="cluster-name"></span></p>
    </div>
    
    <div class="compliance-summary">
        <h2>Compliance Score: <span id="score" class="score">85.9%</span></h2>
        <div class="progress-bar">
            <div class="progress-fill" id="progress-fill" style="width: 85.9%; background-color: #28a745;"></div>
        </div>
    </div>
    
    <div class="failed-checks">
        <h2>Failed Checks</h2>
        <table id="failed-table">
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Description</th>
                    <th>Severity</th>
                    <th>Status</th>
                </tr>
            </thead>
            <tbody></tbody>
        </table>
    </div>
</body>
</html>
```

<!-- chunk: 🔧 不合规项修复 -->## 🔧 不合规项修复

## 自动化修复脚本

## 1. Master节点修复

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# Master节点CIS合规修复脚本

set -e

echo "Starting CIS compliance remediation for Master nodes..."

# 1.1.1 - Fix API server file permissions
fix_api_server_permissions() {
    local file="/etc/kubernetes/manifests/kube-apiserver.yaml"
    if [ -f "$file" ]; then
        chmod 644 "$file"
        chown root:root "$file"
        echo "✓ Fixed API server file permissions"
    fi
}

# 1.2.1 - Disable anonymous authentication
disable_anonymous_auth() {
    local manifest="/etc/kubernetes/manifests/kube-apiserver.yaml"
    if grep -q "anonymous-auth" "$manifest"; then
        sed -i 's/--anonymous-auth=true/--anonymous-auth=false/g' "$manifest"
    else
        sed -i '/- kube-apiserver/a\    - --anonymous-auth=false' "$manifest"
    fi
    echo "✓ Disabled anonymous authentication"
}

# 1.2.2 - Remove token auth file
remove_token_auth_file() {
    local manifest="/etc/kubernetes/manifests/kube-apiserver.yaml"
    sed -i '/--token-auth-file/d' "$manifest"
    echo "✓ Removed token auth file configuration"
}

# 执行修复
fix_api_server_permissions
disable_anonymous_auth
remove_token_auth_file

echo "Master node remediation completed. Restarting kubelet..."
systemctl restart kubelet
```
## 2. Worker节点修复

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# Worker节点CIS合规修复脚本

set -e

echo "Starting CIS compliance remediation for Worker nodes..."

# 4.1.1 - Fix kubelet service file permissions
fix_kubelet_permissions() {
    local service_file="/etc/systemd/system/kubelet.service.d/10-kubeadm.conf"
    if [ -f "$service_file" ]; then
        chmod 644 "$service_file"
        chown root:root "$service_file"
        echo "✓ Fixed kubelet service file permissions"
    fi
}

# 4.2.1 - Configure kubelet authentication
configure_kubelet_auth() {
    local config_file="/var/lib/kubelet/config.yaml"
    if [ -f "$config_file" ]; then
        # 确保启用客户端证书认证
        yq eval '.authentication.x509.clientCAFile = "/etc/kubernetes/pki/ca.crt"' -i "$config_file"
        # 禁用匿名认证
        yq eval '.authentication.anonymous.enabled = false' -i "$config_file"
        echo "✓ Configured kubelet authentication"
    fi
}

# 4.2.2 - Configure authorization mode
configure_authorization() {
    local config_file="/var/lib/kubelet/config.yaml"
    if [ -f "$config_file" ]; then
        yq eval '.authorization.mode = "Webhook"' -i "$config_file"
        echo "✓ Configured kubelet authorization mode"
    fi
}

# 执行修复
fix_kubelet_permissions
configure_kubelet_auth
configure_authorization

echo "Worker node remediation completed. Restarting kubelet..."
systemctl daemon-reload
systemctl restart kubelet
```
<!-- chunk: 🚨 告警与通知 -->## 🚨 告警与通知

## 合规状态监控

## 1. Prometheus告警规则
```yaml
# CIS合规告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cis-compliance-alerts
  namespace: monitoring
spec:
  groups:
  - name: cis.rules
    rules:
    - alert: LowCISComplianceScore
      expr: cis_compliance_score < 80
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "CIS compliance score below threshold"
        description: "Current compliance score is {{ $value }}%, below the 80% threshold"
        
    - alert: CriticalCISViolation
      expr: cis_failed_checks{severity="CRITICAL"} > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Critical CIS benchmark violations detected"
        description: "{{ $value }} critical violations found in latest scan"
        
    - alert: MasterNodeNonCompliant
      expr: cis_master_nodes_compliant / cis_master_nodes_total * 100 < 90
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Master nodes compliance below 90%"
        description: "Only {{ $value }}% of master nodes are compliant"
```

## 2. Slack通知集成
```yaml
# Slack通知配置
apiVersion: v1
kind: Secret
metadata:
  name: compliance-notifications
  namespace: security-audit
type: Opaque
data:
  slack-webhook: <base64_encoded_webhook_url>
---
apiVersion: batch/v1
kind: Job
metadata:
  name: compliance-notification
  namespace: security-audit
spec:
  template:
    spec:
      containers:
      - name: notifier
        image: curlimages/curl:latest
        command:
        - /bin/sh
        - -c
        - |
          COMPLIANCE_SCORE=$(kubectl get cm cis-results -o jsonpath='{.data.score}')
          
          if [ "$COMPLIANCE_SCORE" -lt 80 ]; then
            curl -X POST -H 'Content-type: application/json' \
              --data "{
                \"text\": \"🚨 CIS Compliance Alert\",
                \"attachments\": [{
                  \"color\": \"danger\",
                  \"fields\": [{
                    \"title\": \"Compliance Score\",
                    \"value\": \"${COMPLIANCE_SCORE}%\",
                    \"short\": true
                  }, {
                    \"title\": \"Status\",
                    \"value\": \"Below Threshold\",
                    \"short\": true
                  }]
                }]
              }" $SLACK_WEBHOOK
          fi
        env:
        - name: SLACK_WEBHOOK
          valueFrom:
            secretKeyRef:
              name: compliance-notifications
              key: slack-webhook
      restartPolicy: Never
```

<!-- chunk: 🔧 实施检查清单 -->## 🔧 实施检查清单

## 基准检查准备
- [ ] 确认Kubernetes版本对应的CIS基准版本
- [ ] 部署CIS检查工具(kube-bench等)
- [ ] 配置检查范围和排除项
- [ ] 建立检查结果收集机制
- [ ] 设置定期扫描计划
- [ ] 配置告警和通知机制

## 合规修复实施
- [ ] 分析检查结果识别不合规项
- [ ] 制定修复计划和优先级排序
- [ ] 实施自动化修复脚本
- [ ] 验证修复效果和回归测试
- [ ] 建立持续监控机制
- [ ] 维护合规状态报告

## 持续改进
- [ ] 定期更新CIS基准检查规则
- [ ] 优化检查性能和准确性
- [ ] 完善修复自动化流程
- [ ] 建立合规性趋势分析
- [ ] 持续培训和意识提升
- [ ] 维护合规性文档和证据

---

*本文档为企业实施CIS Kubernetes基准合规检查提供完整的技术方案和操作指南*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生产运维 MOC
- [[13-生产运维/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[01-集群基础/02-设计原则/02-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 09-软件物料清单
- 10-GitOps流水线实践

## Related

- [[02-工作负载/02-Java-on-K8s/02-jvm-gc-container-tuning.md|03-jvm-gc-container-tuning]]

- [[08-安全/README.md|返回目录]]- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]

## See Also

- 06-apm-application-performance-monitoring
- 07-zero-trust-security-architecture
- 09-software-bill-of-materials
- 10-gitops-pipeline-practices


<!-- risk-assessed -->
