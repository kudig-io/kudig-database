---
title: longhorn v1.3 Release Notes
description: longhorn v1.3 Release Notes — Kubernetes 生产运维知识库
summary: longhorn v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
- scheduler
- helm
- docker
- pdb
- daemonset
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- longhorn v1.3 Release Notes 是什么
- 如何 longhorn v1.3 Release Notes
trigger_keywords:
- longhorn
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Longhorn|longhorn]] v1.3 Release Notes

Source: [v1.3.3](https://github.com/longhorn/longhorn/releases/tag/v1.3.3)

## Release Note
### **v1.3.3 released!** 🎆

This release introduces improvements and bug fixes as described below about stability, performance, space efficiency, resilience, and so on. Please try it and feedback. Thanks for all the contributions!

## Installation

> **Please ensure your [[Kubernetes|Kubernetes]]es 集群配置最佳实践|Kubernetes cluster]] is >= v1.18 and <= v1.24 before installing Longhorn v1.3.3.**

Longhorn supports 3 installation ways including Rancher App Marketplace, Kubectl, and [[Helm|Helm]]. Follow the installation instructions [here](https://longhorn.io/docs/1.3.3/deploy/install/).

## Upgrade

> **Please read the [important notes](https://longhorn.io/docs/1.3.3/deploy/important-notes/) first and ensure your Kubernetes cluster is >= v1.18 and <= v1.24 before upgrading to Longhorn v1.3.3 from v1.2.x or v1.3.x. Only support upgrading from v1.2.x and v1.3.x.**

Follow the upgrade instructions [here](https://longhorn.io/docs/1.3.3/deploy/upgrade/).

## Deprecation & Incompatibilities

N/A

## Known Issues after Release

Please follow up on [here](https://github.com/longhorn/longhorn/wiki/Outstanding-Known-Issues-of-Releases) about any outstanding issues found after this release.


## Highlights
  
  - [IMPROVEMENT] Use PDB to protect Longhorn components from unexpected drains ([3304](https://github.com/longhorn/longhorn/issues/3304)) - @yangchiu @PhanLe1010
  - [IMPROVEMENT] Periodically clean up [[17-系统基础/06-知识字典/storage/volume-snapshots.md|volume snapshots]] ([3836](https://github.com/longhorn/longhorn/issues/3836)) - @c3y1huang @chriscchien
  - [IMPROVEMENT] Recurring jobs create new snapshots while being not able to clean up old ones ([4898](https://github.com/longhorn/longhorn/issues/4898)) - @mantissahz @chriscchien
  
## Improvement
  
  - [IMPROVEMENT] Change the script into a docker run command mentioned in 'recovery from longhorn backup without system installed' doc ([1521](https://github.com/longhorn/longhorn/issues/1521)) - @weizhe0422 @chriscchien
  - [IMPROVEMENT] liveness and readiness probes with longhorn csi plugin daemonset ([3907](https://github.com/longhorn/longhorn/issues/3907)) - @c3y1huang @roger-ryao
  - [IMPROVEMENT]  Too many debug-level log messages in engine instance-manager ([4427](https://github.com/longhorn/longhorn/issues/4427)) - @derekbit @chriscchien
  - [IMPROVEMENT] share-manager pod bypasses the kubernetes scheduler ([4789](https://github.com/longhorn/longhorn/issues/4789)) - @joshimoo @chriscchien
  - [IMPROVEMENT] Unify the format of returned error messages in longhorn-engine ([4828](https://github.com/longhorn/longhorn/issues/4828)) - @derekbit
  - [IMPROVEMENT] Affinity in the longhorn-ui deployment within the helm chart ([4987](https://github.com/longhorn/longhorn/issues/4987)) - @mantissahz @chriscchien
  - [IMPROVEMENT] Upgrade tcmalloc in longhorn-engine ([5050](https://github.com/longhorn/longhorn/issues/5050)) - @derekbit
  - [IMPROVEMENT] Fix Guaranteed Engine Manager CPU recommendation forumula in UI ([5338](https://github.com/longhorn/longhorn/issues/5338)) - @c3y1huang @smallteeths @roger-ryao
  - [IMPROVEMENT] Set write-cache of longhorn block device to off explicitly ([5382](https://github.com/longhorn/longhorn/issues/5382)) - @derekbit @chriscchien
  - [DOC] Update Kubernetes version info to have consistent description from the longhorn documentation in chart ([5399](https://github.com/longhorn/longhorn/issues/5399)) - @ChanYiLin @roger-ryao
  - [IMPROVEMENT] Fix BackingImage uploading/downloading flow to prevent client timeout ([5443](https://github.com/longhorn/longhorn/issues/5443)) - @ChanYiLin @chriscchien
  - [IMPROVEMENT] Create a new setting so that Longhorn removes PDB for instance-manager-r that doesn't have any running instance inside it ([5549](https://github.com/longhorn/longhorn/issues/5549)) - @PhanLe1010 @khushboo-rancher
  - [IMPROVEMENT] Deprecate the setting `allow-node-drain-with-last-healthy-replica` and replace it by `node-drain-policy` setting ([5585](https://github.com/longhorn/longhorn/issues/5585)) - @PhanLe1010
  - [IMPROVEMENT][UI] Recurring jobs create new snapshots while being not able to clean up old one ([5610](https://github.com/longhorn/longhorn/issues/5610)) - @mantissahz @smallteeths @roger-ryao
  - [IMPROVEMENT] Only activate replica if it doesn't have deletion timestamp during volume engine upgrade ([5632](https://github.com/longhorn/longhorn/issues/5632)) - @PhanLe1010 @roger-ryao
  
## Performance
  
  - [TASK] Disable tcmalloc in data path because newer tcmalloc version leads to performance drop ([5096](https://github.com/longhorn/longhorn/issues/5096)) - @derekbit @chriscchien
  
## Stability
  
  - [BUG] Longhorn won't fail all replicas if there is no valid backend during the engine starting stage ([1330](https://github.com/longhorn/longhorn/issues/1330)) - @derekbit @roger-ryao
  - [BUG] Engine binary cannot be recovered after being removed accidentally ([4380](https://github.com/longhorn/longhorn/issues/4380)) - @yangchiu @c3y1huang
  - [BUG] volume is stuck in attaching/detaching loop with error `Failed to init frontend: device...` ([4959](https://github.com/longhorn/longhorn/issues/4959)) - @derekbit @PhanLe1010 @chriscchien
  - [BUG] Memory leak in CSI plugin caused by stuck umount processes if the RWX volume is already gone ([5296](https://github.com/longhorn/longhorn/issues/5296)) - @derekbit @roger-ryao
  - [BUG] share-manager pod failed to restart after kubelet restart ([5507](https://github.com/longhorn/longhorn/issues/5507)) - @yangchiu @derekbit
  - [BUG] RWX volume is stuck at detaching when the attached node is down  ([5558](https://github.com/longhorn/longhorn/issues/5558)) - @derekbit @roger-ryao
  
## Bugs
  
  - [BUG] Restoring volume stuck forever if the backup is already deleted. ([1867](https://github.com/longhorn/longhorn/issues/1867)) - @mantissahz @chriscchien
  - [BUG] Duplicated default instance manager leads to engine/replica cannot be started ([3000](https://github.com/longhorn/longhorn/issues/3000)) - @PhanLe1010 @roger-ryao
  - [BUG] Delete a uploading backing image, the corresponding LH temp file is not deleted ([3682](https://github.com/longhorn/longhorn/issues/3682)) - @ChanYiLin @chriscchien
  - [BUG] Replica rebuilding failure with error "Replica must be closed, Can not add in state: open" ([3828](https://github.com/longhorn/longhorn/issues/3828)) - @mantissahz @roger-ryao
  - [BUG] Max length of volume name not consist between frontend and backend ([3917](https://github.com/longhorn/longhorn/issues/3917)) - @weizhe0422 @roger-ryao
  - [BUG] Can't delete volumesnapshot if backup removed first  ([4107](https://github.com/longhorn/longhorn/issues/4107)) - @weizhe0422 @chriscchien
  - [BUG] LH continuously reports `invalid customized default setting taint-toleration` ([4554](https://github.com/longhorn/longhorn/issues/4554)) - @weizhe0422 @roger-ryao
  - [BUG] longhorn-engine integration test test_restore_to_file_with_backing_file failed after upgrade to sles 15.4 ([4632](https://github.com/longhorn/longhorn/issues/4632)) - @mantissahz
  - [BUG] The old instance-manager-r Pods are not deleted after upgrade ([4726](https://github.com/longhorn/longhorn/issues/4726)) - @mantissahz @chriscchien
  - [BUG] Replica Auto Balance repeatedly delete the local replica and trigger rebuilding ([4761](https://github.com/longhorn/longhorn/issues/4761)) - @c3y1huang @roger-ryao
  - [BUG] Unable to reuse existing failed replica causes test case test_allow_volume_creation_with_degraded_availability_restore failed ([4791](https://github.com/longhorn/longhorn/issues/4791)) - @yangchiu @mantissahz
  - [BUG] Volume metafile getting deleted or empty results in a detach-attach loop ([4846](https://github.com/longhorn/longhorn/issues/4846)) - @mantissahz @chriscchien
  - [BUG] Backing image is stuck at `in-progress` status if the provided checksum is incorrect ([4852](https://github.com/longhorn/longhorn/issues/4852)) - @FrankYang0529 @chriscchien
  - [BUG] Duplicate channel close error in the backing image manage related components ([4865](https://github.com/longhorn/longhorn/issues/4865)) - @weizhe0422 @roger-ryao
  - [BUG] The node ID of backing image data source somehow get changed then lead to file handling failed ([4887](https://github.com/longhorn/longhorn/issues/4887)) - @shuo-wu @chriscchien
  - [BUG] Cannot upload a backing image larger than 10G ([4902](https://github.com/longhorn/longhorn/issues/4902)) - @smallteeths @shuo-wu @chriscchien
  - [BUG] System backup showing wrong age ([5047](https://github.com/longhorn/longhorn/issues/5047)) - @smallteeths @khushboo-rancher
  - [BUG] Longhorn 1.3.2 fails to backup & restore volumes behind Internet proxy  ([5054](https://github.com/longhorn/longhorn/issues/5054)) - @mantissahz @chriscchien
  - [BUG] Sync up with backup target during DR volume activation ([5292](https://github.com/longhorn/longhorn/issues/5292)) - @yangchiu @weizhe0422
  - [BUG] environment_check.sh does not handle differnt kernel versions in cluster correctly ([5304](https://github.com/longhorn/longhorn/issues/5304)) - @achims311 @roger-ryao
  - [BUG] Replica rebuilding caused by rke2/kubelet restart ([5340](https://github.com/longhorn/longhorn/issues/5340)) - @derekbit @chriscchien
  - [BUG] Error message not consistent between create/update recurring job when retain number greater than 50 ([5434](https://github.com/longhorn/longhorn/issues/5434)) - @c3y1huang @chriscchien
  - [BUG] Do not copy Host header to API requests forwarded to Longhorn Manager ([5438](https://github.com/longhorn/longhorn/issues/5438)) - @yangchiu @smallteeths
  - [BUG] test case test_backup_lock_deletion_during_restoration failed ([5458](https://github.com/longhorn/longhorn/issues/5458)) - @yangchiu @derekbit
  - [BUG] Volume restoration will never complete if attached node is down ([5464](https://github.com/longhorn/longhorn/issues/5464)) - @derekbit @weizhe0422 @chriscchien
  - [BUG] Physical node down test failed ([5477](https://github.com/longhorn/longhorn/issues/5477)) - @derekbit @chriscchien
  - [BUG] Backing image with sync failure ([5481](https://github.com/longhorn/longhorn/issues/5481)) - @ChanYiLin @roger-ryao
  - [BUG] Example of data migration doesn't work for hidden/./dot-files) ([5484](https://github.com/longhorn/longhorn/issues/5484)) - @hedefalk @shuo-wu @chriscchien
  - [BUG] test case test_dr_volume_with_backup_block_deletion failed ([5489](https://github.com/longhorn/longhorn/issues/5489)) - @yangchiu @derekbit
  - [BUG] RWX volume attachment failed if tried more enough times ([5537](https://github.com/longhorn/longhorn/issues/5537)) - @yangchiu @derekbit
  - [BUG] Value overlapped in page Instance Manager Image ([5622](https://github.com/longhorn/longhorn/issues/5622)) - @smallteeths @chriscchien
  - [BUG] Instance manager PDB created with wrong selector thus blocking the draining of the wrongly selected node forever ([5680](https://github.com/longhorn/longhorn/issues/5680)) - @PhanLe1010 @chriscchien
  - [BUG] During volume live engine upgrade, if the replica pod is killed, the volume is stuck in upgrading forever ([5684](https://github.com/longhorn/longhorn/issues/5684)) - @yangchiu @PhanLe1010
  - [BUG] Instance manager PDBs cannot be removed if the longhorn-manager pod on its spec node is not available ([5688](https://github.com/longhorn/longhorn/issues/5688)) - @PhanLe1010 @roger-ryao
  - [BUG] `test_replica_auto_balance_when_replica_on_unschedulable_node` Error in creating volume with nodeSelector and dataLocality parameters ([5745](https://github.com/longhorn/longhorn/issues/5745)) - @c3y1huang @roger-ryao
  - [BUG] Resources such as replicas are somehow not mutated when network is unstable  ([5762](https://github.com/longhorn/longhorn/issues/5762)) - @derekbit
  
## Misc
  
  - [DOC] RWX support for NVIDIA JETSON Ubuntu 18.4LTS kernel requires enabling NFSV4.1 ([3157](https://github.com/longhorn/longhorn/issues/3157)) - @yangchiu @derekbit
  - [TASK][UI] add new recurring job tasks ([5272](https://github.com/longhorn/longhorn/issues/5272)) - @smallteeths @chriscchien
  
## Contributors

- @ChanYiLin
- @FrankYang0529
- @PhanLe1010
- @achims311
- @c3y1huang
- @chriscchien
- @derekbit
- @hedefalk
- @innobead
- @joshimoo
- @khushboo-rancher
- @mantissahz
- @roger-ryao
- @shuo-wu
- @smallteeths
- @weizhe0422
- @yangchiu


<!-- risk-assessed -->
