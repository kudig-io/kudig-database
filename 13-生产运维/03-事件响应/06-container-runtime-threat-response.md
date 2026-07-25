---
title: 容器运行时威胁响应：Falco 与 Tetragon
description: 面向阿里云/专有云 K8s 的容器运行时威胁响应方案，讲解 Falco、Tetragon 的部署、规则编写与异常行为处置。
summary: 面向阿里云/专有云 K8s 的容器运行时威胁响应方案，讲解 Falco、Tetragon 的部署、规则编写与异常行为处置。
category: security
tags:
- k8s
- security
- falco
- tetragon
- runtime-security
- ebpf
- threat-detection
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 运维工程师
estimated_read_time: 20min
intent_queries:
- 容器运行时威胁响应
- Falco Tetragon 安全检测
- K8s 运行时异常行为处置
trigger_keywords:
- Falco
- Tetragon
- 运行时安全
- 威胁检测
- eBPF
prerequisites:
- kubectl-basics
- security-basics
- linux-basics
- ebpf-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器运行时威胁响应：Falco 与 Tetragon

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解容器运行时威胁检测工具 Falco、Tetragon 的部署与响应流程。

## 目录

1. [运行时安全概述](#运行时安全概述)
2. [Falco 部署与规则](#falco-部署与规则)
3. [Tetragon 部署与策略](#tetragon-部署与策略)
4. [常见威胁检测场景](#常见威胁检测场景)
5. [告警与响应](#告警与响应)
6. [取证与溯源](#取证与溯源)
7. [性能与稳定性](#性能与稳定性)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 运行时安全概述

### 1.1 为什么需要运行时安全

容器运行时的威胁包括：
- 容器逃逸
- 异常进程执行
- 敏感文件访问
- 网络异常连接
- 提权操作

### 1.2 Falco vs Tetragon

| 特性 | Falco | Tetragon |
|:---|:---|:---|
| 检测机制 | 系统调用 + kprobe | eBPF |
| 规则语言 | Falco 规则 YAML | Cilium CRD / TracingPolicy |
| 响应能力 | 告警 | 告警 + 可执行 Kill |
| 性能开销 | 中 | 低 |
| 学习曲线 | 平缓 | 较陡 |

---

## 2. Falco 部署与规则

### 2.1 使用 Helm 部署 Falco

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Falco chart 仓库
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm repo update

# 安装 Falco
helm install falco falcosecurity/falco \
  --namespace falco \
  --create-namespace \
  --set driver.kind=modern_ebpf \
  --set tty=true
```
### 2.2 自定义规则示例

```yaml
# custom-rules.yaml
- rule: Unauthorized Shell in Production Pod
  desc: Detect shell execution in production pods
  condition: >
    spawned_process
    and container.name != ""
    and k8s.ns.name = "production"
    and (proc.name in (shell_procs))
  output: >
    Shell executed in production pod
    user=%user.name command=%proc.cmdline
    pod=%k8s.pod.name namespace=%k8s.ns.name
  priority: WARNING

- rule: Sensitive File Access
  desc: Detect access to sensitive files
  condition: >
    open_read
    and fd.name in (/etc/shadow, /etc/kubernetes/pki/*.key)
  output: >
    Sensitive file accessed
    user=%user.name file=%fd.name
    pod=%k8s.pod.name namespace=%k8s.ns.name
  priority: CRITICAL
```

### 2.3 加载自定义规则

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create configmap falco-custom-rules \
  --from-file=custom-rules.yaml -n falco

helm upgrade falco falcosecurity/falco \
  --namespace falco \
  --set collectors.containerd.enabled=true \
  --set customRules."custom-rules\.yaml"=custom-rules.yaml
```
---

## 3. Tetragon 部署与策略

### 3.1 部署 Tetragon

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add cilium https://helm.cilium.io
helm install tetragon cilium/tetragon \
  --namespace kube-system
```
### 3.2 TracingPolicy 示例

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: detect-crypto-mining
spec:
  kprobes:
    - call: "__x64_sys_execve"
      syscall: true
      args:
        - index: 0
          type: "string"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Prefix"
              values:
                - "/tmp/xmrig"
                - "/tmp/miner"
          matchActions:
            - action: Sigkill
```

### 3.3 查看 Tetragon 事件

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 实时查看事件
kubectl exec -it -n kube-system ds/tetragon -c tetragon -- tetra getevents -o compact
```
---

## 4. 常见威胁检测场景

### 4.1 挖矿程序检测

| 工具 | 检测点 |
|:---|:---|
| Falco | 异常进程名、高 CPU、/tmp 执行 |
| Tetragon | 特定二进制执行、网络连接到矿池 |

### 4.2 容器逃逸检测

| 工具 | 检测点 |
|:---|:---|
| Falco | mount /proc、privileged 操作 |
| Tetragon | cap_sys_admin 使用、namespace 操作 |

### 4.3 敏感凭证访问

| 工具 | 检测点 |
|:---|:---|
| Falco | 读取 /var/run/secrets、ServiceAccount token |
| Tetragon | open 系统调用监控 |

---

## 5. 告警与响应

### 5.1 Falco Sidekick 集成

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm install falcosidekick falcosecurity/falcosidekick \
  --namespace falco \
  --set config.slack.webhookurl=https://hooks.slack.com/...
```
### 5.2 自动响应 playbook

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# auto-respond.sh
# 根据 Falco 告警自动隔离可疑 Pod

POD=$1
NS=$2

echo "隔离可疑 Pod ${NS}/${POD}"
kubectl label pod ${POD} -n ${NS} security.isolated=true
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: isolate-${POD}
  namespace: ${NS}
spec:
  podSelector:
    matchLabels:
      security.isolated: "true"
  policyTypes:
    - Ingress
    - Egress
EOF
```
---

## 6. 取证与溯源

### 6.1 收集 Falco 事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出最近 1 小时 Falco 事件
kubectl logs -n falco -l app.kubernetes.io/name=falco --since=1h > /tmp/falco-events.log
```
### 6.2 关联审计日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 根据 Pod 名称关联 K8s 审计日志
kubectl audit-log search --pod=<pod-name> --namespace=<ns> --since=1h
```
---

## 7. 性能与稳定性

### 7.1 性能优化

| 优化项 | 建议 |
|:---|:---|
| 规则数量 | 控制在 100 条以内，避免过多规则 |
| 事件采样 | 对高频事件开启采样 |
| 过滤条件 | 尽早过滤，减少内核态开销 |
| 资源限制 | 为 Falco/Tetragon 设置 CPU/Mem limits |

### 7.2 阿里云/专有云注意

- 确认内核版本支持 eBPF（建议 4.19+）
- 专有云节点可能需关闭 SELinux 或调整 AppArmor
- 阿里云集群推荐使用阿里云容器安全服务

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| Falco/Tetragon 部署 | DaemonSet 全节点覆盖 | `kubectl get ds -n falco` |
| 自定义规则 | 覆盖挖矿、逃逸、凭证访问 | 规则文件 |
| 告警集成 | 对接 Slack/钉钉/告警中心 | 测试告警 |
| 自动响应 | 可疑 Pod 隔离脚本 | 演练验证 |
| 事件保留 | 7-30 天 | 日志策略 |
| 性能基线 | CPU/Mem 开销 < 5% | 监控 |

---

## Falco 告警降噪

Falco 默认规则可能产生大量误报，需根据业务特征进行白名单与优先级调整。

### 降噪策略

| 策略 | 示例 |
|:---|:---|
| 白名单 | 排除 CI/CD、监控、日志代理的常规 Shell |
| 聚合 | 相同告警在 5 分钟内只触发一次 |
| 抑制 | 低优先级告警不在夜间通知 |
| 分级 | 将挖矿、反向 Shell 设为 P0 |

### 与 SIEM 集成

将 Falco 告警通过 falcosidekick 发送到 SIEM：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-falcosidekick-config
  namespace: falco
data:
  config.yaml: |
    webhook:
      address: "https://siem.example.com/webhook"
      customHeaders: "Authorization: Bearer {{token}}"
```

### 运行时威胁狩猎

定期使用 Tetragon 或 inspektor-gadget 执行主动狩猎：

- 查找异常的出站连接
- 查找容器内新创建的 SUID 文件
- 查找挂载敏感目录的行为
- 查找未授权访问 K8s API 的 Pod

## 运行时威胁情报

将 Falco/Tetragon 检测与威胁情报结合，可提升检测准确率。

### 情报来源

| 来源 | 用途 |
|:---|:---|
| 阿里云恶意 IP 库 | 匹配异常出站连接 |
| 开源 Feodo/AbuseIPDB | 识别 C2 通信 |
| 内部黑名单 | 记录历史攻击者 |
| CVE 情报 | 关联已知漏洞利用行为 |

### 自动化响应

通过 falcosidekick 将告警发送到 SOAR 平台，实现自动隔离：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falcosidekick-config
  namespace: falco
data:
  config.yaml: |
    webhook:
      address: "https://soar.example.com/falco"
      customHeaders: "Authorization: Bearer {{token}}"
```

SOAR 收到告警后可调用 K8s API 自动应用隔离 NetworkPolicy 或删除 Pod。

## 典型工单场景与处理

**场景**：Falco 告警某容器内启动了 `/bin/bash -i`。

处理步骤：
1. 确认是否为运维人员正常操作。
2. 如非授权操作，立即隔离 Pod。
3. 保存进程树、网络连接与文件系统快照。
4. 追踪入侵路径并修复漏洞或凭证泄露。

## 运行时威胁检测规则维护

Falco/Tetragon 规则需要定期维护，以适应业务变化并降低误报。

### 规则维护流程

1. 收集业务正常行为白名单。
2. 根据新出现的威胁更新检测规则。
3. 在测试环境验证规则准确性。
4. 分批灰度发布到生产环境。
5. 定期 review 告警质量并调整阈值。

### 常见白名单场景

| 场景 | 白名单示例 |
|:---|:---|
| CI/CD 构建 | 允许构建容器内执行 Shell |
| 日志采集 | 允许 filebeat/fluentd 读取日志 |
| 监控探针 | 允许 blackbox-exporter 发起探测 |
| 调试 | 允许指定运维账号在指定命名空间执行 Shell |

### 运行时响应自动化

通过 Tetragon 的内置动作，可在检测到威胁时自动执行：

- `Sigkill`：终止进程
- `Notify`：发送告警
- `Override`：覆盖返回值
- `Post`：调用外部 webhook

## 运行时安全运营

运行时安全不是一次性部署，而是持续运营过程。需要定期 review 规则、更新基线并演练响应。

### Tetragon 与 Falco 对比

| 维度 | Falco | Tetragon |
|:---|:---|:---|
| 规则语言 | YAML | Cilium CRD / BPF |
| 实时阻断 | 需结合 sidekick | 内置 Sigkill 等动作 |
| K8s 原生 | 支持 | 深度集成 |
| 性能 | 较低 | 较高 |
| 学习曲线 | 平缓 | 陡峭 |

### 运行时安全运营 checklist

- [ ] 每月 review 一次检测规则与误报
- [ ] 每季度演练一次挖矿 / 反向 Shell 响应
- [ ] 运行时告警已接入 SIEM 与值班通知
- [ ] 新应用上线前完成运行时基线采集
- [ ] 重要生产命名空间启用精细化策略

## Related

- [[13-生产运维/03-事件响应/20-incident-response-process.md|安全事件响应与应急处理流程]]
- [[08-安全/03-运行时安全/01-falco-cloud-native-security.md|Falco 云原生安全]]

## See Also

- [[13-生产运维/03-事件响应/01-security-incident-response-playbook.md|云原生安全事件响应手册]]
- [[19-故障诊断/11-工具/03-ebpf-diagnostic-tools.md|eBPF 诊断工具]]

```

<!-- risk-assessed -->
