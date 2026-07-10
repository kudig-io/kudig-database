---
title: 电商系统 Kubernetes 生产架构设计 (domain-20-application-patterns)
description: 'title: 电商系统 Kubernetes 生产架构设计'
summary: 'title: 电商系统 Kubernetes 生产架构设计'
category: general
tags:
- architecture
- best-practice
- prometheus
- grafana
- jaeger
- istio
- envoy
- coredns
- harbor
- minio
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 电商系统 Kubernetes 生产架构设计 是什么
- 如何 电商系统 Kubernetes 生产架构设计
- Kubernetes 20 application patterns 最佳实践
trigger_keywords:
- 电商系统
- Kubernetes
- 生产架构设计
- application
- patterns
prerequisites:
- kubectl-basics
- prometheus-basics
- service-mesh-basics
- monitoring-basics
- kafka-basics
- redis-basics
- mysql-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 电商系统 [[Kubernetes|Kubernetes]] 生产架构设计
description: '# 电商系统 Kubernetes 生产架构设计'
category: application-architecture
tags:
- k8s
- architecture
- industry
- [[Prometheus|prometheus]]
- grafana
- [[Jaeger|jaeger]]
- [[Istio|istio]]
- envoy
- minio
- redis
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- 电商架构师
- 后端开发 TL
- SRE
- 云原生工程师
estimated_read_time: 5min
intent_queries:
- 电商系统 K8s生产架构 微服务拆分
- 电商订单链路 Kubernetes StatefulSet
- 秒杀系统 Redis Lua 库存扣减 K8s
- 电商搜索 Elasticsearch K8s部署
- 电商支付 PCI-DSS Kubernetes 安全
trigger_keywords:
- 电商架构
- 微服务
- 订单系统
- 库存扣减
- 秒杀
- Redis Cluster
- Elasticsearch
- StatefulSet
- HPA
- Karpenter
related_domains:
- domain-01-cluster-fundamentals
- domain-11-production-operations
- domain-03-networking-traffic
related_topics:
- 41-beauty-ecommerce
- 31-instant-retail
- 49-livestream-ecommerce
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 电商系统 Kubernetes 生产架构设计

> **适用场景**: B2C / B2B / O2O / 直播电商 / 跨境电商  
> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **目标读者**: 电商架构师、SRE、后端开发 TL

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、整体架构全景](#一整体架构全景)
- [二、微服务拆分架构](#二微服务拆分架构)
- [三、流量入口与网关架构](#三流量入口与网关架构)
- [四、订单核心链路架构](#四订单核心链路架构)
- [五、商品与搜索架构](#五商品与搜索架构)
- [六、支付与财务架构](#六支付与财务架构)
- [七、库存与供应链架构](#七库存与供应链架构)
- [八、营销与秒杀架构](#八营销与秒杀架构)
- [九、数据层架构](#九数据层架构)
- [十、K8s 部署架构](#十k8s-部署架构)
- [十一、高可用与灾备](#十一高可用与灾备)

---

<!-- chunk: 一、整体架构全景 -->## 一、整体架构全景

```mermaid
flowchart TB
    subgraph Edge["边缘加速层"]
        CDN["CDN / DCDN<br/>静态资源 + 动态加速"]
        WAF["WAF / Bot 管理<br/>防爬虫/防羊毛"]
        DDoS["DDoS 防护"]
    end

    subgraph Gateway["网关接入层"]
        DNS["智能 DNS / GSLB"]
        LB["L4/L7 负载均衡"]
        API_GW["API Gateway<br/>Envoy / APISIX / Higress"]
        BFF["BFF 层<br/>聚合/适配/鉴权"]
    end

    subgraph Biz["业务服务层 (微服务)"]
        USER["用户服务"]
        PRODUCT["商品服务"]
        ORDER["订单服务"]
        CART["购物车服务"]
        PAYMENT["支付服务"]
        INVENTORY["库存服务"]
        PROMO["营销服务"]
        SEARCH["搜索服务"]
        RECOMMEND["推荐服务"]
        LOGISTICS["物流服务"]
    end

    subgraph Platform["平台服务层"]
        AUTH["统一认证中心"]
        MSG["消息中心"]
        FILE["文件服务"]
        CONFIG["配置中心"]
        SCHEDULE["调度中心"]
    end

    subgraph Data["数据层"]
        MYSQL["MySQL Cluster<br/>订单/用户/商品"]
        REDIS["Redis Cluster<br/>缓存/限流/会话"]
        ES["Elasticsearch<br/>商品搜索"]
        KAFKA["Kafka<br/>事件总线"]
        MONGO["MongoDB<br/>商品详情/日志"]
        OSS["对象存储<br/>图片/视频"]
    end

    subgraph Observability["可观测性"]
        PROM["Prometheus"]
        GRAF["Grafana"]
        JAEG["Jaeger"]
        LOKI["Loki"]
    end

    CDN --> WAF --> DDoS --> DNS --> LB --> API_GW --> BFF
    BFF --> USER & PRODUCT & ORDER & CART & PAYMENT & INVENTORY & PROMO & SEARCH & RECOMMEND & LOGISTICS
    USER --> AUTH
    ORDER --> MSG & SCHEDULE
    PRODUCT --> FILE
    PAYMENT --> CONFIG
    Biz --> Data
    Biz --> Observability

    style Biz fill:#e3f2fd
    style Data fill:#e8f5e9
    style Gateway fill:#fff8e1
    style Observability fill:#f3e5f5
```

---

<!-- chunk: 二、微服务拆分架构 -->## 二、微服务拆分架构

```mermaid
flowchart LR
    subgraph Client["客户端"]
        APP["App / H5 / 小程序"]
        ADMIN["运营后台"]
    end

    subgraph Gateway["API Gateway"]
        RATE["限流 10w QPS"]
        AUTH["JWT 鉴权"]
        ROUTE["动态路由"]
    end

    subgraph Core["核心域"]
        ORDER["订单服务<br/> StatefulSet"]
        PAY["支付服务<br/> 加密隔离"]
        INV["库存服务<br/> Redis + DB"]
    end

    subgraph Support["支撑域"]
        USER["用户服务"]
        PROD["商品服务"]
        CART["购物车服务"]
        SEARCH["搜索服务"]
        REC["推荐服务"]
    end

    subgraph Platform["平台域"]
        MSG["消息中心"]
        CFG["配置中心"]
        LOG["日志中心"]
        MON["监控中心"]
    end

    APP --> Gateway --> Core & Support
    ADMIN --> Gateway --> Support
    Core --> Platform
    Support --> Platform
    ORDER --> PAY & INV
    CART --> INV
    SEARCH --> PROD
    REC --> USER & PROD

    style Core fill:#ffccbc
    style Platform fill:#e8f5e9
```

## 领域驱动设计 (DDD) 映射

| 领域 | 服务 | K8s 工作负载 | 数据库 | 关键特性 |
|:---|:---|:---|:---|:---|
| **核心域** | 订单、支付、库存 | StatefulSet / Deployment | MySQL + Redis | 强一致性、事务 |
| **支撑域** | 用户、商品、购物车 | Deployment | MySQL + ES | 最终一致性 |
| **通用域** | 消息、文件、配置 | Deployment | MongoDB + OSS | 高可用 |
| **平台域** | 监控、日志、链路 | DaemonSet / Deployment | Prometheus + Loki | 可观测性 |

---

<!-- chunk: 三、流量入口与网关架构 -->## 三、流量入口与网关架构

```mermaid
flowchart TB
    subgraph Users["用户流量"]
        MOBILE["移动端 App"]
        WEB["Web 浏览器"]
        MINI["小程序"]
        OPEN["Open API<br/>第三方接入"]
    end

    subgraph Ingress["入口层"]
        DNS["GeoDNS<br/>就近调度"]
        LB["L7 Load Balancer"]
        RATE["全局限流<br/>令牌桶"]
    end

    subgraph Gateway["API Gateway"]
        SSL["TLS 终止<br/>mTLS"]
        AUTH["OAuth2 / JWT<br/>验签鉴权"]
        ROUTE["路由匹配<br/>路径/Header"]
        TRANSFORM["协议转换<br/>gRPC <> HTTP"]
        CIRCUIT["熔断降级<br/>Hystrix"]
        RETRY["重试策略<br/>指数退避"]
    end

    subgraph BFF["BFF 聚合层"]
        BFF_MOBILE["BFF-Mobile<br/>字段裁剪"]
        BFF_WEB["BFF-Web<br/>SSR 适配"]
        BFF_ADMIN["BFF-Admin<br/>权限校验"]
    end

    Users --> DNS --> LB --> RATE --> Gateway
    SSL --> AUTH --> ROUTE --> TRANSFORM --> CIRCUIT --> RETRY
    Gateway --> BFF_MOBILE & BFF_WEB & BFF_ADMIN
    BFF_MOBILE -->|gRPC| CoreSvc["核心业务服务"]
    BFF_WEB -->|HTTP| CoreSvc
    BFF_ADMIN -->|HTTP| CoreSvc

    style Gateway fill:#fff8e1
    style BFF fill:#e3f2fd
```

## Gateway 生产配置

```yaml
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: ecommerce-gateway
  namespace: ecommerce
spec:
  selector:
    istio: ingressgateway
  servers:
    - port:
        number: 443
        name: https
        protocol: HTTPS
      tls:
        mode: SIMPLE
        credentialName: ecommerce-tls-secret
      hosts:
        - "*.ecommerce.com"
        - "api.ecommerce.com"
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: ecommerce-routes
  namespace: ecommerce
spec:
  hosts:
    - api.ecommerce.com
  gateways:
    - ecommerce-gateway
  http:
    - matchers:
      - - uri=""
      - prefix="/api/v1/order"
      - route=""
      - - destination=""
      - host="order-service"
      - port=""
      - number="8080"
      - timeout="3s"
      - retries=""
      - attempts="3"
      - perTryTimeout="1s"
      - retryOn="gateway-error,connect-failure,refused-stream"
      - fault=""
      - delay=""
      - percentage=""
      - value="0.1"
      - fixedDelay="5s"
    - matchers:
      - - uri=""
      - prefix="/api/v1/payment"
      - route=""
      - - destination=""
      - host="payment-service"
      - port=""
      - number="8080"
      - timeout="10s"
```

---

<!-- chunk: 四、订单核心链路架构 -->## 四、订单核心链路架构

## 下单链路时序

```mermaid
sequenceDiagram
    participant User as 用户
    participant GW as API Gateway
    participant BFF as BFF 聚合
    participant Cart as 购物车服务
    participant Order as 订单服务
    participant Stock as 库存服务
    participant Pay as 支付服务
    participant MQ as Kafka
    participant DB as MySQL

    User->>GW: 提交订单
    GW->>BFF: 转发请求
    BFF->>Cart: 获取购物车数据
    Cart-->>BFF: 商品列表

    BFF->>Order: 创建订单
    Order->>Stock: 预占库存
    Stock-->>Order: 预占成功
    Order->>DB: 写入订单 (状态: 待支付)
    DB-->>Order: 订单创建成功
    Order-->>BFF: 返回订单信息
    BFF-->>GW: 返回
    GW-->>User: 订单创建成功，等待支付

    Order->>MQ: 发布 OrderCreated 事件
    MQ->>Pay: 消费事件
    User->>Pay: 发起支付
    Pay->>DB: 更新订单状态
    Pay->>MQ: 发布 PaymentSuccess 事件
    MQ->>Stock: 确认扣减库存
    MQ->>Logistics: 创建物流单
```

## 订单服务 K8s 部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: order-service
  namespace: ecommerce
spec:
  serviceName: order-service
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  template:
    metadata:
      labels:
        app: order-service
        version: v1.2.0
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values:
                      - order-service
              topologyKey: kubernetes.io/hostname
      containers:
        - name: order
          image: ecommerce/order-service:v1.2.0
          ports:
            - containerPort: 8080
              name: http
            - containerPort: 9090
              name: grpc
          env:
            - name: DB_HOST
              valueFrom:
                secretKeyRef:
                  name: order-db-secret
                  key: host
            - name: KAFKA_BROKERS
              value: "kafka-0.kafka:9092,kafka-1.kafka:9092"
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
          livenessProbe:
            httpGet:
              path: /health/live
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /health/ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 5
          volumeMounts:
            - name: order-logs
              mountPath: /app/logs
      volumes:
        - name: order-logs
          emptyDir: {}
---
apiVersion: v1
kind: Service
metadata:
  name: order-service
  namespace: ecommerce
  labels:
    app: order-service
spec:
  selector:
    app: order-service
  ports:
    - name: http
      port: 8080
      targetPort: 8080
    - name: grpc
      port: 9090
      targetPort: 9090
---
# HPA 配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: order-service-hpa
  namespace: ecommerce
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: StatefulSet
    name: order-service
  minReplicas: 3
  maxReplicas: 20
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
    - type: Resource
      resource:
        name: memory
        target:
          type: Utilization
          averageUtilization: 80
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Percent
          value: 100
          periodSeconds: 15
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
```

---

<!-- chunk: 五、商品与搜索架构 -->## 五、商品与搜索架构

```mermaid
flowchart TB
    subgraph Ingestion["数据摄入"]
        ADMIN["运营后台<br/>商品发布"]
        IMPORT["批量导入"]
        SYNC["异构同步<br/>ERP/WMS"]
    end

    subgraph Pipeline["处理流水线"]
        VALIDATE["数据校验"]
        ENRICH["数据增强<br/>分类/标签/SEO"]
        INDEX["索引构建"]
    end

    subgraph SearchLayer["搜索服务层"]
        ES_CLUSTER["Elasticsearch Cluster<br/>7 节点"]
        REDIS_CACHE["Redis<br/>热门搜索缓存"]
        QUERY_BUILDER["Query Builder<br/>DSL 生成"]
        RANKING["排序引擎<br/>相关性/销量/个性化"]
    end

    subgraph APIs["搜索 API"]
        FULLTEXT["全文搜索<br/>分词/高亮"]
        FACET["聚合筛选<br/>品牌/价格/分类"]
        SUGGEST["搜索建议<br/>前缀匹配"]
    end

    ADMIN --> VALIDATE
    IMPORT --> VALIDATE
    SYNC --> VALIDATE
    VALIDATE --> ENRICH --> INDEX --> ES_CLUSTER
    ES_CLUSTER --> QUERY_BUILDER --> RANKING
    RANKING --> FULLTEXT & FACET & SUGGEST
    REDIS_CACHE --> FULLTEXT

    style SearchLayer fill:#e3f2fd
    style Pipeline fill:#e8f5e9
```

## Elasticsearch K8s 部署

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: product-search
  namespace: ecommerce
spec:
  version: 8.12.0
  nodeSets:
    - name: master
      count: 3
      config:
        node.roles: ["master"]
      podTemplate:
        spec:
          containers:
            - name: elasticsearch
              resources:
                requests:
                  memory: 4Gi
                  cpu: "1"
                limits:
                  memory: 4Gi
    - name: data-hot
      count: 3
      config:
        node.roles: ["data_hot", "data_content", "ingest"]
      podTemplate:
        spec:
          containers:
            - name: elasticsearch
              resources:
                requests:
                  memory: 16Gi
                  cpu: "4"
                limits:
                  memory: 16Gi
          affinity:
            podAntiAffinity:
              requiredDuringSchedulingIgnoredDuringExecution:
                - labelSelector:
                    matchLabels:
                      elasticsearch.k8s.elastic.co/cluster-name: product-search
                  topologyKey: kubernetes.io/hostname
      volumeClaimTemplates:
        - metadata:
            name: elasticsearch-data
          spec:
            storageClassName: fast-ssd
            accessModes: ["ReadWriteOnce"]
            resources:
              requests:
                storage: 500Gi
```

---

<!-- chunk: 六、支付与财务架构 -->## 六、支付与财务架构

```mermaid
flowchart TB
    subgraph Security["安全隔离区"]
        subgraph PaymentZone["支付专区 (PCI-DSS)"]
            PAY_API["支付 API Gateway<br/>mTLS + WAF"]
            PAY_SVC["支付核心服务<br/>加密运算"]
            PAY_DB["支付数据库<br/>TDE 加密"]
            PAY_KMS["KMS<br/>密钥管理"]
            PAY_VAULT["Vault<br/>动态凭据"]
        end
    end

    subgraph Channels["支付渠道"]
        WECHAT["微信支付"]
        ALIPAY["支付宝"]
        UNION["银联"]
        CREDIT["信用卡<br/>Stripe/Adyen"]
    end

    subgraph Reconciliation["对账体系"]
        RECON["对账引擎"]
        LEDGER["总账系统"]
        SETTLE["结算系统"]
    end

    User["用户"] --> PAY_API --> PAY_SVC
    PAY_SVC --> PAY_DB
    PAY_SVC --> PAY_KMS
    PAY_SVC --> PAY_VAULT
    PAY_SVC --> WECHAT & ALIPAY & UNION & CREDIT
    PAY_SVC --> RECON --> LEDGER --> SETTLE

    style PaymentZone fill:#ffebee
    style Security fill:#fff3e0
```

## 支付服务安全部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-service
  namespace: ecommerce-payment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: payment-service
  template:
    metadata:
      labels:
        app: payment-service
        compliance-level: pci-dss
    spec:
      serviceAccountName: payment-sa
      securityContext:
        runAsNonRoot: true
        seccompProfile:
          type: RuntimeDefault
      nodeSelector:
        node-type: secure
      tolerations:
        - key: node-type
          operator: Equal
          value: secure
          effect: NoSchedule
      containers:
        - name: payment
          image: ecommerce/payment-service:v2.0.0
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            runAsUser: 1000
            capabilities:
              drop:
                - ALL
          ports:
            - containerPort: 8443
              name: https
          env:
            - name: HSM_ENABLED
              value: "true"
            - name: VAULT_ADDR
              value: "https://vault-internal:8200"
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
          volumeMounts:
            - name: tmp
              mountPath: /tmp
            - name: vault-token
              mountPath: /vault/secrets
              readOnly: true
      volumes:
        - name: tmp
          emptyDir: {}
        - name: vault-token
          csi:
            driver: secrets-store.csi.k8s.io
            readOnly: true
            volumeAttributes:
              secretProviderClass: vault-payment
```

---

<!-- chunk: 七、库存与供应链架构 -->## 七、库存与供应链架构

```mermaid
flowchart TB
    subgraph Inventory["库存核心"]
        INV_API["库存 API"]
        INV_CACHE["Redis Cluster<br/>实时库存"]
        INV_DB["MySQL<br/>库存流水"]
        INV_MQ["Kafka<br/>库存变更事件"]
    end

    subgraph Channels["渠道库存"]
        ONLINE["线上库存<br/>App/Web/小程序"]
        OFFLINE["门店库存<br/>POS 系统"]
        CROSS["跨境库存<br/>保税仓/海外仓"]
    end

    subgraph Warehouse["仓储系统"]
        WMS["WMS<br/>仓储管理"]
        PICK["拣货系统"]
        PACK["打包系统"]
        SHIP["发货系统"]
    end

    ONLINE --> INV_API
    OFFLINE --> INV_API
    CROSS --> INV_API
    INV_API --> INV_CACHE --> INV_DB
    INV_API --> INV_MQ
    INV_MQ --> WMS --> PICK --> PACK --> SHIP

    style Inventory fill:#e3f2fd
    style Warehouse fill:#e8f5e9
```

## 库存扣减策略

```mermaid
stateDiagram-v2
    [*] --> Available: 商品上架
    Available --> Reserved: 用户下单<br/>预占库存
    Reserved --> Deducted: 支付成功<br/>确认扣减
    Reserved --> Released: 支付超时<br/>释放库存
    Deducted --> Locked: 售后锁定
    Locked --> Refunded: 退款完成<br/>库存回补
    Released --> Available
    Refunded --> Available
```

---

<!-- chunk: 八、营销与秒杀架构 -->## 八、营销与秒杀架构

## 秒杀系统架构

```mermaid
flowchart TB
    subgraph Protection["防护层"]
        CDN["CDN 预热<br/>静态化"]
        RATE["全局限流<br/>100 QPS/用户"]
        CAPTCHA["验证码<br/>防机器人"]
        QUEUE["排队系统<br/>异步化"]
    end

    subgraph Core["秒杀核心"]
        STOCK["Redis 库存<br/>原子扣减<br/>DECR"]
        ORDER_ASYNC["异步下单<br/>Kafka"]
        ORDER_CREATE["订单创建"]
    end

    subgraph Fallback["降级策略"]
        CACHE_RESULT["缓存结果<br/>防重放"]
        WAIT_MSG["排队通知"]
        SOLD_OUT["售罄标记"]
    end

    Users["海量用户"] --> CDN --> RATE --> CAPTCHA --> QUEUE
    QUEUE --> STOCK
    STOCK -->|库存充足| ORDER_ASYNC --> ORDER_CREATE
    STOCK -->|库存不足| SOLD_OUT
    QUEUE --> WAIT_MSG
    ORDER_CREATE --> CACHE_RESULT

    style Protection fill:#fff8e1
    style Core fill:#e3f2fd
    style Fallback fill:#ffebee
```

## 秒杀核心代码

```yaml
# Redis Lua 脚本：原子扣减库存
# 秒杀库存扣减 Lua 脚本（在 K8s ConfigMap 中管理）
apiVersion: v1
kind: ConfigMap
metadata:
  name: seckill-scripts
  namespace: ecommerce
data:
  deduct_stock.lua: |
    local stock_key = KEYS[1]
    local order_key = KEYS[2]
    local user_id = ARGV[1]
    local stock = redis.call('GET', stock_key)
    
    if tonumber(stock) <= 0 then
      return -1  -- 库存不足
    end
    
    -- 检查是否已购买（防重）
    local ordered = redis.call('SISMEMBER', order_key, user_id)
    if ordered == 1 then
      return -2  -- 已购买
    end
    
    -- 原子扣减 + 记录购买
    redis.call('DECR', stock_key)
    redis.call('SADD', order_key, user_id)
    return 1  -- 成功
---
# 秒杀服务 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: seckill-service
  namespace: ecommerce
spec:
  replicas: 10
  selector:
    matchLabels:
      app: seckill-service
  template:
    metadata:
      labels:
        app: seckill-service
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: seckill-service
                topologyKey: kubernetes.io/hostname
      containers:
        - name: seckill
          image: ecommerce/seckill-service:v1.0
          env:
            - name: REDIS_CLUSTER
              value: "redis-cluster:6379"
            - name: LUA_SCRIPT_PATH
              value: "/scripts/deduct_stock.lua"
          resources:
            requests:
              cpu: "1"
              memory: "2Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
```

---

<!-- chunk: 九、数据层架构 -->## 九、数据层架构

```mermaid
flowchart TB
    subgraph SQL["关系型数据库"]
        MYSQL_MASTER["MySQL 主库<br/>写操作"]
        MYSQL_SLAVE1["从库 1<br/>读操作"]
        MYSQL_SLAVE2["从库 2<br/>读操作"]
        MYSQL_BACKUP["定时备份<br/>xtrabackup"]
    end

    subgraph Cache["缓存层"]
        REDIS_MASTER["Redis Master"]
        REDIS_SLAVE["Redis Replica"]
        REDIS_SENTINEL["Sentinel<br/>故障转移"]
        REDIS_CLUSTER["Cluster 模式<br/>分片"]
    end

    subgraph NoSQL["文档/搜索引擎"]
        MONGO["MongoDB<br/>商品详情"]
        ES["Elasticsearch<br/>商品搜索"]
    end

    subgraph MQ["消息队列"]
        KAFKA_BROKER["Kafka Broker"]
        KAFKA_TOPIC["Topic 分区"]
        KAFKA_CONSUMER["Consumer Group"]
    end

    subgraph ObjectStore["对象存储"]
        OSS["S3 / OSS / MinIO<br/>图片/视频"]
        CDN_EDGE["CDN 边缘节点"]
    end

    MYSQL_MASTER --> MYSQL_SLAVE1 & MYSQL_SLAVE2
    MYSQL_MASTER --> MYSQL_BACKUP
    REDIS_MASTER --> REDIS_SLAVE
    REDIS_SENTINEL --> REDIS_MASTER & REDIS_SLAVE
    KAFKA_BROKER --> KAFKA_TOPIC --> KAFKA_CONSUMER
    OSS --> CDN_EDGE

    style SQL fill:#e3f2fd
    style Cache fill:#fff8e1
    style MQ fill:#e8f5e9
```

---

<!-- chunk: 十、K8s 部署架构 -->## 十、K8s 部署架构

## Namespace 组织

```mermaid
flowchart TB
    subgraph Infra["基础设施层"]
        NS_INGRESS["ingress-nginx"]
        NS_MONITORING["monitoring"]
        NS_LOGGING["logging"]
    end

    subgraph Platform["平台服务层"]
        NS_AUTH["auth-platform"]
        NS_MSG["message-platform"]
        NS_CONFIG["config-platform"]
    end

    subgraph Business["业务服务层"]
        NS_CORE["ecommerce-core<br/>订单/支付/库存"]
        NS_SUPPORT["ecommerce-support<br/>用户/商品/搜索"]
        NS_PROMO["ecommerce-promo<br/>营销/秒杀"]
    end

    subgraph Data["数据层"]
        NS_DB["database"]
        NS_CACHE["cache"]
        NS_MQ["messaging"]
    end

    Infra --> Platform --> Business --> Data

    style Business fill:#e3f2fd
    style Data fill:#e8f5e9
```

## 节点池规划

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: ecommerce-core
spec:
  template:
    spec:
      requirements:
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["c7.2xlarge", "c7.4xlarge"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]
        - key: topology.kubernetes.io/zone
          operator: In
          values: ["cn-beijing-a", "cn-beijing-b", "cn-beijing-c"]
      taints:
        - key: workload-type
          value: core
          effect: NoSchedule
  limits:
    cpu: 500
    memory: 2000Gi
---
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: ecommerce-spot
spec:
  template:
    spec:
      requirements:
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["c7.xlarge", "c7.2xlarge"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot"]
      taints:
        - key: workload-type
          value: spot
          effect: NoSchedule
  limits:
    cpu: 1000
```

---

<!-- chunk: 十一、高可用与灾备 -->## 十一、高可用与灾备

```mermaid
flowchart TB
    subgraph Primary["生产中心"]
        P_INGRESS["入口集群"]
        P_APP["业务集群"]
        P_DB["数据库主库"]
        P_CACHE["缓存主节点"]
    end

    subgraph DR["灾备中心"]
        D_INGRESS["入口集群"]
        D_APP["业务集群"]
        D_DB["数据库从库<br/>实时同步"]
        D_CACHE["缓存副本"]
    end

    subgraph Global["全局调度"]
        GSLB["GSLB / GeoDNS"]
        HEALTH["健康检查"]
    end

    Users["用户"] --> GSLB
    GSLB -->|正常| P_INGRESS
    GSLB -->|故障切换| D_INGRESS
    P_INGRESS --> P_APP --> P_DB & P_CACHE
    D_INGRESS --> D_APP --> D_DB & D_CACHE
    P_DB -->|binlog 同步| D_DB
    P_CACHE -->|AOF 复制| D_CACHE
    HEALTH --> P_APP & D_APP

    style Primary fill:#c8e6c9
    style DR fill:#fff8e1
```

## 电商系统 SLA 矩阵

| 服务 | 可用性目标 | RTO | RPO | 策略 |
|:---|:---:|:---:|:---:|:---|
| 商品浏览 | 99.99% | 0 | 0 | 多活 + CDN |
| 购物车 | 99.95% | 5min | 0 | 同城双活 |
| 订单创建 | 99.99% | 1min | 0 | 强一致主从 |
| 支付 | 99.999% | 0 | 0 | 金融级多活 |
| 库存 | 99.99% | 30s | 0 | Redis Cluster |
| 搜索 | 99.9% | 10min | 5min | ES 快照 |
| 推荐 | 99.5% | 30min | 1h | 离线计算 |

---

<!-- chunk: 参考链接 -->## 参考链接

- [阿里巴巴电商系统架构演进](https://developer.aliyun.com/ebook/read/7556)
- [美团外卖系统架构](https://tech.meituan.com/)
- [Kubernetes 有状态应用管理](https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/)

---

<!-- chunk: 多云部署方案对照 -->## 多云部署方案对照

## 阿里云服务 → 多云映射表

| 能力域 | 阿里云服务 | AWS 对应 | GCP 对应 | Azure 对应 |
|:---|:---|:---|:---|:---|
| 容器编排 | **ACK** (容器服务) | **EKS** | **GKE** | **AKS** |
| 对象存储 | **OSS** | **S3** | **GCS** | **Blob Storage** |
| CDN | **CDN / DCDN** | **CloudFront** | **Cloud CDN** | **Azure CDN** |
| Web 防火墙 | **WAF** | **AWS WAF** | **Cloud Armor** | **Azure WAF (Front Door)** |
| DDoS 防护 | **DDoS 防护** | **Shield** | **Cloud Armor** | **Azure DDoS Protection** |
| 密钥管理 | **KMS** | **KMS** | **Cloud KMS** | **Key Vault** |
| 负载均衡 | **ALB / SLB** | **ALB / NLB** | **Cloud Load Balancing** | **Azure Load Balancer / App GW** |
| 消息队列 | **RocketMQ** | **SQS / SNS** | **Pub/Sub** | **Service Bus** |
| 数据库 | **RDS MySQL / PolarDB** | **RDS MySQL / Aurora** | **Cloud SQL / AlloyDB** | **Azure MySQL / Cosmos DB** |
| 缓存 | **Redis 云版** | **ElastiCache** | **Memorystore** | **Azure Cache for Redis** |
| 搜索 | **Elasticsearch 云版** | **OpenSearch** | **Elastic Cloud on GCP** | **Azure Cognitive Search** |
| 容器镜像 | **ACR** | **ECR** | **Artifact Registry** | **ACR (Azure)** |
| 节点自动伸缩 | **ASK / Karpenter** | **Karpenter** | **GKE Autopilot** | **AKS Karpenter / Virtual Nodes** |
| DNS | **云解析 DNS** | **Route 53** | **Cloud DNS** | **Azure DNS** |
| 日志 | **SLS (日志服务)** | **CloudWatch Logs** | **Cloud Logging** | **Log Analytics** |
| 链路追踪 | **ARMS / 链路追踪** | **X-Ray** | **Cloud Trace** | **Application Insights** |

## 多云部署注意事项

1. **数据主权与合规**: 电商涉及支付数据需关注 PCI-DSS，不同云厂商的 PCI-DSS 认证范围不同，需确认目标 Region 的合规状态。
2. **网络互通**: 多云部署时需通过 VPN / 专线打通 VPC，注意跨云通信延迟对订单链路的影响（建议同城双活优先）。
3. **对象存储兼容**: 应用层使用 S3 兼容 API（如 MinIO / AWS SDK），可降低迁移成本；避免直接依赖 OSS SDK 的私有 API。
4. **K8s 版本对齐**: 各云 K8s 发行版（ACK / EKS / GKE / AKS）版本发布节奏不同，需统一升级窗口，避免 API 兼容性问题。
5. **节点池策略**: AWS 用 Karpenter、GCP 用 Autopilot、Azure 用 Virtual Nodes，HPA/VPA 行为有差异，需分别压测。
6. **支付通道隔离**: 支付 PCI-DSS 区域建议与业务区域在同一云内，避免跨云传输敏感数据。

## 云中立方案（开源替代）

| 能力域 | 开源方案 | 说明 |
|:---|:---|:---|
| 容器编排 | **Kubernetes** (原生) | 使用 kubeadm / Cluster API 自建，或 RKE2 / k3s |
| 对象存储 | **MinIO** | S3 兼容，可部署在任意 K8s 集群 |
| CDN / 边缘加速 | **Cloudflare** / **Fastly** | 独立 CDN 服务商，不绑定云 |
| WAF | **ModSecurity** + **OWASP CRS** | 开源 WAF，配合 Ingress Controller |
| 密钥管理 | **HashiCorp Vault** | 已在支付架构中使用，可替代云 KMS |
| 负载均衡 | **HAProxy** / **MetalLB** | K8s 原生 Service + MetalLB |
| 消息队列 | **Apache Kafka** / **RocketMQ** (开源版) | K8s Operator 部署 |
| 数据库 | **MySQL** (Operator) / **TiDB** | Vitess / TiDB Operator 分布式方案 |
| 缓存 | **Redis Cluster** | K8s StatefulSet 或 Redis Operator |
| 搜索 | **Elasticsearch** (ECK) / **OpenSearch** | Elastic Cloud on K8s Operator |
| 镜像仓库 | **Harbor** | 企业级开源镜像仓库 |
| 节点伸缩 | **Karpenter** (开源版) / **Cluster Autoscaler** | Karpenter 已支持多云 |
| 日志 | **Loki** + **Promtail** | 轻量级，已在架构图中使用 |
| 链路追踪 | **Jaeger** / **OpenTelemetry** | 已在架构图中使用 |
| DNS | **CoreDNS** + **ExternalDNS** | K8s 原生 DNS 管理 |

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-application-architecture MOC
- [[domain-20-application-patterns/行业架构/README.md|Topic 应用层架构设计最佳实践]]
- [[domain-20-application-patterns/行业架构/02-mini-program-architecture.md|小程序平台架构设计]]
- [[domain-20-application-patterns/行业架构/03-cms-architecture.md|内容管理系统 CMS 架构设计]]
- [[domain-20-application-patterns/行业架构/04-im-rtc-architecture.md|实时通信 IM/RTC 架构设计]]
- [[domain-20-application-patterns/行业架构/05-online-education-architecture.md|在线教育平台 Kubernetes 生产架构设计]]
- [[domain-20-application-patterns/行业架构/06-fintech-architecture.md|金融科技FinTech Kubernetes生产架构设计]]
- [[domain-20-application-patterns/行业架构/07-iot-platform-architecture.md|物联网 IoT 平台架构设计]]
- [[domain-20-application-patterns/行业架构/08-ai-ml-inference-architecture.md|AI/ML 推理服务 Kubernetes 生产架构设计]]
- [[domain-20-application-patterns/行业架构/09-gaming-backend-architecture.md|游戏后端 Kubernetes 生产架构设计]]
- [[domain-20-application-patterns/行业架构/10-social-media-architecture.md|社交媒体平台Kubernetes生产架构设计]]
- [[domain-20-application-patterns/行业架构/11-smart-retail-architecture.md|智慧零售与新零售Kubernetes生产架构设计]]

## See Also

- 95-industrial-metaverse
- 96-carbon-capture
- 02-mini-program-architecture
- 03-cms-architecture


<!-- risk-assessed -->
