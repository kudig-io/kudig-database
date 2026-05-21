---
title: helm v2.0 Release Notes
description: helm v2.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v2.0 Release Notes 是什么
- 如何 helm v2.0 Release Notes
trigger_keywords:
- helm
- v2.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

# helm v2.0 Release Notes

Source: [v2.0.2](https://github.com/helm/helm/releases/tag/v2.0.2)

The Helm 2.0.2 release is a bug fix release. No new features have been added.
- Join the discussion in [Kubernetes Slack](https://slack.k8s.io/): `#helm` 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://engineyard.zoom.us/j/366425549)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Notable Changes Since 2.0.1
- A substantial bug with `helm upgrade -f` has been fixed
- An updated Sprig library (2.7.0) has been included because it contains a substantial bug fix for `quote`
- `helm upgrade` and `helm rollback` can both deal with failed releases now
- gRPC's max message size is now 10M instead of 4M (which helps with large charts)

Version 2.0.2 is compatible with other version 2.0.x releases (client and server versions can be intermixed).

## Installing and Updating

Helm binaries:
- [OSX](https://get.helm.sh/helm-v2.0.2-darwin-amd64.tar.gz)
- [Linux](https://get.helm.sh/helm-v2.0.2-linux-amd64.tar.gz)
- [Linux i386](https://get.helm.sh/helm-v2.0.2-linux-386.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.0.2-windows-amd64.zip)

The [Quickstart Guide](https://github.com/kubernetes/helm/blob/master/docs/quickstart.md) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://github.com/kubernetes/helm/blob/master/docs/install.md). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## What Next?
- A 2.0.3 milestone exists, and we'll use that to capture any critical issues that come up before 2.1.0
- The 2.1.0 milestone is getting closer, and we may release early for Kubernetes 1.5 compatibility 

## All Changes Since 2.0.1

fix(tiller): fix spurious "no release found" errors. 9bcfa1f16e5d9de4503c848cbf06c423d954d164
fix(tiller): increase the max message size for grpc 98e0b97072025fd10f1d16c5f39ad8170b094308
fix(helm): give different error if key is not private 3cb2fd7fbbd2b41aa50fef2051ae4d51be9e9488
fix(helm): add trimSuffix to helper functions d8133cf9a88d4c95ffd83f207dedad3102574ce2
fix(helm): fix broken --values flag 70256d812c924d682d4246fac875eb9c1562c162
feat(tiller): update sprig to 2.7.0 8f2567ecb4f34efd5493da595fcb3154069f4c30
fix(ci): do not push canary image on release 7d79406b95f99c2968999b59e8e9043ddfd61827
