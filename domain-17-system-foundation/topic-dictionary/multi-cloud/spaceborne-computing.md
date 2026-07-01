---
title: 太空计算（Spaceborne Computing）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- nvidia
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 太空计算（Spaceborne Computing） 是什么
- 如何 太空计算（Spaceborne Computing）
trigger_keywords:
- 太空计算
- Spaceborne
- Computing
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# 太空计算（Spaceborne Computing）

## 概述

**太空计算（Spaceborne Computing）** 是将边缘计算和人工智能能力部署到卫星、空间站和其他太空平台上的新兴领域。随着低轨卫星（LEO, Low Earth Orbit）星座（如 Starlink、OneWeb、中国星网）的爆发式增长，以及在轨数据处理需求的激增，[[Kubernetes|Kubernetes]] 和容器化技术正在进入太空。2026 年，NASA、ESA 以及多家商业航天公司已经开始在卫星上运行轻量级 Kubernetes 发行版（如 [[k3s|K3s]]），用于**星上 AI 推理、地球观测数据处理、自主导航和故障检测**。

## 核心概念/原理

### 1. 为什么要在太空运行 Kubernetes

传统航天任务中，卫星收集的原始数据需全部下载到地面站处理，存在以下瓶颈：
- **带宽受限**：卫星与地面站的通信窗口有限，大量原始数据（如高分辨率图像、雷达信号）难以及时回传
- **延迟高**：从数据采集到地面处理可能需要数小时，无法满足实时决策需求
- **地面成本高昂**：建设和维护全球地面站网络需要巨额投资

**星上计算（On-board Processing）** 通过在卫星本地处理数据，仅将结果或关键信息回传地面，显著提升了任务效率。

### 2. 太空计算环境特点

在卫星上运行软件与地球数据中心截然不同：
- **极端资源受限**：卫星的 CPU、内存、存储和功耗极其有限（通常只有几瓦到几十瓦）
- **辐射干扰**：宇宙射线可能导致单粒子翻转（SEU），引发内存位翻转或计算错误
- **间歇性通信**：卫星只在经过地面站时才能通信，大部分时间处于离线自治状态
- **热管理挑战**：太空真空环境中散热困难，计算设备必须控制功耗以避免过热
- **不可维护性**：发射后无法像数据中心一样随时更换硬件，必须通过软件更新（OTA）迭代

### 3. 卫星边缘计算架构

```
地面控制中心（Mission Control）
    ↓ 上行指令 / 下发 AI 模型、K8s 配置
LEO 卫星星座
    ├── 卫星 A：光学载荷 + K3s + AI 推理（船舶检测）
    ├── 卫星 B：SAR 雷达 + K3s + 变化检测（建筑、植被）
    ├── 卫星 C：通信中继 + K3s + 数据压缩和路由
    ↓ 下行结果数据（仅传有价值的目标坐标和缩略图）
地面站 / 云数据中心
```

### 4. 星上 AI 推理

现代卫星越来越多地搭载 AI 加速器（如 NVIDIA Jetson、Intel Movidius、Google Coral）：
- **目标检测**：在卫星上实时检测船舶、飞机、车辆、森林火灾
- **图像筛选**：只拍摄云层覆盖率 < 10% 的区域，或将模糊的图像在星上丢弃
- **数据压缩**：使用 AI 编码技术将原始遥感数据压缩 10–100 倍后再回传
- **异常检测**：监测卫星自身传感器数据，提前发现姿态、电源、热控异常

## 关键机制或特性

### 轻量级 Kubernetes 在太空

由于资源受限，卫星上通常运行 **K3s** 或定制的容器运行时：
- **单节点 K3s**：卫星上通常只有一个控制平面/工作节点合一的实例
- **容器镜像精简**：使用 Distroless 或 Alpine 基础镜像，将镜像体积压缩到 MB 级
- **不可变基础设施**： root 文件系统只读，所有配置通过 ConfigMap 和 Secret 注入
- **Graceful Degradation**：当辐射导致某个 Pod 异常时，Kubernetes 自动重启；若节点整体问题，则降级到安全模式

### 容错与辐射加固

- **EDAC（Error Detection and Correction）**：使用带 ECC 的内存检测和纠正位翻转
- **三模冗余（TMR）**：关键计算通过三个独立实例投票表决，屏蔽单点问题
- **Watchdog 机制**：若 K3s 或主应用长时间无响应，硬件 Watchdog 强制重启整个计算单元
- **Checkpoint 与状态恢复**：定期将关键状态持久化到辐射加固的存储器中

### OTA（Over-The-Air）软件更新

卫星发射后，唯一的维护方式就是无线软件更新：
- **GitOps in Space**：地面控制中心通过 GitOps 将新的 AI 模型或应用配置推送到卫星
- **差分更新**：只传输变更的容器层或模型权重，减少上行带宽消耗
- **A/B 升级与回滚**：卫星上保留新旧两个版本，新版本中如出现异常可自动回滚
- **更新窗口管理**：只在卫星与地面站通信的窗口期内执行关键更新

### 星际网络（Delay-Tolerant Networking, DTN）

卫星之间的通信采用 **DTN（延迟容忍网络）** 协议：
- 数据在节点间以"存储-转发"方式传递
- 适用于高延迟、间歇性连接的太空环境
- Kubernetes 上的 DTN 应用通常以 Sidecar 模式运行，与普通业务容器共享网络命名空间

## 使用场景

1. **海事监控卫星**：在轨运行船舶检测 AI 模型，只将发现的可疑船只坐标和图像回传，每天减少 90% 的下行数据量
2. **森林火灾预警**：多光谱卫星实时分析植被温度和烟雾特征，发现火点后立即通过星间链路告警邻近卫星和地面中心
3. **农作物健康监测**：卫星在轨道上直接计算 NDVI（植被指数），仅将异常农田区域的数据传回地面
4. **太空垃圾规避**：卫星通过星载摄像头和雷达数据，自主计算轨道交会风险并执行规避机动
5. **深空探测自主决策**：火星探测器在通信中断期间，依靠本地 Kubernetes + AI 系统自主规划路径和采集样本

## 最佳实践/注意事项

- **极致精简**：卫星上的每个容器、每个库都必须经过严格审查，消除一切不必要的依赖
- **功耗预算优先**：所有计算任务必须服从严格的功耗预算，高负载 AI 推理可能需要错峰运行
- **辐射测试是必需的**：所有星载软硬件必须在地面通过辐射环境模拟测试（如质子加速器和重离子加速器）
- **确定性执行**：避免使用复杂的动态内存分配和垃圾回收，优先使用 Rust/C++ 等确定性语言
- **离线自治设计**：卫星在 90% 的时间处于与地面失联状态，应用必须具备完整的离线决策能力
- **安全隔离**：卫星上的不同载荷任务应在独立的 Namespace 或容器中运行，防止任务间相互干扰
- **日志与遥测管理**：由于下行带宽稀缺，必须对日志进行高度压缩和优先级筛选，只传关键遥测
- **热循环适应**：卫星每 90 分钟绕地球一圈，经历剧烈的温差变化（-150°C 到 +120°C），计算设备必须能承受宽温范围

## 参考链接

- [NASA Spaceborne Computing](https://www.nasa.gov/spaceborne-computing)
- [KubOS - Satellite OS for Space Missions](https://www.kubos.com/)
- [Ubotica - CogniSAT AI for Satellites](https://ubotica.com/)
- [Orbit Fab - Space Infrastructure](https://www.orbitfab.com/)
- [Spire Global - Space-Based Data & Analytics](https://spire.com/)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
