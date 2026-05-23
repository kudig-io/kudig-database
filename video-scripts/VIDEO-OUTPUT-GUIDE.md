---
title: 数字人视频输出建议 (video-scripts)
description: 本文档提供数字人视频生成的配置建议，包括平台选择、参数设置、内容优化等。
category: general
tags:
- k8s
- etcd
- hpa
- vpa
- ingress
- rbac
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 数字人视频输出建议 是什么
- 如何 数字人视频输出建议
trigger_keywords:
- 数字人视频输出建议
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# 数字人视频输出建议

> 本文档提供数字人视频生成的配置建议，包括平台选择、参数设置、内容优化等。

---

## 平台选择指南

### 腾讯智影（推荐国内）

| 指标 | 说明 |
|:---|:---|
| 优势 | 中文数字人丰富、本土化、API 稳定 |
| 劣势 | 需要国内企业认证、个人用户限额 |
| 适用场景 | 技术培训、故障排查演示、内部分享 |
| 价格 | 按分钟计费，有免费额度 |

**推荐配置：**
- 分辨率：1920x1080
- 帧率：30fps
- 数字人：professional-engineer（男声，专业工程师形象）
- 语速：1.2x

### HeyGen（国际版）

| 指标 | 说明 |
|:---|:---|
| 优势 | 英文数字人多、质量高、API 完善 |
| 劣势 | 中文支持较弱、需要海外账户 |
| 适用场景 | 英文技术分享、开源社区、国际化内容 |
| 价格 | 按分钟计费，批量有折扣 |

**推荐配置：**
- 分辨率：1280x720（英文场景）
- 数字人：1_english_professional
- 语速：1.0x

### 剪映（本地）

| 指标 | 说明 |
|:---|:---|
| 优势 | 免费、本地处理、无需上传 |
| 劣势 | API 不稳定、需要手动操作 |
| 适用场景 | 测试、小批量、快速预览 |
| 价格 | 免费（需开启开发者模式） |

---

## 视频参数配置

### 分辨率

| 用途 | 分辨率 | 说明 |
|:---|:---|:---|
| 标准 | 1920x1080 | 主流，适合大多数场景 |
| 高清 | 2560x1440 | 适合大屏展示、培训 |
| 竖屏 | 1080x1920 | 抖音、快手等短视频平台 |
| 宽屏 | 1920x1200 | 16:10 比例 |

### 时长控制

| 类型 | 建议时长 | 说明 |
|:---|:---|:---|
| 单一问题 | 5-8 分钟 | 一个故障树/技能 |
| 综合培训 | 10-15 分钟 | 多个相关技能组合 |
| 快速概览 | 2-3 分钟 | 高层介绍，无细节 |
| 深度讲解 | 15-20 分钟 | 完整问题复盘 |

### 字幕配置

- **字幕**：建议添加，便于听力障碍用户和静音观看
- **位置**：底部居中，黑底白字
- **字体**：系统默认或思源黑体

---

## 内容优化建议

### 主播台词优化

1. **开头**：前 30 秒抓住注意力
   ```
   ❌ "今天我们讲一个故障排查..."
   ✅ "Pod 反复重启，exit code 137，一小时损失 50 万！"
   ```

2. **语速**：诊断部分 1.0x，操作部分 1.3x

3. **停顿**：关键命令执行后停顿 1-2 秒

4. **方言**：普通话为主，避免专业术语英文发音

### 视觉素材建议

| 类型 | 素材 | 说明 |
|:---|:---|:---|
| 动画 | 故障树 Mermaid | 展示 TE→IE→BE 层级关系 |
| 截图 | kubectl 输出 | 真实命令输出，非模拟 |
| 界面 | [[entities/kubernetes|k8s]] Dashboard | 监控面板、事件日志 |
| 图示 | 架构图 | 网络拓扑、组件交互 |

### 背景音乐

- **不建议**：技术培训以清晰为主
- **如需**：选择轻音乐，音量 < 10%

---

## 批量生成建议

### 生产流程

```
1. 脚本生成 (video-content-generator.py)
   ↓
2. 人工审核脚本（30分钟/个）
   ↓
3. 批量提交视频生成
   ↓
4. 下载 MP4（每个约 5-15 分钟）
   ↓
5. 后期剪辑（添加片头/片尾）
   ↓
6. 上传平台
```

### 批量提交策略

| 平台 | 每日限额 | 建议 |
|:---|:---|:---|
| 腾讯智影 | 100 分钟 | 每天 10 个视频 |
| HeyGen | 60 分钟 | 每天 6 个视频 |
| 剪映 | 无限制 | 按需提交 |

### 优先级排序

建议按以下优先级生成视频：

1. **P0**：高频问题（Pod CrashLoop、Node NotReady、DNS 问题）
2. **P1**：核心技能（证书过期、存储问题、网络连通性）
3. **P2**：进阶专题（HPA/VPA、RBAC、Ingress）
4. **P3**：深度专题（控制平面、etcd、Operator）

---

## 输出格式规范

### 文件命名

```
{topic}-{date}-{platform}.mp4

示例：
pod-crashloop-20260518-tencent.mp4
node-notready-20260518-heygen.mp4
```

### 目录结构

```
video-output/
├── pod-crashloop-20260518-tencent.mp4
├── node-notready-20260518-tencent.mp4
├── fta-pod-20260518-tencent.mp4
└── metadata/
    ├── pod-crashloop-20260518-tencent.meta.json
    └── batch-20260518.json
```

### 元数据记录

每个视频生成后，记录以下信息：

```json
{
  "title": "Pod CrashLoop 故障排查",
  "source_script": "video-scripts/pod-crashloop.md",
  "platform": "tencent",
  "avatar": "professional-engineer",
  "duration_min": 7,
  "generated_at": "2026-05-18T20:30:00Z",
  "status": "completed"
}
```

---

## 常见问题

### Q: 数字人声音不自然？
**A**: 选择"专业"级别数字人，避免使用免费/基础版本

### Q: 视频生成失败？
**A**: 检查 API 配额、脚本格式、敏感词过滤

### Q: 如何批量管理？
**A**: 使用 `video-generator.py --batch` 批量处理，元数据自动记录

### Q: 需要人工配音吗？
**A**: 建议先使用数字人快速产出，后续再考虑专业配音

---

## 平台 API 申请

### 腾讯智影
- 官网：https://vcdn.zxin.com/
- 需要：企业认证、API Key/Secret
- 环境变量：`TENCENT_API_KEY`, `TENCENT_API_SECRET`

### HeyGen
- 官网：https://www.heygen.com/
- 需要：账号、API Key
- 环境变量：`HEYGEN_API_KEY`

### 剪映
- 下载剪映桌面版
- 设置 → 开发者模式 → 开启
- 本地 WebSocket 连接

---

## 联系与反馈

视频生成过程中遇到问题，请：
1. 检查 `scripts/video-generator.py --help` 查看用法
2. 查看 `scripts/video-content-generator.py --help` 查看脚本生成
3. 提交 GitHub Issue 反馈问题