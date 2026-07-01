---
title: gatekeeper v3.18 Release Notes
description: gatekeeper v3.18 Release Notes — Kubernetes 生产运维知识库
summary: gatekeeper v3.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- crd
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gatekeeper v3.18 Release Notes 是什么
- 如何 gatekeeper v3.18 Release Notes
trigger_keywords:
- gatekeeper
- v3.18
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# gatekeeper v3.18 Release Notes

Source: [v3.18.3](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.18.3)

⚠ Warning: Operation `generate` is now required to guard CRD and VAP/VAPB generation. Please update your singleton deployment (e.g. gatekeeper-audit) to include `--operation=generate`. If you are not using audit, you need to add it to the controller manager deployment. https://open-policy-agent.github.io/gatekeeper/website/docs/operations/#generation

## Bug Fixes
- CP(#3921) CP(#3857) CP(#3802) CP(#3925) (#3930) [#3930](https://github.com/open-policy-agent/gatekeeper/pull/3930) ([Jaydip Gabani](https://github.com/open-policy-agent/gatekeeper/commit/8c7b2ad04a5513c59e3881588ece1ca5be6523c0))

## Chores
- Prepare v3.18.3 release (#3936) [#3936](https://github.com/open-policy-agent/gatekeeper/pull/3936) ([github-actions[bot]](https://github.com/open-policy-agent/gatekeeper/commit/5be06a95665624a619a8082677dcf942043bf514))