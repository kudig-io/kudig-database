---
title: trivy v0.13 Release Notes
description: trivy v0.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.13 Release Notes 是什么
- 如何 trivy v0.13 Release Notes
trigger_keywords:
- trivy
- v0.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Trivy|trivy]] v0.13 Release Notes

Source: [v0.13.0](https://github.com/aquasecurity/trivy/releases/tag/v0.13.0)

## Important change
###  Support npm and RubyGems versioning semantics (#740)
npm and RubyGems have different versioning/constraint semantics from other languages, so we developed libraries for them.  In the future, we will probably develop libraries for other languages such as Python.

## New features
### Skip downloading DB if a remote DB is not updated (#717)
Once the vulnerability DB is downloaded, it will not be updated within one hour so that Trivy will not download the same DB many times by mistake.

## Support
### Add back support for FreeBSD & OpenBSD (#728)
Provide binaries for FreeBSD & OpenBSD

### Add support for ppc64le architecture (#724)
Provide binaries for the ppc64le (Power) architecture.

## Bug fixes
### Handle ksplice advisories of Oracle Linux(#745)
Skip ksplice advisories when the installed package is not a ksplice package during Oracle Linux scanning. Also, if the package is a ksplice one, we should not use the normal advisories.

### Skip packages from unsupported repository (remi) (#695)
Skip scanning RPM packages installed from the remi repository

## Changelog

1391b3b fix(oracle): handle ksplice advisories (#745)
b6d5b82 fix: version comparison (#740)
9dfb0fe updated Readme.md (#737)
4555469 Add suse sles 15.2 to the EOL list as well (#734)
c189aa6 Update README.md (#731)
8442528 Warn when a user attempts to use trivy without a detectable lockfile (#729)
d09787e Add back support for FreeBSD & OpenBSD (#728)
0285a89 Add support for ppc64le architecture (#724)
7d7784f Skip packages from unsupported repository (remi) (#695)
ca6f196 Skip downloading DB if a remote DB is not updated (#717)
e621cf2 Sunsetting VendorVectors (#718)
906ab54 Add GitHub Container Registry to README (#712)
1549c25 update BUG_REPORT.md using H2 instead of bold formatting (#714)
fe1d07e fix(ci/deb): do not remove old packages for EOL versions (#706)
793a1aa Add linter check support (#679)
4a94477 Optimize images (#696)
9bc2b19 Update triage.md (#701)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.13.0`
- `docker pull docker.io/aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:0.13.0`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
