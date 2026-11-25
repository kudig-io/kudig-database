---
title: Pod Pending 状态深度诊断
description: '# 05 - Pod Pending 状态深度诊断 (Pod Pending Diagnosis)'
summary: 'Pod 处于 Pending 状态表示 Pod 已被 [[kubernetes|Kubernetes]]es API|Kubernetes API]] Server 接受，但尚未被调度到节点或容器镜像尚未拉取。'
category: troubleshooting
tags:
- pod
- pending
- 调度
- scheduler
- Insufficient
- taint
- affinity
- pvc
- quota
- topology
tier: core
created: '2026-05-23'
last_updated: '2026-07-02'
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- Pod 一直 Pending 怎么办
- Pod stuck in Pending state
- 调度失败怎么排查
- Insufficient cpu memory
- no nodes available to schedule
- node had taint that pod didn't tolerate
- 调度器不工作
- Pending 时间太长
trigger_keywords:
- Pod
- Pending
- 状态深度诊断
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- monitoring-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/pod-fta.md
  label: '故障树: pod'
- type: skill
  path: ../故障诊断/topic-skills/03-pod-pending.md
  label: '运维技能: 03-pod-pending'
---

> **内容已并入主场**：[[wiki/17-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]（本页为重排去重桩，正文见主场）
