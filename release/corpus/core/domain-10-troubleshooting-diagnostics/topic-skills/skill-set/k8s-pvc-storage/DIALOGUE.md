---
title: Node NotReady 远程顾问对话脚本
summary: 节点NotReady问题的远程顾问对话脚本，覆盖kubelet、CNI、资源耗尽排查。
category: dialogue
tags:
- dialogue
- remote-advisor
- node-notready
- skill
- k8s
- troubleshooting
tier: core
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
relationships:
- target: '[[skills/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[entities/cilium.md]]'
  type: uses
- target: '[[entities/kubernetes.md]]'
  type: uses
- target: '[[scripts/video-scripts/node-notready.md]]'
  type: uses
---


# [[scripts/video-scripts/node-notready.md|Node NotReady]] 远程顾问对话脚本

> 本脚本用于远程顾问指导现场工程师诊断和修复 [[entities/kubernetes.md|Kubernetes]] 节点 NotReady 问题。
> 顾问无法直接连接集群，所有诊断依赖工程师执行命令并反馈结果。

---

## 对话入口

### 场景 A：工程师直接描述问题

工程师："节点 NotReady 了"

顾问："收到。为了快速定位问题，请告诉我三个信息：
1. **影响范围**：涉及几个节点？是否影响业务？
2. **发生时间**：问题是突然出现还是逐渐恶化？
3. **最近变更**：最近 24 小时内是否有部署、扩缩容、配置变更或集群升级？"

### 场景 B：工程师提供部分信息

工程师："我收到 Prometheus 告警 KubeNodeNotReady"

顾问："告警已确认。在深入诊断之前，请确认三点：
1. 这个告警涉及**几个节点**？
2. 控制平面节点（master）是否正常？
3. 业务侧是否有异常表现，如服务不可用或延迟升高？"

### 场景 C：工程师从监控页面发现

工程师："控制台看到有几个节点变红了"

顾问："节点状态异常已确认。请切换到命令行执行以下操作，或者把控制台看到的节点状态截图发给我。如果控制台能显示具体错误信息（如 DiskPressure、MemoryPressure），也请一并告知。"

---

## Round 1：快速确认

顾问："现在开始第一轮诊断。请执行以下命令并粘贴完整输出：

```bash
kubectl get nodes | grep -E '(NotReady|Unknown)'
```

> **如果无法执行 kubectl**：请通过集群管理控制台（如 Rancher、OpenShift Console、ACK 控制台）查看节点状态，或者告诉我你当前能访问的界面和看到的信息。"

### 分支 1.1：单个节点 NotReady

工程师：（提供单个节点输出）

顾问："单个节点 NotReady，当前为 P2 优先级（2 小时内修复）。下一步请执行：

```bash
kubectl get nodes -o wide
kubectl describe node <node-name> | grep -A 20 'Conditions:'
```

> **如果无法执行 describe**：请通过控制台查看该节点的 Conditions 状态，或者执行 `kubectl get node <node-name> -o yaml` 并把 `status.conditions` 部分贴给我。"

### 分支 1.2：多个节点 NotReady

工程师：（提供多个节点输出）

顾问："多个节点 NotReady，这是一个高影响问题。请**立即**执行以下命令确认控制平面状态：

```bash
kubectl get nodes -l node-role.kubernetes.io/control-plane
# 如果没有 control-plane 标签，尝试：
kubectl get nodes | grep -E 'master|control-plane'
```

> **如果无法执行 kubectl**：请通过控制台确认 master/控制平面节点状态。如果控制平面也异常，或超过 50% 节点受影响，这个问题需要**立即升级**到高级支持团队。"

### 分支 1.3：工程师无法执行 kubectl

工程师："kubectl 连不上，超时了"

顾问："kubectl 无法连接，说明问题可能涉及控制平面或网络层面。请尝试以下替代方案：

**替代方案 A（集群控制台）**：通过 Rancher / ACK / OpenShift / 云厂商控制台查看节点列表，告诉我：
1. 总共有几个节点？几个显示异常？
2. 控制平面节点（master）状态如何？

**替代方案 B（SSH 到节点）**：如果你能通过 SSH 连接到任意一个节点，请执行：

```bash
# 在节点上检查 kubelet 状态
systemctl status kubelet
# 检查容器运行时
systemctl status containerd
```

**替代方案 C（云厂商 CLI）**：如果你有云厂商 CLI（如 aws cli、aliyun cli），请执行节点状态查询命令。

> 如果以上都无法执行，且业务已中断，请**立即升级**到高级支持。"

### 分支 1.4：阿里云ACK/专有云节点特定排查

工程师："节点 NotReady，我们在阿里云 ACK/专有云环境"

顾问："阿里云环境有额外的诊断维度，请按以下顺序排查：

**步骤 1：ECS实例状态检查**（远程顾问最可靠的排查手段）
请登录阿里云控制台或使用 aliyun CLI 检查 ECS 状态：

```bash
# 通过 aliyun CLI 查询异常节点对应的 ECS 实例状态
aliyun ecs DescribeInstances --RegionId <region-id> --InstanceIds '["<ecs-instance-id>"]'

# 批量查询节点池下所有 ECS 状态
aliyun ecs DescribeInstances --RegionId <region-id> --Tag.1.Key ack.aliyun.com --Tag.1.Value <cluster-id>
```

> **如果无法执行 aliyun CLI**：请登录阿里云控制台，进入 ECS 控制台，告诉我：
> 1. 异常节点对应的 ECS 实例状态是否为 **Running**？
> 2. 实例的 **系统事件** 是否有维护、重启、IO  hang 等事件？
> 3. 实例的 **监控图表**（CPU、内存、磁盘、网络）是否有异常突增？

**步骤 2：ACK节点池状态检查**

```bash
# 查看 ACK 节点池信息（如安装了 ack-node-problem-detector）
kubectl get nodes -o custom-columns=NAME:.metadata.name,POOL:.metadata.labels["alibabacloud.com/nodepool-id"],ZONE:.metadata.labels["topology.kubernetes.io/zone"]

# 查看节点问题事件
kubectl get events --field-selector reason=NodeProblemDetected --sort-by='.lastTimestamp' | tail -20
```

> **如果无法执行 kubectl**：请登录 ACK 控制台，进入集群的 **节点管理 > 节点池** 页面，告诉我：
> 1. 异常节点属于哪个节点池？该节点池状态是否正常？
> 2. 节点池的 **期望节点数** 与 **当前节点数** 是否一致？
> 3. 节点池是否有 **伸缩活动** 正在进行（可能导致节点状态波动）？

**步骤 3：阿里云特有网络与存储检查**

```bash
# 检查 Terway 网络组件状态（ACK使用Terway作为默认CNI）
kubectl get pods -n kube-system | grep -E 'terway|cilium'

# 检查云盘 CSI 插件状态
kubectl get pods -n kube-system | grep -E 'diskplugin|nasplugin|ossplugin'
```

> **如果无法执行 kubectl**：请通过 ACK 控制台 **组件管理** 页面确认以下组件状态：
> 1. **Terway** / **Flannel** 网络组件是否正常运行？
> 2. **CSI-Plugin** / **CSI-Provisioner** 存储组件是否正常运行？
> 3. **cloud-controller-manager** 是否正常（影响节点路由同步）？

**阿里云特有场景与替代方案**：

| 阿里云特有场景 | 诊断方法 | 替代方案 |
|:---|:---|:---|
| ECS 系统事件导致节点冻结 | 控制台查看系统事件 / `aliyun ecs DescribeInstanceHistoryEvents` | 等待系统事件完成或重启 ECS |
| 云盘 IO Hang 导致 DiskPressure | 控制台查看云盘监控 / 实例系统事件 | 通过控制台强制重启 ECS 实例 |
| Terway ENI IP 耗尽 | `kubectl get pod -n kube-system -l app=terway` 日志 | 扩容节点 ENI 辅助 IP 配额 |
| 节点池自动缩容误驱逐 | ACK 控制台查看节点池伸缩活动记录 | 暂停节点池弹性伸缩，手动锁定节点 |
| 安全组变更导致节点失联 | 控制台检查 ECS 安全组规则 | 恢复原始安全组或添加apiserver通信端口 |
| 专有云平台底座异常 | 专有云 ASO/天基控制台查看底座服务 | 联系阿里云 TAM / 驻场工程师处理 |

> **重要**：阿里云 ACK 托管版集群中，控制平面由阿里云托管。如果多个节点同时 NotReady 且 ECS 状态正常，可能是 **VPC 路由表** 或 **安全组** 被修改，请优先检查控制台的 **集群网络** 配置变更记录。

---

## Round 2：深度诊断

### 场景：已确认单个节点异常，进入深度诊断

顾问："现在进入深度诊断。请执行以下命令并告诉我结果：

```bash
kubectl describe node <node-name>
```

重点关注 Conditions 区域是否有 DiskPressure、MemoryPressure、PIDPressure、KubeletNotReady、NetworkUnavailable 等标记。"

#### 分支 2.1：资源压力类（DiskPressure / MemoryPressure / PIDPressure）

工程师："看到 DiskPressure=True"（或 MemoryPressure / PIDPressure）

顾问："资源压力导致的 NotReady。下一步请执行资源检查命令。

**如果你能通过 SSH 连接到该节点**，请执行：

```bash
df -h
free -h
nproc
```

> **如果无法 SSH**：请执行以下 kubectl 替代命令：

```bash
kubectl top node <node-name>
kubectl get --raw /api/v1/nodes/<node-name>/proxy/stats/summary | head -100
```

同时请告诉我：
1. 该节点上的 Pod 数量：`kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> | wc -l`
2. 是否有日志类 Pod 产生大量数据？"

#### 分支 2.2：Kubelet 或容器运行时异常

工程师："Conditions 里有 KubeletNotReady"（或 "PLEG is not healthy"）

顾问："kubelet 或容器运行时（PLEG）异常。请执行以下检查：

**如果你能通过 SSH 连接该节点**：

```bash
# 检查 kubelet 状态
systemctl status kubelet
journalctl -u kubelet -n 100 --no-pager

# 检查容器运行时
systemctl status containerd
crictl ps
```

> **如果无法 SSH**：请通过 kubectl 获取相关日志：

```bash
# 查看该节点上 kube-system 命名空间的 Pod
kubectl get pods -n kube-system --field-selector spec.nodeName=<node-name>

# 如果有 node-exporter 或日志收集 Pod，提取 kubelet 相关日志
kubectl logs -n <monitoring-namespace> <node-exporter-pod> 2>/dev/null | tail -50
```

另外请确认：该节点的 `/var/log` 或系统日志能否通过其他渠道获取？"

#### 分支 2.3：网络或证书问题

工程师："没有明显的资源压力标记，但节点状态是 Unknown"（或 "NetworkUnavailable=True"）

顾问："可能是网络分区或证书问题。请执行以下检查：

```bash
# 检查节点 Lease
kubectl get lease -n kube-node-lease <node-name> -o yaml

# 检查节点事件
kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp' | tail -20
```

> **如果无法执行上述命令**：请告诉我：
1. 该节点是自建机房还是云厂商节点？
2. 云厂商控制台是否显示该节点网络异常（如安全组变更、VPC 路由问题）？
3. 同一可用区 / 子网的其他节点是否正常？

**证书检查**（如果能 SSH 到节点）：

```bash
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

> **如果无法 SSH**：请确认最近是否进行过证书轮转、CA 更新或集群升级操作。"

#### 分支 2.4：Conditions 无明显异常标记

工程师："Conditions 看起来都正常，但节点状态是 NotReady"

顾问："这种情况需要检查 Node Lease 和 kubelet 心跳。请执行：

```bash
kubectl get lease -n kube-node-lease
kubectl get node <node-name> -o json | jq '.status.conditions'
```

> **如果无法执行 jq**：请使用 `kubectl get node <node-name> -o yaml` 并把 `status.conditions` 部分贴给我。

同时请检查：
1. 该节点的时钟是否与其他节点同步？（如果能 SSH，执行 `timedatectl status` 或 `ntpdate -q <ntp-server>`）
2. 最近是否有节点自动扩缩容操作（如 Cluster Autoscaler 日志）？"

---

## Round 3：根因确认与修复

### 场景：已完成深度诊断，进入修复阶段

#### 分支 3.1：磁盘空间不足（DiskPressure）

顾问："根因确认：磁盘压力导致节点 NotReady。

**修复步骤**（请按顺序执行）：

步骤 1：清理容器镜像和日志（SSH 到节点执行）：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

```bash
# 清理未使用的镜像
crictl rmi --prune
# 或使用 docker（如果运行时支持）
docker system prune -a -f  # ⚠️ 强制清理，可能杀运行中容器

# 清理日志
cd /var/log && find . -name '*.log' -size +100M -exec ls -lh {} \;
# 手动清理过大的日志文件（> 500MB 且非当前活跃日志）
```

> **如果无法 SSH**：请执行以下替代方案：
> 1. 通过 DaemonSet 或临时 Job 在节点上执行清理
> 2. 联系云厂商支持扩容节点磁盘
> 3. 如果节点上有日志收集 Pod，删除旧日志索引

步骤 2：验证修复：

```bash
kubectl get node <node-name>
# 等待 DiskPressure=False，节点状态变为 Ready
```

步骤 3：防止复发：
- 配置 logrotate 策略
- 设置镜像清理 CronJob
- 监控磁盘使用率告警阈值

> **注意**：如果清理后磁盘使用率仍 > 85%，建议安排节点替换或磁盘扩容。"

#### 分支 3.2：kubelet 或 containerd 异常

顾问："根因确认：kubelet 或容器运行时异常。

**修复步骤**（需要 SSH 到节点）：

步骤 1：重启 kubelet：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
systemctl restart kubelet
systemctl status kubelet
```

步骤 2：如果 kubelet 重启后仍异常，重启 containerd：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
systemctl restart containerd
systemctl status containerd
```

> **如果无法 SSH**：
> 1. 尝试通过云厂商控制台"重启节点"或"重置节点"
> 2. 如果集群支持节点替换，标记该节点不可调度并替换：`kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data`，然后创建新节点
> 3. 如果以上均不可行，请**升级**到高级支持团队，需要直接节点访问权限

步骤 3：验证修复：

```bash
kubectl get node <node-name> -w
# 观察节点状态在 1-2 分钟内恢复为 Ready
```

> **警告**：重启 containerd 会影响该节点上所有容器的运行，可能导致短暂的服务中断。请在业务低峰期执行，或确保 Pod 有副本分布到其他节点。"

#### 分支 3.3：证书过期

顾问："根因确认：kubelet 客户端证书过期。

**修复步骤**：

步骤 1：确认证书过期时间（SSH 到节点）：

```bash
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

步骤 2：手动触发证书轮转：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 方法 A：重启 kubelet 自动轮转
systemctl restart kubelet

# 方法 B：如果自动轮转失败，手动执行
kubeadm certs renew kubelet-client
# 或
kubeadm alpha certs renew kubelet-client
```

> **如果无法 SSH**：
> 1. 通过云厂商托管集群的控制台检查是否有证书自动修复功能
> 2. 如果集群使用外部 CA，联系 CA 管理员重新签发证书
> 3. 此修复通常需要节点访问权限，如无法获取请**升级**

步骤 3：验证：

```bash
kubectl get node <node-name>
# 检查证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

> **注意**：证书问题可能影响多个节点。请检查其他节点是否存在同样问题：`kubectl get nodes` 查看是否有更多节点陆续变为 NotReady。"

#### 分支 3.4：内存压力（MemoryPressure）

顾问："根因确认：内存压力导致节点 NotReady。

**修复步骤**：

步骤 1：识别内存消耗大户：

```bash
kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by=memory
```

> **如果无法使用 metrics-server**：请执行：
> ```bash
> kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> -o json | \
>   jq '.items[] | {name: .metadata.name, namespace: .metadata.namespace, mem: .spec.containers[].resources.requests.memory}'
> ```

步骤 2：决策：
- 如果某个 Pod 内存泄漏 → 重启该 Pod：`kubectl delete pod <pod-name> -n <namespace>`
- 如果整体内存不足 → 执行节点排空并替换：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
```

> **如果 drain 卡住**：添加 `--grace-period=30 --timeout=60s --force` 参数，或手动删除无法驱逐的 Pod。

步骤 3：节点恢复后重新调度：

```bash
kubectl uncordon <node-name>
```

> **注意**：内存压力修复可能涉及业务 Pod 的迁移，请确认相关应用有副本分布到其他健康节点。"

#### 分支 3.5：网络分区或 CNI 异常

顾问："根因确认：网络问题导致节点与 apiserver 失联。

**修复步骤**：

步骤 1：确认网络范围：

```bash
# 检查同一子网的其他节点
kubectl get nodes -o wide | grep <node-subnet>
```

步骤 2：检查 CNI Pod 状态：

```bash
kubectl get pods -n kube-system | grep -E 'cni|calico|flannel|cilium|weave'
```

步骤 3：根据 CNI 类型修复：

- **Calico**：检查 calico-node Pod 日志，确认 BGP 连接状态
- **Flannel**：检查 flannel DaemonSet，确认 VXLAN 接口正常
- **[[entities/cilium.md|Cilium]]**：检查 cilium-agent Pod，确认 eBPF 状态

> **如果无法执行 kubectl**：
> 1. 检查云厂商安全组/ACL 是否有变更
> 2. 检查 VPC 路由表是否丢失到该节点的路由
> 3. 网络问题通常需要基础设施团队介入，如无法快速确认请**升级**

步骤 4：验证：

```bash
kubectl get node <node-name>
# 同时检查节点上 Pod 的网络连通性
kubectl run netshoot --rm -it --image nicolaka/netshoot -- /bin/bash
# 在 netshoot 中执行 ping/curl 测试
```

---

## 升级路径

当满足以下条件之一时，顾问应明确建议**升级**到高级支持或值班经理：

### 🔴 立即升级（P0）

- **超过 50% 节点**变为 NotReady
- **所有控制平面节点**（master）NotReady
- `kubectl get nodes` 命令本身超时或无法执行，且无法通过任何替代方式获取节点状态
- NotReady 节点数量在 5 分钟内持续增加，呈扩散趋势
- 业务已全面中断，有外部用户投诉

### 🟠 建议升级（P1）

- 30%-50% 节点受影响
- 工程师**无法 SSH 到任何节点**，且云厂商控制台也无法操作
- 根因涉及**硬件问题**（磁盘损坏、内存问题、主板问题）
- 根因涉及**内核 panic / OOM killer 级联**，需要串口日志分析
- 证书过期且**手动轮转失败**，需要重新签发 CA 证书
- 网络问题涉及**底层基础设施**（交换机问题、VPC 路由黑洞、ISP 问题）

### 🟡 可能升级（P2→P1）

- 单个节点反复出现 NotReady，修复后短期内复发
- 工程师对执行修复命令存在顾虑（如担心影响业务）
- 修复步骤需要维护窗口，但当前处于业务高峰

### 升级话术

顾问："当前情况已超出本 [[skills/skill-k8s-node-notready-SKILL.md|Skill]] 的自主修复范围，建议立即升级。

**请执行以下操作**：
1. 通知值班经理 / 高级 SRE 团队
2. 在工单系统中标记优先级为 P0/P1
3. 保持当前收集的所有诊断信息，准备交接
4. 如果需要，我可以协助整理当前已确认的问题现象和已执行的诊断步骤

**当前已确认信息**：
- 影响节点数：X / Y
- 控制平面状态：正常 / 异常
- 已排查根因：[已排除/已确认]
- 已尝试修复：[已执行的操作]
- 当前状态：[节点状态/业务影响]

请把这些信息同步给接手的高级工程师。"

---

## 附录：常用命令速查

| 目的 | 命令 | 替代方案 |
|------|------|----------|
| 查看节点状态 | `kubectl get nodes` | 云厂商控制台 |
| 查看节点详情 | `kubectl describe node <name>` | `kubectl get node <name> -o yaml` |
| 查看资源使用 | `kubectl top node <name>` | SSH 执行 `free -h; df -h` |
| 检查 kubelet | SSH: `systemctl status kubelet` | 查看节点上 kubelet 相关 Pod 日志 |
| 检查 containerd | SSH: `systemctl status containerd` | 查看节点事件 |
| 检查证书 | SSH: `openssl x509 -in ... -noout -dates` | 检查最近证书变更记录 |
| 排空节点 | `kubectl drain <name> --ignore-daemonsets` | 云厂商控制台"移除节点" |
| 查看 Lease | `kubectl get lease -n kube-node-lease` | 直接查看节点心跳时间戳 |

---

> 本对话脚本基于 SKILL-SKILL-001（K8s Node NotReady 诊断与修复）设计。
> 完整根因目录参考 `reference/root-cause-catalog.md`
> 完整修复手册参考 `reference/remediation-playbook.md`

## 相关案例

- [[concepts/case-studies/2026-01-15-node-notready-pod-eviction.md|2026-01-15-node-notready-pod-eviction]]
## Related

- [[entities/kubelet.md|kubelet]]
- [[concepts/etcd-×-PVC.md|etcd-×-PVC]]
- [[concepts/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[concepts/etcd-×-StatefulSet.md|etcd-×-StatefulSet]]
- [[concepts/Deployment-×-PVC.md|Deployment-×-PVC]]
- [[concepts/StatefulSet-×-Service.md|StatefulSet-×-Service]]
- [[concepts/Deployment-×-Service.md|Deployment-×-Service]]
- [[concepts/apiserver-×-PVC.md|apiserver-×-PVC]]
- [[concepts/apiserver-×-PV.md|apiserver-×-PV]]
- [[concepts/apiserver-×-Service.md|apiserver-×-Service]]
- [[concepts/apiserver-×-Ingress.md|apiserver-×-Ingress]]
- [[concepts/StatefulSet-×-Ingress.md|StatefulSet-×-Ingress]]
- [[concepts/Deployment-×-Ingress.md|Deployment-×-Ingress]]
- [[concepts/etcd-×-Service.md|etcd-×-Service]]
- [[concepts/etcd-×-Ingress.md|etcd-×-Ingress]]
- [[concepts/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[concepts/apiserver-×-NetworkPolicy.md|apiserver-×-NetworkPolicy]]
- [[concepts/StatefulSet-×-NetworkPolicy.md|StatefulSet-×-NetworkPolicy]]
- [[concepts/etcd-×-RBAC.md|etcd-×-RBAC]]
- [[concepts/Deployment-×-RBAC.md|Deployment-×-RBAC]]
- [[concepts/Deployment-×-NetworkPolicy.md|Deployment-×-NetworkPolicy]]
