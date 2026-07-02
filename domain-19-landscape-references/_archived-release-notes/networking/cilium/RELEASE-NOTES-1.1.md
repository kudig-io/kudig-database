---
title: cilium v1.1 Release Notes
description: cilium v1.1 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- cilium
- docker
- kafka
- ingress
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
- cilium v1.1 Release Notes 是什么
- 如何 cilium v1.1 Release Notes
trigger_keywords:
- cilium
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Cilium|cilium]] v1.1 Release Notes

Source: [v1.1.6](https://github.com/cilium/cilium/releases/tag/v1.1.6)

Changes
-------

```
    André Martins (12):
          test: set default CRI socket
          Revert "Revert "test: update k8s to 1.8.14, 1.10.4 and 1.11.0""
          test: update k8s to 1.9.9 and 1.10.5
          Revert "Revert "ginkgo-kubernetes-all.Jenkinsfile: move k8s 1.10 and 1.12 to same stage""
          tests: disable k8s 1.12-alpha.0 tests
          Revert "test/k8sT: use specific commit for cilium/star-wars-demo YAMLs"
          test: fix star wars demo to run star-wars v1.0
          fix alignment in Go structs
          test: fix star wars demo
          examples/kubernetes: add better comment for bpf-maps volume
          crio: don't mount bpf path for k8s >= 1.11
          policy: do policy modifications based on the CNP identifiable labels
    
    Daniel Borkmann (2):
          daemon: fix potential nil pointer dereference
          bpf, perf: refine barriers, tail pointer update and buffers
    
    Eloy Coto (2):
          Test: Fix issues with kubernetes test on old branchs.
          Test/Demos: Make assert more robust.
    
    Ian Vernon (1):
          test/k8sT: wait for DNS to be ready in Kafka pods
    
    Jarno Rajahalme (8):
          envoy: Make NACK cancel the WaitGroup
          xds: Start versioning at 1.
          envoy: Pass error detail when NACK
          envoy: Update generated protobufs
          envoy: Use separate clusters for egress and ingress redirects.
          bpf: Do not redirect replies from a pod to a proxyport.
          bpf: Use 'forwarding_reason' instead of potentially overwritten 'ret'
          envoy: Pass nil completion if Acks are not expected.

    Joe Stringer (7):
          lxcmap: Fix invalid dumping of IPv4 entries
          daemon: Improve syncLXCMap failure log
          bpf: Add basic endpointKey.ToIP() test
          examples/kubernetes: Clean up pidfiles on startup
          examples/kubernetes: Synchronize CRIO init YAMLs
          pidfile: Add 'Remove' to provide pidfile deletion
          daemon: Clean up k8s health EP pidfile on startup
    
    Maciej Kwiek (1):
          protect bpf.PerfEvent.Read from infinite loop
    
    Romain Lenglet (2):
          test: Fix the semantics of WithTimeout's Timeout
          proxy: Check whether a port is already open before allocating
    
    Thomas Graf (5):
          k8s: Include type of derived k8s resource in policy rule
          k8s: Fix CNP delete handling to not rely on rules being embedded
          agent: Fix temporary corruption of BPF endpoint map on restart
          bpf: Avoid additional cgo call per perf read
          endpoint: Skip conntrack clean on endpoint restore
```

Release binaries
----------------

* [cilium-agent-x86_64](http://releases.cilium.io/v1.1.6/cilium-agent-x86_64) ([1f3200c7ed5c7d02d300](http://releases.cilium.io/v1.1.6/cilium-agent-x86_64.sha256sum))
* [cilium-bugtool-x86_64](http://releases.cilium.io/v1.1.6/cilium-bugtool-x86_64) ([36cac852f0b1225dec14](http://releases.cilium.io/v1.1.6/cilium-bugtool-x86_64.sha256sum))
* [cilium-cni-x86_64](http://releases.cilium.io/v1.1.6/cilium-cni-x86_64) ([3c0ddfd2abf583d5d497](http://releases.cilium.io/v1.1.6/cilium-cni-x86_64.sha256sum))
* [cilium-docker-x86_64](http://releases.cilium.io/v1.1.6/cilium-docker-x86_64) ([c50ae479ee72d410e941](http://releases.cilium.io/v1.1.6/cilium-docker-x86_64.sha256sum))
* [cilium-envoy-x86_64](http://releases.cilium.io/v1.1.6/cilium-envoy-x86_64) ([0e10db2f91c072d6e2cf](http://releases.cilium.io/v1.1.6/cilium-envoy-x86_64.sha256sum))
* [cilium-health-x86_64](http://releases.cilium.io/v1.1.6/cilium-health-x86_64) ([e75356cd1bf06e5e93d0](http://releases.cilium.io/v1.1.6/cilium-health-x86_64.sha256sum))
* [cilium-node-monitor-x86_64](http://releases.cilium.io/v1.1.6/cilium-node-monitor-x86_64) ([a9e24912409fe0840957](http://releases.cilium.io/v1.1.6/cilium-node-monitor-x86_64.sha256sum))
* [cilium-x86_64](http://releases.cilium.io/v1.1.6/cilium-x86_64) ([47b8b7136dbf3db24c91](http://releases.cilium.io/v1.1.6/cilium-x86_64.sha256sum))
* [v1.1.6.tar.gz](http://releases.cilium.io/v1.1.6/v1.1.6.tar.gz) ([fb8efc56c584c5cf27f9](http://releases.cilium.io/v1.1.6/v1.1.6.tar.gz.sha256sum))
* [v1.1.6.zip](http://releases.cilium.io/v1.1.6/v1.1.6.zip) ([d880dffcc28d29075103](http://releases.cilium.io/v1.1.6/v1.1.6.zip.sha256sum))


<!-- risk-assessed -->
