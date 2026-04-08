# minikube v1.17 Release Notes

Source: [v1.17.1](https://github.com/kubernetes/minikube/releases/tag/v1.17.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.17.1 - 2020-01-28

Features:

* Support for macOS/arm64 (Apple M1 Chip)
* Add new flag --user and to log executed commands [#10106](https://github.com/kubernetes/minikube/pull/10106)
* Unhide --schedule flag for scheduled stop [#10274](https://github.com/kubernetes/minikube/pull/10274)
* Make the ssh driver opt-in and not default [#10269](https://github.com/kubernetes/minikube/pull/10269)

Bugs:

* fixing debian and arch concurrent multiarch builds [#9998](https://github.com/kubernetes/minikube/pull/9998)
* configure the crictl yaml file to avoid the warning [#10221](https://github.com/kubernetes/minikube/pull/10221)

Thank you to our contributors for this release!

- Anders F Björklund
- BLasan
- Ilya Zuyev
- Jiefeng He
- Jorropo
- Medya Ghazizadeh
- Niels de Vos
- Priya Wadhwa
- Sharif Elgamal
- Steven Powell
- Thomas Strömberg
- andrzejsydor

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`9fe214053aaff2352f150505f583e3d7f662e522d39db1839ce85f77ab51f1d5`