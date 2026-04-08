# minikube v1.29 Release Notes

Source: [v1.29.0](https://github.com/kubernetes/minikube/releases/tag/v1.29.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.29.0 - 2023-01-26

Features:
* Bump QEMU driver priority from experimental to default [#15556](https://github.com/kubernetes/minikube/pull/15556)
* Ability to set static-ip for Docker driver [#15553](https://github.com/kubernetes/minikube/pull/15553)
* GCP-Auth Addon: automatically attach credentials to newly created namespaces [#15403](https://github.com/kubernetes/minikube/pull/15403)
* Allow forcing 1 CPU on Linux with docker and none driver [#15611](https://github.com/kubernetes/minikube/pull/15611) [#15610](https://github.com/kubernetes/minikube/pull/15610)

Major Improvements:
* Large improvements to cgroup detection and CNI and CRI configurations [#15463](https://github.com/kubernetes/minikube/pull/15463)
* Prevent redownloading kicbase when already downloaded [#15528](https://github.com/kubernetes/minikube/pull/15528)
* Warn when using an old ISO/Kicbase image [#15235](https://github.com/kubernetes/minikube/pull/15235)

Minor Improvements:
* Check brew install paths for socket_vmnet [#15701](https://github.com/kubernetes/minikube/pull/15701)
* Include gcp-auth logs in 'minikube logs' output [#15666](https://github.com/kubernetes/minikube/pull/15666)
* Use absolute path when calling crictl version [#15642](https://github.com/kubernetes/minikube/pull/15642)
* Add additional memory overhead for VirtualBox when `--memory=max` [#15317](https://github.com/kubernetes/minikube/pull/15317)
* Update Windows installer to create system-wide shortcut [#15405](https://github.com/kubernetes/minikube/pull/15405)
* Add `--subnet` validation [#15530](https://github.com/kubernetes/minikube/pull/15530)
* Warn users if using VirtualBox on macOS 13+ [#15624](https://github.com/kubernetes/minikube/pull/15624)
* Add groups check to SSH driver [#15513](https://github.com/kubernetes/minikube/pull/15513)
* Update references to deprecated beta.kubernetes.io [#15225](https://github.com/kubernetes/minikube/pull/15225)

Bugs:
* Fix possible race condition when enabling multiple addons [#15706](https://github.com/kubernetes/minikube/pull/15706)
* Fix cpus config field not supporting max value [#15479](https://github.com/kubernetes/minikube/pull/15479)
* Fix subnet checking failing if IPv6 network found [#15394](https://github.com/kubernetes/minikube/pull/15394)
* Fix Docker tunnel failing if too many SSH keys [#15560](https://github.com/kubernetes/minikube/pull/15560)
* Fix kubelet localStorageCapacityIsolation option [#15336](https://github.com/kubernetes/minikube/pull/15336)
* Fix setting snapshotter to unimplemented fuse-overlayfs [#15272](https://github.com/kubernetes/minikube/pull/15272)
* Remove progress bar for kic download with JSON output [#15482](https://github.com/kubernetes/minikube/pull/15482)

Version Upgrades:
* Bump default Kubernetes version from 1.25.3 to 1.26.1 [#15683](https://github.com/kubernetes/minikube/pull/15683)
* Addons: Update auto-pause from 0.0.2 to 0.0.3 [#15331](https://github.com/kubernetes/minikube/pull/15331)
* Addons: Update cloud-spanner from 1.4.6 to 1.5.0 [#15440](https://github.com/kubernetes/minikube/pull/15440) [#15667](https://github.com/kubernetes/minikube/pull/15667) [#15707](https://github.com/kubernetes/minikube/pull/15707)
* Addons: Update headlamp from 0.13.0 to 0.14.1 [#15401](https://github.com/kubernetes/minikube/pull/15401) [#15515](https://github.com/kubernetes/minikube/pull/15515)
* Addons: Update ingress from 1.2.1 to 1.5.1 [#15339](https://github.com/kubernetes/minikube/pull/15339)
* Addons: Update metrics-server from 0.6.1 to 0.6.2 [#15411](https://github.com/kubernetes/minikube/pull/15411)
* Addons: Update kubevirt from 1.17 to 1.24.7 [#15310](https://github.com/kubernetes/minikube/pull/15310)
* CNI: Update cilium from 1.9.9 to 1.12.3 [#15242](https://github.com/kubernetes/minikube/pull/15242)
* Kicbase: Update buildkit from 0.10.3 to v0.11.0 [#15630](https://github.com/kubernetes/minikube/pull/15630)
* Kicbase/ISO: Update containerd from 1.6.9 to 1.6.15 [#15541](https://github.com/kubernetes/minikube/pull/15541)
* Kicbase/ISO: Update cri-dockerd from 0.2.2 to 0.3.0 [#15541](https://github.com/kubernetes/minikube/pull/15541)
* Kicbase/ISO: Update docker from 20.10.20 to 20.10.23 [#15341](https://github.com/kubernetes/minikube/pull/15341) [#15541](https://github.com/kubernetes/minikube/pull/15541) [#15703](https://github.com/kubernetes/minikube/pull/15703)
* Update KVM-docker-machine amd64 base image from Ubuntu 16.04 to 20.04 [#15628](https://github.com/kubernetes/minikube/pull/15628)


For a more detailed changelog see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Aarav Arora
- Akihiro Suda
- Anders F Björklund
- Andrew Stanton
- Carlos Santana
- Chris Kannon
- Eng Zer Jun
- Felipe Labbate
- Ian Stewart
- Jan Hutař
- Jeff MAURY
- Kaylen Dart
- Kush Mansingh
- Ludovic Maître
- Medya Ghazizadeh
- Olivier Lemasle
- Paco Xu
- Paul S. Schweigert
- Predrag Rogic
- Ronnel Santiago
- Sharif Elgamal
- Shubh Bapna
- Steven Powell
- Yuiko Mouri
- ckannon
- imjoseangel
- joaquimrocha
- jongwooo
- mardi2020
- shixiuguo
- Товарищ программист

Thank you to our PR reviewers for this release!

- spowelljr (61 comments)
- medyagh (41 comments)
- afbjorklund (7 comments)
- atoato88 (5 comments)
- t-inu (5 comments)
- mqasimsarfraz (4 comments)
- AkihiroSuda (2 comments)
- sharifelgamal (2 comments)
- profnandaa (1 comments)

Thank you to our triage members for this release!

- afbjorklund (98 comments)
- spowelljr (34 comments)
- medyagh (10 comments)
- kant777 (6 comments)
- kundan2707 (6 comments)

Check out our [contributions leaderboard](https://minikube.sigs.k8s.io/docs/contrib/leaderboard/v1.29.0/) for this release!

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## Binary Checksums

darwin-amd64: `cb54ef7cdbcef7f078ac2966bc6c0aa43db75bbde8b361f428858137e803dac0`
darwin-arm64: `d3ba1faf5080d3c19affd7355b9e8158ee0fb1fe41422083cdd61e6f1593360d`
linux-amd64: `aafb65cbee8e971ec00509fdb1817254b17d6bee4890b839c3b6e8f11e97413a`
linux-arm: `9cf1c74c258f09e9133d02c371dbe0a85817c992d8dee815701c78e6fe442dcb`
linux-arm64: `e53bee7692b8b58144290c082c0191d50af7cc2994daf34d87e0abef9e73fae2`
linux-ppc64le: `742ba9a2867632faf7cd13c4a165081631fa93696d27ec71cfb99ed97f404362`
linux-s390x: `c95b67a9f8e1be02538f6b1708c32d6e8468dea0b65c338d67d04199da114e88`
windows-amd64.exe: `c53785c02c0327e74342e7100dd2fa8a52a689c9fea7782a9a1491b5d68dbd6a`

## ISO Checksums

amd64: `3a55746a11849247c2dd29a3f252cb7ac029d6726dfaa3be650216e53f4c9350`  
arm64: `a6c092806400bb17f71afbf96d93df63eabfed301dd3e9a35a3ead0232c28698`