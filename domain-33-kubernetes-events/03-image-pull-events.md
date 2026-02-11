# 03 - 镜像拉取事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档系统性覆盖 Kubernetes 容器镜像拉取全流程的所有事件类型,详细解析 imagePullPolicy、镜像拉取失败排查、私有仓库认证和 Docker Hub 限流等生产环境高频问题。**

---

## 目录

- [一、镜像拉取事件总览](#一镜像拉取事件总览)
- [二、镜像拉取策略与事件流程](#二镜像拉取策略与事件流程)
- [三、Normal 事件详解](#三normal-事件详解)
- [四、Warning 事件详解](#四warning-事件详解)
- [五、私有仓库认证排查](#五私有仓库认证排查)
- [六、常见镜像拉取失败场景](#六常见镜像拉取失败场景)
- [七、Docker Hub 限流问题](#七docker-hub-限流问题)
- [八、镜像仓库镜像与代理配置](#八镜像仓库镜像与代理配置)
- [九、生产环境最佳实践](#九生产环境最佳实践)
- [十、相关文档交叉引用](#十相关文档交叉引用)

---

## 一、镜像拉取事件总览

### 1.1 镜像拉取事件汇总表

| Event Reason | 中文名称 | 类型 | 来源组件 | 关联资源 | 适用版本 | 生产频率 |
|:---|:---|:---|:---|:---|:---|:---|
| `Pulling` | 开始拉取镜像 | Normal | kubelet | Pod | v1.0+ | 高频 |
| `Pulled` | 镜像拉取成功 | Normal | kubelet | Pod | v1.0+ | 高频 |
| `AlreadyPresent` | 镜像已存在本地 | Normal | kubelet | Pod | v1.0+ | 高频 |
| `Failed` (ErrImagePull) | 镜像拉取失败 | Warning | kubelet | Pod | v1.0+ | 中频 |
| `BackOff` (ImagePullBackOff) | 退避重试拉取镜像 | Warning | kubelet | Pod | v1.0+ | 中频 |
| `ErrImageNeverPull` | 镜像策略禁止拉取 | Warning | kubelet | Pod | v1.0+ | 低频 |
| `InspectFailed` | 镜像检查失败 | Warning | kubelet | Pod | v1.0+ | 罕见 |

### 1.2 事件流转示意图

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Pod 创建 / 容器启动                            │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
                ┌───────────────────────────────┐
                │  检查 imagePullPolicy 配置   │
                └───────────────┬───────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
   ┌─────────┐           ┌──────────┐           ┌─────────┐
   │ Always  │           │IfNotPre  │           │  Never  │
   │         │           │  sent    │           │         │
   └────┬────┘           └────┬─────┘           └────┬────┘
        │                     │                       │
        │            ┌────────┴─────────┐            │
        │            │                  │            │
        │            ▼                  ▼            │
        │   ┌───────────────┐   ┌─────────────┐    │
        └──▶│ 检查本地镜像   │   │ 使用本地镜像 │◀───┤
            └───────┬───────┘   └──────┬──────┘    │
                    │                  │            │
            ┌───────┴──────┐           │            │
            │              │           │            │
            ▼              ▼           │            ▼
      ┌──────────┐   ┌─────────┐      │      ┌────────────────┐
      │ 镜像存在  │   │镜像不存在│      │      │ 镜像不存在本地  │
      └─────┬────┘   └────┬────┘      │      └────────┬───────┘
            │             │            │               │
            │(Always时)   │            │               │
            │             │            │               │
            ▼             ▼            ▼               ▼
      ┌──────────────────────────┐  ┌──────┐   ┌────────────────┐
      │  Event: Pulling          │  │Event:│   │Event: ErrImage │
      │  开始拉取镜像             │  │Already│   │   NeverPull    │
      └───────────┬──────────────┘  │Presen│   │ Pod: ImagePull │
                  │                 │  t   │   │   BackOff      │
                  │                 └──┬───┘   └────────────────┘
                  │                    │
          ┌───────┴────────┐           │
          │                │           │
          ▼                ▼           │
    ┌──────────┐      ┌────────┐      │
    │ 拉取成功  │      │ 拉取失败│      │
    └─────┬────┘      └────┬───┘      │
          │                │           │
          ▼                ▼           │
   ┌────────────┐    ┌───────────┐    │
   │Event:Pulled│    │Event:     │    │
   │   (Xs)     │    │ErrImage   │    │
   └──────┬─────┘    │Pull       │    │
          │          └─────┬─────┘    │
          │                │           │
          │                ▼           │
          │          ┌────────────┐   │
          │          │Event:      │   │
          │          │ImagePull   │   │
          │          │BackOff     │   │
          │          │(退避重试)   │   │
          │          └────────────┘   │
          │                            │
          └────────────┬───────────────┘
                       │
                       ▼
              ┌─────────────────┐
              │   启动容器       │
              └─────────────────┘
```

---

## 二、镜像拉取策略与事件流程

### 2.1 imagePullPolicy 配置详解

| 策略 | 英文全称 | 行为说明 | 适用场景 | 对应事件 |
|:---|:---|:---|:---|:---|
| `Always` | Always Pull | 每次启动都从仓库拉取镜像(即使本地已存在) | 生产环境推荐,确保镜像最新 | `Pulling` → `Pulled` / `ErrImagePull` |
| `IfNotPresent` | Pull If Not Present | 仅当本地不存在时拉取镜像 | 开发测试环境,节省流量 | `AlreadyPresent` / `Pulling` → `Pulled` |
| `Never` | Never Pull | 永不拉取,仅使用本地镜像 | 离线环境/预加载镜像场景 | `AlreadyPresent` / `ErrImageNeverPull` |

### 2.2 默认策略规则

```yaml
# Kubernetes 的 imagePullPolicy 默认逻辑:

# 1. 如果镜像标签是 ":latest" → 默认 Always
spec:
  containers:
    - name: app
      image: nginx:latest        # 默认 imagePullPolicy: Always

# 2. 如果镜像没有标签(等同于 :latest) → 默认 Always
spec:
  containers:
    - name: app
      image: nginx               # 等同于 nginx:latest,默认 Always

# 3. 如果镜像有明确版本标签 → 默认 IfNotPresent
spec:
  containers:
    - name: app
      image: nginx:1.25.3        # 默认 imagePullPolicy: IfNotPresent

# 4. 显式指定策略 → 使用指定策略
spec:
  containers:
    - name: app
      image: nginx:1.25.3
      imagePullPolicy: Always    # 明确指定
```

### 2.3 镜像拉取流程决策树

```
开始
 │
 ├─ imagePullPolicy = Never?
 │   ├─ 是 → 镜像存在本地?
 │   │   ├─ 是 → AlreadyPresent → 启动容器
 │   │   └─ 否 → ErrImageNeverPull → Pod 失败
 │   │
 │   └─ 否 → imagePullPolicy = Always?
 │       ├─ 是 → 直接拉取镜像 → Pulling
 │       │   ├─ 成功 → Pulled → 启动容器
 │       │   └─ 失败 → ErrImagePull → ImagePullBackOff
 │       │
 │       └─ 否 (IfNotPresent) → 镜像存在本地?
 │           ├─ 是 → AlreadyPresent → 启动容器
 │           └─ 否 → 拉取镜像 → Pulling
 │               ├─ 成功 → Pulled → 启动容器
 │               └─ 失败 → ErrImagePull → ImagePullBackOff
```

---

## 三、Normal 事件详解

### 3.1 Pulling - 开始拉取镜像

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

`Pulling` 事件表示 kubelet 已开始从容器镜像仓库拉取指定的容器镜像。此事件标志着镜像拉取流程的开始阶段,是 Pod 启动流程中的关键步骤之一。

在生产环境中,这是一个正常的操作事件,通常在以下场景触发:
- Pod 首次调度到某个节点时(该节点尚未缓存该镜像)
- 使用 `imagePullPolicy: Always` 策略的容器启动时
- 镜像标签为 `:latest` 时(默认 Always 策略)

此事件的出现意味着网络流量即将发生,镜像大小和网络带宽将直接影响拉取耗时。在大规模集群中,同时拉取大量镜像可能导致镜像仓库负载升高或网络带宽瓶颈。

#### 典型事件消息

```bash
$ kubectl describe pod nginx-deployment-7d5bc-xyz12

Events:
  Type     Reason     Age   From               Message
  ----     ------     ----  ----               -------
  Normal   Scheduled  30s   default-scheduler  Successfully assigned default/nginx-deployment-7d5bc-xyz12 to node-03
  Normal   Pulling    28s   kubelet            Pulling image "nginx:1.25.3"
```

```bash
# 使用 kubectl get events 查看
$ kubectl get events --field-selector reason=Pulling

LAST SEEN   TYPE     REASON    OBJECT                             MESSAGE
45s         Normal   Pulling   pod/nginx-deployment-7d5bc-xyz12   Pulling image "nginx:1.25.3"
12s         Normal   Pulling   pod/redis-cluster-0                Pulling image "redis:7.2-alpine"
```

#### 影响面说明

- **用户影响**: Pod 启动时间延长,取决于镜像大小和网络速度(通常几秒到几分钟)
- **服务影响**: 如果是新部署或滚动更新,会影响服务就绪时间和流量切换速度
- **集群影响**: 大规模拉取(如 DaemonSet 部署到所有节点)会产生显著网络流量和镜像仓库负载
- **关联事件链**: `Scheduled` → `Pulling` → `Pulled` → `Created` → `Started`

#### 排查建议

当 `Pulling` 事件持续时间过长(超过预期)时,可以进行以下排查:

1. **检查镜像大小和拉取进度**

```bash
# 查看节点上正在拉取的镜像
kubectl get events --field-selector reason=Pulling -A --sort-by='.lastTimestamp' | tail -10

# 登录到对应节点查看 containerd/docker 日志
# containerd 环境
sudo crictl images
sudo journalctl -u containerd -f | grep -i pull

# docker 环境
sudo docker images
sudo journalctl -u docker -f | grep -i pull
```

2. **检查镜像仓库连接性**

```bash
# 在节点上测试镜像仓库连通性
curl -I https://registry-1.docker.io/v2/

# 测试私有仓库
curl -I https://your-registry.example.com/v2/

# 检查 DNS 解析
nslookup registry-1.docker.io
dig registry-1.docker.io
```

3. **检查网络带宽和延迟**

```bash
# 测试到镜像仓库的网络速度
time curl -o /dev/null https://registry-1.docker.io/v2/

# 检查节点网络带宽使用情况
iftop -i eth0
nethogs
```

4. **检查 kubelet 配置和日志**

```bash
# 查看 kubelet 日志中的镜像拉取详情
sudo journalctl -u kubelet -f | grep -E "Pulling|image"

# 检查 kubelet 的镜像拉取并发配置
ps aux | grep kubelet | grep -E "serialize-image-pulls|registry-"
```

#### 解决建议

| 问题原因 | 解决方案 | 优先级 |
|:---|:---|:---|
| **镜像体积过大** | 优化 Dockerfile 分层,使用多阶段构建,减小镜像体积 | 高 |
| **网络带宽不足** | 使用本地镜像仓库或区域性镜像缓存(如 Harbor/Dragonfly) | 高 |
| **镜像仓库限流** | 配置镜像仓库镜像或使用企业版仓库服务 | 高 |
| **镜像仓库响应慢** | 更换为地理位置更近的镜像仓库或使用 CDN 加速 | 中 |
| **多个容器同时拉取** | 调整 kubelet 的 `--serialize-image-pulls=false` 允许并发拉取 | 中 |
| **标签使用 :latest** | 使用明确的版本标签(如 `v1.2.3`),避免每次都拉取 | 中 |

---

### 3.2 Pulled - 镜像拉取成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

`Pulled` 事件表示 kubelet 已成功从镜像仓库下载完整的容器镜像,并完成镜像层的解压和验证。镜像现在已缓存在节点本地,可以用于创建容器。

事件消息中通常会包含拉取耗时信息(如 "in 3.5s"),这对于性能分析非常有价值。拉取时间受多个因素影响:
- 镜像大小(压缩后大小决定传输时间,解压后大小影响存储)
- 网络带宽(节点到镜像仓库的有效吞吐量)
- 镜像层数(层越多,元数据处理越复杂)
- 镜像仓库性能(并发拉取能力和地理分布)

此事件的出现是 Pod 能够正常启动的前置条件,紧接着会触发容器创建和启动流程。

#### 典型事件消息

```bash
$ kubectl describe pod nginx-deployment-7d5bc-xyz12

Events:
  Type     Reason     Age   From               Message
  ----     ------     ----  ----               -------
  Normal   Scheduled  35s   default-scheduler  Successfully assigned default/nginx-deployment-7d5bc-xyz12 to node-03
  Normal   Pulling    33s   kubelet            Pulling image "nginx:1.25.3"
  Normal   Pulled     28s   kubelet            Successfully pulled image "nginx:1.25.3" in 5.2s (5.2s including waiting)
  Normal   Created    27s   kubelet            Created container nginx
  Normal   Started    26s   kubelet            Started container nginx
```

```bash
# 查看所有成功拉取的镜像事件
$ kubectl get events --field-selector reason=Pulled -A

LAST SEEN   TYPE     REASON   OBJECT                             MESSAGE
2m15s       Normal   Pulled   pod/nginx-deployment-7d5bc-xyz12   Successfully pulled image "nginx:1.25.3" in 5.2s
1m30s       Normal   Pulled   pod/redis-cluster-0                Successfully pulled image "redis:7.2-alpine" in 12.8s
45s         Normal   Pulled   pod/mysql-statefulset-0            Successfully pulled image "mysql:8.0.35" in 25.6s
```

#### 影响面说明

- **用户影响**: Pod 启动流程顺利进行,即将进入容器创建和启动阶段
- **服务影响**: 正向影响,表明服务部署/更新流程正常
- **集群影响**: 节点镜像缓存已更新,后续相同镜像的 Pod 可直接使用本地镜像(如果策略为 IfNotPresent)
- **关联事件链**: `Pulling` → `Pulled` → `Created` → `Started` → `Ready`(如有 readinessProbe)

#### 排查建议

虽然 `Pulled` 是成功事件,但在性能优化场景下需要关注拉取耗时:

1. **统计镜像拉取时间分布**

```bash
# 提取拉取时间并统计
kubectl get events -A --field-selector reason=Pulled -o json | \
  jq -r '.items[] | .message' | \
  grep -oP '(?<=in )\d+\.?\d*s' | \
  sort -n

# 查找拉取时间超过 30 秒的事件
kubectl get events -A --field-selector reason=Pulled -o json | \
  jq -r '.items[] | select(.message | test("in [3-9][0-9]+\\.[0-9]+s|in [0-9]{3,}\\.[0-9]+s")) | "\(.involvedObject.namespace)/\(.involvedObject.name): \(.message)"'
```

2. **分析慢拉取事件的镜像特征**

```bash
# 查看慢拉取事件的完整信息
kubectl get events -A --field-selector reason=Pulled --sort-by='.lastTimestamp' -o wide | tail -20

# 检查对应镜像的大小
crictl images | grep <image-name>
```

3. **对比不同节点的拉取速度**

```bash
# 按节点分组统计拉取事件
kubectl get events -A --field-selector reason=Pulled -o json | \
  jq -r '.items[] | "\(.source.host)\t\(.message)"' | \
  sort
```

#### 解决建议

| 观察到的现象 | 优化方案 | 优先级 |
|:---|:---|:---|
| **拉取时间超过 60 秒** | 使用区域镜像缓存或私有镜像仓库,缩短网络距离 | 高 |
| **大镜像拉取慢** | 优化镜像大小,使用 Alpine 基础镜像,清理不必要的文件 | 高 |
| **特定节点拉取慢** | 检查该节点网络配置,可能存在带宽限制或路由问题 | 中 |
| **首次拉取慢** | 考虑在 Pod 调度前预热镜像(DaemonSet 预拉取/ImagePuller) | 中 |
| **频繁拉取相同镜像** | 检查 imagePullPolicy 设置,考虑改为 IfNotPresent | 低 |

---

### 3.3 AlreadyPresent - 镜像已存在本地

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

`AlreadyPresent` 事件表示 kubelet 检测到所需的容器镜像已经缓存在节点本地存储中,无需从远程镜像仓库拉取。这是镜像管理中最理想的情况,可以显著加快 Pod 启动速度。

此事件通常在以下场景出现:
- **imagePullPolicy 为 IfNotPresent**: 镜像已存在,直接使用本地缓存
- **imagePullPolicy 为 Never**: 强制使用本地镜像
- **同一节点上运行过相同镜像的其他 Pod**: 镜像已被先前的 Pod 拉取

在生产环境中,高比例的 `AlreadyPresent` 事件意味着良好的镜像缓存命中率,这对于减少镜像仓库负载、降低网络流量和加快应用部署速度都非常有益。对于滚动更新场景,如果节点上已有旧版本镜像,新版本镜像仍需拉取,不会触发此事件。

#### 典型事件消息

```bash
$ kubectl describe pod nginx-deployment-7d5bc-abc89

Events:
  Type     Reason            Age   From               Message
  ----     ------            ----  ----               -------
  Normal   Scheduled         5s    default-scheduler  Successfully assigned default/nginx-deployment-7d5bc-abc89 to node-03
  Normal   AlreadyPresent    4s    kubelet            Container image "nginx:1.25.3" already present on machine
  Normal   Created           3s    kubelet            Created container nginx
  Normal   Started           2s    kubelet            Started container nginx
```

```bash
# 查看所有使用本地镜像的事件
$ kubectl get events --field-selector reason=AlreadyPresent -A

LAST SEEN   TYPE     REASON           OBJECT                             MESSAGE
15s         Normal   AlreadyPresent   pod/nginx-deployment-7d5bc-abc89   Container image "nginx:1.25.3" already present on machine
32s         Normal   AlreadyPresent   pod/redis-cluster-1                Container image "redis:7.2-alpine" already present on machine
```

#### 影响面说明

- **用户影响**: Pod 启动速度最快,无镜像拉取延迟(通常 < 1秒进入容器创建阶段)
- **服务影响**: 正向影响,滚动更新或扩容时响应速度快
- **集群影响**: 无额外网络流量,不增加镜像仓库负载,节省带宽成本
- **关联事件链**: `Scheduled` → `AlreadyPresent` → `Created` → `Started`(跳过 Pulling/Pulled 阶段)

#### 排查建议

`AlreadyPresent` 是正常且高效的事件,但在某些场景下需要注意:

1. **验证镜像版本是否符合预期**

```bash
# 检查节点上的镜像列表
kubectl debug node/node-03 -it --image=busybox -- sh
# 在调试容器中
crictl images | grep nginx

# 或直接在节点上执行
ssh node-03 "crictl images | grep nginx"

# 查看镜像详细信息(包括创建时间、digest)
crictl inspecti <image-id>
```

2. **确认 imagePullPolicy 配置**

```bash
# 检查 Pod 的 imagePullPolicy 设置
kubectl get pod nginx-deployment-7d5bc-abc89 -o jsonpath='{.spec.containers[*].imagePullPolicy}'

# 查看完整容器配置
kubectl get pod nginx-deployment-7d5bc-abc89 -o jsonpath='{.spec.containers[*]}' | jq .
```

3. **检查镜像缓存策略**

```bash
# 查看节点镜像磁盘使用情况
kubectl get nodes -o wide
kubectl describe node node-03 | grep -A 10 "Allocated resources"

# 检查 kubelet 镜像垃圾回收配置
ps aux | grep kubelet | grep -E "image-gc|image-minimum"
```

4. **验证是否需要强制更新镜像**

```bash
# 如果使用 :latest 标签但镜像未更新,可能需要强制拉取
# 查看 Pod 镜像标签
kubectl get pod nginx-deployment-7d5bc-abc89 -o jsonpath='{.spec.containers[*].image}'

# 如果标签是 :latest 但仍显示 AlreadyPresent,检查 imagePullPolicy
# 预期应该是 Always,但可能被错误设置为 IfNotPresent
```

#### 解决建议

| 场景 | 建议方案 | 优先级 |
|:---|:---|:---|
| **需要确保最新镜像** | 使用明确版本标签(如 `v1.2.3`)+ Always 策略,或使用 digest 引用 | 高 |
| **镜像标签是 :latest** | 确保 imagePullPolicy 为 Always,或改用语义化版本标签 | 高 |
| **节点镜像缓存过旧** | 配置 kubelet 的 `imageMaximumGCAge` 或手动清理旧镜像 | 中 |
| **加速首次部署** | 使用 DaemonSet 或 ImagePuller 预热常用镜像到所有节点 | 中 |
| **优化存储空间** | 配置镜像 GC 策略 `--image-gc-high-threshold` 和 `--image-gc-low-threshold` | 低 |

---

## 四、Warning 事件详解

### 4.1 Failed (ErrImagePull) - 镜像拉取失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

`Failed` 事件(reason 为 `ErrImagePull`)表示 kubelet 在尝试拉取容器镜像时遇到了错误,导致镜像拉取失败。这是容器启动失败的最常见原因之一,通常由配置错误、网络问题或镜像仓库访问权限问题引起。

此事件是镜像拉取失败的首次报告,kubelet 会立即开始退避重试机制(Backoff),后续会转变为 `ImagePullBackOff` 事件。失败的具体原因会在事件消息的 `message` 字段中详细说明,通常包含底层容器运行时(containerd/docker)返回的错误信息。

常见失败原因包括:
- **镜像名称或标签错误**: 镜像不存在或标签拼写错误
- **镜像仓库认证失败**: 私有仓库缺少 ImagePullSecret 或凭据过期
- **网络连接问题**: 无法访问镜像仓库或 DNS 解析失败
- **镜像仓库限流**: 超过 Docker Hub 等公共仓库的拉取速率限制
- **TLS 证书问题**: HTTPS 连接证书验证失败

#### 典型事件消息

```bash
$ kubectl describe pod nginx-deployment-7d5bc-xyz12

Events:
  Type     Reason          Age                From               Message
  ----     ------          ----               ----               -------
  Normal   Scheduled       2m15s              default-scheduler  Successfully assigned default/nginx-deployment-7d5bc-xyz12 to node-03
  Normal   Pulling         2m14s              kubelet            Pulling image "nginx:1.25.999"
  Warning  Failed          2m10s              kubelet            Failed to pull image "nginx:1.25.999": rpc error: code = NotFound desc = failed to pull and unpack image "docker.io/library/nginx:1.25.999": failed to resolve reference "docker.io/library/nginx:1.25.999": docker.io/library/nginx:1.25.999: not found
  Warning  Failed          2m10s              kubelet            Error: ErrImagePull
```

```bash
# 认证失败示例
$ kubectl describe pod private-app-xyz12

Events:
  Warning  Failed   10s   kubelet   Failed to pull image "myregistry.example.com/private/app:v1.0": rpc error: code = Unknown desc = failed to pull and unpack image "myregistry.example.com/private/app:v1.0": failed to resolve reference "myregistry.example.com/private/app:v1.0": pulling from host myregistry.example.com failed with status code [manifests v1.0]: 401 Unauthorized
```

```bash
# 查看所有镜像拉取失败事件
$ kubectl get events --field-selector reason=Failed,type=Warning -A | grep -i "image"

NAMESPACE   LAST SEEN   TYPE      REASON   OBJECT                      MESSAGE
default     2m10s       Warning   Failed   pod/nginx-deployment-...    Failed to pull image "nginx:1.25.999": rpc error...
default     1m30s       Warning   Failed   pod/redis-cluster-0         Failed to pull image "redis:wrong-tag": rpc error...
```

#### 影响面说明

- **用户影响**: Pod 无法启动,停留在 `ErrImagePull` 或 `ImagePullBackOff` 状态,服务不可用
- **服务影响**: 如果是新部署,服务无法上线;如果是滚动更新,可能触发回滚或部署卡住
- **集群影响**: 持续的失败重试会产生大量事件对象和 kubelet 日志,增加 etcd 和日志存储压力
- **关联事件链**: `Pulling` → `Failed` (ErrImagePull) → `BackOff` (ImagePullBackOff,循环重试)

#### 排查建议

1. **分析错误消息确定失败原因**

```bash
# 查看详细错误消息
kubectl describe pod <pod-name> | grep -A 5 "Failed"

# 提取错误类型
kubectl describe pod <pod-name> | grep -oP 'rpc error: code = \K\w+'
# 常见错误码:
#   NotFound - 镜像不存在
#   Unknown - 通用错误,需看详细描述
#   Unavailable - 镜像仓库不可达
```

2. **验证镜像名称和标签**

```bash
# 检查 Pod 配置中的镜像名称
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'

# 验证镜像在仓库中是否存在
# Docker Hub
curl -s "https://hub.docker.com/v2/repositories/library/nginx/tags?page_size=100" | jq -r '.results[].name' | grep 1.25

# 私有 Harbor 仓库
curl -u username:password "https://harbor.example.com/v2/project/repo/tags/list"
```

3. **测试镜像仓库连接性**

```bash
# 在对应节点上测试网络连接
kubectl debug node/<node-name> -it --image=nicolaka/netshoot

# 在调试容器中测试
curl -v https://registry-1.docker.io/v2/
curl -v https://your-private-registry.example.com/v2/

# 测试 DNS 解析
nslookup registry-1.docker.io
dig registry-1.docker.io
```

4. **检查私有仓库认证配置**

```bash
# 查看 Pod 所在命名空间的 ImagePullSecrets
kubectl get pod <pod-name> -o jsonpath='{.spec.imagePullSecrets[*].name}'

# 查看 Secret 内容
kubectl get secret <secret-name> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq .

# 检查 ServiceAccount 的 ImagePullSecrets
kubectl get serviceaccount default -o jsonpath='{.imagePullSecrets[*].name}'
```

5. **检查 kubelet 日志中的详细错误**

```bash
# 在对应节点上查看 kubelet 日志
sudo journalctl -u kubelet -n 200 | grep -i "pull"

# 查看容器运行时日志
# containerd
sudo journalctl -u containerd -n 200 | grep -E "pull|image"

# docker
sudo journalctl -u docker -n 200 | grep -E "pull|image"
```

#### 解决建议

| 错误原因 | 解决方案 | 优先级 |
|:---|:---|:---|
| **镜像名称或标签错误** | 核对镜像名称拼写,检查标签是否存在于仓库,更新 Deployment/Pod 配置 | 高 |
| **私有仓库认证失败** | 创建或更新 ImagePullSecret,确保凭据有效,检查 Secret 关联到 Pod/ServiceAccount | 高 |
| **镜像不存在** | 确认镜像已推送到仓库,检查镜像构建流程,验证镜像仓库路径 | 高 |
| **网络连接问题** | 检查节点到镜像仓库的网络连通性,配置代理或防火墙规则,检查 DNS | 高 |
| **Docker Hub 限流** | 使用认证拉取(提高限额)或切换到镜像仓库,或使用企业仓库 | 中 |
| **TLS 证书问题** | 将仓库证书添加到节点信任列表,或配置 kubelet/containerd 跳过验证(不推荐) | 中 |
| **镜像仓库不可用** | 检查仓库服务状态,配置高可用镜像仓库,或使用备用仓库 | 中 |

---

### 4.2 BackOff (ImagePullBackOff) - 退避重试拉取镜像

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

`BackOff` 事件(对应 Pod 状态 `ImagePullBackOff`)表示 kubelet 在首次镜像拉取失败后,正处于指数退避(Exponential Backoff)重试阶段。这是 Kubernetes 的自我保护机制,避免频繁的失败重试给镜像仓库和网络带来过大压力。

退避重试机制的工作原理:
- **首次失败**: 触发 `ErrImagePull` 事件,立即开始第一次重试
- **后续失败**: 每次失败后,等待时间按指数增长(10s → 20s → 40s → 80s → 160s → ...)
- **最大退避时间**: 上限为 5 分钟(300 秒),之后以 5 分钟间隔持续重试
- **成功后重置**: 如果某次重试成功,退避计时器重置

`ImagePullBackOff` 状态通常意味着问题持续存在未解决,Pod 会一直处于此状态直到:
- 问题被修复,镜像成功拉取
- Pod 被删除或更新
- 等待时间超过 Pod 的 `spec.activeDeadlineSeconds`(如果设置)

#### 典型事件消息

```bash
$ kubectl describe pod nginx-deployment-7d5bc-xyz12

Events:
  Type     Reason          Age                    From               Message
  ----     ------          ----                   ----               -------
  Normal   Scheduled       10m                    default-scheduler  Successfully assigned default/nginx-deployment-7d5bc-xyz12 to node-03
  Normal   Pulling         10m                    kubelet            Pulling image "nginx:wrong-tag"
  Warning  Failed          10m                    kubelet            Failed to pull image "nginx:wrong-tag": rpc error: code = NotFound desc = failed to pull and unpack image...
  Warning  Failed          10m                    kubelet            Error: ErrImagePull
  Normal   BackOff         9m30s (x4 over 10m)    kubelet            Back-off pulling image "nginx:wrong-tag"
  Warning  Failed          9m30s (x4 over 10m)    kubelet            Error: ImagePullBackOff
```

```bash
# 查看 Pod 状态
$ kubectl get pod nginx-deployment-7d5bc-xyz12

NAME                               READY   STATUS             RESTARTS   AGE
nginx-deployment-7d5bc-xyz12       0/1     ImagePullBackOff   0          10m
```

```bash
# 查看所有处于 ImagePullBackOff 的 Pod
$ kubectl get pods -A --field-selector status.phase=Pending -o json | \
  jq -r '.items[] | select(.status.containerStatuses[]?.state.waiting.reason == "ImagePullBackOff") | "\(.metadata.namespace)/\(.metadata.name)"'

default/nginx-deployment-7d5bc-xyz12
production/api-server-abc123
```

#### 影响面说明

- **用户影响**: Pod 长时间无法启动,服务持续不可用,用户请求失败
- **服务影响**: 如果是滚动更新,旧版本 Pod 可能已被删除,导致服务容量下降或完全不可用
- **集群影响**: 持续的重试产生大量事件聚合(`count` 字段不断增长),消耗 etcd 存储和 API Server 资源
- **关联事件链**: `Failed` (ErrImagePull) → `BackOff` → `Failed` (ImagePullBackOff) → `BackOff` (循环往复)

#### 排查建议

1. **检查 Pod 状态和重试计数**

```bash
# 查看 Pod 详细状态
kubectl get pod <pod-name> -o yaml | yq eval '.status.containerStatuses[].state.waiting' -

# 查看事件的重试次数
kubectl describe pod <pod-name> | grep -E "Back-off|Failed" | grep -oP '\(x\K\d+'

# 计算退避等待时间
# 如果 count = 10,说明已重试多次,当前可能在等待最大退避时间(5分钟)
```

2. **分析根本原因(同 ErrImagePull 排查)**

```bash
# 查看最初的失败消息
kubectl describe pod <pod-name> | grep -A 2 "ErrImagePull"

# 检查镜像配置
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'

# 验证镜像是否存在
skopeo inspect docker://nginx:1.25.3
```

3. **检查是否有临时网络问题**

```bash
# 如果怀疑是临时网络问题,可以查看重试时间线
kubectl get events --field-selector involvedObject.name=<pod-name> --sort-by='.lastTimestamp'

# 如果失败时间集中在某个时间段,可能是网络或仓库临时故障
```

4. **检查 Deployment/ReplicaSet 状态**

```bash
# 查看 Deployment 是否处于异常状态
kubectl get deployment <deployment-name>
kubectl describe deployment <deployment-name>

# 检查是否有多个 ReplicaSet(滚动更新卡住)
kubectl get replicaset -l app=<app-label>
```

#### 解决建议

| 场景 | 解决方案 | 优先级 |
|:---|:---|:---|
| **镜像配置错误** | 修正镜像名称/标签后,Deployment/Pod 会自动重新拉取 | 高 |
| **认证问题** | 添加/更新 ImagePullSecret 后,kubelet 会在下次重试时使用新凭据 | 高 |
| **临时网络故障** | 等待下次退避重试,如果网络恢复会自动成功 | 中 |
| **持续失败** | 删除 Pod 强制重建(Deployment 会自动创建新 Pod): `kubectl delete pod <pod-name>` | 中 |
| **回滚部署** | 如果是更新导致的问题,回滚到上一版本: `kubectl rollout undo deployment/<name>` | 中 |
| **加速重试** | 删除 Pod 会立即创建新 Pod,跳过当前退避等待时间 | 低 |

---

### 4.3 ErrImageNeverPull - 镜像策略禁止拉取

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

`ErrImageNeverPull` 事件表示容器的 `imagePullPolicy` 被设置为 `Never`,要求仅使用节点本地已存在的镜像,但 kubelet 在本地未找到该镜像。由于策略禁止从远程仓库拉取镜像,Pod 将无法启动。

此场景通常出现在:
- **离线或空气隔离(Air-Gapped)环境**: 节点无法访问外部镜像仓库,依赖预加载镜像
- **镜像预热策略**: 期望镜像通过其他方式(如 DaemonSet/脚本)预先加载到节点
- **配置错误**: 错误地将 imagePullPolicy 设置为 Never,但未确保镜像已在节点上
- **镜像 GC 误删**: 镜像曾存在但被 kubelet 的垃圾回收机制删除

与 `ErrImagePull` 不同,此错误不会触发退避重试机制,因为 kubelet 知道重试也无法拉取镜像。Pod 会立即进入 `ErrImageNeverPull` 状态并保持失败,直到问题解决(镜像被加载到节点或策略被修改)。

#### 典型事件消息

```bash
$ kubectl describe pod offline-app-xyz12

Events:
  Type     Reason              Age   From               Message
  ----     ------              ----  ----               -------
  Normal   Scheduled           30s   default-scheduler  Successfully assigned default/offline-app-xyz12 to node-03
  Warning  ErrImageNeverPull   28s   kubelet            Container image "myapp:v1.0" is not present with pull policy of Never
  Warning  Failed              28s   kubelet            Error: ErrImageNeverPull
```

```bash
# Pod 状态
$ kubectl get pod offline-app-xyz12

NAME                 READY   STATUS              RESTARTS   AGE
offline-app-xyz12    0/1     ErrImageNeverPull   0          45s
```

```bash
# 查看所有 ErrImageNeverPull 事件
$ kubectl get events --field-selector reason=ErrImageNeverPull -A

NAMESPACE   LAST SEEN   TYPE      REASON              OBJECT                   MESSAGE
default     2m15s       Warning   ErrImageNeverPull   pod/offline-app-xyz12    Container image "myapp:v1.0" is not present...
```

#### 影响面说明

- **用户影响**: Pod 完全无法启动,服务不可用,且不会自动恢复
- **服务影响**: 如果是关键服务,会导致服务完全下线,需要人工介入修复
- **集群影响**: 影响较小,不会产生大量重试事件,但会阻塞 Pod 调度队列
- **关联事件链**: `Scheduled` → `ErrImageNeverPull` → `Failed`(停止,不再重试)

#### 排查建议

1. **验证 imagePullPolicy 配置**

```bash
# 检查 Pod 的 imagePullPolicy
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].imagePullPolicy}'

# 查看完整容器配置
kubectl get pod <pod-name> -o yaml | yq eval '.spec.containers[] | {"name": .name, "image": .image, "imagePullPolicy": .imagePullPolicy}' -
```

2. **检查节点上的镜像列表**

```bash
# 方法1: 使用 kubectl debug
kubectl debug node/<node-name> -it --image=busybox
# 在 debug 容器中
crictl images | grep <image-name>

# 方法2: 直接 SSH 到节点
ssh <node-name>
sudo crictl images | grep <image-name>

# 方法3: 使用 kubectl get nodes 后查询
kubectl get nodes <node-name> -o json | jq '.status.images[] | select(.names[] | contains("<image-name>"))'
```

3. **检查镜像是否被 GC 删除**

```bash
# 查看 kubelet 日志中的镜像 GC 记录
sudo journalctl -u kubelet | grep -E "ImageGarbageCollect|image gc"

# 检查 kubelet 的镜像 GC 配置
ps aux | grep kubelet | grep -oP -- '--image-gc-[a-z-]+=\K[^ ]+'
# 关键参数:
#   --image-gc-high-threshold (默认 85%)
#   --image-gc-low-threshold (默认 80%)
```

4. **验证是否为离线环境配置错误**

```bash
# 检查节点网络连通性
kubectl debug node/<node-name> -it --image=nicolaka/netshoot
# 在 debug 容器中测试
curl -v https://registry-1.docker.io/v2/
# 如果无法连接,确认是否为预期的离线环境
```

#### 解决建议

| 问题原因 | 解决方案 | 优先级 |
|:---|:---|:---|
| **配置错误** | 修改 imagePullPolicy 为 `IfNotPresent` 或 `Always`,允许拉取镜像 | 高 |
| **镜像未预加载** | 使用 `crictl pull` 或 `docker pull` 手动加载镜像到节点 | 高 |
| **离线环境缺镜像** | 通过离线方式传输镜像文件(`crictl load`/`docker load`),或使用本地仓库 | 高 |
| **镜像被 GC 删除** | 调整 kubelet GC 阈值,或将关键镜像标记为不可删除(取决于运行时) | 中 |
| **节点镜像存储不足** | 清理无用镜像释放空间,或扩展节点存储容量 | 中 |
| **使用 DaemonSet 预热** | 创建 DaemonSet 在所有节点预先拉取镜像: `imagePullPolicy: Always` | 低 |

---

### 4.4 InspectFailed - 镜像检查失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 罕见 |

#### 事件含义

`InspectFailed` 事件表示 kubelet 在镜像拉取成功后,尝试检查(inspect)镜像元数据时发生错误。这通常意味着镜像数据已下载到节点,但在解析镜像配置、层信息或元数据时遇到了问题。

此事件非常罕见,可能的原因包括:
- **镜像数据损坏**: 传输过程中镜像文件损坏或校验和不匹配
- **容器运行时故障**: containerd/docker 的镜像存储或数据库出现问题
- **磁盘 I/O 错误**: 节点磁盘故障导致镜像层读取失败
- **镜像格式不兼容**: 镜像使用了当前运行时不支持的格式或特性
- **权限问题**: 容器运行时进程无法读取镜像存储目录

与 `ErrImagePull` 不同,`InspectFailed` 发生在镜像拉取之后,这意味着网络和仓库访问都是正常的,问题出在节点本地的镜像处理环节。

#### 典型事件消息

```bash
$ kubectl describe pod corrupted-image-xyz12

Events:
  Type     Reason         Age   From               Message
  ----     ------         ----  ----               -------
  Normal   Scheduled      2m    default-scheduler  Successfully assigned default/corrupted-image-xyz12 to node-03
  Normal   Pulling        2m    kubelet            Pulling image "myapp:v1.0"
  Normal   Pulled         1m    kubelet            Successfully pulled image "myapp:v1.0" in 45s
  Warning  InspectFailed  1m    kubelet            Failed to inspect image "myapp:v1.0": rpc error: code = Unknown desc = failed to resolve image "myapp:v1.0": unable to inspect image: reading manifest: unexpected EOF
  Warning  Failed         1m    kubelet            Error: InspectFailed
```

```bash
# Pod 状态
$ kubectl get pod corrupted-image-xyz12

NAME                      READY   STATUS          RESTARTS   AGE
corrupted-image-xyz12     0/1     InspectFailed   0          3m
```

```bash
# 查看所有 InspectFailed 事件
$ kubectl get events --field-selector reason=InspectFailed -A

NAMESPACE   LAST SEEN   TYPE      REASON          OBJECT                         MESSAGE
default     3m15s       Warning   InspectFailed   pod/corrupted-image-xyz12      Failed to inspect image "myapp:v1.0": rpc error...
```

#### 影响面说明

- **用户影响**: Pod 无法启动,即使镜像已拉取到节点
- **服务影响**: 服务部署失败,需要人工介入诊断和修复
- **集群影响**: 可能指示节点存储或容器运行时问题,影响该节点上其他 Pod 的镜像操作
- **关联事件链**: `Pulling` → `Pulled` → `InspectFailed` → `Failed`

#### 排查建议

1. **检查镜像完整性**

```bash
# 在节点上验证镜像
ssh <node-name>
sudo crictl inspecti <image-id>

# 尝试列出镜像
sudo crictl images | grep <image-name>

# 查看容器运行时日志
sudo journalctl -u containerd -n 100 | grep -i inspect
```

2. **尝试手动拉取和检查镜像**

```bash
# 删除可能损坏的镜像
sudo crictl rmi <image-id>

# 重新拉取镜像
sudo crictl pull <image-name>

# 验证镜像
sudo crictl inspecti <new-image-id>
```

3. **检查节点存储健康状态**

```bash
# 检查磁盘错误
dmesg | grep -i "I/O error"
journalctl -xe | grep -i "I/O error"

# 检查文件系统
df -h
sudo fsck -n /var/lib/containerd  # 根据实际路径调整

# 检查 inode 使用情况
df -i
```

4. **检查容器运行时状态**

```bash
# 检查 containerd 服务状态
systemctl status containerd

# 查看 containerd 数据库
ls -lh /var/lib/containerd/io.containerd.metadata.v1.bolt/

# 检查是否有进程卡死
ps aux | grep containerd
```

5. **检查镜像是否使用了不支持的特性**

```bash
# 从其他节点或环境检查镜像元数据
skopeo inspect docker://<image-name> | jq .

# 检查镜像的 manifest 和 config
crane manifest <image-name>
crane config <image-name> | jq .
```

#### 解决建议

| 问题原因 | 解决方案 | 优先级 |
|:---|:---|:---|
| **镜像数据损坏** | 删除镜像(`crictl rmi`)后重新拉取,或使用 digest 确保完整性 | 高 |
| **容器运行时故障** | 重启容器运行时服务(`systemctl restart containerd`),或重启节点 | 高 |
| **磁盘 I/O 错误** | 检查磁盘健康(SMART),修复文件系统,或更换故障磁盘 | 高 |
| **运行时数据库损坏** | 备份并清理 containerd 数据库,重启服务(可能影响其他容器) | 中 |
| **镜像格式不兼容** | 更新容器运行时版本,或重新构建镜像使用兼容格式 | 中 |
| **权限问题** | 检查 containerd 用户权限,修复镜像存储目录权限 | 低 |
| **节点异常** | 考虑将 Pod 调度到其他节点,将问题节点标记为 NoSchedule 排查 | 低 |

---

## 五、私有仓库认证排查

### 5.1 ImagePullSecret 配置详解

Kubernetes 使用 `imagePullSecrets` 机制为私有镜像仓库提供认证凭据。

#### 5.1.1 创建 Docker Registry Secret

```bash
# 方法1: 使用命令行创建(推荐)
kubectl create secret docker-registry my-registry-secret \
  --docker-server=myregistry.example.com \
  --docker-username=myuser \
  --docker-password=mypassword \
  --docker-email=myemail@example.com \
  -n default

# 方法2: 使用现有 Docker 配置文件
kubectl create secret generic my-registry-secret \
  --from-file=.dockerconfigjson=$HOME/.docker/config.json \
  --type=kubernetes.io/dockerconfigjson \
  -n default

# 方法3: 使用 YAML 清单(适合 GitOps)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Secret
metadata:
  name: my-registry-secret
  namespace: default
type: kubernetes.io/dockerconfigjson
data:
  .dockerconfigjson: $(cat ~/.docker/config.json | base64 -w 0)
EOF
```

#### 5.1.2 在 Pod 中使用 ImagePullSecret

```yaml
# 方法1: 在 Pod spec 中直接指定
apiVersion: v1
kind: Pod
metadata:
  name: private-app
spec:
  containers:
    - name: app
      image: myregistry.example.com/private/app:v1.0
  imagePullSecrets:
    - name: my-registry-secret

---
# 方法2: 关联到 ServiceAccount(推荐,避免每个 Pod 都配置)
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-serviceaccount
  namespace: default
imagePullSecrets:
  - name: my-registry-secret

---
# Deployment 使用该 ServiceAccount
apiVersion: apps/v1
kind: Deployment
metadata:
  name: private-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: private-app
  template:
    metadata:
      labels:
        app: private-app
    spec:
      serviceAccountName: my-serviceaccount  # 使用关联了 imagePullSecrets 的 SA
      containers:
        - name: app
          image: myregistry.example.com/private/app:v1.0
```

#### 5.1.3 为默认 ServiceAccount 添加 ImagePullSecret

```bash
# 方法1: 使用 kubectl patch
kubectl patch serviceaccount default -n default \
  -p '{"imagePullSecrets": [{"name": "my-registry-secret"}]}'

# 方法2: 使用 YAML 更新
kubectl edit serviceaccount default -n default
# 在 spec 下添加:
#   imagePullSecrets:
#   - name: my-registry-secret

# 验证配置
kubectl get serviceaccount default -n default -o yaml | grep -A 2 imagePullSecrets
```

### 5.2 常见认证错误排查

#### 5.2.1 401 Unauthorized - 认证失败

```bash
# 错误消息示例
Failed to pull image "myregistry.example.com/private/app:v1.0": rpc error: code = Unknown desc = failed to pull and unpack image "myregistry.example.com/private/app:v1.0": failed to resolve reference "myregistry.example.com/private/app:v1.0": pulling from host myregistry.example.com failed with status code [manifests v1.0]: 401 Unauthorized

# 排查步骤:
# 1. 验证 Secret 是否存在
kubectl get secret my-registry-secret -n default

# 2. 检查 Secret 内容是否正确
kubectl get secret my-registry-secret -n default -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq .

# 3. 验证凭据是否有效(在节点上测试)
echo 'mypassword' | sudo crictl login myregistry.example.com -u myuser --password-stdin

# 4. 检查 Pod 是否关联了 Secret
kubectl get pod <pod-name> -o jsonpath='{.spec.imagePullSecrets[*].name}'

# 5. 检查 ServiceAccount 的 imagePullSecrets
kubectl get serviceaccount default -o jsonpath='{.imagePullSecrets[*].name}'
```

#### 5.2.2 Secret 未关联到 Pod

```bash
# 现象: Pod 描述中无 imagePullSecrets 信息
kubectl get pod <pod-name> -o yaml | grep -A 5 imagePullSecrets
# 如果输出为空,说明未关联

# 解决方案:
# 方法1: 更新 Deployment 添加 imagePullSecrets
kubectl patch deployment <deployment-name> \
  -p '{"spec":{"template":{"spec":{"imagePullSecrets":[{"name":"my-registry-secret"}]}}}}'

# 方法2: 为 ServiceAccount 添加 imagePullSecrets(影响使用该 SA 的所有 Pod)
kubectl patch serviceaccount default -n <namespace> \
  -p '{"imagePullSecrets": [{"name": "my-registry-secret"}]}'

# 方法3: 编辑 Deployment YAML
kubectl edit deployment <deployment-name>
# 在 spec.template.spec 下添加:
#   imagePullSecrets:
#   - name: my-registry-secret
```

#### 5.2.3 Secret 格式错误

```bash
# 检查 Secret 数据格式
kubectl get secret my-registry-secret -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq .

# 正确的格式示例:
{
  "auths": {
    "myregistry.example.com": {
      "username": "myuser",
      "password": "mypassword",
      "email": "myemail@example.com",
      "auth": "bXl1c2VyOm15cGFzc3dvcmQ="
    }
  }
}

# auth 字段是 "username:password" 的 base64 编码
echo -n 'myuser:mypassword' | base64
# 输出: bXl1c2VyOm15cGFzc3dvcmQ=

# 如果格式错误,重新创建 Secret
kubectl delete secret my-registry-secret -n default
kubectl create secret docker-registry my-registry-secret \
  --docker-server=myregistry.example.com \
  --docker-username=myuser \
  --docker-password=mypassword \
  --docker-email=myemail@example.com \
  -n default
```

#### 5.2.4 多仓库认证配置

```bash
# 场景: Pod 需要从多个私有仓库拉取镜像

# 方法1: 创建包含多个仓库的 Secret
cat <<EOF > docker-config.json
{
  "auths": {
    "registry1.example.com": {
      "username": "user1",
      "password": "pass1",
      "auth": "$(echo -n 'user1:pass1' | base64)"
    },
    "registry2.example.com": {
      "username": "user2",
      "password": "pass2",
      "auth": "$(echo -n 'user2:pass2' | base64)"
    }
  }
}
EOF

kubectl create secret generic multi-registry-secret \
  --from-file=.dockerconfigjson=docker-config.json \
  --type=kubernetes.io/dockerconfigjson \
  -n default

# 方法2: 使用多个 Secret
imagePullSecrets:
  - name: registry1-secret
  - name: registry2-secret
```

### 5.3 Harbor 私有仓库集成

```bash
# Harbor 仓库认证配置示例

# 1. 创建 Harbor Robot Account Secret
kubectl create secret docker-registry harbor-secret \
  --docker-server=harbor.example.com \
  --docker-username='robot$myproject+deployer' \
  --docker-password='<robot-account-token>' \
  -n production

# 2. 使用项目级别凭据
kubectl create secret docker-registry harbor-secret \
  --docker-server=harbor.example.com \
  --docker-username=admin \
  --docker-password=Harbor12345 \
  -n production

# 3. Harbor HTTPS 证书问题
# 如果 Harbor 使用自签名证书,需要将证书添加到节点信任列表

# containerd 配置 (/etc/containerd/config.toml)
[plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.example.com".tls]
  ca_file = "/etc/containerd/certs.d/harbor.example.com/ca.crt"
  insecure_skip_verify = false

# docker 配置 (/etc/docker/daemon.json)
{
  "insecure-registries": ["harbor.example.com"]
}

# 或将证书添加到系统信任存储
sudo cp harbor-ca.crt /usr/local/share/ca-certificates/
sudo update-ca-certificates
sudo systemctl restart containerd
```

### 5.4 云厂商容器镜像仓库集成

```bash
# ========== AWS ECR ==========
# 创建 ECR 凭据(需要定期刷新,因为 token 有效期 12 小时)

# 方法1: 使用 aws CLI 创建
TOKEN=$(aws ecr get-login-password --region us-west-2)
kubectl create secret docker-registry ecr-secret \
  --docker-server=123456789012.dkr.ecr.us-west-2.amazonaws.com \
  --docker-username=AWS \
  --docker-password="${TOKEN}" \
  -n default

# 方法2: 使用 IAM Role for Service Account (IRSA,推荐)
# 参考 AWS EKS 文档配置 IRSA,无需 imagePullSecret

# ========== Azure ACR ==========
# 创建 ACR Service Principal
az ad sp create-for-rbac --name acr-sp --role acrpull --scopes /subscriptions/<subscription-id>/resourceGroups/<rg>/providers/Microsoft.ContainerRegistry/registries/<acr-name>

kubectl create secret docker-registry acr-secret \
  --docker-server=myregistry.azurecr.io \
  --docker-username=<service-principal-id> \
  --docker-password=<service-principal-password> \
  -n default

# 或使用 AKS 的 Managed Identity(推荐)

# ========== Google GCR ==========
# 创建 Service Account Key
gcloud iam service-accounts keys create gcr-key.json --iam-account=<sa-email>

kubectl create secret docker-registry gcr-secret \
  --docker-server=gcr.io \
  --docker-username=_json_key \
  --docker-password="$(cat gcr-key.json)" \
  -n default

# ========== 阿里云 ACR ==========
kubectl create secret docker-registry acr-secret \
  --docker-server=registry.cn-hangzhou.aliyuncs.com \
  --docker-username=<aliyun-account> \
  --docker-password=<aliyun-password> \
  -n default
```

---

## 六、常见镜像拉取失败场景

### 6.1 镜像名称或标签错误

```bash
# 现象: NotFound 错误
Failed to pull image "nginx:1.25.999": rpc error: code = NotFound desc = failed to pull and unpack image "docker.io/library/nginx:1.25.999": failed to resolve reference "docker.io/library/nginx:1.25.999": docker.io/library/nginx:1.25.999: not found

# 排查:
# 1. 验证镜像标签是否存在
curl -s "https://hub.docker.com/v2/repositories/library/nginx/tags?page_size=100" | jq -r '.results[].name' | grep 1.25

# 2. 检查拼写错误
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'

# 3. 验证镜像仓库路径
#    docker.io/library/nginx:1.25.3 (正确)
#    docker.io/nginx:1.25.3 (错误,缺少 library)

# 解决:
kubectl set image deployment/<deployment-name> <container-name>=nginx:1.25.3
```

### 6.2 DNS 解析失败

```bash
# 现象: DNS 相关错误
Failed to pull image "nginx:1.25.3": rpc error: code = Unknown desc = failed to pull and unpack image "docker.io/library/nginx:1.25.3": failed to resolve reference "docker.io/library/nginx:1.25.3": failed to do request: Head "https://registry-1.docker.io/v2/library/nginx/manifests/1.25.3": dial tcp: lookup registry-1.docker.io on 169.254.169.254:53: no such host

# 排查:
# 1. 测试 DNS 解析
kubectl debug node/<node-name> -it --image=nicolaka/netshoot
nslookup registry-1.docker.io
dig registry-1.docker.io

# 2. 检查节点 DNS 配置
cat /etc/resolv.conf

# 3. 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns

# 解决:
# 1. 修复节点 DNS 配置
sudo vi /etc/resolv.conf
# 添加可用 DNS 服务器,如:
#   nameserver 8.8.8.8
#   nameserver 1.1.1.1

# 2. 修复 CoreDNS
kubectl edit configmap coredns -n kube-system
kubectl rollout restart deployment coredns -n kube-system

# 3. 使用镜像 digest 引用(绕过域名解析,仅在镜像已缓存时有效)
kubectl set image deployment/<name> <container>=nginx@sha256:xxxxx
```

### 6.3 网络连接超时

```bash
# 现象: Timeout 或 Connection refused
Failed to pull image "nginx:1.25.3": rpc error: code = Unknown desc = failed to pull and unpack image "docker.io/library/nginx:1.25.3": failed to do request: Head "https://registry-1.docker.io/v2/library/nginx/manifests/1.25.3": dial tcp 52.54.232.21:443: i/o timeout

# 排查:
# 1. 测试网络连通性
kubectl debug node/<node-name> -it --image=nicolaka/netshoot
curl -v --max-time 10 https://registry-1.docker.io/v2/
telnet registry-1.docker.io 443

# 2. 检查防火墙规则
sudo iptables -L -n | grep 443
sudo firewall-cmd --list-all

# 3. 检查代理配置
env | grep -i proxy
cat /etc/systemd/system/containerd.service.d/http-proxy.conf

# 解决:
# 1. 配置 HTTP 代理 (containerd)
sudo mkdir -p /etc/systemd/system/containerd.service.d
cat <<EOF | sudo tee /etc/systemd/system/containerd.service.d/http-proxy.conf
[Service]
Environment="HTTP_PROXY=http://proxy.example.com:8080"
Environment="HTTPS_PROXY=http://proxy.example.com:8080"
Environment="NO_PROXY=localhost,127.0.0.1,10.0.0.0/8,172.16.0.0/12,192.168.0.0/16"
EOF

sudo systemctl daemon-reload
sudo systemctl restart containerd

# 2. 使用本地镜像仓库或镜像
```

### 6.4 TLS 证书验证失败

```bash
# 现象: x509 证书错误
Failed to pull image "myregistry.example.com/app:v1.0": rpc error: code = Unknown desc = failed to pull and unpack image "myregistry.example.com/app:v1.0": failed to resolve reference "myregistry.example.com/app:v1.0": failed to do request: Head "https://myregistry.example.com/v2/app/manifests/v1.0": x509: certificate signed by unknown authority

# 排查:
# 1. 测试 TLS 连接
openssl s_client -connect myregistry.example.com:443 -showcerts

# 2. 检查证书详情
echo | openssl s_client -connect myregistry.example.com:443 2>/dev/null | openssl x509 -noout -text

# 解决:
# 方法1: 添加 CA 证书到节点信任列表(推荐)
sudo cp myregistry-ca.crt /usr/local/share/ca-certificates/
sudo update-ca-certificates
sudo systemctl restart containerd

# 方法2: 配置 containerd 跳过验证(不推荐,仅用于测试)
sudo vi /etc/containerd/config.toml
# 添加:
[plugins."io.containerd.grpc.v1.cri".registry.configs."myregistry.example.com".tls]
  insecure_skip_verify = true

sudo systemctl restart containerd

# 方法3: 配置为 HTTP 仓库(不推荐)
[plugins."io.containerd.grpc.v1.cri".registry.configs."myregistry.example.com"]
  [plugins."io.containerd.grpc.v1.cri".registry.configs."myregistry.example.com".tls]
    insecure_skip_verify = true
```

### 6.5 镜像层下载中断

```bash
# 现象: unexpected EOF 或 context canceled
Failed to pull image "large-image:v1.0": rpc error: code = Unknown desc = failed to pull and unpack image "docker.io/library/large-image:v1.0": failed to copy: httpReadSeeker: failed open: unexpected EOF

# 排查:
# 1. 检查网络稳定性
ping -c 100 registry-1.docker.io
mtr registry-1.docker.io

# 2. 检查镜像大小
crane manifest <image> | jq '.layers[] | .size' | awk '{sum+=$1} END {print sum/1024/1024 " MB"}'

# 3. 查看 kubelet 日志中的超时配置
ps aux | grep kubelet | grep -oP -- '--image-pull-progress-deadline=\K[^ ]+'

# 解决:
# 1. 增加 kubelet 镜像拉取超时时间
# 编辑 kubelet 配置 /var/lib/kubelet/config.yaml
imageGCHighThresholdPercent: 85
imageGCLowThresholdPercent: 80
imageMinimumGCAge: 2m0s
imageMaximumGCAge: 0s
# 或使用启动参数:
# --image-pull-progress-deadline=5m (默认 1m)

# 2. 优化镜像大小,使用多阶段构建

# 3. 使用区域镜像缓存,缩短传输距离
```

---

## 七、Docker Hub 限流问题

### 7.1 Docker Hub 限流策略

| 用户类型 | 拉取限制 | 计费周期 | 限流依据 |
|:---|:---|:---|:---|
| **匿名用户** | 100 次 / 6 小时 | 6 小时滚动窗口 | 源 IP 地址 |
| **免费认证用户** | 200 次 / 6 小时 | 6 小时滚动窗口 | 用户账户 |
| **Pro 用户** | 5000 次 / 天 | 24 小时滚动窗口 | 用户账户 |
| **Team/Business** | 无限制 | - | - |

> **重要**: 在 Kubernetes 集群中,如果不使用认证,所有节点的拉取会算作同一个源 IP(通常是 NAT 网关 IP),极易触发限流。

### 7.2 限流错误识别

```bash
# 典型限流错误消息
Failed to pull image "nginx:1.25.3": rpc error: code = Unknown desc = failed to pull and unpack image "docker.io/library/nginx:1.25.3": failed to copy: httpReadSeeker: failed open: unexpected status code https://registry-1.docker.io/v2/library/nginx/blobs/sha256:xxxxx: 429 Too Many Requests - Server message: toomanyrequests: You have reached your pull rate limit. You may increase the limit by authenticating and upgrading: https://www.docker.com/increase-rate-limit

# 关键信息:
# - status code: 429 Too Many Requests
# - toomanyrequests: You have reached your pull rate limit
```

### 7.3 检查当前限流状态

```bash
# 方法1: 使用 curl 检查剩余配额(匿名)
TOKEN=$(curl -s "https://auth.docker.io/token?service=registry.docker.io&scope=repository:ratelimitpreview/test:pull" | jq -r .token)
curl -s --head -H "Authorization: Bearer $TOKEN" https://registry-1.docker.io/v2/ratelimitpreview/test/manifests/latest | grep -i ratelimit
# 输出示例:
# ratelimit-limit: 100;w=21600
# ratelimit-remaining: 42;w=21600

# 方法2: 使用认证用户检查配额
TOKEN=$(curl -s -u "username:password" "https://auth.docker.io/token?service=registry.docker.io&scope=repository:ratelimitpreview/test:pull" | jq -r .token)
curl -s --head -H "Authorization: Bearer $TOKEN" https://registry-1.docker.io/v2/ratelimitpreview/test/manifests/latest | grep -i ratelimit
# 输出示例:
# ratelimit-limit: 200;w=21600
# ratelimit-remaining: 185;w=21600
```

### 7.4 解决 Docker Hub 限流问题

#### 方案1: 使用 Docker Hub 认证(提高限额到 200 次)

```bash
# 1. 创建 Docker Hub 账户凭据 Secret
kubectl create secret docker-registry dockerhub-secret \
  --docker-server=https://index.docker.io/v1/ \
  --docker-username=<dockerhub-username> \
  --docker-password=<dockerhub-password> \
  --docker-email=<dockerhub-email> \
  -n default

# 2. 关联到默认 ServiceAccount
kubectl patch serviceaccount default -n default \
  -p '{"imagePullSecrets": [{"name": "dockerhub-secret"}]}'

# 3. 验证配置
kubectl get serviceaccount default -o yaml | grep -A 2 imagePullSecrets

# 4. 重启受影响的 Pod
kubectl rollout restart deployment/<deployment-name>
```

#### 方案2: 使用镜像仓库镜像(Mirror)

```bash
# 配置 containerd 使用国内镜像(如阿里云/腾讯云/DaoCloud)

# 编辑 /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".registry]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
      endpoint = ["https://mirror.gcr.io", "https://dockerproxy.com"]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."gcr.io"]
      endpoint = ["https://gcr.dockerproxy.com"]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."k8s.gcr.io"]
      endpoint = ["https://k8s.dockerproxy.com"]

# 重启 containerd
sudo systemctl restart containerd

# 验证配置
sudo crictl info | jq .config.registry
```

#### 方案3: 迁移到私有镜像仓库

```bash
# 1. 搭建 Harbor/Registry 私有仓库

# 2. 同步 Docker Hub 镜像到私有仓库
# 使用 skopeo 批量同步
for image in nginx:1.25.3 redis:7.2 mysql:8.0; do
  skopeo copy \
    docker://docker.io/library/$image \
    docker://harbor.example.com/library/$image \
    --dest-creds username:password
done

# 3. 更新 Deployment 镜像地址
kubectl set image deployment/nginx nginx=harbor.example.com/library/nginx:1.25.3

# 4. 使用 kustomize 或 Helm 统一管理镜像前缀
# kustomize 示例:
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
images:
  - name: nginx
    newName: harbor.example.com/library/nginx
    newTag: 1.25.3
```

#### 方案4: 使用镜像预热(减少拉取次数)

```bash
# 使用 DaemonSet 预热镜像到所有节点
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-prewarmer
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: image-prewarmer
  template:
    metadata:
      labels:
        app: image-prewarmer
    spec:
      initContainers:
        # 预拉取常用镜像
        - name: prewarm-nginx
          image: nginx:1.25.3
          command: ['sh', '-c', 'echo "Image prewarmed"']
        - name: prewarm-redis
          image: redis:7.2-alpine
          command: ['sh', '-c', 'echo "Image prewarmed"']
      containers:
        - name: pause
          image: k8s.gcr.io/pause:3.9
          resources:
            requests:
              cpu: 1m
              memory: 1Mi
EOF
```

#### 方案5: 使用镜像缓存代理(Pull-Through Cache)

```bash
# 部署 Registry 作为 Pull-Through Cache
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: registry-config
  namespace: kube-system
data:
  config.yml: |
    version: 0.1
    log:
      level: info
    storage:
      filesystem:
        rootdirectory: /var/lib/registry
      delete:
        enabled: true
    http:
      addr: :5000
    proxy:
      remoteurl: https://registry-1.docker.io
      username: <dockerhub-username>
      password: <dockerhub-password>
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: docker-registry-proxy
  namespace: kube-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: docker-registry-proxy
  template:
    metadata:
      labels:
        app: docker-registry-proxy
    spec:
      containers:
        - name: registry
          image: registry:2.8
          volumeMounts:
            - name: config
              mountPath: /etc/docker/registry
            - name: storage
              mountPath: /var/lib/registry
          ports:
            - containerPort: 5000
      volumes:
        - name: config
          configMap:
            name: registry-config
        - name: storage
          emptyDir: {}
---
apiVersion: v1
kind: Service
metadata:
  name: docker-registry-proxy
  namespace: kube-system
spec:
  selector:
    app: docker-registry-proxy
  ports:
    - port: 5000
      targetPort: 5000
EOF

# 配置 containerd 使用此代理
# 编辑 /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
  endpoint = ["http://docker-registry-proxy.kube-system.svc.cluster.local:5000"]
```

---

## 八、镜像仓库镜像与代理配置

### 8.1 配置 containerd 镜像加速

```toml
# /etc/containerd/config.toml 完整配置示例

version = 2

[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"

  [plugins."io.containerd.grpc.v1.cri".registry]
    config_path = ""

    [plugins."io.containerd.grpc.v1.cri".registry.auths]
      # 私有仓库认证配置
      [plugins."io.containerd.grpc.v1.cri".registry.auths."harbor.example.com"]
        username = "admin"
        password = "Harbor12345"

    [plugins."io.containerd.grpc.v1.cri".registry.configs]
      # 仓库特定配置
      [plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.example.com".tls]
        ca_file = "/etc/containerd/certs.d/harbor.example.com/ca.crt"
        insecure_skip_verify = false

      [plugins."io.containerd.grpc.v1.cri".registry.configs."myregistry.example.com".tls]
        insecure_skip_verify = true

    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      # Docker Hub 镜像
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = [
          "https://harbor.example.com/v2/dockerhub-proxy",
          "https://mirror.gcr.io",
          "https://dockerproxy.com"
        ]

      # GCR 镜像
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."gcr.io"]
        endpoint = ["https://gcr.dockerproxy.com"]

      # Quay.io 镜像
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."quay.io"]
        endpoint = ["https://quay.dockerproxy.com"]

      # k8s.gcr.io/registry.k8s.io 镜像
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.k8s.io"]
        endpoint = ["https://k8s.dockerproxy.com", "https://registry.aliyuncs.com/google_containers"]

      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."k8s.gcr.io"]
        endpoint = ["https://k8s.dockerproxy.com"]
```

```bash
# 应用配置
sudo systemctl restart containerd

# 验证配置
sudo crictl info | jq .config.registry

# 测试拉取
sudo crictl pull docker.io/library/nginx:1.25.3
```

### 8.2 国内镜像加速服务

| 提供商 | Docker Hub 镜像地址 | 说明 |
|:---|:---|:---|
| **阿里云** | `https://<your-id>.mirror.aliyuncs.com` | 需注册获取专属加速地址 |
| **腾讯云** | `https://mirror.ccs.tencentyun.com` | 腾讯云用户可用 |
| **DaoCloud** | `https://docker.m.daocloud.io` | 公共镜像 |
| **网易云** | `https://hub-mirror.c.163.com` | 公共镜像 |
| **百度云** | `https://mirror.baidubce.com` | 百度云用户可用 |
| **Docker Proxy** | `https://dockerproxy.com` | 第三方公共镜像 |

> **注意**: 部分镜像服务可能不稳定或有使用限制,建议企业用户搭建私有镜像仓库。

### 8.3 配置 HTTP 代理

```bash
# ========== containerd 配置代理 ==========

# 创建 systemd drop-in 配置
sudo mkdir -p /etc/systemd/system/containerd.service.d
cat <<EOF | sudo tee /etc/systemd/system/containerd.service.d/http-proxy.conf
[Service]
Environment="HTTP_PROXY=http://proxy.example.com:8080"
Environment="HTTPS_PROXY=http://proxy.example.com:8080"
Environment="NO_PROXY=localhost,127.0.0.1,10.0.0.0/8,172.16.0.0/12,192.168.0.0/16,.svc,.cluster.local"
EOF

# 重载并重启服务
sudo systemctl daemon-reload
sudo systemctl restart containerd

# 验证代理配置
sudo systemctl show containerd | grep -i proxy

# ========== docker 配置代理 ==========

# 创建 docker 代理配置
sudo mkdir -p /etc/systemd/system/docker.service.d
cat <<EOF | sudo tee /etc/systemd/system/docker.service.d/http-proxy.conf
[Service]
Environment="HTTP_PROXY=http://proxy.example.com:8080"
Environment="HTTPS_PROXY=http://proxy.example.com:8080"
Environment="NO_PROXY=localhost,127.0.0.1,10.0.0.0/8,172.16.0.0/12,192.168.0.0/16"
EOF

sudo systemctl daemon-reload
sudo systemctl restart docker

# ========== kubelet 配置代理(如果需要) ==========

# 编辑 /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
Environment="HTTP_PROXY=http://proxy.example.com:8080"
Environment="HTTPS_PROXY=http://proxy.example.com:8080"
Environment="NO_PROXY=localhost,127.0.0.1,10.0.0.0/8,172.16.0.0/12,192.168.0.0/16,<api-server-ip>"

sudo systemctl daemon-reload
sudo systemctl restart kubelet
```

### 8.4 搭建 Harbor 私有仓库

```bash
# 使用 Helm 部署 Harbor

# 1. 添加 Harbor Helm 仓库
helm repo add harbor https://helm.goharbor.io
helm repo update

# 2. 创建 values.yaml 配置
cat <<EOF > harbor-values.yaml
expose:
  type: ingress
  tls:
    enabled: true
    certSource: secret
    secret:
      secretName: harbor-tls
      notarySecretName: notary-tls
  ingress:
    hosts:
      core: harbor.example.com
      notary: notary.harbor.example.com
    className: nginx

persistence:
  enabled: true
  persistentVolumeClaim:
    registry:
      size: 200Gi
      storageClass: standard
    chartmuseum:
      size: 10Gi
    jobservice:
      size: 10Gi
    database:
      size: 10Gi
    redis:
      size: 10Gi

harborAdminPassword: "HarborAdmin123"

# 启用镜像扫描
trivy:
  enabled: true

# 启用 Replication(镜像同步)
# 可以同步 Docker Hub 镜像到 Harbor
EOF

# 3. 部署 Harbor
kubectl create namespace harbor
helm install harbor harbor/harbor \
  -n harbor \
  -f harbor-values.yaml

# 4. 等待 Harbor 就绪
kubectl get pods -n harbor -w

# 5. 配置 Replication Policy(Web UI)
# 登录 https://harbor.example.com
# Project → 创建项目 "dockerhub-proxy"
# Administration → Replications → New Replication Rule:
#   Name: dockerhub-sync
#   Replication mode: Pull-based
#   Source registry: 添加 Docker Hub (https://hub.docker.com)
#   Source resource filter: library/** (同步所有 library 镜像)
#   Destination namespace: dockerhub-proxy
#   Trigger Mode: Scheduled (如每天凌晨2点)

# 6. 配置 containerd 使用 Harbor
# 编辑 /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
  endpoint = ["https://harbor.example.com/v2/dockerhub-proxy"]

[plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.example.com".auth]
  username = "admin"
  password = "HarborAdmin123"
```

---

## 九、生产环境最佳实践

### 9.1 镜像拉取性能优化

| 实践 | 说明 | 影响 |
|:---|:---|:---|
| **使用明确版本标签** | 避免 `:latest`,使用 `:v1.2.3` 或 digest 引用 | 提高缓存命中率,避免不必要的拉取 |
| **配置 imagePullPolicy** | 生产环境使用 `Always`,非生产使用 `IfNotPresent` | 平衡安全性和性能 |
| **镜像预热** | 使用 DaemonSet 或脚本预拉取镜像到节点 | 加速首次部署,减少拉取失败风险 |
| **本地镜像仓库** | 部署 Harbor/Registry,缩短网络距离 | 显著提升拉取速度,降低外部依赖 |
| **镜像大小优化** | 使用 Alpine 基础镜像,多阶段构建,删除无用文件 | 减少传输时间和存储占用 |
| **并发拉取** | 配置 `--serialize-image-pulls=false` | 提高多镜像 Pod 的启动速度 |
| **配置镜像 GC** | 合理设置 GC 阈值,避免频繁删除常用镜像 | 提高缓存命中率 |

### 9.2 镜像安全最佳实践

```bash
# 1. 使用镜像 digest 确保完整性和不变性
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
spec:
  template:
    spec:
      containers:
        - name: app
          image: nginx@sha256:5f3e13ba94f1c9e5a79f6f96b0f6c13c6e5a1f6a7e8f9b0e1f2f3f4f5f6f7f8f
          imagePullPolicy: IfNotPresent

# 2. 启用镜像扫描(Harbor/Trivy)
# Harbor Webhook 集成到 CI/CD,阻止部署存在高危漏洞的镜像

# 3. 使用 OPA/Gatekeeper 策略强制镜像来源
apiVersion: templates.gatekeeper.sh/v1beta1
kind: ConstraintTemplate
metadata:
  name: k8sallowedrepos
spec:
  crd:
    spec:
      names:
        kind: K8sAllowedRepos
      validation:
        openAPIV3Schema:
          properties:
            repos:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8sallowedrepos
        violation[{"msg": msg}] {
          container := input.review.object.spec.containers[_]
          not strings.any_prefix_match(container.image, input.parameters.repos)
          msg := sprintf("Image '%v' not from allowed repos", [container.image])
        }
---
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedRepos
metadata:
  name: allowed-repos
spec:
  match:
    kinds:
      - apiGroups: ["apps"]
        kinds: ["Deployment", "StatefulSet", "DaemonSet"]
  parameters:
    repos:
      - "harbor.example.com/"
      - "gcr.io/my-project/"

# 4. 定期更新基础镜像,修复安全漏洞
# 使用 Renovate/Dependabot 自动化镜像版本更新

# 5. 最小化镜像权限,使用 securityContext
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  readOnlyRootFilesystem: true
  allowPrivilegeEscalation: false
  capabilities:
    drop:
      - ALL
```

### 9.3 镜像拉取监控和告警

```yaml
# Prometheus 告警规则示例

groups:
  - name: image-pull-alerts
    interval: 30s
    rules:
      # 镜像拉取失败告警
      - alert: ImagePullFailed
        expr: |
          increase(
            kube_pod_container_status_waiting_reason{reason=~"ErrImagePull|ImagePullBackOff"}[5m]
          ) > 0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 镜像拉取失败"
          description: "容器 {{ $labels.container }} 的镜像拉取持续失败超过 5 分钟"

      # 镜像拉取时间过长告警
      - alert: ImagePullSlow
        expr: |
          increase(
            container_image_pull_duration_seconds_sum[5m]
          ) / increase(
            container_image_pull_duration_seconds_count[5m]
          ) > 120
        for: 10m
        labels:
          severity: info
        annotations:
          summary: "镜像拉取速度过慢"
          description: "过去 5 分钟平均镜像拉取时间超过 120 秒"

      # ImagePullBackOff 数量过多
      - alert: HighImagePullBackOff
        expr: |
          count(
            kube_pod_container_status_waiting_reason{reason="ImagePullBackOff"}
          ) > 5
        for: 10m
        labels:
          severity: critical
        annotations:
          summary: "大量 Pod 处于 ImagePullBackOff 状态"
          description: "当前有 {{ $value }} 个 Pod 处于 ImagePullBackOff,可能存在镜像仓库或网络问题"

      # Docker Hub 限流告警(需要自定义 exporter)
      - alert: DockerHubRateLimitApproaching
        expr: |
          dockerhub_ratelimit_remaining < 20
        labels:
          severity: warning
        annotations:
          summary: "Docker Hub 拉取配额即将耗尽"
          description: "剩余配额: {{ $value }}, 建议配置认证或使用镜像"
```

```bash
# 自定义 Prometheus Exporter 监控 Docker Hub 限流
# 部署一个简单的 exporter 定期检查 Docker Hub 配额

cat <<'EOF' > dockerhub-ratelimit-exporter.sh
#!/bin/bash
# Docker Hub Rate Limit Exporter for Prometheus

TOKEN=$(curl -s "https://auth.docker.io/token?service=registry.docker.io&scope=repository:ratelimitpreview/test:pull" | jq -r .token)
RESPONSE=$(curl -s --head -H "Authorization: Bearer $TOKEN" https://registry-1.docker.io/v2/ratelimitpreview/test/manifests/latest)

LIMIT=$(echo "$RESPONSE" | grep -i "ratelimit-limit:" | awk '{print $2}' | cut -d';' -f1)
REMAINING=$(echo "$RESPONSE" | grep -i "ratelimit-remaining:" | awk '{print $2}' | cut -d';' -f1)

cat <<METRICS
# HELP dockerhub_ratelimit_limit Docker Hub rate limit
# TYPE dockerhub_ratelimit_limit gauge
dockerhub_ratelimit_limit ${LIMIT:-0}

# HELP dockerhub_ratelimit_remaining Docker Hub rate limit remaining
# TYPE dockerhub_ratelimit_remaining gauge
dockerhub_ratelimit_remaining ${REMAINING:-0}
METRICS
EOF

chmod +x dockerhub-ratelimit-exporter.sh

# 部署为 Kubernetes CronJob,定期更新指标
```

### 9.4 镜像拉取故障排查清单

```bash
# ========== 快速诊断脚本 ==========

#!/bin/bash
# image-pull-diagnostic.sh

POD_NAME=$1
NAMESPACE=${2:-default}

echo "=== Image Pull Diagnostic for Pod: $NAMESPACE/$POD_NAME ==="
echo ""

# 1. Pod 基本信息
echo "1. Pod Status:"
kubectl get pod $POD_NAME -n $NAMESPACE -o wide
echo ""

# 2. 容器镜像配置
echo "2. Container Image Configuration:"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{range .spec.containers[*]}{.name}{"\t"}{.image}{"\t"}{.imagePullPolicy}{"\n"}{end}' | column -t
echo ""

# 3. ImagePullSecrets
echo "3. ImagePullSecrets:"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.imagePullSecrets[*].name}'
echo ""
echo ""

# 4. 容器状态
echo "4. Container Status:"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{range .status.containerStatuses[*]}{.name}{"\t"}{.state}{"\n"}{end}'
echo ""

# 5. 相关事件
echo "5. Related Events:"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME --sort-by='.lastTimestamp' | tail -20
echo ""

# 6. 镜像拉取失败详细信息
echo "6. Image Pull Failure Details:"
kubectl describe pod $POD_NAME -n $NAMESPACE | grep -A 10 "Failed"
echo ""

# 7. 节点信息
NODE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeName}')
echo "7. Node: $NODE"
echo "   Node Images:"
kubectl get node $NODE -o json | jq -r '.status.images[] | select(.names[] | contains("'$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.containers[0].image}' | cut -d':' -f1)'")) | .names[0]'
echo ""

# 8. ServiceAccount ImagePullSecrets
SA=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.serviceAccountName}')
echo "8. ServiceAccount: $SA"
kubectl get serviceaccount $SA -n $NAMESPACE -o jsonpath='{.imagePullSecrets[*].name}'
echo ""
echo ""

# 9. 检查镜像仓库连通性
IMAGE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.containers[0].image}')
REGISTRY=$(echo $IMAGE | cut -d'/' -f1)
echo "9. Registry Connectivity Test: $REGISTRY"
kubectl debug node/$NODE -it --image=nicolaka/netshoot -- curl -I https://$REGISTRY/v2/ 2>&1 | head -5
echo ""

# 10. 建议
echo "10. Troubleshooting Suggestions:"
if kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME | grep -q "not found"; then
    echo "   - Check if image name and tag are correct"
    echo "   - Verify the image exists in the registry"
elif kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME | grep -q "Unauthorized"; then
    echo "   - Verify imagePullSecret is correctly configured"
    echo "   - Check if credentials are valid"
elif kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME | grep -q "timeout"; then
    echo "   - Check network connectivity to registry"
    echo "   - Verify firewall/proxy settings"
elif kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME | grep -q "Too Many Requests"; then
    echo "   - Configure Docker Hub authentication to increase rate limit"
    echo "   - Use a mirror registry or private registry"
fi

echo ""
echo "=== Diagnostic Complete ==="
```

### 9.5 镜像拉取配置检查脚本

```bash
#!/bin/bash
# image-pull-config-audit.sh
# 审计集群中所有 Deployment 的镜像配置

echo "=== Kubernetes Image Pull Configuration Audit ==="
echo ""

# 1. 检查使用 :latest 标签的 Deployment
echo "1. Deployments using ':latest' tag:"
kubectl get deployments -A -o json | jq -r '.items[] | select(.spec.template.spec.containers[].image | endswith(":latest")) | "\(.metadata.namespace)/\(.metadata.name)"' | sort | uniq
echo ""

# 2. 检查未指定 imagePullPolicy 的 Deployment(使用默认值)
echo "2. Deployments without explicit imagePullPolicy:"
kubectl get deployments -A -o json | jq -r '.items[] | select(.spec.template.spec.containers[].imagePullPolicy == null) | "\(.metadata.namespace)/\(.metadata.name)"' | sort | uniq
echo ""

# 3. 检查未使用 imagePullSecrets 的 Deployment(可能拉取私有镜像失败)
echo "3. Deployments without imagePullSecrets:"
kubectl get deployments -A -o json | jq -r '.items[] | select(.spec.template.spec.imagePullSecrets == null) | "\(.metadata.namespace)/\(.metadata.name)"' | sort | uniq
echo ""

# 4. 统计镜像来源
echo "4. Image Registry Distribution:"
kubectl get deployments -A -o json | jq -r '.items[].spec.template.spec.containers[].image' | sed 's|/.*||' | sort | uniq -c | sort -rn
echo ""

# 5. 检查使用 imagePullPolicy: Never 的 Deployment(高风险)
echo "5. Deployments with imagePullPolicy: Never:"
kubectl get deployments -A -o json | jq -r '.items[] | select(.spec.template.spec.containers[].imagePullPolicy == "Never") | "\(.metadata.namespace)/\(.metadata.name)"' | sort | uniq
echo ""

# 6. 检查 Docker Hub 镜像(可能受限流影响)
echo "6. Deployments using Docker Hub images:"
kubectl get deployments -A -o json | jq -r '.items[] | select(.spec.template.spec.containers[].image | test("^docker.io|^library/|^[^/]+/[^/]+$")) | "\(.metadata.namespace)/\(.metadata.name)"' | sort | uniq | head -10
echo "   (showing first 10, use full output for complete list)"
echo ""

echo "=== Audit Complete ==="
```

---

## 十、相关文档交叉引用

### 10.1 Domain-33 内部文档

- **[01-event-system-architecture.md](./01-event-system-architecture.md)** - Kubernetes 事件系统架构与 API 参考
- **[02-pod-container-lifecycle-events.md](./02-pod-container-lifecycle-events.md)** - Pod 和容器生命周期事件

### 10.2 Domain-22: 容器镜像管理

- **[Domain-22: Container Image Management](../domain-22-container-image-management/README.md)** - 镜像构建、存储、分发和安全管理
- **[Domain-22: Dockerfile Best Practices](../domain-22-container-image-management/01-dockerfile-best-practices.md)** - 镜像构建最佳实践
- **[Domain-22: Image Registry Setup](../domain-22-container-image-management/02-image-registry-setup.md)** - 镜像仓库搭建和配置

### 10.3 Domain-12: 故障排查

- **[Domain-12: Troubleshooting / 27-image-registry-troubleshooting.md](../../topic-structural-trouble-shooting/02-node-components/05-image-registry-troubleshooting.md)** - 镜像仓库故障排查
- **[Domain-12: Troubleshooting / 03-container-runtime-troubleshooting.md](../../topic-structural-trouble-shooting/02-node-components/03-container-runtime-troubleshooting.md)** - 容器运行时(containerd/docker)故障排查
- **[Domain-12: Troubleshooting / 01-pod-troubleshooting.md](../../topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md)** - Pod 故障排查综合指南

### 10.4 Domain-2: 节点管理

- **[Domain-2: Node Management](../domain-2-node-management/README.md)** - 节点配置和管理
- **[Domain-2: Kubelet Configuration](../domain-2-node-management/02-kubelet-configuration.md)** - kubelet 配置参数详解

### 10.5 Domain-5: 网络

- **[Domain-5: Networking / DNS Configuration](../domain-5-networking/03-dns-configuration.md)** - DNS 配置和故障排查(影响镜像仓库域名解析)
- **[Domain-5: Networking / Proxy Configuration](../domain-5-networking/05-proxy-configuration.md)** - 代理配置(影响镜像拉取)

### 10.6 Domain-7: 安全

- **[Domain-7: Security / Secret Management](../domain-7-security/03-secret-management.md)** - Secret 管理(包括 ImagePullSecret)
- **[Domain-7: Security / RBAC](../domain-7-security/01-rbac.md)** - RBAC 权限管理(ServiceAccount imagePullSecrets)

### 10.7 运维工具

- **[crictl 命令参考](https://kubernetes.io/docs/tasks/debug/debug-cluster/crictl/)** - 容器运行时 CLI 工具
- **[skopeo 工具](https://github.com/containers/skopeo)** - 镜像检查和迁移工具
- **[crane 工具](https://github.com/google/go-containerregistry/tree/main/cmd/crane)** - Google 开发的镜像操作工具

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 03/15
