---
title: 节点与容器运行时运维指南
description: Kubernetes 节点与容器运行时运维指南，覆盖 containerd、镜像拉取、kubelet PLEG、Node Problem Detector、descheduler、OS 补丁周期与镜像 GC。
summary: 节点与容器运行时运维指南，覆盖 containerd、镜像拉取、kubelet PLEG、NPD、descheduler、OS 补丁与镜像 GC，适用于 Kubernetes 生产节点日常运维。
category: production-operations
tags:
- production
- best-practices
- playbook
- node
- container-runtime
- containerd
- kubelet
- pleg
- node-problem-detector
- descheduler
- image-gc
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- 节点与容器运行时运维指南是什么
- 如何运维 Kubernetes 节点与 containerd
- kubelet PLEG Node Problem Detector descheduler 镜像 GC 最佳实践
trigger_keywords:
- 节点运维
- containerd
- kubelet PLEG
- Node Problem Detector
- descheduler
- OS 补丁
- 镜像 GC
prerequisites:
- kubectl-basics
- container-runtime-basics
- linux-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 节点与容器运行时运维指南

> **适用范围**: Kubernetes 生产节点（containerd/CRI-O）与节点级组件的日常运维。  
> **目标读者**: SRE、运维工程师、平台工程师。  
> **最后更新**: 2026-07-01

节点与容器运行时是 Kubernetes 工作负载的实际承载层。控制平面再稳定，如果节点或运行时出现问题，应用也无法正常运行。本指南是 [[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/10-production-readiness-operations-guide|生产运维域生产就绪运维指南]] 的节点与运行时专项 runbook，覆盖 containerd、镜像拉取、kubelet PLEG、Node Problem Detector、descheduler、OS 补丁周期与镜像 GC，帮助团队建立标准化的节点运维能力。

---

## 1. 适用场景与范围

本指南适用于：

- 节点日常巡检、故障排查与生命周期管理。
- containerd/CRI-O 配置、升级、回滚与故障恢复。
- 镜像拉取异常、镜像 GC、磁盘压力治理。
- kubelet PLEG 健康、节点 NotReady、PLEG not healthy 事件处理。
- Node Problem Detector 部署与自定义规则。
- descheduler 策略配置与 Pod 重调度。
- OS 内核补丁、节点替换与维护窗口管理。

---

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必备 CLI
kubectl version --client
ctr --version              # containerd CLI
crictl --version
systemctl --version
journalctl --version
```
- 具备节点 SSH 或特权 Pod（如 debug node）访问权限。
- 已部署 Node Problem Detector 与 descheduler（建议）。
- 已建立节点维护窗口与替换流程。

---

## 3. 核心概念/架构

```
工作负载 Pod → containerd (CRI) → runc → Linux cgroup/namespace
                ↑
           kubelet (PLEG)
                ↑
      Node Problem Detector
                ↑
       事件 / 条件 / 告警
```

- **containerd**: CRI 实现，负责镜像管理、容器生命周期、存储与网络沙箱。
- **PLEG (Pod Lifecycle Event Generator)**: kubelet 监听容器运行时状态变化，生成 Pod 事件。
- **Node Problem Detector**: 检测节点硬件、内核、容器运行时异常，并上报 NodeCondition 或事件。
- **descheduler**: 根据策略驱逐 Pod，优化集群资源分布与节点利用率。

---

## 4. 标准操作流程

### 4.1 节点巡检

节点巡检应每日自动化执行，关键指标进入 Prometheus/Grafana。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 节点状态与污点
kubectl get nodes -o custom-columns='NAME:.metadata.name,STATUS:.status.conditions[-1].type,TAINTS:.spec.taints[*].key'

# 节点资源使用
kubectl top nodes

# 节点事件
kubectl get events --field-selector type!=Normal --sort-by=.lastTimestamp | tail -50

# 登录节点查看 containerd
sudo systemctl status containerd --no-pager
sudo containerd --version
```
### 4.2 containerd 运维

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看运行时版本（全节点）
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'

# 查看镜像列表与占用
sudo crictl images -o json | jq -r '.images[] | "\(.size) \(.repoTags[0])"' | sort -nr | head -20

# 清理未引用镜像（谨慎）
sudo crictl images -q | xargs -r -n1 sudo crictl rmi

# 查看 sandbox 与任务
sudo crictl pods
sudo ctr -n k8s.io tasks list
```
### 4.3 镜像拉取故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 事件
kubectl describe pod <pod> -n <ns>

# 节点侧拉取测试
sudo crictl pull registry.example.com/app:v1.0

# 检查 containerd 日志
sudo journalctl -u containerd -f

# 检查 imagePullSecrets
kubectl get sa default -n <ns> -o jsonpath='{.imagePullSecrets}'
```
### 4.4 kubelet PLEG 健康

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 PLEG 相关事件
kubectl get events --field-selector reason=PLEGUnhealthy --sort-by=.lastTimestamp

# 查看 kubelet 日志
sudo journalctl -u kubelet -n 200 | grep -i pleg

# 检查节点上 Pod 数量
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node> | wc -l
```
PLEG not healthy 常见根因：

- 节点上 Pod 数量过多（> 250）。
- containerd/runc 响应慢或死锁。
- 节点磁盘 I/O 饱和、CPU 节流。

### 4.5 Node Problem Detector

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 部署 NPD
kubectl apply -f https://raw.githubusercontent.com/kubernetes/node-problem-detector/master/deployment/node-problem-detector.yaml

# 查看节点条件
kubectl get node <node> -o jsonpath='{.status.conditions}' | jq .

# 自定义规则示例：检测 frequent unregister net device
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: npd-custom-rules
  namespace: kube-system
data:
  custom-plugin-monitor.json: |
    {
      "plugin": "journald",
      "pluginConfig": {
        "source": "kernel"
      },
      "conditions": [],
      "rules": [
        {
          "type": "temporary",
          "reason": "FrequentUnregisterNetDevice",
          "logPath": "/var/log/journal",
          "pattern": "unregister_netdevice: waiting for.*to become free"
        }
      ]
    }
EOF
```
### 4.6 descheduler 策略

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 descheduler
helm repo add descheduler https://kubernetes-sigs.github.io/descheduler/
helm upgrade --install descheduler descheduler/descheduler -n kube-system \
  --set schedule="*/30 * * * *"

# 示例策略：移除高利用率节点上的 Pod 以平衡资源
kubectl apply -f - <<EOF
apiVersion: "descheduler/v1alpha2"
kind: "DeschedulerPolicy"
profiles:
- name: BalanceResources
  pluginConfig:
  - name: HighNodeUtilization
    args:
      thresholds:
        cpu: 20
        memory: 20
        pods: 5
  plugins:
    balance:
      enabled:
      - HighNodeUtilization
EOF
```
### 4.7 OS 补丁与节点替换

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
# 1. 标记节点不可调度
kubectl cordon <node>

# 2. 驱逐 Pod（保留 DaemonSet）
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force

# 3. 执行补丁或替换实例
# （云厂商控制台、Ansible、Immutable Infrastructure）

# 4. 恢复节点
kubectl uncordon <node>
```
补丁策略：

- 优先使用不可变节点替换，而非原地升级。
- 制定月度补丁窗口，关键 CVE 按需紧急补丁。
- 维护节点镜像版本矩阵，确保所有节点版本一致。

### 4.8 镜像 GC 与磁盘保护

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Kubelet GC 阈值
ps aux | grep kubelet | grep -E 'image-gc-low-threshold|image-gc-high-threshold'

# 建议配置
# --image-gc-low-threshold=80
# --image-gc-high-threshold=85
# --eviction-hard=memory.available<500Mi,nodefs.available<10%

# 查看节点磁盘水位
kubectl describe node <node> | grep -A 5 "Allocated resources"
df -h /var/lib/containerd
```
---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|---|---|---|
| 节点运行时版本一致 | `kubectl get nodes -o jsonpath='{..containerRuntimeVersion}'` | 版本一致且受支持 |
| containerd 运行 | `sudo systemctl is-active containerd` | active |
| 磁盘水位 | `df -h /var/lib/containerd` | < 80% |
| NPD 运行 | `kubectl get pods -n kube-system -l app=node-problem-detector` | Running |
| descheduler 运行 | `kubectl get cronjob -n kube-system descheduler` | 按计划执行 |
| 镜像 GC 阈值 | `ps aux \| grep image-gc` | low=80, high=85 |
| 节点补丁版本 | `kubectl get nodes -o jsonpath='{..osImage}'` | 版本一致 |
| PLEG 事件 | `kubectl get events --field-selector reason=PLEGUnhealthy` | 无新事件 |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| 节点 NotReady | PLEG unhealthy、containerd 卡死、磁盘压力 | `kubectl describe node`；`journalctl -u kubelet` | 重启 containerd/kubelet、清理磁盘、替换节点 |
| Pod 长时间 ContainerCreating | sandbox 镜像拉取失败、CNI 未就绪 | `kubectl describe pod`；`sudo crictl pods` | 修正 sandbox_image、检查 CNI、重启 containerd |
| ImagePullBackOff | 仓库不可达、凭证过期、tag 错误 | `kubectl describe pod`；`sudo crictl pull` | 检查网络、更新 imagePullSecret、修正 tag |
| 节点 DiskPressure | 镜像层堆积、emptyDir 未限制、日志过大 | `kubectl describe node`；`df -h` | 清理镜像、限制 ephemeral storage、调整 GC 阈值 |
| descheduler 误驱逐 | 阈值过严、PDB 未配置 | `kubectl get pdb -A` | 放宽阈值、为关键服务配置 PDB |
| NPD 误报 | 规则正则过宽 | `kubectl logs -n kube-system ds/node-problem-detector` | 调优规则、降低 sensitivity |
| 容器运行时版本漂移 | 部分节点未升级 | `kubectl get nodes -o jsonpath='{..containerRuntimeVersion}'` | 统一节点镜像或滚动升级 containerd |

---

## 7. 风险与注意事项

- **节点级操作影响面大**: drain、cordon、containerd 重启都会影响该节点所有 Pod，必须在变更窗口执行。
- **镜像清理需谨慎**: `crictl rmi` 可能删除共享层，导致其他 Pod 重新拉取，低峰期执行。
- **PLEG 阈值有限**: 单个节点 Pod 数量过多会拖慢 PLEG，建议控制在 200-250 以下。
- **OS 补丁回滚**: 原地升级后若内核不兼容，需有回滚内核或替换实例的预案。
- **descheduler 与 PDB**: 未配置 PDB 的关键服务可能在重调度时中断，确保关键应用已配置 PDB。
- **节点镜像版本漂移**: 长期未补丁会导致安全漏洞，应通过 IaC 统一基线并定期替换。
- **debug 容器权限**: 使用节点级 debug Pod 时需严格控制 RBAC，避免权限滥用。

---

## 8. 相关 Runbook / 推荐阅读

### 同域核心文档

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/10-production-readiness-operations-guide|生产运维域生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/02-production-sre-daily-ops|生产环境日常巡检与值班手册]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/03-change-management-guide|变更管理指南]]

### 跨域参考

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/10-production-readiness-operations-guide|容器运行时生产就绪运维指南]]
- [[02-containerd-production-operations|containerd 生产运维指南]]
- [[domain-10-troubleshooting-diagnostics/核心排障/06-node-notready-diagnosis.md|节点 NotReady 诊断]]
- [[32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-02-workloads-applications/00-core-workloads/10-kubelet-configuration|Kubelet 配置]]

---

## 9. 节点与运行时进阶实践

### 9.1 节点生命周期管理

生产环境应优先采用不可变基础设施管理节点。通过预配置的节点镜像（AMI/镜像模板）创建新节点，替换旧节点，而不是在旧节点上原地升级。这种方式可以确保节点配置一致、可回滚，并减少补丁带来的不确定性。节点替换流程应自动化，结合 Cluster Autoscaler、Karpenter 或云厂商自动伸缩组实现滚动替换。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点创建时间与版本
kubectl get nodes -o custom-columns='NAME:.metadata.name,AGE:.metadata.creationTimestamp,OS:.status.nodeInfo.osImage,RUNTIME:.status.nodeInfo.containerRuntimeVersion'
```
### 9.2 容器运行时选型与升级

containerd 是目前 Kubernetes 生产环境的主流运行时，具有稳定性高、社区活跃、与 Kubernetes 集成紧密等优点。CRI-O 在 OpenShift 等场景中也有广泛应用。无论选择哪种运行时，都应建立版本基线，避免集群内运行时版本漂移。升级运行前应先在 staging 节点验证，并准备好回滚方案（如切换回旧版本二进制或替换节点）。

### 9.3 节点可观测性强化

除了 Kubernetes 原生指标，还应关注节点级指标：CPU/Memory/磁盘 I/O/网络吞吐量、containerd 进程状态、kubelet 日志中的错误与警告、内核日志中的硬件故障与驱动异常。可以通过 node-exporter、cAdvisor、NPD 与 systemd journal 采集这些数据，并在 Grafana 中建立节点健康仪表板。

```bash
# 实时查看节点资源
top -p $(pgrep -d',' containerd)
iostat -x 1
ss -s
```

### 9.4 磁盘与镜像治理

节点磁盘是常见的故障源。应建立镜像保留策略，避免过期镜像占用大量磁盘。可以通过 kubelet 的 image GC 参数自动清理，也可以在低峰期手动清理未引用镜像。同时，应限制 emptyDir 大小、配置日志轮转、监控 /var/log 与 /var/lib/containerd 的水位，防止磁盘压力导致节点 NotReady。

### 9.5 网络与 DNS 问题排查

节点级网络问题经常表现为 Pod 无法访问外部服务、DNS 解析失败或跨节点通信异常。排查时应检查 CNI 插件状态、iptables/ipvs 规则、内核路由表、DNS 配置（如 CoreDNS 缓存与转发）以及安全组/网络 ACL 规则。使用 `crictl` 进入容器网络命名空间进行抓包，可以快速定位网络层面的异常。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看容器网络命名空间
sudo crictl pods -q --name <pod-name> | xargs -I{} sudo crictl inspectp {} | jq '.info.runtimeSpec.linux.namespaces[] | select(.type=="network") | .path'

# 在容器网络命名空间中执行命令
sudo nsenter -n -t <container-pid> -- ping -c 3 8.8.8.8
```
### 9.6 kubelet 配置优化

kubelet 是节点上最核心的组件，其配置直接影响 Pod 调度、资源管理与故障恢复。应根据节点规格合理设置 `maxPods`、`evictionHard`、`systemReserved`、`kubeReserved` 等参数。对于大规格节点，适当增加 `kubeAPIQPS` 与 `kubeAPIBurst` 可以提升 kubelet 与 API Server 的通信效率。所有 kubelet 配置应通过配置文件或 cloud-init 统一管理，避免手动修改导致配置漂移。

---

## 10. 节点运维检查清单与事件响应

### 10.1 每日巡检检查清单

每日应检查：节点是否全部 Ready，版本是否一致；containerd/CRI-O 是否正常运行；节点磁盘使用率是否低于 80%；NPD 是否上报新的异常事件；descheduler 是否按计划执行；kubelet 日志中是否有 PLEG、Eviction 等异常；镜像 GC 是否正常工作。这些检查应自动化，并通过告警及时通知值班人员。

### 10.2 变更前检查清单

节点变更（如升级 containerd、OS 补丁、节点替换）前，需确认：变更已在 staging 验证；已标记节点不可调度并正确 drain；关键应用已配置 PDB；回滚方案已准备；变更窗口已通知；监控告警与 On-Call 已就位。批量变更时应分批进行，避免同时影响过多节点导致服务不可用。

### 10.3 事件响应流程

节点故障时，应遵循以下流程：确认影响范围（单个节点还是批量节点）；查看节点事件、kubelet 日志、系统日志与监控指标；判断根因（PLEG、containerd、磁盘、网络、内核等）；采取修复措施（重启服务、清理磁盘、替换节点）；验证修复结果；记录事件并复盘。对于批量节点故障，应立即升级并启动应急预案。

### 10.4 常见误区

- **误区一：节点 NotReady 直接重启**。应先查看日志确认根因，盲目重启可能掩盖问题或导致数据丢失。
- **误区二：随意清理镜像**。清理镜像可能影响其他 Pod 启动，应在低峰期并评估影响后执行。
- **误区三：忽略节点版本漂移**。长期不升级会导致安全漏洞与兼容性问题，应通过 IaC 统一基线。
- **误区四：descheduler 配置过激进**。未配置 PDB 的关键服务可能被误驱逐，应谨慎设置阈值。
- **误区五：NPD 规则过宽泛**。过于敏感的规则会导致大量误报，应逐步调优并验证。

### 10.5 与平台工程的协作

节点与运行时运维需要与平台工程、安全、网络团队紧密协作。平台工程团队负责节点镜像、IaC 与自动化工具；安全团队负责 [[32-发布/package/2026-07-02_18-40/corpus/core/domain-17-system-foundation/01-linux/01-k8s-node-os-image-hardening-baseline|OS 加固]]、漏洞扫描与合规审计；网络团队负责节点网络连通性与带宽。建议定期召开运维复盘会议，分享节点故障案例与改进措施，持续提升节点运维成熟度。

---

*本指南应根据节点规模、运行时版本与补丁策略定期更新。建议每次 containerd/OS 升级后补充新的验证命令与已知问题，并将节点健康指标纳入日常值班巡检。*


<!-- risk-assessed -->
