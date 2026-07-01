---
title: helm v3.11 Release Notes
description: helm v3.11 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- containerd
- docker
- statefulset
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.11 Release Notes 是什么
- 如何 helm v3.11 Release Notes
trigger_keywords:
- helm
- v3.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Helm|helm]] v3.11 Release Notes

Source: [v3.11.3](https://github.com/helm/helm/releases/tag/v3.11.3)

Helm v3.11.2 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.11.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.11.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-darwin-amd64.tar.gz.sha256sum) / 9d029df37664b50e427442a600e4e065fa75fd74dac996c831ac68359654b2c4)
- [MacOS arm64](https://get.helm.sh/helm-v3.11.3-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-darwin-arm64.tar.gz.sha256sum) / 267e4d50b68e8854b9cc44517da9ab2f47dec39787fed9f7eba42080d61ac7f8)
- Linux amd64](https://get.helm.sh/helm-v3.11.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-linux-amd64.tar.gz.sha256sum) / ca2d5d40d4cdfb9a3a6205dd803b5bc8def00bd2f13e5526c127e9b667974a89)
- [Linux arm](https://get.helm.sh/helm-v3.11.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-linux-arm.tar.gz.sha256sum) / 0816db0efd033c78c3cc1c37506967947b01965b9c0739fe13ec2b1eea08f601)
- [Linux arm64](https://get.helm.sh/helm-v3.11.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-linux-arm64.tar.gz.sha256sum) / 9f58e707dcbe9a3b7885c4e24ef57edfb9794490d72705b33a93fa1f3572cce4)
- [Linux i386](https://get.helm.sh/helm-v3.11.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-linux-386.tar.gz.sha256sum) / 09c111400d953eda371aaa6e5f0f65acc7af6c6b31a9f327414bb6f0756ea215)
- [Linux ppc64le](https://get.helm.sh/helm-v3.11.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-linux-ppc64le.tar.gz.sha256sum) / 9f0a8299152ec714cee7bdf61066ba83d34d614c63e97843d30815b55c942612)
- [Linux s390x](https://get.helm.sh/helm-v3.11.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.11.3-linux-s390x.tar.gz.sha256sum) / e8b0682166628a9c16bf185d60c3d766a8ff814bff362de88280ef202148fbec)
- [Windows amd64](https://get.helm.sh/helm-v3.11.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.11.3-windows-amd64.zip.sha256sum) / ae146d2a90600c6958bc801213daef467237cf475e26ab3f476dfb8e0d9549b7)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.12.0 is the next feature release and be on May 10, 2023.

## Changelog

- chore(deps): bump golang.org/x/text from 0.7.0 to 0.9.0 66a969e7cc08af2377d055f4e6283c33ee84be33 (dependabot[bot])
- Fix goroutine leak in perform 548366cb6c91301e595c9093ffd0ec119ca9dad0 (willzgli)
- Fix goroutine leak in action install 4a3a2683536b4d46639dc7460846e44f426e5e01 (Matt Farina)
- Fix 32bit-x86 typo in testsuite 272f6b9d80e35d68efb4e45942aa4d746e2df0f3 (Dirk Müller)
- chore(deps): bump github.com/docker/docker 88b2db4a07f4ea9a11751e8c3de615d6e080301a (dependabot[bot])
- chore(deps): bump github.com/containerd/containerd from 1.6.15 to 1.7.0 b6a8417daca5fe1458d8a9394164494afe410a23 (dependabot[bot])
- Fixes Readiness Check for statefulsets using partitioned rolling update. (#11774) 7994bb4d357a3846263dfb22b97da867159253fe (Aman Nijhawan)