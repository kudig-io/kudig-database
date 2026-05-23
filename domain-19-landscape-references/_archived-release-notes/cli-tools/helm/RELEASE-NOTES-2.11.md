---
title: helm v2.11 Release Notes
description: helm v2.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- helm v2.11 Release Notes 是什么
- 如何 helm v2.11 Release Notes
trigger_keywords:
- helm
- v2.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- policy-basics
created: "2026-05-23"
---

# [[Helm|helm]] v2.11 Release Notes

Source: [v2.11.0](https://github.com/helm/helm/releases/tag/v2.11.0)

Helm v2.11.0 is a feature release. This release continues our focus on improving stability in this release, limiting our enhancements to things that improve the reliability of Helm in production. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
  - `#charts` for discussion on the community chart repositories
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## What's Changed?

- added Kubernetes v1.11 support
- Updated Sprig to v2.16.0
- Helm has now switched from a CLA to a DCO. [More info here](https://www.helm.sh/blog/helm-dco/index.html)
- `helm repo add --username` without providing a password with `--password` opens a hidden password prompt
- fixed a bug where `helm lint` would fail when a `required` template value was not provided
- allowed helm zsh autocompletion to be auto-loaded by compinit
- both compiled binaries of `helm` and `tiller` are now distributed with each release
- fixed an issue where tiller running locally wouldn't respect kubectl's auth provider plugins
- many improvements to the "fake" helm client for simulating upgrades, rendering, etc. in test scenarios
- added a --tls-hostname flag to `helm` commands that interact with tiller
- `helm create` has been updated to create release labels that conform to the label conventions suggested by SIG Apps
- fixed a bug in `helm list` where only the first release in a given chunk from tiller was displayed
- releases in `helm list` can now be sorted by chart name via `helm list --chart-name`
- fixed a regression where proxy environment variables were not respected
- TLS flags can now be set from environment variables! See `helm help` for more information
- fixed a race condition in `helm init --wait`
- fixed a bug where `helm dependency build` and `helm dependency update` did not respect a repository's credentials

There were so many fixes this release that we're probably missing a few noteworthy ones, so we suggest by having a look at [the changelog](https://github.com/helm/helm/compare/v2.10.0...v2.11.0) for the full list of fixes and enhancements!

## Installation and Upgrading

Download Helm 2.11. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v2.11.0-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-darwin-amd64.tar.gz.sha256))
- Linux amd64](https://get.helm.sh/helm-v2.11.0-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-linux-amd64.tar.gz.sha256))
- [Linux arm](https://get.helm.sh/helm-v2.11.0-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-linux-arm.tar.gz.sha256))
- [Linux arm64](https://get.helm.sh/helm-v2.11.0-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-linux-arm64.tar.gz.sha256))
- [Linux i386](https://get.helm.sh/helm-v2.11.0-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-linux-386.tar.gz.sha256))
- [Linux ppc64le](https://get.helm.sh/helm-v2.11.0-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-linux-ppc64le.tar.gz.sha256))
- [Linux s390x](https://get.helm.sh/helm-v2.11.0-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v2.11.0-linux-s390x.tar.gz.sha256))
- [Windows amd64](https://get.helm.sh/helm-v2.11.0-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.11.0-windows-amd64.zip.sha256))

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get) on any system with `bash`.

## What's Next

- v2.11.1 will contain only bug fixes.
- v2.12.0 is the next feature release.

## Changelog

- fix(helm): fix regression with TLS flags/environment variables not being parsed (#4657) 2e55dbe1fdb5fdb96b75ff144a339489417b146b (Matthew Fisher)
- Fix credentials not set for ResolveChartVersion default HTTP client (#4662) 19467a536a23c46d0a8d5df9136ebbcf4e43917e (Caleb Delnay)
- fix(helm): fix selector typo in service template for 'helm create' (#4663) 3946629409699a7f90997ee82bb90bb0066a4735 (Qiang Li)
- Make ping() request a specific image. Add a getTillerPodImage method. (#4622) 6e8719e11e59979dc5bcc32f0c7ddfd825175b83 (Louis Munro)
- Check for err before working with newRelease. (#4630) 28d295be2a94115b786ee277dffcc2b5483bde47 (Steve Wolter)
- chore: update Sprig to 2.16 (#4652) 4cc4aa578562dcc556a3329902680fa1b8ab3352 (Matt Butcher)
- Fix race condition in `helm list` (#4620) 8d408876a059223c9a29d99efb35b3497a53504c (Matthew Fisher)
- Fix for checking helm version slice bounds out of range (#4609) 67b142ab0daeec1c3cd34b5813b9982c1afdf37d (Robert James Hernandez)
- bump version to v2.11 3a551d01d8e021679fd3d05ef9b97136e99b79e7 (Matthew Fisher)
- Be explicit about where occurences of <CHARTNAME> will be replaced in starter charts (#4548) 094b97ab5d7e2f6eda6d0ab0f2ede9cf578c003c (Anton Osmond)
- Fix grammer for tests (#4599) c539454c9cddaa50f1de184062e172cea4932aa8 (Ian Chen)
- introduce `helm init --automount-service-account-token` (#4589) 10db6a6fb56253aaa1aadf14cc7e5bd1a76aa6f8 (Matthew Fisher)
- allow settings TLS flags from environment variables (#4590) bef59e40dc75874edcaef683da644ac162820fb3 (Matthew Fisher)
- fix(release_server): handle the case when requested values is empty (#4604) 941b1f4d68b6cf3d299df880152d90227e1a441e (Matthew Fisher)
- Avoid importing k8s.io/kubernetes from pkg/helm (#4499) 37a731db798de00bf94837e207a8a418c76b6557 (Fabian Ruff)
- Set proxy for all connections, fixes #4326 (#4579) 2e9855b98ba0a04a4aa1576f3ed769b0dbd15c42 (Christian Köberl)
- feat(helm): added new helm notes command to display notes provided by the chart of a release 7f703f50a95a48ce960ccc83cb7ed6d1801588cb (Arash Deshmeh)
- feat(helm): add ability to sort release list by chart name 380ca1a9232c1b022011bd5c5a946a0206518a3f (Arash Deshmeh)
- fix(helm): Add --tiller-tls-hostname flag to 'helm init' 1b34a511d4ae38e43518e99a8250330515e3a93c (aswinkarthik)
- feat(helm): hiding password input on terminal 518a9d63e6cdcc06a816c6b6ca27ff36fe1636ec (roc)
- Add containerized options for tests in Makefile 17151b99eadd4402e7c38a4808d3f186b55df31e (mattjmcnaughton)
- Tweeks per bacon review c4d6b5a434bf87c33e5fd0ad5679af015c924844 (smurfralf)
- Improve documentation for helm upgrade (#4030) eeb3a1454a54194bada37661dab57d397c3e899b (smurfralf)
- Fix typo in message.go 0b4e086e0572ae405a65af0a61c2dbe5a4ced2e5 (Jon Huhn)
- Messages are encrypted when using TLS 0b3924b2ac7ea2204523e976647825788d854481 (Tim Hobbs)
- Moving from CLA to DCO in contribution guide a4e2e8b505e47336fa893da3a297fbe6094bba94 (Matt Farina)
- Fix helm create note for k8 label convention changes 7306b4c28eadcd06a9f090546d6edec8a0067ac0 (Martin Hickey)
- docs: remove extraneous "it" 8c69e1af4fdc904daf6b2613607c0a4492b1a181 (Dustin Specker)
- strip out all extra lines other than the first for parsing c15a355da75570b74ad3b93ba70a7c38702169af (Matthew Fisher)
- Fix typo in parser.go d92939119993174ae264dfec06db7cb1b07e37c9 (Jon Huhn)
- Update zoom.us link 1ed6ffbdb08ab3c617ebe9b042a959a7aa098f49 (Nick Schuch)
- docs(README): Updating for Helm in CNCF a8229323cd88bf8b4c457bf7b2a4b0d93f81f349 (Matt Farina)
- fix: link to custom resource definitions section f15d65845019f549679d06e18db9ec7ce7686922 (Alexey Volkov)
- fix(client): fix bug in list releases to append all releases 38eb73760b44f25b517f6f2f3c48cbb7dc047bb8 (Matt Tucker)
- add Tillerless Helm blog post and plugin references c4c9287a0ae6d2e6bf458e2577aae65fe93474b0 (rimas)
- Add basic tutorial for beginners (#4466) 204f823b5eefe458b51a8d240cab611a06d03d84 (muffin87)
- sort links alphabetically 8f7c0079fe86319c7bca00624e39f49b3e67e2cd (Matthew Fisher)
- docs(alpine): quote release label value (#4460) e8b003af9a4fd1f0dc9d3b4a3eae2fca422a2226 (Matt Butcher)
- Add link to doc for Helm Stop plugin c658639ccc96645286beb119ce2baa31c9512235 (Martin Hickey)
- docs(generated): regenerate markdown docs 59a60a6e7c91f42ebb76e8257fbf013f12ba5a57 (Adam Reese)
- ref(*): kubernetes v1.11 support efadbd88035654b2951f3958167afed014c46bc6 (Adam Reese)
- bump version to v2.10 76f325322a2e76fca1c1ba9bec62a3fada173205 (Matthew Fisher)
- Updating to the k8s label convention e328d00a2ffea7e5e57c2b262087398f85c531fb (Matt Farina)
- fix(helm): fix(helm): add `--tls-hostname` flag to tls flags bd0686731c4d0bcf2bf1282f915bb20da3770c21 (fibonacci1729)
- fix(release_server): fix how we merge values 3e0de0dae9be9dd42386ab7e5a73abd9cc831204 (Michelle Noorali)
- distribute tiller binary with each release c98f3a1a05cd232624a1b36dfd4c0be8034109a9 (Matthew Fisher)
- [manifests] hoist the regex out of SplitManifests 15ef839ff49467c4fd14abab4d564bc2b4747c72 (Mike Lundy)
- [fake] implement rendering and simulated upgrades 0a9c16f42bd2a08f660995b397b0b1b0fdb941fa (Mike Lundy)
- [templates] extract some rendering code into a package 4139a00e17ac292357bd63a5f109d686c06ecb2c (Mike Lundy)
- [tiller] move the Manifest type to its own pkg 67de9f2be4c53acee41a02067dd97f5594ef9925 (Mike Lundy)
- Only propagate query string if refURL is relative to baseURL 152fdaf5ba52745af6b4481bed04a4977a11f1f4 (Tomas Restrepo)
- Propagate query string arguments in repository URL to absolute chart URL db69200152575a079d832b62a8465b72b9298c44 (Tomas Restrepo)
- add support for `auth-provider` from kubeconfig files, addreses #4422 3dddd5080e0fd95ad6b7dde1b9c69031c11483ec (Rimas)
- setup connection after displaying client version a7ab81f8e741a228242d47b22beeaaa507575ea6 (Matthew Fisher)
- revert back to /tiller cfbc1744c71ab926e179da595a3e35647c15e4d2 (Matthew Fisher)
- Fixed error in docs for file access 1b955e63f7862c95c3e5033d67f2b2b6e2a11759 (Michael Huttner)
- use dot notation for release candidates fb64bb66de0c5ccedce1ad52c64aa77fe0cb734a (Matthew Fisher)
- Allow zsh completion to be autoloaded by compinit cd6dd313979e68cb4f700ad87387076e77b94572 (Dusty Rip)
- Do not fail linting because of missing 'required' template values 8ce64076d34ae95aec7c28b58c46916be6009ebe (Curtis Mattoon)
