# Day 2: ACK SDK & API

> **学习时间**: 4-5 小时 | **主题**: ACK SDK 使用与 API 调用方式

---

## 今日目标

- [ ] 掌握 ACK OpenAPI 核心接口
- [ ] 能够使用 aliyun CLI 调用 ACK API
- [ ] 理解 SDK 认证方式 (AK/SK、STS Token、RAM 角色)
- [ ] 能够通过 API 完成集群信息查询

---

## 理论学习 (2h)

### 必读文档

1. **ACK OpenAPI 概览**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: API 接口分类、调用方式、签名机制

2. **ACK RAM 授权**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/243-ack-ram-authorization.md`
   - 重点: RAM 策略与 ACK API 权限映射

### 阅读要点

- ACK API 基于 ROA 风格 (RESTful)
- 认证方式: AccessKey (AK/SK)、STS Token、RAM 角色扮演
- 核心 API 分类:
  - 集群管理: CreateCluster, DeleteCluster, DescribeClusterDetail
  - 节点管理: DescribeClusterNodes, RemoveClusterNodes
  - 节点池: CreateClusterNodePool, DescribeClusterNodePools
  - 组件管理: DescribeClusterAddonsVersion, InstallClusterAddons
  - kubeconfig: DescribeClusterUserKubeconfig

---

## 实践任务 (2.5h)

### 任务 1: aliyun CLI 配置与基础 API 调用 (45min)

```bash
# 配置 aliyun CLI (如果尚未配置)
aliyun configure set \
  --profile default \
  --mode AK \
  --access-key-id <your-ak> \
  --access-key-secret <your-sk> \
  --region cn-hangzhou

# 查看集群列表
aliyun cs GET /api/v1/clusters

# 查看集群详情
aliyun cs GET /clusters/<cluster_id>

# 查看集群 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config

# 查看集群日志
aliyun cs GET /clusters/<cluster_id>/logs
```

### 任务 2: 节点与节点池 API (45min)

```bash
# 查看集群节点列表
aliyun cs GET /clusters/<cluster_id>/nodes

# 查看节点池列表
aliyun cs GET /clusters/<cluster_id>/nodepools

# 查看节点池详情
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>

# 关注返回字段:
# - scaling_group: 弹性伸缩组配置
# - kubernetes_config: K8S 相关配置
# - auto_scaling: 自动伸缩配置
# - management: 托管配置
```

### 任务 3: 组件管理 API (30min)

```bash
# 查看集群已安装组件
aliyun cs GET /clusters/<cluster_id>/components

# 查看可用组件版本
aliyun cs GET /clusters/<cluster_id>/components/upgradestatus

# 常见核心组件:
# - coredns: DNS 服务
# - metrics-server: 指标采集
# - cloud-controller-manager: 云资源控制器
# - csi-plugin / csi-provisioner: 存储插件
# - terway-eniip / flannel: 网络插件
```

### 任务 4: SDK 调用实践 (30min)

```python
# Python SDK 示例
from alibabacloud_cs20151215.client import Client
from alibabacloud_tea_openapi.models import Config

# 初始化客户端
config = Config(
    access_key_id='<your-ak>',
    access_key_secret='<your-sk>',
    region_id='cn-hangzhou'
)
client = Client(config)

# 查看集群列表
response = client.describe_clusters_v1()
for cluster in response.body.clusters:
    print(f"集群: {cluster.name}, 状态: {cluster.state}, 版本: {cluster.current_version}")

# 查看集群详情
detail = client.describe_cluster_detail('<cluster_id>')
print(f"集群类型: {detail.body.cluster_type}")
print(f"节点数量: {detail.body.size}")
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **ACK API 的认证方式有哪几种？各自适合什么场景？**
   - 提示: AK/SK 适合服务端，STS 适合临时授权，RAM 角色适合 ECS 上的应用

2. **如何通过 API 获取集群的 kubeconfig？获取后如何使用？**
   - 提示: DescribeClusterUserKubeconfig 接口，保存到 ~/.kube/config

3. **查看集群节点池信息时，哪些字段最重要？为什么？**
   - 提示: scaling_group 决定节点规格，auto_scaling 决定弹性能力

---

## 今日检验

- [ ] 能使用 aliyun CLI 查询集群、节点、组件信息
- [ ] 理解 ACK API 的认证和签名机制
- [ ] 能说出至少 5 个核心 ACK API 接口的用途
- [ ] 能使用 SDK (Python/Java) 编写简单的集群查询脚本

---

## 核心概念总结

| 概念 | 说明 | 生产注意事项 |
|------|------|--------------|
| OpenAPI | ACK 对外提供的 RESTful API | 注意 API 频率限制 |
| AK/SK | 永久访问凭证 | 生产环境避免硬编码，使用 RAM 角色 |
| STS Token | 临时安全令牌 | 有过期时间，适合跨账号场景 |
| aliyun CLI | 命令行调用工具 | 日常运维和脚本自动化首选 |
| SDK | 编程语言 SDK | 适合自动化系统和工具开发 |

---

## 明日预告

Day 3 将学习 ACK/ACR 控制台操作，熟悉界面功能入口和核心操作流程。
