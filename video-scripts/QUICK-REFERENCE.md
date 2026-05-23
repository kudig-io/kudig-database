---
title: 数字人视频快速参考 (video-scripts)
description: '| `07-pvc-storage-failure` | PVC 存储 | 12min |'
category: general
tags:
- k8s
- ingress
- gateway
- rbac
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 数字人视频快速参考 是什么
- 如何 数字人视频快速参考
trigger_keywords:
- 数字人视频快速参考
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# 数字人视频快速参考

## 常用命令

```bash
# ===== 生成内容脚本 =====

# 列出可用 topic
python3 scripts/video-content-generator.py --type skill --list
python3 scripts/video-content-generator.py --type fta --list

# 生成视频脚本
python3 scripts/video-content-generator.py --type skill --topic [[concepts/pod-lifecycle|pod]]-crashloop -o video-scripts/pod-crashloop.md
python3 scripts/video-content-generator.py --type fta --topic pod-fta -o video-scripts/pod-fta.md

# ===== 生成视频 =====

# 查看支持的数字人形象
python3 scripts/video-generator.py --list-avatars --platform tencent
python3 scripts/video-generator.py --list-avatars --platform heygen

# 生成视频（腾讯智影）
python3 scripts/video-generator.py \
    --platform tencent \
    --script video-scripts/pod-crashloop.md \
    --avatar professional-engineer \
    --output video-output/pod-crashloop.mp4

# 批量生成
python3 scripts/video-generator.py \
    --batch video-scripts/ \
    --platform tencent \
    --avatar professional-engineer \
    --output-dir video-output/
```

## Topic 优先级

### P0 - 高频问题（建议优先生成）
| Topic | 说明 | 时长 |
|:---|:---|:---:|
| `01-node-notready` | 节点 NotReady | 10min |
| `02-pod-crashloop-oomkilled` | Pod 崩溃/OOM | 12min |
| `04-dns-resolution-failure` | DNS 问题 | 8min |
| `06-certificate-expiry` | 证书过期 | 7min |
| `05-service-connectivity` | Service 连通性 | 10min |

### P1 - 核心技能
| Topic | 说明 | 时长 |
|:---|:---|:---:|
| `03-pod-pending` | Pod Pending | 8min |
| `07-pvc-storage-failure` | PVC 存储 | 12min |
| `10-image-pull-failure` | 镜像拉取 | 7min |
| `09-rbac-quota-failure` | RBAC/配额 | 10min |

### P2 - 进阶专题
| Topic | 说明 | 时长 |
|:---|:---|:---:|
| `08-deployment-rollout-failure` | Deployment 发布 | 12min |
| `11-control-plane-failure` | 控制平面 | 15min |
| `13-ingress-gateway-failure` | Ingress 网关 | 10min |
| `14-configmap-secret-failure` | ConfigMap/Secret | 8min |

## 数字人形象

### 腾讯智影
| ID | 声音 | 适用场景 |
|:---|:---|:---|
| `professional-engineer` | 男声-专业 | 故障排查、技术培训 |
| `sre-female` | 女声-冷静 | 演示、讲解 |
| `tech-presenter` | 男声-积极 | 概览、介绍 |

### HeyGen
| ID | 声音 | 适用场景 |
|:---|:---|:---|
| `1_english_professional` | 英文-专业 | 英文分享 |
| `2_chinese_male` | 中文-男声 | 中文技术内容 |
| `3_english_female` | 英文-女声 | 英文演示 |

## 视频输出规范

```
video-output/
├── {topic}-{date}-{platform}.mp4
└── metadata/
    └── {topic}-{date}.meta.json
```

## 状态检查

```bash
# 检查视频生成状态
ls -la video-output/

# 查看元数据
cat video-output/metadata/*.meta.json

# 统计已生成视频
ls video-output/*.mp4 | wc -l
```