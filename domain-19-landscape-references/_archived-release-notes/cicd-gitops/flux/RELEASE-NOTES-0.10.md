---
title: flux v0.10 Release Notes
description: flux v0.10 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.10 Release Notes 是什么
- 如何 flux v0.10 Release Notes
trigger_keywords:
- flux
- v0.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Flux|flux]] v0.10 Release Notes

Source: [v0.10.0](https://github.com/fluxcd/flux2/releases/tag/v0.10.0)

CHANGELOG
- PR #1117 - @stefanprodan - Add push branch and commit template to image automation guide
- PR #1116 - @stefanprodan - Implement get all for sources and images
- PR #1113 - @stefanprodan - Add repo path and push branch to image update cmd
- PR #1103 - @hiddeco - Fix updating of `go.mod` entries for components
- PR #1102 - @hiddeco - Use Host from parsed URL instead of Hostname
- PR #1098 - @kingdonb - Fix hint in Flux v1 Migration guide
- PR #1095 - @fluxcdbot - Update toolkit components
- PR #1094 - @hiddeco - Move `StatusChecker` to separate and generic pkg
- PR #1093 - @hiddeco - Replace delete opt on GitHub bootstrap with curl in bootstrap action
- PR #1091 - @joebowbeer - Fix deployment name in image update guide 
- PR #1086 - @squat - cmd/flux/export_source*: fix typo in comment
- PR #1075 - @SomtochiAma - Implement flux logs command
- PR #1069 - @hiddeco - docs: fix link to source-controller documentation
- PR #1066 - @hiddeco - Make manifests dir `bundle.sh` configurable
- PR #1065 - @stefanprodan - Add Go 1.16 to prerequisites (contributing doc)
- PR #1062 - @hiddeco - Improve build process embedded manifests
- PR #1060 - @relu - Install Bash, Fish, ZSH auto complete in AUR pkgs

