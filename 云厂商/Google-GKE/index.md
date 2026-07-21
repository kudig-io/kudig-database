---
title: Google Cloud GKE
description: GKE 知识域 — Autopilot/Standard 模式、Dataplane V2、Workload Identity、存储集成、故障排查
category: subdomain
tags:
- gke
- google-cloud
- autopilot
- workload-identity
- dataplane-v2
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# Google Cloud GKE

> Google Kubernetes Engine — 云原生 K8s 的标杆服务。

## GKE 模式对比

| 模式 | 管理范围 | 适用 |
|------|----------|------|
| Autopilot | 全托管（节点也托管） | 简化运维、Serverless |
| Standard | 半托管（自管节点） | 需要节点控制 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[云厂商/Google-GKE/google-cloud-gke-overview.md\|GKE 概览]] | 架构/定价/区域 | beginner |
| [[云厂商/Google-GKE/02-gke-autopilot-serverless.md\|Autopilot 模式]] | Serverless K8s 实践 | intermediate |
| [[云厂商/Google-GKE/03-gke-networking-dataplane-v2.md\|Dataplane V2]] | eBPF 网络数据平面 | advanced |
| [[云厂商/Google-GKE/04-gke-storage-filestore-gcs.md\|存储集成]] | Filestore/GCS CSI | intermediate |
| [[云厂商/Google-GKE/05-gke-workload-identity-security.md\|Workload Identity]] | 无密钥身份认证 | advanced |
| [[云厂商/Google-GKE/06-gke-troubleshooting-playbook.md\|故障排查]] | GKE 常见问题处理 | advanced |

## Related

- [[云厂商/Azure-AKS/index.md|Azure AKS]]
- [[云厂商/index.md|云厂商总索引]]
