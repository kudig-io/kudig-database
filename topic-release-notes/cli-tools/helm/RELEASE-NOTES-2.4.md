---
title: helm v2.4 Release Notes
description: helm v2.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- rbac
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v2.4 Release Notes 是什么
- 如何 helm v2.4 Release Notes
trigger_keywords:
- helm
- v2.4
- Release
- Notes
- release
- notes
---

# helm v2.4 Release Notes

Source: [v2.4.2](https://github.com/helm/helm/releases/tag/v2.4.2)

This release includes several bug fixes. Users of Helm 2.4.1 are encouraged to update.

Notable changes include:

- Fix for the `--wait` flag
- Failed runs of `helm test` return non-zero codes
- And a `--devel` flag on fetch, install, and upgrade now installs unstable chart versions

The last item is to provide a convenience for installing charts whose versions indicate that the chart is unstable (e.g. `foo-1.2.3-alpha.1`). A change to Helm 2.4.0 caused `helm install` to never automatically install an unstable version. This behavior can be overridden with `--devel`.

The community keeps growing, and we'd love to see you there.

- Join the discussion in [Kubernetes Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-devs` for discussing PRs, code, and bugs 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/4526666954)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Installation and Upgrading

Download Helm 2.4.2. The common platform binaries are here:

- [OSX](https://get.helm.sh/helm-v2.4.2-darwin-amd64.tar.gz)
- [Linux](https://get.helm.sh/helm-v2.4.2-linux-amd64.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.4.2-windows-amd64.zip)

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

_Note:_ On Kubernetes 1.6, you will need to decide how to configure your RBACs for Tiller.

The [Quickstart Guide](https://github.com/kubernetes/helm/blob/master/docs/quickstart.md) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://github.com/kubernetes/helm/blob/master/docs/install.md). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## What's Next?
- Helm 2.4.3 will be our next patch release.
- Helm 2.5.0 is the next major release.

## Changelog

- fix(helm): add --devel flag to allow dev releases again 0b4c60b7d4ddf5ed75997efdbe974446554e2f0b (Matt Butcher)
- Revert "Added tests for --repo flag for helm fetch command" f2726258a5a8b7a239ee6346aaa1e0ff09a55f68 (Matt Butcher)
- chore(glide): update to Sprig 2.12.0 10432b5e52ccc6b4be7b8f1461d5cc28de3c6424 (Matt Butcher)
- fix(Dockerfile): add ca-certificates f9696b9f42f2215a02b01cccc1f149d2780493ba (Matt Butcher)
- Updated StartLocalServerForTests to be private method 02652197dc0fb1f91a7a3712951a97b87c21596f (Sushil Kumar)
- Added tests for --repo flag for helm fetch command 4222d6de9be27b9e2099468183d9d451ae797446 (Sushil Kumar)
- fix(lint): add KubeVersion and TillerVersion to linter 504f0f4d0a8786a9125ebd034a33469b428bc2c9 (Matt Butcher)
- fix(*): return non-zero exit code on test failure ddfd9a05aa91d38291fb18388f2a965404e46fcf (Michelle Noorali)
- Return error exit-code in case of error 52ccf9274818ccda5dff8bc0a5fa289c66ad820e (Sushil Kumar)
- Updated review comments :) 11f3091b0e81ad4c38e83abbaae3bf7862d21aed (Sushil Kumar)
- Fixes messages for plugin remove option 61d01766985b7bc58649eb9b0f4f955524fa282c (Sushil Kumar)
- Errors out in case requested plugin exists ce4fc14eb9ade0d165652d00abde24b9b209481d (Sushil Kumar)
- Errors out in case requested plugin version does not exists 8102b4b852e24758f727e53cf5d0f3ba0f376e7e (Sushil Kumar)
- fix(Dockerfile): only copy tiller binary in Dockerfile c27b24fb2e162cbbd69be493e101b37e37bdc070 (Adam Reese)
- fix(tiller): Fixes bug with `--wait` and updated deployments 79c492b6aba9f018608d635530f00a17bf3539b8 (Taylor Thomas)