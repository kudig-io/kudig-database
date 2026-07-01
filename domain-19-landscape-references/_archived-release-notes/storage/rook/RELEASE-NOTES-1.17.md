---
title: rook v1.17 Release Notes
description: rook v1.17 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rook
- ceph
- job
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.17 Release Notes 是什么
- 如何 rook v1.17 Release Notes
trigger_keywords:
- rook
- v1.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Rook|rook]] v1.17 Release Notes

Source: [v1.17.9](https://github.com/rook/rook/releases/tag/v1.17.9)

# Improvements
Rook v1.17.9 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- pool: Retry pool status updates in the radosnamespace controller (#16700, @parth-gr)
- object: Fix user quotas being overwritten when OBC bucketOwner is set (#16672, @jhoblitt)
- mon: Wait for the canary [[Pods|pods]] to be terminated (#16619, @sp98)
- mon: Respond quickly to the mon canary pod deletion (#16629, @travisn)
- namespace: Blocklist `ip:nonce` in cleanup job (#16532, @Madhu-1)
- core: Fix typos in ObjectZoneSpec.ZoneGroup and ObjectZoneGroupSpec.Realm field descriptions (#16496, @jhoblitt)
