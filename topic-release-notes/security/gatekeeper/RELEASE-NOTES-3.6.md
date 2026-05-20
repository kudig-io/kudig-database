---
title: gatekeeper v3.6 Release Notes
description: gatekeeper v3.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- pdb
- crd
- webhook
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gatekeeper v3.6 Release Notes 是什么
- 如何 gatekeeper v3.6 Release Notes
trigger_keywords:
- gatekeeper
- v3.6
- Release
- Notes
- release
- notes
---

# gatekeeper v3.6 Release Notes

Source: [v3.6.0](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.6.0)

This stable release includes bug fixes and new features. 

## Notable updates since last stable version
- ConstraintTemplate CRD moves to v1 🎉 
- Reduce System.Mutate runtime by 87% 🔨 
- Fix race conditions in watch manager and constraint controllers 🐎 
- Remove non-specific webhook request metrics 📊 
- Add prefix-based matching for namespaces and excludedNamespaces 🔡 
- Add integer keyValue support to mutation path parser / mutators 🔢 
- Helm enable to config controller manager & audit port 🎊 
- Add helm hooks to upgrade CRDs 🆙 
- Add metrics reporting for mutation 📈 

## Commits
- aad6c27: fix whitespace error in the debugging docs (#1465) (rob salmond) [#1465](https://github.com/open-policy-agent/gatekeeper/pull/1465)
- 07e2fd0: Add metrics reporting for mutation (#1435) (Julian Katz) [#1435](https://github.com/open-policy-agent/gatekeeper/pull/1435)
- f695654: Add frameworks apis to scheme (#1470) (Julian Katz) [#1470](https://github.com/open-policy-agent/gatekeeper/pull/1470)
- 821db67: update with k8s v1.22.0 (#1477) (Sertaç Özercan) [#1477](https://github.com/open-policy-agent/gatekeeper/pull/1477)
- 5975122: Add label to bats http.send test for idempotence (#1473) (Ivan Font) [#1473](https://github.com/open-policy-agent/gatekeeper/pull/1473)
- 407611a: Deduplicate mutator controller logic (#1474) (Max Smythe) [#1474](https://github.com/open-policy-agent/gatekeeper/pull/1474)
- 6a8ff89: Make Context usage consistent (#1457) (Will Beason) [#1457](https://github.com/open-policy-agent/gatekeeper/pull/1457)
- aa8ad45: Add helm hooks to upgrade CRDs (#1485) (Rita Zhang) [#1485](https://github.com/open-policy-agent/gatekeeper/pull/1485)
- c70dfd0: Unify Gatekeeper and controller-runtime metrics into a single endpoint (#1482) (Oren Shomron) [#1482](https://github.com/open-policy-agent/gatekeeper/pull/1482)
- e00262b: Refactor core.Reconciler (#1489) (Will Beason) [#1489](https://github.com/open-policy-agent/gatekeeper/pull/1489)
- a1b50a0: Update the upper limit of request duration metrics to 3 seconds (#1504) (Tsubasa Umeuchi) [#1504](https://github.com/open-policy-agent/gatekeeper/pull/1504)
- 0238780: Dynamically change the API version of the PDB in Helm Chart (#1502) (Yuki Iwai) [#1502](https://github.com/open-policy-agent/gatekeeper/pull/1502)
- 1901725: Helm enable to config controller manager & audit port (#1438) (Edvin N) [#1438](https://github.com/open-policy-agent/gatekeeper/pull/1438)
- c3e9cd4: V1 constrainttemplate docs (#1492) (Julian Katz) [#1492](https://github.com/open-policy-agent/gatekeeper/pull/1492)
- dd97b8a: run gator test (#1463) (Will Beason) [#1463](https://github.com/open-policy-agent/gatekeeper/pull/1463)
- 93ad7e4: Refactor mutator Matches() to make extension easy (#1494) (Julian Katz) [#1494](https://github.com/open-policy-agent/gatekeeper/pull/1494)
- mutation process to allProcesses list (#1516) [#1516](https://github.com/open-policy-agent/gatekeeper/pull/1516) ([Spencer McCreary](https://github.com/open-policy-agent/gatekeeper/commit/0fc227e33ccd12d7da52d16d84d14d32383c29e3))
- 94ced7f: Update supported k8s versions (#1517) (Rita Zhang) [#1517](https://github.com/open-policy-agent/gatekeeper/pull/1517)
- 9503ef2: Prepare v3.6.0 release (#1518) (Sertaç Özercan) [#1518](https://github.com/open-policy-agent/gatekeeper/pull/1518)