---
title: trivy v0.26 Release Notes
description: trivy v0.26 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.26 Release Notes 是什么
- 如何 trivy v0.26 Release Notes
trigger_keywords:
- trivy
- v0.26
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Trivy|trivy]] v0.26 Release Notes

Source: [v0.26.0](https://github.com/aquasecurity/trivy/releases/tag/v0.26.0)

## Changelog
* a0047a79 feat(alpine): warn mixing versions (#2000)
* d786655a Update ASFF template (#1914)
* a02cf651 chore(deps): replace `[[containerd|containerd]]/containerd` version to fix CVE-2022-23648 (#1994)
* 613e38cc chore(deps): bump alpine from 3.15.3 to 3.15.4 (#1993)
* 3b6d65be test(go): add integration tests for gomod (#1989)
* 22f5b938 fix(python): fixed panic when scan .egg archive (#1992)
* 485637c2 fix(go): set correct go modules type (#1990)
* 6fdb554a feat(alpine): support apk repositories (#1987)
* d9bddb90 docs: add CBL-Mariner (#1982)
* 1cf1873f docs(go): fix version (#1986)
* d77dbe8a feat(go): support go.mod in Go 1.17+ (#1985)
* 32bd1e48 ci: fix URLs in the PR template (#1972)
* 94a5a180 ci: add semantic pull requests check (#1968)
* 72d94b21 docs(issue): added docs for wrong detection issues (#1961)

