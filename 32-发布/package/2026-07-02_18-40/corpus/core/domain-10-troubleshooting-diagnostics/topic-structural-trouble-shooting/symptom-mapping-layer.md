---
title: 症状快速映射层 (Symptom-SOP-RootCause Mapping) [topic-structural-trouble-shooting]
description: 'description: ''**适用场景**: AI Agent + 人工运维快速定位排查路径'''
summary: 'description: ''**适用场景**: AI Agent + 人工运维快速定位排查路径'''
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- etcd
- apiserver
- kubelet
- prometheus
- grafana
- jaeger
- istio
- envoy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 1h
intent_queries:
- 症状快速映射层 (Symptom-SOP-RootCause Mapping) 是什么
- 如何 症状快速映射层 (Symptom-SOP-RootCause Mapping)
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 症状快速映射层 (Symptom-SOP-RootCause Mapping) 故障排查
- 症状快速映射层 (Symptom-SOP-RootCause Mapping) 排障步骤
trigger_keywords:
- 症状快速映射层
- Symptom-SOP-RootCause
- Mapping
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
- backup-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 症状快速映射层 (Symptom-SOP-RootCause Mapping)
description: '**适用场景**: AI Agent + 人工运维快速定位排查路径'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- grafana
- istio
- envoy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 症状快速映射层 (Symptom-SOP-RootCause Mapping) 是什么
- 如何 症状快速映射层 (Symptom-SOP-RootCause Mapping)
- 症状快速映射层 (Symptom-SOP-RootCause Mapping) 故障排查
- 症状快速映射层 (Symptom-SOP-RootCause Mapping) 排障步骤
trigger_keywords:
- 症状快速映射层
- Symptom-SOP-RootCause
- Mapping
- structural
- trouble
- shooting
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

# 症状快速映射层 (Symptom-SOP-RootCause Mapping)

> **版本**: v1.0
> **适用场景**: AI Agent + 人工运维快速定位排查路径
> **更新日期**: 2026-05-18

---

<!-- chunk: 一、映射引擎设计 -->## 一、映射引擎设计

## 1.1 设计原则

```
症状输入 → 向量化匹配 → 候选路径排序 → 执行验证 → 根因确认

设计目标:
  1. 机器可读: AI Agent 可直接解析和执行
  2. 人类可读: 运维人员可快速理解
  3. 可扩展: 易于添加新的症状模式
  4. 可验证: 每个映射都有明确的验证条件
```

## 1.2 输入 Schema

```yaml
symptom_input:
  # 必填字段
  primary_symptom: string      # 主要症状，如 "Pod CrashLoopBackOff"
  secondary_symptoms: [string]  # 伴随症状，如 ["OOMKilled", "Exit Code 137"]

  # 可选字段
  context:
    namespace: string          # 命名空间
    workload_type: string      # 工作负载类型 (Deployment/StatefulSet/...)
    cluster_type: string       # 集群类型 (ACK/自建K8s/混合云)
    cloud_provider: string     # 云厂商 (Alibaba/AWS/GCP/Azure)

  observable:
    error_logs: [string]       # 错误日志片段
    exit_code: integer         # 容器退出码
    events: [string]           # Kubernetes Events
    metrics: object            # 相关指标

  urgency: enum[P0/P1/P2]     # 紧急程度
```

## 1.3 输出 Schema

```yaml
diagnosis_output:
  ranked_paths:
    - path_id: string
      probability: float       # 匹配概率 0.0-1.0
      root_cause: string       # 推测根因
      fta_path: string         # FTA 路径，如 "TE-2 → IE-2.1 → BE-2.3"
      confidence: float        # 置信度 0.0-1.0

      diagnostic_steps:         # 诊断步骤
        - step: integer
          command: string       # 执行的命令
          expected_result: string  # 期望结果
          validation: string    # 如何验证结果

      related_docs:             # 相关文档
        - path: string
          type: enum[structural/domain/skill/febm]
          relevance: float

      auto_heal_actions:         # 可自动执行的修复动作
        - action_id: string
          risk_level: enum[low/medium/high]
          auto_executable: boolean
          requires_approval: boolean

  unknown_symptom_flag: boolean  # 是否为未知症状
  recommended_escalation: string  # 升级建议（当 unknown_symptom_flag=true 时）
```

---

<!-- chunk: 二、症状快速映射表 -->## 二、症状快速映射表

## 2.1 Pod 相关症状

```yaml
symptom_mappings:

  # ============================================================================
  # Pod CrashLoopBackOff
  # ============================================================================
  - symptom: "Pod CrashLoopBackOff"
    category: "workload"
    aliases:
      - "Pod 反复重启"
      - "CrashLoopBackOff"
      - "容器启动失败反复重启"

    intent_queries:
      - "Pod 一直崩溃重启"
      - "container is restarting repeatedly"
      - "CrashLoopBackOff是什么原因"
      - "Pod启动后几秒就崩了"
      - "容器退出码137"
      - "Exit Code 137"
      - "OOMKilled和CrashLoop区别"
      - "应用容器不断崩溃"

    diagnostic_decision_tree:
      - condition: "存在 OOMKilled 日志或 Exit Code 137"
        likely_root_cause: "OOMKilled - 内存超出限制"
        fta_path: "TE-2 → IE-2.1 → BE-2.3"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe pod {namespace}/{pod} | grep -A5 'Last State'"
            expected: "Last State: Terminated, Reason: OOMKilled"
          - command: "kubectl top pod {namespace}/{pod} --containers"
            expected: "memory usage > 90% of limit"

      - condition: "存在 'Error' 或 'CrashLoopBackOff' 日志但无 OOM"
        likely_root_cause: "应用配置错误或启动命令错误"
        fta_path: "TE-2 → IE-2.1 → BE-2.1"
        confidence: 0.70
        verification_steps:
          - command: "kubectl logs {namespace}/{pod} --previous --tail=100"
            expected: "应用错误日志（非 OOM）"
          - command: "kubectl describe pod {namespace}/{pod} | grep -A10 'Containers'"
            expected: "配置错误（如端口/路径/环境变量）"

      - condition: "ImagePullBackOff 错误"
        likely_root_cause: "镜像拉取失败"
        fta_path: "TE-2 → IE-2.1 → BE-2.2"
        confidence: 0.90
        verification_steps:
          - command: "kubectl describe pod {namespace}/{pod} | grep -i 'image'"
            expected: "BackOff 原因（如 404/认证失败/网络不可达）"

      - condition: "Evicted 状态"
        likely_root_cause: "节点压力驱逐"
        fta_path: "TE-2 → IE-2.1 → BE-2.4"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe pod {namespace}/{pod} | grep -A5 'Events'"
            expected: "Reason: Evicted, Message: 节点资源压力"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#三te-2-应用服务不可用-p0"
        type: "fta"
        relevance: 0.90
      - path: "domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md"
        type: "domain"
        relevance: 0.90

    auto_heal_actions:
      - action_id: "HA-2.3.1"
        description: "增加内存 limits（当确认为 OOM 时）"
        risk_level: "low"
        auto_executable: true
        command: |
          kubectl patch deployment {deployment} -n {namespace} -p \
            '{"spec":{"template":{"spec":{"containers":[{
              "name":"{container}",
              "resources":{"limits":{"memory":"2Gi"}}}]}}}}'

  # ============================================================================
  # Pod Pending (调度失败)
  # ============================================================================
  - symptom: "Pod Pending"
    category: "scheduling"
    aliases:
      - "Pod 处于 Pending 状态"
      - "调度失败"
      - "Pod 无法调度"

    intent_queries:
      - "Pod一直卡在Pending状态"
      - "pod cannot be scheduled"
      - "调度失败是什么原因"
      - "FailedScheduling"
      - "Pod无法分配到节点"
      - "no nodes available"
      - "insufficient cpu memory"
      - "节点资源不足导致调度失败"
      - "Pod调度超时"

    diagnostic_decision_tree:
      - condition: "kubectl describe 显示 'FailedScheduling' + 'insufficient cpu/memory'"
        likely_root_cause: "节点资源不足"
        fta_path: "TE-3 → IE-3.1 → BE-3.1"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe node {node}"
            expected: "Allocated resources > 90%"
          - command: "kubectl top nodes"
            expected: "CPU/Memory 使用率 > 90%"

      - condition: "kubectl describe 显示 'FailedScheduling' + 'node selector/affinity'"
        likely_root_cause: "节点选择器/亲和性不匹配"
        fta_path: "TE-3 → IE-3.1 → BE-3.2"
        confidence: 0.80
        verification_steps:
          - command: "kubectl describe pod {namespace}/{pod} | grep -A5 'Node Selector'"
            expected: "节点选择器标签无匹配节点"
          - command: "kubectl get nodes --show-labels"
            expected: "节点标签与 Pod nodeSelector 不匹配"

      - condition: "kubectl describe 显示 'FailedScheduling' + 'taint/toleration'"
        likely_root_cause: "污点阻止调度"
        fta_path: "TE-3 → IE-3.1 → BE-3.3"
        confidence: 0.80
        verification_steps:
          - command: "kubectl describe pod {namespace}/{pod} | grep -i 'taint'"
            expected: "节点污点无对应 Toleration"

      - condition: "kubectl describe 显示 'FailedScheduling' + 'quota'"
        likely_root_cause: "命名空间资源配额超限"
        fta_path: "TE-3 → IE-3.1 → BE-3.4"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe namespace {namespace} | grep -A5 'ResourceQuota'"
            expected: "CPU/Memory quota 耗尽"
          - command: "kubectl describe resourcequota -n {namespace}"
            expected: "hard limit reached"

      - condition: "kubectl describe 显示 'FailedScheduling' + 'PVC pending'"
        likely_root_cause: "存储卷挂载等待"
        fta_path: "TE-3 → IE-3.3 → BE-3.9"
        confidence: 0.75
        verification_steps:
          - command: "kubectl describe pod {namespace}/{pod} | grep -i 'volume'"
            expected: "PendingContainer, 等待 PVC 绑定"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#四te-3-pod启动失败-p1"
        type: "fta"
        relevance: 0.90

  # ============================================================================
  # Pod OOMKilled
  # ============================================================================
  - symptom: "Pod OOMKilled"
    category: "resource"
    aliases:
      - "OOMKilled"
      - "内存溢出被杀死"
      - "Exit Code 137"

    intent_queries:
      - "Pod被OOMKilled"
      - "OOMKilled exit code 137"
      - "内存超限被杀死"
      - "container memory limit exceeded"
      - "java outofmemory in kubernetes"
      - "JVM heap大于容器limit"
      - "内存泄漏导致OOM"
      - "HPA扩容后内存不足"
      - "应用内存持续接近limit"

    diagnostic_decision_tree:
      - condition: "kubectl describe 显示 'OOMKilled' + memory usage 接近 limit"
        likely_root_cause: "应用内存泄漏或 limit 设置不当"
        fta_path: "TE-2 → IE-2.1 → BE-2.3"
        confidence: 0.90
        verification_steps:
          - command: "kubectl top pod {namespace}/{pod} --containers"
            expected: "memory usage > 95% of limit"
          - command: "kubectl logs {namespace}/{pod} --previous | grep -i 'outofmemory'"
            expected: "OutOfMemoryError 或类似日志"

      - condition: "JVM 应用 + memory usage 持续接近 limit"
        likely_root_cause: "JVM heap 配置 > container limit"
        fta_path: "TE-2 → IE-2.1 → BE-2.3.2"
        confidence: 0.85
        verification_steps:
          - command: "kubectl exec {namespace}/{pod} -- env | grep -i 'heap'"
            expected: "JAVA_OPTS 或 JAVA_TOOL_OPTIONS 中 heap 设置"
          - command: "kubectl exec {namespace}/{pod} -- jcmd GC.heap_info"
            expected: "JVM heap 使用量接近容器 limit"

      - condition: "HPA 扩容后出现 OOM"
        likely_root_cause: "HPA 扩容后连接池配置未同步调整"
        fta_path: "TE-2 → IE-2.1 → BE-2.3.3"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get hpa {deployment} -n {namespace}"
            expected: "当前副本数 > 原始副本数"
          - command: "检查应用连接池配置"
            expected: "maxConnections 固定，未随副本数扩容"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-oom-memory-diagnosis.md"
        type: "structural"
        relevance: 0.98
      - path: "domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md"
        type: "domain"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#三te-2-应用服务不可用-p0"
        type: "fta"
        relevance: 0.90

    auto_heal_actions:
      - action_id: "HA-2.3.1"
        description: "增加内存 limits"
        risk_level: "low"
        auto_executable: true
        command: |
          kubectl patch deployment {deployment} -n {namespace} -p \
            '{"spec":{"template":{"spec":{"containers":[{
              "name":"{container}",
              "resources":{"limits":{"memory":"2Gi"}}}]}}}}'

  # ============================================================================
  # Node NotReady
  # ============================================================================
  - symptom: "Node NotReady"
    category: "node"
    aliases:
      - "节点 NotReady"
      - "节点不可用"
      - "kubelet 问题"

    intent_queries:
      - "节点显示NotReady"
      - "node is not ready"
      - "kubelet stopped posting status"
      - "节点kubelet进程崩溃"
      - "节点网络分区"
      - "Node Unknown状态"
      - "节点被驱逐"
      - "DiskPressure MemoryPressure"
      - "节点磁盘空间不足"
      - "节点SSH无法连接"

    diagnostic_decision_tree:
      - condition: "kubectl describe node 显示 'KubeletNotReady' + 'PLEG'"
        likely_root_cause: "kubelet 或容器运行时问题"
        fta_path: "TE-1 → IE-1.2 → BE-1.5"
        confidence: 0.85
        verification_steps:
          - command: "journalctl -u kubelet -n 100 | grep -i 'error'"
            expected: "kubelet 错误日志"
          - command: "systemctl status kubelet"
            expected: "kubelet 服务状态"

      - condition: "kubectl describe node 显示 'NetworkReady' + 'route failed'"
        likely_root_cause: "节点网络问题"
        fta_path: "TE-1 → IE-1.2 → BE-1.7"
        confidence: 0.80
        verification_steps:
          - command: "journalctl -u kubelet -n 50 | grep -i 'network'"
            expected: "网络相关错误"
          - command: "ip route"
            expected: "路由表异常"

      - condition: "节点磁盘压力"
        likely_root_cause: "节点磁盘空间不足"
        fta_path: "TE-1 → IE-1.2 → BE-1.7.2"
        confidence: 0.85
        verification_steps:
          - command: "df -h /"
            expected: "根分区使用率 > 90%"
          - command: "docker system df"
            expected: "Docker 磁盘占用过大"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/06-node-notready-diagnosis.md"
        type: "structural"
        relevance: 0.98
      - path: "domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md"
        type: "domain"
        relevance: 0.95

  # ============================================================================
  # Service 不可用
  # ============================================================================
  - symptom: "Service 不可用"
    category: "network"
    aliases:
      - "Service 无法访问"
      - "Endpoint 为空"
      - "Service timeout"

    intent_queries:
      - "Service无法访问"
      - "service endpoint is empty"
      - "Service超时"
      - "kube-proxy异常"
      - "ClusterIP无法连接"
      - "Headless Service解析失败"
      - "ExternalName Service不工作"
      - "LoadBalancer IP获取失败"
      - "Ingress访问502 Bad Gateway"
      - "Service端口不通"

    diagnostic_decision_tree:
      - condition: "kubectl get endpoints 显示为空"
        likely_root_cause: "Pod 未就绪或 Selector 不匹配"
        fta_path: "TE-2 → IE-2.2 → BE-2.5"
        confidence: 0.90
        verification_steps:
          - command: "kubectl get pods -l {selector} -n {namespace}"
            expected: "无 Running 状态的 Pod"
          - command: "kubectl describe service {name} -n {namespace} | grep -i 'endpoints'"
            expected: "endpoints 为空或 selector 不匹配"

      - condition: "Endpoints 存在但无法连接"
        likely_root_cause: "kube-proxy 或网络策略问题"
        fta_path: "TE-2 → IE-2.2 → BE-2.7"
        confidence: 0.75
        verification_steps:
          - command: "kubectl get pods -n kube-system -l k8s-app=kube-proxy"
            expected: "kube-proxy Pod 正常 Running"
          - command: "iptables -L -n -t nat | grep {service_ip}"
            expected: "存在 NAT 规则"

      - condition: "Ingress 访问失败"
        likely_root_cause: "Ingress Controller 或规则配置问题"
        fta_path: "TE-2 → IE-2.3 → BE-2.8"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get pods -n ingress-nginx"
            expected: "Ingress Controller Pod Running"
          - command: "kubectl describe ingress {name} -n {namespace}"
            expected: "Ingress 配置正确，后端 Service 正常"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-service-comprehensive-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#三te-2-应用服务不可用-p0"
        type: "fta"
        relevance: 0.90

  # ============================================================================
  # DNS 解析失败
  # ============================================================================
  - symptom: "DNS 解析失败"
    category: "network"
    aliases:
      - "DNS 不可用"
      - "nslookup 失败"
      - "CoreDNS 异常"

    intent_queries:
      - "DNS解析失败"
      - "DNS lookup failed"
      - "nslookup超时"
      - "CoreDNS Pod不运行"
      - "kube-dns不可用"
      - "域名无法解析"
      - "DNS_PROBE_FINISHED_NXDOMAIN"
      - "DNS_PROBE_FINISHED_NO_INTERNET"
      - "Pod内ping不通域名"
      - "集群内DNS解析异常"

    diagnostic_decision_tree:
      - condition: "CoreDNS Pod 不 Running"
        likely_root_cause: "CoreDNS Pod 问题"
        fta_path: "TE-4 → IE-4.1 → BE-4.1"
        confidence: 0.90
        verification_steps:
          - command: "kubectl get pods -n kube-system -l k8s-app=kube-dns"
            expected: "所有 CoreDNS Pod Running"
          - command: "kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50"
            expected: "CoreDNS 日志无异常"

      - condition: "DNS 配置错误"
        likely_root_cause: "kube-dns ConfigMap 配置问题"
        fta_path: "TE-4 → IE-4.1 → BE-4.2"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get configmap kube-dns -n kube-system -o yaml"
            expected: "upstreamServers 和 stubDomains 配置正确"

      - condition: "网络策略阻断 DNS"
        likely_root_cause: "NetworkPolicy 误阻断 CoreDNS"
        fta_path: "TE-4 → IE-4.1 → BE-4.3"
        confidence: 0.75
        verification_steps:
          - command: "kubectl get networkpolicy -n kube-system"
            expected: "存在允许 CoreDNS 流量的策略"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md"
        type: "structural"
        relevance: 0.98
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#五te-4-网络通信异常-p1"
        type: "fta"
        relevance: 0.90

  # ============================================================================
  # PVC 挂载失败
  # ============================================================================
  - symptom: "PVC 挂载失败"
    category: "storage"
    aliases:
      - "PVC Pending"
      - "卷挂载失败"
      - "ContainerCreation 失败"

    intent_queries:
      - "PVC挂载失败"
      - "persistentvolumeclaim pending"
      - "volume mount failed"
      - "存储卷挂载异常"
      - "CSI驱动异常"
      - "PVC绑定超时"
      - "StorageClass不存在"
      - "Dynamically provisioned volume failed"
      - "挂载点不存在"
      - "ReadWriteMany访问模式异常"

    diagnostic_decision_tree:
      - condition: "PVC 处于 Pending 状态"
        likely_root_cause: "StorageClass 配置错误或 PV 资源不足"
        fta_path: "TE-5 → IE-5.1 → BE-5.1"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe pvc {name} -n {namespace}"
            expected: "Events 中显示 Pending 原因"

      - condition: "CSI 驱动异常"
        likely_root_cause: "CSI Driver 未就绪或配置错误"
        fta_path: "TE-5 → IE-5.1 → BE-5.3"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get pods -n kube-system | grep csi"
            expected: "CSI Driver Pod Running"
          - command: "kubectl describe csidriver"
            expected: "CSI Driver 配置正确"

      - condition: "挂载参数错误"
        likely_root_cause: "mountOptions 不兼容或路径不存在"
        fta_path: "TE-5 → IE-5.2 → BE-5.4"
        confidence: 0.75
        verification_steps:
          - command: "kubectl describe pvc {name} | grep -A10 'Mount Options'"
            expected: "mountOptions 与文件系统兼容"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#六te-5-存储访问失败-p1"
        type: "fta"
        relevance: 0.90

  # ============================================================================
  # 证书过期
  # ============================================================================
  - symptom: "证书过期"
    category: "security"
    aliases:
      - "TLS handshake error"
      - "certificate has expired"
      - "x509"

    intent_queries:
      - "证书过期"
      - "TLS handshake error"
      - "certificate has expired"
      - "x509 certificate expired"
      - "API Server证书过期"
      - "kubelet证书过期"
      - "etcd证书异常"
      - "SSL handshake failed"
      - "证书链验证失败"
      - "Kubernetes API认证失败"

    diagnostic_decision_tree:
      - condition: "API Server 无法连接"
        likely_root_cause: "API Server 证书过期"
        fta_path: "TE-7 → IE-7.1 → BE-7.1.1"
        confidence: 0.90
        verification_steps:
          - command: "openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates"
            expected: "notBefore/notAfter，当前时间在有效期内"
          - command: "curl -k https://{apiserver}:6443/healthz"
            expected: "返回 'ok'"

      - condition: "kubectl 无法认证"
        likely_root_cause: "kubelet 证书过期"
        fta_path: "TE-7 → IE-7.1 → BE-7.5.2"
        confidence: 0.85
        verification_steps:
          - command: "openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates"
            expected: "证书在有效期内"

      - condition: "SLB HTTPS 失败"
        likely_root_cause: "阿里云 SLB 证书问题"
        fta_path: "TE-7 → IE-7.1 → BE-7.1.3"
        confidence: 0.80
        verification_steps:
          - command: "aliyun slb describe CACertificate --region {region}"
            expected: "证书状态 Active"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/06-security-auth/02-certificate-troubleshooting.md"
        type: "structural"
        relevance: 0.98
      - path: "domain-10-troubleshooting-diagnostics/13-certificate-troubleshooting.md"
        type: "domain"
        relevance: 0.95

  # ============================================================================
  # etcd 问题
  # ============================================================================
  - symptom: "etcd 问题"
    category: "control-plane"
    aliases:
      - "etcd 不可用"
      - "etcd leader 丢失"
      - "etcd 磁盘满"

    intent_queries:
      - "etcd集群不可用"
      - "etcd leader election failed"
      - "etcd disk space exhausted"
      - "etcd数据库写入失败"
      - "etcd connection refused"
      - "etcd请求超时"
      - "etcd db size quota exceeded"
      - "failed to commit write transaction"
      - "etcd mvcc database too large"
      - "etcd集群脑裂"

    diagnostic_decision_tree:
      - condition: "etcd leader 丢失"
        likely_root_cause: "etcd 仲裁丢失"
        fta_path: "TE-1 → IE-1.1 → BE-1.2 → BE-1.2.2"
        confidence: 0.90
        verification_steps:
          - command: "kubectl exec -n kube-system etcd-{node} -- etcdctl endpoint status"
            expected: "存在 IS_LEADER=true 的节点"
          - command: "kubectl exec -n kube-system etcd-{node} -- etcdctl endpoint health"
            expected: "所有 endpoint is healthy"

      - condition: "etcd 磁盘空间告警"
        likely_root_cause: "etcd 磁盘空间耗尽"
        fta_path: "TE-1 → IE-1.1 → BE-1.2 → BE-1.2.1"
        confidence: 0.95
        verification_steps:
          - command: "kubectl exec -n kube-system etcd-{node} -- etcdctl endpoint status"
            expected: "DB size 接近 quota"
          - command: "df -h /var/lib/etcd"
            expected: "磁盘使用率 > 80%"

      - condition: "API Server 响应超时"
        likely_root_cause: "etcd 性能降级"
        fta_path: "TE-1 → IE-1.1 → BE-1.2 → BE-1.2.4"
        confidence: 0.85
        verification_steps:
          - command: "kubectl exec -n kube-system etcd-{node} -- etcdctl check perf"
            expected: "DB 性能正常"
          - command: "curl -s --max-time 5 https://{apiserver}:2379/metrics | grep etcd"
            expected: "etcd 指标正常"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/02-etcd-troubleshooting.md"
        type: "structural"
        relevance: 0.98
      - path: "domain-01-cluster-fundamentals/02-etcd-troubleshooting.md"
        type: "domain"
        relevance: 0.95
      - path: "[[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/13-etcd-maintenance|10-etcd-maintenance]].md"
        type: "domain"
        relevance: 0.90

  # ============================================================================
  # Terway 网络问题 (ACK 特有)
  # ============================================================================
  - symptom: "Terway 网络问题"
    category: "network"
    cluster_type: "ACK"
    aliases:
      - "Pod 无 IP"
      - "ENI 分配失败"
      - "Terway 异常"

    intent_queries:
      - "Pod无法获取IP"
      - "Terway ENI分配失败"
      - "Pod IP分配失败阿里云"
      - "failed to allocate pod IP"
      - "ENI quota exceeded"
      - "aliyun eni ip exhausted"
      - "Terway网络模式切换失败"
      - "VPC CIDR耗尽"
      - "ACK Pod IP无法分配"
      - "弹性网卡配额不足"

    diagnostic_decision_tree:
      - condition: "Pod 无法获取 IP"
        likely_root_cause: "ENI 多队列压力或 IPAM 问题"
        fta_path: "TE-9 → IE-9.1 → BE-9.1"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe pod {pod} | grep -i 'eni'"
            expected: "IP 已分配（如果未分配显示原因）"
          - command: "kubectl logs -n kube-system -l app=terway --tail=50"
            expected: "Terway 日志无异常"

      - condition: "VPC CIDR 耗尽"
        likely_root_cause: "子网容量不足"
        fta_path: "TE-9 → IE-9.1 → BE-9.2.1"
        confidence: 0.90
        verification_steps:
          - command: "aliyun vpc DescribeVpcs --region {region}"
            expected: "VPC CIDR 剩余容量充足"
          - command: "aliyun vpc DescribeVSwitches --VpcId {vpc_id}"
            expected: "交换机可用 IP 数量 > Pod 需求"

      - condition: "Pod 间网络不通"
        likely_root_cause: "ENI 带宽瓶颈或安全组冲突"
        fta_path: "TE-9 → IE-9.1 → BE-9.1.1"
        confidence: 0.80
        verification_steps:
          - command: "aliyun ecs DescribeInstances --region {region}"
            expected: "ENI 绑定数未超限"
          - command: "kubectl exec {pod} -- ip addr"
            expected: "Pod IP 配置正确"

    related_docs:
      - path: "domain-03-networking-traffic/42-terway-usage-guide.md"
        type: "structural"
        relevance: 0.90
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#十te-9-terway-网络问题-p1-新增"
        type: "fta"
        relevance: 0.95

  # ============================================================================
  # ASM 服务网格问题 (ACK 特有)
  # ============================================================================
  - symptom: "ASM 服务网格问题"
    category: "service-mesh"
    cluster_type: "ACK"
    aliases:
      - "Envoy sidecar 异常"
      - "xDS 配置推送失败"
      - "mTLS 失败"

    intent_queries:
      - "Envoy sidecar无法连接"
      - "Istio xDS配置推送失败"
      - "mTLS证书握手失败"
      - "istio-proxy初始化失败"
      - "Envoy连接重置"
      - "pilot-agent status异常"
      - "Istio流量全部中断"
      - "ASM服务网格不可用"
      - "Citadel证书签发失败"
      - "DestinationRule配置不生效"

    diagnostic_decision_tree:
      - condition: "Envoy sidecar 无法连接"
        likely_root_cause: "Envoy 资源耗尽或健康检查失败"
        fta_path: "TE-10 → IE-10.1 → BE-10.1"
        confidence: 0.85
        verification_steps:
          - command: "kubectl exec -n {namespace} {pod} -c istio-proxy -- pilot-agent status"
            expected: "Agent 状态正常，xDS 接收成功"
          - command: "kubectl logs -n {namespace} {pod} -c istio-proxy --tail=50"
            expected: "Envoy 日志无异常"

      - condition: "xDS 配置推送失败"
        likely_root_cause: "Istiod OOM 或配置错误"
        fta_path: "TE-10 → IE-10.2 → BE-10.3"
        confidence: 0.85
        verification_steps:
          - command: "kubectl get pods -n istio-system -l app=istiod"
            expected: "Istiod Pod Running"
          - command: "kubectl logs -n istio-system istiod-* --tail=50"
            expected: "Istiod 无错误日志"

      - condition: "mTLS 证书问题"
        likely_root_cause: "Citadel 证书签发失败"
        fta_path: "TE-10 → IE-10.2 → BE-10.3.2"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get secrets -n {namespace} | grep 'istio' | grep 'cert'"
            expected: "mTLS 证书存在且未过期"
          - command: "kubectl exec -n {namespace} {pod} -c istio-proxy -- openssl s_client -connect {upstream}:443"
            expected: "mTLS 握手成功"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#十一te-10-asm-服务网格问题-p1-新增"
        type: "fta"
        relevance: 0.95

  # ============================================================================
  # 监控告警异常 (Prometheus/ARMS)
  # ============================================================================
  - symptom: "监控告警异常"
    category: "observability"
    aliases:
      - "Prometheus 不可用"
      - "指标丢失"
      - "告警未触发"

    intent_queries:
      - "Prometheus指标丢失"
      - "监控告警未触发"
      - "alertmanager无法发送告警"
      - "Prometheus OOM"
      - "指标采集失败"
      - "监控面板空白"
      - "ARMS数据中断"
      - "Grafana无法连接Prometheus"
      - "自定义指标暴露失败"
      - "监控数据延迟过高"

    diagnostic_decision_tree:
      - condition: "Prometheus OOM 或无法抓取指标"
        likely_root_cause: "Prometheus 内存压力或 WAL 损坏"
        fta_path: "TE-8 → IE-8.1 → BE-8.1"
        confidence: 0.85
        verification_steps:
          - command: "kubectl top pod -n monitoring prometheus-*"
            expected: "内存使用率 < 90%"
          - command: "kubectl logs -n monitoring prometheus-* --tail=50 | grep -i 'error'"
            expected: "无 OOM 或 WAL 损坏错误"

      - condition: "ARMS 数据丢失 (ACK)"
        likely_root_cause: "ARMS Java Agent 注入失败或采集端异常"
        fta_path: "TE-8 → IE-8.4 → BE-8.10"
        confidence: 0.80
        verification_steps:
          - command: "kubectl exec -n {namespace} {pod} -- ls -la /usr/local/arms"
            expected: "ARMS Agent 文件存在"
          - command: "kubectl exec -n {namespace} {pod} -- ps aux | grep -i 'arms'"
            expected: "ARMS Agent 进程运行中"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md#九te-8-监控告警异常-p2"
        type: "fta"
        relevance: 0.90

  # ============================================================================
  # Ingress 访问异常
  # ============================================================================
  - symptom: "Ingress 访问异常"
    category: "network"
    aliases:
      - "Ingress 502"
      - "Ingress 无法访问"
      - "Ingress 404"

    intent_queries:
      - "Ingress 502 Bad Gateway"
      - "Ingress访问超时"
      - "nginx-ingress-controller无法访问后端"
      - "Ingress class not found"
      - "ALB Ingress配置错误"
      - "Ingress证书TLS握手失败"
      - "Ingress路径路由失败"
      - "Ingress域名解析异常"
      - "CLB Ingress无法绑定"
      - "Ingress健康检查失败"

    diagnostic_decision_tree:
      - condition: "Ingress 返回 502"
        likely_root_cause: "Ingress Controller 无法连接后端 Service"
        fta_path: "TE-2 → IE-2.3 → BE-2.8"
        confidence: 0.90
        verification_steps:
          - command: "kubectl get pods -n ingress-nginx"
            expected: "Ingress Controller Pod Running"
          - command: "kubectl describe ingress {name} -n {namespace}"
            expected: "Ingress 配置正确，backend service 正常"

      - condition: "Ingress 返回 404"
        likely_root_cause: "Ingress 路径配置错误或 IngressClass 未找到"
        fta_path: "TE-2 → IE-2.3 → BE-2.8.2"
        confidence: 0.85
        verification_steps:
          - command: "kubectl get ingressclass"
            expected: "IngressClass 存在"
          - command: "kubectl describe ingress {name}"
            expected: "annotations 中 ingress-class 正确"

      - condition: "Ingress TLS 握手失败"
        likely_root_cause: "证书配置错误或 Secret 不存在"
        fta_path: "TE-7 → IE-7.1 → BE-7.3"
        confidence: 0.90
        verification_steps:
          - command: "kubectl get secret {secret-name} -n {namespace}"
            expected: "Secret 存在且包含 tls.crt 和 tls.key"
          - command: "openssl s_client -connect {domain}:443"
            expected: "证书链验证成功"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md"
        type: "structural"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md"
        type: "fta"
        relevance: 0.90

  # ============================================================================
  # 数据库连接异常
  # ============================================================================
  - symptom: "数据库连接异常"
    category: "data"
    aliases:
      - "MySQL 连接失败"
      - "PostgreSQL 无法连接"
      - "数据库 timeout"

    intent_queries:
      - "MySQL连接被拒绝"
      - "database connection timeout"
      - "PostgreSQL无法连接"
      - "数据库连接池耗尽"
      - "Too many connections"
      - "Redis连接失败"
      - "MongoDB副本集连接异常"
      - "数据库DNS解析失败"
      - "应用无法连接数据库"
      - "数据库证书验证失败"

    diagnostic_decision_tree:
      - condition: "应用日志显示 'connection refused'"
        likely_root_cause: "数据库 Service 未就绪或网络策略阻断"
        fta_path: "TE-2 → IE-2.4 → BE-2.9"
        confidence: 0.85
        verification_steps:
          - command: "kubectl get svc -n {namespace} | grep {db-name}"
            expected: "Service 存在且 ClusterIP 可达"
          - command: "kubectl exec -it {pod} -- nc -zv {svc}:{port}"
            expected: "连接成功"

      - condition: "连接超时"
        likely_root_cause: "数据库负载过高或连接池配置不当"
        fta_path: "TE-2 → IE-2.4 → BE-2.10"
        confidence: 0.80
        verification_steps:
          - command: "kubectl exec -it {pod} -- mysql -h {host} -u {user} -p -e 'show status'"
            expected: "数据库状态正常"
          - command: "检查数据库连接池配置"
            expected: "max_connections 未超限"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting.md"
        type: "structural"
        relevance: 0.85
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md"
        type: "fta"
        relevance: 0.80

  # ============================================================================
  # 缓存服务异常
  # ============================================================================
  - symptom: "缓存服务异常"
    category: "data"
    aliases:
      - "Redis 不可用"
      - "Memcached 连接失败"
      - "缓存数据丢失"

    intent_queries:
      - "Redis cluster unavailable"
      - "Redis连接超时"
      - "Memcached服务异常"
      - "缓存命中率急剧下降"
      - "Redis Sentinel切换失败"
      - "Redis持久化失败"
      - "缓存服务器OOM"
      - "Redis复制中断"
      - "Cache数据不一致"
      - "Redis集群分片失败"

    diagnostic_decision_tree:
      - condition: "Redis 集群节点全部不可达"
        likely_root_cause: "Redis 集群分片或 Sentinel 问题"
        fta_path: "TE-2 → IE-2.4 → BE-2.11"
        confidence: 0.85
        verification_steps:
          - command: "kubectl get pods -l app=redis -n {namespace}"
            expected: "所有 Redis Pod Running"
          - command: "kubectl exec -it {pod} -- redis-cli cluster nodes"
            expected: "所有节点状态正常"

      - condition: "缓存命中率接近 0"
        likely_root_cause: "缓存过期策略或内存不足"
        fta_path: "TE-2 → IE-2.4 → BE-2.12"
        confidence: 0.75
        verification_steps:
          - command: "kubectl exec -it {pod} -- redis-cli info stats | grep hit_rate"
            expected: "命中率 > 50%"
          - command: "kubectl exec -it {pod} -- redis-cli info memory"
            expected: "used_memory < maxmemory"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md"
        type: "structural"
        relevance: 0.80

  # ============================================================================
  # 消息队列异常
  # ============================================================================
  - symptom: "消息队列异常"
    category: "data"
    aliases:
      - "Kafka 消费延迟"
      - "RabbitMQ 连接失败"
      - "消息堆积"

    intent_queries:
      - "Kafka consumer lag持续增长"
      - "RabbitMQ connection refused"
      - "消息队列消费失败"
      - "Kafka分区副本不足"
      - "RabbitMQ channel异常"
      - "RocketMQ nameserver不可用"
      - "消息堆积无法消费"
      - "Kafka leader election失败"
      - "MQTT broker连接超时"
      - "消息队列Consumer Group Rebalance"

    diagnostic_decision_tree:
      - condition: "Kafka consumer lag 持续增长"
        likely_root_cause: "消费者处理速度低于生产速度"
        fta_path: "TE-2 → IE-2.4 → BE-2.13"
        confidence: 0.85
        verification_steps:
          - command: "kubectl exec -it kafka-0 -- kafka-consumer-groups.sh --bootstrap-server localhost:9092 --group {group} --describe"
            expected: "CURRENT-OFFSET 接近 LAST-OFFSET"
          - command: "检查消费者日志"
            expected: "无消费异常"

      - condition: "RabbitMQ 连接失败"
        likely_root_cause: "RabbitMQ Service 或认证问题"
        fta_path: "TE-2 → IE-2.4 → BE-2.14"
        confidence: 0.85
        verification_steps:
          - command: "kubectl get svc rabbitmq -n {namespace}"
            expected: "Service 存在且端口正确"
          - command: "kubectl exec -it rabbitmq-0 -- rabbitmqctl status"
            expected: "RabbitMQ 节点状态 Running"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting.md"
        type: "structural"
        relevance: 0.85

  # ============================================================================
  # 备份失败
  # ============================================================================
  - symptom: "备份失败"
    category: "backup"
    aliases:
      - "Velero 备份失败"
      - "备份任务异常"
      - "Restic 备份报错"

    intent_queries:
      - "Velero备份失败"
      - "backup task failed"
      - "Restic连接超时"
      - "快照创建失败"
      - "Kubernetes备份不可用"
      - "etcd备份失败"
      - "VolumeSnapshot无法创建"
      - "备份存储桶访问异常"
      - "Velero schedule not running"
      - "Backup storage location不可用"

    diagnostic_decision_tree:
      - condition: "Velero Pod 日志显示备份失败"
        likely_root_cause: "存储访问权限或对象存储问题"
        fta_path: "TE-11 → IE-11.1 → BE-11.1"
        confidence: 0.85
        verification_steps:
          - command: "kubectl logs -n velero deployment/velero --tail=50"
            expected: "无错误日志"
          - command: "kubectl get backup -n velero {backup-name}"
            expected: "Phase 为 Completed"

      - condition: "VolumeSnapshot 创建失败"
        likely_root_cause: "CSI Snapshotter 配置问题"
        fta_path: "TE-5 → IE-5.3 → BE-5.8"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get volumesnapshot -n {namespace}"
            expected: "VolumeSnapshot Ready"
          - command: "kubectl describe volumesnapshot -n {namespace}"
            expected: "无错误事件"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta.md"
        type: "fta"
        relevance: 0.95
      - path: "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/04-backup-restore-troubleshooting.md"
        type: "structural"
        relevance: 0.90

  # ============================================================================
  # HPA/VPA 扩缩容异常
  # ============================================================================
  - symptom: "HPA/VPA 扩缩容异常"
    category: "scheduling"
    aliases:
      - "HPA 无法扩容"
      - "VPA 异常"
      - "Pod 副本数不生效"

    intent_queries:
      - "HPA无法扩容"
      - "horizontalpodautoscaler不工作"
      - "VPA推荐值未应用"
      - "HPA replicas为0"
      - "自动扩容失败"
      - "HPA metrics获取失败"
      - "VPA OOM预防不生效"
      - "HPA冷却期异常"
      - "Pod副本数与HPA期望不符"
      - "custom metrics获取失败"

    diagnostic_decision_tree:
      - condition: "HPA 副本数始终为 0"
        likely_root_cause: "指标获取失败或条件不满足"
        fta_path: "TE-3 → IE-3.5 → BE-3.15"
        confidence: 0.85
        verification_steps:
          - command: "kubectl describe hpa {name} -n {namespace}"
            expected: "Conditions 显示 AbleToScale"
          - command: "kubectl top pods -n {namespace}"
            expected: "指标可获取"

      - condition: "VPA 推荐值未自动应用"
        likely_root_cause: "VPA 模式为 Off 或 admission webhook 问题"
        fta_path: "TE-3 → IE-3.5 → BE-3.16"
        confidence: 0.80
        verification_steps:
          - command: "kubectl get vpa {name} -n {namespace} -o yaml | grep updateMode"
            expected: "updateMode 为 Auto"
          - command: "kubectl get pods -n kube-system | grep vpa"
            expected: "VPA admission webhook Pod Running"

    related_docs:
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/list/hpa-fta.md"
        type: "fta"
        relevance: 0.90
      - path: "domain-10-troubleshooting-diagnostics/topic-fta/list/vpa-fta.md"
        type: "fta"
        relevance: 0.85
```

---

<!-- chunk: 三、未知症状处理 -->## 三、未知症状处理

## 3.1 未知症状升级路径

```yaml
unknown_symptom_handling:
  # 当症状无法匹配已知映射时的处理流程

  step_1:
    action: "模糊匹配搜索"
    description: "在症状描述中搜索关键词，匹配部分映射"
    threshold: 0.5  # 关键词匹配度 > 50% 时返回候选

  step_2:
    action: "扩展搜索"
    description: "搜索相近的故障域和文档"
    sources:
      - "topic-structural-trouble-shooting (全部)"
      - "domain-10-troubleshooting-diagnostics (全部)"

  step_3:
    action: "FTA 路径回溯"
    description: "从顶事件开始，遍历故障树寻找可能路径"
    fta_entry_points:
      - "TE-1: 集群完全不可用 (影响整个集群)"
      - "TE-2: 应用服务不可用 (影响特定应用)"
      - "TE-4: 网络通信异常 (网络层面问题)"

  step_4:
    action: "人工升级"
    description: "通知人工运维工程师介入"
    escalation_template: |
      未知症状: {symptom}
      上下文: {context}
      已尝试的匹配: {attempted_matches}
      建议: 请人工排查，可参考 domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/README.md
```

## 3.2 未知症状示例

```
输入: "Pod 的 sidecar 容器无法连接主容器"
匹配度: 0.4 (低于阈值)

处理:
  1. 模糊匹配: 搜索 "sidecar" → 找到 ASM/Istio 相关文档
  2. 扩展搜索: 搜索 "容器间连接" → TE-10 服务网格问题
  3. FTA 回溯: TE-10 → IE-10.1 → BE-10.1 (Envoy 健康检查)
  4. 输出: "可能为 Envoy sidecar 配置问题，建议检查 05-service-mesh-istio-troubleshooting.md"
```

---

<!-- chunk: 四、执行引擎伪代码 -->## 四、执行引擎伪代码

```python
class SymptomMappingEngine:
    """症状映射引擎"""

    def __init__(self, mapping_table, fta_graph, knowledge_base):
        self.mapping_table = mapping_table  # YAML 映射表
        self.fta_graph = fta_graph           # FTA 知识图谱
        self.knowledge_base = knowledge_base # 文档知识库

    def map_symptom(self, symptom_input):
        """
        将症状映射到诊断路径
        """

        # 1. 精确匹配
        exact_match = self.exact_match(symptom_input)
        if exact_match and exact_match.confidence > 0.85:
            return exact_match

        # 2. 模糊匹配
        fuzzy_matches = self.fuzzy_match(symptom_input)

        # 3. FTA 回溯
        if not fuzzy_matches:
            fta_candidates = self.fta_backtrack(symptom_input)

        # 4. 排序并返回
        ranked_paths = self.rank_candidates(
            exact_match,
            fuzzy_matches,
            fta_candidates
        )

        return ranked_paths

    def exact_match(self, symptom_input):
        """精确匹配"""
        primary = symptom_input.primary_symptom
        for mapping in self.mapping_table:
            if primary in mapping.symptom or primary in mapping.aliases:
                # 检查条件是否满足
                for condition in mapping.diagnostic_decision_tree:
                    if self.evaluate_condition(condition, symptom_input):
                        return self.build_diagnosis_output(mapping, condition)
        return None

    def evaluate_condition(self, condition, symptom_input):
        """评估条件是否满足"""
        # 检查 secondary_symptoms, observable, context 等
        return True

    def rank_candidates(self, exact, fuzzy, fta_candidates):
        """对候选路径进行排序"""
        all_candidates = []
        if exact:
            all_candidates.append(exact)
        all_candidates.extend(fuzzy)
        all_candidates.extend(fta_candidates)

        # 按 confidence 排序
        return sorted(all_candidates, key=lambda x: x.confidence, reverse=True)
```

---

<!-- chunk: 五、集成说明 -->## 五、集成说明

## 5.1 与 FTA 的集成

```
症状映射引擎 → FTA 知识图谱

输入: "Pod OOMKilled"
  ↓
映射引擎匹配: BE-2.3 OOMKilled (confidence: 0.90)
  ↓
FTA 路径: TE-2 → IE-2.1 → BE-2.3
  ↓
获取 FTA 元数据:
  - root_causes: ["应用内存泄漏", "JVM heap > limit", ...]
  - healing_actions: [HA-2.3.1, HA-2.3.2, ...]
  - related_docs: [structural/domain/skill/febm]
  ↓
输出完整诊断路径
```

## 5.2 与 Skills 的集成

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
症状映射引擎 → Skills 自动化技能

当 auto_heal_actions 存在时:
  → 检查 skill.auto_executable == true
  → 检查 risk_level 是否允许自动执行
  → 执行 skill.command
  → 验证修复结果

示例:
  症状: Pod OOMKilled
  自动修复: HA-2.3.1 (增加内存 limit)
  → skill: oom-healing-skill
  → 执行: kubectl patch deployment ...
  → 验证: kubectl rollout status
```
---

<!-- chunk: 六、生产级 SLO/SLA 集成 -->## 六、生产级 SLO/SLA 集成

## 6.1 SLO 映射配置

```yaml
slo_integration:
  # 症状 → SLO 影响映射
  slo_mappings:
    - symptom: "Pod CrashLoopBackOff"
      affected_slo:
        - name: "服务可用性"
          target: 99.95%
          current_impact: -0.01%
          alert_threshold: 99.90%

    - symptom: "Node NotReady"
      affected_slo:
        - name: "集群可用性"
          target: 99.99%
          current_impact: -0.05%
          alert_threshold: 99.95%
        - name: "节点数量"
          target: >= 3 nodes
          current_impact: -1 node

    - symptom: "etcd 问题"
      affected_slo:
        - name: "集群可用性"
          target: 99.99%
          current_impact: -0.10%
          alert_threshold: 99.95%
        - name: "API 延迟 P99"
          target: < 500ms
          current_impact: > 2000ms

    - symptom: "Ingress 访问异常"
      affected_slo:
        - name: "API 成功率"
          target: 99.99%
          current_impact: -0.05%
          alert_threshold: 99.95%
        - name: "延迟 P99"
          target: < 1s
          current_impact: > 5s

  # 错误预算策略
  error_budget_policy:
    rolling_window: 30d
    consumed_budget_threshold: 50%  # 消耗 50% 错误预算触发告警
    exhausted_budget_action: "停止非关键变更，强制 review"
```

## 6.2 On-Call 升级流程

```yaml
oncall_escalation:
  # 按 severity 和持续时间自动升级
  P0_critical:
    auto_escalate_after: 5m
    channels:
      - type: "pagerduty"
        timeout: 2m
      - type: "slack_incidents"
        channel: "#incidents-critical"
      - type: "sms"
        level: L1 on-call
    escalation_chain:
      - name: "L1 On-Call SRE"
        timeout: 5m
      - name: "L2 Platform Lead"
        timeout: 10m
      - name: "Engineering Manager"
        timeout: 15m
      - name: "CTO"
        timeout: 30m

  P1_high:
    auto_escalate_after: 15m
    channels:
      - type: "slack_incidents"
        channel: "#incidents-high"
      - type: "pagerduty"
        timeout: 5m
    escalation_chain:
      - name: "L1 On-Call SRE"
        timeout: 15m
      - name: "L2 On-Call SRE"
        timeout: 30m

  P2_medium:
    auto_escalate_after: 1h
    channels:
      - type: "slack_incidents"
        channel: "#incidents-medium"
    escalation_chain:
      - name: "L2 On-Call SRE"
        timeout: 4h
      - name: "Next Business Day Review"
        timeout: 24h
```

## 6.3 运行时告警抑制

```yaml
alert_suppression:
  # 已知告警噪音抑制
  correlated_suppression:
    - alert: "Pod CrashLoopBackOff"
      suppressed_by:
        - alert: "Deployment replicas < desired"
          window: 10m
        - alert: "Service endpoints empty"
          window: 5m

  # 维护窗口抑制
  maintenance_window:
    enabled: true
    annotation_key: "kudig.io/maintenance-window"
    autoSuppress_before: 5m   # 维护开始前 5 分钟自动抑制
    autoSuppress_after: 10m   # 维护结束后 10 分钟自动恢复

  # 变更相关抑制
  change_suppression:
    enabled: true
    suppress_on_change_types:
      - "kubectl rollout"
      - "kubectl apply"
      - "helm upgrade"
    suppression_window: 15m
    annotation_key: "kudig.io/change-id"
```

---

<!-- chunk: 七、生产 Runbook 自动化 -->## 七、生产 Runbook 自动化

## 7.1 Runbook 执行引擎

```yaml
runbook_engine:
  # Runbook 标准格式
  runbook_schema:
    title: string              # Runbook 标题
    check_items: [string]     # 检查项
    fix_steps: [string]       # 修复步骤（含验证点）
    rollback: string          # 回滚方案
    validation: string        # 回归验证

  # 自动执行条件
  auto_execution:
    enabled: true
    preconditions:
      - confidence_threshold: 0.85
      - risk_level: "low"
      - maintenance_window_check: true
      - change_freeze_check: true

  # 执行反馈
  feedback_loop:
    success_notification: "#runbook-success"
    failure_escalation: "#runbook-automation-failed"
    post_execution_survey: true
```

## 7.2 标准 Runbook 模板

```yaml
runbooks:
  - id: "RB-POD-001"
    title: "Pod CrashLoopBackOff 快速恢复"
    trigger:
      symptom: "Pod CrashLoopBackOff"
      confidence_min: 0.85
      risk_level: "low"
    check_items:
      - "kubectl describe pod {ns}/{pod} | grep -A5 'Last State'"
      - "kubectl top pod {ns}/{pod} --containers"
      - "检查应用日志: kubectl logs {ns}/{pod} --previous"
    fix_steps:
      - step: 1
        action: "确认 OOMKilled 后，增加内存 limit"
        command: |
          kubectl patch deployment {deployment} -n {ns} -p \
            '{"spec":{"template":{"spec":{"containers":[{
              "name":"{container}",
              "resources":{"limits":{"memory":"2Gi"}}}]}}}}'
        verification: "kubectl rollout status deployment/{deployment} -n {ns}"
        risk: "medium"

      - step: 2
        action: "如非 OOM，检查应用配置错误"
        command: "kubectl logs {ns}/{pod} --previous | grep -E 'Error|Exception|Failed'"
        verification: "kubectl get pod {ns}/{pod} -o wide"
        risk: "low"

    rollback: "kubectl rollout undo deployment/{deployment} -n {ns}"
    validation: |
      # 回归验证清单
      1. Pod 状态为 Running
      2. 重启次数不再增长
      3. 应用日志无新错误
      4. 健康检查通过

  - id: "RB-NODE-001"
    title: "Node NotReady 快速恢复"
    trigger:
      symptom: "Node NotReady"
      confidence_min: 0.80
      risk_level: "medium"
    check_items:
      - "kubectl describe node {node} | grep -A10 'Conditions'"
      - "journalctl -u kubelet -n 50 | grep -i error"
      - "kubectl get events --field-selector involvedObject.name={node}"
    fix_steps:
      - step: 1
        action: "如果 kubelet 进程异常，重启 kubelet"
        command: "systemctl restart kubelet"
        verification: "systemctl status kubelet"
        risk: "medium"

      - step: 2
        action: "如果节点磁盘压力，清理临时文件"
        command: |
          kubectl debug node/{node} --rm -it --image=ubuntu -- \
            bash -c "docker system prune -af; rm -rf /tmp/*"
        verification: "df -h /"
        risk: "low"

      - step: 3
        action: "如果节点网络分区，标记为 unschedulable 并驱逐 Pod"
        command: |
          kubectl cordon {node}
          kubectl drain {node} --ignore-daemonsets --delete-emptydir-data
        verification: "kubectl get nodes"
        risk: "high"

    rollback: |
      # 如果是误判，手动恢复
      kubectl uncordon {node}
    validation: |
      # 回归验证清单
      1. Node 状态为 Ready
      2. kubelet 服务正常
      3. 磁盘使用率 < 85%
      4. 网络连通性正常
```

---

<!-- chunk: 八、生产事件管理集成 -->## 八、生产事件管理集成

## 8.1 事件生命周期

```yaml
incident_lifecycle:
  # 事件创建
  creation:
    auto_create_from_symptom: true
    min_confidence: 0.75
    min_severity: "P1"
    fields:
      title: "自动: {symptom} @ {affected_service}"
      description: "AI 分析: {diagnosis_output}"
      severity: "{symptom.urgency}"
      affected_service: "{context.workload_type}"
      cluster: "{context.cluster_type}"

  # 事件状态机
  state_machine:
    states:
      - name: "detected"
        transitions_to: ["investigating", "resolved"]
      - name: "investigating"
        transitions_to: ["identified", "mitigated", "resolved"]
      - name: "identified"
        transitions_to: ["mitigated", "resolved"]
      - name: "mitigated"
        transitions_to: ["resolved"]
      - name: "resolved"
        post_actions:
          - "生成事后报告"
          - "更新知识库"
          - "扣除错误预算"

  # 自动化关联
  auto_linking:
    related_docs: true
    related_incidents: true
    related_changes: true
    related_alerts: true
```

## 8.2 事后报告模板

```markdown
# 事后报告 (Postmortem)

<!-- chunk: 事件概要 -->## 事件概要
- **事件 ID**: {incident_id}
- **持续时间**: {start_time} - {end_time} ({duration})
- **影响范围**: {affected_services} ({affected_users})
- **严重程度**: P{severity}
- **状态**: {final_state}

<!-- chunk: 时间线 -->## 时间线
| 时间 | 动作 | 负责人 |
|------|------|--------|
| HH:MM | 事件检测 | AI Agent |
| HH:MM | 初步响应 | On-Call SRE |
| HH:MM | 根因识别 | SRE Lead |
| HH:MM | 修复实施 | SRE |
| HH:MM | 验证完成 | SRE |

<!-- chunk: 根因分析 -->## 根因分析
**根本原因**: {root_cause}

**触发因素**: {triggering_factor}

**为什么没有更早发现**: {detection_gap}

<!-- chunk: 错误预算影响 -->## 错误预算影响
- **消耗**: {error_budget_consumed}%
- **SLO**: {slo_name} ({slo_target})
- **实际达成**: {actual_slo_achieved}

<!-- chunk: 行动项 (Action Items) -->## 行动项 (Action Items)
| 行动项 | 负责人 | 截止日期 | 状态 |
|--------|--------|----------|------|
| 优化告警阈值 | @sre | +7d | open |
| 添加自动恢复 Runbook | @platform | +14d | open |
| 扩充监控覆盖 | @sre | +30d | open |

<!-- chunk: 复盘结论 -->## 复盘结论
- **根因修复**: 需要/不需要
- **预防措施**: 已添加/待添加
- **是否可以自动化**: 是/否
```

---

<!-- chunk: 九、变更 Freeze 与安全护栏 -->## 九、变更 Freeze 与安全护栏

## 9.1 变更 Freeze 配置

```yaml
change_freeze:
  schedule:
    # 常规冻结期
    regular_freeze:
      - period: "周五 18:00 - 周一 09:00"
        reason: "周末非工作时间"
      - period: "节假日 3天 前 - 节假日 1天 后"
        reason: "法定节假日"
      - period: "发布窗口 前后 2h"
        reason: "发布窗口保护"

    # 事件触发冻结
    event_triggered:
      active_on_severity: "P0-P1"
      duration: 4h
      window_before_incident: 1h

  enforcement:
    block_changes: true
    exception_approval: "VP Engineering"
    override_annotation: "kudig.io/change-freeze-override"
```

## 9.2 安全护栏配置

```yaml
security_guardrails:
  # 高风险操作确认
  high_risk_confirmation:
    - action: "kubectl delete namespace"
      confirmation: "输入 namespace 名称确认"
      notification: "#security-alerts"

    - action: "kubectl delete deployment"
      confirmation: "输入 deployment 名称确认"
      notification: "#security-alerts"

    - action: "kubectl scale deployment --replicas=0"
      confirmation: "确认要将副本数降为 0?"
      notification: "#security-alerts"

    - action: "kubectl exec -it"
      confirmation: "需要交互式终端?"
      audit_log: true

  # 自动化操作限制
  automation_restrictions:
    max_concurrent_auto_actions: 3
    block_on_change_freeze: true
    require_incident_context: true
    notification_channels:
      - "#automation-actions"
      - "pagerduty:L1-oncall"
```

---

<!-- chunk: 十、多集群与云厂商适配 -->## 十、多集群与云厂商适配

## 10.1 多集群统一问题映射

```yaml
multi_cluster_mapping:
  # 统一症状模型 → 各集群特定实现
  symptom_normalization:
    - normalized_symptom: "Pod CrashLoopBackOff"
      aks_equivalent: "Pod CrashLoopBackOff + AKS node issues"
      eks_equivalent: "Pod CrashLoopBackOff + EKS node issues"
      gke_equivalent: "Pod CrashLoopBackOff + GKE node issues"
      ack_equivalent: "Pod CrashLoopBackOff + ACK node issues + Terway"

    - normalized_symptom: "etcd 问题"
      self_hosted: "etcd cluster health check"
      managed:
        aks: "Not applicable (Azure manages control plane)"
        eks: "Not applicable (AWS manages control plane)"
        gke: "Not applicable (GCP manages control plane)"
        ack: "etcd 异常需要通过阿里云控制面排查"

  # 跨集群关联分析
  cross_cluster_analysis:
    enabled: true
    correlation_window: 5m
    min_clusters_affected: 2
    action: "跨集群级联问题告警"
```

## 10.2 云厂商特定故障模式

```yaml
cloud_provider_specific:
  alibaba_ack:
    # Terway 特有
    terway_ip_exhaustion:
      detection: "kubectl describe node | grep aliyun.com/eni-ip"
      auto_recovery: "标记节点 unschedulable，触发扩容"

    eni_quota_exceeded:
      detection: "ACK 控制台 → 弹性网卡配额"
      workaround: "升级 ECS 实例规格"

  aws_eks:
    # 节点组特有
    eks_node_termination:
      detection: "Auto Scaling Group lifecycle events"
      auto_recovery: "等待新节点加入"

  gcp_gke:
    # 节点池特有
    gke_node_pool_upgrade:
      detection: "GKE 控制面版本升级事件"
      auto_recovery: "暂时阻止新 Pod 调度"
```

---

<!-- chunk: 十一、知识库自学习 -->## 十一、知识库自学习

## 11.1 诊断模式学习

```yaml
learning_engine:
  # 从 resolved 事件中学习
  pattern_learning:
    enabled: true
    learning_sources:
      - type: "resolved_incidents"
        min_confidence: 0.90
      - type: "oncall_manual_overrides"
      - type: "runbook_feedback"

  # 模式提取
  pattern_extraction:
    - trigger: "相同症状 + 不同根因"
      action: "扩充 diagnostic_decision_tree"

    - trigger: "相同根因 + 新验证步骤"
      action: "优化 verification_steps"

    - trigger: "新症状未被覆盖"
      action: "创建新 symptom_mapping"

  # 学习反馈
  feedback:
    confidence_update: true
    new_symptom_proposal: true
    runbook_generation: true
    knowledge_base_sync: true
```

---

<!-- chunk: 十二、性能与可观测性 -->## 十二、性能与可观测性

## 12.1 映射引擎性能指标

```yaml
performance_metrics:
  # 映射延迟 SLO
  mapping_latency_slo:
    p50: < 100ms
    p95: < 500ms
    p99: < 1000ms
    alert_on_p99_exceeded: true

  # 匹配质量指标
  matching_quality:
    top_match_accuracy: 0.85   # 最高置信度匹配的正确率
    top3_match_coverage: 0.95 # Top3 包含正确根因的比例
    unknown_symptom_rate: 0.05 # 无法匹配的症状比例

  # 自动化效果指标
  automation_effectiveness:
    auto_execution_rate: 0.70  # 可自动执行的修复比例
    auto_success_rate: 0.90    # 自动执行成功率
    rollback_rate: 0.05         # 需要回滚的比例
```

## 12.2 可观测性集成

```yaml
observability_integration:
  tracing:
    enabled: true
    jaeger_endpoint: "http://jaeger-collector:14268/api/traces"
    sampling_rate: 0.1

  metrics:
    enabled: true
    prometheus_metrics:
      - name: "symptom_mapping_duration_seconds"
        type: "histogram"
        labels: ["symptom_category", "confidence_bucket"]

      - name: "diagnosis_paths_total"
        type: "counter"
        labels: ["symptom", "root_cause"]

      - name: "auto_heal_execution_total"
        type: "counter"
        labels: ["action_id", "status"]

  logging:
    structured_logging: true
    log_level: "info"
    sensitive_data_masking: true
```

---

<!-- chunk: 十三、合规与审计 -->## 十三、合规与审计

## 13.1 操作审计日志

```yaml
audit_log:
  # 记录所有自动化操作
  operations:
    - action: "auto_heal_execution"
      fields:
        - timestamp
        - incident_id
        - action_id
        - command_executed
        - risk_level
        - operator (system vs human)
        - approval_chain

    - action: "runbook_execution"
      fields:
        - timestamp
        - runbook_id
        - execution_result
        - verification_status

    - action: "escalation"
      fields:
        - timestamp
        - incident_id
        - from_level
        - to_level
        - reason

  # 合规保留期
  retention:
    standard_logs: 90d
    security_logs: 1y
    audit_logs: 7y
```

## 13.2 合规检查项

```yaml
compliance:
  # SOC2 / ISO27001 相关
  access_control:
    privileged_access_review: 90d
    service_account_usage_audit: 30d

  # 数据保护
  data_handling:
    pii_masking_in_logs: true
    encryption_at_rest: required
    encryption_in_transit: required

  # 变更管理
  change_management:
    major_change_approval: "Change Advisory Board"
    emergency_change_approval: "On-Call Lead"
    change_freeze_enforcement: required
```

---

> **版本**: v1.1
> **维护团队**: SRE Team / Platform Team
> **更新日期**: 2026-05-19
> **下一步**: 集成到 AI Agent 执行引擎，支持:
> - [ ] SLA/SLO 自动关联与告警
> - [ ] Runbook 自动执行
> - [ ] 多集群统一映射
> - [ ] 知识库自学习

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/高级排障/MOC.md|topic-structural-trouble-shooting MOC]]
- [[domain-10-troubleshooting-diagnostics/高级排障/README.md|Kubernetes 结构化故障排查知识库]]
- [[domain-10-troubleshooting-diagnostics/高级排障/00-configuration-first-methodology.md|疑难问题系统性排查方法论：配置优先（Configuration-First）]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-dra-troubleshooting|DRA（动态资源分配）故障排查指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/13-etcd-maintenance|etcd 维护专项文档]]

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide

## See Also

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-dra-troubleshooting|09-dra-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/13-etcd-maintenance|10-etcd-maintenance]]
- [[domain-10-troubleshooting-diagnostics/高级排障/00-configuration-first-methodology.md|00-configuration-first-methodology]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-dra-troubleshooting|09-dra-troubleshooting]]


<!-- risk-assessed -->
