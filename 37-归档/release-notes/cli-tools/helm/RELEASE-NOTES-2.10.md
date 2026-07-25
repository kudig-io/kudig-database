---
title: helm v2.10 Release Notes
description: helm v2.10 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- opa
- job
- ingress
- rbac
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- helm v2.10 Release Notes 是什么
- 如何 helm v2.10 Release Notes
trigger_keywords:
- helm
- v2.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Helm|helm]] v2.10 Release Notes

Source: [v2.10.0](https://github.com/helm/helm/releases/tag/v2.10.0)

Helm v2.10.0 is a feature release. This release continues our focus on improving stability in this release, limiting our enhancements to things that improve the reliability of Helm in production. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
  - `#charts` for discussion on the community chart repositories
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## What's Changed?

Plenty!

- Masterminds/sprig, the library that provides functions to Helm templates, was bumped to v2.15.0. The release notes on what's been added can be found [here](https://github.com/Masterminds/sprig/releases/tag/v2.15.0)
- Added the `--col-width` flag to `helm search`
- Added the`--description` flag to specify a description for a given release on install, upgrade, rollback, and delete operations
- Added the `--tls-hostname` flag when connecting to Tiller through a tunnel (in other words, the default behaviour of `helm` commands)
- Added s390x releases for the Helm client
- Added support for injecting a `name` label into the namespace created through `helm install --namespace`
- Added support for reading from stdin when using plugin getters
- Added the ability to specify a kubeconfig file using `--kubeconfig`
- Allowed helm plugins to return their respective exit codes rather than `1` on a non-zero exit code
- Changed Tiller's Docker image to run as user `nobody`
- Fixed a bug where `helm init -o` wouldn't display the service and secrets to be installed
- Fixed a bug where `helm install --dep-up` wouldn't deploy the updated dependencies
- Fixed a bug where `helm install --set foo=null` wouldn't coerce into a null type
- Fixed a bug where `helm install --set-string foo=null,bar=true` wouldn't coerce into string types
- Fixed a bug where `helm lint` wouldn't fail when Chart.yaml was missing
- Fixed a bug where `helm list` on a fresh cluster would show a stack trace
- Fixed a bug where `helm template -x` would error out due to pathing issues for Windows users
- Fixed a bug where `helm upgrade --force --dry-run` wouldn't obey `--dry-run`
- Fixed a bug where installing packaged charts from servers returning `application/x-tar` headers rather than `application/x-gzip` headers resulted in an error
- Fixed a regression where users needed to `--set` a bogus field for `--reuse-values` to work
- Fixed a regression where using Tiller with the [helm-local](https://github.com/adamreese/helm-local) plugin wouldn't support kubeconfig files with `auth-provider` authorization support
- Fixed the `--tiller-namespace` flag for helm plugins
- Suppressed the 'unauthenticated users' warning when `--tiller-tls-verify` is present on `helm init`

There were so many fixes this release that we're probably missing a few noteworthy ones, so we suggest by having a look at the changelog for the full list of fixes and enhancements!

## Installation and Upgrading

Download Helm 2.10. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v2.10.0-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-darwin-amd64.tar.gz.sha256))
- [Linux amd64](https://get.helm.sh/helm-v2.10.0-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-linux-amd64.tar.gz.sha256))
- [Linux arm](https://get.helm.sh/helm-v2.10.0-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-linux-arm.tar.gz.sha256))
- [Linux arm64](https://get.helm.sh/helm-v2.10.0-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-linux-arm64.tar.gz.sha256))
- [Linux i386](https://get.helm.sh/helm-v2.10.0-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-linux-386.tar.gz.sha256))
- [Linux ppc64le](https://get.helm.sh/helm-v2.10.0-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-linux-ppc64le.tar.gz.sha256))
- [Linux s390x](https://get.helm.sh/helm-v2.10.0-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v2.10.0-linux-s390x.tar.gz.sha256))
- [Windows amd64](https://get.helm.sh/helm-v2.10.0-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.10.0-windows-amd64.zip.sha256))

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get) on any system with `bash`.

## What's Next

- v2.10.1 will contain only bug fixes.
- v2.11.0 is the next feature release.

## Changelog

- bump version to v2.10 9ad53aac42165a5fadc6c87be0dea6b115f93090 (Matthew Fisher)
- add support for `auth-provider` from kubeconfig files, addreses #4422 313c1b171b95e7bfce1bfd012946906ced07c218 (Rimas)
- fix(helm): fix(helm): add `--tls-hostname` flag to tls flags df03a10a09b267329e6b662dadc002e46b2f50b6 (fibonacci1729)
- Only propagate query string if refURL is relative to baseURL 4241705db74d7fa333eedf88b74fe7eb2e8d4696 (Tomas Restrepo)
- Propagate query string arguments in repository URL to absolute chart URL b40e8ce8302dcaac589749ccf852308edfcf6afa (Tomas Restrepo)
- fix(release_server): fix how we merge values 39d41d09c6ab4937eca03d622891c536d47fe2f4 (Michelle Noorali)
- setup connection after displaying client version 28b4171da00d49b8820a060b97affc85e61e332c (Matthew Fisher)
- revert back to /tiller 1d8193dc0bbf203a200368814d9c4b9e4040a587 (Matthew Fisher)
- replace  github org with The Kubernetes package manager 5a56449a2f243bf33ae82761c7fdbb701cb459eb (Rimas)
- [tiller] make update --force --dry-run obey dry-run 0736022d98228e35ffc9cf87871a123629425757 (Mike Lundy)
- soften the recommendation of hypens in chart names 089af0e1aaca15394de947bbd686fd4a03197c83 (Matthew Fisher)
- add every release artifact to the release notes 6307aad16dea397ce8697261f95ea361e7fee736 (Matthew Fisher)
- add amendments to release checklist e294439e01dbff1a200112dc2d864a3663165039 (Matthew Fisher)
- fix `helm template -x` pathing issues on Windows 1a1ea6383004e13c400070ec50e5003da69963e5 (Matthew Fisher)
- change copyright to "Copyright The Helm Authors" 2d77db11fa47005150e682fb13c3cf49eab98fbb (Matthew Fisher)
- fix path output for Windows d8b46d840be34f2232cd72ee980b646ccdf8aa6c (Matthew Fisher)
- Adding space for correct formatting on docs.helm.sh 9effff424f38591ec6eb9e22138afe259aba2c73 (Anders Rasmussen)
- README: updated links to helm/helm ade712dad2864277b79ef93347056cf7f367e35d (Bj??rn Magnusson)
- feat: Set values from local files (#3758) dc939086267357aa1a87418fa38ed91e68b535bc (KUOKA Yusuke)
- Dashes are not allowed in the name a65710679837ff12879d90c76aa99764e6f2178f (nashasha1)
- docs(OWNERS): add rimusz as emeritus maintainer (#4357) 4f1fec3edbcf06f3e358a53375570ef8c8195410 (Matt Butcher)
- Snap installation information added (#4366) 84856089c687e4b715631bb2380efbb561c30626 (Ihor Dvoretskyi)
- feat(helm): Detailed exit code for helm plugins (#4367) 3e5b4066d2a067ef49f6ca39b3f84d2ebcad25ba (KUOKA Yusuke)
- feat(helm): Add the --kubeconfig flag (#4235) 4efa18a5ec135229e36d3aeaca0f3cfa5a0fdb38 (Rohan Chakravarthy)
- Add missing space 02f8130dbe1d46e5c8c47c8db9f2420c69e4aeb0 (Dan Clarke)
- docs(rbac.md): delete redundant step 30f245c0d3178cb185c0a72e0c79369626e221ce (Michelle Noorali)
- ref(docs): add more links to tiller rbac info 57e7b07c3e9fedd46cd81db4e9315f7edca3dd6e (Michelle Noorali)
- Slack channels now have URL's 93ead1d0c9d7bdfb70c3530e9e8ab8bb26553442 (Ihor Dvoretskyi)
- helm template cmd - conformity of output (#3811) 01da56f956329bbfcef14ac164f9ab49081ad108 (Erik Sundell)
- [fake] return the correct error message for missing releases a6ab04be4d6f214e664a388101cdd8ace56015ab (Mike Lundy)
- [fake] make InstallReleaseFromChart obey DryRun 327080161dce6921ec5ca085d404b7cadbdc3ae7 (Mike Lundy)
- Include exact details to configure storage=secret fc7b1caf9ba81d1b6f9dcf7e0ec66be79d5d99c5 (A. Stiles)
- Revert "fix(helm): add `--tls-hostname` flag to tls flags." aedd306e83fa8319ddaaa9df0a9e5111c3538e37 (Brian)
- fix(helm): add --tls-hostname flag to tls-flags 7faf62a209e91bfae17522e4a63c750a3482f6f3 (fibonacci1729)
- docs(*): update tiller_ssl docs to reflect IP SAN usage. aaf1c6a35218c40d9facc7b8fe1060226cae5fc6 (fibonacci1729)
- CONTRIBUTING.md: Corrected slack channel/s in support channels 917c1a6b1169b28d0f4841d640f2cb13a5177b9f (Bj??rn Magnusson)
- docs(chore): scale svg up to 620 initial width/height c6a572a38e1f9e7d9bbd1e902137198d8eba4dbf (flynnduism)
- fix(helm): return when listing with no releases or nil 6767f3cf08b30fda5f031cda7e2d0cb9d66a7bd8 (fibonacci1729)
- chore(docs): lowercase `Kind` for clarity 5a43a80e95e9155a2880cf12119822e33734c127 (Michelle Noorali)
- docs(helm): add helm-cos to related helm plugins c551fb7ef6e51e149a77d650440f8a61c7d493d5 (roc)
- Some small typos: 69f10ca73be76e2fc2567b97f8a8bb142e38a49d (AdamDang)
- docs(chore): add svg logo a3a1034e084e6b695ce9c4f9f2d7dfd03e5703ca (flynnduism)
- Support Stdin when using downloaders of plugin 81994381c1e940d8938766293c36fa03db0026d8 (roc)
- Change permission of index.yaml d75d35da82d707614fae9626ec1a5e5cd8dc25f5 (Junya Ogasawara)
- add nil check in if statement configmap example, also note why b3c583b855f0c10bc3adb2a4f019e0e752643d83 (Jon Kalfayan)
- chore(sprig): bump sprig to 2.15.0 aae233e7629e87ed391321df8e7bc0bf66c89371 (Gage Hugo)
- Add 'name' label to created namespaces 5041d741878820c000546b5ed87d5938aa6c4996 (James Munnelly)
- add SECURITY_CONTACTS f89a5a729913bb3ebfca1218432c0acbdb84c47e (Michelle Noorali)
- Fix(helm): fix the bug of the charts not deployed after downloaded in helm install --dep-up 4d579bbbdff7d9b7c6b380d256f29a5732147d22 (rocky)
- typo fix for template test 5d2140be384eaba7ed8bb0ff1bc2fb9be6284b4d (liyongxin)
- typo fix for lengh to length 63760b29377fde5e3d2ac9f3a3df97d133161858 (liyongxin)
- Remove trailing whitespace 0ddbcc1fe9a920c1d4146b008b08579752a0567a (Dean Coakley)
- Add s390 architecture in TARGETS c19dea18b0f93a710a8cd6c33a30456bce0c61aa (Alice Frosi)
- test: add test for 'ParseIntoString' and update test for 'ParseInto' 8817f7baa714161b0c3fa7299f0e0b0160cba96a (Herbert M??hlburger)
- Typo fix: indentifies -> identifies b4cfb5f2f3efe957b1cb1b671ee33cfe18aeaf93 (ruicao)
- feat(helm): Added the ability to handle custom description to client side for install, upgrade, rollback, and delete commands. Updated the documentation according to the changes. d8c4cbb0e0e31276241d87d6386aad8d90011117 (Arash Deshmeh)
- added ability to handle custom release descriptions to tiller a32868e48b82c97dba246b5ed4003bf8966a7725 (Arash Deshmeh)
- added description field to api types UpdateReleaseRequest, InstallReleaseRequest, UninstallReleaseRequest, and RollbackReleaseRequest 8389fe1d58a35757762acb194b4ecf878dcb0849 (Arash Deshmeh)
- Increase error message specificity for name reuse ddd1f48c40e0afb7ec44ee08ca365d113b4d2d49 (mattjmcnaughton)
- Add link to cdwv/awesome-helm e4a80cdc3b12a7a31437a155bd00c7efab7130a8 (andrzejwp)
- Setting DisableCompression to true to disable unwanted httpclient tranformations 3eaa1bfd3b040cb644c7974d32047dc70037cf95 (tariq1890)
- Fix incorrect timestamp when helm package 96a85a06ed48e618c53441cdc988680eb00f8e0c (mattjmcnaughton)
- Added documentation for the tpl function 4d63e1f84f45fbd66cb70e7148a9021772b534bb (Lukas Eichler)
- Typo fix: retruns->returns 4d22b104b72cc1b6b024d047c423ba496ec05e0c (AdamDang)
- Fix test failure message e8b788dc4f3533934394e783cda94e65faf75918 (mattjmcnaughton)
- small typo fix 03d502c4313fdd516bf1ec4d5d9021accd377106 (Rob Salmond)
- Add test to make sure --set flag interprets `null` as `nil` a7362d76bc6002140a153daa4ab68b87b509af89 (Elmar Ritsch)
- Add test to make sure --set-string flag takes `null` as string 7308e766e2633bbe08f0c441e9a84a46acca38f9 (Elmar Ritsch)
- Parse booleans and null as string values with --set-string flag 272d9bc6efff112b578e48334054534290d3af95 (Elmar Ritsch)
- Documentation: add syntax highlighting to code examples & fix spelling of kube primitives 4e29c7e2fd4ced670da320c7587cd8a9e7a895e2 (radbaron)
- Fix concurrency issues with helm install 53c8e9b67ed9482edd1c03146a55b73b014134b5 (mattjmcnaughton)
- Fix inaccurate comment in `tiller` 2b04523bafa1cc96eb92b64980f7672a399e3715 (mattjmcnaughton)
- update security vulnerability email af37c6715ad2e39c2bbb542730a39194341de423 (Michelle Noorali)
- Update tiller_ssl.md 215abe9fd42571a99f42cb03737461208b418a1e (mf-lit)
- docs(helm): Added double quotes to example 8415e2716c31df14aa859a9b3e692f02c154a878 (Shubham Jain)
- adding docs for GKE for helm 2 20fe27c0dec8029760d9ade1f02eb1905d89813f (Ryan Hartje)
- make docs 93e8f0a694e18b1c1310a193c01f874648d898df (Johnny Bergstr??m)
- Add helm template --is-upgrade 43b19dccbee45dcf77a047fabb1aa051ff346d56 (Johnny Bergstr??m)
- fix charts doc: extra space in requirement condition 091dd84c71814fdeb8b67aaaf90179629f1f7f84 (Eric Zhang)
- removed --install option from --namespace. 4f36bbf2bd56dec4a7254706e9bc6d70b7185bd0 (Yeni Capote Diaz)
- Migrate 'template' to 'include' in 'helm create' to conform to Helm best practices 4fbc733ade47ed8eb35229783e8f53465e14ae27 (Alejandro Vicente Grabovetsky)
- Update capabilities.go 89c29f3ff107bd107a3a115c69e41bcf89422bc0 (AdamDang)
- Don't request Tiller version in installer 931c80edea0301a068b93a6782a8000a619dcb79 (James Meickle)
- Typo fix: mach->match 51f92b47877bcc36401dacc6b624eecff8a8931d (AdamDang)
- replace with a link to the latest releases page e52c7dbf921f8633068c3a7031a6cb6793b01d98 (Matthew Fisher)
- Suppress 'unauthenticated users' warning when --tiller-tls-verify present 4e08a023ddd71a211a774ffd2538b5701ba05ea9 (Jonathan Hall)
- fix lint warning f2abb2ae1409ef0c897bd5f7a943561b17af0b3a (Rajat Jindal)
- Support values files on remote servers with self-signed certs / client cert authentication (#4003) 32bd9d82e860427e7685df8732b16a5dfc6dfa86 (Doug Winter)
- fix(kube): run schema validation on BuildUnstructured b831efdf5853dceb0249eaf441024d115b5cb633 (Adam Reese)
- chore(docs): mv index.md to README.md 1bb34bc255b5379a9af0b832599a472ceca2d7a1 (Michelle Noorali)
- Upgrade moniker version (#4034) 26943763a14b50f516e8a0642b4aad8c6589636c (Gabriel Silva Vinha)
- feat(tiller): support CRD installation (#3982) 0699ec4248f3809b16b0e486f1aacf498c9a88c5 (Matt Butcher)
- docs(release_checklist): Adds information about new release meetings b133da06e5767312273f4c9abc0b79d578d975fe (Taylor Thomas)
- Typo fix: usa helm->use helm ddb536aa7a9287a6bf45994f4430891d00c1f529 (AdamDang)
- Typo fix: evalutes->evaluates 07bebe6bff7885cafc800c063e52bf4669ed1c93 (AdamDang)
- Refactor to be more precise a1b5b69248a7e112bc970e50cbf1e757c2530b66 (Herbert M??hlburger)
- Revert "Fix tiller deployment on RBAC clusters" 4d1a401a9fb52cfbad97c31a9d038935434b413c (Matthew Fisher)
- Revert "toYaml - Fix #3470 and #3410's trailing \n issues" f7f686f7d065218ef6df3fbb75ce6348e699a0f3 (Matthew Fisher)
- #3763 replace backslash with forward slash dcdeefbd63d04a8254d3eee0a2557f6f513005e8 (Herbert M??hlburger)
- docs(helm): update Globs examples to work correctly c67fab5934bb76ccd7ab2420a4d2fc8da4ae4af1 (Julien Bordellier)
- build helm binary in tiller image 0a3580e8d934cf8fa810f3cab2053d4612e5b7e7 (Matthew Fisher)
- remove need for type reflection d9395bcc0668429a48cdfe8ff2389bb9d00f4e0e (Matthew Fisher)
- Change tiller's Dockerfile to use USER nobody + upgrades to alpine:3.7 f0d78180d1b5b26955c70b80c4e38553b316d9ea (Julien Bordellier)
- Fix for - Downloader plugins not used when downloading new repo's index.yaml #3938 c2fa72ebcdc9a12381eec29b642374af75f5b45c (eyalbe4)
- Add quoting support in ingress to allow wildcard domain cf3ded91f2143fed6850dee70630c1049bf937ec (Julien Bordellier)
- Changed whitespacing in comments 718578036d4c13604ec557a32dfd31b5403aac31 (BarryWilliams)
-  add --col-width to `helm search` (#3949) 7a65f7479acd2ae76289b0742d5e2349c22fec18 (Marat Garafutdinov)
- fix(kube): output internal object table cefee4b749122bc38d019c2791faf79a4ab1376f (Adam Reese)
- Fix --tiller-namespace flag for plugins ed39f16ee57c094476ea61ef4983efcb501e7643 (Fabian Ruff)
- Avoid to call 'go' with empty -tags argument 6b2384f8b4dbc64e0452dfb60b987e94e8c21f00 (Julius Kammerl)
- Update install.go bcf5688e9a9da64fe5c8580c435bd248ec499e99 (AdamDang)
- Typo fix in plugins.md "that that"->"that" 28fb950588f64c4341886509a62d3b7670578e7a (AdamDang)
- fixed flag for tls ca cert option in the documentation b0eb40b2ca67146b7bb76e0197bfba0c83c5dd60 (Colin Dickson)
- feat(list): Optional output as JSON and YAML 49c3d50e4eb4b61135d58a6cb52a5cd3304e2c51 (Sean Eagan)
- fix(kube): get correct versioned object from info helper 6ffff5fea9febb3c2522a391a80cb09b838288a7 (Adam Reese)
- swallow the error when returning the default HTTP client aa2976f0cea3b80278c07c31ffc40072ff81b289 (Matthew Fisher)
- fix(kube): use correct object type in watch 31ddd707e8f832c2ac429aa09284e196f1664ec6 (Adam Reese)
- fix(pkg/strvals): evaluate "null" values 1850aeade9efe7edb960b130daa46c727ad98bab (Michelle Noorali)
- ref(pkg/plugin): create clean path for extracting plugins fed7e69c81eef1cf0686a5a73d01901c1584e169 (Michelle Noorali)
- Correct the returned message in reset_test.go fac7caf5d266c33cc95d0413e73a9a4636e48317 (AdamDang)
- Correct the returned message 9f78c33c644cbf4599c30605470b752126573471 (AdamDang)
- Revert "feat: add --set and --values options to 'helm package'" 7b8aae466761448522acbd3beb2a16ad2283013a (Matthew Fisher)
- Add App Version to the helm ls command. 0209ac32ddb8da3fb70e82855970146eec4031f9 (Derek Bassett)
- Typo fix in functions_and_pipelines.md f291fdbb43730dbb266d7aeb146c40aa978a0c66 (AdamDang)
- feat(helm): support removing multiple repositories with repo remove command 223c89e6aac873a8fe9b0228d39364f566881831 (Arash Deshmeh)

> ⚠️ **弃用警告**: `PodSecurityPolicy` 已在 Kubernetes v1.25 中正式移除。
> 请使用 [Pod Security Admission (PSA)](https://kubernetes.io/docs/concepts/security/pod-security-admission/) 替代。
> PSA 通过命名空间标签强制执行 Pod 安全标准 (Privileged / Baseline / Restricted)。

- Create PodSecurityPolicy before Pods and ServiceAccounts 7631b8a926d1cc533a94951b6a96c948081e78e7 (Sergii Manannikov)
- fix(helm): resolve linter's warning on template command unit tests 75682ed5844400e5dcd24ec7532c0fa454defd4d (Arash Deshmeh)
- Document apiVersion field 4ce35363d40fa6e8d8789121384d397748571e57 (Jesse Weinstein)
- test helm template -x with non-existent manifest fb1dd48b5a1994e8a81a9b734cb6503d024799cd (Chance Zibolski)
- Correctly use subtests in helm template tests b9adc356e8d1fe68b139a0b4c2495aa1e5fdebf8 (Chance Zibolski)
- Disambigutate chartPaths variables in helm template tests ced20f6f4ebacbc120cd7e580668e3797aa7f2a9 (Chance Zibolski)
- test helm template -x against subcharts stored as tgz's 62a58e8362b443898599f12baba8cce96719d1f5 (Chance Zibolski)
- Add working template to mariner chart for use in testing subcharts d7e71709935b2f3b93c90c85ecdbf3d247cc7ebf (Chance Zibolski)
- Update helm template -x to support children chart manifests in more cases 62f85322ef114089724835e545d8d32d9bc30e12 (Chance Zibolski)
- remove optional field from charts and templates 00afbd7e74bf8d5ef7f7f30fc5e914b4a50ac7ef (Matthew Fisher)
- Remove Mercurial build-time dependency 87cd8ce79a16e3a17d97c7668b8d021a42932006 (Manuel R??ger)
- Basic auth credentials from repo not used in install/upgrade/fetch commands #3858 fa62c476fc2156495dce4cce76780d7c166f3011 (eyalbe4)
- Fix some typo 4b09b0489b6ba1fbd413aaf599fe44e6697a1b1c (xianlubird)
- Proper none not in capitals 79ffa98ec0c05415961e1e87f1415fd47136d277 (AdamDang)
- fix(helm): refactor tiller release install unit tests using chart and install request stubs c22492ff016ea85d7b05be013ecd4458b00b02ae (Arash Deshmeh)
- Typo fix 3f5e82c83207c7444f98cc65a73bf71a0babeb1d (Erik Sundell)
- fix(helm) refactor reset command unit tests to remove duplication in test code 826781a1a3ceddaf0abeced14bee867d86c96b4e (Arash Deshmeh)
- typo fix get->Get's ee9ef91df0ea72646ae0ea23497622bfd019a944 (AdamDang)
- fix(docs): Add the missing docs 85282ab864973cbbb85d97849c37be0432df7149 (Taylor Thomas)
- fix(package): Adds missing `set-string` flag and parameter b718b1c87038d44f2519f50d2df25454aad5f926 (Taylor Thomas)
- remove unused servicePort from default ingress template 8bd42d264515d65372c6728c2762df1b6e9da7ce (Steffen Windoffer)
- Fix some typos 138de17c64e6c01468f4e17ddff0231b34d3f683 (AdamDang)
- Updated tests for PR #3837 05a1f7f46c2ce2beacbb099d06a99422376f848d (Erik Sundell)
- toYaml - Fixes #3470 - trailing \n issue 6cdf6cee56dcd90e3dca82950b15d46e6bb4587b (Erik Sundell)
- toYaml - Fixes #3410 - trailing \n issue 35132d141c54598e7dab23207acce7e6fb1dfa4a (Erik Sundell)
- Updates readme with choco install command c6faed101b4ad9c60c99f7f0fdb8edf093b73490 (Stefan Henseler)
- rename TPR to CRD f52d0f4f18003ab6392a1b768abc2bd68cf82649 (Matthew Fisher)
- Fixes typos introduced in #3540. Closes #3823 1e7915587f383a5e37f8506c9c1784bd37f43753 (Daryl Walleck)
- ref(cmd/upgrade): update reuse-values flag descrip e922a873bc30acfb1ed65031014eea6e25f604c1 (Michelle Noorali)
- ref(pkg/tiller): clarify reuseValues comment e02802ba95e17c2a21dceeb56df893c01001e5d8 (Michelle Noorali)
- fix(pkg/tiller): reuseValues combines all prev val 9266731dc45aac2aa6a726e5a887ee1078aaf35f (Michelle Noorali)
- ref(cmd/helm): show grpc error msg from prettyError 499636d70c85d7615eb27d9c58d69ef55f26fc4a (Michelle Noorali)
- Switch flag to update existing record bac4f45f94bf2856215f9e5657ee6a67d16d71fd (Stuart Leeks)
- using existing mechanism to flag for failures 9b20d91e427da1d8bc5c961a25174ccee2f4ff6c (Ryan Hartje)
- Typo fix helm->Helm 58ac6023653632afe1bb13d01ed5516fff3737f5 (AdamDang)
- Fix #3822 1d3ae54185e369477f42fb15d3a97f80c014c09c (Zack Williams)
- Removes unnecessary if block 458ba8ce37d5068ec03384257d4cbeed79e4906d (Gijs Kunze)
- fix(helm) refactor release_testing unit tests to utilize runReleaseCases ea7c3fefc8cd79ac497965a7fca2aef390f82287 (Arash Deshmeh)
- closes #3795 09e0ab18090fed1c0b2c91fa401154fcd8e93a0f (ReSearchITEng)
- Update rbac.md 026e6b55e5510e0e521760829e041aeac26b22be (AdamDang)
- Fix tiller deployment on RBAC clusters 1e03f1bce5eaab384c8ddabbf38b8f566d1c1d14 (John Koleszar)
- return a non 0 exit code when lint fails due to missing Chart.yaml 343acc5a60c8d63272f6ad9511473218e6b7a6ea (Ryan Hartje)
- Update files.go 6a59683c01f053ab6490a82f64985aeea9894358 (cameronconradt)
- The default defined role does not allow Tiller to launch Kubernetes jobs. 00376d4d1f7ea78e81cd2a9c1905c3d606402b2c (Derek Bassett)
- Update CONTRIBUTING.md dfc6d74637ba5fc2c9b9e95fdb32639449f1d726 (fqsghostcloud)
- (fix) Handle caFile alone being set for repos 332dc83c46b1cb135bc42b7afa1c9b50629e0e15 (Morgan Parry)
- fix `helm get manifest` context deadline exceeded error 87c64e7987f348c0a99a4f6e12d6ec187723b7cc (Matthew Fisher)
- fix output leak from tiller release install test a43ddcc191f851e343075142a3b0f9a456e95be0 (Arash Deshmeh)
- Fixed SIGSEGV when running helm create with -p and no values.yaml file 99da9fb54817a00e0138a08ce0b0643b923e5b6e (Ali Rizwan)
- feat: add --set and --values options to 'helm package' a930eb7ff4dcba13831174496dda2445e81ebeaa (Arash Deshmeh)
- update docstrings on Deployment, Service, and Secret 8bb4984f94fb9cbc90daa95276afd9bc781c833c (ryane)
- don't wrap helm init -o/--dry-run output in List 61e88cbc412a0a77e4eba3dc03b888cc18ea647e (ryane)
- fix(helm): add service, secret manifests in `init -o` af80059b45804a3c9eb51b98f2bb8eded091d677 (ryane)
- fix(helm): fix importValues warnings from disabled charts fbe80437a499e6b5c7301588522b03dde593ce10 (Justin Scott)


<!-- risk-assessed -->
