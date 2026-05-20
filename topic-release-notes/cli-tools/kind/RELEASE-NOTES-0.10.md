---
title: kind v0.10 Release Notes
description: kind v0.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.10 Release Notes 是什么
- 如何 kind v0.10 Release Notes
trigger_keywords:
- kind
- v0.10
- Release
- Notes
- release
- notes
---

# kind v0.10 Release Notes

Source: [v0.10.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.10.0)

In `v0.10.0` the most important changes we're "shipping" are about improving sustainability:

- Doubled the set of project owners, granting @aojea & @amwat full access to help maintain the project.
A huge thank you to them for their existing contributions and for stepping up here :heart:
- Rewritten and revamped contributor guide https://kind.sigs.k8s.io/docs/contributing/getting-started/

Additionally we're trying a new approach to tracking [priorities for v0.11.0](https://github.com/kubernetes-sigs/kind/issues/2024) which you can see in our issue tracker.

Otherwise we've made a number of bug fixes and minor improvements.

Some users may appreciate the ability to run kind v0.10.0 (NOT the currently installed v0.7.0) in the free [Google Cloud Shell](https://cloud.google.com/shell) for quick demos, workshops, etc.

Simply install kind somewhere under $HOME and add it to your $PATH to use it in cloud shell:

```console
GO111MODULE=on go get sigs.k8s.io/kind@v0.10.0
export PATH="$(go env GOPATH)/bin:${PATH}"
kind create cluster
```

To persist this `PATH` for future usage you can edit your `$HOME/.bashrc` in cloudshell.

<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.20.2` image: `kindest/node:v1.20.2@sha256:8f7ea6e7642c0da54f04a7ee10431549c0257315b3a634f6ef2fecaaedb19bab`
- We're only actively supporting Kubernetes `v1.14.0`+, however limited best effort functionality still encompases `v1.13.0` for now.
- Images from KIND `v0.8.0+` should continue to work for now, but lack various improvements.
- udev is explicitly disabled at the node level, though as always please try not to depend on the inner details of nodes beyond providing a particular Kubernetes version with kind
- IPv6 pod subnet defaults to /56 instead of /64 (a necessary fix for newer Kubernetes, see #1903)


<h1 id="new-features">New Features</h1>

- `kind export logs` now includes the kind version
- Node images compiled without dockershim for Kubernetes v1.19+ possible, reducing size
- Reduced kind binary size further (~7MB) with improved build options
- Improved zsh completion
- Experimental github actions CI for podman, docker, cgroupsv2
- Expanded WSL2 documentation
- Revamped contributor guide
- Improved docs site implementation, including table of contents generation
- Updated dependencies
- New Loadbalancer user guide
- Better support for running in nested container environments when using images built with v0.10.0+
  - This enables cloud shell support

New Node images have been built for kind `v0.10.0`, please use these **exact** images (IE like `v1.20.2:@sha256:8f7ea6e7642c0da54f04a7ee10431549c0257315b3a634f6ef2fecaaedb19bab` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.20: `kindest/node:v1.20.2@sha256:8f7ea6e7642c0da54f04a7ee10431549c0257315b3a634f6ef2fecaaedb19bab`
  - 1.19: `kindest/node:v1.19.7@sha256:a70639454e97a4b733f9d9b67e12c01f6b0297449d5b9cbbef87473458e26dca`
  - 1.18: `kindest/node:v1.18.15@sha256:5c1b980c4d0e0e8e7eb9f36f7df525d079a96169c8a8f20d8bd108c0d0889cc4`
  - 1.17: `kindest/node:v1.17.17@sha256:7b6369d27eee99c7a85c48ffd60e11412dc3f373658bc59b7f4d530b7056823e`
  - 1.16: `kindest/node:v1.16.15@sha256:c10a63a5bda231c0a379bf91aebf8ad3c79146daca59db816fb963f731852a99`
  - 1.15: `kindest/node:v1.15.12@sha256:67181f94f0b3072fb56509107b380e38c55e23bf60e6f052fbd8052d26052fb5`
  - 1.14: `kindest/node:v1.14.10@sha256:3fbed72bcac108055e46e7b4091eb6858ad628ec51bf693c21f5ec34578f6180`


<h1 id="fixes">Fixes</h1>

- Fixed development scripts when CDPATH is in use
- Fixed building node images with bazel when CWD is not within the source directory
- Disabled fancy terminal output when in `st` (see #1924)
- Fixed various typos
- Improved cgroups handling
- Improved some error messages


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @AkihiroSuda
- @AndiDog
- @aojea
- @arangamani
- @BenTheElder
- @FedericoAntoniazzi
- @frelon
- @Git-Jiro
- @goneri
- @gunadhya
- @Hellcatlk
- @Huang-Wei
- @jbarrick-mesosphere
- @jessehu
- @jlucktay
- @jsoref
- @k8s-ci-robot
- @kensipe
- @knabben
- @micarlise
- @mjpitz
- @nicholasdille
- @nicks
- @yangy2000
