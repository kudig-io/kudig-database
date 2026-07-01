---
title: falco v0.16 Release Notes
description: falco v0.16 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- calico
- falco
- rag
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
- falco v0.16 Release Notes 是什么
- 如何 falco v0.16 Release Notes
trigger_keywords:
- falco
- v0.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
---



# [[Falco|falco]] v0.16 Release Notes

Source: [0.16.0](https://github.com/falcosecurity/falco/releases/tag/0.16.0)

Released 2019-07-16

## Major Changes

* Clean up error reporting to provide more meaningful error messages along with context when loading rules files. When run with -V, the results of the validation ("OK" or error message) are sent to standard output. [[#708](https://github.com/falcosecurity/falco/pull/708)]

* Improve rule loading performance by optimizing lua parsing paths to avoid expensive pattern matches. [[#694](https://github.com/falcosecurity/falco/pull/694)]

* Bump falco engine version to 4 to reflect new fields `ka.useragent`, others. [[#710](https://github.com/falcosecurity/falco/pull/710)] [[#681](https://github.com/falcosecurity/falco/pull/681)]

* Add Catch2 as a unit testing framework. This will add additional coverage on top of the regression tests using Avocado. [[#687](https://github.com/falcosecurity/falco/pull/687)]

## Minor Changes

* Add SYSDIG_DIR Cmake option to specify location for sysdig source code when building falco. [[#677](https://github.com/falcosecurity/falco/pull/677)] [[#679](https://github.com/falcosecurity/falco/pull/679)] [[#702](https://github.com/falcosecurity/falco/pull/702)]

* New field `ka.useragent` reports the useragent from k8s audit events. [[#709](https://github.com/falcosecurity/falco/pull/709)]

* Add clang formatter for C++ syntax formatting. [[#701](https://github.com/falcosecurity/falco/pull/701)] [[#689](https://github.com/falcosecurity/falco/pull/689)]

* Partial changes towards lua syntax formatting. No particular formatting enforced yet, though. [[#718](https://github.com/falcosecurity/falco/pull/718)]

* Partial changes towards yaml syntax formatting. No particular formatting enforced yet, though. [[#714](https://github.com/falcosecurity/falco/pull/714)]

* Add cmake syntax formatting. [[#703](https://github.com/falcosecurity/falco/pull/703)]

* Token bucket unit tests and redesign. [[#692](https://github.com/falcosecurity/falco/pull/692)]

* Update github PR template. [[#699](https://github.com/falcosecurity/falco/pull/699)]

* Fix PR template for kind/rule-*. [[#697](https://github.com/falcosecurity/falco/pull/697)]

## Bug Fixes

* Remove an unused cmake file. [[#700](https://github.com/falcosecurity/falco/pull/700)]

* Misc Cmake cleanups. [[#673](https://github.com/falcosecurity/falco/pull/673)]

* Misc k8s install docs improvements. [[#671](https://github.com/falcosecurity/falco/pull/671)]

## Rule Changes

* Allow k8s.gcr.io/kube-proxy image to run privileged. [[#717](https://github.com/falcosecurity/falco/pull/717)]

* Add runc to the list of possible container entrypoint parents. [[#712](https://github.com/falcosecurity/falco/pull/712)]

* Skip Source RFC 1918 addresses when considering outbound connections. [[#685](https://github.com/falcosecurity/falco/pull/685)]

* Add additional `user_XXX` placeholder macros to allow for easy customization of rule exceptions. [[#685](https://github.com/falcosecurity/falco/pull/685)]

* Let weaveworks programs change namespaces. [[#685](https://github.com/falcosecurity/falco/pull/685)]

* Add additional openshift images. [[#685](https://github.com/falcosecurity/falco/pull/685)]

* Add openshift as a k8s binary. [[#678](https://github.com/falcosecurity/falco/pull/678)]

* Add dzdo as a binary that can change users. [[#678](https://github.com/falcosecurity/falco/pull/678)]

* Allow azure/calico binaries to change namespaces. [[#678](https://github.com/falcosecurity/falco/pull/678)]

* Add back trusted_containers list for backport compatibility [[#675](https://github.com/falcosecurity/falco/pull/675)]

* Add mkdirat as a syscall for mkdir operations. [[#667](https://github.com/falcosecurity/falco/pull/667)]

* Add container id/repository to rules that can work with containers. [[#667](https://github.com/falcosecurity/falco/pull/667)]