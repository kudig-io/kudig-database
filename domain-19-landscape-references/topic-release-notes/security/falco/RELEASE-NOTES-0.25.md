---
title: falco v0.25 Release Notes
description: falco v0.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.25 Release Notes 是什么
- 如何 falco v0.25 Release Notes
trigger_keywords:
- falco
- v0.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# falco v0.25 Release Notes

Source: [0.25.0](https://github.com/falcosecurity/falco/releases/tag/0.25.0)


Released on 2020-08-25

### Major Changes

* new(userspace/falco): print the Falco and driver versions at the very beginning of the output. [[#1303](https://github.com/falcosecurity/falco/pull/1303)] - [@leogr](https://github.com/leogr)
* new: libyaml is now bundled in the release process. Users can now avoid installing libyaml directly when getting Falco from the official release. [[#1252](https://github.com/falcosecurity/falco/pull/1252)] - [@fntlnz](https://github.com/fntlnz)


### Minor Changes

* docs(test): step-by-step instructions to run integration tests locally [[#1313](https://github.com/falcosecurity/falco/pull/1313)] - [@leodido](https://github.com/leodido)
* update: renameat2 syscall support [[#1355](https://github.com/falcosecurity/falco/pull/1355)] - [@fntlnz](https://github.com/fntlnz)
* update: support for 5.8.x kernels [[#1355](https://github.com/falcosecurity/falco/pull/1355)] - [@fntlnz](https://github.com/fntlnz)


### Bug Fixes

* fix(userspace/falco): correct the fallback mechanism for loading the kernel module [[#1366](https://github.com/falcosecurity/falco/pull/1366)] - [@leogr](https://github.com/leogr)
* fix(falco-driver-loader): script crashing when using arguments [[#1330](https://github.com/falcosecurity/falco/pull/1330)] - [@antoinedeschenes](https://github.com/antoinedeschenes)


### Rule Changes

* rule(macro user_trusted_containers): add `sysdig/node-image-analyzer` and `sysdig/agent-slim` [[#1321](https://github.com/falcosecurity/falco/pull/1321)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro falco_privileged_images): add `docker.io/falcosecurity/falco` [[#1326](https://github.com/falcosecurity/falco/pull/1326)] - [@nvanheuverzwijn](https://github.com/nvanheuverzwijn)
* rule(EphemeralContainers Created): add new rule to detect ephemeral container created [[#1339](https://github.com/falcosecurity/falco/pull/1339)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro user_read_sensitive_file_containers): replace endswiths with exact image repo name [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro user_trusted_containers): replace endswiths with exact image repo name [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro user_privileged_containers): replace endswiths with exact image repo name [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro trusted_images_query_miner_domain_dns): replace endswiths with exact image repo name [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro falco_privileged_containers): append "/" to quay.io/sysdig [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(list falco_privileged_images): add images docker.io/sysdig/agent-slim and docker.io/sysdig/node-image-analyzer [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(list falco_sensitive_mount_images): add image docker.io/sysdig/agent-slim [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(list k8s_containers): prepend docker.io to images [[#1349](https://github.com/falcosecurity/falco/pull/1349)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(macro exe_running_docker_save): add better support for centos [[#1350](https://github.com/falcosecurity/falco/pull/1350)] - [@admiral0](https://github.com/admiral0)
* rule(macro rename): add `renameat2` syscall [[#1359](https://github.com/falcosecurity/falco/pull/1359)] - [@leogr](https://github.com/leogr)
* rule(Read sensitive file untrusted): add trusted images into whitelist [[#1327](https://github.com/falcosecurity/falco/pull/1327)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(Pod Created in Kube Namespace): add new list k8s_image_list as white list [[#1336](https://github.com/falcosecurity/falco/pull/1336)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(list allowed_k8s_users): add "kubernetes-admin" user [[#1323](https://github.com/falcosecurity/falco/pull/1323)] - [@leogr](https://github.com/leogr)

### Statistics

| Merged PRs        | Number  |
|-------------------|---------|
| Not user-facing   | 5       |
| Release note      | 15      |
| Total             | 20      |
