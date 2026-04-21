# helm v3.0 Release Notes

Source: [v3.0.3](https://github.com/helm/helm/releases/tag/v3.0.3)

Helm v3.0.3 is the third patch release for Helm 3 and it includes several bug fixes. Users are encouraged to upgrade for the best experience.

This release was signed with `4614 49C2 5E36 B98E` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [Kubernetes Slack](https://kubernetes.slack.com):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm 3.0.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.0.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-darwin-amd64.tar.gz.sha256))
- [Linux amd64](https://get.helm.sh/helm-v3.0.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-linux-amd64.tar.gz.sha256))
- [Linux arm](https://get.helm.sh/helm-v3.0.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-linux-arm.tar.gz.sha256))
- [Linux arm64](https://get.helm.sh/helm-v3.0.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-linux-arm64.tar.gz.sha256))
- [Linux i386](https://get.helm.sh/helm-v3.0.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-linux-386.tar.gz.sha256))
- [Linux ppc64le](https://get.helm.sh/helm-v3.0.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-linux-ppc64le.tar.gz.sha256))
- [Windows amd64](https://get.helm.sh/helm-v3.0.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.0.3-windows-amd64.zip.sha256))
- [Linux s390x](https://get.helm.sh/helm-v3.0.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.0.3-linux-s390x.tar.gz.sha256))


The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there.

## What's Next

- v3.1.0 will contain new features that do not break backwards compatibility.

## Changelog

- unnecessary import removed ac925eb7279f4a6955df663a0128044a8a6b7593 (Ahmad Kazemi)
- Signed-off-by: Ahmad Kazemi <ahmad.kazemi@recordpoint.com> log.Printf replaced to fix the log issue. bba7bc1c88ec045199b66fe88b1468a6a17102a3 (Ahmad Kazemi)
- Reverting #7266 from the 3.0 release branch and 3.0.3 140cf9d223c67118a6569caa9a30d79515d11184 (Matt Farina)
- fix(package): remove --set, --values, etc. flags cde43f8d9e4d9318c29c5e1f16c4cfd67a356f53 (Matthew Fisher)
- allow limited recursion in templates 7ef5da01c01cb1cb82d0e812cac4688cfcc66965 (zwwhdls)
- fix(chartutil): remove empty lines and a space from rendered chart templates (#7455) ee8c924115cf55b1446ff4be9e9609a105d45c75 (Shota Nakamura)
- fix(helm): improve handling of corrupted storage d3836d6f74bfe6ada6d992868561159b66396cd3 (Cristian Klein)
- Remove references to protobuf (#7425) 4783778ce39659253e6dd7ca2a03cf71a0cda206 (Martin Hickey)
- Allow tests to run on s390x (#7096) b2d18197258c2495d6200afe84e04565f89a5f9e (Vivian Kong)
- Fix: helm3 - kind sorter incorrectly compares unknown and namespace 86839f249031c348412409565b05f7ca5b3e3eab (Bradley Skuse)
- ref(pkg/storage): Refactor Deployed and DeployedAll (#7374) d2c8c71d50c3fb8ed1dac2e2424b5ac77c10b910 (Simon Alling)
- stop with an error immediately if a file or directory with that name already exists (#7187) b63f1900f986960c82dafabf685cb60ba1502490 (海的澜色)
- Do not delete templated CRDs 9711d1c6bfd9cd0cebe47b069a18cfc83ac3bfee (Phil Grayson)
- [helm create] Include serviceAccount.annotations value (#7246) 5d2ef8556a56ac0b2e09689f8e84b1ce1d1df6ff (Naseem)
- Fix a typo "update" -> "updates" (#7346) d2cf1284ee75954ffedaba8af5d8339b68e72368 (Hu Shuai)
- fix(cmd): Fixes logging on action conf init error (#6909) 64e57d92b649a67f222bff23582b5d665b639791 (Jorge I. Gasca)
- Remove duplicated words (#7336) 6d92b59fff43a264a0a0001fd01d4e51ff26cfb3 (Nguyen Hai Truong)
- Improve description for `--all` flag (#7144) 8f6b14695dd745ccbfe7fd122cc93a2185095954 (Xiang Dai)
- fix(comp): tail cannot open +2 for reading 50a647728f78925802abfa4d06bb41a4c52ade05 (Frank Lin PIAT)
- Add back fix for CRD patch creation 4771f256cdaac056a6553af42cfa3ffac5b7896c (Adrian Gonzalez-Martin)
- Port PR #4161 Fix incorrect timestamp when helm package to Helmv3 d72867582536615c311e39578dcdf65421ed9de4 (Romain Grenet)