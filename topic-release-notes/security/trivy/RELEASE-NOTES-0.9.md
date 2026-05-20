---
title: trivy v0.9 Release Notes
description: trivy v0.9 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.9 Release Notes 是什么
- 如何 trivy v0.9 Release Notes
trigger_keywords:
- trivy
- v0.9
- Release
- Notes
- release
- notes
---

# trivy v0.9 Release Notes

Source: [v0.9.2](https://github.com/aquasecurity/trivy/releases/tag/v0.9.2)

## New Features
### Support JUnit XML (#541)
You can see the result on the dashboard if your CI service supports JUnit XML. This is an example of CircleCI.

![image](https://user-images.githubusercontent.com/2253692/87301313-01e6a080-c518-11ea-84d7-f1cda620a470.png)

Azure DevOps (Thank you, @lgulliver)

![image](https://user-images.githubusercontent.com/2253692/87301771-d1ebcd00-c518-11ea-972b-b5f8bd512dfe.png)

This is implemented by @rahul2393.

### Include CVSS score info in a result (#530)

```
      {
        "VulnerabilityID": "CVE-2019-1547",
        "PkgName": "openssl",
        "InstalledVersion": "1.1.1c-r0",
        "FixedVersion": "1.1.1d-r0",
        "CVSS": {
          "nvd": {
            "V2Vector": "AV:L/AC:M/Au:N/C:P/I:N/A:N",
            "V3Vector": "CVSS:3.1/AV:L/AC:H/PR:L/UI:N/S:U/C:H/I:N/A:N",
            "V2Score": 1.9,
            "V3Score": 4.7
          },
          "redhat": {
            "V3Vector": "CVSS:3.0/AV:L/AC:L/PR:L/UI:N/S:U/C:H/I:N/A:N",
            "V3Score": 5.5
          }
        },
        ...
      }
```

## Bug fixes
- fix(writer): `Error retrieving template from path` when --format is not template but template is provided (#556)
- fix(log): write error messages to stderr (#538)
- fix(alpine): replace go-deb-version with go-apk-version (#520)
- fix: MissingBlobs is implemented different in FS and S3 the method log… (#522)

## Changelog

d9fa353 Fixing `Error retrieving template from path` when --format is not template but template is provided (#556)
9a1d746 Adding contrib/junit.tpl to docker image (#554)
d18d17b db: Update trivy-db to include CVSS score info (#530)
4b57c0d docs: fix markdown (#553)
ccd9b2d Added function to escape string in failure message title and descriptions (#551)
ec770cd Added JUNIT support (#541)
b7ec633 chore(docs): mention air-gapped environment (#544)
7aabff1 chore(README): add programming languages (#543)
9dc1bdf fix(log): write error messages to stderr (#538)
2ac672a Use StoreMetadata from trivy-db (#509)
11ae6b2 docs: add more CI options to README (#535)
f201f59 chore(Dockerfile): bump up alpine to 3.12 (#528)
25d45e1 fix(alpine): replace go-deb-version with go-apk-version (#520)
298ba99 fix: MissingBlobs is implemented different in FS and S3 the method log… (#522)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.9.2`
- `docker pull docker.io/aquasec/trivy:latest`
