---
title: kops v1.5 Release Notes
description: kops v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flannel
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.5 Release Notes 是什么
- 如何 kops v1.5 Release Notes
trigger_keywords:
- kops
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
created: "2026-05-23"
---

# kops v1.5 Release Notes

Source: [1.5.3](https://github.[[entities/kubernetes|kubernetes]]/kops/releases/tag/1.5.3)

* **Important for Terraform Users** Make ELB naming unambiguous by including the full cluster name.  This will cause the ELBs to be recreated if using Terraform with private topologies, causing disruption of external access to the API and of external access to the bastion (if enabled).  Expected disruption is less than 5 minutes.  Use `export KOPS_FEATURE_FLAGS=+UseLegacyELBName` to keep the legacy naming and avoid disruption.  Fix #1899

* Fix terraform output of shared subnets.  Fix #1977
* Add support for i3 instances (thanks @geojaz)

* Experimental drain rolling-update, 
* Experimental GCE support

* Update Weave to v1.9.3
* Put flannel in guaranteed class (thanks @mihok)
* DNS autoscaler fixes (thanks @MrHohn)
* Remove legacy flags (thanks @mtaufen)
* Add route53 mapper addon (thanks @itskingori)
* Build fixes (thanks @zmerlynn)
* Disable cloudformation delete (thanks @kris-nova)

* Docs fixes (thanks @bowei, @jonchiu, @dosullivan, @DualSpark, @foxylion, @kris-nova
