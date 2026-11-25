---
title: trivy v0.8 Release Notes
description: trivy v0.8 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- harbor
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.8 Release Notes 是什么
- 如何 trivy v0.8 Release Notes
trigger_keywords:
- trivy
- v0.8
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




# [[Trivy|trivy]] v0.8 Release Notes

Source: [v0.8.0](https://github.com/aquasecurity/trivy/releases/tag/v0.8.0)

## New Feature
### Add image subcommand (#493)
We deprecated `$ trivy IMAGE_NAME` and introduced `image` subcommand.

```
$ trivy image alpine:3.11
```

### Add CVSS Vectors to JSON output. (#484)
You can see CVSS vectors in a result JSON.

```
$ trivy image --format=json alpine=3.10.4
[...output snipped...]
        "VendorVectors": {
          "nvd": {
            "v2": "AV:N/AC:L/Au:N/C:N/I:N/A:P",
            "v3": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:H"
          },
          "redhat": {
            "v3": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:H"
          }
        },
[...output snipped...]
```

### Support registry token (#482)
To scan a private image, you can pass a registry token instead of ID/PW. This is useful when you develop a registry integration such as [[Harbor|Harbor]] and Quay.

```
# 🟢 低风险：只读/信息收集，通常无副作用
$ export TRIVY_REGISTRY_TOKEN=$(curl -u "username:password" "https://auth.docker.io/token?service=registry.docker.io&scope=repository:org/private_image:pull")
$ trivy org/private_image:latest
```
## Changelog

78b7529 Add image subcommand (#493)
e2bcb44 fix: remove help template (#500)
a57c27e vulnerability: Add CVSS Vectors to JSON output. (#484)
926f323 feat: support registry token (#482)
aa20adb chore: bump up urfave/cli to v2 (#499)
3e0779a chore(doc): update README (#490)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.8.0`
- `docker pull docker.io/aquasec/trivy:latest`


<!-- risk-assessed -->
