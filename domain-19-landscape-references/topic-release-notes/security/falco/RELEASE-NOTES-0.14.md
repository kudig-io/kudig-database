---
title: falco v0.14 Release Notes
description: falco v0.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
- daemonset
- rbac
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.14 Release Notes 是什么
- 如何 falco v0.14 Release Notes
trigger_keywords:
- falco
- v0.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- ebpf-basics
---

# falco v0.14 Release Notes

Source: [0.14.0](https://github.com/falcosecurity/falco/releases/tag/0.14.0)

Released 2019-02-06

## Major Changes

* Rules versioning support: The falco engine and executable now have an *engine version* that represents the fields they support. Similarly, rules files have an optional *required_engine_version: NNN* object that names the minimum engine version required to read that rules file. Any time the engine adds new fields, event sources, etc, the engine version will be incremented, and any time a rules file starts using new fields, event sources, etc, the required engine version will be incremented. [[#492](https://github.com/falcosecurity/falco/pull/492)]

* Allow SSL for K8s audit endpoint/embedded webserver [[#471](https://github.com/falcosecurity/falco/pull/471)]

* Add stale issues bot that automatically flags old github issues as stale after 60 days of inactivity and closes issues after 67 days of inactivity. [[#500](https://github.com/falcosecurity/falco/pull/500)]

* Support bundle: When run with `--support`, falco will print a json object containing necessary information like falco version, command line, operating system information, and falco rules files contents. This could be useful when reporting issues. [[#517](https://github.com/falcosecurity/falco/pull/517)]

## Minor Changes

* Support new third-party library dependencies from open source sysdig. [[#498](https://github.com/falcosecurity/falco/pull/498)]

* Add CII best practices badge. [[#499](https://github.com/falcosecurity/falco/pull/499)]

* Fix kernel module builds when running on centos as a container by installing gcc 5 by hand instead of directly from debian/unstable. [[#501](https://github.com/falcosecurity/falco/pull/501)]

* Mount `/etc` when running as a container, which allows container to build kernel module/ebpf program on COS/Minikube. [[#475](https://github.com/falcosecurity/falco/pull/475)]

* Improved way to specify the source of generic event objects [[#480](https://github.com/falcosecurity/falco/pull/480)]

* Readability/clarity improvements to K8s Audit/K8s Daemonset READMEs. [[#503](https://github.com/falcosecurity/falco/pull/503)]

* Add additional RBAC permissions to track deployments/daemonsets/replicasets. [[#514](https://github.com/falcosecurity/falco/pull/514)]

## Bug Fixes

* Fix formatting of nodejs examples README [[#502](https://github.com/falcosecurity/falco/pull/502)]

## Rule Changes

* Remove FPs for `Launch Sensitive Mount Container` rule [[#509](https://github.com/falcosecurity/falco/pull/509/files)]

* Update Container rules/macros to use the more reliable `container.image.{repository,tag}` that always return the repository/tag of an image instead of `container.image`, which may not for some docker daemon versions. [[#513](https://github.com/falcosecurity/falco/pull/513)]
