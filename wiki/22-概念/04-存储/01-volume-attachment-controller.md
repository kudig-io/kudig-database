---
sources:
- "存储/K8s存储/19-volume-attachment-controller.md"
title: VolumeAttachment 与 Attach/Detach 控制器
summary: 深度解析 VolumeAttachment 对象、AD Controller 与 CSI external-attacher 的卷挂接/卸接生命周期。
category: concepts
tags:
- volumeattachment
- attach-detach-controller
- csi
- external-attacher
- volume-lifecycle
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 存储架构师
- SRE
- CSI 驱动开发者
estimated_read_time: 20min
intent_queries:
- VolumeAttachment 是什么
- Attach/Detach Controller 如何工作
- CSI external-attacher 原理
- 卷挂接卡住如何排查
trigger_keywords:
- VolumeAttachment
- Attach/Detach
- AD Controller
- external-attacher
- 卷挂接
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/06-存储/01-K8s存储/20-volume-attachment-controller.md|VolumeAttachment 与 Attach/Detach 控制器]]（本页为重排去重桩，正文见主场）
