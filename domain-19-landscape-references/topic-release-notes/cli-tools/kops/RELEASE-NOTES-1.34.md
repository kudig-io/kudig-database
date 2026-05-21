---
title: kops v1.34 Release Notes
description: kops v1.34 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- kubelet
- cilium
- containerd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.34 Release Notes 是什么
- 如何 kops v1.34 Release Notes
trigger_keywords:
- kops
- v1.34
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
- etcd-basics
---

# kops v1.34 Release Notes

Source: [v1.34.2](https://github.com/kubernetes/kops/releases/tag/v1.34.2)

## What's Changed
* Automated cherry pick of #17755: Include maxParallelImagePulls field in Kubelet config by @hakman in https://github.com/kubernetes/kops/pull/17772
* Automated cherry pick of #17773: aws: Tag Launch Template network interfaces by @hakman in https://github.com/kubernetes/kops/pull/17774
* Automated cherry pick of #17776: aws: Enable CloudWatch metrics for the warm pool of an ASG by @recollir in https://github.com/kubernetes/kops/pull/17778
* Automated cherry pick of #17783: chore(upup): bump aws-cni to 1.20.5 by @moshevayner in https://github.com/kubernetes/kops/pull/17786
* Automated cherry pick of #17792: aws: Disable the kubelet systemd unit during warm pool warming by @dezmodue in https://github.com/kubernetes/kops/pull/17801
* Automated cherry pick of #17800: aws: Allow warm pool with mixed instances policy by @kimxogus in https://github.com/kubernetes/kops/pull/17803
* gcp: cherrypick ccm upgrade to release 1.34 by @upodroid in https://github.com/kubernetes/kops/pull/17794
* Automated cherry pick of #17812: etcd-manager: Update etcd versions by @hakman in https://github.com/kubernetes/kops/pull/17813
* Automated cherry pick of #17712: gce: bump GCE PD CSI Driver by @upodroid in https://github.com/kubernetes/kops/pull/17835
* Automated cherry pick of #17899: Update Cilium to v1.18.6 by @hakman in https://github.com/kubernetes/kops/pull/17900
* Automated cherry pick of #17689: Migrate Kindnet to Kubernetes prod registry by @rifelpet in https://github.com/kubernetes/kops/pull/17903
* Automated cherry pick of #17854: Add iam:ListInstanceProfiles permission to Karpenter by @hakman in https://github.com/kubernetes/kops/pull/17905
* Cherry-pick #17879: bump golang to 1.25.6 by @justinsb in https://github.com/kubernetes/kops/pull/17906
* deps: bump containerd to v1.7.29 to address vulnerabilities by @justinsb in https://github.com/kubernetes/kops/pull/17907
* Automated cherry pick of #17917: hetzner: Update default server type to cx23 by @hakman in https://github.com/kubernetes/kops/pull/17918
* Automated cherry pick of #17867: Use a different systemd-networkd configuration for AL2023
#17882: Disable cloud-init network hotplug on Ubuntu 24.04 for Cilium and Ama…
#17933: Set MACAddressPolicy=none for AWS VPC CNI on AL2023 by @rifelpet in https://github.com/kubernetes/kops/pull/17935
* Automated cherry pick of #17945: Create iproute2 symlink for kuberouter on older distros by @rifelpet in https://github.com/kubernetes/kops/pull/17947
* Automated cherry pick of #17956: versionbump: go 1.25.7 by @hakman in https://github.com/kubernetes/kops/pull/17958
* Automated cherry pick of #17861: Feature: pull user defined images for warm pool instances by @hakman in https://github.com/kubernetes/kops/pull/17960
* chore: Back-port pulling CNI plugins from GitHub by @hakman in https://github.com/kubernetes/kops/pull/17970
* Automated cherry pick of #17980: chore: Add asset hashes for February 2026 releases by @hakman in https://github.com/kubernetes/kops/pull/17982
* Automated cherry pick of #17976: drop cdn.dl.k8s.io as a mirror
#17987: drop storage.googleapis.com/k8s-artifacts-cni as a mirror by @hakman in https://github.com/kubernetes/kops/pull/17989
* Automated cherry pick of #18021: chore: Add hashes for additional February releases by @hakman in https://github.com/kubernetes/kops/pull/18023
* Automated cherry pick of #18026: chore: Bump etcd-manager to v3.0.20260227 by @hakman in https://github.com/kubernetes/kops/pull/18028
* Automated cherry pick of #18043: Fix node bootstrap challenge response hashing by @rifelpet in https://github.com/kubernetes/kops/pull/18045
* Automated cherry pick of #18058: chore: Bump Go to v1.25.8 by @hakman in https://github.com/kubernetes/kops/pull/18060
* Release 1.34.2 by @hakman in https://github.com/kubernetes/kops/pull/18091


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.34.1...v1.34.2