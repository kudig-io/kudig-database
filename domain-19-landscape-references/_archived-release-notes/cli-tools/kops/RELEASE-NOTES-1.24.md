---
title: kops v1.24 Release Notes
description: kops v1.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- cilium
- calico
- coredns
- containerd
- pdb
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.24 Release Notes 是什么
- 如何 kops v1.24 Release Notes
trigger_keywords:
- kops
- v1.24
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
- cni-basics
- etcd-basics
created: "2026-05-23"
---

# kops v1.24 Release Notes

Source: [v1.24.5](https://github.com/kubernetes/kops/releases/tag/v1.24.5)

## What's Changed
* Automated cherry pick of #14458: Update [[containerd|containerd]] to v1.6.9 by @hakman in https://github.com/kubernetes/kops/pull/14465
* Automated cherry pick of #14466: Update Calico and Canal by @hakman in https://github.com/kubernetes/kops/pull/14479
* Automated cherry pick of #14503: use the same tolerations config for coredns-autoscaler by @MoShitrit in https://github.com/kubernetes/kops/pull/14505
* Automated cherry pick of #14513: add a condition for the aws-cni ClusterRole based on the by @MoShitrit in https://github.com/kubernetes/kops/pull/14515
* Update Go to v1.18.8 by @hakman in https://github.com/kubernetes/kops/pull/14555
* Manual cherry pick of #14551: Update Calico and Canal to latest versions by @hakman in https://github.com/kubernetes/kops/pull/14558
* Automated cherry pick of #14550: Update containerd to v1.6.10 by @hakman in https://github.com/kubernetes/kops/pull/14556
* Automated cherry pick of #14564: use sprig join for template functions by @johngmyers in https://github.com/kubernetes/kops/pull/14568
* Automated cherry pick of #14576: aws: Fix SIGSEGV when using instance selector by @hakman in https://github.com/kubernetes/kops/pull/14581
* Automated cherry pick of #14595: Add generics alternatives for fi.Bool/Float*/Int*/String*() by @hakman in https://github.com/kubernetes/kops/pull/14598
* Automated cherry pick of #14602: Remove CloudFormation tests by @hakman in https://github.com/kubernetes/kops/pull/14606
* Cherry pick of #14442: Fix pdb for identity webhook by @johngmyers in https://github.com/kubernetes/kops/pull/14618
* Automated cherry pick of #14650: Add `ec2:DescribeAvailabilityZones` to the AWS CCM by @johngmyers in https://github.com/kubernetes/kops/pull/14653
* Automated cherry pick of #14648: aws: Limit the number of target groups updated per by @johngmyers in https://github.com/kubernetes/kops/pull/14652
* Automated cherry pick of #14655: gce: Allow [[Cilium|Cilium]] to connect to its etcd cluster by @hakman in https://github.com/kubernetes/kops/pull/14657
* Release 1.24.5 by @hakman in https://github.com/kubernetes/kops/pull/14662


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.24.4...v1.24.5