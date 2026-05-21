---
title: video-scripts/ - 数字人视频脚本
description: '| 数字人参数 | 形象、声音、语速、分辨率配置 |'
category: general
tags:
- k8s
- etcd
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- video-scripts/ - 数字人视频脚本 是什么
- 如何 video-scripts/ - 数字人视频脚本
trigger_keywords:
- video-scripts
- 数字人视频脚本
prerequisites:
- kubectl-basics
- etcd-basics
- gpu-scheduling-basics
---

# video-scripts/ - 数字人视频脚本

> 本目录存放由 `scripts/video-content-generator.py` 自动生成的数字人播报脚本。
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
video-output/                 ← 输出 MP4 视频
```

## 快速开始

```bash
# 第一步：生成视频内容脚本（FTA 故障树）
python3 scripts/video-content-generator.py --type fta --topic pod-crashloop --output video-scripts/pod-crashloop.md

# 生成 Skills 技能视频脚本
python3 scripts/video-content-generator.py --type skill --topic node-notready --output video-scripts/node-notready.md

# 生成 FEBM 取证视频脚本
python3 scripts/video-content-generator.py --type febm --topic etcd-data-integrity --output video-scripts/etcd-febm.md

# 第二步：调用平台生成视频
python3 scripts/video-generator.py --platform tencent \
    --script video-scripts/pod-crashloop.md \
    --avatar professional-engineer \
    --output video-output/pod-crashloop.mp4

# 批量生成
python3 scripts/video-generator.py --batch video-scripts/ \
    --platform tencent --avatar professional-engineer --output-dir video-output/
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

## Related

- [[domain-19-landscape-references/98-merged-indexes/README-from-domain-19-landscape-references|Domain-34: CNCF Landscape 开源项目]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[domain-03-networking-traffic/98-merged-indexes/MOC-from-domain-03-networking-traffic|domain-03-networking-traffic MOC]] — Cross-reference
- [[domain-20-application-patterns/98-merged-indexes/README-from-domain-20-application-patterns|Topic 应用层架构设计最佳实践]] — Cross-reference
- [[domain-20-application-patterns/98-merged-indexes/MOC-from-domain-20-application-patterns|topic-application-architecture MOC]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- [[domain-08-release-change-management/98-merged-indexes/MOC-from-domain-08-release-change-management|domain-08-release-change-management MOC]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- [[domain-09-reliability-engineering/98-merged-indexes/README-from-domain-09-reliability-engineering|Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity)]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
