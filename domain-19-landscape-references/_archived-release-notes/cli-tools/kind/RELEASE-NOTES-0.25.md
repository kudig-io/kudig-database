---
title: kind v0.25 Release Notes
description: kind v0.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- ingress
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.25 Release Notes 是什么
- 如何 kind v0.25 Release Notes
trigger_keywords:
- kind
- v0.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kind v0.25 Release Notes

Source: [v0.25.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.25.0)

This release contains a number of small networking fixes and the latest [[Kubernetes|Kubernetes]] releases. Happy KubeCon!

<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is now Kubernetes v1.31.2: `kindest/node:v1.31.2@sha256:18fbefc20a7113353c7b75b5c869d7145a6abd6269154825872dc59c1329912e`

<h1 id="new-features">New Features</h1>

- Improved documentation for [[Ingress|Ingress]] installation
- Updated to latest go 1.22.x (1.22.9)

Images pre-built for this release:

- v1.31.2: `kindest/node:v1.31.2@sha256:18fbefc20a7113353c7b75b5c869d7145a6abd6269154825872dc59c1329912e`
- v1.30.6: `kindest/node:v1.30.6@sha256:b6d08db72079ba5ae1f4a88a09025c0a904af3b52387643c285442afb05ab994`
- v1.29.10: `kindest/node:v1.29.10@sha256:3b2d8c31753e6c8069d4fc4517264cd20e86fd36220671fb7d0a5855103aa84b`
- v1.28.15: `kindest/node:v1.28.15@sha256:a7c05c7ae043a0b8c818f5a06188bc2c4098f6cb59ca7d1856df00375d839251`
- v1.27.16: `kindest/node:v1.27.16@sha256:2d21a61643eafc439905e18705b8186f3296384750a835ad7a005dceb9546d20`
- v1.26.15: `kindest/node:v1.26.15@sha256:c79602a44b4056d7e48dc20f7504350f1e87530fe953428b792def00bc1076dd`

Additional images pre-built for this release:
- v1.32.0: `kindest/node:v1.32.0@sha256:2458b423d635d7b01637cac2d6de7e1c1dca1148a2ba2e90975e214ca849e7cb`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Updated kube-network-policies with a DNS fix for network policies
- Fix conflict with developing kube-network-policies
- Detect new docker ipv6 failure message and fallback to ipv4 only gracefully
  - Fixes github codespaces, however we recommend ensuring ip6tables works on your host or at least disabling ipv6 in the docker daemon settings then: https://docs.docker.com/engine/release-notes/27/#ipv6
- Workaround podman no longer returning host IP for portmaps
- Aggregate ipmasq sync errors in kindnetd


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release!**

Users whose commits are in this release (alphabetically by user name)

- @aojea
- @AkihiroSuda
- @BenTheElder
- @CharlieTLe
- @dependabot[bot]
- @elieser1101
- @k8s-ci-robot
- @khanhtc1202
- @kebe7jun
- @neolit123
- @network-charles
- @richburroughs
- @stmcginnis
- @sword-jin


Thank you to everyone who contributed in any way.

A special thank you to @neolit123 for all your help over the years, and stepping down when you no longer had the time.
Thank you!
