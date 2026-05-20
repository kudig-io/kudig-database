---
title: trivy v0.16 Release Notes
description: trivy v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- redis
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.16 Release Notes 是什么
- 如何 trivy v0.16 Release Notes
trigger_keywords:
- trivy
- v0.16
- Release
- Notes
- release
- notes
---

# trivy v0.16 Release Notes

Source: [v0.16.0](https://github.com/aquasecurity/trivy/releases/tag/v0.16.0)

## Features
### Support Podman (#825)

[EXPERIMENTAL] This feature might change without preserving backwards compatibility.

Scan your image in Podman (>=2.0) running locally. The remote Podman is not supported. Before performing Trivy commands, you must enable the podman.sock systemd service on your machine. For more details, see here

```
$ systemctl --user enable --now podman.socket
```

Then, you can scan your image in Podman.

```
$ cat Dockerfile
FROM alpine:3.12
RUN apk add --no-cache bash
$ podman build -t test .
$ podman images
REPOSITORY                TAG     IMAGE ID      CREATED      SIZE
localhost/test            latest  efc372d4e0de  About a minute ago  7.94 MB
$ trivy image test
```

### Support modular packages in RHEL 8/CentOS 8 (#790)
Trivy is able to scan RHEL 8/CentOS 8 more accurately now.

### Add redis cache backend configuration options in the Helm chart (#784)
Trivy can be deployed to Kubernetes with Redis cache.

Thanks, @czunker!

### Support PEP 440 (#816)
Trivy is able to scan Python vulnerabilities more accurately now.

### Support alpine 3.13 (#819)
Trivy is able to scan Alpine Linux 3.13 now.

## Fixes
- Fix compatibility for Jenkins xunit plugin (#820)
- Update EOL dates (#824)
- Parse redis backend url (#804)
- Fix errors in SARIF format (#801)
- Fix env variable for github token (#796)
- Set unknown severity for empty values (#793)
- Remove global flags from filesystem command (#772)
- Fix formatting of log message (#785)

## Changelog

cdabe7f Fix compatibility for Jenkins xunit plugin (#820)
b0fe439 README: add Gitlab job that uses a container with trivy (#823)
6685cd4 feat: support Podman (#825)
7a683bd fix(eol): update EOL dates (#824)
6ed03a8 fix(python): follow PEP 440 (#816)
182cb80 Support alpine 3.13 (#819)
2acd1ca Changed the output string to "Using your github token". (#814)
dd35bfd Align comment with code (#812)
1f17e71 Parse redis backend url (#804)
0954f6b Update README.md (#810)
6b29bf1 Added nodeSelector, affinity and tolerations to helm chart (#803)
f6afdf0 Fix readme typo in policy flag (#805)
412847d Fix errors in SARIF format (#801)
5b27862 Fix env variable for github token (#796)
6ed25c1 fix(vulnerability): set unknown severity for empty values (#793)
e2c483f Remove global flags from filesystem command (#772)
5c5e0cb Add imagePullSecrets to helm Chart (#789)
b9b84cd Add redis cache backend configuration options (#784)
e517bcc Update README.md (#735)
7f5a6d4 feat(redhat): support modular packages (#790)
8de09dd Fix formatting of log message (#785)
e08ae8d chore(ci): migrate unit tests to GitHub Actions (#779)
a00d719 shifted: brews.github to brews.tap (#780)

## Docker images

- `docker pull docker.io/aquasec/trivy:0.16.0`
- `docker pull docker.io/aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:0.16.0`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
