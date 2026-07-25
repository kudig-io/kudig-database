---
title: 生产运维/topic-publish/video-scripts/ - 数字人视频脚本
description: 'description: ''| 数字人参数 | 形象、声音、语速、分辨率配置 |'''
category: publish
tags:
- k8s
- publish
- release
- etcd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 生产运维/topic-publish/video-scripts/ - 数字人视频脚本 是什么
- 如何 生产运维/topic-publish/video-scripts/ - 数字人视频脚本
trigger_keywords:
- 生产运维
- topic-publish
- video-scripts
- 数字人视频脚本
- publish
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


---
title: 生产运维/topic-publish/video-scripts/ - 数字人视频脚本
description: '| 数字人参数 | 形象、声音、语速、分辨率配置 |'
category: general
tags:
- k8s
- etcd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 生产运维/topic-publish/video-scripts/ - 数字人视频脚本 是什么
- 如何 生产运维/topic-publish/video-scripts/ - 数字人视频脚本
trigger_keywords:
- 生产运维/topic-publish/video-scripts
- 数字人视频脚本

tier: peripheral---

# 生产运维/topic-publish/video-scripts/ - 数字人视频脚本

> 本目录存放由 `scripts/video-content-generator.py` 自动生成的数字人播报脚本。
> 已合并到 生产运维/topic-publish/ 下统一管理。
> 生成的脚本可直接用于腾讯智影、HeyGen、剪映等平台生成视频。

## 工作流程

```
知识库内容 (FTA/FEBM/Skills)
    │
    ▼
video-content-generator.py   ← 生成播报脚本
    │
    ▼
video-generator.py            ← 调用平台 API 生成视频
    │
    ▼
生产运维/topic-publish/video-output/   ← 输出 MP4 视频
```

## 快速开始

```bash
# 第一步：生成视频内容脚本（FTA 故障树）
python3 scripts/video-content-generator.py --type fta --topic pod-crashloop --output 生产运维/topic-publish/video-scripts/pod-crashloop.md

# 生成 Skills 技能视频脚本
python3 scripts/video-content-generator.py --type skill --topic node-notready --output 生产运维/topic-publish/video-scripts/node-notready.md

# 生成 FEBM 取证视频脚本
python3 scripts/video-content-generator.py --type febm --topic etcd-data-integrity --output 生产运维/topic-publish/video-scripts/etcd-febm.md

# 第二步：调用平台生成视频
python3 scripts/video-generator.py --platform tencent \
    --script 生产运维/topic-publish/video-scripts/pod-crashloop.md \
    --avatar professional-engineer \
    --output 生产运维/topic-publish/video-output/pod-crashloop.mp4

# 批量生成
python3 scripts/video-generator.py --batch 生产运维/topic-publish/video-scripts/ \
    --platform tencent --avatar professional-engineer --output-dir 生产运维/topic-publish/video-output/
```

## 查看可用 Topic

```bash
# 列出所有 FTA 故障树 topic
python3 scripts/video-content-generator.py --type fta --list

# 列出所有 Skills topic
python3 scripts/video-content-generator.py --type skill --list

# 列出所有 FEBM topic
python3 scripts/video-content-generator.py --type febm --list
```

## 查看支持的数字人形象

```bash
# 腾讯智影
python3 scripts/video-generator.py --list-avatars --platform tencent

# HeyGen
python3 scripts/video-generator.py --list-avatars --platform heygen
```

## 生成的脚本结构

每个脚本包含以下部分：

| 部分 | 内容 |
|:---|:---|
| 视频结构 | 段落划分、时长、镜头类型 |
| 主播台词 | 可直接使用的解说词 |
| 诊断步骤 | 从文档提取的命令和判定条件 |
| 修复命令 | bash 代码块，可直接复制使用 |
| 数字人参数 | 形象、声音、语速、分辨率配置 |
| 背景素材 | 建议的动画/截图/界面素材 |

## 脚本格式

生成的脚本为 Markdown 格式，可直接用文本编辑器查看和修改。

## 注意事项

1. **API 凭据**：使用前请设置环境变量
   ```bash
   export TENCENT_API_KEY=your_key
   export TENCENT_API_SECRET=your_secret
   export HEYGEN_API_KEY=your_key
   ```

2. **内容审核**：生成的脚本建议人工审核后再提交视频生成

3. **视频长度**：建议每个视频控制在 5-15 分钟

4. **批量处理**：批量生成时请确保 API 配额充足

<!-- risk-assessed -->
