---
title: cert-manager v1.7 Release Notes
description: cert-manager v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v1.7 Release Notes 是什么
- 如何 cert-manager v1.7 Release Notes
trigger_keywords:
- cert-manager
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v1.7 Release Notes

Source: [v1.7.3](https://github.com/cert-manager/cert-manager/releases/tag/v1.7.3)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.7.3 is in effect a bug fix release which increases some hard-coded timeouts which were preventing the use of certain ACME issuers
which sometimes had slower response times. This is known to include ZeroSSL and Sectigo.

These issues were reported by many users. We'd like to thank the following for their help and feedback on this topic:

- @JoooostB
- @fatz
- @jgreat
- @sashokbg
- @mycloudedu
- @hadogenes
- @SudonymTM
- @amalucelli
- @MilheiroSantos
- @dverbeek84
- @kxs-jnadeau
- @fablarosa
- @nik-nazarov
- @omBratteng
- @shubham-root
- @alphabet5
- @hawksight

Thanks also to the cert-manager maintainers who were involved in reviewing this fix and helping to move things forwards:

- @irbekrm
- @jahrlin
- @maelvls
- @JoshVanL
- @wallrj
- @jakexks
- @munnerz

## Changes since v1.7.2

### Bug

- Increase timeouts for issuer and clusterissuer controllers to 2 minutes and increase ACME client HTTP timeouts to 90 seconds, in order to enable the use of slower ACME issuers which take a long time to process certain requests. ([#5232](https://github.com/cert-manager/cert-manager/pull/5232), @JoooostB @SgtCoDFish)

### Other (Cleanup)

- Bumps go to 1.17.11 and base images to latest distroless base images ([#5234](https://github.com/cert-manager/cert-manager/pull/5234), @SgtCoDFish)
