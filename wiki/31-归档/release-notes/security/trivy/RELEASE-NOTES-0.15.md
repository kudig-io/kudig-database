---
title: trivy v0.15 Release Notes
description: trivy v0.15 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.15 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.15 Release Notes 是什么
- 如何 trivy v0.15 Release Notes
trigger_keywords:
- trivy
- v0.15
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Trivy|trivy]] v0.15 Release Notes

Source: [v0.15.0](https://github.com/aquasecurity/trivy/releases/tag/v0.15.0)

## Features
### NuGet Scanner (#686)
Trivy now supports a lock file `packages.lock.json` of NuGet.

```
packages.lock.json
==================
Total: 1 (UNKNOWN: 0, LOW: 0, MEDIUM: 1, HIGH: 0, CRITICAL: 0)

+-------------+------------------+----------+-------------------+----------------+--------------------------------------+
|   LIBRARY   | VULNERABILITY ID | SEVERITY | INSTALLED VERSION | FIXED VERSION  |                TITLE                 |
+-------------+------------------+----------+-------------------+----------------+--------------------------------------+
| MessagePack | CVE-2020-5234    | MEDIUM   | 1.9.10            | 2.1.90, 1.9.11 | Untrusted data can lead to DoS       |
|             |                  |          |                   |                | attack due to hash collisions and... |
|             |                  |          |                   |                | -->avd.aquasec.com/nvd/cve-2020-5234 |
+-------------+------------------+----------+-------------------+----------------+--------------------------------------+
```

Thanks to @Johannestegner

### Redis support as the cache backend (#770)
For the detail, see [here](https://github.com/aquasecurity/trivy#specify-cache-backend)
```
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker run -d --name redis -p 6379:6379 redis:5.0
$ trivy server --cache-backend redis://localhost:6379
```
```
$ trivy client alpine:3.11
```

### HTML template (#567)

```
$ trivy image -f template --template "@contrib/html.tpl" -o report.html alpine:3.12 
```

Thanks to @irrandon

### [[Helm|Helm]] chart (#751, #769)
For the detail, see [here](https://github.com/aquasecurity/trivy/tree/main/helm/trivy)

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
$ cd helm/trivy
$ helm install my-release .
```
Thanks to @czunker

## Fixes
### redhat: skip modular packages (#776)
Close https://github.com/aquasecurity/trivy/issues/771 and https://github.com/aquasecurity/trivy/issues/741

Thanks to @masahiro331 

### Make the table output less wide. (#763)

```
alpine:3.10 (alpine 3.10.5)
===========================
Total: 4 (UNKNOWN: 0, LOW: 0, MEDIUM: 4, HIGH: 0, CRITICAL: 0)

+--------------+------------------+----------+-------------------+---------------+---------------------------------------+
|   LIBRARY    | VULNERABILITY ID | SEVERITY | INSTALLED VERSION | FIXED VERSION |                 TITLE                 |
+--------------+------------------+----------+-------------------+---------------+---------------------------------------+
| libcrypto1.1 | CVE-2020-1971    | MEDIUM   | 1.1.1g-r0         | 1.1.1i-r0     | openssl: EDIPARTYNAME                 |
|              |                  |          |                   |               | NULL pointer de-reference             |
|              |                  |          |                   |               | -->avd.aquasec.com/nvd/cve-2020-1971  |
+--------------+                  +          +                   +               +                                       +
| libssl1.1    |                  |          |                   |               |                                       |
|              |                  |          |                   |               |                                       |
|              |                  |          |                   |               |                                       |
+--------------+------------------+          +-------------------+---------------+---------------------------------------+
| musl         | CVE-2020-28928   |          | 1.1.22-r3         | 1.1.22-r4     | In musl libc through 1.2.1,           |
|              |                  |          |                   |               | wcsnrtombs mishandles particular      |
|              |                  |          |                   |               | combinations of destination buffer... |
|              |                  |          |                   |               | -->avd.aquasec.com/nvd/cve-2020-28928 |
+--------------+                  +          +                   +               +                                       +
| musl-utils   |                  |          |                   |               |                                       |
|              |                  |          |                   |               |                                       |
|              |                  |          |                   |               |                                       |
|              |                  |          |                   |               |                                       |
+--------------+------------------+----------+-------------------+---------------+---------------------------------------+
```


## Changelog

08ca1b0 Feat: NuGet Scanner (#686)
7b86f81 feat(cache): support Redis (#770)
8cd4afe fix(redhat): skip module packages (#776)
b606b62 chore: migrate from master to main (#778)
5c2b14b chore(circleci): remove gofmt (#777)
a19a023 chore(README): remove experimental (#775)
e6cef75 NVD: Add timestamps. (#761)
1371f72 (fix): Make the table output less wide. (#763)
8ecaa2f Add gitHubToken to prevent rate limit problems (#769)
8132174 Add helm chart to install trivy in server mode. (#751)
bcc2850 chore(docs): add nix install (#762)
cb36972 HTML template (#567)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.15.0`
- `docker pull docker.io/aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:0.15.0`
- `docker pull ghcr.io/aquasecurity/trivy:latest`


<!-- risk-assessed -->
