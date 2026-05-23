---
title: cri-o v1.17 Release Notes
description: cri-o v1.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v1.17 Release Notes 是什么
- 如何 cri-o v1.17 Release Notes
trigger_keywords:
- cri-o
- v1.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cri-o v1.17 Release Notes

Source: [v1.17.5](https://github.com/cri-o/cri-o/releases/tag/v1.17.5)

CRI-O 1.17.5

Welcome to the v1.17.5 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/cri-o/cri-o/issues.

### Contributors

* Peter Hunt
* Fabiano Fidêncio
* Kir Kolyshkin
* Sascha Grunert
* Mrunal Patel
* Ryan Phillips
* Lokesh Mandvekar
* Michael Cambria
* Urvashi Mohnani
* Daniel J Walsh
* Snir Sheriber
* Ted Yu

### Changes

* dd507bd68 Revert "runtime_vm: Cleanup process when the Container is Stopped"
* d89c14b92 Switch to updated version of creack/pty
* 926517f37 Bump to v1.17.5
* f264ce30b runtime_vm: Store logs in the correct format
* 1e5cec409 Revert "Fix potentially unclosed file in runtimeVM#CreateContainer"
* e9a87953e runtime_vm: set container creation time
* 67dcd05de runtime_vm: Cleanup process when the Container is Stopped
* 0957c451b runtime_vm: Avoid possible deadlock on UpdateContainerStatus()
* 570558727 runtime_vm: Improve CreateContainer cleanup in case of failures
* cc3ec8086 runtime_vm: Create deleteContainer() helper
* 35d47cd69 internal/oci/runtime_vm: lock around map access
* 04dd575a1 internal/oci/runtime_vm: fix resizePty signature
* 6aaea4094 runtime_vm: Ignore ttrpc.ErrClosed when shutting the container down
* 9374b7c2d Fix naming test panic
* 61a236b49 port error: check for error
* 55e020dce Copy spec to not touch original spec on exec(sync)
* a13886c9c more retErr fixes
* 26d57a42f change variable name err to retErr for deferred comparisons
* 78009728b Fix potentially unclosed file in runtimeVM#CreateContainer
* bf58f2916 oci: check state before stop atomically
* 71165c783 log: drop exec'd infofs
* 565e4e6c3 Add info logs for image pull and status CRI calls
* efa674409 bump c/image to 5.4.4
* 1f07c6d1b test: refactor some of main.yml
* 7a160c89b bump version of libpod to get selinux
* c693567d0 Automatically label containers running systemd with the correct label
* f56528144 Use bats v1.2.0 release for CI
* 79ad06023 Makefile: include -nobuild install targets
* 9a2d22334 removes the one minute network stop timeout
* d1252747c Close progress goroutine after completion
* 8573e7ba7 Ensure we get the correct centos vault URL
* 9cf05b566 Add info logs where needed
* 56b047272 test: check for rw mounts
* 6cb67db3f cni ctx: call cancel func
* 636cd24f0 give fraction of timeout to network{start,stop} calls
* 40fd3a27d Pass context from caller to ocicni
* 52c072aea Update ocicni vendor code to get new methods that support context argument
* 5763da125 crio wipe: add version-file-persist
* 694daca8f test: update image digest to fix test
* 95b65e07f systemd unit: drop requirement of crio wipe
* fe41ba220 test/pod: TerminationGracePeriod passthru test
* 96d772c57 CreateContainer: pass TerminationGracePeriod
* ccefb16ad Update installation steps for CentOS
* 86145db6b oci: drop container level privileged flag
* d5d6a218c tree_status: show the git diff

### Dependency Changes

Previous release can be found at [v1.17.4](https://github.com/cri-o/cri-o/releases/tag/v1.17.4)

* **github.com/containers/buildah**               v1.14.2 -> v1.14.8
* **github.com/containers/common**                v0.4.2 -> v0.8.4
* **github.com/containers/image/v5**              v5.3.1 -> v5.4.4
* **github.com/containers/libpod**                v1.8.1 -> v1.9.2
* **github.com/containers/ocicrypt**              b87a4a69c741 -> v1.0.2
* **github.com/containers/storage**               01ee1656d552 -> v1.19.1
* **github.com/creack/pty**                       v1.1.7 -> v1.1.11
* **github.com/cri-o/ocicni**                     deac903fd99b -> 513ef787b8c9
* **github.com/fsnotify/fsnotify**                v1.4.7 -> v1.4.9
* **github.com/gogo/protobuf**                    65acae22fc9d -> v1.3.1
* **github.com/imdario/mergo**                    v0.3.8 -> v0.3.9
* **github.com/klauspost/compress**               v1.10.3 -> v1.10.5
* **github.com/opencontainers/selinux**           v1.4.0 -> v1.5.1
* **github.com/openshift/imagebuilder**           v1.1.1 -> v1.1.4
* **github.com/rootless-containers/rootlesskit**  v0.8.0 -> v0.9.3
* **github.com/spf13/cobra**                      v0.0.6 -> v0.0.7
* **github.com/urfave/cli/v2**                    v2.1.1 -> d648edd48d89
* **github.com/vbauerster/mpb/v5**                v5.0.4 **_new_**
* **github.com/xeipuuv/gojsonschema**             v1.1.0 -> v1.2.0
* **golang.org/x/crypto**                         bac4c82f6975 -> 4bdfaf469ed5
* **golang.org/x/net**                            13f9640d40b9 -> d3edc9973b7e
* **golang.org/x/sync**                           cd5d95a43a6e -> 43a5402ce75a
* **golang.org/x/sys**                            9197077df867 -> 1957bb5e6d1f
* **golang.org/x/time**                           c4c64cad1fd0 -> 555d28b269f0
