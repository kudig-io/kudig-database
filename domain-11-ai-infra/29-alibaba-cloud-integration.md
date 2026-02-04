# 15 - 阿里云特定集成表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [help.aliyun.com/product/85222.html](https://help.aliyun.com/product/85222.html)

## ACK产品版本对比

| 版本 | 控制平面 | 适用场景 | 成本 | SLA | 功能差异 |
|-----|---------|---------|------|-----|---------|
| **ACK专有版** | 用户管理 | 完全控制需求 | 按ECS计费 | 99.5% | 自行维护控制平面 |
| **ACK托管版** | 阿里云托管 | 一般生产环境 | 免控制平面费 | 99.95% | 自动运维控制平面 |
| **ACK Pro版** | 阿里云托管增强 | 大规模/高可用 | 按集群计费 | 99.95% | 增强调度/安全/可观测 |
| **ACK Serverless** | 完全托管 | 弹性场景 | 按Pod计费 | 99.95% | 无需管理节点 |
| **ACK Edge** | 边缘托管 | 边缘计算 | 混合计费 | - | 边缘节点管理 |
| **ACK One** | 多集群管理 | 多集群场景 | 按集群计费 | - | 跨集群编排 |

## ACK版本支持

| K8S版本 | ACK支持状态 | 推荐度 | EOL日期 |
|--------|------------|-------|--------|
| **v1.28** | 维护中 | 可用 | 参考官方 |
| **v1.29** | 维护中 | 推荐 | 参考官方 |
| **v1.30** | 维护中 | 推荐 | 参考官方 |
| **v1.31** | 最新稳定 | 强烈推荐 | - |
| **v1.32** | 最新 | 推荐 | - |

## ACR(容器镜像服务)集成

| 功能 | 配置方式 | 版本要求 | 说明 |
|-----|---------|---------|------|
| **免密拉取** | 集群配置开启 | v1.25+ | ACK自动配置imagePullSecrets |
| **镜像加速** | 按需开启 | v1.25+ | P2P加速大镜像分发 |
| **漏洞扫描** | ACR企业版 | - | 自动扫描CVE |
| **镜像签名** | ACR企业版 | v1.28+ | 内容信任验证 |
| **制品同步** | ACR配置 | - | 跨地域镜像同步 |
| **Helm Chart** | ACR企业版 | - | Helm Chart仓库 |

```yaml
# ACK免密拉取配置
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/namespace/image:tag
  # 无需配置imagePullSecrets，ACK自动注入
```

## 云盘CSI集成

| 云盘类型 | StorageClass | IOPS | 适用场景 |
|---------|-------------|------|---------|
| **ESSD PL0** | alicloud-disk-essd-pl0 | 10,000 | 开发测试 |
| **ESSD PL1** | alicloud-disk-essd | 50,000 | 一般生产 |
| **ESSD PL2** | alicloud-disk-essd-pl2 | 100,000 | 中型数据库 |
| **ESSD PL3** | alicloud-disk-essd-pl3 | 1,000,000 | 大型数据库 |
| **SSD** | alicloud-disk-ssd | 25,000 | 一般应用 |
| **高效云盘** | alicloud-disk-efficiency | 5,000 | 冷数据 |

```yaml
# ESSD PL1 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

## NAS CSI集成

| NAS类型 | 特点 | StorageClass | 适用场景 |
|--------|------|-------------|---------|
| **通用型NAS** | 标准性能 | alicloud-nas | 文件共享 |
| **极速型NAS** | 高性能 | alicloud-nas-extreme | 高IO需求 |
| **CPFS** | 并行文件系统 | alicloud-cpfs | AI/HPC |

## SLB负载均衡集成

| 注解 | 用途 | 示例值 |
|-----|------|-------|
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec` | SLB规格 | slb.s2.small |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type` | 地址类型 | internet/intranet |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id` | 复用已有SLB | lb-xxx |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-force-override-listeners` | 覆盖监听 | true |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth` | 带宽 | 100 |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id` | HTTPS证书 | cert-xxx |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag` | 健康检查 | on |

```yaml
# SLB Service示例
apiVersion: v1
kind: Service
metadata:
  name: nginx-svc
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.small"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 80
  selector:
    app: nginx
```

## ALB Ingress集成

| 功能 | 注解 | 说明 |
|-----|------|------|
| **健康检查** | `alb.ingress.kubernetes.io/healthcheck-path` | 健康检查路径 |
| **SSL证书** | `alb.ingress.kubernetes.io/ssl-redirect` | HTTPS重定向 |
| **灰度发布** | `alb.ingress.kubernetes.io/canary` | 灰度流量控制 |
| **限流** | `alb.ingress.kubernetes.io/traffic-limit-qps` | QPS限制 |
| **跨域** | `alb.ingress.kubernetes.io/enable-cors` | CORS支持 |
| **后端协议** | `alb.ingress.kubernetes.io/backend-protocol` | HTTP/HTTPS/gRPC |

## Terway CNI网络模式

| 模式 | 说明 | 性能 | IP消耗 | 适用场景 |
|-----|------|------|-------|---------|
| **VPC** | VPC路由模式 | 高 | 低 | 小型集群 |
| **ENI** | 弹性网卡模式 | 最高 | 高 | 性能敏感 |
| **ENIIP** | ENI多IP模式 | 高 | 中 | 大型集群推荐 |
| **Trunk ENI** | 中继ENI模式 | 最高 | 中 | 高密度部署 |

## ARMS可观测性集成

| 功能 | 配置方式 | 数据源 |
|-----|---------|-------|
| **Prometheus监控** | 组件安装 | 组件指标 |
| **应用监控** | Agent注入 | APM数据 |
| **前端监控** | SDK集成 | 前端性能 |
| **告警** | 控制台配置 | 所有数据源 |
| **Grafana** | 自动集成 | Prometheus |

## SLS日志集成

| 功能 | 配置方式 | 采集对象 |
|-----|---------|---------|
| **容器日志** | Logtail DaemonSet | stdout/文件日志 |
| **K8S事件** | 控制台开启 | Event API |
| **Ingress日志** | 注解配置 | 访问日志 |
| **审计日志** | 控制台开启 | API审计 |

```yaml
# Logtail采集配置
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: app-stdout
spec:
  logstore: app-stdout
  shardCount: 2
  lifeCycle: 30
  logtailConfig:
    inputType: plugin
    configName: app-stdout
    inputDetail:
      plugin:
        inputs:
        - type: service_docker_stdout
          detail:
            Stdout: true
            Stderr: true
            IncludeEnv:
              APP: "myapp"
```

## 弹性伸缩集成

| 功能 | 配置方式 | 触发条件 |
|-----|---------|---------|
| **节点自动伸缩** | 节点池配置 | Pod资源需求 |
| **定时伸缩** | 节点池配置 | Cron表达式 |
| **Virtual Node** | 组件安装 | ECI扩展 |
| **Spot实例** | 节点池配置 | 成本优化 |

## 安全集成

| 功能 | 产品 | 配置方式 |
|-----|------|---------|
| **Secret加密** | KMS | 集群配置 |
| **镜像签名** | ACR+KMS | 策略配置 |
| **网络隔离** | 安全组 | 节点池配置 |
| **运行时安全** | 云安全中心 | Agent安装 |
| **审计** | 操作审计 | 自动集成 |
| **合规检查** | 配置审计 | 规则配置 |

## ACK常用CLI命令

```bash
# 获取集群列表
aliyun cs DescribeClustersV1

# 获取kubeconfig
aliyun cs GET /k8s/<cluster-id>/user_config | jq -r '.config' > ~/.kube/config

# 获取集群详情
aliyun cs DescribeClusterDetail --ClusterId <cluster-id>

# 升级集群
aliyun cs UpgradeCluster --ClusterId <cluster-id> --version 1.31.x

# 添加节点池
aliyun cs CreateClusterNodePool --ClusterId <cluster-id> --body '{...}'

# 扩容节点池
aliyun cs ScaleClusterNodePool --ClusterId <cluster-id> --NodepoolId <nodepool-id> --body '{"count": 5}'
```

## ACK成本优化

| 优化项 | 方法 | 节省比例 |
|-------|------|---------|
| **Spot实例** | 节点池配置 | 50-90% |
| **预留实例** | 提前购买 | 30-60% |
| **节省计划** | 承诺使用 | 20-50% |
| **弹性伸缩** | 按需扩缩 | 变化 |
| **资源画像** | ARMS分析 | 优化资源配置 |
| **SLB复用** | 注解配置 | 减少SLB数量 |

# 15 - 阿里云特定集成表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [help.aliyun.com/product/85222.html](https://help.aliyun.com/product/85222.html)

## 目录

1. [ACK产品矩阵](#ack产品矩阵)
2. [容器镜像服务 (ACR)](#容器镜像服务-acr)
3. [存储集成](#存储集成)
4. [网络集成](#网络集成)
5. [负载均衡集成](#负载均衡集成)
6. [可观测性集成](#可观测性集成)
7. [安全集成](#安全集成)
8. [弹性伸缩](#弹性伸缩)
9. [成本优化](#成本优化)
10. [最佳实践](#最佳实践)

---

## ACK产品矩阵

### 产品版本对比

| 版本 | 控制平面 | 适用场景 | 成本 | SLA | 功能差异 |
|-----|---------|---------|------|-----|---------|
| **ACK专有版** | 用户管理 | 完全控制需求 | 按ECS计费 | 99.5% | 自行维护控制平面 |
| **ACK托管版** | 阿里云托管 | 一般生产环境 | 免控制平面费 | 99.95% | 自动运维控制平面 |
| **ACK Pro版** | 阿里云托管增强 | 大规模/高可用 | 0.48元/集群/小时 | 99.95% | 增强调度/安全/可观测 |
| **ACK Serverless** | 完全托管 | 弹性场景 | 按Pod计费 | 99.95% | 无需管理节点 |
| **ACK Edge** | 边缘托管 | 边缘计算 | 混合计费 | - | 边缘节点管理 |
| **ACK One** | 多集群管理 | 多集群场景 | 按集群计费 | - | 跨集群编排 |

### ACK Pro版增强特性

| 特性分类 | 功能 | 说明 |
|---------|------|------|
| **可靠性** | etcd容灾备份 | 自动备份，异地容灾 |
| | API Server高可用 | 多副本，异地容灾 |
| **安全** | 审计日志增强 | 180天保留 |
| | Secret加密 | KMS集成 |
| | 安全巡检 | 自动安全扫描 |
| **可观测** | 事件中心 | 90天事件保留 |
| | ARMS集成 | 免费额度 |
| **调度** | 增强调度器 | Gang调度，拓扑感知 |

### 成本对比 (200节点集群/月)

| 版本 | 控制平面成本 | 节点成本 | 监控成本 | 总成本 |
|-----|------------|---------|---------|-------|
| **托管版** | 0元 | 60,000元 | 1,500元 | 61,500元 |
| **Pro版** | 345元 | 60,000元 | 500元(包含额度) | 60,845元 |
| **专有版** | 3,000元(3台Master) | 60,000元 | 1,500元 | 64,500元 |

**结论**: 大规模集群使用Pro版更经济

---

## 容器镜像服务 (ACR)

### ACR版本对比

| 特性 | 个人版 | 企业版基础版 | 企业版标准版 | 企业版高级版 |
|-----|-------|------------|------------|------------|
| **价格** | 免费 | 300元/月 | 600元/月 | 1200元/月 |
| **存储空间** | 无限 | 1TB | 3TB | 10TB |
| **镜像拉取QPS** | 10 | 300 | 1000 | 3000 |
| **漏洞扫描** | ✗ | ✓ | ✓ | ✓ |
| **镜像同步** | ✗ | ✓ 同地域 | ✓ 跨地域 | ✓ 跨地域 |
| **镜像加速** | ✗ | ✗ | ✓ P2P | ✓ P2P |
| **Helm Chart** | ✗ | ✓ | ✓ | ✓ |
| **构建规则** | 100 | 无限 | 无限 | 无限 |
| **实例数** | 共享 | 独享 | 独享 | 独享 |

### ACR企业版架构

```
┌─────────────────────────────────────┐
│  ACR企业版实例 (独享资源)            │
│                                     │
│  ┌──────────┐  ┌─────────────┐    │
│  │ 镜像仓库  │  │ Helm Chart  │    │
│  └──────────┘  └─────────────┘    │
│  ┌──────────┐  ┌─────────────┐    │
│  │ 漏洞扫描  │  │ 镜像签名    │    │
│  └──────────┘  └─────────────┘    │
└─────────────────────────────────────┘
         ↓ VPC专线/公网
┌─────────────────────────────────────┐
│  ACK集群                            │
│  - 免密拉取                          │
│  - P2P加速                          │
│  - 自动漏洞扫描                      │
└─────────────────────────────────────┘
```

### ACK免密拉取配置

```yaml
# ACK自动配置，无需手动配置imagePullSecrets
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/namespace/image:tag
  # ACK自动注入imagePullSecrets
```

### 镜像加速配置

```yaml
# 启用P2P加速 (ACR企业版标准版及以上)
# 在ACK集群中自动启用，无需配置

# 对于大镜像(>1GB)，拉取速度提升10倍
# 示例: 2GB镜像从20分钟降至2分钟
```

### 镜像构建最佳实践

```yaml
# ACR构建规则配置
代码源: GitHub/GitLab/码云
触发条件: 
  - Push到main分支
  - 创建Tag
构建规则:
  - Tag: {branch}-{commit_short}-{timestamp}
  - latest: 仅main分支
构建配置:
  Dockerfile路径: ./Dockerfile
  构建目录: ./
  构建参数: 
    - BUILD_ENV=production
镜像扫描: 自动扫描
构建通知: 钉钉/邮件
```

### 镜像同步策略

```yaml
# 跨地域同步配置
源实例: 华东1(杭州)
目标实例:
  - 华北2(北京) - 容灾
  - 华南1(深圳) - 就近访问
同步规则:
  命名空间: production/*
  同步触发: 实时同步
  同步策略: 增量同步
```

---

## 存储集成

### 云盘CSI完整配置

#### 性能等级对比

| 云盘类型 | PL等级 | 容量范围 | 最大IOPS | 最大吞吐 | 适用场景 | 月成本(100GB) |
|---------|--------|---------|---------|---------|---------|--------------|
| **ESSD** | PL0 | 40-32768GB | 10,000 | 180MB/s | 开发测试 | 105元 |
| **ESSD** | PL1 | 20-32768GB | 50,000 | 350MB/s | 一般生产 | 150元 |
| **ESSD** | PL2 | 461-32768GB | 100,000 | 750MB/s | 中型数据库 | 210元 |
| **ESSD** | PL3 | 1261-32768GB | 1,000,000 | 4000MB/s | 大型数据库 | 350元 |
| **SSD** | - | 20-32768GB | 25,000 | 300MB/s | 一般应用 | 100元 |
| **高效云盘** | - | 20-32768GB | 5,000 | 140MB/s | 冷数据 | 35元 |

#### 生产级StorageClass配置

```yaml
# ESSD PL1 - 推荐通用配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd-pl1
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  encrypted: "true"  # 启用加密
  kmsKeyId: ""  # 留空使用默认密钥
  regionId: cn-hangzhou
  zoneId: cn-hangzhou-h  # 指定可用区
  resourceGroupId: ""  # 资源组
reclaimPolicy: Retain  # 生产环境必须Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定
mountOptions:
  - noatime
  - nodiratime

---
# ESSD PL2 - 数据库专用
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd-pl2-db
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL2
  encrypted: "true"
  provisionedIops: "100000"  # 预配置IOPS
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### NAS CSI完整配置

#### NAS类型对比

| NAS类型 | 协议 | 吞吐量 | IOPS | 时延 | 适用场景 | 月成本(1TB) |
|--------|------|-------|------|------|---------|-----------|
| **通用型** | NFS/SMB | 150MB/s | 15,000 | 1-10ms | 通用共享 | 120元 |
| **极速型** | NFS | 600MB/s | 30,000 | <1ms | 高性能 | 660元 |
| **CPFS** | POSIX | 100GB/s | 百万级 | <1ms | AI/HPC | 按用量 |

#### NAS动态供给配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-nas-auto
provisioner: nasplugin.csi.alibabacloud.com
parameters:
  volumeAs: subpath  # 子目录模式
  server: "nas-id.cn-hangzhou.nas.aliyuncs.com:/share/"
  archiveOnDelete: "false"  # 删除时是否归档
reclaimPolicy: Retain
mountOptions:
  - vers=4.1
  - noresvport
  - rsize=1048576
  - wsize=1048576
  - hard
  - timeo=600
  - retrans=2
```

### OSS CSI配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-oss
provisioner: ossplugin.csi.alibabacloud.com
parameters:
  bucket: "my-bucket"
  url: "oss-cn-hangzhou.aliyuncs.com"
  otherOpts: "-o max_stat_cache_size=0 -o allow_other"
reclaimPolicy: Retain
volumeBindingMode: Immediate
```

---

## 网络集成

### Terway CNI网络模式详解

| 模式 | 原理 | Pod IP | 性能 | IP消耗 | 适用场景 |
|-----|------|--------|------|-------|---------|
| **VPC路由** | VPC路由表 | VPC子网 | 高 | 低(共享节点IP段) | 小型集群(<200节点) |
| **ENI独占** | Pod独占ENI | VPC IP | 最高(无封装) | 高(1 Pod=1 ENI) | 性能关键应用 |
| **ENI多IP** | ENI辅助IP | VPC IP | 高 | 中(1 ENI=10+ Pod) | 推荐(大规模) |
| **Trunk ENI** | 中继ENI | VPC IP | 最高 | 中 | 高密度部署 |

### Terway性能对比

| 网络模式 | 带宽(Gbps) | 延迟(us) | PPS(万) | 网络损耗 |
|---------|-----------|---------|---------|---------|
| **ENI独占** | 25 | <50 | 480 | 0% |
| **ENI多IP** | 25 | <80 | 480 | <3% |
| **Trunk ENI** | 25 | <60 | 480 | <2% |
| **VPC路由** | 25 | <100 | 450 | <5% |
| **Flannel VXLAN** | 10 | >200 | 300 | 15-20% |

### 网络策略与安全组集成

```yaml
# Terway NetworkPolicy自动创建安全组规则
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-same-namespace
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector: {}  # 同namespace的Pod

# Terway会自动创建对应的安全组规则，性能优于iptables
```

---

## 负载均衡集成

### SLB Service注解大全

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx-svc
  annotations:
    # === 基础配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s3.medium"  # SLB规格
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"  # internet/intranet
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybytraffic"  # paybytraffic/paybybandwidth
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "100"  # 带宽(Mbps)
    
    # === 复用已有SLB ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-force-override-listeners: "true"  # 强制覆盖监听
    
    # === HTTPS配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "https:443"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "cert-xxx"  # 证书ID
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-forward-port: "80:443"  # HTTP重定向到HTTPS
    
    # === 健康检查 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"  # tcp/http
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "2"
    
    # === 会话保持 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-persistence-timeout: "1800"  # 会话保持时间(秒)
    
    # === 高级配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"  # wrr/wlc/rr
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-master-zoneid: "cn-hangzhou-h"  # 主可用区
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-slave-zoneid: "cn-hangzhou-i"  # 备可用区
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-backend-label: "app=nginx,env=prod"  # 后端筛选标签
spec:
  type: LoadBalancer
  ports:
  - port: 443
    targetPort: 80
    protocol: TCP
  selector:
    app: nginx
```

### ALB Ingress Controller配置

```yaml
# ALB IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: alb
spec:
  controller: ingress.k8s.alibabacloud/alb

---
# ALB Ingress配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    alb.ingress.kubernetes.io/address-type: "internet"  # internet/intranet
    alb.ingress.kubernetes.io/vswitch-ids: "vsw-xxx,vsw-yyy"  # 多可用区
    
    # === 健康检查 ===
    alb.ingress.kubernetes.io/healthcheck-enabled: "true"
    alb.ingress.kubernetes.io/healthcheck-path: "/health"
    alb.ingress.kubernetes.io/healthcheck-protocol: "HTTP"
    alb.ingress.kubernetes.io/healthcheck-method: "HEAD"
    alb.ingress.kubernetes.io/healthcheck-httpcode: "http_2xx,http_3xx"
    alb.ingress.kubernetes.io/healthcheck-timeout-seconds: "5"
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "2"
    alb.ingress.kubernetes.io/healthy-threshold-count: "3"
    alb.ingress.kubernetes.io/unhealthy-threshold-count: "3"
    
    # === SSL配置 ===
    alb.ingress.kubernetes.io/ssl-redirect: "true"  # HTTP->HTTPS
    alb.ingress.kubernetes.io/listen-ports: '[{"HTTP": 80},{"HTTPS": 443}]'
    alb.ingress.kubernetes.io/certificate-arn: "cert-arn"
    
    # === 灰度发布 ===
    alb.ingress.kubernetes.io/canary: "true"
    alb.ingress.kubernetes.io/canary-by-header: "X-Canary"
    alb.ingress.kubernetes.io/canary-by-header-value: "true"
    alb.ingress.kubernetes.io/canary-weight: "10"  # 10%流量
    
    # === 限流 ===
    alb.ingress.kubernetes.io/traffic-limit-qps: "1000"
    
    # === CORS ===
    alb.ingress.kubernetes.io/enable-cors: "true"
    alb.ingress.kubernetes.io/cors-allow-origin: "*"
    alb.ingress.kubernetes.io/cors-allow-methods: "GET,POST,PUT,DELETE"
    alb.ingress.kubernetes.io/cors-allow-headers: "DNT,X-CustomHeader,Keep-Alive"
    
    # === 后端协议 ===
    alb.ingress.kubernetes.io/backend-protocol: "HTTP"  # HTTP/HTTPS/gRPC
spec:
  ingressClassName: alb
  tls:
  - hosts:
    - app.example.com
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

---

## 可观测性集成

### ARMS Prometheus完整配置

```bash
# 安装ARMS Prometheus组件
# 通过ACK控制台"应用"-"ARMS Prometheus"一键安装

# 或通过Helm安装
helm install arms-prometheus \
  --namespace arms-prom \
  --create-namespace \
  --set cluster_id=<cluster-id> \
  --set uid=<uid> \
  --set region_id=cn-hangzhou \
  ack-arms-prometheus
```

```yaml
# ServiceMonitor自动发现
apiVersion: v1
kind: Service
metadata:
  name: app-metrics
  labels:
    app: myapp
spec:
  ports:
  - name: metrics
    port: 9090
  selector:
    app: myapp

---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-monitor
  labels:
    release: arms-prometheus
spec:
  selector:
    matchLabels:
      app: myapp
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
```

### SLS日志完整配置

```yaml
# Logtail DaemonSet自动采集
apiVersion: log.alibabacloud.com/v1alpha1
kind: AliyunLogConfig
metadata:
  name: app-logs
  namespace: kube-system
spec:
  project: k8s-log-<cluster-id>
  logstore: app-stdout
  shardCount: 2
  lifeCycle: 30  # 保留30天
  logtailConfig:
    inputType: plugin
    configName: app-stdout
    inputDetail:
      plugin:
        inputs:
        - type: service_docker_stdout
          detail:
            Stdout: true
            Stderr: true
            IncludeLabel:
              io.kubernetes.container.name: "app"
            ExcludeLabel:
              io.kubernetes.container.name: "istio-proxy"
        processors:
        - type: processor_json  # JSON日志解析
          detail:
            SourceKey: content
            KeepSource: false
            ExpandDepth: 1
            ExpandConnector: ""
    outputDetail:
      endpoint: cn-hangzhou.log.aliyuncs.com
      logstoreName: app-stdout
```

---

## 安全集成

### KMS密钥加密Secret

```yaml
# 集群创建时启用KMS Secret加密
# ACK自动使用KMS加密etcd中的Secret

# 验证Secret已加密
kubectl get secrets -o yaml | grep "encryptionConfig"
```

### 安全巡检配置

```bash
# ACK Pro版自动启用安全巡检
# 巡检项目:
# - Workload安全配置检查
# - RBAC权限配置检查
# - 网络策略配置检查
# - 镜像漏洞扫描
# - 合规性检查(等保2.0)

# 查看巡检报告
# ACK控制台 -> 安全 -> 安全巡检
```

---

## 弹性伸缩

### 节点自动扩缩容配置

```yaml
# 节点池弹性配置
节点池配置:
  自动扩缩容: 启用
  最小节点数: 3
  最大节点数: 100
  扩容策略:
    - 优先级: 按资源请求扩容
    - 触发条件: Pod Pending超过30秒
    - 扩容步长: 按需计算
  缩容策略:
    - 节点空闲时间: 10分钟
    - 节点利用率阈值: <50%
    - 保护节点: 带有特定标签的节点
  实例类型:
    - ecs.c7.2xlarge (CPU优化)
    - ecs.g7.2xlarge (通用型)
  Spot实例: 启用
  Spot比例: 50%
```

### Virtual Node (ECI)配置

```bash
# 安装Virtual Node组件
# ACK控制台 -> 应用 -> Virtual Node

# 创建虚拟节点
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: virtual-node-affinity-configuration
  namespace: kube-system
data:
  affinity: |
    nodeAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        preference:
          matchExpressions:
          - key: type
            operator: In
            values:
            - virtual-kubelet
EOF
```

```yaml
# Pod调度到ECI
apiVersion: v1
kind: Pod
metadata:
  name: eci-pod
spec:
  nodeSelector:
    type: virtual-kubelet
  containers:
  - name: app
    image: nginx
    resources:
      limits:
        cpu: "2"
        memory: "4Gi"
```

---

## 成本优化

### Spot实例策略

| 实例类型 | 折扣 | 适用场景 | 回收风险 |
|---------|-----|---------|---------|
| **抢占式实例** | 70-90% | 无状态应用，批处理 | 中(5分钟通知) |
| **按量付费** | 0% | 弹性需求 | 无 |
| **包年包月** | 15-60% | 稳定负载 | 无 |
| **节省计划** | 20-50% | 承诺使用量 | 无 |

### 成本优化配置示例

```yaml
# 节点池混合配置
节点池1(稳定业务):
  付费类型: 包年包月
  数量: 10台 ecs.c7.2xlarge
  用途: 核心服务
  
节点池2(弹性业务):
  付费类型: 按量付费
  自动扩缩: 3-20台
  实例类型: ecs.c7.2xlarge
  用途: 应对流量高峰
  
节点池3(批处理):
  付费类型: 抢占式实例
  Spot比例: 100%
  自动扩缩: 0-50台
  实例类型: ecs.c7.4xlarge
  用途: 离线计算
```

### 成本监控

```yaml
# ARMS监控成本指标
指标类型:
  - 节点成本: 按实例类型统计
  - 存储成本: 云盘/NAS/OSS
  - 网络成本: 公网流量/SLB
  - 监控成本: ARMS/SLS

告警规则:
  - 月成本超预算80%
  - 单日成本环比增长50%
  - 闲置资源(利用率<20%持续7天)
```

---

## 最佳实践

### 架构师视角: ACK集群设计模板

```yaml
集群规划:
  版本选择: ACK Pro版 + v1.30+
  节点规格:
    - Master: 托管(Pro版)
    - Worker: 3个节点池
      - 稳定池: 10台 ecs.c7.2xlarge (包年包月)
      - 弹性池: 3-20台 ecs.c7.2xlarge (按量)
      - Spot池: 0-50台 ecs.c7.4xlarge (Spot)
  网络:
    - CNI: Terway ENI多IP模式
    - Pod网段: 172.20.0.0/16 (65536个IP)
    - Service网段: 172.21.0.0/16
  存储:
    - 系统盘: 120GB ESSD PL0
    - 数据盘: 按需 ESSD PL1/PL2
  安全:
    - Secret加密: KMS
    - NetworkPolicy: 启用
    - 安全组: 最小权限
    - 审计日志: 180天保留

组件配置:
  监控: ARMS Prometheus
  日志: SLS
  Ingress: ALB Ingress Controller
  镜像: ACR企业版标准版
  证书: cert-manager + Let's Encrypt
  GitOps: ArgoCD

成本预算(月):
  - 集群管理费: 345元 (Pro版)
  - 节点成本: 60,000元 (平均30台)
  - 存储成本: 5,000元
  - 网络成本: 3,000元
  - 监控日志: 500元 (ARMS免费额度)
  总计: 68,845元
```

### 产品经理视角: ACK功能检查清单

```markdown
## 功能需求评估

### 基础设施
- [ ] 集群版本: Pro版 vs 托管版
- [ ] 节点规格: CPU/内存/网络
- [ ] 可用区: 单可用区 vs 多可用区
- [ ] VPC网络: 网段规划

### 应用交付
- [ ] 镜像仓库: ACR版本选择
- [ ] CI/CD: 云效 vs ArgoCD
- [ ] 灰度发布: ALB金丝雀 vs Istio
- [ ] 域名证书: cert-manager vs 手动

### 可观测性
- [ ] 监控: ARMS vs 自建Prometheus
- [ ] 日志: SLS vs ELK
- [ ] 链路追踪: ARMS vs Jaeger
- [ ] 告警: 钉钉/短信/电话

### 安全合规
- [ ] Secret加密: KMS
- [ ] 镜像扫描: ACR扫描
- [ ] 网络隔离: NetworkPolicy
- [ ] 审计日志: 180天保留
- [ ] 等保认证: 是否需要

### 成本优化
- [ ] Spot实例比例
- [ ] 节省计划
- [ ] 资源利用率目标
- [ ] 成本预算

### 容灾备份
- [ ] 多可用区部署
- [ ] 异地容灾
- [ ] 备份策略
- [ ] RTO/RPO目标
```

### 运维视角: ACK日常运维检查清单

```bash
#!/bin/bash
# ACK集群健康检查脚本

echo "=== 1. 集群状态检查 ==="
kubectl get nodes
kubectl get componentstatuses

echo "=== 2. 核心组件检查 ==="
kubectl get pods -n kube-system
kubectl get pods -n kube-system | grep -v Running

echo "=== 3. 资源使用检查 ==="
kubectl top nodes
kubectl top pods -A --sort-by=cpu | head -20

echo "=== 4. PV状态检查 ==="
kubectl get pv | grep -v Bound

echo "=== 5. 证书有效期检查 ==="
kubectl get certificates -A

echo "=== 6. HPA状态检查 ==="
kubectl get hpa -A

echo "=== 7. Ingress状态检查 ==="
kubectl get ingress -A

echo "=== 8. 事件检查(最近1小时) ==="
kubectl get events -A --sort-by='.lastTimestamp' | grep -v Normal | tail -50

echo "=== 9. Pod重启检查 ==="
kubectl get pods -A -o json | jq -r '.items[] | select(.status.containerStatuses[]?.restartCount > 5) | "\(.metadata.namespace)/\(.metadata.name): \(.status.containerStatuses[].restartCount)"'

echo "=== 10. 成本提醒 ==="
# 通过ARMS API查询本月成本
# aliyun arms QueryMetricByPage ...
```

---

**ACK最佳实践**: 使用Pro版，配置弹性伸缩，集成ARMS监控，启用安全加固，定期成本优化

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)