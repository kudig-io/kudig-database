---
title: cilium v1.0 Release Notes
description: cilium v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- cilium
- docker
- ingress
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.0 Release Notes 是什么
- 如何 cilium v1.0 Release Notes
trigger_keywords:
- cilium
- v1.0
- Release
- Notes
- release
- notes
---

# cilium v1.0 Release Notes

Source: [v1.0.7](https://github.com/cilium/cilium/releases/tag/v1.0.7)

Changes
-------

```
    Daniel Borkmann (1):
          bpf, perf: refine barriers, tail pointer update and buffers
    
    Eloy Coto (1):
          Test/Demos: Make assert more robust.
    
    Jarno Rajahalme (3):
          envoy: Use separate clusters for egress and ingress redirects.
          bpf: Do not redirect replies from a pod to a proxyport.
          bpf: Use 'forwarding_reason' instead of potentially overwritten 'ret'
    
    Joe Stringer (4):
          daemon: Improve syncLXCMap failure log
          lxcmap: Fix invalid dumping of IPv4 entries
          bpf: Add basic endpointKey.ToIP() test
          examples/kubernetes: Clean up pidfiles on startup
    
    John Fastabend (1):
          agent: Fix backport to resolve temporary endpoing map corruption
    
    Maciej Kwiek (1):
          protect bpf.PerfEvent.Read from infinite loop
    
    Romain Lenglet (2):
          proxy: Check whether a port is already open before allocating
          controller: Fix controller update
    
    Thomas Graf (3):
          launcher: Wait for process to exit and release resources
          agent: Fix temporary corruption of BPF endpoint map on restart
          bpf: Avoid additional cgo call per perf read
```

Release binaries
----------------

* [cilium-agent-x86_64](http://releases.cilium.io/v1.0.7/cilium-agent-x86_64) ([a3ac52970a18993ffb3e](http://releases.cilium.io/v1.0.7/cilium-agent-x86_64.sha256sum))
* [cilium-bugtool-x86_64](http://releases.cilium.io/v1.0.7/cilium-bugtool-x86_64) ([ff43893b1c0705438918](http://releases.cilium.io/v1.0.7/cilium-bugtool-x86_64.sha256sum))
* [cilium-cni-x86_64](http://releases.cilium.io/v1.0.7/cilium-cni-x86_64) ([10ac9b47338a0b3075eb](http://releases.cilium.io/v1.0.7/cilium-cni-x86_64.sha256sum))
* [cilium-docker-x86_64](http://releases.cilium.io/v1.0.7/cilium-docker-x86_64) ([0957b8a90db89b42aad6](http://releases.cilium.io/v1.0.7/cilium-docker-x86_64.sha256sum))
* [cilium-envoy-x86_64](http://releases.cilium.io/v1.0.7/cilium-envoy-x86_64) ([238774bb3dc0299d202c](http://releases.cilium.io/v1.0.7/cilium-envoy-x86_64.sha256sum))
* [cilium-health-x86_64](http://releases.cilium.io/v1.0.7/cilium-health-x86_64) ([13808606faf9a027f128](http://releases.cilium.io/v1.0.7/cilium-health-x86_64.sha256sum))
* [cilium-node-monitor-x86_64](http://releases.cilium.io/v1.0.7/cilium-node-monitor-x86_64) ([f9cda29e1d4eb994543b](http://releases.cilium.io/v1.0.7/cilium-node-monitor-x86_64.sha256sum))
* [cilium-x86_64](http://releases.cilium.io/v1.0.7/cilium-x86_64) ([4310157875f28cd1f9d3](http://releases.cilium.io/v1.0.7/cilium-x86_64.sha256sum))
* [v1.0.7.tar.gz](http://releases.cilium.io/v1.0.7/v1.0.7.tar.gz) ([fb678d81909a2953eacb](http://releases.cilium.io/v1.0.7/v1.0.7.tar.gz.sha256sum))
* [v1.0.7.zip](http://releases.cilium.io/v1.0.7/v1.0.7.zip) ([b317dfe79e7599632d4f](http://releases.cilium.io/v1.0.7/v1.0.7.zip.sha256sum))
