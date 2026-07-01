---
title: trivy v0.23 Release Notes
description: trivy v0.23 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- redis
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.23 Release Notes 是什么
- 如何 trivy v0.23 Release Notes
trigger_keywords:
- trivy
- v0.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
---



# [[Trivy|trivy]] v0.23 Release Notes

Source: [v0.23.0](https://github.com/aquasecurity/trivy/releases/tag/v0.23.0)

## Changelog

449add24 docs: add ACR navigator (#1651)
cb9afc84 fix: update example Rego files and docs (#1628)
78b2b899 feat(option): show a link to GitHub Discussions for --light deprecation (#1650)
52fd3c2e fix(sarif): fix the warning message (#1647)
8d5882be refactor: migrate to prefixed buckets (#1644)
84dd33f7 feat(mariner): add support for CBL-Mariner (#1640)
9e903a1d docs: commercial use available (#1641)
f4c746a2 feat: support azure acr (#1611)
420f8ab1 feat(os-pkg): add data sources (#1636)
d2827cba feat(redhat): support build info in RHEL (#807)
ce703ce4 fix: change links in pull_request_template to static URLs (#1634)
50bb938a feat(lang-pkg): add data sources (#1625)
a31ddbe9 feat(detector): support custom detector (#1615)
3a4e18ac docs(contribution): change role who should resolve comments (#1618)
8ba68361 docs: add PR template (#1602)
f5c55739 feat(rocky): support Rocky Linux (#1570)
eab2b425 Add the ability to set dockerhub credentials in the [[Helm|helm]] chart (#1569)
cabd18da feat(cache): redis TLS support (#1297)
02c3c365 feat(java): add support for PAR files (#1599)
4f7b7683 refactor(rust): move rust-advisory-db to OSV (#1591)
d754cb8c feat: log ignored vulnerabilities on debug (#1378)
a936e675 chore(mod): hcl2json deps update (#1585)
af116d3c fix(rpm): do not ignore installed files via third-party rpm (#1594)
b5073600 feat(fs): allow scanning a single file (#1578)
7fcbf44b refactor(python): drop Safety DB (#1580)
478d2799 feat: added insecure tls skip to scan git repo (#1528)
33bd41b4 Supress git clone output (#1590)
39a10089 fix(alma): skip modular package because MODULARITYLABEL is not set (#1588)
37abd612 feat(photon os): added EOL dates check (#1587)
78de33e8 docs: update supported os (#1586)
22054626 BREAKING: remove root command (#1579)
28ddcf1a docs: add Rust to Language-specific Packages Table (#1577)
df134c73 docs: update int doc for gitlab ci (#1575)
8da20c8c BREAKING: migrate the sarif template to Go code (#1437)
714b5ca2 refactor: remove unused field (#1567)
51e152b0 chore(deps): bump helm/chart-testing-action from 2.1.0 to 2.2.0 (#1554)
884daff4 docs: gitlab integration (#1381)
2a8336b9 feat(alma): support AlmaLinux (#1238)
1e171af1 docs: added note about default template path when Trivy installed using rpm (#1551)
e65274e0 BREAKING: Trivy DB from GHCR (#1539)
db35450b feat(cli): Do not set default commands when a plugin is being run (#1549)
24254d19 fix: add fingerprint field to codequality template (#1541)
2ee07456 fix(image): correct handling of uncompressed layers (#1544)
0aef82c5 chore: helm chart app version 0.22.0 (#1535)
8b2a7997 test(integration): use fixtures (#1532)


## Docker images

- `docker pull aquasec/trivy:0.23.0`
- `docker pull ghcr.io/aquasecurity/trivy:0.23.0`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.23.0`
