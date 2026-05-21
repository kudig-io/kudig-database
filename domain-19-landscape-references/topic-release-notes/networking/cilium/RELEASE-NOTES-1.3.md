---
title: cilium v1.3 Release Notes
description: cilium v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- cilium
- docker
- job
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.3 Release Notes 是什么
- 如何 cilium v1.3 Release Notes
trigger_keywords:
- cilium
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---

# cilium v1.3 Release Notes

Source: [v1.3.8](https://github.com/cilium/cilium/releases/tag/v1.3.8)

Changes
-------

```
André Martins (9):
          test: replace guestbook test docker image
          pkg/endpoint: fix assignment in nil map on restore
          test/helpers: install Cilium's SA if exists
          test/k8sT: remove unused variables
          Jenkinsfile: backport all Jenkinsfile from master
          daemon: fix endpoint restore when endpoints are not available
          pkg/lock: fix RUnlockIgnoreTime
          *.Jenkinsfile: remove leftover failFast
          examples/kubernetes: bump cilium to v1.3.7
    
    Ian Vernon (4):
          contrib: fix up check-fmt.sh
          test: make function provided to WithTimeout run asynchronously
          vendor: Update vishvananda/netlink
          test: bump CNI version to 0.7.5 for K8s 1.12
    
    Ifeanyi Ubah (2):
          pkg/health: Fix IPv6 URL format in HTTP probe
          test: Enable IPv6 forwarding in test VMs
    
    Jarno Rajahalme (3):
          docs: Update urllib3 dependency to address CVE-2019-11324
          proxylib: Fix egress enforcement
          envoy: Prevent resending NACKed resources also when there are no ACK observers.
    
    Joe Stringer (1):
          contrib: Fix cherry-pick script
    
    John Fastabend (1):
          Replace deprecated provider for k8s upstream tests
    
    Maciej Kwiek (8):
          Add jenkins stage for loading vagrant boxes
          Recover from ginkgo fail in WithTimeout helper
          Jenkins separate directories for parallel builds
          Don't set debug to true in monitor test
          Preload vagrant boxes in k8s upstream jenkinsfile
          Change nightly CI job label from fixed to baremetal
          Retry provisioning vagrant vms in CI
          retry vm provisioning, increase timeout
    
    Martynas Pumputis (4):
          contrib: Exit early if no git remote is found
          mac: Add function to generate a random MAC addr
          endpoint: Set random MAC addrs for veth when creating it
          bpf: Set random MAC addrs for cilium interfaces
    
    Ray Bejjani (4):
          CI: Consolidate Vagrant box information into 1 file
          CI: Clean VMs and reclaim disk after jobs complete
          CI: Clean workspace when all stages complete
          CI: Clean VMs and reclaim disk in nightly test
    
    Thomas Graf (1):
          bpf: Remove unneeded debug instructions to stay below instruction limit
    
    刘群 (1):
          doc: fix up Ubuntu apt-get install command
```

Release binaries
----------------

* [cilium-agent-x86_64](http://releases.cilium.io/v1.3.8/cilium-agent-x86_64) ([cb13e98aff462f91df78](http://releases.cilium.io/v1.3.8/cilium-agent-x86_64.sha256sum))
* [cilium-bugtool-x86_64](http://releases.cilium.io/v1.3.8/cilium-bugtool-x86_64) ([410268e8de9ff549cdc9](http://releases.cilium.io/v1.3.8/cilium-bugtool-x86_64.sha256sum))
* [cilium-cni-x86_64](http://releases.cilium.io/v1.3.8/cilium-cni-x86_64) ([e746b2a59e9b2c908672](http://releases.cilium.io/v1.3.8/cilium-cni-x86_64.sha256sum))
* [cilium-docker-x86_64](http://releases.cilium.io/v1.3.8/cilium-docker-x86_64) ([7b715b64a826373199f9](http://releases.cilium.io/v1.3.8/cilium-docker-x86_64.sha256sum))
* [cilium-envoy-x86_64](http://releases.cilium.io/v1.3.8/cilium-envoy-x86_64) ([72730b49425eb0bfad48](http://releases.cilium.io/v1.3.8/cilium-envoy-x86_64.sha256sum))
* [cilium-health-x86_64](http://releases.cilium.io/v1.3.8/cilium-health-x86_64) ([337cc3b925b3893207a4](http://releases.cilium.io/v1.3.8/cilium-health-x86_64.sha256sum))
* [cilium-node-monitor-x86_64](http://releases.cilium.io/v1.3.8/cilium-node-monitor-x86_64) ([b266783a22a5d4bc1353](http://releases.cilium.io/v1.3.8/cilium-node-monitor-x86_64.sha256sum))
* [cilium-x86_64](http://releases.cilium.io/v1.3.8/cilium-x86_64) ([4b497008ac208d60baaa](http://releases.cilium.io/v1.3.8/cilium-x86_64.sha256sum))
* [v1.3.8.tar.gz](http://releases.cilium.io/v1.3.8/v1.3.8.tar.gz) ([e62fb73a20eccabf3ee2](http://releases.cilium.io/v1.3.8/v1.3.8.tar.gz.sha256sum))
* [v1.3.8.zip](http://releases.cilium.io/v1.3.8/v1.3.8.zip) ([f8c1ce1bd66d81c1d970](http://releases.cilium.io/v1.3.8/v1.3.8.zip.sha256sum))