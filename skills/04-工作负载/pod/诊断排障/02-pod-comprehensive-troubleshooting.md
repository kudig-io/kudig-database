---
title: Pod 全面故障排查
description: '# 08 - Pod 全面故障排查 (Pod Comprehensive Troubleshooting)'
summary: 'kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name>'
category: troubleshooting
tags:
- pod
- crashloop
- imagepullbackoff
- terminating
- containercreating
- evicted
- unknown
- kubelet
- flannel
- calico
tier: core
created: '2026-05-23'
last_updated: '2026-07-02'
difficulty: beginner
reading_level: beginner
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Pod 卡住了怎么办
- Pod 状态异常
- CrashLoopBackOff
- ImagePullBackOff
- Terminating 无法删除
- ContainerCreating
- Evicted
trigger_keywords:
- Pod
- 全面故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- cni-basics
- etcd-basics
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
---

> **内容已并入主场**：[[wiki/19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]（本页为重排去重桩，正文见主场）
