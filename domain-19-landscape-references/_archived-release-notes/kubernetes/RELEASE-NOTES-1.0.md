---
title: Kubernetes v1.0 Release Notes
description: Kubernetes v1.0 Release Notes — Kubernetes 生产运维知识库
summary: Kubernetes v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
- flannel
- minio
- job
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
- Kubernetes v1.0 Release Notes 是什么
- 如何 Kubernetes v1.0 Release Notes
trigger_keywords:
- Kubernetes
- v1.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Kubernetes|Kubernetes]] v1.0 Release Notes

Source: GitHub Release [v1.0.7](https://github.com/kubernetes/kubernetes/releases/tag/v1.0.7)

## [Documentation](http://releases.k8s.io/v1.0.7/docs/README.md)

## [Examples](http://releases.k8s.io/v1.0.7/examples)

## Changes since 1.0.6
- Set rlimit for openfile handles to 64k #14191 (ArtfulCoder)
- fixed log format #14882 (ArtfulCoder)
- Added UdpIdleTimeout flag #15797 (ArtfulCoder)
- Add a cloud-provider hook to scrub DNS for [[Pods|pods]] #16219 (thockin)
- AWS: Create one storage pool for aufs, not two #13803 (justinsb)
- Rename e2e-gce-release job to e2e-gce-release-1.0 #15410 (jlowdermilk)
- Add script to use gcloud to print GCP resources used, and call in Jenkins runs #15189 (ixdy)
- Adding retry logic around service updates #11077 (krousey)
- Use the cluster name instead of the minion tag as the prefix for the firewall rules created in gke e2e tests #14333 (roberthbailey)
- Fix typo that caused an error at end of vagrant up #13154 (derekwaynecarr)
- Stop allowing unnamespaced POST for namespaced objects #11252 (nikhiljindal)
- Move Vagrant provider to Flannel #13986 (derekwaynecarr)
- Don't reuse credentials on cluster create #13068 (jlowdermilk)
- Add config for the main gke jenkins jobs to e2e.sh #13863 (jlowdermilk)
- rate-limit events record in kubelet #13192 (jiangyaoguo)

| binary | hash alg | hash |
| --- | --- | --- |
| `kubernetes.tar.gz` | md5 | `11756de09344eed8de4cfbe6340aa20a` |
| `kubernetes.tar.gz` | sha1 | `e564d3dfa75ec1782e11cf9dd8f420db4f0ed792` |
