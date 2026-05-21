---
title: kops v1.8 Release Notes
description: kops v1.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flannel
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.8 Release Notes 是什么
- 如何 kops v1.8 Release Notes
trigger_keywords:
- kops
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
---

# kops v1.8 Release Notes

Source: [1.8.1](https://github.com/kubernetes/kops/releases/tag/1.8.1)

Release 1.8.1 is a small patch release, which updates network plugins, but also tolerates a new schema
file that will be added in kops 1.9.0.  This will provide a downgrade option from kops 1.9.0.

* Ignore keyset.yaml files; provides a downgrade option from (upcoming) kops 1.9.0
* Update flannel, weave, romana, kopeio-networking, calico, canal
* Stop passing deprecated require-kubeconfig flag for kubernetes >= 1.9