---
title: rook v1.4 Release Notes
description: rook v1.4 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rook
- ceph
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.4 Release Notes 是什么
- 如何 rook v1.4 Release Notes
trigger_keywords:
- rook
- v1.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Rook|rook]] v1.4 Release Notes

Source: [v1.4.9](https://github.com/rook/rook/releases/tag/v1.4.9)

# Improvements
Rook v1.4.9 is a patch release limited in scope and focusing on small feature additions and bug fixes.

This patch release updates the Rook CRDs to v1 as part of the migration to [[Helm|Helm]] 3. While we have tested this helm upgrade scenario, you may want to consider upgrading to 1.5 where there has been more comprehensive testing with this conversion rather than deploy v1.4.9 with helm.

## Ceph
- Update to Helm 3 and convert deprecated v1beta1 resources to v1 (#6910, @travisn)
- Add devices to schema at overall storage level (#6938, @travisn)
- Update operator base image and example manifests to Ceph v15.2.8 (#6847, @travisn)
- Tune fast device class for OSD on PVC in the Azure (#6303, @subhamkrai)
- RGW [[Service|service]] selector should not change during upgrade (#6742, @travisn)
