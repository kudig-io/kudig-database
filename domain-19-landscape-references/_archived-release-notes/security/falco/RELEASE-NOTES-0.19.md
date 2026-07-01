---
title: falco v0.19 Release Notes
description: falco v0.19 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
- docker
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.19 Release Notes 是什么
- 如何 falco v0.19 Release Notes
trigger_keywords:
- falco
- v0.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- logging-basics
---



# [[Falco|falco]] v0.19 Release Notes

Source: [0.19.0](https://github.com/falcosecurity/falco/releases/tag/0.19.0)

Released on 2020-01-23

### Major Changes

* new: security audit [[#977](https://github.com/falcosecurity/falco/pull/977)]
* instead of crashing, now falco will report the error when an internal error occurs while handling an event to be inspected. the log line will be of type error and will contain the string `error handling inspector event` [[#746](https://github.com/falcosecurity/falco/pull/746)]
* build: bump [[gRPC|grpc]] to 1.25.0 [[#939](https://github.com/falcosecurity/falco/pull/939)]
* build: (most of) dependencies are bundled dynamically (by default) [[#968](https://github.com/falcosecurity/falco/pull/968)]
* test: integration tests now can run on different distributions via docker containers, for now CentOS 7 and Ubuntu 18.04 with respective rpm and deb packages [[#1012](https://github.com/falcosecurity/falco/pull/1012)]

### Minor Changes

* proposal: rules naming convention [[#980](https://github.com/falcosecurity/falco/pull/980)]
* update: also allow posting json arrays containing k8s audit events to the k8s_audit endpoint. [[#967](https://github.com/falcosecurity/falco/pull/967)]
* update: add support for k8s audit events to the falco-event-generator container. [[#997](https://github.com/falcosecurity/falco/pull/997)]
* update: falco-tester base image is fedora:31 now [[#968](https://github.com/falcosecurity/falco/pull/968)]
* build: switch to circleci [[#968](https://github.com/falcosecurity/falco/pull/968)]
* build: bundle openssl into falco-builder docker image [[#1004](https://github.com/falcosecurity/falco/pull/1004)]
* build: falco-builder docker image revamp (centos:7 base image) [[#1004](https://github.com/falcosecurity/falco/pull/1004)]
* update: puppet module had been renamed from "sysdig-falco" to "falco" [[#922](https://github.com/falcosecurity/falco/pull/922)]
* update: adds a hostname field to grpc output [[#927](https://github.com/falcosecurity/falco/pull/927)]
* build: download grpc from their github repo [[#933](https://github.com/falcosecurity/falco/pull/933)]
* update: ef_drop_falco is now ef_drop_simple_cons [[#922](https://github.com/falcosecurity/falco/pull/922)]
* update(docker): use host_root environment variable rather than sysdig_host_root [[#922](https://github.com/falcosecurity/falco/pull/922)]
* update: ef_drop_falco is now ef_drop_simple_cons [[#922](https://github.com/falcosecurity/falco/pull/922)]

### Bug Fixes

* fix: providing clang into docker-builder [[#972](https://github.com/falcosecurity/falco/pull/972)]
* fix: prevent throwing json type error c++ exceptions outside of the falco engine when procesing k8s audit events. [[#928](https://github.com/falcosecurity/falco/pull/928)]
* fix(docker/kernel/linuxkit): correct from for falco minimal image [[#913](https://github.com/falcosecurity/falco/pull/913)]

### Rule Changes

* rules(list network_tool_binaries): add some network tools to detect suspicious network activity. [[#973](https://github.com/falcosecurity/falco/pull/973)]
* rules(write below etc): allow automount to write to /etc/mtab [[#957](https://github.com/falcosecurity/falco/pull/957)]
* rules(macro user_known_k8s_client_container): when executing the docker client, exclude fluentd-gcp-scaler container running in the `kube-system` namespace to avoid false positives [[#962](https://github.com/falcosecurity/falco/pull/962)]
* rules(the docker client is executed in a container): detect the execution of the docker client in a container and logs it with warning priority.  [[#915](https://github.com/falcosecurity/falco/pull/915)]
* rules(list k8s_client_binaries): create and add docker, kubectl, crictl [[#915](https://github.com/falcosecurity/falco/pull/915)]
* rules(macro container_entrypoint): add docker-runc-cur  [[#914](https://github.com/falcosecurity/falco/pull/914)]
* rules(list user_known_chmod_applications): add hyperkube [[#914](https://github.com/falcosecurity/falco/pull/914)]
* rules(list network_tool_binaries): add some network tools to detect suspicious network activity. [[#975](https://github.com/falcosecurity/falco/pull/975)]
* rules(macro user_known_k8s_client_container): macro to match kube-system namespace [[#955](https://github.com/falcosecurity/falco/pull/955)]
* rules(contact k8s api server from container): now it can automatically resolve the cluster ip address  [[#952](https://github.com/falcosecurity/falco/pull/952)]
* rules(macro k8s_api_server): new macro to match the default k8s api server [[#952](https://github.com/falcosecurity/falco/pull/952)]
* rules(macro sensitive_vol_mount): add more sensitive host paths [[#929](https://github.com/falcosecurity/falco/pull/929)]
* rules(macro sensitive_mount): add more sensitive paths [[#929](https://github.com/falcosecurity/falco/pull/929)]
* rules(macro consider_metadata_access): macro to decide whether to consider metadata or not (off by default) [[#943](https://github.com/falcosecurity/falco/pull/943)]
* rules(contact cloud metadata service from container): add rules to detect access to gce instance metadata [[#943](https://github.com/falcosecurity/falco/pull/943)]
* rules(macro sensitive_vol_mount): align sensitive mounts macro between k8s audit rules and syscall rules [[#950](https://github.com/falcosecurity/falco/pull/950)]
* rules(macro consider_packet_socket_communication): macro to consider or not packet socket communication (off by default) [[#945](https://github.com/falcosecurity/falco/pull/945)]
* rules(packet socket created in container): rule to detect raw packets creation [[#945](https://github.com/falcosecurity/falco/pull/945)]
* rules(macro exe_running_docker_save): fixed false positives in multiple rules that were caused by the use of docker in docker [[#951](https://github.com/falcosecurity/falco/pull/951)]
* rules(modify shell configuration file): fixed a false positive by excluding "exe_running_docker_save" [[#949](https://github.com/falcosecurity/falco/pull/949)]
* rules(update package repository): fixed a false positive by excluding "exe_running_docker_save". [[#948](https://github.com/falcosecurity/falco/pull/948)]
* rules(the docker client is executed in a container): when executing the docker client, exclude containers running in the `kube-system` namespace to avoid false positives [[#955](https://github.com/falcosecurity/falco/pull/955)]
* rules(list user_known_chmod_applications): add kubelet [[#944](https://github.com/falcosecurity/falco/pull/944)]
* rules(set setuid or setgid bit): fixed a false positive by excluding "exe_running_docker_save" [[#946](https://github.com/falcosecurity/falco/pull/946)]
* rules(macro user_known_package_manager_in_container): allow users to specify conditions that match a legitimate use case for using a package management process in a container. [[#941](https://github.com/falcosecurity/falco/pull/941)]

### Statistics

| Merged PRs          | Number |
|-------------------|---------|
| Not user-facing    | 12 |
| Release note        | 32 |
| Total                      | 44 |