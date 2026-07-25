---
title: 实时通信 (IM / RTC) Kubernetes 生产架构设计
description: 'title: 实时通信 IM/RTC 架构设计'
summary: 'title: 实时通信 IM/RTC 架构设计'
category: general
tags:
- architecture
- best-practice
- prometheus
- redis
- mysql
- kafka
- elasticsearch
- statefulset
- gateway
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 实时通信 (IM / RTC) Kubernetes 生产架构设计 是什么
- 如何 实时通信 (IM / RTC) Kubernetes 生产架构设计
- Kubernetes 20 application patterns 最佳实践
trigger_keywords:
- 实时通信
- IM
- RTC
- Kubernetes
- 生产架构设计
- application
- patterns
prerequisites:
- kubectl-basics
- prometheus-basics
- kafka-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 实时通信 IM/RTC 架构设计
description: '# 实时通信 (IM / RTC) [[Kubernetes|Kubernetes]] 生产架构设计'
category: application-architecture
tags:
- k8s
- architecture
- industry
- [[Prometheus|prometheus]]
- redis
- mysql
- kafka
- elasticsearch
- [[StatefulSet|statefulset]]
- gateway
last_updated: 2026-05-18
difficulty: expert
reading_level: expert
audience:
- 实时通信架构师
- 音视频工程师
- 后端开发工程师
estimated_read_time: 5min
intent_queries:
- 即时通讯 IM 即时消息 Kubernetes 部署
- 音视频通话 WebRTC SFU MCU 架构
- 实时消息推送信令服务设计
- 消息存储漫游搜索系统
- 阿里云 RTC 实时音视频
trigger_keywords:
- 即时通讯
- IM聊天
- RTC音视频通话
- WebRTC
- SFU选择性转发
- MCU混音混画
- 信令服务器
- 消息推送
- 阿里云RTC
- 实时消息
related_domains:
- 网络
- 故障诊断
related_topics:
- topic-im-rtc-architecture
- topic-streaming-architecture
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

# 实时通信 (IM / RTC) Kubernetes 生产架构设计

> **适用场景**: 即时消息 / 音视频通话 / 直播连麦 / 在线客服 / 会议系统  
> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **目标读者**: 实时通信架构师、音视频工程师、后端 TL

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、整体架构全景](#一整体架构全景)
- [二、IM 消息系统架构](#二im-消息系统架构)
- [三、音视频通话 (RTC) 架构](#三音视频通话-rtc-架构)
- [四、直播连麦架构](#四直播连麦架构)
- [五、信令服务器架构](#五信令服务器架构)
- [六、媒体服务器架构](#六媒体服务器架构)
- [七、全球加速网络架构](#七全球加速网络架构)
- [八、K8s 部署架构](#八k8s-部署架构)

---

<!-- chunk: 一、整体架构全景 -->## 一、整体架构全景

```mermaid
flowchart TB
    subgraph Clients["客户端"]
        IOS["iOS App"]
        ANDROID["Android App"]
        WEB["Web / H5"]
        MINI["小程序"]
        DESKTOP["桌面端"]
    end

    subgraph Edge["边缘接入层"]
        DNS["智能 DNS<br/>就近接入"]
        LB["L4 负载均衡<br/>长连接保持"]
        QUIC["QUIC / HTTP3<br/>0-RTT 握手"]
    end

    subgraph Signaling["信令层"]
        SIGNAL["信令服务器<br/>WebSocket / TCP"]
        PRESENCE["在线状态<br/>心跳/订阅"]
        PUSH["消息推送<br/>APNs / FCM / 厂商通道"]
    end

    subgraph Media["媒体层"]
        SFU["SFU 服务器<br/>选择性转发"]
        MCU["MCU 服务器<br/>混音混画"]
        CDN_MEDIA["媒体 CDN<br/>直播分发"]
    end

    subgraph Business["业务层"]
        MSG_SVC["消息服务<br/>存储/漫游/搜索"]
        GROUP_SVC["群组服务<br/>成员/权限/禁言"]
        USER_SVC["用户服务<br/>资料/关系链"]
        CALL_SVC["通话服务<br/>呼叫/接听/挂断"]
    end

    subgraph Data["数据层"]
        TIMESCALE["TimescaleDB<br/>消息时序"]
        REDIS["Redis<br/>会话/状态/路由"]
        KAFKA["Kafka<br/>消息队列"]
        OSS["对象存储<br/>媒体文件"]
    end

    Clients --> Edge --> Signaling --> Business --> Data
    Signaling --> Media --> CDN_MEDIA
    Media --> OSS

    style Signaling fill:#e3f2fd
    style Media fill:#fff8e1
    style Business fill:#e8f5e9
```

---

<!-- chunk: 二、IM 消息系统架构 -->## 二、IM 消息系统架构

## 消息收发流程

```mermaid
sequenceDiagram
    participant Sender as 发送者 (Alice)
    participant GW as 接入网关
    participant Logic as 消息逻辑层
    participant Storage as 消息存储
    participant Route as 路由服务
    participant Push as 推送服务
    participant Receiver as 接收者 (Bob)

    Sender->>GW: 发送消息 (msg_id=uuid)
    GW->>Logic: 转发消息
    Logic->>Logic: 反垃圾/敏感词过滤
    Logic->>Storage: 写入消息 (MySQL/TiDB)
    Storage-->>Logic: 确认

    alt Bob 在线
        Logic->>Route: 查询 Bob 路由
        Route-->>Logic: Bob -> Gateway-2
        Logic->>GW: 推送消息
        GW->>Receiver: WebSocket 推送
        Receiver-->>GW: ACK
    else Bob 离线
        Logic->>Push: 发送离线推送
        Push->>Receiver: APNs/FCM/厂商推送
    end

    Logic->>Storage: 更新已读状态
```

## 消息存储模型

```mermaid
flowchart TB
    subgraph WritePath["写入路径"]
        MSG_IN["消息进入"]
        SEQ["分配 SeqID<br/>全局递增"]
        WRITE_DB["写入接收方收件箱<br/>写扩散"]
        WRITE_SENDER["写入发送方发件箱"]
    end

    subgraph ReadPath["读取路径"]
        SYNC["消息同步<br/>拉模式 / 推模式"]
        ROAM["消息漫游<br/>多设备同步"]
        SEARCH["消息搜索<br/>Elasticsearch"]
    end

    subgraph Storage["存储层"]
        HOT["热数据<br/>Redis (最近 7 天)"]
        WARM["温数据<br/>MySQL/TiDB (90 天)"]
        COLD["冷数据<br/>S3 / HDFS (永久)"]
    end

    MSG_IN --> SEQ --> WRITE_DB & WRITE_SENDER --> Storage
    Storage --> SYNC & ROAM & SEARCH
    HOT --> WARM --> COLD

    style WritePath fill:#e3f2fd
    style ReadPath fill:#fff8e1
    style Storage fill:#e8f5e9
```

## 消息队列 K8s 配置

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: im-message-router
  namespace: im-system
spec:
  serviceName: im-message-router
  replicas: 5
  selector:
    matchLabels:
      app: im-message-router
  template:
    metadata:
      labels:
        app: im-message-router
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values:
                      - im-message-router
              topologyKey: kubernetes.io/hostname
      containers:
        - name: router
          image: im/message-router:v3.0
          ports:
            - containerPort: 8080
              name: http
            - containerPort: 9090
              name: grpc
            - containerPort: 10000
              name: websocket
          env:
            - name: KAFKA_BROKERS
              value: "kafka-0.kafka:9092,kafka-1.kafka:9092,kafka-2.kafka:9092"
            - name: REDIS_CLUSTER
              value: "redis-cluster:6379"
            - name: WS_MAX_CONNECTIONS
              value: "100000"
          resources:
            requests:
              cpu: "2"
              memory: "4Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
          livenessProbe:
            httpGet:
              path: /health/live
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 10
---
# WebSocket 长连接负载均衡
apiVersion: v1
kind: Service
metadata:
  name: im-gateway
  namespace: im-system
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: nlb
    service.beta.kubernetes.io/aws-load-balancer-backend-protocol: tcp
spec:
  type: LoadBalancer
  selector:
    app: im-message-router
  ports:
    - name: websocket
      port: 10000
      targetPort: 10000
      protocol: TCP
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3 小时会话保持
```

---

<!-- chunk: 三、音视频通话 (RTC) 架构 -->## 三、音视频通话 (RTC) 架构

```mermaid
flowchart TB
    subgraph MediaPath["媒体传输路径"]
        subgraph Alice["Alice (发送方)"]
            A_CAP["采集<br/>摄像头/麦克风"]
            A_ENCODE["编码<br/>H.264 / VP8 / AV1"]
            A_SEND["发送<br/>RTP/SRTP"]
        end

        subgraph Network["网络层"]
            TURN["TURN Server<br/>中继"]
            STUN["STUN Server<br/>NAT 穿透"]
            SFU_S["SFU Server<br/>选择性转发"]
        end

        subgraph Bob["Bob (接收方)"]
            B_RECV["接收<br/>RTP/SRTP"]
            B_DECODE["解码<br/>H.264 / VP8 / AV1"]
            B_RENDER["渲染<br/>屏幕/扬声器"]
        end
    end

    subgraph Quality["质量优化"]
        AEC["回声消除 AEC"]
        NS["噪声抑制 NS"]
        AGC["自动增益 AGC"]
        BWE["带宽估计<br/>GCC / SCReAM"]
        FEC["前向纠错 FEC"]
        NACK["丢包重传 NACK"]
    end

    A_CAP --> A_ENCODE --> A_SEND --> SFU_S --> B_RECV --> B_DECODE --> B_RENDER
    A_SEND --> STUN & TURN --> SFU_S
    Quality --> A_ENCODE & B_DECODE & SFU_S

    style MediaPath fill:#e3f2fd
    style Quality fill:#e8f5e9
```

## WebRTC 信令流程

```mermaid
sequenceDiagram
    participant Alice as Alice
    participant Signaling as 信令服务器
    participant Bob as Bob
    participant SFU as SFU 服务器

    Alice->>Signaling: 发起呼叫 (call)
    Signaling->>Bob: 推送呼叫通知 (invite)
    Bob-->>Signaling: 接听 (accept)
    Signaling-->>Alice: 对方已接听

    Alice->>Alice: 创建 PeerConnection
    Alice->>Alice: createOffer()
    Alice->>Signaling: 发送 SDP Offer
    Signaling->>Bob: 转发 SDP Offer

    Bob->>Bob: 创建 PeerConnection
    Bob->>Bob: createAnswer()
    Bob->>Signaling: 发送 SDP Answer
    Signaling->>Alice: 转发 SDP Answer

    Alice->>Signaling: 发送 ICE Candidate
    Signaling->>Bob: 转发 ICE Candidate
    Bob->>Signaling: 发送 ICE Candidate
    Signaling->>Alice: 转发 ICE Candidate

    Alice->>SFU: 连接媒体通道 (RTP)
    Bob->>SFU: 连接媒体通道 (RTP)
    SFU->>SFU: 选择性转发
    SFU->>Alice: 转发 Bob 的媒体
    SFU->>Bob: 转发 Alice 的媒体
```

---

<!-- chunk: 四、直播连麦架构 -->## 四、直播连麦架构

```mermaid
flowchart TB
    subgraph Anchor["主播端"]
        A_PUSH["推流<br/>RTMP / WHIP"]
        A_BEAUTY["美颜/滤镜<br/>GPU 加速"]
        A_MIX["混音<br/>BGM + 人声"]
    end

    subgraph Audience["观众端"]
        U1["观众 1<br/>拉流"]
        U2["观众 2<br/>拉流"]
        U3["观众 3<br/>连麦中"]
    end

    subgraph MediaServer["媒体服务器集群"]
        INGEST["接入节点<br/>收流"]
        TRANSCODE["转码节点<br/>多清晰度"]
        SFU_LIVE["SFU 节点<br/>连麦转发"]
        CDN_ORIGIN["CDN 源站"]
    end

    subgraph CDN["分发网络"]
        EDGE1["边缘节点<br/>北京"]
        EDGE2["边缘节点<br/>上海"]
        EDGE3["边缘节点<br/>广州"]
    end

    Anchor --> A_PUSH --> INGEST --> TRANSCODE --> CDN_ORIGIN --> CDN
    U3 -->|连麦| SFU_LIVE --> INGEST
    CDN --> EDGE1 & EDGE2 & EDGE3 --> Audience

    style MediaServer fill:#e3f2fd
    style CDN fill:#e8f5e9
```

---

<!-- chunk: 五、信令服务器架构 -->## 五、信令服务器架构

```mermaid
flowchart TB
    subgraph Connection["连接管理"]
        WS["WebSocket<br/>长连接"]
        TCP["TCP<br/>私有协议"]
        QUIC_CONN["QUIC<br/>0-RTT"]
    end

    subgraph Session["会话管理"]
        HEARTBEAT["心跳检测<br/>30s 间隔"]
        RECONNECT["断线重连<br/>会话恢复"]
        MULTI_DEV["多设备管理<br/>在线状态"]
    end

    subgraph Routing["消息路由"]
        USER_ROUTE["用户路由表<br/>UserID -> Gateway"]
        GROUP_ROUTE["群组路由表<br/>GroupID -> Users"]
        BROADCAST["广播<br/>系统通知"]
    end

    Connection --> Session --> Routing

    style Connection fill:#e3f2fd
    style Session fill:#fff8e1
    style Routing fill:#e8f5e9
```

---

<!-- chunk: 六、媒体服务器架构 -->## 六、媒体服务器架构

```mermaid
flowchart TB
    subgraph Ingest["媒体接入"]
        RTMP["RTMP 推流<br/>传统直播"]
        SRT["SRT 推流<br/>低延迟"]
        WHIP["WHIP 推流<br/>WebRTC"]
        RTP["RTP 接入<br/>RTC"]
    end

    subgraph Process["媒体处理"]
        DEMUX["解封装<br/>FLV / MP4 / WebM"]
        DECODE["解码<br/>FFmpeg / 硬件"]
        FILTER["滤镜处理<br/>水印/字幕/叠加"]
        ENCODE["编码<br/>多码率输出"]
        MUX["封装<br/>HLS / DASH / FLV"]
    end

    subgraph Output["输出分发"]
        HLS["HLS<br/>m3u8 + ts"]
        DASH["DASH<br">MPD + fmp4"]
        FLV["HTTP-FLV<br/>低延迟"]
        WEBRTC_OUT["WebRTC<br">超低延迟"]
    end

    Ingest --> Process --> Output

    style Process fill:#e3f2fd
    style Output fill:#e8f5e9
```

## Media Server K8s 部署 (基于 Mediasoup / Janus)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mediasoup-worker
  namespace: im-media
spec:
  replicas: 10
  selector:
    matchLabels:
      app: mediasoup-worker
  template:
    metadata:
      labels:
        app: mediasoup-worker
    spec:
      hostNetwork: true  # WebRTC 需要公网 IP
      nodeSelector:
        node-type: media
      tolerations:
        - key: node-type
          operator: Equal
          value: media
          effect: NoSchedule
      containers:
        - name: mediasoup
          image: im/mediasoup-worker:v2.0
          ports:
            - containerPort: 10000
              protocol: UDP
              name: rtp
            - containerPort: 20000
              protocol: UDP
              name: rtcp
          env:
            - name: RTC_MIN_PORT
              value: "10000"
            - name: RTC_MAX_PORT
              value: "10100"
            - name: WORKER_NUM
              value: "4"
          resources:
            requests:
              cpu: "4"
              memory: "4Gi"
            limits:
              cpu: "8"
              memory: "8Gi"
          securityContext:
            capabilities:
              add:
                - NET_BIND_SERVICE
---
# 媒体节点池 (GPU / 高网络吞吐)
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: media-nodes
spec:
  template:
    spec:
      requirements:
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["c7.8xlarge", "g7.2xlarge"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]
      taints:
        - key: node-type
          value: media
          effect: NoSchedule
  limits:
    cpu: 500
```

---

<!-- chunk: 七、全球加速网络架构 -->## 七、全球加速网络架构

```mermaid
flowchart TB
    subgraph RegionCN["中国区"]
        CN_BJ["北京节点"]
        CN_SH["上海节点"]
        CN_SZ["深圳节点"]
    end

    subgraph RegionAPAC["亚太区"]
        SG["新加坡节点"]
        JP["东京节点"]
        IN["孟买节点"]
    end

    subgraph RegionEU["欧洲区"]
        DE["法兰克福节点"]
        UK["伦敦节点"]
    end

    subgraph RegionUS["美洲区"]
        US_WEST["硅谷节点"]
        US_EAST["弗吉尼亚节点"]
    end

    CN_BJ <-->|专线| CN_SH <-->|专线| CN_SZ
    CN_SZ <-->|海底光缆| SG <-->|专线| JP
    SG <-->|专线| IN
    SG <-->|专线| DE <-->|专线| UK
    DE <-->|专线| US_EAST <-->|专线| US_WEST

    Users["全球用户"] -->|就近接入| RegionCN & RegionAPAC & RegionEU & RegionUS

    style RegionCN fill:#e3f2fd
    style RegionAPAC fill:#e8f5e9
    style RegionEU fill:#fff8e1
    style RegionUS fill:#ffebee
```

---

<!-- chunk: 八、K8s 部署架构 -->## 八、K8s 部署架构

## Namespace 组织

```mermaid
flowchart TB
    subgraph Infra["基础设施"]
        NS_DB["im-database"]
        NS_CACHE["im-cache"]
        NS_MQ["im-messaging"]
    end

    subgraph Platform["平台服务"]
        NS_SIGNAL["im-signaling<br/>信令/WebSocket"]
        NS_MEDIA["im-media<br/>SFU/MCU"]
        NS_PUSH["im-push<br/>推送服务"]
        NS_BUSINESS["im-business<br/>消息/群组/用户"]
    end

    subgraph Gateway["网关层"]
        NS_EDGE["im-edge<br/>接入网关"]
        NS_CDN["im-cdn<br/>媒体分发"]
    end

    subgraph Ops["运维"]
        NS_MONITOR["im-monitoring<br/>QoS 监控"]
        NS_LOG["im-logging<br/>通话日志"]
    end

    Infra --> Platform --> Gateway
    Ops --> Platform & Gateway

    style Platform fill:#e3f2fd
    style Gateway fill:#e8f5e9
```

## RTC 质量监控告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: im-quality-alerts
  namespace: im-monitoring
spec:
  groups:
    - name: rtc-quality
      rules:
        - alert: RTCAudioQualityDegraded
          expr: |
            (
              sum(rate(rtc_audio_packet_loss_ratio[1m]))
              by (room_id, user_id)
            ) > 0.05
          for: 30s
          labels:
            severity: warning
          annotations:
            summary: "RTC 音频丢包率超过 5%"

        - alert: RTCVideoFreeze
          expr: |
            (
              sum(rate(rtc_video_freeze_count[1m]))
              by (room_id, user_id)
            ) > 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "RTC 视频出现卡顿"

        - alert: WebSocketConnectionDrop
          expr: |
            (
              im_websocket_connections -
              im_websocket_connections  offset 5m
            ) < -1000
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "WebSocket 连接数骤降超过 1000"
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [WebRTC 官方文档](https://webrtc.org/getting-started/overview)
- [Mediasoup 文档](https://mediasoup.org/documentation/)
- [Janus Gateway](https://janus.conf.meetecho.com/)
- [WebTransport / WHIP / WHEP](https://datatracker.ietf.org/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-application-architecture MOC
- [[04-应用模式/02-行业架构/README.md|Topic 应用层架构设计最佳实践]]
- [[04-应用模式/02-行业架构/01-ecommerce-architecture.md|电商系统 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/02-mini-program-architecture.md|小程序平台架构设计]]
- [[04-应用模式/02-行业架构/03-cms-architecture.md|内容管理系统 CMS 架构设计]]
- [[04-应用模式/02-行业架构/05-online-education-architecture.md|在线教育平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/06-fintech-architecture.md|金融科技FinTech Kubernetes生产架构设计]]
- [[04-应用模式/02-行业架构/07-iot-platform-architecture.md|物联网 IoT 平台架构设计]]
- [[04-应用模式/02-行业架构/08-ai-ml-inference-architecture.md|AI/ML 推理服务 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/09-gaming-backend-architecture.md|游戏后端 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/10-social-media-architecture.md|社交媒体平台Kubernetes生产架构设计]]
- [[04-应用模式/02-行业架构/11-smart-retail-architecture.md|智慧零售与新零售Kubernetes生产架构设计]]

## Related

- 20-microservice-governance-architecture

## See Also

- 02-mini-program-architecture
- 03-cms-architecture
- 05-online-education-architecture
- 06-fintech-architecture


<!-- risk-assessed -->
