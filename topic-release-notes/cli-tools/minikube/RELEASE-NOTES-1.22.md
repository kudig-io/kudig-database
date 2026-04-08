# minikube v1.22 Release Notes

Source: [v1.22.0](https://github.com/kubernetes/minikube/releases/tag/v1.22.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.22.0 - 2021-07-07

Features:
* `minikube version`: add `--components` flag to list all included software [#11843](https://github.com/kubernetes/minikube/pull/11843)

Minor Improvements:
* auto-pause: add support for other container runtimes [#11834](https://github.com/kubernetes/minikube/pull/11834)
* windows: support renaming binary to `kubectl.exe` and running as kubectl [#11819](https://github.com/kubernetes/minikube/pull/11819)

Bugs:
* Fix "kubelet Default-Start contains no runlevels" error [#11815](https://github.com/kubernetes/minikube/pull/11815)

Version Upgrades:
* bump default kubernetes version to v1.21.2 & newest kubernetes version to v1.22.0-beta.0 [#11901](https://github.com/kubernetes/minikube/pull/11901)

For a more detailed changelog, including changes occuring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Anders F Björklund
- Andriy Dzikh
- Dakshraj Sharma
- Ilya Zuyev
- Jeff MAURY
- Maxime Kjaer
- Medya Ghazizadeh
- Rajwinder Mahal
- Sharif Elgamal
- Steven Powell

Thank you to our PR reviewers for this release!

- medyagh (27 comments)
- sharifelgamal (10 comments)
- andriyDev (5 comments)
- spowelljr (4 comments)
- ilya-zuyev (3 comments)

Thank you to our triage members for this release!

- medyagh (16 comments)
- spowelljr (7 comments)
- afbjorklund (4 comments)
- mahalrs (4 comments)
- sharifelgamal (3 comments)

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`4aa1622b1b5822fe3aa466313dbbbc905bff634c06d7a22f577acde1796294e1`