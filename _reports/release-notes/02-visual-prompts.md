---
title: 发布会视觉素材 — AI 图片生成提示词
description: 科技感深蓝色背景, 中央悬浮一个发光的六边形知识网络结构,
category: general
tags:
- k8s
- etcd
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布会视觉素材 — AI 图片生成提示词 是什么
- 如何 发布会视觉素材 — AI 图片生成提示词
trigger_keywords:
- 发布会视觉素材
- AI
- 图片生成提示词
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# 发布会视觉素材 — AI 图片生成提示词

> 用于生成发布会海报、社交媒体配图、PPT 背景等视觉素材

---

## 1. 主海报 (Hero Image)

### 提示词 (中文)

```
科技感深蓝色背景, 中央悬浮一个发光的六边形知识网络结构,
每个节点是一个发光的圆球代表不同技术域 (标注 etcd/networking/security/AI),
节点间有光线连接形成知识图谱, 网络下方有流动的数据粒子流,
整体风格: 未来科技、数据可视化、深蓝+青色渐变,
右下角有 [[entities/kubernetes.md|kubernetes]] 风轮 logo 的抽象化呈现,
16:9 宽幅, 4K 高清, 适合发布会主视觉
```

### 提示词 (English)

```
Futuristic dark blue tech background, a glowing hexagonal knowledge
network floating in the center, each node is a luminous sphere
representing different tech domains (etcd/networking/security/AI),
connected by light beams forming a knowledge graph, flowing data
particles beneath, style: futuristic tech, data visualization,
dark blue to cyan gradient, abstract Kubernetes wheel logo in
bottom right, 16:9 widescreen, 4K, suitable for launch event hero image
```

---

## 2. 六大提问模式图

### 提示词

```
信息图风格, 深色背景, 展示 6 种 AI 对话模式的图标矩阵:
第一行: 放大镜+书本(深度研究), 扳手+警告灯(问题排查), 终端+代码(命令解读)
第二行: 架构图+积木(架构设计), 闪电+表格(速查参考), 路线图+书包(学习路径)
每个图标下方有中文标签, 图标用青色发光效果, 连线形成环形流程,
整体风格: 扁平科技信息图, 适合 PPT 展示
```

---

## 3. 数据亮点图

### 提示词

```
数据可视化风格, 深色背景, 中央展示一组发光的数字:
"3,346 篇知识文档" "40 个知识域" "97 个行业场景" "218 个 CNCF 项目"
数字用大号发光字体, 周围环绕小型图标 (文档/地球/服务器/云),
底部有渐变光带, 整体科技感强烈, 适合发布会数据展示页
```

---

## 4. 知识域全景图

### 提示词

```
树状信息图, 深色背景, 展示 K8s 知识体系全景:
中央是 Kubernetes logo, 周围辐射出 8 个主分支:
控制平面/网络/存储/安全/可观测/工作负载/AI基础设施/平台运维
每个分支下有 3-5 个子节点, 用不同颜色区分,
节点间有虚线连接表示依赖关系,
风格: 技术架构图, 清晰简洁, 适合 PPT 展示
```

---

## 5. 问题排查流程图

### 提示词

```
流程图风格, 深色背景, 展示 AI Agent 的问题排查流程:
用户提问 → Agent 检索知识库 → 匹配诊断模式 → 调用诊断脚本 → 输出结构化方案
每个步骤用圆角矩形表示, 箭头连接, 关键步骤高亮,
左侧有输入示例 (Pod CrashLoopBackOff), 右侧有输出示例 (诊断步骤),
风格: 技术流程图, 青色+白色, 适合 PPT 展示
```

---

## 6. 社交媒体封面 (1:1)

### 提示词

```
正方形科技海报, 深蓝色渐变背景, 中央大字:
"kudig-database" 用白色科技字体,
下方副标题: "3,346 篇 K8s 生产运维知识, 让智能体拥有专家级大脑"
底部有 Kubernetes 风轮 logo 和版本号 v1.0,
四角有发光的数据节点装饰, 整体简洁有力
```

---

## 7. 朋友圈/微信群分享图

### 提示词

```
竖版科技海报 (9:16), 深蓝渐变背景, 从上到下:
顶部: "kudig-database v1.0 发布" 标题
中部: 6 个图标代表 6 大提问模式 (放大镜/扳手/终端/架构图/闪电/路线图)
中下: 核心数据 "3,346 篇 | 40 知识域 | 97 行业 | 218 CNCF"
底部: "让每一个 K8s 问题都有答案" slogan
整体风格: 科技感、简洁、适合移动端阅读
```

---

## 使用建议

1. 主海报用提示词 1, 用于发布会主背景和邀请函
2. 六大模式图用提示词 2, 用于演示环节的过渡页
3. 数据亮点用提示词 3, 用于开场数据展示
4. 知识域全景用提示词 4, 用于产品介绍环节
5. 流程图用提示词 5, 用于技术原理讲解
6. 社交媒体用提示词 6/7, 用于传播推广
