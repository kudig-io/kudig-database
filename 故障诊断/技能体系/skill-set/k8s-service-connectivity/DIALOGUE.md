---
title: 服务连通性问题 — 远程顾问对话脚本
summary: 服务连通性问题的远程顾问对话脚本，覆盖Service、Endpoint、kube-proxy排查。
category: troubleshooting
tags:
- networking
- remote-consultant
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
dialogue_id: DIALOGUE-SKILL-NET-002
skill_id: SKILL-NET-002
version: 1.0.0
role: remote-consultant
language: zh
relationships:
- target: '[[技能/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[实体/cilium.md]]'
  type: uses
- target: '[[实体/coredns.md]]'
  type: uses
- target: '[[实体/deployment.md]]'
  type: uses
- target: '[[系统基础/知识字典/networking/ingress.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Service 连通性问题诊断 — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**，只能通过对话指导现场工程师执行操作。

---

## 对话入口

### 入口 A：工程师明确报告 Service 访问失败

**工程师**：「Pod 连不上 Service」/「Service IP 不通」/「Connection refused」/「No route to host」

**顾问回应**：
> 收到，Service 连通性问题直接影响微服务间通信。作为远程顾问，我无法直连你的集群，请你配合执行检查命令。先回答三个问题（30 秒内）：
> 1. **影响范围**：多少 Service / Pod / 命名空间受影响？
> 2. **紧急程度**：业务是否已中断？是否有用户投诉？
> 3. **发生时间**：问题是突然发生还是逐渐恶化？最近是否有服务发布、扩缩容或网络变更？

---

### 入口 B：工程师报告应用连接超时（疑似 Service 问题）

**工程师**：「服务间调用超时」/「应用日志显示连接失败」/「间歇性连不上下游服务」

**顾问回应**：
> 服务间调用超时可能由多种原因引起，Service 层问题是常见根因之一。先确认问题是否出在 Service 层。请确认：1. 应用配置的下游地址是 Service 名称还是 Pod IP？2. 超时是否发生在特定时间？3. 是持续失败还是间歇性失败？
> 然后执行基础检查：`kubectl get svc -n <namespace>`
> **如果无法执行** → 通过集群管理控制台查看 Service 列表
> **如果控制台也没有** → 请告诉我应用配置中使用的 Service 名称和命名空间
> 请把 Service 列表或相关信息贴给我。

---

### 入口 C：工程师报告 Endpoints 为空或缺失

**工程师**：「Endpoints 是空的」/「Service 没有后端 Pod」/「kubectl get endpoints 没有 IP」

**顾问回应**：
> Endpoints 为空是 Service 连通性问题的典型症状，通常由 Selector 不匹配或后端 Pod 未 Ready 引起。请执行：
> ```bash
> kubectl get endpoints -n <namespace>
> kubectl get svc -n <namespace> -o wide
> ```
> **如果无法执行** → `kubectl get svc,endpoints -n <namespace>`
> **如果连这个也不行** → 请告诉我：Service 名称是什么？它的 ClusterIP 是多少？
> 请把输出贴给我。

---

### 入口 D：工程师报告外部访问 Service 失败（NodePort/LoadBalancer）

**工程师**：「NodePort 不通」/「LoadBalancer 外部 IP 无法访问」/「[[系统基础/知识字典/networking/ingress.md|Ingress]] 后端超时」

**顾问回应**：
> 外部访问失败涉及 Service 网络链路的多个环节。先定位是 Service 内部问题还是外部暴露层问题。请执行：
> ```bash
> kubectl get svc -n <namespace> | grep -E "NodePort|LoadBalancer"
> ```
> **如果无法执行** → `kubectl get svc -n <namespace>`，手动找出类型为 NodePort 或 LoadBalancer 的 Service
> **如果无法 get svc** → 请告诉我 Service 名称和类型（ClusterIP/NodePort/LoadBalancer）
> 同时检查 Service 的 Endpoints：`kubectl get endpoints <service-name> -n <namespace>`
> 请把 Service 类型、外部 IP/端口和 Endpoints 信息贴给我。

---

## Round 1：快速定位问题层级

> 目标：判断问题发生在 **Service 配置 → Endpoints → Pod 就绪 → kube-proxy → 网络策略** 的哪一层。

---

### Round 1 — 分支 A：Service 存在但访问超时/拒绝

**工程师反馈**：Service 存在，但从客户端 Pod 访问超时或 Connection refused。

**顾问指令**：
> 请获取 Service 详细信息并进行基础连通性测试。
> 1. 查看 Service 详情：`kubectl describe svc <service-name> -n <namespace>`
> **如果无法执行** → `kubectl get svc <service-name> -n <namespace> -o yaml`
> 2. 测试 Service 直接访问（从集群内 Pod）：`kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nc -vz <service-cluster-ip> <service-port>`
> **如果无法创建临时 Pod** → `kubectl exec -it <client-pod> -n <client-ns> -- nc -vz <service-cluster-ip> <service-port>`
> **如果没有 nc** → `kubectl exec -it <client-pod> -n <client-ns> -- sh -c "echo '' > /dev/tcp/<service-cluster-ip>/<service-port>"`
> 3. 检查 Endpoints：`kubectl get endpoints <service-name> -n <namespace>`
> **如果无法执行** → `kubectl describe svc <service-name> -n <namespace> | grep -A 5 Endpoints`

**分支决策**：
- **A1**：Endpoints 为空 → Round 2 — 分支 A（Endpoints 缺失排查）
- **A2**：Endpoints 有 IP 但 nc/curl 不通 → Round 2 — 分支 B（kube-proxy/网络层排查）
- **A3**：Endpoints 有 IP 且 nc 通，但应用层超时 → Round 2 — 分支 C（应用层/端口排查）

---

### Round 1 — 分支 B：多个 Service 同时不可达

**工程师反馈**：多个 Service 或整个命名空间的服务发现失效。

**顾问指令**：
> 多个 Service 同时问题通常指向集群级组件异常（kube-proxy、[[实体/coredns.md|CoreDNS]]、CNI）。先确认范围。
> 1. 检查 kube-proxy Pod 状态：`kubectl get pods -n kube-system -l k8s-app=kube-proxy`
> **如果没有 kube-proxy 标签** → `kubectl get pods -n kube-system | grep -i proxy`
> 2. 检查 CoreDNS 状态：`kubectl get pods -n kube-system -l k8s-app=kube-dns`
> **如果没有 kube-dns 标签** → `kubectl get pods -n kube-system | grep -i dns`
> 3. 检查 CNI 组件状态：`kubectl get pods -n kube-system | grep -E 'calico|cilium|flannel|weave|antrea'`
> **如果无法执行** → `kubectl get pods -n kube-system`，把非 Running 的 Pod 告诉我
> 4. 检查节点状态：`kubectl get nodes`
> **如果无法执行** → 请告诉我节点数量和是否有 NotReady 节点

**分支决策**：
- **B1**：kube-proxy Pod 异常或部分节点缺失 → Round 2 — 分支 D（kube-proxy 修复）
- **B2**：CoreDNS Pod 异常 → 升级至 [[技能/skill-k8s-node-notready-SKILL.md|SKILL]]-NET-001（DNS 问题诊断）
- **B3**：CNI Pod 异常 → 升级至 SKILL-NET-003（网络深度诊断）
- **B4**：节点 NotReady → 升级至 SKILL-NODE-001（节点问题诊断）

---

### Round 1 — 分支 C：外部访问 NodePort/LoadBalancer 失败

**工程师反馈**：集群内部可以访问 Service，但外部无法通过 NodePort 或 LoadBalancer 访问。

**顾问指令**：
> 外部访问失败可能涉及 Service 外部暴露配置、云提供商集成或节点防火墙。
> 1. 检查 Service 外部端口配置：`kubectl get svc <service-name> -n <namespace> -o wide`
> **如果无法执行** → `kubectl describe svc <service-name> -n <namespace> | grep -E "Type|NodePort|ExternalIP|LoadBalancer Ingress"`
> 2. 检查 LoadBalancer 状态：`kubectl get svc <service-name> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress[*].ip}'`
> **如果无法执行 jsonpath** → `kubectl describe svc <service-name> -n <namespace> | grep -A 5 "LoadBalancer Ingress"`
> 3. 检查 NodePort 在每个节点是否监听：`ssh <node> "ss -tlnp | grep <node-port>"`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- ss -tlnp | grep <node-port>`

**分支决策**：
- **C1**：LoadBalancer 外部 IP 未分配 → Round 2 — 分支 E（云提供商/负载均衡排查）
- **C2**：NodePort 在节点上未监听 → Round 2 — 分支 D（kube-proxy 修复）
- **C3**：NodePort 已监听但外部不通 → Round 2 — 分支 F（防火墙/安全组排查）

---

### Round 1 — 分支 ACK-C：阿里云SLB健康检查与Terway网络排查（ACK特有）

**工程师反馈**：「LoadBalancer Service外部不通」/「SLB健康检查失败」/「Terway模式下Pod间通信异常」/「ACK Pro集群Service访问超时」

**顾问指令**：
> 阿里云 ACK 环境中，Service 连通性涉及 **SLB负载均衡**、**Terway网络插件**、**CCM（cloud-controller-manager）** 等特有组件。请按以下步骤排查：
>
> **步骤 1：阿里云SLB健康检查状态检查**
> ```bash
> # 查看 Service 的 LoadBalancer ID（ACK通过注解关联SLB）
> kubectl get svc <service-name> -n <namespace> -o yaml | grep -E "slb-id|loadbalancer"
> ```
> **如果无法执行** → 请登录 **阿里云控制台 > 负载均衡 SLB**，告诉我：
> 1. 与该 Service 关联的 SLB 实例状态是否为 **运行中**？
> 2. SLB 的 **监听** 配置中，健康检查状态是否为 **异常**？
> 3. SLB **后端服务器组** 中，ECS 实例的健康状态是 **可用** 还是 **不可用**？
>
> **步骤 2：CCM（cloud-controller-manager）状态检查**
> ```bash
> # 查看 CCM Pod 状态和日志
> kubectl get pods -n kube-system | grep cloud-controller
> kubectl logs -n kube-system -l app=cloud-controller-manager --tail=50
> ```
> **如果无法执行** → 请登录 **ACK 控制台 > 集群 > 组件管理**，确认：
> 1. **cloud-controller-manager** 组件是否正常运行？
> 2. 该组件是否有异常事件或重启记录？
>
> **步骤 3：Terway网络模式排查**
> ```bash
> # 确认集群使用的网络插件模式（Terway/Flannel）
> kubectl get configmap terway-config -n kube-system -o yaml | grep -i "network_policy|eni"
> # 查看 Terway Pod 状态
> kubectl get pods -n kube-system -l app=terway-eniip -o wide
> # 检查Pod的ENI辅助IP分配情况
> kubectl get pod <pod-name> -n <namespace> -o yaml | grep -E "eni|vpc"
> ```
> **如果无法执行 kubectl** → 请登录 **ACK 控制台 > 集群 > 集群信息**，确认：
> 1. 集群 **网络插件** 是 **Terway** 还是 **Flannel**？
> 2. 如果是 Terway，是 **ENI** 模式还是 **ENIIP** 模式？
>
> **步骤 4：Pod 网络连通性测试（Terway特有）**
> ```bash
> # 在Pod所在节点上检查ENI和路由
> ssh <node-ip> "ip addr show | grep eni"
> ssh <node-ip> "ip route | grep <pod-ip>"
> # 检查Terway分配的VSwitch和安全组
> kubectl logs -n kube-system <terway-pod-name> --tail=50 | grep -i "error|fail|timeout"
> ```
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- ip addr show`

**阿里云ACK Service特有诊断矩阵**：

| ACK特有场景 | 诊断方法 | 修复方案 |
|:---|:---|:---|
| SLB健康检查失败导致后端被摘除 | SLB控制台查看健康检查日志 / `curl <pod-ip>:<port>/healthz` | 修正Pod readinessProbe路径，或调整SLB健康检查端口/路径 |
| SLB实例配额超限 | 阿里云控制台查看SLB配额 | 释放闲置SLB实例，或申请配额提升 |
| CCM无法同步SLB后端 | `kubectl logs -n kube-system -l app=cloud-controller-manager` | 重启CCM Pod，或检查CCM的RAM角色权限 |
| Terway ENI IP 耗尽 | `kubectl logs -n kube-system -l app=terway-eniip` | 扩容节点交换机（VSwitch）的可用IP段 |
| Terway安全组规则阻断 | 控制台检查节点安全组与ENI安全组差异 | 统一安全组规则，确保Service端口放行 |
| 专有云中CCM与Apsara LB对接异常 | 专有云ASO/天基控制台查看CCM组件状态 | 联系阿里云驻场工程师修复底座网络服务 |
| ACK Serverless（ASK）Service异常 | ASK控制台查看SLB与Pod映射关系 | 检查Virtual Node的vSwitch配置和安全组 |

> **远程顾问无法直连时的阿里云控制台排查**：
> 1. **SLB 控制台**：检查 SLB 实例监听的健康检查状态，确认后端 ECS 是否全部被标记为 **异常**
> 2. **ACK 控制台 > 集群 > 节点管理**：确认节点状态为 **正常**，且节点安全组允许 SLB 网段访问
> 3. **ACK 控制台 > 集群 > 运维管理 > 网络诊断**：使用 ACK 内置网络诊断工具检查 Service 连通性
> 4. **云监控 > 负载均衡**：查看 SLB 的 **后端健康状态** 和 **流量监控**
> 5. 如果是 **专有云**，请通过 **ASO/天基控制台** 查看 **Apsara LoadBalancer** 服务状态

**分支决策**：
- **ACK-C1**：SLB健康检查失败 → 修正Pod readinessProbe，或调整SLB健康检查配置
- **ACK-C2**：CCM组件异常 → 重启CCM，或检查CCM RAM角色权限
- **ACK-C3**：Terway网络异常（ENI/IP耗尽/安全组阻断）→ 扩容ENI IP，或修正安全组规则
- **ACK-C4**：专有云平台网络底座异常 → 升级至阿里云TAM/驻场工程师

---

## Round 2：分层深入诊断

> 目标：根据 Round 1 确定的层级，执行针对性的深度检查。

---

### Round 2 — 分支 A：Endpoints 缺失排查

**顾问指令**：
> Endpoints 为空说明 Service 的 Selector 没有匹配到就绪的 Pod。请排查。
> 1. 检查 Service 的 Selector：`kubectl get svc <service-name> -n <namespace> -o jsonpath='{.spec.selector}'`
> **如果无法执行 jsonpath** → `kubectl get svc <service-name> -n <namespace> -o yaml | grep -A 5 selector`
> 2. 检查是否有 Pod 匹配该 Selector：`kubectl get pods -n <namespace> -l <selector-key>=<selector-value>`
> **例如**：`kubectl get pods -n <namespace> -l app=nginx`
> **如果无法执行** → `kubectl get pods -n <namespace> -o wide`，手动查看 Pod 标签
> 3. 检查匹配到的 Pod 是否 Ready：`kubectl get pods -n <namespace> -l <selector-key>=<selector-value> -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'`
> **如果无法执行 jsonpath** → `kubectl get pods -n <namespace> -l <selector-key>=<selector-value>`，告诉我 READY 列的值
> 4. 检查 Pod 的 readinessProbe：`kubectl describe pod <pod-name> -n <namespace> | grep -A 5 "Readiness"`
> **如果无法执行** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 10 readinessProbe`

**分支决策**：
- **A1**：Service Selector 错误（拼写错误或标签不匹配）→ Round 3 — 分支 G（修正 Selector）
- **A2**：Selector 正确但 Pod 未 Ready → Round 3 — 分支 H（排查 Pod 就绪状态）
- **A3**：Pod 已 Ready 但 Endpoints 仍为空 → Round 3 — 分支 I（EndpointSlice/controller 排查）

---

### Round 2 — 分支 B：kube-proxy/网络层排查

**顾问指令**：
> Endpoints 有 IP 但 Service IP 不通，说明 kube-proxy 的转发规则可能异常。
> 1. 检查 kube-proxy Pod 状态：`kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide`
> **如果无法执行** → `kubectl get pods -n kube-system | grep -i proxy`
> 2. 检查 Service 所在节点的 iptables/ipvs 规则：`iptables -t nat -L KUBE-SERVICES | grep <service-cluster-ip>`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- iptables -t nat -L KUBE-SERVICES | grep <service-cluster-ip>`
> **如果使用 IPVS** → `ipvsadm -Ln | grep <service-cluster-ip>`
> 3. 检查 kube-proxy 日志：`kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50`
> **如果无法执行** → `kubectl logs <kube-proxy-pod-name> -n kube-system --tail=50`
> **如果 logs 为空** → `kubectl get events -n kube-system --field-selector reason=Failed | grep -i proxy | tail -20`

**分支决策**：
- **B1**：kube-proxy Pod CrashLoopBackOff 或频繁重启 → Round 3 — 分支 J（重启/修复 kube-proxy）
- **B2**：iptables/ipvs 规则缺失或错误 → Round 3 — 分支 K（重建 iptables/ipvs 规则）
- **B3**：kube-proxy 日志显示 CNI/网络错误 → 升级至 SKILL-NET-003（网络深度诊断）

---

### Round 2 — 分支 C：应用层/端口排查

**顾问指令**：
> Service IP 可连通（TCP 握手成功），但应用层超时，说明问题出在 Pod 应用端口或应用本身。
> 1. 检查 Service 的 targetPort 是否正确：`kubectl get svc <service-name> -n <namespace> -o jsonpath='{range .spec.ports[*]}{.port}{"->"}{.targetPort}{"\n"}{end}'`
> **如果无法执行 jsonpath** → `kubectl get svc <service-name> -n <namespace> -o yaml | grep -A 10 "ports:"`
> 2. 直接测试 Pod IP 和 targetPort：`kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nc -vz <pod-ip> <target-port>`
> **如果无法创建临时 Pod** → `kubectl exec -it <client-pod> -n <client-ns> -- nc -vz <pod-ip> <target-port>`
> **如果没有 nc** → `kubectl exec -it <client-pod> -n <client-ns> -- sh -c "echo '' > /dev/tcp/<pod-ip>/<target-port>"`
> 3. 检查 Pod 内的端口监听：`kubectl exec -it <pod-name> -n <namespace> -- ss -tlnp`
> **如果没有 ss** → `kubectl exec -it <pod-name> -n <namespace> -- netstat -tlnp`
> **如果没有 netstat** → `kubectl exec -it <pod-name> -n <namespace> -- sh -c "cat /proc/net/tcp"`

**分支决策**：
- **C1**：targetPort 与 Pod 实际监听端口不一致 → Round 3 — 分支 L（修正 targetPort）
- **C2**：Pod 内端口未监听 → Round 3 — 分支 M（应用启动/端口绑定排查）
- **C3**：Pod 端口已监听但直连仍超时 → Round 3 — 分支 N（应用性能/连接数排查）

---

### Round 2 — 分支 D：kube-proxy 修复

**顾问指令**：
> kube-proxy 异常会导致 Service 转发规则无法维护。请按以下步骤排查和修复。
> 1. 确认 kube-proxy 运行模式：`kubectl get configmap kube-proxy -n kube-system -o yaml | grep -i mode`
> **如果无法执行** → 检查 kube-proxy DaemonSet 参数：`kubectl get daemonset kube-proxy -n kube-system -o yaml | grep -i mode`
> 2. 检查 kube-proxy 日志中的错误：`kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100 | grep -i "error|fail|warn"`
> **如果无法执行** → 逐个查看 kube-proxy Pod：`kubectl logs <kube-proxy-pod> -n kube-system --tail=100`
> 3. 检查 kube-proxy 配置是否冲突：`kubectl get configmap kube-proxy -n kube-system -o yaml > /tmp/kube-proxy-config.yaml`
> **如果无法备份** → 直接查看：`kubectl get configmap kube-proxy -n kube-system -o yaml | head -50`

**分支决策**：
- **D1**：kube-proxy 配置错误 → Round 3 — 分支 O（修正 kube-proxy 配置）
- **D2**：kube-proxy Pod 异常但配置正确 → Round 3 — 分支 J（重启 kube-proxy）
- **D3**：kube-proxy 正常但规则仍异常 → Round 3 — 分支 K（重建规则）

---

### Round 2 — 分支 E：云提供商/负载均衡排查

**顾问指令**：
> LoadBalancer 外部 IP 未分配，通常与云提供商控制器或注解配置有关。
> 1. 检查 Service 的 LoadBalancer 状态：`kubectl get svc <service-name> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress}'`
> **如果无法执行 jsonpath** → `kubectl describe svc <service-name> -n <namespace> | grep -A 10 "LoadBalancer Ingress"`
> 2. 检查云提供商控制器 Pod：`kubectl get pods -n kube-system | grep -E 'cloud-controller|aws-cloud|azure-cloud|gcp-cloud|alicloud'`
> **如果无法执行** → `kubectl get pods --all-namespaces | grep -i cloud`
> 3. 检查 Service 注解：`kubectl get svc <service-name> -n <namespace> -o jsonpath='{.metadata.annotations}'`
> **如果无法执行** → `kubectl describe svc <service-name> -n <namespace> | grep -A 20 Annotations`

**分支决策**：
- **E1**：云控制器 Pod 异常 → 升级决策点（云提供商支持）
- **E2**：Service 注解配置错误 → Round 3 — 分支 P（修正 Service 注解）
- **E3**：云配额/SLB 数量超限 → 升级决策点（云提供商控制台处理）

---

### Round 2 — 分支 F：防火墙/安全组排查

**顾问指令**：
> NodePort 在节点上已监听但外部不通，说明防火墙或安全组可能阻断了流量。
> 1. 检查节点防火墙规则：`iptables -L -n | grep <node-port>`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- iptables -L -n | grep <node-port>`
> **如果使用 firewalld** → `ssh <node-ip> "firewall-cmd --list-ports"`
> **如果使用 ufw** → `ssh <node-ip> "ufw status"`
> 2. 检查云安全组/ACL：登录云控制台，检查节点安全组的入站规则是否允许 NodePort 范围（默认 30000-32767）；确认是否有 Network ACL 限制；确认是否配置了源 IP 白名单
> 3. 检查 NetworkPolicy：`kubectl get networkpolicy -n <namespace>`
> **如果无法执行** → `kubectl get networkpolicy --all-namespaces`
> **如果使用 [[实体/cilium.md|Cilium]]** → `kubectl get ciliumnetworkpolicy -n <namespace>`

**分支决策**：
- **F1**：节点防火墙阻断 → Round 3 — 分支 Q（开放防火墙端口）
- **F2**：云安全组/ACL 阻断 → Round 3 — 分支 R（调整安全组规则）
- **F3**：NetworkPolicy 阻断入站 → Round 3 — 分支 S（修正 NetworkPolicy）

---

## Round 3：精确修复与验证

> 目标：执行最终修复动作，验证 Service 连通性恢复正常，决定是否升级。

---

### Round 3 — 分支 G：修正 Service Selector

**顾问指令**：
> Service 的 Selector 与 Pod 标签不匹配，需要修正。
> 1. 查看当前 Service Selector 和 Pod 实际标签：`kubectl get svc <service-name> -n <namespace> -o yaml | grep -A 5 selector`
> `kubectl get pods -n <namespace> -l app=<current-label> --show-labels`
> **如果无法执行 show-labels** → `kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.labels}'`
> 2. 修正 Service Selector：`kubectl patch svc <service-name> -n <namespace> --type merge -p '{"spec":{"selector":{"app":"<correct-label>"}}}'`
> **如果无法 patch** → `kubectl edit svc <service-name> -n <namespace>`（手动修改）
> **如果无交互式终端** → 准备修正后的 YAML 文件：`kubectl apply -f fixed-service.yaml`
> 3. 验证 Endpoints 是否填充：`kubectl get endpoints <service-name> -n <namespace>`
> **如果无法执行** → `kubectl get svc <service-name> -n <namespace> -o jsonpath='{.subsets[*].addresses[*].ip}'`

**分支决策**：
- **G1**：Endpoints 已填充，访问恢复 → 修复完成
- **G2**：Endpoints 仍为空 → 检查 Pod 是否 Ready，返回 Round 2 — 分支 A
- **G3**：无法修改 Service（权限不足）→ 升级决策点

---

### Round 3 — 分支 H：排查 Pod 就绪状态

**顾问指令**：
> Pod 未 Ready 导致无法加入 Endpoints。请排查 Pod 未就绪的原因。
> 1. 查看 Pod 状态和条件：`kubectl describe pod <pod-name> -n <namespace> | grep -A 20 Conditions`
> **如果无法执行** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 20 conditions`
> 2. 检查 readinessProbe 配置：`kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 15 readinessProbe`
> **如果无法执行** → `kubectl describe pod <pod-name> -n <namespace> | grep -A 10 "Readiness probe"`
> 3. 检查 Pod 日志：`kubectl logs <pod-name> -n <namespace> --tail=50`
> **如果无法执行** → `kubectl logs <pod-name> -n <namespace> --previous --tail=50`
> **如果 logs 也拿不到** → 检查事件：`kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> | tail -20`

**分支决策**：
- **H1**：readinessProbe 配置错误（端口/路径错误）→ 修正 readinessProbe 后重新部署
- **H2**：应用启动失败导致未 Ready → 升级至 SKILL-POD-001（Pod 问题诊断）
- **H3**：Pod 因资源限制无法启动 → 升级至 SKILL-NODE-001（节点资源排查）

---

### Round 3 — 分支 I：EndpointSlice/Controller 排查

**顾问指令**：
> Pod 已 Ready 但 Endpoints 仍为空，可能是 EndpointSlice Controller 或 EndpointSlice 资源异常。
> 1. 检查 EndpointSlice：`kubectl get endpointslices -n <namespace> | grep <service-name>`
> **如果无法执行** → `kubectl get endpointslices --all-namespaces`
> **如果无 EndpointSlice 权限** → `kubectl get endpoints -n <namespace> <service-name> -o yaml`
> 2. 检查 kube-controller-manager 状态：`kubectl get pods -n kube-system | grep -i controller`
> **如果无法执行** → `kubectl get pods -n kube-system`
> 3. 检查 EndpointSlice Controller 日志：`kubectl logs -n kube-system -l component=kube-controller-manager --tail=50 | grep -i endpoint`
> **如果无法执行** → `kubectl logs <kube-controller-manager-pod> -n kube-system --tail=100 | grep -i "endpoint|error"`

**分支决策**：
- **I1**：EndpointSlice 存在但地址为空 → 删除重建 Service
- **I2**：kube-controller-manager 异常 → 升级决策点（控制平面问题）
- **I3**：EndpointSlice 完全不存在 → 删除并重建 Service

---

### Round 3 — 分支 J：重启 kube-proxy

**顾问指令**：
> kube-proxy Pod 异常，需要重启恢复。
> 1. 重启 kube-proxy（DaemonSet 方式）：`kubectl rollout restart daemonset kube-proxy -n kube-system`
> **如果无法执行** → `kubectl delete pod -n kube-system -l k8s-app=kube-proxy`
> **如果无删除权限** → `kubectl get daemonset kube-proxy -n kube-system`，确认后请管理员重启节点 kube-proxy 服务
> 2. 验证 kube-proxy 重启成功：`kubectl get pods -n kube-system -l k8s-app=kube-proxy`
> **如果无法执行** → 间隔 30 秒执行 `kubectl get pods -n kube-system | grep proxy`
> 3. 验证 Service 规则恢复：`kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nc -vz <service-cluster-ip> <service-port>`

**分支决策**：
- **J1**：kube-proxy 重启后规则恢复 → 修复完成
- **J2**：重启后仍异常 → Round 3 — 分支 O（修正 kube-proxy 配置）
- **J3**：无权限重启 → 升级决策点

---

### Round 3 — 分支 K：重建 iptables/ipvs 规则

**顾问指令**：
> kube-proxy 规则缺失或错误，需要强制重建。
> 1. 在问题节点上重建规则（如可 SSH）：`iptables -t nat -F KUBE-SERVICES && iptables -t nat -F KUBE-POSTROUTING && iptables -t nat -F KUBE-FORWARD`
> **如果无法 SSH** → 创建特权 Pod 在节点网络命名空间执行
> **注意**：此操作会清空 kube-proxy 管理的规则，kube-proxy 会自动重建，但会有短暂中断
> 2. 触发 kube-proxy 重新同步：`kubectl delete pod -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name>`
> **如果无法执行** → 修改 kube-proxy ConfigMap 添加无关注解触发滚动更新
> 3. 如果使用 IPVS 模式：`ipvsadm --clear`
> **如果无法 SSH** → 重启 kube-proxy 后等待自动重建

**分支决策**：
- **K1**：规则重建后 Service 恢复 → 修复完成
- **K2**：规则重建后仍异常 → 升级至 SKILL-NET-003（网络深度诊断）
- **K3**：无法执行规则重建 → 升级决策点

---

### Round 3 — 分支 L：修正 targetPort

**顾问指令**：
> Service 的 targetPort 与 Pod 实际监听端口不匹配，需要修正。
> 1. 查看当前 Service 端口映射：`kubectl get svc <service-name> -n <namespace> -o yaml | grep -A 20 "ports:"`
> **如果无法执行** → `kubectl describe svc <service-name> -n <namespace> | grep -A 10 "Port:"`
> 2. 查看 Pod 实际监听端口：`kubectl exec -it <pod-name> -n <namespace> -- ss -tlnp`
> **如果没有 ss** → `kubectl exec -it <pod-name> -n <namespace> -- netstat -tlnp`
> **如果无法 exec** → 查看应用配置文件或 Dockerfile 中的 EXPOSE 指令
> 3. 修正 targetPort：`kubectl patch svc <service-name> -n <namespace> --type merge -p '{"spec":{"ports":[{"port":<port>,"targetPort":<correct-port>}]}}'`
> **如果无法 patch** → `kubectl edit svc <service-name> -n <namespace>`
> **如果无交互式终端** → `kubectl apply -f fixed-service.yaml`

**分支决策**：
- **L1**：targetPort 修正后访问恢复 → 修复完成
- **L2**：targetPort 正确但应用层仍超时 → Round 3 — 分支 N（应用性能排查）
- **L3**：无法修改 Service → 升级决策点

---

### Round 3 — 分支 S：修正 NetworkPolicy

**顾问指令**：
> NetworkPolicy 阻断了到 Service 的流量，需要修正策略规则。
> 1. 查看当前 NetworkPolicy：`kubectl get networkpolicy -n <namespace> -o yaml`
> **如果无法执行** → `kubectl get networkpolicy -n <namespace>`，逐个 describe
> **如果使用 Cilium** → `kubectl get ciliumnetworkpolicy -n <namespace>`
> 2. 添加允许规则（示例 YAML）：创建允许 Service 流量的 NetworkPolicy，允许来自所有命名空间的 Pod 通过 TCP 访问目标端口
> **如果无法 apply**（权限不足）→ 把修改后 YAML 给管理员审批后执行
> 3. 验证：`kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nc -vz <service-cluster-ip> <service-port>`

**分支决策**：
- **S1**：添加 allow 规则后访问恢复 → 修复完成
- **S2**：修改后仍不通 → 检查 CNI 全局策略，升级决策
- **S3**：无权限修改 → 升级决策点

---

## 验证修复

**顾问指令**：
> 修复已应用，验证 Service 连通性是否恢复正常。
> 1. 验证 Endpoints 已填充：`kubectl get endpoints <service-name> -n <namespace>`
> **如果无法执行** → `kubectl describe svc <service-name> -n <namespace> | grep -A 5 Endpoints`
> 2. 验证 Service IP 可达：`kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nc -vz <service-cluster-ip> <service-port>`
> **如果无法创建临时 Pod** → `kubectl exec -it <client-pod> -n <client-ns> -- nc -vz <service-cluster-ip> <service-port>`
> **如果没有 nc** → `kubectl exec -it <client-pod> -n <client-ns> -- sh -c "echo '' > /dev/tcp/<service-cluster-ip>/<service-port>"`
> 3. 验证 DNS 解析（如通过 Service 名访问）：`kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <service-name>.<namespace>.svc.cluster.local`
> **如果无法执行** → `kubectl exec -it <client-pod> -n <client-ns> -- nslookup <service-name>.<namespace>.svc.cluster.local`
> 4. 验证应用层连通（HTTP 服务）：`kubectl run curl-test --image=curlimages/curl --rm -it --restart=Never -- http://<service-cluster-ip>:<service-port>/health`
> **如果无法执行** → `kubectl exec -it <client-pod> -n <client-ns> -- curl -s http://<service-cluster-ip>:<service-port>/health`
> 5. 如果是外部访问，验证 NodePort/LoadBalancer：`curl -v http://<node-ip>:<node-port>/health`
> **如果无法从外部测试** → `kubectl run ext-test --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"hostNetwork":true}}' -- curl -s http://<node-ip>:<node-port>/health`
> 请告诉我以上验证结果。如果全部通过，问题已修复。

---

## 升级决策点

| 条件 | 升级路径 | 说明 |
|------|---------|------|
| 涉及 DNS 解析失败 | **SKILL-NET-001** | DNS 问题诊断 |
| 涉及 CNI/底层网络问题 | **SKILL-NET-003** | 网络深度诊断 |
| 涉及节点 NotReady 或资源不足 | **SKILL-NODE-001** | 节点问题诊断 |
| 涉及控制平面组件异常 | **控制平面团队** | kube-controller-manager 等 |
| 涉及云提供商负载均衡问题 | **云提供商支持** | SLB/ELB/NLB 相关问题 |
| 涉及安全策略或准入控制 | **SKILL-SEC-003** | 安全策略相关 |
| 涉及应用层问题（Pod 崩溃） | **SKILL-POD-001** | Pod 问题诊断 |

**顾问升级话术**：
> 根据目前排查结果，这个问题超出了常规 Service 连通性问题处理范围，可能涉及 **{具体原因}**。建议：
> 1. **立即止损**：如果可能，临时将客户端切换到直接访问 Pod IP，或增加健康检查容错时间
> 2. **升级诊断**：我会整理当前收集的所有信息，你可以提交给 **{升级目标团队}**
> 3. **持续监控**：继续观察 Service Endpoints 和 kube-proxy 指标，必要时在节点抓包分析
> 是否需要我帮你整理排查结果摘要？

---

## 附录：常用命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Service 和 Endpoints
kubectl get svc,endpoints -n <namespace>
kubectl describe svc <svc> -n <namespace>
# 查看 Pod 标签（验证 Selector）
kubectl get pods -n <namespace> -l app=<label> --show-labels
# 测试 Service 连通性
kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never -- nc -vz <svc-ip> <port>
# 查看 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50
# 检查 iptables 规则（需节点权限）
iptables -t nat -L KUBE-SERVICES | grep <svc-ip>
# 检查 NetworkPolicy
kubectl get networkpolicy -n <namespace>
# 修改 Service Selector
kubectl patch svc <svc> -n <namespace> -p '{"spec":{"selector":{"app":"<label>"}}}'
# 修改 targetPort
kubectl patch svc <svc> -n <namespace> --type merge -p '{"spec":{"ports":[{"port":80,"targetPort":8080}]}}'
# 重启 kube-proxy
kubectl rollout restart daemonset kube-proxy -n kube-system
# 查看 EndpointSlice
kubectl get endpointslices -n <namespace>
```
---

### Round 1 — 分支 D：Service DNS 解析失败但 IP 可通

**工程师反馈**：通过 Service ClusterIP 访问正常，但通过 Service 名称访问超时，疑似 DNS 问题。

**顾问指令**：
> Service IP 可通但名称解析失败，说明问题可能不在 Service 本身，而在 CoreDNS 或 DNS 解析链路。先确认 DNS 层面。
> 1. 测试 DNS 解析：`kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <service-name>.<namespace>.svc.cluster.local`
> **如果无法创建临时 Pod** → `kubectl exec -it <client-pod> -n <client-ns> -- nslookup <service-name>.<namespace>.svc.cluster.local`
> **如果 nslookup 不存在** → `kubectl exec -it <client-pod> -n <client-ns> -- cat /etc/resolv.conf` 查看 nameserver 配置
> **如果无法 exec** → 查看 Pod 的 `/etc/resolv.conf`：`kubectl get pod <client-pod> -n <client-ns> -o jsonpath='{.spec.dnsConfig}'`
> 2. 检查 CoreDNS Pod 状态：`kubectl get pods -n kube-system -l k8s-app=kube-dns`
> **如果没有 kube-dns 标签** → `kubectl get pods -n kube-system | grep -i coredns`
> **如果无法执行** → `kubectl get pods -n kube-system`，告诉我是否有 coredns 相关 Pod
> 3. 检查 CoreDNS 日志：`kubectl logs -n kube-system -l k8s-app=kube-dns --tail=30`
> **如果无法执行** → `kubectl logs <coredns-pod> -n kube-system --tail=30`
> **如果 logs 为空** → `kubectl get events -n kube-system | grep -i dns | tail -20`

**分支决策**：
- **D1**：CoreDNS Pod 异常 → 升级至 SKILL-NET-001（DNS 问题诊断）
- **D2**：DNS 解析正常但应用仍超时 → 返回 Round 1 — 分支 A（Service 连通性排查）
- **D3**：resolv.conf 配置异常（如 search 域缺失）→ Round 3 — 分支 T（修正 DNS 配置）

---

### Round 1 — 分支 E：服务网格环境中 Service 访问异常

**工程师反馈**：已启用 Istio/Linkerd 的服务网格环境中，Pod 间 Service 访问返回 503 或连接重置。

**顾问指令**：
> 服务网格环境增加了 sidecar 代理层，问题可能出在 Envoy 配置、mTLS 协商或流量策略。请执行：
> 1. 确认 sidecar 注入状态：`kubectl get pod <client-pod> -n <namespace> -o jsonpath='{.spec.containers[*].name}'`
> **如果无法执行** → `kubectl describe pod <client-pod> -n <namespace> | grep -A 10 "Containers:"`
> **如果无法 describe** → `kubectl get pod <client-pod> -n <namespace> -o yaml | grep -A 20 containers`
> 2. 检查 Envoy sidecar 日志：`kubectl logs <client-pod> -n <namespace> -c istio-proxy --tail=50`
> **如果没有 istio-proxy 容器** → 确认是否使用其他网格：`kubectl logs <client-pod> -n <namespace> -c linkerd-proxy --tail=50`
> **如果无法执行** → 查看 sidecar 状态：`istioctl proxy-status | grep <client-pod>` 或 `linkerd stat pod -n <namespace>`
> 3. 检查 mTLS 策略：`kubectl get peerauthentication -n <namespace>`
> **如果无法执行** → `kubectl get peerauthentication --all-namespaces`
> **如果使用 Linkerd** → `linkerd identity pod/<client-pod> -n <namespace>`

**分支决策**：
- **E1**：sidecar 未注入 → Round 3 — 分支 W（重新注入 sidecar）
- **E2**：Envoy 日志显示 mTLS 握手失败 → Round 3 — 分支 X（调整 mTLS 策略）
- **E3**：AuthorizationPolicy 阻断 → Round 3 — 分支 Y（修正 AuthorizationPolicy）
- **E4**：非网格相关问题 → 返回 Round 1 — 分支 A

---

## Round 2 扩展分支

### Round 2 — 分支 G：Istio/Linkerd 网格层深度排查

**顾问指令**：
> 服务网格层的连通性问题需要检查 Envoy 配置和网格策略。请执行：
> 1. 查看 Envoy cluster 配置：`istioctl proxy-config cluster <client-pod>.<namespace> | grep <target-service>`
> **如果无法执行** → `kubectl exec <client-pod> -n <namespace> -c istio-proxy -- curl -s localhost:15000/clusters | grep <target-service>`
> **如果没有 istioctl** → `kubectl exec <client-pod> -n <namespace> -c istio-proxy -- curl -s localhost:15000/config_dump | grep -A 10 <target-service>`
> 2. 查看 Envoy listener 配置：`istioctl proxy-config listener <client-pod>.<namespace> | grep <service-port>`
> **如果无法执行** → `kubectl exec <client-pod> -n <namespace> -c istio-proxy -- curl -s localhost:15000/listeners`
> 3. 检查 DestinationRule 配置：`kubectl get destinationrule -n <namespace>`
> **如果无法执行** → `kubectl get destinationrule --all-namespaces`
> **如果使用 Linkerd** → 检查 traffic split：`kubectl get trafficsplit -n <namespace>`

**分支决策**：
- **G1**：Envoy cluster 中无目标服务 → Round 3 — 分支 Z（检查 ServiceEntry/exportTo）
- **G2**：DestinationRule 配置了错误的 subset → Round 3 — 分支 AA（修正 DestinationRule）
- **G3**：OutlierDetection 导致全部 Endpoint 被剔除 → Round 3 — 分支 AB（调整 OutlierDetection）

---

### Round 2 — 分支 H：MetalLB/外部 LB 深度排查

**顾问指令**：
> 外部负载均衡器问题需要检查 LB 控制器和底层网络宣告。请执行：
> 1. 检查 Service LoadBalancer 状态：`kubectl get svc <service-name> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress[*].ip}'`
> **如果无法执行** → `kubectl describe svc <service-name> -n <namespace> | grep -A 5 "LoadBalancer Ingress"`
> **如果 describe 无信息** → `kubectl get svc <service-name> -n <namespace> -o yaml | grep -A 10 loadBalancer`
> 2. 检查 MetalLB speaker 日志（如果使用 MetalLB）：`kubectl logs -n metallb-system -l app=metallb -c speaker --tail=50`
> **如果无法执行** → `kubectl get pods -n metallb-system`，查看 speaker 状态
> **如果无 metallb-system** → 检查云控制器日志：`kubectl logs -n kube-system | grep -i "loadbalancer|cloud"`
> 3. 验证 LB IP 可达性：`ping <lb-ip>` 或 `arping -I <interface> <lb-ip>`（L2 模式）
> **如果无法从本地测试** → `kubectl run net-test --image=nicolaka/netshoot --rm -it --restart=Never --overrides='{"spec":{"hostNetwork":true}}' -- ping -c 3 <lb-ip>`

**分支决策**：
- **H1**：MetalLB speaker 未宣告 IP → Round 3 — 分支 AC（修复 MetalLB 配置）
- **H2**：云 LB 健康检查失败 → Round 3 — 分支 AD（修正健康检查配置）
- **H3**：LB IP 与现有设备冲突 → Round 3 — 分支 AE（解决 IP 冲突）

---

## Round 3 扩展分支

### Round 3 — 分支 T：修正 DNS 配置

**顾问指令**：
> Pod 的 DNS 解析配置异常，需要修正 resolv.conf 或 dnsConfig。
> 1. 查看当前 Pod DNS 配置：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsPolicy}'`
> **如果无法执行** → `kubectl describe pod <pod-name> -n <namespace> | grep -A 5 "DNS Policy"`
> 2. 查看 dnsConfig：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsConfig}'`
> **如果无 dnsConfig** → 检查集群默认 DNS 配置：`kubectl get configmap coredns -n kube-system -o yaml | grep -A 10 "kubernetes"`
> 3. 如需修改，在 [[实体/deployment.md|Deployment]] 中增加 dnsConfig：
> ```yaml
> dnsPolicy: ClusterFirst
> dnsConfig:
>   searches:
>     - <namespace>.svc.cluster.local
>     - svc.cluster.local
>     - cluster.local
> ```
> **如果无法修改 Deployment** → 准备修正后的 YAML 给管理员审批

**分支决策**：
- **T1**：DNS 配置修正后解析恢复 → 修复完成
- **T2**：配置正确但解析仍失败 → 升级至 SKILL-NET-001（DNS 问题诊断）
- **T3**：无权限修改 → 升级决策点

---

### Round 3 — 分支 U：调整 ServiceEntry/Egress 策略

**顾问指令**：
> Istio 网格中对外部服务或跨集群服务的访问被阻断，需要配置 ServiceEntry 和 Egress 规则。
> 1. 检查现有 ServiceEntry：`kubectl get serviceentry -n <namespace>`
> **如果无法执行** → `kubectl get serviceentry --all-namespaces`
> 2. 创建或修改 ServiceEntry 允许目标服务：
> ```yaml
> apiVersion: networking.istio.io/v1beta1
> kind: ServiceEntry
> metadata:
>   name: <target>-egress
>   namespace: <namespace>
> spec:
>   hosts:
>     - <target-service>
>   ports:
>     - number: <port>
>       name: <protocol>
>       protocol: <TCP/HTTP>
>   resolution: DNS
>   location: MESH_EXTERNAL
> ```
> **如果无法 apply** → 将 YAML 提交给管理员执行
> 3. 如需放行特定 CIDR，配置 Egress Gateway 或 Sidecar outboundTrafficPolicy

**分支决策**：
- **U1**：ServiceEntry 配置后访问恢复 → 修复完成
- **U2**：仍被阻断 → 检查 AuthorizationPolicy 和 Egress Gateway
- **U3**：无权限创建 ServiceEntry → 升级决策点

---

## 确认语气短语扩展

以下短语可用于在对话中确认工程师已理解指令或操作已完成：

### 操作确认
- "收到，请继续。"
- "确认已执行，结果如何？"
- "明白，请提供输出。"
- "好的，这一步完成了吗？"
- "了解了，请继续下一步。"
- "确认，执行结果请贴给我。"
- "已收到，是否有报错？"
- "清楚，请把输出贴一下。"
- "没问题，执行后告诉我结果。"
- "好的，操作成功了吗？"

### 结果确认
- "结果符合预期吗？"
- "输出正常吗？有没有错误信息？"
- "请确认 Endpoints 是否已填充。"
- "请验证访问是否已恢复。"
- "连通性测试通过了吗？"
- "请再次确认现象是否消失。"
- "修复后还有超时吗？"
- "请检查是否还有告警。"

### 升级确认
- "如果这一步无法执行，请告诉我。"
- "如果操作后没有改善，我们需要升级处理。"
- "如果权限不足无法修改，请立即联系管理员。"
- "如果结果仍然异常，我会准备升级材料。"
- "如果这里看不到预期输出，可能需要检查更高层组件。"

---

## "如果无法执行"替代方案扩展

以下扩展替代方案可补充到各 Round 分支中：

### 完全无 kubectl 权限的极端情况
**如果工程师没有任何 kubectl 权限**：
1. 请工程师截图集群管理控制台（如 Rancher、OpenShift Console、ACK 控制台）的 Service 页面
2. 请工程师联系有权限的同事，提供以下信息让其代为执行：
   - 命名空间和 Service 名称
   - 需要执行的命令
   - 将输出转发给你
3. 如果完全无法获取集群信息，请工程师提供：
   - 应用错误日志（应用自身的 stdout/stderr 或日志文件）
   - 监控系统的服务状态截图
   - 最近是否有部署变更的工单号或记录

### 无法创建临时 Pod 的替代方案
**所有使用 `kubectl run net-test` 的场景**：
- **替代 A**：使用已有的运行中的 Pod 执行 `kubectl exec`
- **替代 B**：使用 DaemonSet 中的节点调试 Pod（如 netshoot DaemonSet）
- **替代 C**：使用 `kubectl debug` 创建临时调试容器
- **替代 D**：如果集群禁用了临时 Pod 创建，请工程师在节点上直接安装 netcat/telnet/curl 工具进行测试

### 无法进入容器的替代方案
**所有使用 `kubectl exec` 的场景**：
- **替代 A**：检查是否有 debug sidecar 或运维容器共享 PID/network 命名空间
- **替代 B**：使用 `kubectl cp` 将脚本复制到容器后执行（如果容器有 shell）
- **替代 C**：查看容器启动命令和参数，从镜像 ENTRYPOINT 推断应用配置
- **替代 D**：从 CI/CD 系统或镜像仓库获取 Dockerfile，查看 EXPOSE 和端口配置

### 无法查看日志的替代方案
**所有使用 `kubectl logs` 的场景**：
- **替代 A**：查看节点上容器运行时的日志文件（如 `/var/log/containers/`）
- **替代 B**：通过日志聚合系统查询（如 ELK/Loki/EFK）
- **替代 C**：查看容器运行时的 `crictl logs` 输出
- **替代 D**：从监控系统查看容器 stdout/stderr 指标
- **替代 E**：如果应用有日志文件卷，通过 `kubectl cp` 拷贝日志文件

### 无法修改资源的替代方案
**所有使用 `kubectl patch/edit/apply` 的场景**：
- **替代 A**：请有权限的管理员执行，你提供精确的 YAML 和命令
- **替代 B**：通过 GitOps 工具（ArgoCD/Flux）修改仓库中的配置并同步
- **替代 C**：通过集群管理控制台的 UI 编辑功能修改
- **替代 D**：准备变更工单，按流程审批后执行

### 网络工具缺失的替代方案
**所有依赖 nc/curl/telnet/ss 的场景**：
- **替代 A**：使用 `/dev/tcp/<ip>/<port>` Bash 内置功能测试连通性
- **替代 B**：使用 Python：`python -c "import socket; s=socket.socket(); s.connect(('<ip>', <port>)); print('OK')"`
- **替代 C**：使用 wget：`wget -q -O- http://<ip>:<port>/health` 或 `wget --timeout=3 -qO- <url>`
- **替代 D**：使用 PHP：`php -r "if(fsockopen('<ip>',<port>)){echo 'OK';}"`
- **替代 E**：如果容器只有基本工具，尝试用 `cat < /dev/tcp/<ip>/<port>` 配合 echo 测试

---

*对话脚本扩展版本: 1.1.0 | 技能: Service 连通性问题诊断与修复 | 模式: L2-semi-auto*

## 相关案例

- [[概念/case-studies/2026-09-15-multicluster-network-partition.md|2026-09-15-multicluster-network-partition]]


<!-- risk-assessed -->
