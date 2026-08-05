---
title: Falco (entities)
description: Falco — Kubernetes 生产运维知识库
summary: Falco — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- runtime
- falco
- detection
- ebpf
- cilium
- daemonset
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Falco 是什么
- 如何 Falco
trigger_keywords:
- Falco
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Falco

Falco is the de facto runtime security threat detection engine for cloud native environments, graduated from CNCF.

## Key Facts

- **Status**: CNCF graduated
- **Engine**: eBPF or kernel module (dual engine)
- **Detection**: Rule-based system call monitoring
- **Output**: JSON events to stdout, files, or notification systems

## Typical Attack Detections

| Detection Rule | What It Catches |
|---------------|----------------|
| Terminal shell in container | Interactive shell or bash execution |
| Read sensitive file | Access to /etc/shadow, SSH keys |
| Container mounted host filesystem | Potential container escape |
| Crypto mining detected | Cryptocurrency mining processes |
| Outbound connection to C2 | Known command-and-control servers |
| Unexpected process execution | Unauthorized binary execution |
| Network activity from unexpected process | Lateral movement indicators |

## Deployment

Falco deploys as a [[daemonset|DaemonSet]] with one pod per node, monitoring all container syscalls. Recommended configuration uses eBPF driver (safer than kernel module).

```bash
# Helm 安装 Falco（eBPF 驱动）
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm install falco falcosecurity/falco \
  -n falco --create-namespace \
  --set driver.kind=ebpf \
  --set falcosidekick.enabled=true \
  --set falcosidekick.config.slack.webhookurl="https://hooks.slack.com/..."

# 验证部署
kubectl get pods -n falco
kubectl logs -l app.kubernetes.io/name=falco -n falco --tail=20
```

```yaml
# 自定义规则示例（ConfigMap）
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-custom-rules
  namespace: falco
data:
  custom_rules.yaml: |
    - rule: Detect kubectl in container
      desc: Detect kubectl execution inside containers
      condition: >
        spawned_process and container and
        proc.name = "kubectl"
      output: >
        kubectl executed in container
        (user=%user.name container=%container.name image=%container.image.repository
        command=%proc.cmdline)
      priority: WARNING
      tags: [container, mitre_execution]
    - rule: Detect crypto mining
      desc: Detect cryptocurrency mining processes
      condition: >
        spawned_process and container and
        (proc.name in (xmrig, minerd, cpuminer) or
         proc.cmdline contains "stratum+tcp")
      output: >
        Crypto mining detected
        (user=%user.name container=%container.name process=%proc.name)
      priority: CRITICAL
      tags: [container, mitre_impact]
```

## 运维操作

```bash
# 🟢 低风险：查看 Falco 告警
kubectl logs -l app.kubernetes.io/name=falco -n falco -f | jq 'select(.priority=="Critical")'

# 🟢 低风险：检查 Falco 状态
kubectl get pods -n falco -o wide
falcoctl artifact list  # 查看已安装规则集

# 🟡 中风险：更新规则集
falcoctl artifact update falco-rules
kubectl rollout restart daemonset/falco -n falco

# 🟡 中风险：调整告警级别
kubectl edit configmap falco -n falco  # 修改 rules_files

# 🔴 高风险：禁用 Falco（失去运行时安全监控）
kubectl scale daemonset/falco -n falco --replicas=0
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Falco Pod CrashLoop | eBPF 驱动不兼容 | `kubectl logs -l app=falco -n falco` | 检查内核版本，切换为 kernel module |
| 告警过多（噪音） | 规则未调优 | `kubectl logs falco -n falco \| jq .rule` | 添加例外或调整规则优先级 |
| 无告警输出 | 规则未加载 | `falco --validate /etc/falco/falco_rules.yaml` | 检查 rules_files 配置 |
| 性能影响 | 高 syscall 率 | `top -p $(pgrep falco)` | 调整 buffer_size，启用 modern-bpf |
| Sidekick 未转发 | Webhook 配置错误 | `kubectl logs -l app=falcosidekick -n falco` | 检查输出配置（Slack/PagerDuty） |

```
排查流程：
├── Falco 未运行？
│   ├── kubectl get pods -n falco → 检查 Pod 状态
│   ├── kubectl logs → 查看启动错误
│   └── 检查内核版本和 eBPF 支持
├── 告警异常？
│   ├── 检查规则文件是否加载
│   ├── falco --validate → 验证规则语法
│   └── 检查输出配置（stdout/file/webhook）
└── 性能问题？
    ├── 检查 CPU/内存使用
    ├── 调整 syscall buffer 大小
    └── 考虑使用 modern-bpf 驱动
```

## 生产案例

### 案例 1：检测容器逃逸攻击

- **场景**：安全团队发现某容器尝试挂载宿主机 /proc 目录
- **排查**：Falco 触发 "Container mounted host filesystem" 告警，定位到特定 Pod
- **方案**：立即隔离 Pod，审查 SecurityContext，添加 PodSecurityPolicy 禁止 hostPath 挂载
- **效果**：5 分钟内发现并遏制攻击，后续零容器逃逸事件

### 案例 2：加密货币挖矿检测

- **场景**：节点 CPU 异常飙高，怀疑被植入挖矿程序
- **排查**：Falco 触发 "Crypto mining detected" 告警，定位到被入侵的容器（通过漏洞利用植入 xmrig）
- **方案**：删除恶意 Pod，修补应用漏洞，添加 NetworkPolicy 限制出站连接，启用 Falco 自动响应（杀死恶意 Pod）
- **效果**：从发现到清除 < 10min，后续通过 Falco + NetworkPolicy 双重防护零复发

## Related

- [[kuasar]] — Kuasar
- [[deployment]] — Deployment
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — [[22-概念/05-安全/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[cilium]] — Cilium
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[23-实体/06-安全/tetragon.md|Tetragon]]

- 99-falco-runtime-security-guide
- 01-falco-cloud-native-security
- falco
- RELEASE-NOTES-0.43
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.42
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.28
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.31
- [[23-实体/15-参考与索引/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|发布说明阅读指南]] — Cross-reference
- [[22-概念/11-交叉分析/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — Cross-reference
- [[22-概念/11-交叉分析/eBPF × 运行时安全.md|eBPF x 运行时安全]] — Cross-reference
- [[22-概念/12-研究/security-tool-evolution.md|安全工具演进]] — Cross-reference
- [[23-实体/06-安全/trivy.md|Trivy]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
