# 工业视觉检测架构设计 — 阿里云视角

> **适用版本**: Kubernetes v1.29 - v1.33 | **最后更新**: 2026-04-24
> **作者**: 阿里云解决方案架构师 | **标签**: `#工业视觉` `#AOI` `#缺陷检测` `#阿里云`

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

工业视觉检测（AOI）用机器视觉替代人工质检：

| 挑战 | 说明 | 架构影响 |
|:---|:---|:---|
| 检测速度 | 产线高速运动 | 高帧率相机 + 边缘推理 |
| 缺陷多样 | 微小缺陷难识别 | 深度学习 + 数据增强 |
| 产线集成 | 与 PLC/MES 对接 | 标准接口 + 低延迟 |
| 模型迭代 | 新缺陷持续出现 | 数据闭环 + 在线学习 |
| 环境干扰 | 光照/振动/灰尘 | 图像预处理 |

### 1.2 核心场景

- **PCB 检测**: 焊点/元件/线路缺陷
- **半导体检测**: 晶圆/芯片缺陷
- **锂电池检测**: 极片/隔膜/外观缺陷
- **汽车零部件**: 尺寸/表面/装配检测
- **食品医药**: 异物/包装/标签检测

---

## 2. 业务架构

### 2.1 工业视觉检测全景架构

```mermaid
graph TB
    subgraph 硬件层
        H1[工业相机]
        H2[光源控制器]
        H3[运动控制]
        H4[触发传感器]
    end

    subgraph 边缘层
        E1[边缘推理盒]
        E2[图像预处理]
        E3[实时推理]
        E4[结果输出]
    end

    subgraph 平台层
        P1[模型管理]
        P2[数据标注]
        P3[模型训练]
        P4[质量分析]
        P5[设备管理]
    end

    subgraph 集成层
        I1[MES 对接]
        I2[PLC 控制]
        I3[报警系统]
        I4[剔除机构]
    end

    H1 & H2 & H3 & H4 --> E1 & E2 & E3 & E4
    E1 & E2 & E3 & E4 --> P1 & P2 & P3 & P4 & P5
    E4 --> I1 & I2 & I3 & I4
```

### 2.2 检测流水线时序

```mermaid
sequenceDiagram
    participant SENSOR as 触发传感器
    participant CAMERA as 工业相机
    participant EDGE as 边缘推理盒
    participant PLC as PLC 控制器
    participant MES as MES 系统

    SENSOR->>CAMERA: 产品到位触发
    CAMERA->>CAMERA: 高速拍照
    CAMERA->>EDGE: 传输图像
    EDGE->>EDGE: 图像预处理
    EDGE->>EDGE: AI 模型推理
    EDGE->>EDGE: 缺陷判定
    alt 合格
        EDGE->>PLC: OK 信号
        PLC->>MES: 质检通过
    else 不合格
        EDGE->>PLC: NG 信号 + 缺陷坐标
        PLC->>PLC: 触发剔除机构
        PLC->>MES: 质检不合格 + 缺陷类型
        MES->>MES: 记录缺陷数据
    end
```

---

## 3. 技术架构

### 3.1 K8s 部署

```yaml
# 模型训练 GPU Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vision-model-training
  namespace: industrial-visual
spec:
  replicas: 2
  selector:
    matchLabels:
      app: vision-model-training
  template:
    metadata:
      labels:
        app: vision-model-training
    spec:
      nodeSelector:
        accelerator: nvidia-a100
      runtimeClassName: nvidia
      containers:
        - name: trainer
          image: registry.cn-hangzhou.aliyuncs.com/vision/model-training:v2.0.0-gpu
          ports:
            - containerPort: 8080
          env:
            - name: MODEL_TYPE
              value: "yolov8-seg"
            - name: BATCH_SIZE
              value: "32"
          resources:
            requests:
              nvidia.com/gpu: 2
              memory: "64Gi"
              cpu: "16000m"
            limits:
              nvidia.com/gpu: 2
              memory: "128Gi"
              cpu: "32000m"
          volumeMounts:
            - name: dataset-volume
              mountPath: /datasets
            - name: model-output
              mountPath: /models
      volumes:
        - name: dataset-volume
          persistentVolumeClaim:
            claimName: vision-dataset-pvc
        - name: model-output
          persistentVolumeClaim:
            claimName: vision-model-pvc
```

---

## 4. 核心数据流

### 4.1 缺陷数据闭环

```mermaid
flowchart LR
    A[产线检测] --> B[缺陷图片保存]
    B --> C[人工复核]
    C --> D[数据标注]
    D --> E[模型重训练]
    E --> F[模型验证]
    F --> G[边缘部署]
    G --> A
```

---

## 5. 安全与合规

- **产线安全**: 检测系统不影响生产节拍
- **数据安全**: 产品图片保密
- **质量合规**: 检测标准符合行业规范

---

## 6. 可观测性

- **检测速度**: 单件 < 100ms
- **检测准确率**: > 99.5%
- **误检率**: < 0.5%

---

## 7. 阿里云组件映射

| 功能域 | **阿里云云原生方案** |
|:---|:---|
| 容器平台 | **ACK Pro + GPU** |
| AI | **PAI / 视觉智能** |
| 对象存储 | **OSS** |
| 数据库 | **PolarDB** |
| 可观测性 | **ARMS + SLS** |

---

## 8. 生产检查清单

- [ ] 检测速度满足产线节拍
- [ ] 缺陷检出率 > 99.5%
- [ ] 与 PLC/MES 接口联调
- [ ] 光照稳定性验证
- [ ] 模型版本管理规范

---

**维护者**: 阿里云解决方案架构师团队 | **许可证**: MIT
