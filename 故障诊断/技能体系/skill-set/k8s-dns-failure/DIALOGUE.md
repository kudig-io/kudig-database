---
title: DNS解析问题 — 远程顾问对话脚本
summary: DNS解析问题的远程顾问对话脚本，覆盖CoreDNS、Service DNS、NodeLocal DNSCache排查。
category: troubleshooting
tags:
- networking
- remote-consultant
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
dialogue_id: DIALOGUE-SKILL-NET-001
skill_id: SKILL-NET-001
version: 1.0.0
role: remote-consultant
language: zh
relationships:
- target: '[[skills/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[entities/coredns.md]]'
  type: uses
- target: '[[entities/deployment.md]]'
  type: uses
- target: '[[entities/kubelet.md]]'
  type: uses
- target: '[[系统基础/topic-dictionary/networking/service.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# DNS 解析问题诊断 — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**，只能通过对话指导现场工程师执行操作。

---

## 对话入口

### 入口 A：工程师明确报告 DNS 问题

**工程师**：「Pod 里域名解析失败了」/「nslookup 不通」/「[[entities/coredns.md|CoreDNS]] 好像挂了」

**顾问回应**：
> 收到，DNS 问题直接影响服务发现，我们尽快排查。作为远程顾问，我无法直连你的集群，请你配合执行检查命令，我会根据输出给出下一步。
>
> 先回答三个问题（30 秒内）：
> 1. **影响范围**：多少节点/Pod/命名空间受影响？
> 2. **紧急程度**：业务是否已中断？有用户投诉吗？
> 3. **发生时间**：问题是突然发生还是逐渐恶化？最近有变更吗？

---

### 入口 B：工程师报告应用连接超时（疑似 DNS）

**工程师**：「服务间调用超时」/「应用连不上外部数据库」/「间歇性连接失败」

**顾问回应**：
> 服务间调用超时可能由多种原因引起，DNS 解析失败是常见根因之一。先确认是否是 DNS 问题。
>
> 请在一个受影响的 Pod 中执行：
> ```bash
> kubectl exec -it <pod-name> -n <namespace> -- nslookup kubernetes.default
> ```
> **如果无法执行**（没有 shell 或 nslookup）→ `kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default`
> **如果无法创建临时 Pod**（权限受限）→ `kubectl exec -it <pod-name> -n <namespace> -- cat /etc/resolv.conf`
> 请把输出贴给我。

---

### 入口 C：工程师报告 CoreDNS 相关告警

**工程师**：「CoreDNS Pod 报错了」/「kube-system 里有 Pod 重启」

**顾问回应**：
> CoreDNS 是集群 DNS 的核心组件，先确认它自身的状态。
>
> ```bash
> kubectl get pods -n kube-system -l k8s-app=kube-dns
> ```
> **如果没有 kube-dns 标签** → `kubectl get pods -n kube-system | grep -i dns`
> **如果 grep 也没有结果** → `kubectl get pods --all-namespaces | grep -i coredns`
> 请把 Pod 状态、重启次数贴给我。

---

## Round 1：快速定位问题层级

> 目标：判断问题发生在 **Pod 内部 → CoreDNS 服务 → 集群网络 → 外部 DNS** 的哪一层。

---

### Round 1 — 分支 A：Pod 内 nslookup 失败

**工程师反馈**：`nslookup kubernetes.default` 返回 `NXDOMAIN` 或超时。

**顾问指令**：
> 确认是 Pod 内解析失败。先检查 DNS 配置。
>
> ```bash
> kubectl exec -it <pod-name> -n <namespace> -- cat /etc/resolv.conf
> ```
> **如果无法执行**（没有 cat）→ `kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 5 dnsPolicy`
>
> 正常输出应包含 `nameserver 10.96.0.10`（ClusterIP，以集群为准）。若看到 `nameserver 127.0.0.53`，说明 `dnsPolicy` 有问题。

**分支决策**：
- **A1**：resolv.conf 正常（nameserver 是 ClusterIP）→ Round 2 — 分支 A（CoreDNS 服务端排查）
- **A2**：resolv.conf 异常（nameserver 是 127.0.0.53）→ Round 2 — 分支 B（dnsPolicy 修复）
- **A3**：resolv.conf 缺失或为空 → Round 2 — 分支 C（Pod 配置深度排查）

---

### Round 1 — 分支 B：内部域名成功，外部域名失败

**工程师反馈**：`nslookup kubernetes.default` 成功，但外部域名失败。

**顾问指令**：
> 集群内部 DNS 正常，问题出在外部解析或自定义域名配置。
>
> ```bash
> kubectl get configmap coredns -n kube-system -o yaml
> ```
> **如果没有 coredns ConfigMap** → `kubectl get configmap --all-namespaces | grep -i dns`
> **如果 RBAC 权限不足** → `kubectl get configmap coredns -n kube-system`
>
> 先确认 ConfigMap 是否存在，告诉我结果。重点关注 `Corefile` 中 `forward` 指向的上游 DNS。

**分支决策**：
- **B1**：forward 指向的 IP 不可达 → Round 2 — 分支 D（上游 DNS 修复）
- **B2**：有自定义 hosts 或 rewrite 规则 → Round 2 — 分支 E（自定义规则排查）
- **B3**：配置看起来正常 → Round 2 — 分支 F（节点 DNS 和网络策略排查）

---

### Round 1 — 分支 C：CoreDNS Pod 本身异常

**工程师反馈**：CoreDNS Pod 处于 CrashLoopBackOff、Pending 或频繁重启。

**顾问指令**：
> CoreDNS 自身不健康是根因。先收集 Pod 状态和日志。
>
> 1. 查看 Pod 状态和事件：`kubectl describe pod <coredns-pod-name> -n kube-system`
>    **如果不知道 Pod 名** → `kubectl get pods -n kube-system -o wide | grep -i coredns`
> 2. 查看日志（已崩溃加 `--previous`）：`kubectl logs <coredns-pod-name> -n kube-system --tail=50`
>    **如果 logs 为空** → `kubectl logs <coredns-pod-name> -n kube-system --previous --tail=50`
>    **如果 previous 也拿不到** → `kubectl get events -n kube-system --field-selector reason=BackOff | tail -20`
> 请把 Events 和日志贴给我。

**分支决策**：
- **C1**：OOMKilled 或内存不足 → Round 2 — 分支 G（资源扩容）
- **C2**：配置错误（Corefile 语法错误）→ Round 2 — 分支 H（配置回滚/修复）
- **C3**：网络或 CNI 错误 → Round 2 — 分支 I（CNI/网络排查）

---

### Round 1 — 分支 D：阿里云DNS/云解析/PrivateZone排查（ACK/专有云特有）

**工程师反馈**：「Pod内解析外部域名失败」/「阿里云PrivateZone记录不生效」/「云解析DNS间歇性超时」

**顾问指令**：
> 阿里云环境除了 CoreDNS 外，还涉及 **云解析DNS**、**PrivateZone**、**节点本地DNS缓存** 等特有链路。请按以下步骤排查：
>
> **步骤 1：检查 ACK 节点本地 DNS 缓存（node-local-dns）状态**
> ```bash
> # 查看 node-local-dns DaemonSet 状态
> kubectl get daemonset node-local-dns -n kube-system
> kubectl get pods -n kube-system -l k8s-app=node-local-dns
> ```
> **如果未安装 node-local-dns** → 检查是否使用了 ACK 默认的 CoreDNS 直接解析模式
> **如果 Pod 有异常** → `kubectl logs -n kube-system -l k8s-app=node-local-dns --tail=50`
>
> **步骤 2：检查阿里云 PrivateZone 关联**
> ```bash
> # 查看 CoreDNS 配置中是否有 PrivateZone forward 规则
> kubectl get configmap coredns -n kube-system -o yaml | grep -A 5 "privatezone|alidns"
> ```
> **如果无法执行** → 请确认：该集群是否在 ACK 控制台 **运维管理 > DNS** 中绑定了 **PrivateZone**？
>
> **步骤 3：验证外部域名解析链路**
> ```bash
> # 从 Pod 内测试阿里云DNS服务器
> kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup aliyun.com 223.5.5.5
> # 测试 PrivateZone 内网域名（如有）
> kubectl run dns-test2 --image=busybox:1.36 --rm -it --restart=Never -- nslookup <private-domain>. 100.100.2.136
> ```
> **如果无法创建临时 Pod** → `kubectl exec -it <pod-name> -n <namespace> -- cat /etc/resolv.conf`
> 检查 nameserver 是否指向了 **169.254.20.10**（node-local-dns）或 **kube-dns ClusterIP**

**阿里云DNS特有诊断（远程顾问适用）**：

| 阿里云DNS特有场景 | 诊断方法 | 修复方案 |
|:---|:---|:---|
| 节点本地DNS缓存（node-local-dns）IP冲突 | `kubectl logs node-local-dns-xx -n kube-system` 查看 bind 错误 | 修改 node-local-dns ConfigMap 中的 localIP，避免与现有网段冲突 |
| 云解析DNS限速导致外部域名解析失败 | 从Pod内连续 `nslookup` 测试，观察是否间歇性超时 | 在 CoreDNS forward 中配置多个上游 DNS（223.5.5.5, 223.6.6.6） |
| PrivateZone 记录与公网DNS冲突 | 阿里云控制台检查 PrivateZone 解析记录 | 调整 PrivateZone Zone 名称，避免与公网域名重叠 |
| ACK 集群未关联 PrivateZone | ACK 控制台 **运维管理 > DNS** 查看绑定状态 | 在 ACK 控制台绑定 PrivateZone，或手动配置 CoreDNS forward |
| 专有云 DNS 服务（Yaochi/Apsara DNS）异常 | 专有云天基/ASO 控制台查看 DNS 服务状态 | 联系阿里云驻场工程师重启 DNS 服务组件 |
| Terway 模式下 Pod DNS 策略异常 | 检查 Pod dnsPolicy 和 Terway 网络配置 | 将 dnsPolicy 改为 ClusterFirst，或配置 terway-exclusive ENI 模式 |

> **远程顾问无法直连时的阿里云控制台排查**：
> 1. 登录 **阿里云控制台 > 云解析DNS** → 检查 **公网递归解析** 服务状态是否正常
> 2. 登录 **阿里云控制台 > 专有网络 VPC > PrivateZone** → 检查关联的 VPC 是否包含该 ACK 集群
> 3. 登录 **ACK 控制台 > 集群 > 运维管理 > DNS** → 查看集群 DNS 组件健康状态
> 4. 如果是 **专有云**，请登录 **ASO/天基控制台** → 检查 **Yaochi DNS** 或 **Apsara DNS** 服务实例状态

**分支决策**：
- **D1**：node-local-dns 异常 → 修复或重启 node-local-dns DaemonSet
- **D2**：PrivateZone 配置问题 → 调整 PrivateZone 绑定或 CoreDNS forward 配置
- **D3**：阿里云云解析DNS服务异常 → 切换备用上游 DNS，或提交阿里云工单
- **D4**：专有云底座DNS服务异常 → 升级至阿里云驻场工程师/TAM处理

---

## Round 2：分层深入诊断

> 目标：根据 Round 1 确定的层级，执行针对性的深度检查。

---

### Round 2 — 分支 A：CoreDNS 服务端排查

**顾问指令**：
> Pod DNS 配置正确，解析失败说明 CoreDNS 服务端没有响应。
>
> 1. 确认 CoreDNS [[系统基础/topic-dictionary/networking/service.md|Service]] ClusterIP：`kubectl get svc kube-dns -n kube-system`
>    **如果没有 kube-dns 服务名** → `kubectl get svc -n kube-system | grep -i dns`
> 2. 从 Pod 内直接测试 CoreDNS 服务 IP：`kubectl exec -it <pod-name> -n <namespace> -- nc -vz <kube-dns-cluster-ip> 53`
>    **如果没有 nc** → `kubectl exec -it <pod-name> -n <namespace> -- sh -c "echo '' > /dev/udp/<kube-dns-cluster-ip>/53"`
>    **如果没有 /dev/udp 支持** → `kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nslookup kubernetes.default <kube-dns-cluster-ip>`
> 3. 检查 CoreDNS Pod 状态：`kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide`
> 请告诉我：CoreDNS Pod 是否全部 Running？从 Pod 内能否连通 53 端口？

**分支决策**：
- **A1**：Pod 全 Running，但 53 端口不通 → Round 3 — 分支 J（NetworkPolicy 修复）
- **A2**：CoreDNS Pod 部分 NotReady → 返回 Round 1 — 分支 C
- **A3**：Pod 正常，端口也通，但解析仍失败 → Round 3 — 分支 K（CoreDNS 深度排查）

---

### Round 2 — 分支 B：dnsPolicy 修复

**顾问指令**：
> Pod 使用了节点本地 DNS 而非集群 DNS。需要修复 dnsPolicy。
>
> 1. 确认当前配置：`kubectl get [[entities/deployment.md|deployment]] <deploy-name> -n <namespace> -o yaml | grep -A 3 dnsPolicy`
>    **如果是裸 Pod** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 3 dnsPolicy`
> 2. 修改为 `ClusterFirst`：`kubectl patch deployment <deploy-name> -n <namespace> --type merge -p '{"spec":{"template":{"spec":{"dnsPolicy":"ClusterFirst"}}}}'`
>    **如果是裸 Pod** → `kubectl delete pod <pod-name> -n <namespace>`，然后修改 YAML 重新创建
>    **如果无法删除**（没有创建权限）→ 请把当前 Pod/Deployment YAML 贴给我，我帮你写修改后的版本
> 3. 验证新 Pod：`kubectl exec -it <new-pod-name> -n <namespace> -- cat /etc/resolv.conf`

**分支决策**：
- **B1**：修改后正确，解析恢复 → 修复完成，进入验证
- **B2**：修改后仍不正确 → 检查 systemd-resolved 冲突，Round 3 — 分支 L
- **B3**：无法修改 → 提供声明式 YAML 方案，或引导联系集群管理员

---

### Round 2 — 分支 C：Pod 配置异常深度排查

**顾问指令**：
> resolv.conf 缺失或为空非常异常，可能由 [[entities/kubelet.md|kubelet]] 或容器运行时 Bug 引起。
>
> 1. 检查 Pod 完整 spec：`kubectl get pod <pod-name> -n <namespace> -o yaml | head -100`
> 2. 检查节点状态：`kubectl get node <node-name> -o wide`
> 3. 检查该节点其他 Pod：`kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>`
> 请把 Pod spec 的 dnsPolicy、dnsConfig 和节点状态告诉我。

**分支决策**：
- **C1**：节点 NotReady → Round 3 — 分支 M（节点恢复）
- **C2**：节点 Ready，多个 Pod 都有 DNS 问题 → Round 3 — 分支 L（节点 DNS 检查）
- **C3**：只有单个 Pod 异常 → 建议删除重建，如仍失败进入升级决策

---

### Round 2 — 分支 D：上游 DNS 修复

**顾问指令**：
> CoreDNS 无法连接上游 DNS。先确认上游可达性。
>
> 1. 从集群内测试上游 DNS：`kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup google.com <upstream-ip>`
>    **如果无法创建临时 Pod** → `kubectl exec -it <pod-name> -n <namespace> -- sh -c "nslookup google.com <upstream-ip>"`
>    **如果 Pod 内也没有 nslookup** → `kubectl exec -it <pod-name> -n <namespace> -- sh -c "echo '' > /dev/udp/<upstream-ip>/53"`
> 2. 检查 NetworkPolicy：`kubectl get networkpolicy --all-namespaces`
>    **如果没有 NetworkPolicy CRD** → `kubectl get ciliumnetworkpolicy --all-namespaces 2>/dev/null || kubectl get caliconetworkpolicy --all-namespaces 2>/dev/null || echo "无 NetworkPolicy"`
> 请告诉我：上游 DNS 是否可达？有没有 NetworkPolicy？

**分支决策**：
- **D1**：上游不可达，无 NetworkPolicy → 联系网络团队修复，或修改 forward 指向可用 DNS
- **D2**：上游不可达，有 NetworkPolicy 阻断 → Round 3 — 分支 J（NetworkPolicy 修复）
- **D3**：上游可达，但 CoreDNS 不转发 → Round 3 — 分支 K（CoreDNS 配置/缓存排查）

---

### Round 2 — 分支 E：自定义规则排查

**顾问指令**：
> CoreDNS 的 hosts 或 rewrite 规则可能配置错误。
>
> 1. 导出备份：`kubectl get configmap coredns -n kube-system -o yaml > /tmp/coredns-backup.yaml`
>    **如果无法写入 /tmp** → `kubectl get configmap coredns -n kube-system -o yaml`（手动保存）
> 2. 检查 Corefile：`kubectl get configmap coredns -n kube-system -o jsonpath='{.data.Corefile}'`
>    **如果无法执行 jsonpath** → `kubectl get configmap coredns -n kube-system -o yaml | grep -A 50 Corefile`
> 请把 Corefile 完整内容贴给我，我帮你审查。

**分支决策**：
- **E1**：hosts 条目 IP 错误或重复 → 修改 ConfigMap，删除错误条目
- **E2**：rewrite 规则正则错误 → 修改 ConfigMap，修正正则
- **E3**：自定义规则正确但解析仍失败 → Round 3 — 分支 K（CoreDNS 深度排查）

---

### Round 2 — 分支 F：节点 DNS 和网络策略排查

**顾问指令**：
> CoreDNS 配置正常，问题可能在节点或网络层面。
>
> 1. 检查节点 resolv.conf：`kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- cat /host/etc/resolv.conf`
>    **如果没有 debug 权限** → `ssh <node-ip> "cat /etc/resolv.conf"`
>    **如果无法 SSH** → 创建 hostNetwork Pod：`kubectl run node-test --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"nodeSelector":{"kubernetes.io/hostname":"<node-name>"},"hostNetwork":true}}' -- cat /etc/resolv.conf`
> 2. 检查 NetworkPolicy：`kubectl get networkpolicy --all-namespaces -o yaml | grep -B 5 -A 10 "port.*53"`
>    **如果无法执行** → `kubectl get networkpolicy --all-namespaces`
> 把结果贴给我。

**分支决策**：
- **F1**：节点 resolv.conf 被 systemd-resolved 覆盖 → Round 3 — 分支 L（节点 DNS 修复）
- **F2**：NetworkPolicy 阻断了 UDP/53 → Round 3 — 分支 J（NetworkPolicy 修复）
- **F3**：节点和策略都正常 → Round 3 — 分支 K（CoreDNS 深度排查）

---

### Round 2 — 分支 G：资源扩容

**顾问指令**：
> CoreDNS 因内存不足被杀死，常见于高并发查询场景。
>
> 1. 检查当前资源限制：`kubectl get deployment coredns -n kube-system -o yaml | grep -A 10 resources`
>    **如果无法执行** → `kubectl describe deployment coredns -n kube-system | grep -A 10 "Limits|Requests"`
> 2. 检查资源使用量：`kubectl top pod -n kube-system -l k8s-app=kube-dns`
>    **如果 metrics-server 不可用** → `kubectl logs <coredns-pod-name> -n kube-system --previous | grep -i "out of memory|oom"`
> 3. 临时扩大资源限制：`kubectl patch deployment coredns -n kube-system --type merge -p '{"spec":{"template":{"spec":{"containers":[{"name":"coredns","resources":{"limits":{"memory":"512Mi","cpu":"500m"},"requests":{"memory":"128Mi","cpu":"100m"}}}]}}}}'`
>    **如果无法 patch** → `kubectl edit deployment coredns -n kube-system`（手动修改）
>    **如果没有交互式终端** → `kubectl set resources deployment coredns -n kube-system --limits=memory=512Mi,cpu=500m --requests=memory=128Mi,cpu=100m`
> 请告诉我当前限制是多少？扩容后是否恢复？

**分支决策**：
- **G1**：扩容后稳定，解析恢复 → 修复完成，建议后续监控
- **G2**：扩容后仍 OOMKilled → 可能存在 DNS 泛洪，Round 3 — 分支 N（DNS 查询分析）
- **G3**：无法修改资源限制 → 升级决策点

---

### Round 2 — 分支 H：配置回滚/修复

**顾问指令**：
> CoreDNS 因配置错误无法启动，需要修复 Corefile。
>
> 1. 备份 ConfigMap：`kubectl get configmap coredns -n kube-system -o yaml > /tmp/coredns-backup-$(date +%Y%m%d-%H%M%S).yaml`
>    **如果无法备份** → `kubectl get configmap coredns -n kube-system -o yaml`（手动保存）
> 2. 查看具体语法错误：`kubectl logs <coredns-pod-name> -n kube-system --previous | tail -30`
>    **如果无法执行** → `kubectl get events -n kube-system --field-selector reason=Failed | grep -i coredns | tail -20`
> 3. 回滚历史版本：`kubectl rollout history deployment coredns -n kube-system`
>    **如果无法执行** → `kubectl get configmap coredns -n kube-system -o yaml`，我帮你手工修复
> 请把错误日志和 Corefile 贴给我。

**分支决策**：
- **H1**：能回滚到历史版本 → 执行 `kubectl rollout undo`，验证恢复
- **H2**：需要手工修复 Corefile → 顾问提供修正后 ConfigMap，工程师 apply
- **H3**：ConfigMap 正常但日志仍报错 → 检查其他挂载 ConfigMap，Round 3 — 分支 K

---

### Round 2 — 分支 I：CNI/网络排查

**顾问指令**：
> CoreDNS Pod 因网络问题无法启动，通常指向 CNI 问题。
>
> 1. 查看 Pod 事件：`kubectl describe pod <coredns-pod-name> -n kube-system | grep -A 20 Events`
>    **如果无法执行** → `kubectl get events -n kube-system --field-selector involvedObject.name=<coredns-pod-name>`
> 2. 检查同节点其他 Pod：`kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> | grep -v Running`
> 3. 检查 CNI Pod 状态：`kubectl get pods -n kube-system | grep -E 'calico|cilium|flannel|weave'`
>    **如果无法执行** → `kubectl get pods -n kube-system`，把非 Running 的 Pod 告诉我

**分支决策**：
- **I1**：CNI Pod 异常 → 升级决策点（网络深度诊断 [[skills/skill-k8s-node-notready-SKILL.md|SKILL]]-NET-003）
- **I2**：只有 CoreDNS 受影响 → 检查 CoreDNS 亲和性/反亲和性配置
- **I3**：节点上大量 Pod 异常 → 节点级网络问题，Round 3 — 分支 M（节点恢复）

---

## Round 3：精确修复与验证

> 目标：执行最终修复动作，验证 DNS 恢复正常，决定是否升级。

---

### Round 3 — 分支 J：NetworkPolicy 修复

**顾问指令**：
> NetworkPolicy 阻断了到 CoreDNS 的流量。需要允许 DNS 查询。
>
> 1. 查看具体策略：`kubectl get networkpolicy -n <namespace> <policy-name> -o yaml`
>    **如果无法执行** → `kubectl describe networkpolicy -n <namespace> <policy-name>`
> 2. 添加 allow-dns 规则（示例）：
> ```yaml
> apiVersion: networking.k8s.io/v1
> kind: NetworkPolicy
> metadata:
>   name: allow-dns
>   namespace: <namespace>
> spec:
>   podSelector: {}
>   policyTypes: [Egress]
>   egress:
>   - to:
>     - namespaceSelector:
>         matchLabels:
>           kubernetes.io/metadata.name: kube-system
>       podSelector:
>         matchLabels:
>           k8s-app: kube-dns
>     ports:
>     - protocol: UDP
>       port: 53
>     - protocol: TCP
>       port: 53
> ```
> **如果无法 apply**（权限不足）→ 把修改后 YAML 给管理员审批后执行
> 3. 验证：`kubectl exec -it <pod-name> -n <namespace> -- nslookup kubernetes.default`

**分支决策**：
- **J1**：allow-dns 后解析恢复 → 修复完成
- **J2**：修改后仍不通 → 检查 CNI 全局策略，升级决策
- **J3**：无权限修改 → 升级决策点

---

### Round 3 — 分支 K：CoreDNS 内部深度排查

**顾问指令**：
> CoreDNS Pod 看起来正常但解析仍失败，需要检查运行时状态。
>
> 1. 检查健康状态：`kubectl exec -it <coredns-pod-name> -n kube-system -- wget -qO- http://localhost:8080/health`
>    **如果没有 wget** → `kubectl exec -it <coredns-pod-name> -n kube-system -- curl -s http://localhost:8080/health`
>    **如果也没有 curl** → `kubectl logs <coredns-pod-name> -n kube-system --tail=20 | grep -i "health|ready"`
> 2. 检查指标：`kubectl port-forward <coredns-pod-name> 9153:9153 -n kube-system & && curl -s http://localhost:9153/metrics | grep "coredns_dns_requests_total"`
>    **如果无法 port-forward** → `kubectl exec -it <coredns-pod-name> -n kube-system -- wget -qO- http://localhost:9153/metrics | grep "coredns_dns_requests_total"`
> 3. 重启 CoreDNS：`kubectl rollout restart deployment coredns -n kube-system`
>    **如果无法执行** → `kubectl delete pod -n kube-system -l k8s-app=kube-dns`
> 请告诉我健康检查和重启后的结果。

**分支决策**：
- **K1**：重启后 DNS 恢复 → 修复完成，建议监控是否复发
- **K2**：健康检查失败（Pod 状态 Running）→ CoreDNS 内部异常，检查插件链
- **K3**：重启后仍间歇性失败 → 升级决策点（网络深度诊断）

---

### Round 3 — 分支 L：节点 DNS 修复

**顾问指令**：
> 节点的 systemd-resolved 劫持了 DNS 查询，导致容器内解析异常。
>
> 1. 检查 systemd-resolved 状态：`ssh <node-ip> "systemctl status systemd-resolved"`
>    **如果无法 SSH** → 创建 hostNetwork Pod：`kubectl run node-debug --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"nodeSelector":{"kubernetes.io/hostname":"<node-name>"},"hostNetwork":true}}' -- systemctl status systemd-resolved`
>    **如果容器内没有 systemd** → `kubectl run node-debug ... -- cat /etc/resolv.conf`
> 2. 检查监听端口：`ssh <node-ip> "ss -tlnp | grep -E '53|systemd-resolve'"`
>    **如果无法执行** → `kubectl run node-debug ... -- ss -tlnp | grep 53`
> 3. 修复方案（二选一）：
>    - 方案 A：禁用 systemd-resolved，恢复传统 /etc/resolv.conf
>    - 方案 B：配置 kubelet 使用 /run/systemd/resolve/resolv.conf
> 请告诉我 systemd-resolved 状态和监听端口。

**分支决策**：
- **L1**：禁用 systemd-resolved 后恢复 → 修复完成
- **L2**：修改 kubelet 配置后恢复 → 修复完成，需重启 kubelet
- **L3**：多节点都有此问题 → 批量修复，建议 Ansible 统一修改

---

### Round 3 — 分支 M：节点恢复

**顾问指令**：
> 节点级异常导致 DNS 不可用，需要恢复节点健康。
>
> 1. 检查节点状态：`kubectl describe node <node-name> | grep -A 20 Conditions`
>    **如果无法执行** → `kubectl get node <node-name> -o yaml | grep -A 20 conditions`
> 2. 检查 kubelet 状态：`ssh <node-ip> "systemctl status kubelet"`
>    **如果无法 SSH** → `kubectl run node-debug --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"nodeSelector":{"kubernetes.io/hostname":"<node-name>"},"hostNetwork":true}}' -- pgrep -a kubelet`
> 3. 检查节点资源压力：`kubectl describe node <node-name> | grep -A 10 "Allocated resources"`
> 请告诉我哪些 Conditions 不是 True/Normal，以及 kubelet 状态。

**分支决策**：
- **M1**：磁盘/内存压力 → 清理资源或扩容节点
- **M2**：kubelet 停止 → 重启 kubelet，检查日志
- **M3**：节点网络不可达 → 升级决策点（基础设施团队介入）

---

### Round 3 — 分支 N：DNS 查询分析

**顾问指令**：
> CoreDNS 扩容后仍 OOM，可能存在异常大量 DNS 查询。需要分析来源。
>
> 1. 检查查询量：`kubectl logs <coredns-pod-name> -n kube-system --tail=100 | grep -c "\[INFO\]"`
>    **如果无法执行** → `kubectl logs <coredns-pod-name> -n kube-system --tail=100 | wc -l`
> 2. 查看高频查询域名：`kubectl logs <coredns-pod-name> -n kube-system --tail=500 | grep "\[INFO\]" | awk '{print $4}' | sort | uniq -c | sort -rn | head -20`
>    **如果无法执行 awk** → `kubectl logs <coredns-pod-name> -n kube-system --tail=200`，把日志贴给我分析
> 3. 找到来源 IP：`kubectl logs <coredns-pod-name> -n kube-system --tail=200 | grep "\[INFO\]" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | sort | uniq -c | sort -rn | head -10`
> 请告诉我：哪些域名被高频查询？来源 IP 是什么？

**分支决策**：
- **N1**：某个应用疯狂查询不存在域名 → 联系应用团队修复，临时加 hosts 缓解
- **N2**：DNS 放大攻击特征 → 启用 CoreDNS cache 和 rate limit，必要时隔离 Pod
- **N3**：无异常查询量 → CoreDNS 可能存在内存泄漏，建议升级版本

---

## 验证修复

**顾问指令**：
> 修复已应用，验证 DNS 是否恢复正常。
>
> 1. 验证集群内部 DNS：`kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default`
>    **如果无法执行** → `kubectl exec -it <pod-name> -n <namespace> -- nslookup kubernetes.default`
>    **如果没有可用 Pod** → `kubectl get svc kubernetes -n default`
> 2. 验证外部 DNS：`kubectl run dns-verify-ext --image=busybox:1.36 --rm -it --restart=Never -- nslookup google.com`
>    **如果无法执行** → `kubectl exec -it <pod-name> -n <namespace> -- nslookup google.com`
> 3. 验证跨命名空间服务发现：`kubectl run dns-verify-cross --image=busybox:1.36 --rm -it --restart=Never -- nslookup <service>.<ns>.svc.cluster.local`
> 4. 检查 CoreDNS Pod 状态：`kubectl get pods -n kube-system -l k8s-app=kube-dns`
> 请告诉我以上四个验证结果。如果全部通过，问题已修复。

---

## 升级决策点

| 条件 | 升级路径 | 说明 |
|------|---------|------|
| 修复后 DNS 仍间歇性失败 | **SKILL-NET-003** | 怀疑 CNI 问题 |
| CoreDNS 正常但节点间不一致 | **SKILL-NET-003** | 可能涉及网络策略或 CNI |
| 多节点同时 DNS 问题 | **基础设施团队** | 底层网络基础设施问题 |
| 怀疑安全策略或 RBAC 阻断 | **SKILL-SEC-003** | 安全策略相关 |
| 需要修改节点系统级配置 | **节点管理团队** | systemd-resolved、kubelet 等 |
| 应用层 DNS 查询异常 | **应用团队** | 需要应用层修复 |

**顾问升级话术**：
> 根据目前排查结果，这个问题超出了常规 DNS 问题处理范围，可能涉及 **{具体原因}**。建议：
>
> 1. **立即止损**：临时在应用 Pod `/etc/hosts` 中添加关键域名映射，或切换到节点 DNS
> 2. **升级诊断**：我会整理当前收集的所有信息，你可以提交给 **{升级目标团队}**
> 3. **持续监控**：继续观察 CoreDNS 内存和查询量指标，必要时深度网络抓包
>
> 是否需要我帮你整理排查结果摘要？

---

## 附录：常用命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Pod 内 DNS 测试
kubectl exec -it <pod> -n <ns> -- nslookup <domain>
kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <domain>

# CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs <coredns-pod> -n kube-system --tail=50

# DNS 配置
kubectl exec -it <pod> -n <ns> -- cat /etc/resolv.conf
kubectl get configmap coredns -n kube-system -o yaml

# 常用修复
kubectl rollout restart deployment coredns -n kube-system
kubectl scale deployment coredns -n kube-system --replicas=3
kubectl patch deployment <deploy> -n <ns> --type merge -p '{"spec":{"template":{"spec":{"dnsPolicy":"ClusterFirst"}}}}'
```
---

*对话脚本版本: 1.0.0 | 技能: DNS 解析问题诊断与修复 | 模式: L2-semi-auto*


<!-- risk-assessed -->
