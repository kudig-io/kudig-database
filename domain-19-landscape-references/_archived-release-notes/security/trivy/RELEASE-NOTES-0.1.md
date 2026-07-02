---
title: trivy v0.1 Release Notes
description: trivy v0.1 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.1 Release Notes 是什么
- 如何 trivy v0.1 Release Notes
trigger_keywords:
- trivy
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Trivy|trivy]] v0.1 Release Notes

Source: [v0.1.7](https://github.com/aquasecurity/trivy/releases/tag/v0.1.7)

## New feature
- Support new OSes
  - Amazon Linux
  - Google Distroless
- Support new build tool
  - Kaniko
- New options
  - `--ignorefile`
    - Specify the .trivyignore path
  - `--timeout`
    - Specify timeout
  - `--template`
    - The result can be exported to your template

## Update
- Go version
  - 1.13
- Alpine version
  - 3.10
## Changelog

d03a64c Update README (#224)
20babc4 Bump Go 1.13 (#218)
a6141ed CI/CD refactor (#209)
a12bb8d fix(db): introduce db schema version (#221)
5ae10e0 Dockerfile: Update runner base to alpine 3.10 (#199)
ff873a2 Support Amazon Linux (#182)
7ad94c3 Update .gitignore (#215)
f850984 test(integration): add integration tests (#201)
9334e60 Changed to be able to specify IgnoreFile as whitelist (#175)
f198b6e Check errors passed through by filepath.Walk (#208)
cb1870e Update README.md (#206)
384205a Remove extra double quote (#204)
d9e64d2 Updated README.md (#203)
5ccb0af Added Docker image badge & missing punctuation's (#189)
da621c3 Add timeout option (#143)
3a28576 added reference for LICENSE (#195)
dbb7a55 Check returned error before deferring file close (#197)
89f2d48 docs: minor tweak (#183)
f933ab4 Improve ubuntu install (#178)
af78d2f Update README.md - typo fix (#186)
0fff415 Support Kaniko (#171)
987538f Display an error message when rpm not found (#167)
2642020 Support distroless and ignore lock files under vendor dir (#166)
c4a2b76 Add rpm to the trivy image (#165)
339d0db Add template writer (#141)
43568cc Update xerrors version (#158)
fbd73f2 Modify cache-dir usage comment (#148)
4a21ad9 env (#154)
18de7e4 README.md is out of date (#145)
90e4c15 Add the RHEL8 support to rpm repository (#138)
4f57216 use COPY on dockerfile rather than add (#132)
e6b6830 fix typo in readme (#130)
4ce651c fix gofmt (#131)

## Docker images

- `docker pull docker.io/aquasec/trivy:0.1.7`
- `docker pull docker.io/aquasec/trivy:latest`


<!-- risk-assessed -->
