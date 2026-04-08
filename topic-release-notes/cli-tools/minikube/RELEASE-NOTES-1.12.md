# minikube v1.12 Release Notes

Source: [v1.12.3](https://github.com/kubernetes/minikube/releases/tag/v1.12.3)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.12.3 - 2020-08-12

Features:

* Make waiting for Host configurable via --wait-timeout flag [#8948](https://github.com/kubernetes/minikube/pull/8948)

Bug Fixes:

* Ignore localhost proxy started with scheme. [#8885](https://github.com/kubernetes/minikube/pull/8885)
* Improve error handling for validating memory limits [#8959](https://github.com/kubernetes/minikube/pull/8959)
* Skip validations if --force is supplied [#8969](https://github.com/kubernetes/minikube/pull/8969)
* Fix handling of parseIP error [#8820](https://github.com/kubernetes/minikube/pull/8820)

Improvements:

* GCP Auth Addon: Exit with better error messages [#8932](https://github.com/kubernetes/minikube/pull/8932)
* Add warning for ingress addon enabled with driver of none [#8870](https://github.com/kubernetes/minikube/pull/8870)
* Update Japanese translation [#8967](https://github.com/kubernetes/minikube/pull/8967)
* Fix for a few typos in polish translations [#8950](https://github.com/kubernetes/minikube/pull/8950)

Thank you to our contributors for this release! 

- Anders F Björklund
- Andrej Guran
- Chris Paika
- Dean Coakley
- Evgeny Shmarnev
- Ling Samuel
- Ma Xinjian
- Marcin Niemira
- Medya Ghazizadeh
- Pablo Caderno
- Priya Wadhwa
- RA489
- Sharif Elgamal
- TAKAHASHI Shuuji
- Thomas Strömberg
- inductor
- priyawadhwa
- programistka

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`c993e057d3ee7e1ea71b3c3dc2cba9de0e2f97bbe078ac8efd9f8d40db2ddfba`