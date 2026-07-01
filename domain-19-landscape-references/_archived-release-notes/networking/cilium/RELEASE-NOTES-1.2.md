---
title: cilium v1.2 Release Notes
description: cilium v1.2 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- cilium
- docker
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
- cilium v1.2 Release Notes 是什么
- 如何 cilium v1.2 Release Notes
trigger_keywords:
- cilium
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---



# [[Cilium|cilium]] v1.2 Release Notes

Source: [v1.2.8](https://github.com/cilium/cilium/releases/tag/v1.2.8)

```
    André Martins (1):
          docs: bump copyright headers to 2017-2019
    
    Dmitry Kharitonov (4):
          added copy buttons for code blocks
          downgrade to es5 syntax
          correct button labels for various cases
          docs: fixed copy buttons icon
    
    Ian Vernon (1):
          endpoint: signal when BPF program is compiled for the first time
    
    Joe Stringer (7):
          contrib: Accept multiple commits in 'cherry-pick'
          docs: Streamline and tidy backporting docs
          backporting: Add summary log option to check-stable
          docs: Update backporting for the latest scripts
          Makefile: Serve render-docs on port 9080.
          docs: Fix backporting shell example formatting
          backporting: Add set-labels commands to check-stable
    
    Martynas Pumputis (1):
          docs: Add note about triggering builds with net-next
    
    Ray Bejjani (1):
          fqdn: Avoid regenerations on each poller update
    
    Romain Lenglet (1):
          endpoint: Update LXC map before proxy ack wait and signalling
    
    Thomas Graf (2):
          kvstore: Release local kvstore lock after timeout
          kvstore: Decrease stale lock timeout from 2 minutes to 30 seconds
```

Release binaries
----------------

* [cilium-agent-x86_64](http://releases.cilium.io/v1.2.8/cilium-agent-x86_64) ([723e9869960747a8ac78](http://releases.cilium.io/v1.2.8/cilium-agent-x86_64.sha256sum))
* [cilium-bugtool-x86_64](http://releases.cilium.io/v1.2.8/cilium-bugtool-x86_64) ([78859689d4c0d45980fd](http://releases.cilium.io/v1.2.8/cilium-bugtool-x86_64.sha256sum))
* [cilium-cni-x86_64](http://releases.cilium.io/v1.2.8/cilium-cni-x86_64) ([47bfcf486d831465984c](http://releases.cilium.io/v1.2.8/cilium-cni-x86_64.sha256sum))
* [cilium-docker-x86_64](http://releases.cilium.io/v1.2.8/cilium-docker-x86_64) ([27a0548f6fc6041d35fe](http://releases.cilium.io/v1.2.8/cilium-docker-x86_64.sha256sum))
* [cilium-envoy-x86_64](http://releases.cilium.io/v1.2.8/cilium-envoy-x86_64) ([14a3fc5bb9d63a63c025](http://releases.cilium.io/v1.2.8/cilium-envoy-x86_64.sha256sum))
* [cilium-health-x86_64](http://releases.cilium.io/v1.2.8/cilium-health-x86_64) ([f32efec30683fcb02948](http://releases.cilium.io/v1.2.8/cilium-health-x86_64.sha256sum))
* [cilium-node-monitor-x86_64](http://releases.cilium.io/v1.2.8/cilium-node-monitor-x86_64) ([d992f3884da0fa17a256](http://releases.cilium.io/v1.2.8/cilium-node-monitor-x86_64.sha256sum))
* [cilium-x86_64](http://releases.cilium.io/v1.2.8/cilium-x86_64) ([43387c0796e44dd9838e](http://releases.cilium.io/v1.2.8/cilium-x86_64.sha256sum))
* [v1.2.8.tar.gz](http://releases.cilium.io/v1.2.8/v1.2.8.tar.gz) ([308091810fba2714a50a](http://releases.cilium.io/v1.2.8/v1.2.8.tar.gz.sha256sum))
* [v1.2.8.zip](http://releases.cilium.io/v1.2.8/v1.2.8.zip) ([350a3a0092ae97c57bcd](http://releases.cilium.io/v1.2.8/v1.2.8.zip.sha256sum)