---
title: trivy v0.12 Release Notes
description: trivy v0.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.12 Release Notes 是什么
- 如何 trivy v0.12 Release Notes
trigger_keywords:
- trivy
- v0.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- logging-basics
created: "2026-05-23"
---

# [[Trivy|trivy]] v0.12 Release Notes

Source: [v0.12.0](https://github.com/aquasecurity/trivy/releases/tag/v0.12.0)

## New features
###  Add --skip-files option (#624)
Trivy traversals directories and looks for all lock files by default. If your image contains lock files which are not maintained by you, you can skip the file.

```
$ trivy image --skip-files "/Gemfile.lock,/app/Pipfile.lock" quay.io/fluentd_elasticsearch/fluentd:v2.9.0
```

### Add health check endpoint to trivy server (#644)

```
$ trivy server &
$ curl http://127.0.0.1:4954/healthz 
ok
```

### Add --skip-update option to fs and repo subcommand (#641)

```
$ trivy fs -h | grep skip-update
   --skip-update               skip db update (default: false) [$TRIVY_SKIP_UPDATE]
```

### Publish the official image in GitHub Container Registry (#627)

```
$ docker pull ghcr.io/aquasecurity/trivy:latest
```

### Add CWE-ID (#614)
Trivy server responds `CWE-ID` in a scan result.

## Fixes
### Show help for subcommands (#628, #629)

```
$ trivy image
NAME:
   trivy image - scan an image

USAGE:
   trivy image [command options] image_name

OPTIONS:
   --template value, -t value  output template [$TRIVY_TEMPLATE]
   --format value, -f value    format (table, json, template) (default: "table") [$TRIVY_FORMAT]
   --input value, -i value     input file path instead of image name [$TRIVY_INPUT]
...
```


## Changelog

49691ba ci(circle): update remote docker version (#683)
87ff0c1 suse: update end of life dates for SLES [[Service|service]] packs (#676)
de30c3f update readme for parallel run issue (#660)
4c3bfb8 fix link for Clear images section in README (#659)
8b21cfe add link to Gitlab CI pipeline in README (#658)
46700f7 test: add tests for mux (#645)
014be7e chore: bump up Go to 1.15 (#646)
b3ff2c3 Add contrib/ to the release chain for Docker (#638)
9c786de Add health check endpoint to trivy server (#644)
188e108 fix(cli): show help for subcommands (#629)
7d7842f Add --skip-update option to fs and repo subcommand (#641)
901a371 goreleaser.yml: Add all templates to archive (#636)
095b5ce fix(cli): show help when no argument is passed (#628)
1d3f70e chore(image): push the official image to GitHub Container Registry as well (#627)
5e308da feat(cli): add --skip-files option (#624)
2231e40 chore(docs): update comparison table (#623)
b3680f0 logo: Add new Trivy logo (#615)
8952779 fix(Readme) - Results using a template (#622)
165d593 Improve Gitlab CI installation step in README (#621)
d8b0962 feat(rpc): add CWE-ID (#614)
d35e8ec Add all templates to the docker image (#619)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.12.0`
- `docker pull docker.io/aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:0.12.0`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
