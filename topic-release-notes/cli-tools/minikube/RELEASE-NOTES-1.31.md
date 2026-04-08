# minikube v1.31 Release Notes

Source: [v1.31.2](https://github.com/kubernetes/minikube/releases/tag/v1.31.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.31.2 - 2023-08-16

docker-env Regression:
* Create `~/.ssh` directory if missing [#16934](https://github.com/kubernetes/minikube/pull/16934)
* Fix adding guest to `~/.ssh/known_hosts` when not needed [#17030](https://github.com/kubernetes/minikube/pull/17030)

Minor Improvements:
* Verify containerd storage separately from docker [#16972](https://github.com/kubernetes/minikube/pull/16972)

Version Upgrades:
* Bump Kubernetes version default: v1.27.4 and latest: v1.28.0-rc.1 [#17011](https://github.com/kubernetes/minikube/pull/17011) [#17051](https://github.com/kubernetes/minikube/pull/17051)
* Addon cloud-spanner: Update cloud-spanner-emulator/emulator image from 1.5.7 to 1.5.9 [#17017](https://github.com/kubernetes/minikube/pull/17017) [#17044](https://github.com/kubernetes/minikube/pull/17044)
* Addon headlamp: Update headlamp-k8s/headlamp image from v0.18.0 to v0.19.0 [#16992](https://github.com/kubernetes/minikube/pull/16992)
* Addon inspektor-gadget: Update inspektor-gadget image from v0.18.1 to v0.19.0 [#17016](https://github.com/kubernetes/minikube/pull/17016)
* Addon metrics-server: Update metrics-server/metrics-server image from v0.6.3 to v0.6.4 [#16969](https://github.com/kubernetes/minikube/pull/16969)
* CNI flannel: Update from v0.22.0 to v0.22.1 [#16968](https://github.com/kubernetes/minikube/pull/16968)

For a more detailed changelog, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Alex Serbul
- Anders F Björklund
- Jeff MAURY
- Medya Ghazizadeh
- Michelle Thompson
- Predrag Rogic
- Seth Rylan Gainey
- Steven Powell
- aiyijing
- joaquimrocha
- renyanda
- shixiuguo
- sunyuxuan
- Товарищ программист

Thank you to our PR reviewers for this release!

- medyagh (8 comments)
- spowelljr (2 comments)
- ComradeProgrammer (1 comments)
- Lyllt8 (1 comments)
- aiyijing (1 comments)

Thank you to our triage members for this release!

- afbjorklund (6 comments)
- vaibhav2107 (5 comments)
- kundan2707 (3 comments)
- spowelljr (3 comments)
- ao390 (2 comments)

Check out our [contributions leaderboard](https://minikube.sigs.k8s.io/docs/contrib/leaderboard/v1.31.2/) for this release!

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## Binary Checksums

darwin-amd64: `c655de8bf27557f706f196a08a0afb8730e98c4976207542e05b73c11bcc0b38`
darwin-arm64: `c00ca6685e2f7b159d9348d2d20b8dc91884c3341e814155a1cb311c26e4cf94`
linux-amd64: `88a80c051696adaa1a2a0c6aba5fde18176fd5afa87be10617ecaab9cd3a719b`
linux-arm: `306fe167b874bde8bdab2cbc223bcd4bed9f389762701973e09ac458a322a3a5`
linux-arm64: `09f450f753fe15da7e84a955f6b62c05856cef2facf564f8e609445036c8cb22`
linux-ppc64le: `11f2f9382cb1f1c1f934dff3df9e2038f7b74df32f28641f53f89b3ff95316c3`
linux-s390x: `b6185c709a9768551c79f9340b1d9ae179804aa0bd9dac6e654aa17e4d0acf73`
windows-amd64.exe: `5754e4b86ee66f111f4460f45730f32592701283ac79e1d9c07b217aab236dae`

## ISO Checksums

amd64: `4cc52896d9ab0444300737ddae6d49dd2dbcf67c14579bf3b975d55213ce96ae`  
arm64: `355556716c1de155eeb04e37ed289808f12f2a650e6aa2967f61ab4539241eb6`