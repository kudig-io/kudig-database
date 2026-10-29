---
title: 容器镜像分发加速全解
description: 容器镜像分发加速全景指南 — 惰性拉取（eStargz/Nydus/SOCI）、P2P 分发（Dragonfly）、registry 层优化（后端缓存/跨区复制/预热）、containerd 配置实操与弹形单元冷启动 P99 优化案例
summary: 面向大规模 Kubernetes 集群的镜像分发加速深度实践 — 三种惰性拉取方案对比、Dragonfly P2P 架构与部署、registry 层优化、containerd snapshotter 配置示例、选型决策表与生产冷启动优化案例
category: container-image-management
tags:
- k8s
- containerd
- performance
- production
- best-practices
- image
- registry
- dragonfly
- nydus
- estargz
- soci
- harbor
tier: core
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 架构师
estimated_read_time: 18min
intent_queries:
- 如何加速 Kubernetes 大规模集群的容器镜像拉取与冷启动
- eStargz、Nydus、SOCI 惰性拉取方案如何选型，Dragonfly P2P 怎么部署
trigger_keywords:
- 镜像分发加速
- 惰性拉取
- lazy pulling
- eStargz
- stargz-snapshotter
- Nydus
- SOCI snapshotter
- Dragonfly
- P2P 分发
- 镜像预热
- 冷启动优化
- ImagePullBackOff
prerequisites:
- kubectl-basics
- containerd-basics
- registry-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# 容器镜像分发加速全解

> **适用场景**: 大规模集群（数百节点以上）、弹性和函数类工作负载（扩容即冷启动）、跨地域多集群、镜像体积大（GB 级 AI/数据类镜像）且拉取慢/带宽贵的团队
> **核心方法**: 惰性拉取（Lazy Pulling）、P2P 分发（Dragonfly）、registry 层优化（缓存/复制/预热）、镜像本身瘦身

---

## 📋 目录

- [一、概述与核心概念](#一概述与核心概念)
- [二、原理与架构](#二原理与架构)
- [三、生产配置实操](#三生产配置实操)
- [四、故障排查与诊断流程](#四故障排查与诊断流程)
- [五、最佳实践与反模式](#五最佳实践与反模式)
- [六、性能与监控](#六性能与监控)
- [七、版本兼容与升级注意](#七版本兼容与升级注意)
- [八、选型决策表](#八选型决策表)
- [九、参考链接与延伸阅读](#九参考链接与延伸阅读)

---

## 一、概述与核心概念

### 1.1 为什么镜像分发是瓶颈

一个 Pod 从调度到 Running，kubelet 需要经历：CRI 拉取镜像 → 解压（unzip/untar 到快照目录）→ 创建容器。对大镜像而言，这两步的耗时常常远超容器进程启动本身：

- **串行下载**：传统 OCI 镜像按层串行拉取，单连接 TCP 吞吐受 RTT/拥塞窗口限制，跨区拉 5GB 镜像可能要几分钟。
- **全量解压**：containerd 默认必须把整层解压完才能挂载 rootfs，容器进程要等最后一字节落盘。
- **重复成本**：同一镜像在扩容 100 台机器时被从 registry 拉取 100 次，出口带宽与 registry 负载线性放大——云厂商 registry 按流量计费时这就是真金白银。
- **冷启动雪崩**：弹性扩容、故障恢复、大促备战等场景下，镜像拉取时间直接进入"节点就绪→Pod Ready"的关键路径，P99 被 IO 尾延迟主导。

相关背景可参考 [[wiki/14-容器运行时/00-总览/01-production-readiness-operations-guide.md|容器运行时生产就绪运维指南]] 中的镜像拉取检查项。

### 1.2 加速的四条主线

| 主线 | 解决的问题 | 典型代表 |
|------|-----------|---------|
| **镜像瘦身** | 从源头减小传输体积 | 多阶段构建、distroless 最小基础镜像、层缓存复用，见 [[wiki/14-容器运行时/04-镜像构建/06-image-layer-optimization.md|镜像层优化]] |
| **惰性拉取（Lazy Pulling）** | 消除"全量下载+全量解压"的启动阻塞 | eStargz + stargz-snapshotter、Nydus + nydus-snapshotter、SOCI snapshotter |
| **P2P 分发** | 大规模并发拉取时降低 registry 带宽与单点压力 | Dragonfly、KRaken 类 P2P 系统 |
| **registry 层优化** | 就近供给、减少跨区流量 | 后端存储缓存（CDN/S3 前置缓存）、跨区复制（geo-replication）、Harbor P2P 预热 |

四条主线相互正交、可叠加：先瘦身，再决定是否上惰性拉取与 P2P，最后用 registry 复制/预热兜底跨区与冷启动。

### 1.3 核心概念速览

- **Lazy Pulling（惰性拉取）**：容器先启动，文件在首次访问时按需从远端拉取。实现载体是 containerd 的 **remote snapshotter** 机制（见 [[wiki/14-容器运行时/03-containerd-CRI-O/13-snapshotter-strategies.md|snapshotter 策略]]）。
- **eStargz**：gzip 兼容的扩展格式（*estargz*），在层内建立文件索引与 chunk 级 footer，允许 stargz-snapshotter 只拉取启动所需文件。
- **Nydus**：蚂蚁/阿里主导的镜像加速服务，采用 RAF（Random Access File）块级 chunk 索引，运行时守护进程 nydusd 通过 FUSE（或 virtiofs/EROFS）按需读取。
- **SOCI（Seekable OCI）**：AWS 主导，**不改变镜像格式**，为已有层生成额外的 zTOC（zone table of contents）索引 artifacts，soci snapshotter 借助索引实现远程随机访问。
- **Dragonfly**：CNCF 毕业项目，P2P 镜像/文件分发系统，核心组件 Manager、Scheduler、Peer（dfdaemon）。
- **预热（Prewarming）**：在新节点/新集群就绪前，主动把热点镜像拉到本地或 P2P 网络中。

---

## 二、原理与架构

### 2.1 传统拉取路径解剖

```text
kubelet → CRI → containerd
  └─ PullImage
      ├─ resolver: 向 registry 认证、解析 manifest → 层 digest 列表
      ├─ fetcher:  逐层下载 blob（串行/受并发配置限制）
      ├─ differ:   逐层解压应用快照（overlayfs lowerdir）
      └─ snapshotter: 组装挂载点 → 容器启动
```

关键结论：**解压在关键路径上**。惰性拉取类方案的本质，是把"下载整层"替换为"声明一个远程快照（remote snapshot）+ 按需读取"，即 containerd 的 `Prepare` 阶段直接返回一个可挂载的占位快照。

### 2.2 惰性拉取三种方案对比

| 维度 | eStargz + stargz-snapshotter | Nydus + nydus-snapshotter | SOCI snapshotter |
|------|------------------------------|----------------------------|------------------|
| 主导方 | Google/容器社区 | 蚂蚁集团、阿里云（CNCF Sandbox，Dragonfly 子项目） | AWS |
| 镜像格式 | 需转换为 eStargz（gzip 兼容，registry 无感知） | 需转换为 Nydus/RAF 格式（chunk 级索引） | **不改镜像**，旁挂 zTOC 索引 manifest（SOCI artifacts） |
| 索引粒度 | 文件级 + chunk footer（tar.gz 内 seek） | **块级 chunk**（RAF，去重、按需加载粒度最细） | zstd 层内 zone 级索引 |
| 运行时组件 | stargz-snapshotter（容器内可注入 `containerd-stargz-grpc`） | nydusd 守护进程（FUSE），或内核态 EROFS/virtiofs | soci snapshotter（每节点 daemon） |
| 兼容 registry | 任意支持 OCI 的 registry | 任意 registry；配合 Nydus 加速可对接 P2P | 任意 registry；原生体验 AWS ECR |
| 典型收益 | 启动时间降至原来的 1/3~1/10（取决于工作集） | 大镜像冷启动可到秒级；chunk 去重进一步省带宽 | 无需改造构建链路即可获得 30%+ 启动提升 |
| 代价 | 构建期需 `nerdctl convert`/`ctr-remote optimize`；运行中 FUSE 有开销 | 生态相对垂直；需要转换与双格式存储 | 索引生成（`soci create`）需要额外 CI 步骤；zstd 层才有效 |

选型直觉：

- **构建链路可控、追求极致冷启动**（弹性/函数/Serverless 容器）：Nydus。
- **想要"零改造镜像"快速收益、或已在 AWS 生态**：SOCI。
- **通用、稳定、社区中立，且接受镜像转换**：eStargz。
- 三者都依赖 containerd（CRI-O 对 lazy pulling 支持有限），见 [[wiki/14-容器运行时/03-containerd-CRI-O/01-containerd-deep-guide.md|containerd 深度指南]]。

### 2.3 P2P 分发：Dragonfly 架构

Dragonfly 把"每台节点都去 registry 拉一遍"变成"节点之间互传分片"：

```text
                ┌─────────────┐
                │  Registry    │  (Harbor / Docker Registry / ECR / ACR ...)
                └──────▲──────┘
                       │ 回源（仅少量 Peer 回源）
┌────────────┐   ┌────┴────────┐
│  Manager   ├───┤  Scheduler  │   Manager: 集群管控/配置/审计
└────────────┘   └────▲────────┘   Scheduler: 为每个下载任务调度分片回源与 Peer 互联
                      │ 任务调度
        ┌─────────────┼─────────────┐
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │ Peer A  │◄──┤ Peer B  │◄──┤ Peer C  │   Peer=dfdaemon，节点上的代理/代理
   └─────────┘   └─────────┘   └─────────┘   既服务本机 containerd，也向其他 Peer 上传分片
```

- **Manager**：多租户、配置下发、与 Harbor 等对接预热任务。
- **Scheduler**：按 piece（分片，默认 4MB 级别）调度，构建 P2P 拓扑（父子图），限流、防环、热度调度。
- **Peer（dfdaemon）**：以 `http_proxy` 或 mirror（registry mirror/containerd hosts）方式劫持拉取请求；本地有 chunk 级缓存（与 nydus chunk 语义互补）。
- 与 registry 配合：Dragonfly 不替换 registry，而是作为**前置代理**。Harbor 等企业仓库见 [[wiki/14-容器运行时/02-镜像管理/01-harbor-enterprise-image-registry.md|Harbor 企业级镜像仓库]]；纯分发场景也可对接 [[wiki/14-容器运行时/02-镜像管理/02-docker-registry-enterprise-distribution.md|Docker Registry 企业分发]] 或云厂商 registry（如 [[wiki/14-容器运行时/02-镜像管理/07-amazon-ecr-enterprise.md|Amazon ECR]]）。

### 2.4 registry 层优化

1. **后端存储缓存**：registry 前挂 CDN 或对象存储前置缓存，把 blob 读压力从存储后端卸载；自建 registry 时将 `storage.filesystem` 换成 s3 后端 + CDN，是性价比最高的一步。
2. **跨区复制（Geo-replication）**：Harbor 原生支持多 registry 实例共享 DB、blob 按需跨区复制到就近站点；效果是"第二次拉取在本地"。
3. **P2P 预热**：把热点镜像在扩容前推入 Dragonfly P2P 网络（Harbor → Dragonfly preheat job），扩容节点命中 P2P 缓存，不回源。
4. **节点侧本地缓存**：kube-fledged / kraken-cache 类 DaemonSet 拉取缓存、containerd 的本地 snapshot 复用（同镜像重复调度天然命中本地快照）。

---

## 三、生产配置实操

### 3.1 方案一：eStargz + stargz-snapshotter

**步骤 1：构建期转换镜像（CI 中执行）**

```bash
# 🟡 中风险：会向 registry 推送新 tag（可回滚，删除 tag 即可）
nerdctl -n k8s.io pull registry.example.com/web/app:1.4.2
nerdctl -n k8s.io image convert \
  --estargz --oci \
  registry.example.com/web/app:1.4.2 \
  registry.example.com/web/app:1.4.2-esgz
nerdctl -n k8s.io push registry.example.com/web/app:1.4.2-esgz
```

`-esgz` 后缀便于灰度回退：Pod spec 里镜像 tag 切回原 tag 即回到普通拉取路径。

**步骤 2：节点部署 stargz-snapshotter**

```bash
# 🟡 中风险：修改节点 containerd 配置并重启（建议先在灰度节点池验证）
SYSTEMD_UNIT_DIR=/etc/systemd/system/containerd-stargz-grpc.service \
  ENABLE_SYSTEMD=1 ./script/setup-host.sh install stargz-snapshotter
```

`/etc/containerd/config.toml` 关键片段：

```toml
version = 2

[proxy_plugins]
  [proxy_plugins.stargz]
    type = "snapshot"
    address = "/run/containerd-stargz-grpc/containerd-stargz-grpc.sock"

[plugins."io.containerd.grpc.v1.cri".container]
  default_runtime_name = "runc"

  [plugins."io.containerd.grpc.v1.cri".container.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    # 关键：让 runc 使用 stargz snapshotter
    snapshotter = "stargz"

[plugins."io.containerd.grpc.v1.cri".registry]
  config_path = "/etc/containerd/certs.d"
```

更完整的 containerd 配置方法论见 [[wiki/14-容器运行时/03-containerd-CRI-O/10-containerd-configuration-deep-guide.md|containerd 配置深度指南]]。

**步骤 3：验证惰性拉取生效**

```bash
# 🟢 低风险：只读检查远程快照是否生效（远程快照会显示 remote 块设备占位）
crictl pull registry.example.com/web/app:1.4.2-esgz
ctr -n k8s.io --address /run/containerd-stargz-grpc/containerd-stargz-grpc.sock \
  snapshot ls | head
```

### 3.2 方案二：Nydus + nydus-snapshotter

```bash
# 🟡 中风险：节点安装 nydus-snapshotter 并修改 containerd（灰度节点池先行）
# 1) 转换镜像（CI）：nydusify 会同时生成 OCI manifest 与 nydus blob
nydusify convert --source registry.example.com/web/app:1.4.2 \
                 --target registry.example.com/web/app:1.4.2-nydus

# 2) 节点：安装 nydus-snapshotter（含 nydusd）
/usr/local/bin/containerd-nydus-grpc --config /etc/nydus/config.toml &

# 3) containerd config.toml 增加 proxy_plugins.nydus，并将
#    CRI container runtimes 的 snapshotter 指向 "nydus"
```

Nydus 关键调优点（`/etc/nydus/config.toml`）：

```toml
[daemon]
# 模式选择：fusedeny（拉取失败快速失败）/ cache 工作集大小
mode = "direct"

[rpc]
# 一层一个 nydusd 或共享守护进程；高密度节点建议共享（share-dfs 模式）
[backend]
# 预取：容器启动后台继续把剩余 chunk 拉满，避免运行中长尾 IO
enable_prefetch = true
```

### 3.3 方案三：SOCI snapshotter

```bash
# 🟡 中风险：生成并推送 zTOC 索引到 registry（新增 artifacts，不影响原镜像）
# CI 中：镜像为 zstd 压缩时收益最大
soci create registry.example.com/web/app:1.4.2
soci push --ref registry.example.com/web/app:1.4.2

# 节点：部署 soci snapshotter daemon（soci-snapshotter-grpc），containerd
# proxy_plugins.soci + snapshotter = "soci"，与 3.1 步骤 2 同构
```

```bash
# 🟢 低风险：检查索引与镜像的关联（referrers 视图）
soci list
oras discover --artifact-type "application/vnd.aws.soci.index.v1+json" \
  registry.example.com/web/app:1.4.2
```

### 3.4 Dragonfly P2P 部署与预热

```bash
# 🟡 中风险：向集群安装 Dragonfly（helm release，可卸载回滚）
helm repo add dragonfly https://dragonflyoss.github.io/helm-charts
helm install dragonfly dragonfly/dragonfly \
  -n dragonfly-system --create-namespace \
  --set manager.replicas=2 \
  --set scheduler.replicas=3 \
  --set seedPeer.replicas=3 \
  --set dfdaemon.proxy.registryMirror.url=https://registry.example.com

# 节点侧：containerd 使用 mirror 指向 dfdaemon
# /etc/containerd/certs.d/registry.example.com/hosts.toml:
#   server = "https://registry.example.com"
#   [host."http://127.0.0.1:65001"]
#     override_path = true
#     capabilities = ["pull", "resolve"]
```

**Harbor 预热打通**（Harbor ≥ 2.x，P2P Adapter 指向 Dragonfly Manager）：

```bash
# 🟡 中风险：创建预热策略会在 P2P 网络中主动拉取镜像（产生回源流量）
# Harbor 控制台：项目 → P2P 预热 → 新建策略，触发器选事件（push 即预热）
# 或 API 手动触发：
curl -s -X POST -H "Authorization: Bearer $HARBOR_TOKEN" \
  -H "Content-Type: application/json" \
  "https://harbor.example.com/api/v2.0/p2p/preheat/policies/42/preheat" \
  -d '{"type":"image"}'
```

### 3.5 registry 后端缓存与跨区复制（Harbor 示例）

```yaml
# 🟡 中风险：变更 registry 后端影响所有拉取，先在灾备站点验证
# harbor.cfg / values.yaml 关键项（示意）
harbor:
  persistence:
    imageChartStorage:
      type: "s3"
      s3:
        region: "cn-north-1"
        bucket: "harbor-blobs"
        # 前置 CDN 时开启，registry 会发验证头
        # secretkey / accesskey 略
  # 跨区复制：目标实例间配置 replication rule（按 namespace/tag 过滤）
  replication:
    - name: "prod-to-dr"
      destination: "harbor-dr.example.com"
      trigger: "scheduled"
```

---

## 四、故障排查与诊断流程

### 4.1 拉取慢/失败通用排查树

```bash
# 🟢 低风险：定位是哪类问题
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[0].state}' | jq .
kubectl describe pod <pod> | grep -A5 -i "events:"
# 常见事件：ImagePullBackOff（认证/不存在/限流）、ErrImagePull（网络/超时）
```

分流决策：

1. **单节点慢 vs 全局慢**：全局慢 → registry/网络/P2P 调度问题；单节点慢 → 磁盘 IO/FUSE/缓存目录问题。
2. **认证失败**：检查 secret 类型（dockerconfigjson）、registry 证书、containerd certs.d 的 CA。
3. **限流（429）**：云厂商 registry 限流 → P2P/mirror 的直接动因。

### 4.2 stargz-snapshotter 排查

```bash
# 🟢 低风险：查看 snapshotter 日志中远程快照决策
journalctl -u containerd-stargz-grpc -n 200 --no-pager | grep -i "remote snapshot"
# "prepared remote snapshot" = 惰性生效；fallback 到正常拉取 = 索引缺失或校验失败
ctr -n k8s.io snapshot ls   # 查看快照是否 remote 占位
```

常见坑：镜像没转 eStargz 却配了 stargz snapshotter —— 行为正确（自动回退普通拉取）但有额外进程开销；`noestargz` 标签问题；PVC 密集写应用触发大量按需读导致长尾。

### 4.3 Nydus 排查

```bash
# 🟢 低风险：nydusd 运行状态与缓存命中率
nydusd check --sock /var/run/nydusd.sock
curl -s http://127.0.0.1:9[PORT]/metrics | grep -E "nydus_fuse.*(hit|miss)"
journalctl -u containerd-nydus-grpc -n 100 --no-pager
```

高频问题：FUSE 死锁类内核 bug（务必固定内核版本，见第七节）、chunk 未命中导致回源风暴（开 prefetch）、双格式存储的存储成本上升。

### 4.4 Dragonfly 排查

```bash
# 🟢 低风险：Peer 视角看一次下载的分片来源
dfctl task list
kubectl -n dragonfly-system logs deploy/dragonfly-scheduler --tail=100 | grep -i "schedule"
# 回源数（parent=registry）过高说明 P2P 命中差：检查 seedPeer 副本与限流配置
```

---

## 五、最佳实践与反模式

### 5.1 最佳实践

1. **先瘦身再加速**：任何分发方案都无法救一个 8GB 的混沌镜像；多阶段构建 + 基础镜像统一 + [[wiki/14-容器运行时/04-镜像构建/05-distroless-minimal-images.md|distroless 最小镜像]] 是第一步。
2. **基础镜像收敛**：集群内基础镜像种类越少，节点本地快照与 P2P 缓存命中率越高。
3. **灰度标签策略**：转换后的加速镜像用独立 tag（`-esgz`/`-nydus`），按节点池灰度切换，出问题秒级回退。
4. **预热前置到扩容流程**：把 Harbor 预热/P2P preheat 挂到 HPA 事件或弹性伸缩流水线里，扩容节点"零回源"。
5. **分场景分层**：长稳服务用普通拉取 + 本地缓存即可；弹性/函数/批处理单元才值得上惰性拉取。
6. **带宽预算**：给 dfdaemon/seedPeer 配置限速，防止大促回源把 registry 出口打满。

### 5.2 反模式

- ❌ **全量转换存量镜像**：一次性转换上万镜像收益低、存储翻倍；只转换冷启动关键路径镜像。
- ❌ **把惰性拉取当免费午餐**：FUSE/网络按需读会把部分成本转移到运行期 IO；对数据密集型（训练/大文件读写）负载可能更慢。
- ❌ **P2P 与惰性拉取无脑叠加**：两者都做 chunk 级缓存，叠加需明确分工（Nydus 负责启动、Dragonfly 负责回源带宽），否则排障复杂度爆炸。
- ❌ **忽略镜像签名与策略校验链路**：转换/双格式仓库可能破坏既有签名流水线（参考 [[wiki/14-容器运行时/02-镜像管理/04-harbor-enterprise-security-scanning.md|Harbor 安全扫描]] 的策略联动）。
- ❌ **无度量就上马**：没有拉取时长 P99 基线，无法证明收益也无法发现回退。

---

## 六、性能与监控

### 6.1 关键指标

| 指标 | 来源 | 说明 |
|------|------|------|
| `image_pull_duration_seconds`（P50/P99） | kubelet/kube-state-metrics 派生或自采 | 核心业务指标，按镜像/节点池分组 |
| Pod 启动时长（Scheduled→Ready P99） | kube-state-metrics | 冷启动优化最终收益 |
| stargz: `stargz_snapshotter_*`（remote prepare 计数） | stargz-snapshotter Prometheus 端点 | 惰性命中率 |
| nydus: `nydus_fuse_read_hit/miss`、chunk 回源 QPS | nydusd metrics | 按需读命中率 |
| dragonfly: `dragonfly_peer_download_task_duration`、回源字节数 | Peer metrics | P2P 效率与回源比 |
| registry: blob GET P99、429 率 | registry/nginx 指标 | 是否需要缓存/CDN |

```bash
# 🟢 低风险：PromQL 示例 — 按节点池看拉取 P99（自采 histogram 场景）
histogram_quantile(0.99, sum(rate(image_pull_duration_seconds_bucket[5m])) by (le, nodepool))
```

### 6.2 生产案例：弹形单元冷启动 P99 优化

**背景**：某平台函数/弹性容器单元，镜像 2.5GB（含运行时 + 依赖），大促扩容 500 节点，Pod Ready P99 约 210s，其中镜像拉取占 ~150s；registry 出口带宽被打满出现 429。

**措施组合**（分三期落地）：

1. **一期（1 周）**：基础镜像收敛 12→3；依赖层重排（变更频率高的层放上层）；registry 出口前置 CDN。收益：P99 210s → 130s。
2. **二期（2 周）**：部署 Dragonfly（3 Scheduler + 3 seedPeer），Harbor push 事件预热；扩容节点命中 P2P。收益：130s → 60s，回源流量下降 ~90%。
3. **三期（4 周）**：冷启动关键镜像转 Nydus（chunk 预取开启），容器 1s 内启动、后台补齐数据。收益：60s → 12s，429 消失。

**经验沉淀**：

- 每期都以 P99 与回源字节数验收，避免"感觉变快"；
- 惰性拉取只给弹性池，长稳池保持普通快照（FUSE 开销不值得）；
- 预热策略按 tag 规则（`*-runtime-*`）而非全仓，预热任务量可控。

---

## 七、版本兼容与升级注意

| 组件 | 兼容要点 |
|------|---------|
| containerd ≥ 1.7 / 2.x | remote snapshotter API 稳定；containerd 2.x 下注意配置迁移（`version=2` 等价 `version=3` 语义变化），升级前在灰度池验证 proxy_plugins |
| stargz-snapshotter | 跟随 containerd 小版本升级；老镜像（非 eStargz）自动回退，无需迁移 |
| nydus-snapshotter / nydusd | **内核强相关**：FUSE 模式注意内核 FUSE 子系统补丁；EROFS/virtiofs 模式依赖较新内核。升级顺序：先升 nydusd 再升 snapshotter |
| SOCI | 索引生成与镜像压缩算法绑定（zstd 最优）；registry 需支持 referrers 或 fallback API（见 OCI 1.1，参考 [[wiki/14-容器运行时/02-镜像管理/10-oci-image-spec-artifacts.md|OCI Image Spec 与 Artifact 生态]]） |
| Dragonfly 2.x | Manager/Scheduler/Peer 版本需匹配（滚动升级顺序：Manager → Scheduler → seedPeer → Peer）；v1→v2 不兼容，需按官方迁移文档 |
| Harbor | 预热/P2P Adapter 需 Harbor ≥ 2.2；跨区复制要求企业版（CE 可用第三方同步工具替代） |

Kubernetes 升级窗口与节点 containerd 配置变更（snapshotter 切换）**不要同批执行**，变更分离原则参考 [[wiki/01-集群基础/06-升级路径/05-kubernetes-v1.37-upgrade-guide.md|Kubernetes v1.37 升级指南]] 的分批节奏。

---

## 八、选型决策表

| 问题 | 推荐路径 |
|------|---------|
| 只有几十个节点、拉取偶发慢 | 先做镜像瘦身 + 基础镜像收敛，不上重型方案 |
| 扩容百节点级、registry 带宽/限流是瓶颈 | **Dragonfly P2P**（改动最小、收益直接） |
| Serverless/函数容器、要求秒级冷启动 | **Nydus**（块级 chunk + 预取），可叠加 Dragonfly 回源 |
| 镜像多、不能动构建链路、AWS/ECR 生态 | **SOCI snapshotter**（不改镜像） |
| 通用 Kubernetes、接受 CI 转换、求稳定社区方案 | **eStargz + stargz-snapshotter** |
| 跨地域多集群 | registry geo-replication + 各区域 P2P，预热联动 CI |
| 数据密集型负载（训练/大文件） | 慎用惰性拉取；优先 P2P + 本地缓存 + 独立数据卷 |

---

## 九、参考链接与延伸阅读

- eStargz 与 stargz-snapshotter：https://github.com/containerd/stargz-snapshotter
- Nydus 与 nydus-snapshotter（RAF 格式、chunk 索引）：https://github.com/dragonflyoss/nydus
- SOCI snapshotter（AWS，zTOC）：https://github.com/awslabs/soci-snapshotter
- Dragonfly（CNCF 毕业项目，Manager/Scheduler/Peer）：https://d7y.io/
- Harbor 跨区复制与 P2P 预热：https://goharbor.io/docs/
- containerd remote snapshotter 设计文档：https://github.com/containerd/containerd/blob/main/docs/PLUGINS.md
- 库内延伸：[[wiki/14-容器运行时/03-containerd-CRI-O/14-nerdctl-production-guide.md|nerdctl 生产指南]]（`nerdctl image convert`）、[[wiki/14-容器运行时/04-镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]
- 库内背景：[[wiki/14-容器运行时/05-运行时迁移/01-docker-to-containerd-migration.md|Docker 到 containerd 迁移]]（mirror 配置细节）

<!-- risk-assessed -->
