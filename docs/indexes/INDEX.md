---
title: KUDIG-DATABASE 知识库全局索引
description: 按逻辑分组的完整知识体系导航 | 1300+ 文档 | 41 知识域 + 1 专题
category: general
tags:
- k8s
- etcd
- scheduler
- prometheus
- grafana
- jaeger
- istio
- envoy
- cilium
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- KUDIG-DATABASE 知识库全局索引 是什么
- 如何 KUDIG-DATABASE 知识库全局索引
trigger_keywords:
- KUDIG-DATABASE
- 知识库全局索引
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

# KUDIG-DATABASE 知识库全局索引

> 按逻辑分组的完整知识体系导航 | 1300+ 文档 | 41 知识域 + 1 专题

---

## 📦 开源项目生态索引 (2026-04-24 新增)

**全景图谱**: [OPEN-SOURCE-ECOSYSTEM.md](./OPEN-SOURCE-ECOSYSTEM.md) — 200+ CNCF / K8s 开源项目全景分类索引，含版本速查与选型决策树。

---

## 🏗️ 应用层架构设计专题 (2026-04-24 新增)

**基于 Kubernetes 的行业应用架构设计最佳实践**，覆盖 60 大核心行业场景，每篇含完整 Mermaid 架构图解与生产级 K8s YAML 配置。**阿里云解决方案架构师实战经验**。

### 第一批 (v7): 通用行业场景
- [01-电商系统架构](./domain-20-application-patterns/topic-application-architecture/01-ecommerce-architecture.md) — 秒杀/支付/库存/搜索/多租户
- [02-小程序平台架构](./domain-20-application-patterns/topic-application-architecture/02-mini-program-architecture.md) — 运行时/Serverless/发布审核
- [03-CMS 内容管理架构](./domain-20-application-patterns/topic-application-architecture/03-cms-architecture.md) — Headless/SSG/多语言/工作流
- [04-实时通信 IM/RTC 架构](./domain-20-application-patterns/topic-application-architecture/04-im-rtc-architecture.md) — WebRTC/SFU/全球加速
- [05-在线教育架构](./domain-20-application-patterns/topic-application-architecture/05-online-education-architecture.md) — 直播课堂/考试/白板
- [06-金融科技架构](./domain-20-application-patterns/topic-application-architecture/06-fintech-architecture.md) — 支付核心/风控/合规/PCI-DSS
- [07-物联网 IoT 架构](./domain-20-application-patterns/topic-application-architecture/07-iot-platform-architecture.md) — EMQX/边缘计算/数字孪生
- [08-AI/ML 推理架构](./domain-20-application-patterns/topic-application-architecture/08-ai-ml-inference-architecture.md) — vLLM/DRA GPU/KServe/RAG
- [09-游戏后端架构](./domain-20-application-patterns/topic-application-architecture/09-gaming-backend-architecture.md) — 帧同步/匹配/区服/GPU
- [10-社交媒体架构](./domain-20-application-patterns/topic-application-architecture/10-social-media-architecture.md) — Feed 流/社交图谱/推荐/审核

### 第二批 (v8/v9): 阿里云视角行业场景
- [11-智慧零售架构](./domain-20-application-patterns/topic-application-architecture/11-smart-retail-architecture.md) — 新零售/全渠道/智能补货/会员中台
- [12-智慧物流架构](./domain-20-application-patterns/topic-application-architecture/12-smart-logistics-architecture.md) — 物流追踪/路径优化/仓储自动化/冷链监控
- [13-数字政务架构](./domain-20-application-patterns/topic-application-architecture/13-digital-government-architecture.md) — 一网通办/数据共享/等保合规/信创替代
- [14-智慧医疗架构](./domain-20-application-patterns/topic-application-architecture/14-smart-healthcare-architecture.md) — 互联网医院/电子病历/医学影像/医保对接
- [15-能源电力架构](./domain-20-application-patterns/topic-application-architecture/15-energy-power-architecture.md) — 新能源监控/智能电网/电力交易/边缘计算
- [16-音视频平台架构](./domain-20-application-patterns/topic-application-architecture/16-video-shortform-architecture.md) — 直播/点播/RTC/CDN/内容审核
- [17-SaaS 多租户架构](./domain-20-application-patterns/topic-application-architecture/17-saas-multi-tenant-architecture.md) — 租户隔离/数据分片/计费计量/自助开通
- [18-数据中台架构](./domain-20-application-patterns/topic-application-architecture/18-data-midplatform-architecture.md) — 数据采集/实时计算/数据治理/BI 分析
- [19-云原生 DevOps 架构](./domain-20-application-patterns/topic-application-architecture/19-cloudnative-devops-architecture.md) — GitOps/CI-CD/平台工程/内部开发者平台
- [20-微服务治理架构](./domain-20-application-patterns/topic-application-architecture/20-microservice-governance-architecture.md) — 服务网格/流量治理/可观测性/混沌工程

### 第三批 (v10): 深耕行业场景
- [21-跨境电商架构](./domain-20-application-patterns/topic-application-architecture/21-cross-border-ecommerce.md) — 全球部署/多币种支付/海关申报/跨境物流
- [22-新能源车联网架构](./domain-20-application-patterns/topic-application-architecture/22-nev-connected-vehicle.md) — T-Box 接入/电池管理/OTA/车路协同
- [23-信创替代架构](./domain-20-application-patterns/topic-application-architecture/23-xinchuang-it-innovation.md) — 国产化芯片/操作系统/数据库/去 IOE
- [24-保险科技架构](./domain-20-application-patterns/topic-application-architecture/24-insurtech.md) — 智能核保/AI 理赔/反欺诈/精算定价
- [25-证券量化交易架构](./domain-20-application-patterns/topic-application-architecture/25-quantitative-trading.md) — FPGA 低延迟/高频交易/实时风控/回测
- [26-航空出行架构](./domain-20-application-patterns/topic-application-architecture/26-aviation-travel.md) — GDS 接入/动态运价/出票/收益管理
- [27-酒店旅游架构](./domain-20-application-patterns/topic-application-architecture/27-hospitality-tourism.md) — OTA 平台/动态定价/打包产品/内容社区
- [28-房地产科技架构](./domain-20-application-patterns/topic-application-architecture/28-proptech.md) — 房产交易/智慧社区/BIM/资产管理
- [29-农业物联网架构](./domain-20-application-patterns/topic-application-architecture/29-agritech-iot.md) — 精准农业/环境监测/无人机植保/溯源
- [30-人力资源 SaaS 架构](./domain-20-application-patterns/topic-application-architecture/30-hrtech-saas.md) — 多租户隔离/薪酬核算/招聘/绩效

### 第四批 (v11): 新兴与细分行业场景
- [31-即时零售架构](./domain-20-application-patterns/topic-application-architecture/31-instant-retail.md) — 同城配送/前置仓/运力调度/30分钟达
- [32-智慧餐饮架构](./domain-20-application-patterns/topic-application-architecture/32-smart-restaurant.md) — 扫码点餐/KDS后厨/会员营销/供应链
- [33-跨境电商海外仓架构](./domain-20-application-patterns/topic-application-architecture/33-crossborder-warehouse.md) — WMS/多仓协同/头程物流/库存同步
- [34-体育科技架构](./domain-20-application-patterns/topic-application-architecture/34-sportstech.md) — 智慧场馆/赛事直播/票务/运动健康
- [35-元宇宙数字孪生架构](./domain-20-application-patterns/topic-application-architecture/35-metaverse-digital-twin.md) — 3D渲染/IoT融合/实时交互/GPU集群
- [36-碳资产管理 ESG 架构](./domain-20-application-patterns/topic-application-architecture/36-carbon-esg-management.md) — 碳盘查/区块链存证/ESG报告/绿色金融
- [37-宠物经济架构](./domain-20-application-patterns/topic-application-architecture/37-pet-economy.md) — 宠物电商/服务预约/智能硬件/宠物保险
- [38-供应链金融架构](./domain-20-application-patterns/topic-application-architecture/38-supply-chain-finance.md) — 应收账款/区块链确权/贸易真实性/联盟链
- [39-智慧园区架构](./domain-20-application-patterns/topic-application-architecture/39-smart-campus.md) — 通行管理/智慧停车/能耗优化/安防监控
- [40-云游戏架构](./domain-20-application-patterns/topic-application-architecture/40-cloud-gaming.md) — GPU渲染串流/WebRTC/边缘节点/云存档

### 第五批 (v12): 细分与前沿行业场景
- [41-美妆电商架构](./domain-20-application-patterns/topic-application-architecture/41-beauty-ecommerce.md) — AI试妆/直播带货/内容种草/正品溯源
- [42-二手交易架构](./domain-20-application-patterns/topic-application-architecture/42-secondhand-circular.md) — C2C信用/AI定价/图像搜索/担保交易
- [43-企业即时通讯架构](./domain-20-application-patterns/topic-application-architecture/43-enterprise-im.md) — 千万级长连接/音视频会议/协同办公/开放平台
- [44-数字营销广告科技架构](./domain-20-application-patterns/topic-application-architecture/44-martech-adtech.md) — RTB实时竞价/DSP/SSP/用户画像/反作弊
- [45-智慧港口航运架构](./domain-20-application-patterns/topic-application-architecture/45-smart-port-shipping.md) — TOS码头系统/无人集卡/海关通关/集装箱追踪
- [46-卫星互联网架构](./domain-20-application-patterns/topic-application-architecture/46-satellite-internet.md) — 低轨卫星/星间链路/遥感数据/天地一体
- [47-智慧矿山架构](./domain-20-application-patterns/topic-application-architecture/47-smart-mining.md) — 无人矿卡/智能综采/瓦斯监测/人员定位
- [48-职业教育培训架构](./domain-20-application-patterns/topic-application-architecture/48-vocational-edtech.md) — 虚拟实训/AI监考/区块链证书/就业对接
- [49-直播电商架构](./domain-20-application-patterns/topic-application-architecture/49-livestream-ecommerce.md) — 千万级并发/直播秒杀/弹幕互动/实时大屏
- [50-无人零售架构](./domain-20-application-patterns/topic-application-architecture/50-unmanned-retail.md) — 视觉识别/重力感应/智能补货/动态定价

### 第六批 (v13): 工业与前沿科技场景
- [51-智能制造 MES 架构](./domain-20-application-patterns/topic-application-architecture/51-smart-manufacturing-mes.md) — MES/APS/SCADA/数字孪生/预测性维护/AI质检
- [52-智慧水务架构](./domain-20-application-patterns/topic-application-architecture/52-smart-water.md) — 智慧供水/智慧排水/漏损控制/爆管预警/水质监测
- [53-新零售 DTC 架构](./domain-20-application-patterns/topic-application-architecture/53-new-retail-dtc.md) — 品牌直营/全渠道融合/会员订阅/柔性供应链
- [54-社交游戏元宇宙架构](./domain-20-application-patterns/topic-application-architecture/54-social-gaming-metaverse.md) — 虚拟空间/Avatar/UGC资产/虚拟经济/NFT
- [55-跨境电商独立站架构](./domain-20-application-patterns/topic-application-architecture/55-crossborder-dtc.md) — DTC品牌出海/全球支付/社媒引流/海外仓履约
- [56-智慧养老架构](./domain-20-application-patterns/topic-application-architecture/56-smart-elderly-care.md) — 跌倒检测/健康监测/紧急救助/智能照护/服务聚合
- [57-数字疗法架构](./domain-20-application-patterns/topic-application-architecture/57-digital-therapeutics.md) — DTx/远程诊疗/AI自适应治疗/区块链证书/疗效评估
- [58-Web3 GameFi 架构](./domain-20-application-patterns/topic-application-architecture/58-web3-gamefi.md) — 链游/NFT铸造/代币经济/P2E/智能合约
- [59-工业互联网平台架构](./domain-20-application-patterns/topic-application-architecture/59-industrial-internet-platform.md) — IIoT/设备上云/工业APP/数字孪生/产能共享
- [60-车路协同自动驾驶架构](./domain-20-application-patterns/topic-application-architecture/60-v2x-autonomous-driving.md) — V2X/协同感知/高精地图/数据闭环/远程接管

### 第七批 (v14): 能源与前沿科技场景
- [61-智慧电网架构](./domain-20-application-patterns/topic-application-architecture/61-smart-grid.md) — 虚拟电厂/负荷预测/新能源预测/需求响应/源网荷储协同
- [62-分布式能源架构](./domain-20-application-patterns/topic-application-architecture/62-distributed-energy.md) — 光伏/储能/微电网/EMS能量管理/碳资产
- [63-工业视觉检测架构](./domain-20-application-patterns/topic-application-architecture/63-industrial-visual-inspection.md) — AOI/缺陷检测/PCB/半导体/锂电池/数据闭环
- [64-AI 制药架构](./domain-20-application-patterns/topic-application-architecture/64-ai-drug-discovery.md) — 分子生成/分子动力学/临床试验/老药新用
- [65-自动驾驶仿真架构](./domain-20-application-patterns/topic-application-architecture/65-autonomous-driving-sim.md) — SIL/HIL/场景生成/传感器仿真/数据闭环
- [66-太空互联网架构](./domain-20-application-patterns/topic-application-architecture/66-space-internet.md) — 低轨卫星/星间链路/遥感服务/物联网/应急通信
- [67-脑机接口架构](./domain-20-application-patterns/topic-application-architecture/67-brain-computer-interface.md) — 神经信号采集/运动想象解码/医疗康复/神经隐私
- [68-量子计算云平台架构](./domain-20-application-patterns/topic-application-architecture/68-quantum-computing-cloud.md) — 量子计算服务/混合计算/量子模拟/量子机器学习
- [69-6G 核心网架构](./domain-20-application-patterns/topic-application-architecture/69-6g-core-network.md) — 通感一体/智能超表面/空天地一体化/算网融合/全息通信
- [70-数字人民币架构](./domain-20-application-patterns/topic-application-architecture/70-ecny-cbdc.md) — e-CNY/双离线支付/智能合约/可控匿名/跨境支付

### 第八批 (v15): 政务与极端科技场景
- [71-智慧税务架构](./domain-20-application-patterns/topic-application-architecture/71-smart-tax.md) — 电子税务局/全电发票/税务风控/大数据治税/银税互动
- [72-数字孪生城市架构](./domain-20-application-patterns/topic-application-architecture/72-digital-twin-city.md) — CIM平台/城市大脑/三维可视化/城市规划仿真/应急指挥
- [73-智慧消防架构](./domain-20-application-patterns/topic-application-architecture/73-smart-firefighting.md) — 消防物联网/AI火眼/应急指挥/消防设施监测/安全评估
- [74-沉浸式 XR 架构](./domain-20-application-patterns/topic-application-architecture/74-immersive-xr.md) — VR/AR/MR/空间计算/云渲染/数字人交互
- [75-情感计算 AI 架构](./domain-20-application-patterns/topic-application-architecture/75-affective-computing.md) — 情绪识别/多模态融合/智能客服/心理评估/驾驶员监测
- [76-合成生物学架构](./domain-20-application-patterns/topic-application-architecture/76-synthetic-biology.md) — 基因设计/蛋白质工程/自动化实验/AlphaFold/生物制造
- [77-可控核聚变监控架构](./domain-20-application-patterns/topic-application-architecture/77-fusion-energy-monitoring.md) — 托卡马克/等离子体控制/偏滤器监测/中子测量/长脉冲运行
- [78-深海探测架构](./domain-20-application-patterns/topic-application-architecture/78-deep-sea-exploration.md) — 载人潜水器/ROV/AUV/海底观测网/声通信/资源勘探
- [79-极地科考架构](./domain-20-application-patterns/topic-application-architecture/79-polar-research.md) — 冰川监测/气象观测/生态研究/天文观测/海洋调查/卫星回传
- [80-TSN时间敏感网络架构](./domain-20-application-patterns/topic-application-architecture/80-tsn-network.md) — 确定性网络/工业以太网/IEEE1588/门控调度/汽车网络

#### 第九批 (v16, 81-90): 前沿科技与基础设施

- [81-智慧海关架构](./domain-20-application-patterns/topic-application-architecture/81-smart-customs.md) — AI审图/风险布控/跨境电商通关/冷链监管/智慧口岸
- [82-司法科技架构](./domain-20-application-patterns/topic-application-architecture/82-legaltech.md) — 智能审判/区块链存证/智慧法院/类案推送/量刑辅助
- [83-文化数字化架构](./domain-20-application-patterns/topic-application-architecture/83-cultural-digitization.md) — 文物三维/数字博物馆/非遗传承/古籍保护/知识图谱
- [84-国家公园架构](./domain-20-application-patterns/topic-application-architecture/84-national-park.md) — 生态监测/野生动物AI/智慧巡护/防火预警/游客服务
- [85-氢能源架构](./domain-20-application-patterns/topic-application-architecture/85-hydrogen-energy.md) — 绿氢制备/加氢站安全/燃料电池/储运管理/氢能车辆
- [86-固态电池架构](./domain-20-application-patterns/topic-application-architecture/86-solid-state-battery.md) — 材料模拟/BMS/分子动力学/高通量计算/安全测试
- [87-柔性制造架构](./domain-20-application-patterns/topic-application-architecture/87-flexible-manufacturing.md) — C2M定制/智能排产/数字主线/产线重构/供应链协同
- [88-纳米材料架构](./domain-20-application-patterns/topic-application-architecture/88-nanomaterials.md) — 高通量筛选/材料基因组/分子模拟/性能预测/安全评估
- [89-CRISPR基因编辑架构](./domain-20-application-patterns/topic-application-architecture/89-crispr-gene-editing.md) — gRNA设计/脱靶检测/基因治疗/功能筛选/伦理合规
- [90-类脑计算架构](./domain-20-application-patterns/topic-application-architecture/90-neuromorphic-computing.md) — 脉冲神经网络/神经形态芯片/边缘智能/脑机接口/机器人控制

[查看完整 README](./domain-20-application-patterns/topic-application-architecture/README.md)

**各 Domain 项目索引**: 全部 40 个 Domain 已新增 `00-open-source-projects-index.md`，覆盖项目清单、版本信息、兼容矩阵与选型指南。

**深度项目指南 (26 篇)**:
- [Prometheus 企业监控部署指南](./domain-06-observability/99-prometheus-enterprise-guide.md)
- [分布式追踪实践指南 (Jaeger/Tempo/OpenTelemetry)](./domain-06-observability/99-distributed-tracing-guide.md)
- [KEDA 事件驱动自动缩放指南](./domain-11-production-operations/99-keda-event-driven-autoscaling-guide.md)
- [Argo CD GitOps 实践指南](./domain-08-release-change-management/99-argo-cd-gitops-guide.md)
- [Flux GitOps 实践指南](./domain-08-release-change-management/99-flux-gitops-guide.md)
- [Tekton CI/CD 实践指南](./domain-08-release-change-management/99-tekton-cicd-guide.md)
- [Istio 服务网格入门指南](./domain-03-networking-traffic/99-istio-service-mesh-guide.md)
- [Linkerd 轻量级服务网格指南](./domain-03-networking-traffic/99-linkerd-service-mesh-guide.md)
- [Harbor 企业镜像仓库指南](./domain-13-container-runtime/99-harbor-enterprise-guide.md)
- [Falco 运行时安全监控指南](./domain-05-security-compliance/99-falco-runtime-security-guide.md)
- [Kyverno K8s 原生策略管理指南](./domain-05-security-compliance/99-kyverno-policy-guide.md)
- [OPA Gatekeeper 策略即代码指南](./domain-05-security-compliance/99-opa-gatekeeper-policy-guide.md)
- [Vault K8s 密钥管理指南](./domain-05-security-compliance/99-vault-k8s-secrets-guide.md)
- [cert-manager TLS 证书管理指南](./domain-05-security-compliance/99-cert-manager-tls-guide.md)
- [SLSA 供应链安全实践指南](./domain-05-security-compliance/99-slsa-supply-chain-security-guide.md)
- [Crossplane 平台工程实践指南](./domain-08-release-change-management/99-crossplane-platform-guide.md)
- [Cilium eBPF 网络与安全指南](./domain-03-networking-traffic/99-cilium-ebpf-network-guide.md)
- [Envoy Gateway API Gateway 指南](./domain-03-networking-traffic/99-envoy-gateway-enterprise-guide.md)
- [Kubeflow AI 平台部署指南](./domain-14-ai-ml-infra/99-kubeflow-ai-platform-guide.md)
- [Backstage 内部开发者平台 (IDP) 构建指南](./domain-07-platform-engineering/99-backstage-idp-guide.md)
- [Serverless / FaaS 实践指南 (Knative/OpenFunction)](./domain-15-specialized-tech/99-serverless-faas-guide.md)
- [Kubernetes v1.29-v1.33 版本特性深度指南](./domain-01-cluster-fundamentals/99-kubernetes-v1.29-v1.33-features-guide.md)
- [Kubernetes 核心组件 v1.29-v1.33 新特性速查](./domain-01-cluster-fundamentals/99-kubernetes-core-components-v1.29-v1.33-update.md)
- [Kubernetes v1.33 升级实操指南](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-upgrade-guide.md)
- [Kubectl v1.29-v1.33 新命令与用法速查](./domain-01-cluster-fundamentals/99-kubectl-v1.29-v1.33-new-commands-guide.md)
- [Kubernetes v1.33 生产环境最佳实践](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-production-best-practices.md)
- [Kubernetes 版本生命周期与支持策略](./domain-01-cluster-fundamentals/99-kubernetes-version-lifecycle-support-policy.md)
- [Kubernetes v1.33 生态系统兼容性矩阵](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-ecosystem-compatibility-matrix.md)
- [Kubernetes v1.33 一页纸速查卡](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-quick-reference-card.md)
- [Kubernetes v1.33 弃用功能与迁移指南](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-deprecation-migration-guide.md)
- [Kubernetes v1.25-v1.33 特性对比总表](./domain-01-cluster-fundamentals/99-kubernetes-v1.25-v1.33-feature-comparison-table.md)
- [Kubernetes v1.29-v1.33 完整 Feature Gate 参考手册](./domain-01-cluster-fundamentals/99-kubernetes-v1.29-v1.33-complete-feature-gates-reference.md)
- [Kubernetes v1.33 实战案例集 (14个案例)](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-practical-cookbook.md)
- [Kubernetes v1.29-v1.33 核心特性架构图集](./domain-01-cluster-fundamentals/99-kubernetes-core-features-mermaid-diagrams.md)
- [Kubernetes 生产环境完整架构蓝图](./domain-11-production-operations/99-kubernetes-production-architecture-blueprint.md)
- [Kubernetes 部署模式架构详解](./domain-11-production-operations/99-kubernetes-deployment-patterns-architecture.md)
- [Kubernetes 多租户与资源隔离架构](./domain-11-production-operations/99-kubernetes-multi-tenant-architecture.md)
- [Kubernetes v1.29-v1.33 设计原理演进与影响分析](./domain-01-cluster-fundamentals/99-kubernetes-v1.33-design-principles-evolution.md)
- [Kubernetes v1.29-v1.33 工作负载管理新特性指南](./domain-02-workloads-applications/99-kubernetes-v1.33-workloads-guide.md)
- [Kubernetes v1.29-v1.33 可观测性新特性指南](./domain-06-observability/99-kubernetes-v1.33-observability-guide.md)
- [Kubernetes v1.29-v1.33 平台运维新特性指南](./domain-07-platform-engineering/99-kubernetes-v1.33-platform-ops-guide.md)
- [CloudNativePG 企业级 PostgreSQL 指南](./domain-16-database-middleware/99-cloudnativepg-enterprise-guide.md)
- [Velero 备份恢复指南](./domain-09-reliability-engineering/99-velero-backup-recovery-guide.md)
- [FinOps 成本优化指南 (Kubecost/OpenCost/Infracost)](./domain-11-production-operations/99-finops-cost-optimization-guide.md)
- [Karpenter 节点自动扩展指南](./domain-11-production-operations/99-karpenter-node-autoscaling-guide.md)
- [WebAssembly 云原生实践指南 (WasmEdge/Spin)](./domain-15-specialized-tech/99-wasmedge-cloud-native-guide.md)
- [K8s 开发者工具链指南 (k9s/Headlamp/stern)](./domain-15-specialized-tech/99-kubernetes-developer-toolchain-guide.md)

**Java/Spring 技术栈深度指南 (2026-04-30 新增, 12 篇)**:
- [Java on Kubernetes 综合实践指南 (统一入口)](./domain-02-workloads-applications/topic-java-kubernetes/README.md) — 一站式 Java + K8s 知识导航
- [Java 容器化最佳实践 (Dockerfile/Jib/Buildpacks/分层JAR)](./domain-13-container-runtime/12-java-containerization-guide.md)
- [GraalVM Native Image 云原生实践指南](./domain-15-specialized-tech/99-graalvm-native-image-guide.md)
- [Spring Boot on Kubernetes 生产实践指南](./domain-02-workloads-applications/99-spring-boot-kubernetes-guide.md)
- [Spring Cloud Kubernetes 与服务网格集成指南](./domain-03-networking-traffic/99-spring-cloud-kubernetes-service-mesh-guide.md)
- [JVM GC 容器调优深度指南 (G1/ZGC/Shenandoah)](./domain-10-troubleshooting-diagnostics/99-jvm-gc-container-tuning-guide.md)
- [Java 应用安全加固指南](./domain-05-security-compliance/99-java-security-kubernetes-guide.md)
- [Tekton Java CI/CD 流水线实践指南](./domain-08-release-change-management/99-tekton-java-cicd-guide.md)
- [Java 应用可观测性整合指南 (Micrometer+OTel+Prometheus)](./domain-06-observability/99-java-observability-kubernetes-guide.md)
- [Java K8s Client 与 Operator SDK 开发指南](./domain-07-platform-engineering/99-java-k8s-client-operator-guide.md)
- [Java 性能调优与资源 Sizing 指南](./domain-10-troubleshooting-diagnostics/99-java-performance-resource-sizing-guide.md)
- [Quarkus/Micronaut 云原生 Java 框架实践指南](./domain-15-specialized-tech/99-quarkus-micronaut-cloud-native-java-guide.md)

**快速选型**: [OPEN-SOURCE-SELECTION-GUIDE.md](./OPEN-SOURCE-SELECTION-GUIDE.md) — 按场景/角色的 30 秒快速选型索引。

**模板工具**: [PROJECT-INDEX-TEMPLATE.md](./PROJECT-INDEX-TEMPLATE.md) — 标准化项目索引模板，供后续快速扩展复用。

---

---

## 核心知识域 (Core Kubernetes)

基础架构、设计原理到问题排查的完整 Kubernetes 技术栈。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 1 | [架构基础](./domain-01-cluster-fundamentals/) | 18 | K8s 架构、核心组件、升级策略、性能调优、安全架构 |
| 2 | [设计原理](./domain-01-cluster-fundamentals/) | 18 | 声明式 API、控制器模式、etcd 共识、Operator 开发 |
| 3 | [控制平面](./domain-01-cluster-fundamentals/) | 30 | etcd、API Server、Scheduler、KCM、CRI/CSI/CNI |
| 4 | [工作负载](./domain-02-workloads-applications/) | 25 | Pod 生命周期、调度器、HPA/VPA、资源管理 |
| 5 | [网络](./domain-03-networking-traffic/) | 41 | CNI、Service、DNS、Ingress、Gateway API |
| 6 | [存储](./domain-04-storage-data/) | 17 | PV/PVC、StorageClass、CSI 驱动、备份恢复 |
| 7 | [安全合规](./domain-05-security-compliance/) | 21 | RBAC、网络安全、运行时安全、审计合规 |
| 8 | [可观测性](./domain-06-observability/) | 30 | 监控指标、日志审计、链路追踪、混沌工程 |
| 9 | [平台运维](./domain-07-platform-engineering/) | 25 | 集群管理、GitOps、成本优化、灾备恢复 |
| 10 | [扩展生态](./domain-15-specialized-tech/) | 16 | CRD/Operator、Helm、CI/CD、服务网格 |
| 11 | [AI 基础设施](./domain-14-ai-ml-infra/) | 36 | GPU 调度、分布式训练、LLM 推理、成本优化 |
| 12 | [问题排查](./domain-10-troubleshooting-diagnostics/) | 42+ | 全组件问题排查、结构化排障 |

---

## 底层基础 (Infrastructure)

Docker、Linux、网络存储底层原理和硬件知识。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 13 | [Docker](./domain-13-container-runtime/) | 12 | 架构、镜像、容器、网络、存储、安全、排障 |
| 14 | [Linux](./domain-17-system-foundation/) | 10 | 系统架构、进程、文件系统、网络、安全、容器基础 |
| 15 | [网络基础](./domain-03-networking-traffic/) | 6 | OSI/TCP-IP、DNS、负载均衡、SDN |
| 16 | [存储基础](./domain-04-storage-data/) | 6 | 存储架构、RAID、分布式系统 |
| 17 | [云厂商](./domain-12-cloud-providers/) | 13家 | 阿里云 ACK、AWS EKS、GCP GKE、Azure AKS 等 |
| 31 | [硬件](./domain-17-system-foundation/) | 18 | CPU、内存、存储、网络硬件、问题排查 |

---

## 企业级专题 (Enterprise)

面向生产环境的运维实践、监控日志、安全合规等。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 18 | [生产运维](./domain-11-production-operations/) | 24 | 架构设计、零信任、GitOps、FinOps、灾备 |
| 19 | [技术白皮书](./domain-19-landscape-references/) | 26 | 深度技术专题、最佳实践白皮书 |
| 20 | [企业监控告警](./domain-06-observability/) | 10 | Prometheus、Grafana、Datadog、Elastic |
| 21 | [日志管理](./domain-06-observability/) | 9 | ELK、Fluentd、Loki、Graylog |
| 22 | [容器镜像管理](./domain-13-container-runtime/) | 7 | Harbor、JFrog、Quay |
| 23 | [GitOps CI/CD](./domain-08-release-change-management/) | 4 | Argo CD、Jenkins、GitHub Actions |
| 24 | [基础设施即代码](./domain-08-release-change-management/) | 5 | Terraform、Ansible、Pulumi |
| 25 | [云原生安全](./domain-05-security-compliance/) | 5 | Falco、Sysdig、Kyverno、Vault |
| 26 | [服务网格](./domain-03-networking-traffic/) | 6 | Istio、Linkerd、Envoy、Dapr |
| 27 | [多云混合](./domain-12-cloud-providers/) | 5 | AWS/Azure/GCP/IBM 多云治理 |
| 28 | [数据库中间件](./domain-16-database-middleware/) | 5 | MySQL、PostgreSQL、Redis、MongoDB |
| 29 | [自动化测试](./domain-08-release-change-management/) | 5 | Selenium、Playwright、AI 测试 |
| 30 | [灾备恢复](./domain-09-reliability-engineering/) | 5 | VMware、Veeam、混沌工程 |

---

## 前沿技术 (Advanced)

CNCF 生态、eBPF、平台工程、边缘计算等前沿领域。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 32 | [YAML 配置清单](./domain-18-manifests-patterns/) | 36 | K8s 全资源 YAML 参考手册 |
| 33 | [Kubernetes Events](./domain-17-system-foundation/) | 15 | 事件体系完整解析 |
| 34 | [CNCF Landscape](./domain-19-landscape-references/) | 218 | Graduated/Incubating/Sandbox 全量项目 |
| 35 | [eBPF 技术](./domain-03-networking-traffic/) | 10 | eBPF 原理、Cilium、可观测性 |
| 36 | [平台工程](./domain-07-platform-engineering/) | 11 | IDP 内部开发者平台 |
| 37 | [边缘计算](./domain-15-specialized-tech/) | 10 | KubeEdge、边缘部署 |
| 38 | [WebAssembly](./domain-15-specialized-tech/) | 10 | Wasm 云原生工作负载 |
| 39 | [供应链安全](./domain-05-security-compliance/) | 10 | SBOM、SLSA、Sigstore |
| 40 | [云原生 API 网关](./domain-03-networking-traffic/) | 14 | Gateway API、Higress、APISIX |

---

## 方法论与实践 (Methodology)

FTA 问题树、FEBM 取证、Skills 技能库等独创方法论。

| 专题 | 文档数 | 说明 |
|:---|:---:|:---|
| [FTA 问题树分析](./domain-10-troubleshooting-diagnostics/topic-fta/) | 29+36 | 方法论 23 篇 + 36 个组件问题树 |
| [FEBM 取证循证](./domain-10-troubleshooting-diagnostics/topic-febm/) | 9 | 从证据到结论的归纳式方法论 |
| [Skills 运维技能库](./domain-10-troubleshooting-diagnostics/topic-skills/) | 18 | 生产级诊断-修复闭环 |
| [结构化问题排查](./domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/) | 49 | 12 个分类 × 结构化流程 + 配置优先方法论 |
| [配置优先排查方法论](./domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/00-configuration-first-methodology.md) | 1 | 疑难问题系统性排查：先配置后链路，CoreDNS 完整示例 |
| [运维词典](./domain-17-system-foundation/topic-dictionary/) | 200+ | 13 个分类的运维知识条目 |

---

## 学习与参考 (Reference)

速查卡、学习计划、部署方案等参考资料。

| 专题 | 文档数 | 说明 |
|:---|:---:|:---|
| [速查卡](./domain-17-system-foundation/topic-cheat-sheet/) | 9 | K8s/Linux/Docker/PromQL/Git/SQL 等 |
| [学习计划](./domain-11-production-operations/topic-learn/) | 46 | 1 个月系统化学习路径 |
| [部署方案](./domain-08-release-change-management/topic-deployment/) | 4 | 从 Demo 到生产的渐进式部署 |
| [集群迁移](./domain-08-release-change-management/topic-migration/) | 10 | 10 步完整迁移指南 |
| [Release Notes](./domain-19-landscape-references/_archived-release-notes/) | 1300+ | K8s 及生态组件版本说明 |
| [Manpage](./man/) | 14 | Unix manpage 参考手册 |

---

## AI 工程 (AI Engineering)

AI Agent、AI Coding 工具相关知识体系。

| 专题 | 文档数 | 说明 |
|:---|:---:|:---|
| [AI Agent 工程](./domain-14-ai-ml-infra/topic-ai-agent/) | 50 | Agent 基础→Harness 工程→OpenClaw |
| [AI Coding 工具](./domain-14-ai-ml-infra/topic-ai-coding/) | 24 | OpenRouter、OpenCode |

---

## 工具与发布 (Tooling)

项目工具、脚本、GitBook 和发布计划。

| 目录 | 说明 |
|:---|:---|
| [scripts/](./scripts/) | 统计、质量检查、FTA 可视化等脚本 |
| [gitbook/](./gitbook/) | mdBook 本地文档浏览系统 |
| [reports/](./reports/) | 质量报告、统计数据 |
| [templates/](./templates/) | 文档模板（Domain/FTA/Skill/速查卡） |
| [domain-11-production-operations/topic-publish/](./domain-11-production-operations/topic-publish/) | 内容发布计划和路线图 |

## Related

- [[references/k8s-glossary-index|K8s 术语表索引]] — Cross-reference
- [[references/k8s-knowledge-map|Kubernetes Knowledge Map]] — Cross-reference
- [[references/KUDIG Templates and Agent Prompts|KUDIG Templates and Agent Prompts]] — Cross-reference
- [[references/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]] — Cross-reference
- [[skills/Symptom Vector Matching Engine|Symptom Vector Matching Engine]] — Cross-reference
