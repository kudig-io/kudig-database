---
title: kind v0.1 Release Notes
description: kind v0.1 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.1 Release Notes 是什么
- 如何 kind v0.1 Release Notes
trigger_keywords:
- kind
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---



# kind v0.1 Release Notes

Source: [0.1.0](https://github.com/kubernetes-sigs/kind/releases/tag/0.1.0)

This release contains many exciting features! Amongst other things we now have support for multi-node clusters thanks to @fabriziopandini, and fixes have been made for kind on Windows.

## UPDATE 2019-02-11

Due to [CVE-2019-5736](https://access.redhat.com/security/vulnerabilities/runcescape) ("the runc escape")

`kindest/node:v1.13.2@sha256:d070e091e7c0e515a37d0834ea72828b2338acdc4bc7a13cbb84274fb14e5e83` 
or:
`kindest/node:v1.13.3@sha256:d1af504f20f3450ccb7aed63b67ec61c156f9ed3e8b0d973b3dee3c95991753c`

Should be used instead of the default image in this release. Example:

```console
kind create cluster --image=kindest/node:v1.13.3@sha256:d1af504f20f3450ccb7aed63b67ec61c156f9ed3e8b0d973b3dee3c95991753c
```

In the future we are looking at not pinning to an **exact** image by default, so that minimal fixes can be silently pushed.
We are additionally hoping to adopt [rootless containers](https://rootlesscontaine.rs/) once support lands in [[Kubernetes|Kubernetes]].

----

## Breaking changes since 0.1.0
- Default clusters will now be Kubernetes v1.13.2, and the default node image is now `kindest/node:v1.13.2@sha256:e14edfa4950e009fe560499c9db6e89daae8bd18bcb372caca6d321a86c52cda`
- nodeLifecycleHooks are no longer supported in the config, we believe this feature was unused and will revisit it later, if you need this, please file an issue with your use case!
- Deleting a cluster now attempts to delete the cluster specific `KUBECONFIG` file on the host (thanks @blakestoddard!)


## New Features
- The current kind `Config` is now at `kind.sigs.k8s.io/v1alpha2`, with support for multi-node clusters! (thanks @fabriziopandini!)
- The default image was upgraded to Kubernetes v1.13.2 (see above)
- kind should now work on hosts where userns remap is enabled in the docker daemon (thanks @vasrem!)
- Improved documentation (thanks @alejandrox1, @jimangel, @tao12345666333, @k8k, @timoreimann, @chuckha, @superbrothers, @benmoss!)
- `kubeadm`'s config is now internally `v1beta1` when new enough Kubernetes images are used, this should improve stable support for future Kubernetes versions. Additionally all config objects are populated, so they may be patched via patches in kind's config.
- When removing a `KUBECONFIG` file, we attempt to detect if the environment variable is set and warn about unsetting it (thanks @blakestoddard, @manparvesh!)
- The base image now contains conntrack, in order to support installing recent `kubeadm` packages. by default it is now `kindest/base:v20181203-d055041`
- `kind build node-image` now supports a `--kube-root` flag to explicitly specify the path to Kubernetes's source code, if unset kind will continue to auto-detect this via go's build tooling. (thanks @inercia!)

## Fixes
- On Windows the `KUBECONFIG` files should now be placed in the default directory correctly (we now use the [homedir](https://github.com/kubernetes/client-go/blob/b831b8de7155117e51afaffeb647007a756ddc92/util/homedir/homedir.go) util from [client-go](https://github.com/kubernetes/client-go))
- On Windows patching kubeadm config should work correctly
- Recent `kubeadm` debian packages should be installable in the base image after adding conntrack (see above)

----

Committers to this release are:
- @BenTheElder 
- @fabriziopandini 
- @kris-nova 
- @blakestoddard 
- @jimangel 
- @tao12345666333 
- @alejandrox1 
- @manparvesh 
- @inercia 
- @superbrothers 
- @chuckha 
- @timoreimann 
- @k8k
- @vincepri 
- @k8s-ci-robot

Thank you for your contributions!
Additionally, special thanks to @neolit123, @krzyzacy, @munnerz, and @amwat for PR reviews!