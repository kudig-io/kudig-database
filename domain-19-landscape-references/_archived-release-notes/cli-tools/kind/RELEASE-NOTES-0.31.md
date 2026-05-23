---
title: kind v0.31 Release Notes
description: kind v0.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.31 Release Notes 是什么
- 如何 kind v0.31 Release Notes
trigger_keywords:
- kind
- v0.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kind v0.31 Release Notes

Source: [v0.31.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.31.0)

This release contains dependency updates and defaults to [[Kubernetes|Kubernetes]] 1.35.0. 

Please take note of the breaking changes from Kubernetes 1.35, and how to prepare for **future** changes to move off of the deprecated kubeam v1beta3 in favor of v1beta4. We will include updated reminders for both again in subsequent releases.

<h1 id="breaking-changes">Breaking Changes</h1>

The default node image is now `kindest/node:v1.35.0@sha256:452d707d4862f52530247495d180205e029056831160e22870e37e3f6c1ac31f`

<h2 id="kubernetes-cgroupv1">Kubernetes 1.35+ Cgroup v1</h2>

Kubernetes [will be removing cgroup v1 support](https://kubernetes.io/blog/2025/12/17/kubernetes-v1-35-release/#removal-of-cgroup-v1-support), and therefore kind node images at those versions will also be dropping support.

You can read more about this change in the Kubernetes release blog: https://kubernetes.io/blog/2025/12/17/kubernetes-v1-35-release/#removal-of-cgroup-v1-support

If you must use kind on cgroup v1, we recommend using an older Kubernetes release for the immediate future, but we also strongly recommend migrating to cgroup v2.

In the near future as Kubernetes support dwindles, KIND will also clean up cgroup v1 workarounds and drop support in future kind releases and images, regardless of Kubernetes version.

Most stable linux distros should be on cgroupv2 out of the box.

This is a reminder to use pinned images by digest, see the note below about images for this release.

<h2 id="kubeadm-config">Kubeadm Config *Future* Breaking Change</h2>

**WARNING**: Future kind releases will [adopt kubeadm v1beta4](https://github.com/kubernetes-sigs/kind/issues/3847) configuration, [kubeadm](https://github.com/kubernetes/kubeadm) v1beta4 has a breaking change to `extraArgs`: https://kubernetes.io/blog/2024/08/23/kubernetes-1-31-kubeadm-v1beta4/. 

If you use the `kubeadmConfigPatches` feature then you may need to prepare for this change. 
We recommend that you use versioned config patches that explicitly match the version required.

KIND uses kubeadm v1beta3 for Kubernetes 1.23+, and will likely use v1beta4 for Kubernetes 1.36+
The exact version is TBD pending work to fix this but expected to be 1.36.
It will definitely be an as-of-yet-unreleased Kubernetes version to avoid surprises, and it will not be on a patch-release boundary.

KIND _may_ still work with older Kubernetes versions at v1beta2, but we no longer test or actively support these as Kubernetes only supports 1.32+ currently: https://kubernetes.io/releases/

You likely only need v1beta3 + v1beta4 patches, you can take your existing patches that work with v1beta3, explicitly set `apiVersion: kubeadm.k8s.io/v1beta3` in the patch at the top level, and make another copy for v1beta4. The v1beta4 patch will need to move `extraArgs` from a map to a list, for examples see: https://kubernetes.io/docs/reference/config-api/kubeadm-config.v1beta4/

For a concrete example of kind config with kubeadm config patch targeting both v1beta3 and v1beta4, consider this simple kind config that sets verbosity of the apiserver logs:

```yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
kubeadmConfigPatches:
# patch for v1beta3 (1.23 ...)
- |
  kind: ClusterConfiguration
  apiVersion: kubeadm.k8s.io/v1beta3
  apiServer:
    extraArgs:
      "v": "4"
# patch for v1beta4 (future)
- |
  kind: ClusterConfiguration
  apiVersion: kubeadm.k8s.io/v1beta4
  apiServer:
    extraArgs:
      - name: "v"
        value: "4"
```

If you only need to target a particular release, you can use one version.

If you only need to target fields that did not change between kubeadm beta versions, you can use a versionless patch, which may be more convenient, but we cannot guarantee there will be no future kubeadm config breaking changes.

<h1 id="new-features">New Features</h1>

- Assorted unspecified dependency updates

Images pre-built for this release:
- v1.35.0: `kindest/node:v1.35.0@sha256:452d707d4862f52530247495d180205e029056831160e22870e37e3f6c1ac31f`
- v1.34.3: `kindest/node:v1.34.3@sha256:08497ee19eace7b4b5348db5c6a1591d7752b164530a36f855cb0f2bdcbadd48`
- v1.33.7: `kindest/node:v1.33.7@sha256:d26ef333bdb2cbe9862a0f7c3803ecc7b4303d8cea8e814b481b09949d353040`
- v1.32.11: `kindest/node:v1.32.11@sha256:5fc52d52a7b9574015299724bd68f183702956aa4a2116ae75a63cb574b35af8`
- v1.31.14: `kindest/node:v1.31.14@sha256:6f86cf509dbb42767b6e79debc3f2c32e4ee01386f0489b3b2be24b0a55aac2b`

**NOTE**: You _must_ use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Detect additional edge case with ipv6 support on the host
- Make development / release scripts GOTOOLCHAIN aware

<h1 id="contributors">Contributors</h1>

Committers for this release:
- @AkihiroSuda
- @adambkaplan
- @afbjorklund
- @aoxn
- @BenTheElder
- @dependabot[bot]
- @k8s-ci-robot
- @kalexmills
- @kishen-v
- @mikejoh
- @rayowang
- @shahar1
- @stmcginnis