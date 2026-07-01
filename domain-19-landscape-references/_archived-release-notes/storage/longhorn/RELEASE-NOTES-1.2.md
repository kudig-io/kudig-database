---
title: longhorn v1.2 Release Notes
description: longhorn v1.2 Release Notes — Kubernetes 生产运维知识库
summary: longhorn v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- longhorn v1.2 Release Notes 是什么
- 如何 longhorn v1.2 Release Notes
trigger_keywords:
- longhorn
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Longhorn|longhorn]] v1.2 Release Notes

Source: [v1.2.6](https://github.com/longhorn/longhorn/releases/tag/v1.2.6)

## Release Note

**v1.2.6 released!** 🎆

This release introduces improvements and bug fixes as described below including stability, data correction, performance and so on. 

There are two important fixes that will prevent rarely potential data corruption during replica rebuilding, and also improve write performance via the specific filesystem block size.
- [4354](https://github.com/longhorn/longhorn/issues/4354): Introduce Data Alignment Correction for existing volumes if the filesystem block size is less than 4096.
- [4594](https://github.com/longhorn/longhorn/issues/4594): Use the specific block size for the filesystem to avoid unnecessary Ready-Modify-Write operations between volume head and snapshots. 

Please try it and feedback. Thanks for all the contributions!

## Installation

Longhorn supports 3 installation ways including Rancher catalog, Kubectl, and [[Helm|Helm]]. Follow the installation instructions [here](https://longhorn.io/docs/1.2.6/deploy/install/).

## Upgrade

> **Note** Please ensure the [[Kubernetes|Kubernetes]]es 集群配置最佳实践|Kubernetes cluster]] is >= v1.18 and <= v1.24 before upgrading to Longhorn v1.2.6 from v1.1.x or v1.2.x. Only support upgrading from v1.1.x and v1.2.x.

Follow the upgrade instructions [here](https://longhorn.io/docs/1.2.6/deploy/upgrade/).

## Deprecation & Incompatibilities

No deprecations and incompatibilities in this version.

## Known Issues after Release

Please follow up on [here](https://github.com/longhorn/longhorn/wiki/Outstanding-Known-Issues-of-Releases) about any outstanding issues found after this release. 


## Highlights
  
  - [BUG] data corruption due to COW and block size not being aligned during rebuilding replicas  ([4354](https://github.com/longhorn/longhorn/issues/4354)) - @PhanLe1010 @chriscchien
  - [Improvement] Using specific block size in Longhorn volume's filesystem ([4594](https://github.com/longhorn/longhorn/issues/4594)) - @derekbit @roger-ryao
  
## Improvement
  
  - [IMPROVEMENT] Change the script into a docker run command mentioned in 'recovery from longhorn backup without system installed' doc ([1521](https://github.com/longhorn/longhorn/issues/1521)) - @weizhe0422 @chriscchien
  - [IMPROVEMENT] Check if node schedulable condition is set before trying to read it ([4581](https://github.com/longhorn/longhorn/issues/4581)) - @weizhe0422 @roger-ryao
  
## Bugs
  
  - [Bug] Degraded volume generate failed replica make volume unschedulable ([3220](https://github.com/longhorn/longhorn/issues/3220)) - @derekbit @chriscchien
  - [BUG] Continuously rebuild when auto-balance==least-effort and existing node becomes unschedulable ([4502](https://github.com/longhorn/longhorn/issues/4502)) - @yangchiu @c3y1huang
  - [BUG] Longhorn accidentally schedule all replicas onto a worker node eventhough the setting Replica Node Level Soft Anti-Affinity is currently disabled ([4546](https://github.com/longhorn/longhorn/issues/4546)) - @yangchiu @mantissahz
  - [BUG] the values.yaml in the longhorn helm chart contains values not used. ([4601](https://github.com/longhorn/longhorn/issues/4601)) - @weizhe0422 @roger-ryao
  - [BUG] longhorn-engine integration test test_restore_to_file_with_backing_file failed after upgrade to sles 15.4 ([4632](https://github.com/longhorn/longhorn/issues/4632)) - @mantissahz
  - [BUG] charts/longhorn/questions.yaml include oudated csi-image tags ([4669](https://github.com/longhorn/longhorn/issues/4669)) - @PhanLe1010 @roger-ryao
  - [BUG] Replica Auto Balance repeatedly delete the local replica and trigger rebuilding ([4761](https://github.com/longhorn/longhorn/issues/4761)) - @c3y1huang @roger-ryao
  - [BUG] Unable to reuse existing failed replica causes test case test_allow_volume_creation_with_degraded_availability_restore failed ([4791](https://github.com/longhorn/longhorn/issues/4791)) - @yangchiu @mantissahz
  
## Misc
  
  - [TASK] Update longhorn components with the latest backupstore ([4552](https://github.com/longhorn/longhorn/issues/4552)) - @derekbit
  
## Contributors

- @PhanLe1010
- @c3y1huang
- @chriscchien
- @derekbit
- @innobead
- @mantissahz
- @roger-ryao
- @weizhe0422
- @yangchiu
