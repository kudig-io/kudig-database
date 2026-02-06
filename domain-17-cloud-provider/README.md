# Domain-17: 云厂商Kubernetes服务

> **文档数量**: 9 篇 | **最后更新**: 2026-02 | **适用版本**: 各云厂商K8s服务

---

## 概述

云厂商Kubernetes服务域全面覆盖主流云服务商的托管Kubernetes服务，包括阿里云ACK、AWS EKS、Google GKE、Azure AKS等。提供各云厂商特色功能、最佳实践和迁移指南。

**核心价值**：
- ☁️ **云原生托管**：各云厂商托管K8s服务特性对比
- 🔄 **混合云部署**：跨云厂商部署策略和最佳实践
- 💰 **成本优化**：云厂商定价模型和成本控制
- 🔧 **运维管理**：云厂商专属运维工具和监控

---

## 文档目录

### 主流云厂商 (01-08)

| # | 文档 | 关键内容 | 云厂商 |
|:---:|:---|:---|:---|
| 01 | [阿里云ACK](./01-alicloud-ack/alicloud-ack-overview.md) | ACK Pro版、Serverless、专有版特性对比 | 阿里云 |
| 02 | [AWS EKS](./03-aws-eks/aws-eks-overview.md) | EKS架构、Fargate、Node Groups管理 | AWS |
| 03 | [Google GKE](./05-google-cloud-gke/google-cloud-gke-overview.md) | GKE Autopilot、Standard模式、Anthos集成 | Google |
| 04 | [Azure AKS](./04-azure-aks/azure-aks-overview.md) | AKS架构、Virtual Nodes、Azure Arc集成 | Azure |
| 05 | [腾讯云TKE](./06-tencent-tke/tencent-tke-overview.md) | TKE架构、Super Node、Service Mesh集成 | 腾讯云 |
| 06 | [华为云CCE](./07-huawei-cce/huawei-cce-overview.md) | CCE Turbo、CCI Serverless、混合云部署 | 华为云 |
| 07 | [天翼云TKE](./08-ctyun-tke/ctyun-tke-overview.md) | 电信级K8s、5G融合、安全合规 | 天翼云 |
| 08 | [移动云CKE](./09-ecloud-cke/ecloud-cke-overview.md) | 运营商网络优势、CDN集成 | 移动云 |
| 09 | [联通云UK8S](./12-ucloud-uk8s/ucloud-uk8s-overview.md) | 5G网络切片、边缘计算优化 | 联通云 |
| 10 | [IBM IKS](./10-ibm-iks/ibm-iks-overview.md) | 企业级安全、混合云部署 | IBM |
| 11 | [Oracle OKE](./11-oracle-oke/oracle-oke-overview.md) | 裸金属节点、企业级安全 | Oracle |
| 12 | [UCloud UK8s](./12-ucloud-uk8s/ucloud-uk8s-overview.md) | 高性价比、部署实践 | UCloud |
| 13 | [字节跳动VEK](./13-volcengine-vek/volcengine-vek-overview.md) | 字节级优化、AI场景优化 | 字节跳动 |

### 专有云解决方案 (14)

| # | 文档 | 关键内容 | 专有云 |
|:---:|:---|:---|:---|
| 14 | [阿里云专有云](./02-alicloud-apsara-ack/alicloud-apsara-ack-overview.md) | Apsara Stack、专有云K8s、混合云架构 | 阿里云 |

---

## 云厂商K8s服务对比

| 云厂商 | 服务名称 | Serverless支持 | 多集群管理 | 成本优势 | 特色优势 |
|:------:|:--------|:--------------|:-----------|:---------|:---------|
| 阿里云 | ACK      | ✅ ACK Serverless | ✅ 多集群联邦 | 高性价比 | 金融级安全 |
| AWS    | EKS      | ✅ Fargate        | ✅ EKS Anywhere | 生态丰富 | 企业级集成 |
| Google | GKE      | ✅ Autopilot      | ✅ Anthos      | 性能优异 | AI/ML优化 |
| Azure  | AKS      | ✅ Virtual Nodes  | ✅ Azure Arc   | 企业集成 | 混合云强 |
| 腾讯云 | TKE      | ✅ Super Node     | ✅ TKE Mesh    | 游戏优化 | 社交场景 |
| 华为云 | CCE      | ✅ CCI Serverless | ✅ 专有云      | 信创适配 | 国产化强 |
| 天翼云 | TKE      | ✅ Serverless     | ✅ 混合云      | 电信优势 | 5G融合 |
| 移动云 | CKE      | ✅ Serverless     | ✅ CDN集成     | 运营商优 | 网络加速 |
| 联通云 | UK8S     | ✅ Serverless     | ✅ 5G切片      | 边缘计算 | 低延迟 |
| IBM    | IKS      | ✅ Serverless     | ✅ 多云管理    | 企业安全 | 合规性强 |
| Oracle | OKE      | ✅ Serverless     | ✅ 裸金属      | 性能优异 | 数据库强 |
| UCloud | UK8s     | ✅ Serverless     | ✅ 高性价比    | 成本优势 | 性价比高 |
| 字节跳动| VEK     | ✅ Serverless     | ✅ 字节优化    | AI原生   | 推荐算法 |

---

## 学习路径建议

### 🎯 单云深度路径
**选择一个云厂商 → 深入学习其K8s服务特性**

### 🔧 多云比较路径  
**01 → 02 → 03 → 04**  
横向对比各云厂商托管K8s服务差异

### 🏢 企业选型路径
**结合业务需求 → 成本分析 → 功能匹配 → 迁移规划**

### 🚀 混合云路径
**09(专有云) → 多公有云集成方案**

---

## 云厂商特色功能

```
阿里云 ACK
    ├── ACK Pro版：企业级增强功能
    ├── Serverless：按需付费、免运维
    └── 专有版：金融级安全、混合云部署
    
AWS EKS  
    ├── Fargate：无服务器容器
    ├── Node Groups：灵活节点管理
    └── EKS Anywhere：本地部署
    
Google GKE
    ├── Autopilot：全自动运维
    ├── Standard：完全控制权
    └── Anthos：混合多云
    
Azure AKS
    ├── Virtual Nodes：虚拟节点扩展
    ├── Azure Arc：混合云管理
    └── 企业集成：Active Directory
    
腾讯云 TKE
    ├── Super Node：无服务器计算
    ├── Service Mesh：微服务治理
    └── 游戏优化：高并发支持
    
华为云 CCE
    ├── CCI Serverless：极速弹性
    ├── 专有云：信创适配
    └── 混合云：跨云部署
    
天翼云 TKE
    ├── 电信级：SLA保障
    ├── 5G融合：网络切片
    └── 安全合规：等保认证
    
移动云 CKE
    ├── CDN集成：网络加速
    ├── 运营商优：全国覆盖
    └── 专属宿主机：物理隔离
    
联通云 UK8S
    ├── 5G切片：低延迟
    ├── 边缘计算：就近部署
    └── 网络优化：QoS保障
    
IBM IKS
    ├── 企业安全：合规认证
    ├── 混合云：多云管理
    └── 金融级：高可用
    
Oracle OKE
    ├── 裸金属：性能优异
    ├── 企业级：安全可靠
    └── 数据库强：深度集成
    
UCloud UK8s
    ├── 高性价比：成本优势
    ├── 部署实践：简单易用
    └── 灵活配置：按需选择
    
字节跳动 VEK
    ├── 字节优化：极致性能
    ├── AI原生：算法优化
    └── 推荐场景：业务适配
```

---

## 相关领域

- **[Domain-1: 架构基础](../domain-1-architecture-fundamentals)** - K8s基础架构
- **[Domain-3: 控制平面](../domain-3-control-plane)** - 控制平面配置
- **[Domain-9: 平台运维](../domain-9-platform-ops)** - 多云运维管理

---

**维护者**: Kusheet Cloud Team | **许可证**: MIT