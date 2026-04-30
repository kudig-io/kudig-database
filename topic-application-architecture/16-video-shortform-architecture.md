# 音视频与短视频平台 Kubernetes 生产架构设计

> **适用场景**: 短视频平台 / 长视频点播 / 直播互动 / 音视频通话 / 云剪辑 / 数字人  
> **云厂商**: 阿里云 ACK + 视频云产品体系  
> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **目标读者**: 音视频架构师、CDN 专家、阿里云解决方案架构师

---

## 📋 目录

- [一、整体架构全景](#一整体架构全景)
- [二、短视频生产与分发架构](#二短视频生产与分发架构)
- [三、直播推拉流架构](#三直播推拉流架构)
- [四、音视频处理流水线架构](#四音视频处理流水线架构)
- [五、推荐与个性化分发架构](#五推荐与个性化分发架构)
- [六、实时互动与连麦架构](#六实时互动与连麦架构)
- [七、版权保护与内容审核架构](#七版权保护与内容审核架构)
- [八、ACK 阿里云部署架构](#八ack-阿里云部署架构)

---

## 一、整体架构全景

```mermaid
flowchart TB
    subgraph Creators["创作者"]
        UPLOADER["视频上传<br">手机/PC"]
        LIVE_STREAMER["主播<br">OBS/手机"]
        EDITOR["云剪辑<br">在线编辑"]
    end

    subgraph MediaCloud["媒体云服务 (阿里云)"]
        VOD_PROC["点播处理<br">转码/水印/加密"]
        LIVE_PROC["直播处理<br">RTS/转码/录制"]
        AI_MED["媒体 AI<br">审核/标签/摘要"]
        CDN_MED["CDN 分发<br">全球加速"]
    end

    subgraph Platform["平台服务 (ACK)"]
        FEED_VIDEO["Feed 推荐<br">个性化"]
        COMMENT["评论系统<br">弹幕/互动"]
        SOCIAL_VIDEO["社交<br">关注/私信"]
        MONETIZE_VIDEO["变现<br">广告/电商/打赏"]
    end

    subgraph Consumers["消费者"]
        MOBILE_VIEWER["移动端<br">App/小程序"]
        WEB_VIEWER["Web 端<br">PC/平板"]
        TV_VIEWER["TV 端<br">OTT/投屏"]
    end

    Creators --> MediaCloud --> Platform --> Consumers

    style MediaCloud fill:#e3f2fd
    style Platform fill:#fff8e1
```

### 阿里云产品映射

| 架构层 | 阿里云方案 | 说明 |
|:---|:---|:---|
| 容器平台 | **ACK Pro** | 业务服务托管 |
| 视频点播 | **视频点播 VOD** | 上传/存储/转码/分发 |
| 直播 | **视频直播 Live** | 推流/拉流/RTS/录制 |
| 实时音视频 | **音视频通信 RTC** | 连麦/会议/互动 |
| CDN | **CDN** + **DCDN** | 静态+动态加速 |
| 媒体处理 | **智能媒体服务 IMS** | AI 审核/标签/摘要 |
| 对象存储 | **OSS** | 媒资存储 |
| 消息队列 | **RocketMQ** | 异步处理 |
| 大数据 | **MaxCompute** + **PAI** | 推荐/分析 |

---

## 二、短视频生产与分发架构

```mermaid
flowchart TB
    subgraph Production["内容生产"]
        CAPTURE["拍摄<br">滤镜/美颜"]
        EDIT["剪辑<br">卡点/字幕/特效"]
        MUSIC["配乐<br">版权音乐库"]
        UPLOAD_VIDEO["上传<br">断点续传"]
    end

    subgraph Processing["云端处理"]
        INSPECT["内容审核<br">机审+人审"]
        TRANSCODE_VIDEO["转码<br">多清晰度"]
        EXTRACT["特征提取<br">标签/封面/指纹"]
        ENCRYPT_VIDEO["加密<br">DRM/私有"]
    end

    subgraph Distribution["分发"]
        REC_VIDEO["推荐引擎<br">冷启动/兴趣"]
        CDN_PUSH["CDN 预热<br">热点推送"]
        P2P["P2P 加速<br">节省带宽"]
    end

    Production --> Processing --> Distribution

    style Production fill:#e3f2fd
    style Processing fill:#fff8e1
    style Distribution fill:#e8f5e9
```

---

## 三、直播推拉流架构

```mermaid
flowchart TB
    subgraph Publisher["推流端"]
        OBS["OBS / 专业设备"]
        MOBILE_LIVE["手机直播"]
        WEB_LIVE["Web 推流<br">WHIP"]
    end

    subgraph Ingestion["接入层"]
        RTMP_INGEST["RTMP 接入"]
        SRT_INGEST["SRT 接入<br">低延迟"]
        WEBRTC_INGEST["WebRTC 接入<br">超低延迟"]
    end

    subgraph ProcessingLive["处理层"]
        TRANSCODE_LIVE["实时转码<br">多码率"]
        RECORD_LIVE["录制<br">时移/回放"]
        AI_LIVE["AI 处理<br">美颜/虚拟背景"]
    end

    subgraph DistributionLive["分发层"]
        HLS_LIVE["HLS<br">iOS/通用"]
        FLV_LIVE["HTTP-FLV<br">低延迟"]
        RTS_LIVE["RTS<br">阿里云超低延迟"]
        WEBRTC_LIVE["WebRTC<br"><1s 延迟"]
    end

    Publisher --> Ingestion --> ProcessingLive --> DistributionLive

    style Ingestion fill:#e3f2fd
    style ProcessingLive fill:#fff8e1
    style DistributionLive fill:#e8f5e9
```

---

## 四、音视频处理流水线架构

```mermaid
flowchart TB
    subgraph Input["输入"]
        RAW_VIDEO["原始视频"]
        RAW_AUDIO["原始音频"]
        SUBTITLE["字幕文件"]
    end

    subgraph Pipeline["处理流水线 (Tekton)"]
        DEMUX["解封装<br">MP4/MKV/FLV"]
        VIDEO_ENCODE["视频编码<br">H.264/H.265/AV1"]
        AUDIO_ENCODE["音频编码<br">AAC/OPUS"]
        PACKAGE["封装<br">DASH/HLS/MP4"]
        DRM["DRM 加密<br">Widevine/FairPlay"]
    end

    subgraph Output["输出"]
        MP4_OUT["MP4<br">下载"]
        HLS_OUT["HLS<br">iOS"]
        DASH_OUT["DASH<br">Android/Web"]
        AUDIO_ONLY["纯音频<br">电台/播客"]
    end

    Input --> Pipeline --> Output

    style Pipeline fill:#e3f2fd
    style Output fill:#e8f5e9
```

### 视频处理 K8s Pipeline

```yaml
apiVersion: tekton.dev/v1beta1
kind: Pipeline
metadata:
  name: video-processing-pipeline
  namespace: media-platform
spec:
  workspaces:
    - name: source
    - name: docker-config
  params:
    - name: input-url
      type: string
    - name: output-prefix
      type: string
  tasks:
    - name: download
      taskSpec:
        steps:
          - name: wget
            image: alpine
            script: |
              wget $(params.input-url) -O /workspace/source/input.mp4
      workspaces:
        - name: source
          workspace: source

    - name: transcode-multi-bitrate
      runAfter: [download]
      taskSpec:
        steps:
          - name: ffmpeg
            image: jrottenberg/ffmpeg:6.0-alpine
            script: |
              # 1080p
              ffmpeg -i /workspace/source/input.mp4 \
                -c:v libx264 -crf 23 -preset fast \
                -c:a aac -b:a 128k \
                -s 1920x1080 \
                /workspace/source/1080p.mp4
              # 720p
              ffmpeg -i /workspace/source/input.mp4 \
                -c:v libx264 -crf 26 -preset fast \
                -c:a aac -b:a 96k \
                -s 1280x720 \
                /workspace/source/720p.mp4
              # 480p
              ffmpeg -i /workspace/source/input.mp4 \
                -c:v libx264 -crf 28 -preset fast \
                -c:a aac -b:a 64k \
                -s 854x480 \
                /workspace/source/480p.mp4
      workspaces:
        - name: source
          workspace: source

    - name: package-hls
      runAfter: [transcode-multi-bitrate]
      taskSpec:
        steps:
          - name: hls-packager
            image: google/shaka-packager:latest
            script: |
              packager \
                'in=/workspace/source/1080p.mp4,stream=video,init_segment=1080p_init.mp4,segment_template=1080p_$Number$.m4s' \
                'in=/workspace/source/720p.mp4,stream=video,init_segment=720p_init.mp4,segment_template=720p_$Number$.m4s' \
                'in=/workspace/source/480p.mp4,stream=video,init_segment=480p_init.mp4,segment_template=480p_$Number$.m4s' \
                'in=/workspace/source/1080p.mp4,stream=audio,init_segment=audio_init.mp4,segment_template=audio_$Number$.m4s' \
                --mpd_output manifest.mpd \
                --hls_master_playlist_output master.m3u8
      workspaces:
        - name: source
          workspace: source

    - name: upload-to-oss
      runAfter: [package-hls]
      taskSpec:
        steps:
          - name: oss-upload
            image: registry.cn-hangzhou.aliyuncs.com/aliyun/ossutil:latest
            script: |
              ossutil cp -r /workspace/source/ \
                oss://media-bucket/processed/$(params.output-prefix)/
      workspaces:
        - name: source
          workspace: source
```

---

## 五、推荐与个性化分发架构

```mermaid
flowchart TB
    subgraph RecallLayer["召回层"]
        CF["协同过滤<br">用户相似"]
        CONTENT_BASED["内容相似<br">标签/Embedding"]
        HOT["热门/趋势<br">全局/分区"]
        FOLLOW_REC["关注流<br">时间序"]
    end

    subgraph RankLayer["排序层"]
        FEATURE["特征拼接<br">用户/内容/上下文"]
        DEEP_MODEL["深度模型<br">DIN/DIEN"]
        MULTI_TASK["多目标<br">播放/点赞/关注"]
    end

    subgraph ReRank["重排序"]
        DIVERSITY["多样性<br">打散/探索"]
        FRESHNESS["新鲜度<br">新内容扶持"]
        QUALITY["质量过滤<br">低俗/重复"]
        AD_INSERT["广告插入<br">频率控制"]
    end

    RecallLayer --> RankLayer --> ReRank

    style RecallLayer fill:#e3f2fd
    style RankLayer fill:#fff8e1
    style ReRank fill:#e8f5e9
```

---

## 六、实时互动与连麦架构

```mermaid
flowchart TB
    subgraph Interaction["互动形式"]
        DANMU_VIDEO["弹幕<br">实时文字"]
        GIFT["礼物<br">动画特效"]
        LIKE_ANI["点赞<br">动画"]
        CO_HOST["连麦<br">观众上麦"]
        PK["PK 对战<br">跨房间"]
    end

    subgraph Signaling["信令"]
        WS_SIGNAL["WebSocket<br">状态同步"]
        ROOM_MGMT["房间管理<br">进出/麦位"]
        PERMISSION["权限<br">禁言/踢人"]
    end

    subgraph MediaMedia["媒体"]
        MIXER["混音混画<br">合流"]
        EFFECT["特效<br">美颜/变声"]
        RECORD_INT["录制<br">精彩片段"]
    end

    Interaction --> Signaling --> MediaMedia

    style Signaling fill:#e3f2fd
    style MediaMedia fill:#e8f5e9
```

---

## 七、版权保护与内容审核架构

```mermaid
flowchart TB
    subgraph UploadCheck["上传检测"]
        FINGERPRINT["指纹提取<br">视频/音频"]
        COMPARE_DB["指纹比对<br">版权库"]
        DUPLICATE["重复检测<br">站内查重"]
    end

    subgraph ContentCheck["内容审核"]
        VIDEO_CHECK["视频审核<br">帧抽测+OCR"]
        AUDIO_CHECK["音频审核<br">ASR+语义"]
        COMMENT_CHECK["评论审核<br">NLP"]
    end

    subgraph ActionCheck["处置"]
        BLOCK_VIDEO["拦截<br">禁止发布"]
        LIMIT["限制<br">仅自己可见"]
        PASS_VIDEO["通过<br">正常发布"]
        APPEAL["申诉<br">人工复核"]
    end

    UploadCheck --> ContentCheck --> ActionCheck

    style UploadCheck fill:#e3f2fd
    style ContentCheck fill:#ffebee
    style ActionCheck fill:#e8f5e9
```

---

## 八、ACK 阿里云部署架构

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: video-recommendation-service
  namespace: media-platform
spec:
  replicas: 20
  selector:
    matchLabels:
      app: video-recommendation
  template:
    metadata:
      labels:
        app: video-recommendation
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
                      values:
                        - video-recommendation
                topologyKey: kubernetes.io/hostname
      containers:
        - name: recommend
          image: registry.cn-hangzhou.aliyuncs.com/media/recommendation:v3.0
          ports:
            - containerPort: 8080
          env:
            - name: REDIS_CLUSTER
              value: "r-bp1xxxxxxxxx.redis.rds.aliyuncs.com:6379"
            - name: FEATURE_SERVICE_URL
              value: "http://feature-service:8080"
            - name: MODEL_PATH
              value: "/models/din_v3"
          resources:
            requests:
              cpu: "4"
              memory: "8Gi"
            limits:
              cpu: "16"
              memory: "32Gi"
---
# HPA 基于 QPS 扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: video-recommend-hpa
  namespace: media-platform
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: video-recommendation-service
  minReplicas: 20
  maxReplicas: 200
  metrics:
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
        target:
          type: AverageValue
          averageValue: "5000"
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
```

---

## 参考链接

- [阿里云视频点播](https://www.aliyun.com/product/vod)
- [阿里云视频直播](https://www.aliyun.com/product/live)
- [阿里云 RTC](https://www.aliyun.com/product/rtc)
- [FFmpeg 文档](https://ffmpeg.org/documentation.html)
