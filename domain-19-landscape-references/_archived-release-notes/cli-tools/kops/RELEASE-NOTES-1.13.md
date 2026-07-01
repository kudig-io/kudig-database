---
title: kops v1.13 Release Notes
description: kops v1.13 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- calico
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.13 Release Notes 是什么
- 如何 kops v1.13 Release Notes
trigger_keywords:
- kops
- v1.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
- etcd-basics
---



# kops v1.13 Release Notes

Source: [1.13.2](https://github.com/kubernetes/kops/releases/tag/1.13.2)

kops 1.13 series of kops, supporting [[Kubernetes|kubernetes]] 1.13 and earlier.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.13-NOTES.md) for the full list of changes. 

For existing clusters, please update to kubernetes 1.12 before updating to kubernetes 1.13.  Technically this is always required, but it is particularly important because of the etcd-upgrade that is in kops 1.12.

## Critical changes

* Point to new CentOS binary locations, which was causing nodes not to start when running CentOS [#7609](https://github.com/kubernetes/kops/pull/7609) and [#7674](https://github.com/kubernetes/kops/pull/7674)

## 1.13.0 to 1.13.1

* fix(addons/networking.projectcalico.org) calico kube-controllers is needed in CRD mode [@phspagiari](https://github.com/phspagiari) [#7517](https://github.com/kubernetes/kops/pull/7517)
* Update to golang 1.11.13 [@justinsb](https://github.com/justinsb) [#7549](https://github.com/kubernetes/kops/pull/7549)
* Add more go 1.11.5 -> 1.11.13 [@justinsb](https://github.com/justinsb) [#7552](https://github.com/kubernetes/kops/pull/7552)
* Add logrotate for [[etcd|etcd]]/etcd-events.log [@mikesplain](https://github.com/mikesplain) [#7614](https://github.com/kubernetes/kops/pull/7614)
* Updated container-selinux url to point to the right path [@igarcia-sugarcrm](https://github.com/igarcia-sugarcrm),[@mikesplain](https://github.com/mikesplain) [#7609](https://github.com/kubernetes/kops/pull/7609)
* Check the HTTP response code when downloading URLs [@rifelpet](https://github.com/rifelpet) [#7611](https://github.com/kubernetes/kops/pull/7611)

## 1.13.1 to 1.13.2

* Pull centos.org packages from the vault [@justinsb](https://github.com/justinsb) [#7674](https://github.com/kubernetes/kops/pull/7674)
