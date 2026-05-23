---
title: kops v1.31 Release Notes
description: kops v1.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- cilium
- containerd
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.31 Release Notes 是什么
- 如何 kops v1.31 Release Notes
trigger_keywords:
- kops
- v1.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
- etcd-basics
- policy-basics
created: "2026-05-23"
---

# kops v1.31 Release Notes

Source: [v1.31.0](https://github.com/kubernetes/kops/releases/tag/v1.31.0)

# Significant changes

* [[Kubernetes|Kubernetes]] minor version upgrades to 1.31 should be performed using a new `kops reconcile` cluster command.

Kubernetes 1.31 introduces stricter checks around the version-skew policy.  While kOps has always followed the version-skew policy, there was an edge case: nodes that were added by an autoscaler during a rolling-update would not always follow the version-skew policy.  We recommend trying the new `kops reconcile` command, see [docs/tutorial/upgrading-kubernetes.md](https://github.com/kubernetes/kops/blob/master/docs/tutorial/upgrading-kubernetes.md) for more details.

# Other changes of note

* [[Cilium|Cilium]] has been upgraded to v1.16.
* Spotinst cluster controller V1 is replaced with Ocean kubernetes controller V2, all old k8s resource are removed except spotinst-kubernetes-cluster-controller Secret.

# Deprecations

* Support for Kubernetes version 1.25 is deprecated and will be removed in kOps 1.31.

* Support for Kubernetes version 1.26 is deprecated and will be removed in kOps 1.32.

**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.31.0-beta.1...v1.31.0

## Changes since 1.31.0-beta.1

* Only set NTH ManagedASGTag label if it doesn't already exist by @rifelpet in https://github.com/kubernetes/kops/pull/17142
* chore(networking): bump aws-vpc-cni version to 1.19.2 by @moshevayner in https://github.com/kubernetes/kops/pull/17164
* Only configure STS region for Route 53 when we obtain it using IDMS by @johngmyers in https://github.com/kubernetes/kops/pull/17166
* Update OldestRecommendedKubernetesVersion to 1.28 for kOps 1.31 by @hakman in https://github.com/kubernetes/kops/pull/17168
* Update Cilium to v1.16.5 by @hakman in https://github.com/kubernetes/kops/pull/17173
* Adding VolumeType for Azure for etcdMembers by @hakman in https://github.com/kubernetes/kops/pull/17175
* Propagate IG NodeLabels to k8s nodes in Hetzner by @rifelpet in https://github.com/kubernetes/kops/pull/17176
* Update Go to v1.23.4 by @hakman in https://github.com/kubernetes/kops/pull/17185
* fix the random order of block_device_mappings render Include by @hakman in https://github.com/kubernetes/kops/pull/17188
* aws: Update EBS CSI driver to v1.38.1 by @hakman in https://github.com/kubernetes/kops/pull/17194
* Use SDK's built-in resolver for S3Path.GetHTTPsUrl by @rifelpet in https://github.com/kubernetes/kops/pull/17195
* Use the same port for hubble-metrics that is used by by @hakman in https://github.com/kubernetes/kops/pull/17198
* Update containerd to v1.7.25 by @hakman in https://github.com/kubernetes/kops/pull/17202
* Update Go to v1.23.5 by @hakman in https://github.com/kubernetes/kops/pull/17218
* chore: add context to rolling update functions by @justinsb in https://github.com/kubernetes/kops/pull/17222
* s3 vfs: fix delete all versions to handle errors by @justinsb in https://github.com/kubernetes/kops/pull/17221
* reconcile: if --yes is not provided, print the same output as `update cluster` does by @justinsb in https://github.com/kubernetes/kops/pull/17223
* chore: refactor factory to accept a cluster by @justinsb in https://github.com/kubernetes/kops/pull/17225
* Remove reconcile flag from `kops update` by @justinsb in https://github.com/kubernetes/kops/pull/17226
* tests: use reconcile command for kOps 1.31+ by @justinsb in https://github.com/kubernetes/kops/pull/17228
* chore: generate kubeconfig on the fly by @justinsb in https://github.com/kubernetes/kops/pull/17227
* Release 1.31.0 by @justinsb in https://github.com/kubernetes/kops/pull/17231

