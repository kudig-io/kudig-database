---
title: gatekeeper v3.12 Release Notes
description: gatekeeper v3.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- helm
- containerd
- docker
- opa
- crd
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- gatekeeper v3.12 Release Notes 是什么
- 如何 gatekeeper v3.12 Release Notes
trigger_keywords:
- gatekeeper
- v3.12
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

# gatekeeper v3.12 Release Notes

Source: [v3.12.0](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.12.0)

This stable release has no other functional changes from [v3.12.0-rc.0](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.12.0-rc.0).

## Notable changes 
- 📦 New [`AssignImage`](https://open-policy-agent.github.io/gatekeeper/website/docs/mutation#assignimage) mutator [#2429](https://github.com/open-policy-agent/gatekeeper/pull/2429)
- 📢 [Emit events](https://open-policy-agent.github.io/gatekeeper/website/docs/customize-startup#alpha-emit-admission-and-audit-events) in the involved objects namespace [#2360](https://github.com/open-policy-agent/gatekeeper/pull/2360)
- 🥳 Update to Open Policy Agent (OPA) [v0.49.2](https://github.com/open-policy-agent/opa/releases/tag/v0.49.2) [#2611](https://github.com/open-policy-agent/gatekeeper/pull/2611)
- 🚂 Added multi-engine support to allow integration with Kubernetes CEL `ValidatingAdmissionPolicy` in the future [#2616](https://github.com/open-policy-agent/gatekeeper/pull/2616)
- 👏 Enable exempt namespace suffix with [`--exempt-namespace-suffix`](https://open-policy-agent.github.io/gatekeeper/website/docs/exempt-namespaces/#exempting-namespaces-from-the-gatekeeper-admission-webhook-using---exempt-namespace-flag) flag [#2636](https://github.com/open-policy-agent/gatekeeper/pull/2636)

## Features
- Allow writing logs to custom file (#2473) [#2473](https://github.com/open-policy-agent/gatekeeper/pull/2473) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/bb11f3e08cdbc4a14792ce496cc6a4a224dec712))
- More verbose logging for audit (#2503) [#2503](https://github.com/open-policy-agent/gatekeeper/pull/2503) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/beb2432ce892554bf1eaddd6f543303cf6d34da2))
- **helm**: Add a network policy for the controller manager (#2514) [#2514](https://github.com/open-policy-agent/gatekeeper/pull/2514) ([Kyle Michel](https://github.com/open-policy-agent/gatekeeper/commit/ac8612db506c727a1331b195ea7dbb41c19aaa3d))
- enforce kind on admission review (#2512) [#2512](https://github.com/open-policy-agent/gatekeeper/pull/2512) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/eb5256d59147e4a0b9952d0b2bd2af7fb4b7b888))
- add the errorlint check for golangci-lint (#2519) [#2519](https://github.com/open-policy-agent/gatekeeper/pull/2519) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/73611cca9d208e4ecad2b4e21534da1349b2a68d))
- implement AssignImage mutator (#2429) [#2429](https://github.com/open-policy-agent/gatekeeper/pull/2429) ([Davis Haba](https://github.com/open-policy-agent/gatekeeper/commit/7824f689cea63b49b55e2374988f43143327d3fd))
- introduce `gci` to unify the order of package import (#2545) [#2545](https://github.com/open-policy-agent/gatekeeper/pull/2545) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/4a27fbf69f34dceeacd27a78a5302a29ee63f116))
- add unconvert check for golang-lint (#2554) [#2554](https://github.com/open-policy-agent/gatekeeper/pull/2554) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/05638fd76296d376f02003a64af3915183149bc4))
- Emit events in the involved objects namespace (#2360) [#2360](https://github.com/open-policy-agent/gatekeeper/pull/2360) ([Craig Trought](https://github.com/open-policy-agent/gatekeeper/commit/48be4ab8098b35ad6cf60c286b70b8ad5eee171f))
- add support for exempt namespace suffix (#2636) [#2636](https://github.com/open-policy-agent/gatekeeper/pull/2636) ([Janusz Marcinkiewicz](https://github.com/open-policy-agent/gatekeeper/commit/8b7a86131d27582b371fbade0cce8a9f71492a1b))

## Bug Fixes
- cutpath for ../ paths (#2498) [#2498](https://github.com/open-policy-agent/gatekeeper/pull/2498) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/923a183112ddc9f9a175881b774ba590581e7986))
- when docker build in arm or other not amd64 env. (#2492) [#2492](https://github.com/open-policy-agent/gatekeeper/pull/2492) ([yanggang](https://github.com/open-policy-agent/gatekeeper/commit/85f543c7a41b403cc0617d6047f2568f9078bc45))
- high-risk vulnerabilities caused by low version of kubebuilder and yq (#2505) [#2505](https://github.com/open-policy-agent/gatekeeper/pull/2505) ([fsl](https://github.com/open-policy-agent/gatekeeper/commit/149fb90ab145980c02b36f8ceeb2a8927386ff04))
- syntax errors in the document (#2520) [#2520](https://github.com/open-policy-agent/gatekeeper/pull/2520) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/996d61b301dc86a585261a2c93ca756e3b433144))
- updating url in doc config (#2549) [#2549](https://github.com/open-policy-agent/gatekeeper/pull/2549) ([Jaydipkumar Arvindbhai Gabani](https://github.com/open-policy-agent/gatekeeper/commit/c1b783b2a55441c0e1746bbcc89366a128619b79))
- add --operation=mutation-controller flag (#2542) [#2542](https://github.com/open-policy-agent/gatekeeper/pull/2542) ([Davis Haba](https://github.com/open-policy-agent/gatekeeper/commit/5ab923ebd210850730e13c782a3976ac4092f6bc))
- add vendor manifests back (#2558) [#2558](https://github.com/open-policy-agent/gatekeeper/pull/2558) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/947676ed8ebf09502209174bf7ab357b51454758))
- add missing namespace to static Helm templates (#2593) [#2593](https://github.com/open-policy-agent/gatekeeper/pull/2593) ([Devon Crouse](https://github.com/open-policy-agent/gatekeeper/commit/b027979fb942febe8dc7fbd521ff0f27616470fe))
- handle empty spec for modifyset (#2585) [#2585](https://github.com/open-policy-agent/gatekeeper/pull/2585) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/4ed46632972828380ee827ed73e131389c725a38))
- piping input in gator (#2589) [#2589](https://github.com/open-policy-agent/gatekeeper/pull/2589) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/ab0c959f7fe6e6c0f67c70c83dd3fcd8488cb5fb))
- generate mock name for expanded resources (#2529) [#2529](https://github.com/open-policy-agent/gatekeeper/pull/2529) ([Davis Haba](https://github.com/open-policy-agent/gatekeeper/commit/f4d2d0f869e225997d456ff75ea7b586859daa8f))
- Allow to change WebhookConfiguration name and change preInstall crd image (#2563) [#2563](https://github.com/open-policy-agent/gatekeeper/pull/2563) ([Jiri Tyr](https://github.com/open-policy-agent/gatekeeper/commit/b057192a6ee858db8a7062ea466d11cf778394f8))
- support source field in Constraints (#2552) [#2552](https://github.com/open-policy-agent/gatekeeper/pull/2552) ([Davis Haba](https://github.com/open-policy-agent/gatekeeper/commit/df9a9d9f2d32233e7f916506da4a82b55da557f0))
- **helm**: switch to curl as ENTRYPOINT for probeWebhook (#2632) [#2632](https://github.com/open-policy-agent/gatekeeper/pull/2632) ([thomasmckay](https://github.com/open-policy-agent/gatekeeper/commit/7b08d2321d2ad4220d98658ef2d405404e3534b8))
- index readiness trackers by GK (not GVK) (#2635) [#2635](https://github.com/open-policy-agent/gatekeeper/pull/2635) ([Davis Haba](https://github.com/open-policy-agent/gatekeeper/commit/73d6a17e60e6ef950c35bbe6081d513d946bcb76))

## Documentation
- generate 3.11 docs (#2501) [#2501](https://github.com/open-policy-agent/gatekeeper/pull/2501) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/c61db246254496a855c568a3fca4536ab03ec0f0))
- fix syntax errors (#2513) [#2513](https://github.com/open-policy-agent/gatekeeper/pull/2513) ([Nico Wang](https://github.com/open-policy-agent/gatekeeper/commit/80d63467ab8e996b34600da4a6c7178a5b954b31))
- Fix typo in website docs (#2528) [#2528](https://github.com/open-policy-agent/gatekeeper/pull/2528) ([triangularcover](https://github.com/open-policy-agent/gatekeeper/commit/3a1aae42dd83e1698e9292b3bc6b12583f5fa9af))
- fix example code snippet for docs (#2539) [#2539](https://github.com/open-policy-agent/gatekeeper/pull/2539) ([triangularcover](https://github.com/open-policy-agent/gatekeeper/commit/92a7573e4cdc773cc87d1c25d4d175d463e4e005))
- fix expansion yaml example (#2551) [#2551](https://github.com/open-policy-agent/gatekeeper/pull/2551) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/f3824f476ad6e8529d4760569d321b6569877dc7))
- update k8s.gcr.io to registry.k8s.io (#2588) [#2588](https://github.com/open-policy-agent/gatekeeper/pull/2588) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/476269f47408e4589915f7e6b08e6998a3de38dc))
- Add background information on mutation (#2387) [#2387](https://github.com/open-policy-agent/gatekeeper/pull/2387) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/bf7b466b7f65745f7f5ecb15b7ce352c8e0bd479))
- Add mutation background to 3.11 (#2590) [#2590](https://github.com/open-policy-agent/gatekeeper/pull/2590) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/6de3e59d17c08457159badef881b7218aa82bea7))
- **helm**: Fix helm chart documentation for setting audit and webhook selectors and affinity (#2617) [#2617](https://github.com/open-policy-agent/gatekeeper/pull/2617) ([Max Falk](https://github.com/open-policy-agent/gatekeeper/commit/ea255fa42d622d5406808a24c7033e29968ef834))

## Code Refactoring
- use Go 1.18 buildinfo (#2541) [#2541](https://github.com/open-policy-agent/gatekeeper/pull/2541) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/588870679e12cd2f373960c4f6c3653484161cd6))

## Tests
- add some audit tests (#2489) [#2489](https://github.com/open-policy-agent/gatekeeper/pull/2489) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/4f8b630bf05c6ca3e7a371a23202f015454d1345))

## Continuous Integration
- Releasing benchmarks and benchmarking PR (#2432) [#2432](https://github.com/open-policy-agent/gatekeeper/pull/2432) ([Jaydipkumar Arvindbhai Gabani](https://github.com/open-policy-agent/gatekeeper/commit/8dc9cc836149831a65e83d1cd4b6351495272121))
- add license lint wf for cncf approved licenses (#2461) [#2461](https://github.com/open-policy-agent/gatekeeper/pull/2461) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/7786db98e41b3af06a9f0692b2827acfe6ddb7fc))
- remove kubebuilder dependency (#2524) [#2524](https://github.com/open-policy-agent/gatekeeper/pull/2524) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/f33c36743c3173144ee557fe79c7c16822c577f2))
- **helm**: remove unused kustomize step when upgrading (#2564) [#2564](https://github.com/open-policy-agent/gatekeeper/pull/2564) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/f0673f57696f19227f981bf252bfc43716a598ff))
- pin golang image to unblock ci (#2573) [#2573](https://github.com/open-policy-agent/gatekeeper/pull/2573) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/6c10e270c2b83fc3991a42f64ca904737bc43f3e))
- move k8s.gcr.io to registry.k8s.io (#2572) [#2572](https://github.com/open-policy-agent/gatekeeper/pull/2572) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/6b3dfade37054d34f16d5bf153d6db045b1742da))
- remove k8s 1.23 from matrix (#2609) [#2609](https://github.com/open-policy-agent/gatekeeper/pull/2609) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/43acc5bacb3103779c1968ce32050ece8f4a817b))
- bump ci to golang 1.20 (#2597) [#2597](https://github.com/open-policy-agent/gatekeeper/pull/2597) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/852d1f4dff24e78b72406aa1f968f9dab5d3971b))
- generate sbom and provenance  (#2540) [#2540](https://github.com/open-policy-agent/gatekeeper/pull/2540) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/30eaf1b2202524f184e9c8aa6a6cc3979571cff0))

## Chores
- modify all error contrast judgments by errors (#2491) [#2491](https://github.com/open-policy-agent/gatekeeper/pull/2491) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/a21f595b6c120e882a660e3953ff32ed33e943d4))
- bump actions/checkout from 3.2.0 to 3.3.0 (#2499) [#2499](https://github.com/open-policy-agent/gatekeeper/pull/2499) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/7bdb64f6637a3691de70dbde873d98a74b082a20))
- bump peaceiris/actions-gh-pages from 3.9.0 to 3.9.1 (#2500) [#2500](https://github.com/open-policy-agent/gatekeeper/pull/2500) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/359c803fc60c442af22c3fa89eb6993f604d8746))
- bump github/codeql-action from 2.1.37 to 2.1.38 (#2517) [#2517](https://github.com/open-policy-agent/gatekeeper/pull/2517) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/b34cd7840321d13355a0cd374441b45e0d8dad38))
- bump peaceiris/actions-gh-pages from 3.9.1 to 3.9.2 (#2521) [#2521](https://github.com/open-policy-agent/gatekeeper/pull/2521) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/ebba1b6a5f2192a3511881d6a91789f0692e0174))
- bump github/codeql-action from 2.1.38 to 2.1.39 (#2525) [#2525](https://github.com/open-policy-agent/gatekeeper/pull/2525) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/ec3d4d9cb1f57976aa3091e60a3dfe6de7b76177))
- update docs (#2526) [#2526](https://github.com/open-policy-agent/gatekeeper/pull/2526) ([Shawn Warren](https://github.com/open-policy-agent/gatekeeper/commit/3c0d6900a1e45edeefee16435dfbbb2d0a1496cb))
- bump k8s.io/client-go from 0.24.9 to 0.24.10 (#2533) [#2533](https://github.com/open-policy-agent/gatekeeper/pull/2533) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/c3489f05f65c1f3801cbbf9103b30c85b756a1f1))
- Upgrade to k8s v0.26.1 and controller-runtime fork (#2530) [#2530](https://github.com/open-policy-agent/gatekeeper/pull/2530) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/8b426fb55da22abc0fe9bc925a3ca1ed08df50fe))
- bump github.com/onsi/gomega from 1.24.1 to 1.24.2 (#2536) [#2536](https://github.com/open-policy-agent/gatekeeper/pull/2536) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/f4435e260da2dbd65b808b48d1afa82841e92980))
- bump ua-parser-js from 0.7.31 to 0.7.33 in /website (#2535) [#2535](https://github.com/open-policy-agent/gatekeeper/pull/2535) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/64ed0328faa062b7a54a2f4071bc462991a6c73c))
- bump github/codeql-action from 2.1.39 to 2.2.1 (#2543) [#2543](https://github.com/open-policy-agent/gatekeeper/pull/2543) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/819c1909dba31aeb9bff1b532f70b667b614b639))
- bump @docusaurus/core from 2.1.0 to 2.3.0 in /website (#2547) [#2547](https://github.com/open-policy-agent/gatekeeper/pull/2547) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/1e977d623df930bcbaab748f230323eb1c6c2c58))
- bump @docusaurus/preset-classic from 2.1.0 to 2.3.0 in /website (#2546) [#2546](https://github.com/open-policy-agent/gatekeeper/pull/2546) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/16d7929f0856ca8dc05fcc5cf64b376ae6e8bb95))
- the linter `structcheck` `varcheck` and `deadcode` are deprecated (since v1.49.0) (#2550) [#2550](https://github.com/open-policy-agent/gatekeeper/pull/2550) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/431db177fe97c6ab90c63b7f3bed3179971a2afc))
- modify the typecheck as prompted (#2553) [#2553](https://github.com/open-policy-agent/gatekeeper/pull/2553) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/0f2c8ea4ed40674d837fd59fb41268428fc5523b))
- bump github/codeql-action from 2.2.1 to 2.2.4 (#2581) [#2581](https://github.com/open-policy-agent/gatekeeper/pull/2581) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/f52732cc51f7f6ec836ee17b6f17e7ccf80d9563))
- bump @docusaurus/core from 2.3.0 to 2.3.1 in /website (#2566) [#2566](https://github.com/open-policy-agent/gatekeeper/pull/2566) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/dc1c49ac352abab2356db3bc76658ec7941d702e))
- bump http-cache-semantics from 4.1.0 to 4.1.1 in /website (#2565) [#2565](https://github.com/open-policy-agent/gatekeeper/pull/2565) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/da977885d22d5a680051bcdaffd0ba47cb007083))
- bump @docusaurus/preset-classic from 2.3.0 to 2.3.1 in /website (#2567) [#2567](https://github.com/open-policy-agent/gatekeeper/pull/2567) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/a97baa7f71eac6e28c74632ea11e59c18a292aae))
- bump sigs.k8s.io/controller-runtime from 0.14.1 to 0.14.4 (#2568) [#2568](https://github.com/open-policy-agent/gatekeeper/pull/2568) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/d8f501a52a5f557e3c90aea07af22111839ad91c))
- clean up repeated package import (#2579) [#2579](https://github.com/open-policy-agent/gatekeeper/pull/2579) ([Fish-pro](https://github.com/open-policy-agent/gatekeeper/commit/0761889c0c50bf0b51061cee1699014401b900c6))
- bump github.com/containerd/containerd from 1.6.12 to 1.6.18 (#2586) [#2586](https://github.com/open-policy-agent/gatekeeper/pull/2586) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/0ba3c15bcf67e94515160d7a1ecbfb37f9c47d12))
- bump golang.org/x/net from 0.4.0 to 0.7.0 (#2594) [#2594](https://github.com/open-policy-agent/gatekeeper/pull/2594) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/8170c5f7dad05cc49d86f8a7dcebaf4a6600adc0))
- bump github.com/stretchr/testify from 1.8.1 to 1.8.2 (#2604) [#2604](https://github.com/open-policy-agent/gatekeeper/pull/2604) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/aea4a496d5653c40859eae08995bf167ea28358a))
- bump dns-packet from 5.3.1 to 5.4.0 in /website (#2610) [#2610](https://github.com/open-policy-agent/gatekeeper/pull/2610) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/9db57b014f80c9c22a65d031397ae95b66840dc9))
- bump github/codeql-action from 2.2.4 to 2.2.5 (#2603) [#2603](https://github.com/open-policy-agent/gatekeeper/pull/2603) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/8410a4476597cae37a23a30c26232a067053dbac))
- update frameworks to 89ae90 (#2611) [#2611](https://github.com/open-policy-agent/gatekeeper/pull/2611) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/5cfb5076065a8211793128bc26d4f814627b06d2))
- bump k8s.io/apiextensions-apiserver from 0.26.1 to 0.26.2 (#2615) [#2615](https://github.com/open-policy-agent/gatekeeper/pull/2615) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/2c2bc3b8e2bd4154be8b5e9bdcad5dedfede3132))
- Upgrade CF for multi-engine (#2616) [#2616](https://github.com/open-policy-agent/gatekeeper/pull/2616) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/2f4e4f3a978b158cd324a2803006b86b422bdb36))
- bump github/codeql-action from 2.2.5 to 2.2.6 (#2619) [#2619](https://github.com/open-policy-agent/gatekeeper/pull/2619) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/1a3fee51fdc7b38d10246ccc7edc7560caa100e7))
- bump @sideway/formula from 3.0.0 to 3.0.1 in /website (#2621) [#2621](https://github.com/open-policy-agent/gatekeeper/pull/2621) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/29bb8bebcf1a9ac2920fe42b9d84af2d9cfbcb34))
- bump github.com/onsi/gomega from 1.27.2 to 1.27.4 (#2623) [#2623](https://github.com/open-policy-agent/gatekeeper/pull/2623) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/a03bc8433f51d2ead5084f04dfa435e984f565a2))
- bump kubectl (#2624) [#2624](https://github.com/open-policy-agent/gatekeeper/pull/2624) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/ada84ab1828a681140f9b25d6c2b49512c3a4afd))
- bump go.uber.org/automaxprocs from 1.5.1 to 1.5.2 (#2627) [#2627](https://github.com/open-policy-agent/gatekeeper/pull/2627) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/8e0371916d868eb175a3a999fb69bd4e988c4a4d))
- bump k8s.io/apiextensions-apiserver from 0.26.2 to 0.26.3 (#2630) [#2630](https://github.com/open-policy-agent/gatekeeper/pull/2630) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/3bc8b22085a0eb044af231dfdfa0683071a04ef5))
- bump actions/setup-go from 3 to 4 (#2625) [#2625](https://github.com/open-policy-agent/gatekeeper/pull/2625) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/5867400c66c272d60b24c8cad506c677a2799ba6))
- bump github/codeql-action from 2.2.6 to 2.2.8 (#2637) [#2637](https://github.com/open-policy-agent/gatekeeper/pull/2637) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/b4638d328b288d8ec44117c8658b80867ba04815))
- Prepare v3.12.0-rc.0 release (#2647) [#2647](https://github.com/open-policy-agent/gatekeeper/pull/2647) ([github-actions[bot]](https://github.com/open-policy-agent/gatekeeper/commit/3ff2c54dd8489d6d5c202b114f566b01a7032db4))

## New Contributors
* @congiv made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2389
* @Fish-pro made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2483
* @yanggangtony made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2492
* @fengshunli made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2505
* @wangzihao05 made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2513
* @krmichelos made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2514
* @triangularcover made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2528
* @swarren83 made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2526
* @devoncrouse made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2593
* @ctrought made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2360
* @gmdfalk made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2617
* @VirrageS made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2636

**Full Changelog**: https://github.com/open-policy-agent/gatekeeper/compare/v3.11.0...v3.12.0