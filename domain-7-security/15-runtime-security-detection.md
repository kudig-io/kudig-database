# 15 - 安全扫描与检测工具

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 中高级 | **参考**: [Falco Documentation](https://falco.org/docs/) | [KubeArmor Documentation](https://kubearmor.io/)

## 一、运行时安全检测架构

### 1.1 运行时安全检测体系

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      Runtime Security Detection Architecture                        │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                           Detection Layers                                     │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │   System     │  │   Container  │  │   Network    │  │   Application│       │ │
│  │  │   系统层     │  │   容器层     │  │   网络层     │  │   应用层     │       │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │ │
│  │         │                 │                 │                 │                │ │
│  │         └─────────────────┼─────────────────┼─────────────────┘                │ │
│  │                           │                 │                                  │ │
│  │                    ┌──────▼─────────────────▼──────┐                          │ │
│  │                    │    Detection Engines          │                          │ │
│  │                    │    检测引擎                  │                          │ │
│  │                    └───────────────────────────────┘                          │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                        Core Detection Technologies                             │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                           eBPF/BPF                                       │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Syscall   │  │   Network   │  │   LSM       │  │   Trace     │         │   │ │
│  │  │  │   系统调用   │  │   网络      │  │   安全模块   │  │   跟踪      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                           Traditional                                    │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Auditd    │  │   SELinux   │  │   AppArmor  │  │   Seccomp   │         │   │ │
│  │  │  │   审计系统   │  │   强制访问   │  │   应用防护   │  │   系统调用   │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                          Alert & Response                                      │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Notification Channels                           │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Slack     │  │   Email     │  │   Webhook   │  │   SIEM      │         │   │ │
│  │  │  │   即时通讯   │  │   邮件      │  │   回调      │  │   安全平台   │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Automated Response                              │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Kill      │  │   Quarantine│  │   Scale     │  │   Remediate │         │   │ │
│  │  │  │   终止进程   │  │   隔离      │  │   缩容      │  │   修复      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 主流检测工具对比

| 工具 | 技术原理 | 性能影响 | 检测能力 | 易用性 | 适用场景 |
|-----|---------|---------|---------|--------|---------|
| **Falco** | eBPF/Syscall | 低-中 | 系统调用、文件操作 | 高 | 通用威胁检测 |
| **KubeArmor** | LSM/eBPF | 低 | 文件、进程、网络 | 中 | Kubernetes专用 |
| **Sysdig** | eBPF | 中 | 全栈监控 | 高 | 商业平台 |
| **Tetragon** | eBPF | 低 | 网络、进程跟踪 | 中 | Cilium生态 |
| **Tracee** | eBPF | 中 | 运行时追踪 | 中 | 安全研究 |

## 二、Falco深度配置与实践

### 2.1 Falco核心配置

#### Falco部署配置

```yaml
# 01-falco-deployment.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
  namespace: security
  labels:
    app: falco
spec:
  selector:
    matchLabels:
      app: falco
  template:
    metadata:
      labels:
        app: falco
    spec:
      containers:
      - name: falco
        image: falcosecurity/falco-no-driver:0.37.0
        securityContext:
          privileged: true
        env:
        - name: FALCO_BPF_PROBE
          value: ""
        - name: FALCO_K8S_API_CERT
          value: "/var/run/secrets/kubernetes.io/serviceaccount/ca.crt"
        - name: FALCO_K8S_API_SERVER
          value: "https://kubernetes.default.svc.cluster.local"
        volumeMounts:
        - name: dev
          mountPath: /host/dev
        - name: proc
          mountPath: /host/proc
        - name: boot
          mountPath: /host/boot
        - name: lib-modules
          mountPath: /lib/modules
        - name: usr
          mountPath: /usr
        - name: etc
          mountPath: /etc
        - name: rules
          mountPath: /etc/falco/rules.d
        - name: falco-config
          mountPath: /etc/falco/falco.yaml
          subPath: falco.yaml
      volumes:
      - name: dev
        hostPath:
          path: /dev
      - name: proc
        hostPath:
          path: /proc
      - name: boot
        hostPath:
          path: /boot
      - name: lib-modules
        hostPath:
          path: /lib/modules
      - name: usr
        hostPath:
          path: /usr
      - name: etc
        hostPath:
          path: /etc
      - name: rules
        configMap:
          name: falco-rules
      - name: falco-config
        configMap:
          name: falco-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-config
  namespace: security
data:
  falco.yaml: |
    # Falco核心配置
    log_level: info
    priority: debug
    buffer_dim: 8388608
    rules_file:
      - /etc/falco/falco_rules.yaml
      - /etc/falco/falco_rules.local.yaml
      - /etc/falco/k8s_audit_rules.yaml
      - /etc/falco/rules.d
    
    # 输出配置
    stdout_output:
      enabled: true
      
    syslog_output:
      enabled: true
      
    program_output:
      enabled: false
      
    http_output:
      enabled: true
      url: http://falco-webhook.security.svc.cluster.local:8080
      user_agent: "falcosecurity/falco"
      
    # Kubernetes集成
    k8s_audit_config:
      enabled: true
      k8s_audit_url: http://localhost:9765/k8s_audit
      ssl_verify: false
      
    # JSON输出格式
    json_output: true
    json_include_output_property: true
```

### 2.2 自定义检测规则

#### Falco规则编写最佳实践

```yaml
# 02-custom-falco-rules.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-custom-rules
  namespace: security
data:
  custom-rules.yaml: |
    # 自定义安全规则集合
    
    # 1. 检测特权容器逃逸尝试
    - rule: Privileged Container Escape Attempt
      desc: Detect attempts to escape from privileged containers
      condition: >
        spawned_process and container
        and proc.name in (chmod, chown, mount, umount)
        and proc.args contains "/proc" or proc.args contains "/sys"
        and container.privileged = true
      output: >
        Privileged container escape attempt (user=%user.name command=%proc.cmdline container=%container.id)
      priority: CRITICAL
      tags: [container, privilege, escape]
      
    # 2. 检测敏感文件访问
    - rule: Sensitive File Access
      desc: Detect access to sensitive system files
      condition: >
        open_read or open_write
        and fd.name startswith /etc/passwd
        or fd.name startswith /etc/shadow
        or fd.name startswith /root/.ssh/
      output: >
        Sensitive file access detected (user=%user.name file=%fd.name command=%proc.cmdline)
      priority: WARNING
      tags: [filesystem, sensitive]
      
    # 3. 检测异常网络连接
    - rule: Suspicious Network Connection
      desc: Detect suspicious outbound network connections
      condition: >
        outbound and evt.type = connect
        and fd.sip != "127.0.0.1" and fd.sip != "10.0.0.0/8"
        and proc.name in (curl, wget, nc, netcat, python, perl)
        and container
      output: >
        Suspicious outbound connection (container=%container.id command=%proc.cmdline dest_ip=%fd.sip dest_port=%fd.sport)
      priority: NOTICE
      tags: [network, suspicious]
      
    # 4. 检测Kubernetes API异常访问
    - rule: K8s API Suspicious Access
      desc: Detect suspicious access to Kubernetes API server
      condition: >
        k8s.audit.verb in (create, update, patch, delete)
        and k8s.audit.resource in (secrets, configmaps, serviceaccounts)
        and k8s.audit.user.name != "system:serviceaccount"
        and not k8s.audit.requestObject.metadata.namespace in (kube-system, security)
      output: >
        Suspicious K8s API access (user=%k8s.audit.user.name verb=%k8s.audit.verb resource=%k8s.audit.resource namespace=%k8s.audit.requestObject.metadata.namespace)
      priority: WARNING
      tags: [kubernetes, api, suspicious]
      
    # 5. 检测挖矿软件活动
    - rule: Cryptominer Activity Detected
      desc: Detect cryptocurrency mining activity
      condition: >
        spawned_process
        and proc.name in (xmrig, cgminer, cpuminer, stratum)
        or proc.cmdline contains "stratum+tcp"
        or proc.cmdline contains "pool.minexmr.com"
      output: >
        Cryptominer activity detected (process=%proc.name cmdline=%proc.cmdline container=%container.id)
      priority: CRITICAL
      tags: [malware, cryptomining]
```

## 三、KubeArmor配置与策略

### 3.1 KubeArmor部署配置

#### KubeArmor核心组件

```yaml
# 03-kubearmor-deployment.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kubearmor
  namespace: security
spec:
  selector:
    matchLabels:
      kubearmor-app: kubearmor
  template:
    metadata:
      labels:
        kubearmor-app: kubearmor
    spec:
      hostPID: true
      hostNetwork: true
      containers:
      - name: kubearmor
        image: kubearmor/kubearmor:latest
        args:
        - -criSocket
        - /run/containerd/containerd.sock
        securityContext:
          privileged: true
          capabilities:
            add:
            - SYS_ADMIN
            - SYS_PTRACE
            - NET_ADMIN
        volumeMounts:
        - name: containerd-sock
          mountPath: /run/containerd/containerd.sock
        - name: cri-sock
          mountPath: /run/crio/crio.sock
        - name: docker-sock
          mountPath: /var/run/docker.sock
        - name: bpffs
          mountPath: /sys/fs/bpf
        - name: localtime
          mountPath: /etc/localtime
        - name: varlibdocker
          mountPath: /var/lib/docker
        - name: varlibcontainerd
          mountPath: /var/lib/containerd
        - name: varlibcontainers
          mountPath: /var/lib/containers
        - name: tmp
          mountPath: /tmp
        env:
        - name: KUBEARMOR_HOST
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        - name: KUBEARMOR_LOG_PATH
          value: "/tmp/kubearmor.log"
      volumes:
      - name: containerd-sock
        hostPath:
          path: /run/containerd/containerd.sock
      - name: cri-sock
        hostPath:
          path: /run/crio/crio.sock
      - name: docker-sock
        hostPath:
          path: /var/run/docker.sock
      - name: bpffs
        hostPath:
          path: /sys/fs/bpf
      - name: localtime
        hostPath:
          path: /etc/localtime
      - name: varlibdocker
        hostPath:
          path: /var/lib/docker
      - name: varlibcontainerd
        hostPath:
          path: /var/lib/containerd
      - name: varlibcontainers
        hostPath:
          path: /var/lib/containers
      - name: tmp
        hostPath:
          path: /tmp
```

### 3.2 KubeArmor安全策略

#### 应用层安全策略示例

```yaml
# 04-kubearmor-policies.yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: wordpress-security-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: wordpress
      
  # 文件系统访问控制
  file:
    matchDirectories:
    - dir: /var/www/html/
      readOnly: true
      recursive: true
    - dir: /tmp/
      ownerOnly: true
    - dir: /var/log/
      readOnly: true
      
    matchPaths:
    - path: /etc/passwd
      readOnly: true
    - path: /etc/shadow
      action: Block
      
  # 进程执行控制
  process:
    matchPaths:
    - path: /usr/bin/apt
      action: Block
    - path: /usr/bin/yum
      action: Block
    - path: /bin/bash
      ownerOnly: true
    - path: /usr/bin/wget
      action: Block
    - path: /usr/bin/curl
      fromSource:
      - path: /usr/sbin/apache2
      
  # 网络访问控制
  network:
    matchProtocols:
    - protocol: TCP
      remotePorts:
      - port: 3306  # MySQL
        action: Allow
      - port: 6379  # Redis
        action: Allow
      - port: 80    # HTTP
        action: Allow
      - port: 443   # HTTPS
        action: Allow
      localPorts:
      - port: 80
        action: Allow
        
  # 系统调用限制
  syscall:
    matchSyscalls:
    - syscall:
      - socket
      - connect
      - bind
      action: Audit
      
  # 动作配置
  action:
    Allow
```

## 四、威胁情报集成

### 4.1 威胁情报平台对接

#### 威胁情报收集器

```yaml
# 05-threat-intel-collector.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: threat-intel-collector
  namespace: security
spec:
  replicas: 1
  selector:
    matchLabels:
      app: threat-intel-collector
  template:
    metadata:
      labels:
        app: threat-intel-collector
    spec:
      containers:
      - name: collector
        image: security/threat-intel-collector:latest
        env:
        - name: VT_API_KEY
          valueFrom:
            secretKeyRef:
              name: threat-intel-keys
              key: virustotal
        - name: ABUSEIPDB_API_KEY
          valueFrom:
            secretKeyRef:
              name: threat-intel-keys
              key: abuseipdb
        - name: ALIENVAULT_OTX_KEY
          valueFrom:
            secretKeyRef:
              name: threat-intel-keys
              key: alienvault
        volumeMounts:
        - name: intel-cache
          mountPath: /var/cache/threat-intel
        - name: rules-output
          mountPath: /etc/falco/rules.d
      volumes:
      - name: intel-cache
        emptyDir: {}
      - name: rules-output
        emptyDir: {}
---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: threat-intel-update
  namespace: security
spec:
  schedule: "*/30 * * * *"  # 每30分钟更新一次
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: updater
            image: security/threat-intel-updater:latest
            command:
            - "/bin/sh"
            - "-c"
            - |
              # 更新威胁情报
              /usr/local/bin/update-intel-feeds.sh
              
              # 生成Falco规则
              /usr/local/bin/generate-falco-rules.py
              
              # 重启Falco应用新规则
              kubectl delete pod -n security -l app=falco
            volumeMounts:
            - name: rules-volume
              mountPath: /etc/falco/rules.d
          volumes:
          - name: rules-volume
            configMap:
              name: falco-custom-rules
          restartPolicy: OnFailure
```

### 4.2 威胁情报规则生成

#### 自动生成威胁检测规则

```python
#!/usr/bin/env python3
# 06-generate-threat-rules.py

import json
import yaml
from datetime import datetime

class ThreatRuleGenerator:
    def __init__(self):
        self.threat_feeds = {}
        self.generated_rules = []
        
    def load_threat_feeds(self):
        """加载威胁情报源"""
        # 从不同源加载威胁情报
        feeds = {
            'malware_hashes': self.load_virus_total_hashes(),
            'suspicious_ips': self.load_abuseipdb_ips(),
            'malicious_domains': self.load_alienvault_domains()
        }
        self.threat_feeds = feeds
        
    def load_virus_total_hashes(self):
        """加载恶意文件哈希"""
        # 模拟加载VT恶意哈希
        return [
            'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
            'd41d8cd98f00b204e9800998ecf8427e'
        ]
        
    def load_abuseipdb_ips(self):
        """加载恶意IP地址"""
        return ['192.168.1.100', '10.0.0.50']
        
    def load_alienvault_domains(self):
        """加载恶意域名"""
        return ['malicious-site.com', 'phishing-domain.net']
        
    def generate_falco_rules(self):
        """生成Falco检测规则"""
        rules = []
        
        # 生成恶意哈希检测规则
        if self.threat_feeds.get('malware_hashes'):
            hash_rule = {
                'rule': 'Malicious File Hash Detected',
                'desc': 'Detect files with known malicious hashes',
                'condition': f"open_read and fd.name in ({', '.join([f'\"{h}\"' for h in self.threat_feeds['malware_hashes']])})",
                'output': 'Malicious file detected (hash=%fd.name)',
                'priority': 'CRITICAL',
                'tags': ['malware', 'hash']
            }
            rules.append(hash_rule)
            
        # 生成恶意IP连接规则
        if self.threat_feeds.get('suspicious_ips'):
            ip_rule = {
                'rule': 'Connection to Malicious IP',
                'desc': 'Detect connections to known malicious IP addresses',
                'condition': f"outbound and fd.sip in ({', '.join([f'\"{ip}\"' for ip in self.threat_feeds['suspicious_ips']])})",
                'output': 'Connection to malicious IP detected (dest_ip=%fd.sip)',
                'priority': 'WARNING',
                'tags': ['network', 'malicious-ip']
            }
            rules.append(ip_rule)
            
        # 生成DNS查询拦截规则
        if self.threat_feeds.get('malicious_domains'):
            dns_rule = {
                'rule': 'Query to Malicious Domain',
                'desc': 'Detect DNS queries to known malicious domains',
                'condition': f"dns.query.name in ({', '.join([f'\"{domain}\"' for domain in self.threat_feeds['malicious_domains']])})",
                'output': 'Query to malicious domain detected (domain=%dns.query.name)',
                'priority': 'WARNING',
                'tags': ['dns', 'malicious-domain']
            }
            rules.append(dns_rule)
            
        self.generated_rules = rules
        return rules
        
    def save_rules(self, output_file):
        """保存生成的规则到文件"""
        rules_yaml = yaml.dump({'rules': self.generated_rules}, default_flow_style=False)
        
        with open(output_file, 'w') as f:
            f.write(f"# Generated threat detection rules\n")
            f.write(f"# Generated at: {datetime.now().isoformat()}\n\n")
            f.write(rules_yaml)
            
        print(f"Rules saved to {output_file}")

def main():
    generator = ThreatRuleGenerator()
    generator.load_threat_feeds()
    rules = generator.generate_falco_rules()
    generator.save_rules('/etc/falco/rules.d/threat-intel-rules.yaml')
    
    print(f"Generated {len(rules)} threat detection rules")

if __name__ == "__main__":
    main()
```

## 五、告警与响应自动化

### 5.1 告警通知配置

#### 多渠道告警集成

```yaml
# 07-alert-notification-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-alert-config
  namespace: security
data:
  alertmanager.yaml: |
    route:
      receiver: 'default-receiver'
      group_by: ['alertname', 'cluster']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 3h
      
      routes:
      - match:
          severity: 'critical'
        receiver: 'critical-alerts'
        group_wait: 10s
        repeat_interval: 1h
        
      - match:
          severity: 'warning'
        receiver: 'warning-alerts'
        group_wait: 1m
        
    receivers:
    - name: 'default-receiver'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#security-alerts'
        send_resolved: true
        title: '{{ template "slack.title" . }}'
        text: '{{ template "slack.text" . }}'
        
    - name: 'critical-alerts'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#security-critical'
        send_resolved: true
        title: '[CRITICAL] {{ .CommonAnnotations.summary }}'
        text: '{{ .CommonAnnotations.description }}'
      pagerduty_configs:
      - routing_key: 'YOUR_PAGERDUTY_ROUTING_KEY'
        description: '{{ .CommonAnnotations.description }}'
        
    - name: 'warning-alerts'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#security-warnings'
        send_resolved: true
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: falco-webhook-handler
  namespace: security
spec:
  replicas: 2
  selector:
    matchLabels:
      app: falco-webhook
  template:
    metadata:
      labels:
        app: falco-webhook
    spec:
      containers:
      - name: webhook-handler
        image: security/falco-webhook-handler:latest
        ports:
        - containerPort: 8080
        env:
        - name: SLACK_WEBHOOK_URL
          valueFrom:
            secretKeyRef:
              name: alert-secrets
              key: slack-webhook
        - name: PAGERDUTY_ROUTING_KEY
          valueFrom:
            secretKeyRef:
              name: alert-secrets
              key: pagerduty-key
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
```

### 5.2 自动化响应机制

#### 安全事件自动化处理

```yaml
# 08-automated-response.yaml
apiVersion: security.k8s.io/v1
kind: AutomatedResponse
metadata:
  name: security-incident-response
spec:
  triggers:
  - name: "critical-process-violation"
    condition: |
      alert.severity == "CRITICAL" AND 
      alert.rule == "Privileged Container Escape Attempt"
    actions:
    - name: "isolate-container"
      type: "network"
      parameters:
        policy: "deny-all-egress"
        duration: "1h"
    - name: "terminate-process"
      type: "process"
      parameters:
        signal: "SIGKILL"
    - name: "notify-security-team"
      type: "notification"
      parameters:
        channel: "pagerduty"
        urgency: "high"
        
  - name: "malware-detection"
    condition: |
      alert.rule == "Cryptominer Activity Detected" OR
      alert.tags contains "malware"
    actions:
    - name: "quarantine-pod"
      type: "workload"
      parameters:
        scale_to: 0
        backup_enabled: true
    - name: "capture-forensics"
      type: "forensics"
      parameters:
        duration: "30m"
    - name: "scan-cluster"
      type: "scan"
      parameters:
        scope: "cluster-wide"
        
  - name: "suspicious-network-activity"
    condition: |
      alert.rule == "Suspicious Network Connection" AND
      alert.priority >= "WARNING"
    actions:
    - name: "block-ip"
      type: "network"
      parameters:
        ip: "{{alert.dest_ip}}"
        duration: "24h"
    - name: "log-analysis"
      type: "analysis"
      parameters:
        timeframe: "1h"
```

这份安全扫描与检测工具文档提供了完整的运行时安全检测解决方案，涵盖了主流工具的配置和实际应用场景。