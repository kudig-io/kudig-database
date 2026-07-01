---
title: trivy v0.14 Release Notes
description: trivy v0.14 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.14 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.14 Release Notes 是什么
- 如何 trivy v0.14 Release Notes
trigger_keywords:
- trivy
- v0.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Trivy|trivy]] v0.14 Release Notes

Source: [v0.14.0](https://github.com/aquasecurity/trivy/releases/tag/v0.14.0)

## Features
### Add primary URLs (#752)
Trivy shows a primary URL in the result as follows.

```
alpine:3.10 (alpine 3.10.5)
===========================
Total: 2 (UNKNOWN: 2, LOW: 0, MEDIUM: 0, HIGH: 0, CRITICAL: 0)

+------------+------------------+----------+-------------------+---------------+--------------------------------+------------------------------------+
|  LIBRARY   | VULNERABILITY ID | SEVERITY | INSTALLED VERSION | FIXED VERSION |             TITLE              |                URL                 |
+------------+------------------+----------+-------------------+---------------+--------------------------------+------------------------------------+
| musl       | CVE-2020-28928   | UNKNOWN  | 1.1.22-r3         | 1.1.22-r4     | In musl libc through 1.2.1,    | avd.aquasec.com/nvd/cve-2020-28928 |
|            |                  |          |                   |               | wcsnrtombs mishandles          |                                    |
|            |                  |          |                   |               | particular combinations of     |                                    |
|            |                  |          |                   |               | destination buffer...          |                                    |
+------------+                  +          +                   +               +                                +                                    +
| musl-utils |                  |          |                   |               |                                |                                    |
|            |                  |          |                   |               |                                |                                    |
|            |                  |          |                   |               |                                |                                    |
|            |                  |          |                   |               |                                |                                    |
+------------+------------------+----------+-------------------+---------------+--------------------------------+------------------------------------+
```

```
[
  {
    "Target": "alpine:3.10 (alpine 3.10.5)",
    "Type": "alpine",
    "Vulnerabilities": [
      {
        "VulnerabilityID": "CVE-2020-28928",
        "PrimaryURL": "https://avd.aquasec.com/nvd/cve-2020-28928",
        ...
      }
    ]
  }
]
```

In these cases, you can see `https://avd.aquasec.com/nvd/cve-2020-28928` as a primary URL.

### Remove rpm dependency (#753)
Trivy no longer requires the `rpm` command on the host. You can scan a RHEL-based image without rpm.

```
$ rpm
bash: rpm: command not found
$ trivy image -o /dev/null centos:7

centos:7 (centos 7.9.2009)
==========================
Total: 601 (UNKNOWN: 0, LOW: 358, MEDIUM: 240, HIGH: 3, CRITICAL: 0)
```

## Bug fixes
### --light shows less results (#755)
There was a bug where vulnerabilities with unknown severity do not appear in the result when using the `--light` option.

## Changelog

9bdbeab feat: remove rpm dependency (#753)
d85cb77 fix(vulnerability): make an empty severity UNKNOWN (#759)
1bee83c chore(README): add TRIVY_INSECURE (#760)
4d18943 feat(vulnerability): add primary URLs (#752)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.14.0`
- `docker pull docker.io/aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:0.14.0`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
