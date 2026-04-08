# minikube v1.4 Release Notes

Source: [v1.4.0](https://github.com/kubernetes/minikube/releases/tag/v1.4.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.4.0 - 2019-09-17

Notable user-facing changes:

* Update default Kubernetes version to v1.16.0 [#5395](https://github.com/kubernetes/minikube/pull/5395)
* Upgrade dashboard to 2.0.0b4 [#5403](https://github.com/kubernetes/minikube/pull/5403)
* Upgrade addon-manager to v9.0.2, improve startup and reconcile latency [#5405](https://github.com/kubernetes/minikube/pull/5405)
* Add --interactive flag to prevent stdin prompts [#5397](https://github.com/kubernetes/minikube/pull/5397)
* Automatically install docker-machine-driver-hyperkit if missing or incompatible [#5354](https://github.com/kubernetes/minikube/pull/5354)
* Driver defaults to the one previously used by the cluster [#5372](https://github.com/kubernetes/minikube/pull/5372)
* Include port names in the 'minikube service' cmd's output [#5290](https://github.com/kubernetes/minikube/pull/5290)
* Include ISO files as part of a GitHub release [#5388](https://github.com/kubernetes/minikube/pull/5388)

Thank you to our contributors for making the final push to our biggest release yet:

- Jan Janik
- Jose Donizetti
- Josh Woodcock
- Medya Ghazizadeh
- Thomas Strömberg
- chentanjun

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`0d9e0240d4efdb3ce2c70b63f258ac44e364b1b308c5cd12950c3cf2f4ca3ca2`