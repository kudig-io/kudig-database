---
title: kops v1.32 Release Notes
description: kops v1.32 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.32 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
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
- kops v1.32 Release Notes 是什么
- 如何 kops v1.32 Release Notes
trigger_keywords:
- kops
- v1.32
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
---



# kops v1.32 Release Notes

Source: [v1.32.4](https://github.com/kubernetes/kops/releases/tag/v1.32.4)

## What's Changed
* Automated cherry pick of #17559: Bump ko-build by @upodroid in https://github.com/kubernetes/kops/pull/17736
* Automated cherry pick of #17709: tests: free up disk space on github actions runner by @hakman in https://github.com/kubernetes/kops/pull/17805
* Automated cherry pick of #17157: Use ephemeral S3 buckets for E2E tests by @hakman in https://github.com/kubernetes/kops/pull/17806
* Automated cherry pick of #17792: aws: Disable the [[kubelet|kubelet]] systemd unit during warm pool warming by @dezmodue in https://github.com/kubernetes/kops/pull/17807
* Automated cherry pick of #17722: scaleway: Fix failing terraform test by @hakman in https://github.com/kubernetes/kops/pull/17978
* chore: Back-port pulling CNINI Plugins|CNI plugins]] from GitHub by @hakman in https://github.com/kubernetes/kops/pull/17973
* Automated cherry pick of #17980: chore: Add asset hashes for February 2026 releases by @hakman in https://github.com/kubernetes/kops/pull/17984
* Automated cherry pick of #17976: drop cdn.dl.k8s.io as a mirror
#17987: drop storage.googleapis.com/k8s-artifacts-cni as a mirror by @hakman in https://github.com/kubernetes/kops/pull/17991
* Automated cherry pick of #17956: versionbump: go 1.25.7 by @hakman in https://github.com/kubernetes/kops/pull/17996
* Automated cherry pick of #18021: chore: Add hashes for additional February releases by @hakman in https://github.com/kubernetes/kops/pull/18025
* Automated cherry pick of #18043: Fix node bootstrap challenge response hashing by @rifelpet in https://github.com/kubernetes/kops/pull/18047
* Automated cherry pick of #18058: chore: Bump Go to v1.25.8 by @hakman in https://github.com/kubernetes/kops/pull/18062
* Release 1.32.3 by @hakman in https://github.com/kubernetes/kops/pull/18093
* Release 1.32.4 by @hakman in https://github.com/kubernetes/kops/pull/18094


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.32.2...v1.32.4