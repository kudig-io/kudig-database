# helm v3.6 Release Notes

Source: [v3.6.3](https://github.com/helm/helm/releases/tag/v3.6.3)

Helm v3.6.3 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [Kubernetes Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.6.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.6.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-darwin-amd64.tar.gz.sha256sum) / 84a1ff17dd03340652d96e8be5172a921c97825fd278a2113c8233a4e8db5236)
- [MacOS arm64](https://get.helm.sh/helm-v3.6.3-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-darwin-arm64.tar.gz.sha256sum) / a50b499dbd0bbec90761d50974bf1e67cc6d503ea20d03b4a1275884065b7e9e)
- [Linux amd64](https://get.helm.sh/helm-v3.6.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-linux-amd64.tar.gz.sha256sum) / 07c100849925623dc1913209cd1a30f0a9b80a5b4d6ff2153c609d11b043e262)
- [Linux arm](https://get.helm.sh/helm-v3.6.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-linux-arm.tar.gz.sha256sum) / 6918e573a70c309fbf6385a0a0d18d090c10b44d318724f1f73e47ede4809635)
- [Linux arm64](https://get.helm.sh/helm-v3.6.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-linux-arm64.tar.gz.sha256sum) / 6fe647628bc27e7ae77d015da4d5e1c63024f673062ac7bc11453ccc55657713)
- [Linux i386](https://get.helm.sh/helm-v3.6.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-linux-386.tar.gz.sha256sum) / e7bafc7dd870621a79f7f2ad0c92e45957817a371b738da4e590ccbc45983244)
- [Linux ppc64le](https://get.helm.sh/helm-v3.6.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-linux-ppc64le.tar.gz.sha256sum) / 12ea5cdda8ee4a585230623254b997b28d4f9fb894ebf509b530af501366d0e9)
- [Linux s390x](https://get.helm.sh/helm-v3.6.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.6.3-linux-s390x.tar.gz.sha256sum) / 1419787383c8062d5cb799d072c9ed10e1c3af66d0d2395832aafaf03d2d4bfb)
- [Windows amd64](https://get.helm.sh/helm-v3.6.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.6.3-windows-amd64.zip.sha256sum) / 797d2abd603a2646f2fb9c3fabba46f2fabae5cbd1eb87c20956ec5b4a2fc634)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.6.4 will contain only bug fixes and is planned for release on August 11, 2021
- 3.7.0 is the next feature release and will be released on September 8, 2021.

## Changelog

- Ensure RawPath match Path when resolving reference d506314abfb5d21419df8c7e7e68012379db2354 (Mathieu Parent)
- Set Helm as manager for managedFields 8b678fea26303c380d25e84a85211e09b3f110cc (Matt Farina)
- fix(dep update): helm dep update is not respecting the "version" stipulated in the requirements fb31357e6b9f3f048d4d7ed57a00a7d942e86fd8 (cndoit18)
- fix(doc): fix kube client interface doc. (#9882) 29d4e1b9beec93d21f09c614b9ca3e770993ee2f (小龙同学)
- use TLS client information from repo config when downloading a chart 8565722b9a8c40b03d47925a715fb77f44ecfd70 (Christophe VILA)
- Adding test for user/pass without repo on Helm install 4ec67120c1340fdc892383296d3219d56453307c (Matt Farina)
- Fix the url being set by WithURL on the getters 473e83e303ca19403eb7cb5f110a001cf7adf632 (Matt Farina)
- tweak basic handling bcb55492d64570eee4216a0a3d6653a98a91c756 (Matt Farina)
- keep existing behavior of returning ErrReleaseNotFound when release(s) failed to decode 877276cedaab6ca583a20defcfa81d2f66d07d26 (Mike Ng)
- fix(sql storage): Query() should return ErrReleaseNotFound immediately when no records are found f2d7ed8d8099dfd94478be40eed1a237bc2d8ef5 (Mike Ng)
- Add Test cases for repository-config without file extension c084ca0259aa5f1b3defe53f74214c3aa27daae7 (Leon Bentrup)
- Correctly determine repository-config lockfile path 7d81733af747f210cd9c06cfdc2e26ef41a7edfe (Leon Bentrup)
- Fixed Test cbd2868ac2f73899dda1df9546d69ccd26230150 (Marcus Speight)
- Added test for lint mode 9fbf594b800fe047db36113e650154914fe1ff4d (Marcus Speight)
- Fail message is now the same as the required message. Fixed #8973 Helm function 'fail' should not fail when doing 'helm lint' bcee7a30fe97133ef4302354f06261abe1a8b917 (Marcus Speight)
- fix helm dep build/update doesn't inherit --insecure-skip-tls-verify from helm repo add 80402dc078908408fb724395177093f358ee7dae (yxxhero)
