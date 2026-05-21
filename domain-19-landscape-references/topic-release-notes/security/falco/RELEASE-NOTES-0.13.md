---
title: falco v0.13 Release Notes
description: falco v0.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- calico
- falco
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.13 Release Notes 是什么
- 如何 falco v0.13 Release Notes
trigger_keywords:
- falco
- v0.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- cni-basics
---

# falco v0.13 Release Notes

Source: [0.13.1](https://github.com/falcosecurity/falco/releases/tag/0.13.1)

Released 2019-01-16

## Major Changes


## Minor Changes

* Unbuffer outputs by default. This helps make output readable when used in environments like K8s. [[#494](https://github.com/falcosecurity/falco/pull/494)]

* Improved documentation for running Falco within K8s and getting K8s Audit Logging to work with Minikube and Falco as a Daemonset within K8s. [[#496](https://github.com/falcosecurity/falco/pull/496)]

* Fix AWS Permissions for Kubernetes Response Engine [[#465](https://github.com/falcosecurity/falco/pull/465)]

* Tighten compilation flags to include `-Wextra` and `-Werror` [[#479](https://github.com/falcosecurity/falco/pull/479)]

* Add `k8s.ns.name` to outputs when `-pk` argument is used [[#472](https://github.com/falcosecurity/falco/pull/472)]

* Remove kubernetes-response-engine from system:masters [[#488](https://github.com/falcosecurity/falco/pull/488)]

## Bug Fixes

* Ensure `-pc`/`-pk` only apply to syscall rules and not k8s_audit rules [[#495](https://github.com/falcosecurity/falco/pull/495)]

* Fix a potential crash that could occur when using the falco engine and rulesets [[#468](https://github.com/falcosecurity/falco/pull/468)]

* Fix a regression where format output options were mistakenly removed [[#485](https://github.com/falcosecurity/falco/pull/485)]

## Rule Changes

* Fix FPs related to calico and writing files below etc [[#481](https://github.com/falcosecurity/falco/pull/481)]

* Fix FPs related to `apt-config`/`apt-cache`, `apk` [[#490](https://github.com/falcosecurity/falco/pull/490)]

* New rules `Launch Package Management Process in Container`, `Netcat Remote Code Execution in Container`, `Lauch Suspicious Network Tool in Container` look for host-level network tools like `netcat`, package management tools like `apt-get`, or network tool binaries being run in a container. [[#490](https://github.com/falcosecurity/falco/pull/490)]

* Fix the `inbound` and `outbound` macros so they work with sendto/recvfrom/sendmsg/recvmsg. [[#470](https://github.com/falcosecurity/falco/pull/470)]

* Fix FPs related to prometheus/openshift writing config below /etc. [[#470](https://github.com/falcosecurity/falco/pull/470)]