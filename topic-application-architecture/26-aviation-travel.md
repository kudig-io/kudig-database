# 航空出行架构设计 — 阿里云视角

> **适用版本**: Kubernetes v1.29 - v1.33 | **最后更新**: 2026-04-24
> **作者**: 阿里云解决方案架构师 | **标签**: `#航空` `#机票` `#GDS` `#收益管理` `#阿里云`

---

## 目录

1. [行业背景](#1-行业背景)
2. [业务架构](#2-业务架构)
3. [技术架构](#3-技术架构)
4. [核心数据流](#4-核心数据流)
5. [安全与合规](#5-安全与合规)
6. [可观测性](#6-可观测性)
7. [阿里云组件映射](#7-阿里云组件映射)
8. [生产检查清单](#8-生产检查清单)

---

## 1. 行业背景

### 1.1 业务特点

航空出行系统面临高并发查询、复杂运价计算、实时库存同步等挑战：

| 挑战 | 说明 | 架构影响 |
|:---|:---|:---|
| 高并发查询 | 航班查询 QPS 10万+ | 多级缓存 + 异步化 |
| 动态运价 | 舱位/日期/航线多维定价 | 规则引擎 + 缓存预热 |
| 库存一致性 | 超售控制 + 实时同步 | 分布式事务 + 锁机制 |
| 退改签复杂 | 多规则组合计算 | 工作流引擎 |
| 多源数据 | GDS/航司直联/OTA | 数据聚合层 |

### 1.2 核心场景

- **航班搜索**: 多维度航班查询与推荐
- **运价计算**: 实时舱位定价与税费计算
- **订单出票**: 座位锁定 + 支付 + 出票流程
- **收益管理**: 动态定价与超售优化
- **航班动态**: 延误/取消实时通知

---

## 2. 业务架构

### 2.1 航空出行全景架构

```mermaid
graph TB
    subgraph 渠道层
        C1[航司官网]
        C2[OTA 平台]
        C3[代理人系统]
        C4[企业差旅]
    end

    subgraph 核心业务层
        B1[航班搜索服务]
        B2[运价引擎]
        B3[订单中心]
        B4[出票服务]
        B5[退改签服务]
    end

    subgraph 数据源层
        D1[GDS 全球分销]
        D2[航司 NDC]
        D3[航司直联]
        D4[机场数据]
    end

    subgraph 支撑层
        S1[收益管理系统]
        S2[会员系统]
        S3[通知中心]
        S4[结算中心]
    end

    C1 & C2 & C3 & C4 --> B1 & B2 & B3 & B4 & B5
    B1 --> D1 & D2 & D3 & D4
    B2 --> D1 & D2
    B3 --> S2 & S4
    B4 --> S3
    B5 --> S4
    S1 --> B2
```

### 2.2 航班搜索时序

```mermaid
sequenceDiagram
    participant USER as 旅客
    participant SEARCH as 搜索服务
    participant CACHE as 多级缓存
    participant FARE as 运价引擎
    participant AGG as 数据聚合层
    participant GDS as GDS/航司

    USER->>SEARCH: 查询 PEK-SHA 2026-05-01
    SEARCH->>CACHE: 读取热点缓存
    alt 缓存命中
        CACHE-->>SEARCH: 返回缓存结果
    else 缓存未命中
        SEARCH->>AGG: 异步并发查询
        AGG->>GDS: 请求航班数据
        GDS-->>AGG: 返回航班列表
        AGG-->>SEARCH: 聚合结果
        SEARCH->>CACHE: 写入缓存 (TTL 60s)
    end
    SEARCH->>FARE: 批量计算运价
    FARE-->>SEARCH: 返回各舱位价格
    SEARCH->>SEARCH: 排序与推荐
    SEARCH-->>USER: 返回航班列表
```

---

## 3. 技术架构

### 3.1 K8s 部署

```yaml
# 航班搜索服务 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: flight-search
  namespace: aviation
spec:
  replicas: 10
  selector:
    matchLabels:
      app: flight-search
  template:
    metadata:
      labels:
        app: flight-search
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values: [flight-search]
                topologyKey: topology.kubernetes.io/zone
      containers:
        - name: search
          image: registry.cn-hangzhou.aliyuncs.com/aviation/flight-search:v3.1.0
          ports:
            - containerPort: 8080
          env:
            - name: REDIS_CLUSTER
              value: "redis-cluster:6379"
            - name: CACHE_TTL_SECONDS
              value: "60"
            - name: GDS_TIMEOUT_MS
              value: "3000"
          resources:
            requests:
              memory: "2Gi"
              cpu: "1000m"
            limits:
              memory: "4Gi"
              cpu: "2000m"
```

```yaml
# 运价引擎 StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: fare-engine
  namespace: aviation
spec:
  serviceName: fare-engine
  replicas: 3
  selector:
    matchLabels:
      app: fare-engine
  template:
    metadata:
      labels:
        app: fare-engine
    spec:
      containers:
        - name: engine
          image: registry.cn-hangzhou.aliyuncs.com/aviation/fare-engine:v2.5.0
          ports:
            - containerPort: 8080
          env:
            - name: RULES_REFRESH_INTERVAL
              value: "300"
          resources:
            requests:
              memory: "4Gi"
              cpu: "2000m"
            limits:
              memory: "8Gi"
              cpu: "4000m"
```

---

## 4. 核心数据流

### 4.1 出票流程

```mermaid
flowchart TD
    A[旅客下单] --> B[座位预占]
    B --> C[价格确认]
    C --> D[支付]
    D --> E{支付成功?}
    E -->|是| F[出票请求]
    E -->|否| G[释放座位]
    F --> H[GDS/航司出票]
    H --> I{出票成功?}
    I -->|是| J[发送电子客票]
    I -->|否| K[自动重试]
    K --> H
    J --> L[行程单推送]
```

---

## 5. 安全与合规

- **PCI-DSS**: 支付数据合规
- **IATA 标准**: NDC/ONE Order 标准对接
- **数据安全**: 旅客 PNR 数据加密

---

## 6. 可观测性

- **搜索响应**: P99 < 200ms
- **出票成功率**: > 99.9%
- **系统可用性**: 99.99%

---

## 7. 阿里云组件映射

| 功能域 | **阿里云云原生方案** |
|:---|:---|
| 容器平台 | **ACK Pro** |
| 缓存 | **Redis 企业版** |
| 数据库 | **PolarDB MySQL** |
| 消息队列 | **RocketMQ** |
| 搜索 | **OpenSearch** |
| 可观测性 | **ARMS + SLS** |

---

## 8. 生产检查清单

- [ ] GDS 接口连通性验证
- [ ] 运价缓存一致性校验
- [ ] 超售控制策略验证
- [ ] 退改签规则端到端测试
- [ ] 航班动态通知延迟 < 30s

---

**维护者**: 阿里云解决方案架构师团队 | **许可证**: MIT
