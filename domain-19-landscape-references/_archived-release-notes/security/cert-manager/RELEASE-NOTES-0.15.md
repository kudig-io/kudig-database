---
title: Kubernetes v0.15 Release Notes
description: Kubernetes v0.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- apiserver
- kubelet
- scheduler
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v0.15 Release Notes 是什么
- 如何 Kubernetes v0.15 Release Notes
trigger_keywords:
- Kubernetes
- v0.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
created: "2026-05-23"
---

# [[Kubernetes|Kubernetes]] v0.15 Release Notes

Source: GitHub Release [v0.15.0](https://github.com/kubernetes/kubernetes/releases/tag/v0.15.0)

### Release 0.15.0
- Enables v1beta3 API and sets it to the default API version (#6098)
  - See the [v1beta3 conversion guide](https://github.com/GoogleCloudPlatform/kubernetes/blob/master/docs/api.md#v1beta3-conversion-tips)
- Added multi-port Services (#6182)
- New Getting Started Guides
  - Multi-node local startup guide (#6505)
  - JUJU (#5414)
  - Mesos on Google Cloud Platform (#5442)
  - Ansible Setup instructions (#6237)
- Added a controller framework (#5270, #5473)
- The [[kubelet|Kubelet]] now listens on a secure HTTPS port (#6380)
- Made kubectl errors more user-friendly (#6338)
- The apiserver now supports client cert authentication (#6190)
- The apiserver now limits the number of concurrent requests it processes (#6207)
- Added rate limiting to pod deleting (#6355)
- Implement Balanced Resource Allocation algorithm as a PriorityFunction in scheduler package (#6150)
- Enabled log collection from master (#6396)
- Added an api endpoint to pull logs from [[Pods|Pods]] (#6497)
- Added latency metrics to scheduler (#6368)
- Added latency metrics to REST client (#6409)
- [[etcd|etcd]] now runs in a pod on the master (#6221)
- nginx now runs in a container on the master (#6334)
- Began creating Docker images for master components (#6326)
- Updated GCE provider to work with gcloud 0.9.54 (#6270)
- Updated AWS provider to fix Region vs Zone semantics (#6011)
- Record event when image GC fails (#6091)
- Add a QPS limiter to the kubernetes client (#6203)
- Decrease the time it takes to run make release (#6196)
- New volume support
  - Added iscsi volume plugin (#5506)
  - Added glusterfs volume plugin (#6174)
  - AWS EBS volume support (#5138)
- Updated to heapster version to v0.10.0 (#6331)
- Updated to etcd 2.0.9 (#6544)
- Updated to Kibana to v1.2 (#6426)
- Bug Fixes
  - Kube-proxy now updates iptables rules if a service's public IPs change (#6123)
  - Retry kube-addons creation if the initial creation fails (#6200)
  - Make kube-proxy more resiliant to running out of file descriptors (#6727)

| binary | hash alg | hash |
| --- | --- | --- |
| `kubernetes.tar.gz` | md5 | `5fcec5fab9ae1885ec7c321855f219f0` |
| `kubernetes.tar.gz` | sha1 | `80ca43c637dd53e8e7fc25ed66fc26a2dbdae10c` |
