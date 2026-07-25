---
title: 容器运行时 生产就绪运维指南
description: 面向 Kubernetes 生产环境的容器运行时（containerd/CRI-O、镜像仓库、镜像构建）生产就绪检查、风险缓解、日常运维与排障手册
summary: 面向 Kubernetes 生产环境的容器运行时生产就绪检查、风险缓解、日常运维与排障手册
category: container-runtime
tags:
- production
- best-practices
- container-runtime
- operations
- containerd
- image-management
- registry
- supply-chain
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 容器运行时 生产就绪运维指南是什么
- 如何按生产环境要求运维 容器运行时
trigger_keywords:
- 生产就绪
- 运维指南
- containerd
- 镜像仓库
- 镜像拉取
- 容器运行时
prerequisites:
- kubectl-basics
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


# 容器运行时 生产就绪运维指南

本指南面向将 Kubernetes 容器运行时（containerd/CRI-O、镜像仓库、镜像构建链路）推入生产环境前的 SRE / 平台工程师，提供可执行的检查清单、风险缓解措施、日常运维命令、故障排查速查以及跨域协作边界。核心目标是让镜像拉取、运行时启动、节点磁盘和供应链安全处于可观测、可回滚、可持续运维的状态。本指南不替代厂商文档，而是聚焦生产化落地时必须补齐的运维闭环。

## 1. 生产环境检查清单

在宣布容器运行时域“生产就绪”前，建议逐项确认以下检查点。该 checklist 应在变更评审会上作为强制 gate，未通过的项必须给出豁免说明或整改计划：

- [ ] **运行时版本与 K8s 版本兼容**：集群所有节点使用统一且受支持的 containerd/CRI-O 版本，避免版本漂移。
  ```bash
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'
  ```
- [ ] **cgroup driver 一致**：containerd/CRI-O 与 Kubelet 均使用 `systemd` driver，避免 Pod 创建失败。
  ```bash
  # 检查 kubelet 配置中的 cgroup driver
  ps aux | grep kubelet | grep cgroup-driver
  # 检查 containerd 配置
  sudo containerd config dump | grep SystemdCgroup
  ```
- [ ] **sandbox 镜像可达**：`sandbox_image` 指向内网可达且已缓存的 pause 镜像（如 ACR 内网地址），防止 Pod 卡在 `SandboxCreate`。
- [ ] **镜像仓库高可用**：Harbor / ACR EE / ECR 等企业级仓库已完成多可用区或多地域部署，并配置 HTTPS/TLS 与有效证书。
- [ ] **Registry mirror 与缓存**：已在 `/etc/containerd/config.toml` 中配置 docker.io 等公网仓库的镜像加速 endpoint，降低拉取延迟与公网依赖。
- [ ] **节点镜像 GC 与磁盘保护**：已配置 `imageGCThreshold` 与 `evictionHard`，并预留充足 `/var/lib/containerd` 空间，避免 `DiskPressure`。
  ```bash
  kubectl describe node <node> | grep -A5 "Allocated resources"
  df -h /var/lib/containerd
  ```
- [ ] **镜像凭证与轮换**：`imagePullSecrets` 已绑定到 ServiceAccount；ACR 临时凭证、RRSA/IRSA 或 credential helper 已启用定期轮换。
- [ ] **运行时安全加固**：默认拒绝 `privileged: true`，启用 seccomp/AppArmor/SELinux 默认策略，对不可信负载使用 gVisor/Kata RuntimeClass。
- [ ] **供应链安全基线**：生产环境禁止拉取 `latest` 标签；镜像签名（cosign/notation）与漏洞扫描已接入准入控制（Kyverno/OPA/Gatekeeper）。
- [ ] **镜像预热与缓存机制**：核心应用镜像已通过 DaemonSet 预拉取、本地 Harbor 缓存或 ACR 镜像加速器在节点就绪前完成缓存。
- [ ] **可观测性接入**：已采集 containerd 指标（端口 1338）、Kubelet 镜像拉取指标与节点磁盘指标；日志保留不少于 7 天。
- [ ] **升级与回滚 SOP**：已制定运行时灰度升级方案、版本回滚包清单，并在测试池完成演练。
- [ ] **灾备与镜像复制**：镜像仓库元数据与 blob 已配置跨区域复制或定期备份，RTO/RPO 满足业务要求。

### 1.1 评审门与责任人

| 检查项 | 验证周期 | 责任人 | 证据产出 |
|--------|---------|--------|---------|
| 运行时版本一致性 | 每次扩缩容/升级后 | SRE | 节点 `containerRuntimeVersion` 汇总 |
| 镜像仓库可用性 | 每日 | 平台工程师 | 健康检查脚本与 TLS 证书有效期 |
| 节点磁盘水位 | 每 4 小时 | SRE | Prometheus 磁盘使用率面板 |
| 镜像漏洞扫描 | 每次镜像推送 | 安全工程师 | Harbor/ACR 扫描报告 |
| 凭证轮换 | 按策略（建议 ≤90 天） | SRE | Secret 更新时间戳 |

## 2. 关键风险与缓解措施

| 风险 | 影响 | 缓解措施与命令/配置 |
|------|------|-------------------|
| **镜像拉取雪崩** | 大规模扩容或滚动更新时并发拉取触发仓库限流，导致大量 `ImagePullBackOff` | 1. 配置 registry mirror 与本地 Harbor 缓存；<br>2. 使用 DaemonSet 预热核心镜像；<br>3. 限制 Kubelet `--registry-qps=5 --registry-burst=10`；<br>4. 对 ACR EE 开启 VPC 内网 endpoint：<br>`aliyun cr GetInstanceVpcEndpoint --RegionId <rid> --InstanceId <id>` |
| **节点磁盘占满** | 镜像层堆积导致 `DiskPressure`，触发 Pod 驱逐 | 1. 配置 Kubelet GC 阈值：<br>`--image-gc-low-threshold=80 --image-gc-high-threshold=85`；<br>2. 定期清理无用镜像：<br>`sudo crictl images -q \| xargs -r -n1 sudo crictl rmi`（需谨慎评估）；<br>3. 监控 `/var/lib/containerd` 使用率并设置告警 |
| **运行时版本不兼容/升级失败** | 升级后容器无法启动、CRI 调用异常，可能引发节点级故障 | 1. 维护版本兼容矩阵；<br>2. 先在小范围 canary 池升级；<br>3. 升级前执行 `kubectl drain <node> --ignore-daemonsets`；<br>4. 保留旧版本 RPM/DEB 包与配置备份，回滚命令：<br>`sudo yum downgrade containerd.io && sudo systemctl restart containerd` |
| **供应链攻击/漏洞镜像** | 引入带有 CVE 或后门的基础镜像，扩大攻击面 | 1. 生产禁用 `latest`，强制使用不可变 tag；<br>2. Harbor Trivy 或 ACR 镜像扫描；<br>3. 使用 cosign/notation 签名，Kyverno 验证 `spec.containers[*].image`；<br>4. 生成并保留 SBOM |
| **特权容器逃逸/运行时入侵** | 攻击者通过 privileged Pod 或运行时漏洞突破节点隔离 | 1. PSA / OPA 限制 privileged；<br>2. 默认 seccomp profile 与 AppArmor/SELinux；<br>3. 部署 Falco/Tetragon 检测异常系统调用；<br>4. 对多租户/不可信负载使用 gVisor/Kata RuntimeClass |

### 2.1 风险验证与止损要点

- **镜像拉取雪崩**：通过 Prometheus 监控 `containerd_image_pull_duration_seconds` 的 P99 与 `kubelet_image_pull_duration_seconds` 的异常突增来提前发现。一旦出现大量 `ImagePullBackOff`，优先扩容本地缓存、降低滚动更新并发度（`maxSurge`/`maxUnavailable`），而非直接删除 Pod 重试。
- **磁盘占满**：设置两级告警：磁盘使用率达到 75% 时 warning，达到 85% 时 critical 并触发自动清理脚本（仅清理 dangling 镜像）。保留至少 20% 的冗余空间用于突发扩容。
- **升级失败**：升级包发布前应在与生产相同 OS 内核版本的测试节点上验证。任何升级窗口必须保证能在 15 分钟内完成回滚，并保留节点 drain/uncordon 的脚本化记录。

### 2.2 关键监控指标

| 指标 | 告警阈值建议 | 说明 |
|------|------------|------|
| `containerd_image_pull_duration_seconds` P99 | > 30s | 镜像拉取延迟突增，可能预示仓库限流或网络异常 |
| `containerd_container_tasks_total` | 按节点基线波动 | 运行容器数异常，可能伴随泄露或调度异常 |
| `node_filesystem_avail_bytes{mountpoint="/var/lib/containerd"}` | < 20% | 节点磁盘空间不足，需触发清理或扩容 |

建议将上述指标接入 Prometheus Alertmanager，并配置 P1 级别告警，确保在业务受影响前介入。

## 3. 日常运维操作

### 3.1 批量确认运行时状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点运行时版本与 Ready 状态
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

# 登录节点查看 containerd 状态
sudo systemctl status containerd --no-pager
sudo containerd --version
```
### 3.2 清理节点镜像释放空间

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 仅清理没有运行容器引用的镜像（生产环境执行前请确认业务影响）
sudo crictl images -q | xargs -r -n1 sudo crictl rmi

# 查看镜像占用排序
sudo crictl images -o json | jq -r '.images[] | "\(.size) \(.repoTags[0])"' | sort -nr | head -20
```
### 3.3 验证镜像仓库连通性与认证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 从节点验证仓库 API 可达
curl -sk https://registry.cn-hangzhou.aliyuncs.com/v2/_catalog

# 使用 crictl 拉取测试镜像（验证凭证与网络）
sudo crictl pull registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9

# 查看节点上指定镜像是否存在
sudo crictl images | grep pause
```
### 3.4 镜像预热（DaemonSet 方式）

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-prepull
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: image-prepull
  template:
    metadata:
      labels:
        app: image-prepull
    spec:
      initContainers:
      - name: prepull
        image: registry.cn-hangzhou.aliyuncs.com/acs/crictl:latest
        command: ["/bin/sh", "-c"]
        args:
        - |
          crictl pull registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9
          crictl pull registry.cn-hangzhou.aliyuncs.com/myapp/backend:v2.3
        volumeMounts:
        - name: cri
          mountPath: /run/containerd
      containers:
      - name: pause
        image: registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9
      volumes:
      - name: cri
        hostPath:
          path: /run/containerd
```

### 3.5 刷新 imagePullSecrets

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为 default ServiceAccount 绑定新的仓库凭证
kubectl create secret docker-registry regcred \
  --docker-server=registry.cn-hangzhou.aliyuncs.com \
  --docker-username=<username> \
  --docker-password=<password> \
  -n <namespace> --dry-run=client -o yaml | kubectl apply -f -

kubectl patch serviceaccount default -n <namespace> -p '{"imagePullSecrets": [{"name": "regcred"}]}'
```
### 3.6 镜像仓库日常巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Harbor 各组件 Pod 状态
kubectl get pods -n harbor

# 查看 Harbor Helm Release 状态
helm status harbor -n harbor

# 查看 Harbor 复制任务与 GC 任务状态
kubectl logs -n harbor deploy/harbor-jobservice --tail=200

# 查看 ACR EE 实例配额与 VPC 访问状态
aliyun cr GET /instances

# 验证镜像签名（cosign 示例）
cosign verify --key cosign.pub registry.example.com/myapp/backend:v2.3
```
### 3.7 运行时灰度升级窗口

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
# 1. 选择 canary 节点并 drain
kubectl drain node-canary-01 --ignore-daemonsets --delete-emptydir-data

# 2. 升级运行时包并验证版本
sudo yum update -y containerd.io
sudo systemctl restart containerd
containerd --version

# 3. 检查该节点 Pod 恢复情况
kubectl uncordon node-canary-01
kubectl get pods --all-namespaces --field-selector spec.nodeName=node-canary-01

# 4. 观察 30 分钟后无异常，再批量升级其他节点
```
### 3.8 日志与审计保留

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 配置 journald 日志轮转
sudo tee /etc/systemd/journald.conf.d/containerd.conf <<EOF
[Journal]
SystemMaxUse=2G
SystemMaxFileSize=100M
MaxFileSec=7day
EOF
sudo systemctl restart systemd-journald
```
## 4. 故障排查速查

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|------|---------|---------|---------|
| `ImagePullBackOff` | 镜像不存在、标签错误、认证失败或网络不通 | `kubectl describe pod <pod> -n <ns>`；`sudo journalctl -u containerd -f`；`sudo crictl pull <image>` | 修正镜像名/tag；更新 `imagePullSecrets`；检查仓库连通性与 mirror 配置 |
| Pod 长时间 `ContainerCreating` | sandbox 镜像拉取失败、CNI 未就绪或 shim 异常 | `kubectl describe pod`；`sudo crictl pods`；`sudo ctr -n k8s.io tasks list` | 修正 `sandbox_image`；重启 containerd；检查 CNI 插件与网络 |
| 节点 `DiskPressure` | 镜像层堆积、日志过大或 emptyDir 未限制 | `kubectl describe node <node>`；`df -h /var/lib/containerd`；`sudo crictl images` | 清理无用镜像；调整 GC 阈值；限制 Pod ephemeral storage |
| `failed to create shim` | runc/shim 二进制损坏或 `/run/containerd` 权限异常 | `sudo journalctl -u containerd -n 200`；`ls -ld /run/containerd` | 重装 containerd；修复目录权限；必要时 drain 节点 |
| 容器反复 OOMKilled | 镜像内 JVM/进程内存未按 cgroup limit 调整 | `kubectl describe pod`；`dmesg \| grep -i oom` | 调整 resources.limits.memory；优化应用内存参数 |
| 镜像拉取延迟高 | 公网带宽瓶颈、仓库限流或镜像体积过大 | `containerd_image_pull_duration_seconds` 指标；`time sudo crictl pull <image>` | 启用 mirror/ACR 加速器；使用 nydus/stargz 延迟加载；拆分镜像层 |

## 5. 与其他域的协作边界

容器运行时域并非孤立存在，生产就绪需要与以下域紧密协作：

- **安全（安全合规）**：负责镜像签名、漏洞扫描、准入控制、seccomp/AppArmor/SELinux 策略以及供应链安全。运行时的安全加固必须与其策略保持一致，避免各自为政导致配置冲突。
- **平台工程（平台工程）**：负责镜像构建流水线、开发者门户、监控告警体系与节点基线镜像。运行时的高可用配置应纳入平台统一基线，避免每个集群自行拼凑。
- **发布变更（发布变更管理）**：负责镜像版本晋升、GitOps 流水线与变更窗口。生产镜像 tag 规范、不可变 artifact 与回滚策略需要与其协同，防止“latest”流入生产。
- **故障诊断（故障诊断）**：提供结构化的 `ImagePullBackOff`、`SandboxCreate` 等排障路径与 FTA。运行时团队应复用其统一模板，避免重复造轮子。
- **生产运维（生产运维）**：负责事件响应、FinOps 与容量管理。节点磁盘、镜像拉取成本与运行时升级窗口需要与其对齐，确保变更在业务低峰期执行。
- **存储（存储数据）**：当使用 overlayfs、nydus、stargz 等 snapshotter 或节点本地盘时，需要评估存储性能、IOPS 与快照策略，避免存储成为扩容瓶颈。

通过明确的域边界，运行时团队可以聚焦在“节点级运行时健康、镜像分发效率、运行时安全基线”三大核心职责上，而其他职责交由对应域负责。

## 6. 推荐阅读

### 本域相关文档

- [[14-容器运行时/03-containerd-CRI-O/01-containerd-deep-guide.md|containerd 深度指南]] — 理解 containerd 架构、CRI 与镜像命名空间。
- [[14-容器运行时/03-containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维指南]] — ACK/ASO 场景下的安装、迁移、配置优化与升级回滚。
- [[14-容器运行时/02-镜像管理/01-harbor-enterprise-image-registry.md|Harbor 企业级镜像仓库深度实践]] — 企业镜像仓库的高可用、安全扫描与灾备。
- [[14-容器运行时/04-镜像构建/01-buildkit-production-guide.md|BuildKit 生产化构建指南]] — 镜像构建加速、缓存与 rootless 构建。
- [[14-容器运行时/01-Docker/07-docker-security-best-practices.md|Docker 安全最佳实践]] — 容器运行时安全基线与限制策略。
- [[14-容器运行时/01-Docker/08-docker-troubleshooting-guide.md|Docker 故障排查指南]] — 容器、网络与存储的通用排错思路。

### 跨域参考

- [[08-安全/README.md|安全]] — 镜像安全、供应链与合规。
- [[10-平台工程/02-运维/06-monitoring-alerting-system.md|监控告警体系]] — 运行时指标接入与告警治理。
- [[19-故障诊断/README.md|故障诊断]] — 统一排障框架与技能库。

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 如何评估容器运行时生产就绪？ | 检查监控、告警、备份、升级、回滚五大维度 |
| containerd 升级最佳策略？ | 滚动升级，先测试环境验证，再生产分批 |
| 如何监控容器运行时健康？ | Prometheus + containerd metrics + 自定义告警 |
| 镜像安全如何保障？ | Trivy 扫描 + cosign 签名 + admission webhook |
| 多运行时如何管理？ | RuntimeClass + 专用节点池 + 统一监控 |
| 磁盘空间如何管理？ | 镜像 GC + 日志轮转 + 独立数据盘 |
| 如何制定客灾方案？ | 定期备份配置 + 镜像多副本 + 快速重建流程 |
| 生产环境常见故障？ | 磁盘满、shim 泄漏、镜像拉取失败、cgroup 错误 |

## 相关工具

| 工具 | 用途 | 使用方式 |
|------|------|----------|
| crictl | CRI 调试 | `crictl ps/pods/info` |
| ctr | containerd 原生 CLI | `ctr images ls` |
| nerdctl | Docker 兼容 CLI | `nerdctl run/build` |
| Trivy | 镜像扫描 | `trivy image <image>` |
| cosign | 镜像签名 | `cosign verify <image>` |
| Prometheus | 监控 | containerd metrics 接入 |

## 版本兼容性

| 组件 | 推荐版本 | 说明 |
|------|----------|------|
| containerd | 1.7.x / 2.0.x | 稳定版 |
| runc | 1.1.x / 1.2.x | 与 containerd 匹配 |
| K8s | 1.28+ | CRI v1 |
| 内核 | 5.15+ | 完整功能支持 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| containerd | `systemctl status containerd` | active |
| CRI | `crictl info` | 返回 JSON |
| 镜像 | `crictl pull <image>` | 成功 |
| 监控 | Prometheus 查询 | 指标正常 |
| 告警 | 触发测试 | 正常通知 |

---

*本指南聚焦容器运行时域的“生产就绪”状态，建议结合具体集群版本与云厂商实践进行裁 剪和演练。*


<!-- risk-assessed -->
