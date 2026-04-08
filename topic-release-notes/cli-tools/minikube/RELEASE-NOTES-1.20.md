# minikube v1.20 Release Notes

Source: [v1.20.0](https://github.com/kubernetes/minikube/releases/tag/v1.20.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.20.0 - 2021-05-06

Feature:
* Add --file flag to 'minikube logs' to automatically put logs into a file. [#11240](https://github.com/kubernetes/minikube/pull/11240)

Minor Improvements:
* Batch logs output to speedup `minikube logs` command [#11274](https://github.com/kubernetes/minikube/pull/11274)
* warn about performance for certain versions of kubernetes [#11217](https://github.com/kubernetes/minikube/pull/11217)

Version Upgrades:
* Update olm addon to v0.17.0 [#10947](https://github.com/kubernetes/minikube/pull/10947)
* Update newest supported Kubernetes version to v1.22.0-alpha.1 [#11287](https://github.com/kubernetes/minikube/pull/11287)

For a more detailed changelog, including changes occuring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Anders F Björklund
- Andriy Dzikh
- Daehyeok Mun
- Ilya Zuyev
- Medya Ghazizadeh
- Predrag Rogic
- Sharif Elgamal
- Steven Powell
- Tomas Kral
- Yanshu
- zhangshj

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`dce14eb2225dfff8c41ace4142e0fba180e09d83770f731f5134fb182ea0ec31`