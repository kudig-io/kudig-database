---
title: gatekeeper v3.3 Release Notes
description: gatekeeper v3.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
- helm
- crd
- webhook
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gatekeeper v3.3 Release Notes 是什么
- 如何 gatekeeper v3.3 Release Notes
trigger_keywords:
- gatekeeper
- v3.3
- Release
- Notes
- release
- notes
---

# gatekeeper v3.3 Release Notes

Source: [v3.3.0](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.3.0)

This stable release includes bug fixes and new features.

## Features 🌈
- [[30d0470](https://github.com/open-policy-agent/gatekeeper/commit/30d0470d2988141c195c53aa55bf04b701eb8bac)]: Helm chart: make namespace creation optional (#981) (Paavo Pokkinen) [#981](https://github.com/open-policy-agent/gatekeeper/pull/981)
- [[13edcf9](https://github.com/open-policy-agent/gatekeeper/commit/13edcf98a6db7872709efbb6f1c4bd315591835d)]: Adding modifiable priority class to controller-manager and audit deployment (#1008) (Borbély Botond) [#1008](https://github.com/open-policy-agent/gatekeeper/pull/1008)
- [[1fc4ad1](https://github.com/open-policy-agent/gatekeeper/commit/1fc4ad1ddf5089cec55decac513fbdff557572c8)]: Add the readiness-retries flag, allowing the user to configure how many (#1014) (Julian Katz) [#1014](https://github.com/open-policy-agent/gatekeeper/pull/1014)
- [[5f9e98a](https://github.com/open-policy-agent/gatekeeper/commit/5f9e98a3dc8676031ec3fc0f6ff2e7d32455a1ec)]: Check that the Config resource is named 'config' (#1057) (Julian Katz) [#1057](https://github.com/open-policy-agent/gatekeeper/pull/1057)
- [[49f2f97](https://github.com/open-policy-agent/gatekeeper/commit/49f2f97a75c5e991424ba367271193632c7f1fbc)]: Allow different settings for audit vs controller (#1006) (Stijn De Haes) [#1006](https://github.com/open-policy-agent/gatekeeper/pull/1006)
- [[7bb367a](https://github.com/open-policy-agent/gatekeeper/commit/7bb367a5f1dc2cd14d5a1b43a453cd959525385c)]: Enable readOnlyRootFilesystem in Helm chart (#1048) (James Alseth) [#1048](https://github.com/open-policy-agent/gatekeeper/pull/1048)
- [[e68a39e](https://github.com/open-policy-agent/gatekeeper/commit/e68a39e65f14e8d60d721292078be8194ab6fe7c)]: Parametrize delete operations and timeout for webhook in helm chart (#1051) (Jonny) [#1051](https://github.com/open-policy-agent/gatekeeper/pull/1051)
- [[764cb0a](https://github.com/open-policy-agent/gatekeeper/commit/764cb0a94b76525a4621ffcd8b9942b62cbd93ac)]: Add validation webhook maximum worker threads (#1021) (Max Smythe) [#1021](https://github.com/open-policy-agent/gatekeeper/pull/1021)
- [[46767a1](https://github.com/open-policy-agent/gatekeeper/commit/46767a11c7078481f0e598b9d08258eef0ac7d4c)]: Allow a tracker with observations but without expectations to be (#1062) (Julian Katz) [#1062](https://github.com/open-policy-agent/gatekeeper/pull/1062)

## Bug Fixes 🐞
- [[dfa7551](https://github.com/open-policy-agent/gatekeeper/commit/dfa755147896a2a085a69cbcdfc751380a9dadd4)]: Fix for selfLink removal (#1007) (Sertaç Özercan) [#1007](https://github.com/open-policy-agent/gatekeeper/pull/1007)
- [[13bbfd8](https://github.com/open-policy-agent/gatekeeper/commit/13bbfd894987e5809790dd052be5ad3ae02e6b5c)]: Fix excluding namespaces from config namespace exclusion (#975) (Sertaç Özercan) [#975](https://github.com/open-policy-agent/gatekeeper/pull/975)

## Experimental mutation changes 🚧
- Implement CRD validation. (#978) [#978](https://github.com/open-policy-agent/gatekeeper/pull/978) ([Federico Paolinelli](https://github.com/open-policy-agent/gatekeeper/commit/f46bfb2e0f47ab4d731104804e017cb782071d51))
- [[394a93c](https://github.com/open-policy-agent/gatekeeper/commit/394a93cf661792d1aaaf1abdf6f10652fd7de804)]: Add mutation cache to webhook and controller (#945) (Marcin Mirecki) [#945](https://github.com/open-policy-agent/gatekeeper/pull/945)
- [[d6c5389](https://github.com/open-policy-agent/gatekeeper/commit/d6c53895216f257d3f747be61cfbde80c072de67)]: Write Mutation function for Assing and AssignMetadata (#937) (Marcin Mirecki) [#937](https://github.com/open-policy-agent/gatekeeper/pull/937)
- [[321dd44](https://github.com/open-policy-agent/gatekeeper/commit/321dd44ee61e78bb1e3f27f6d920b325a2660634)]: Common stats reporting code for validation and mutation (#976) (Marcin Mirecki) [#976](https://github.com/open-policy-agent/gatekeeper/pull/976)
- [[0f20ba0](https://github.com/open-policy-agent/gatekeeper/commit/0f20ba0f7f53d1411b47703980d996ac917a4292)]: Update TestAssignMetadataToMutator to check mutator and path (#1016) (Marcin Mirecki) [#1016](https://github.com/open-policy-agent/gatekeeper/pull/1016)
- [[1459e2f](https://github.com/open-policy-agent/gatekeeper/commit/1459e2f8eae495a0a34c529da794b3f1dda486d4)]: Use ID only for cache deletion. (#1020) (Federico Paolinelli) [#1020](https://github.com/open-policy-agent/gatekeeper/pull/1020)
- [[d33f09a](https://github.com/open-policy-agent/gatekeeper/commit/d33f09af95eed12d99f9bd0334ad0b86292eb4fe)]: Add assign controller (#1019) (Rita Zhang) [#1019](https://github.com/open-policy-agent/gatekeeper/pull/1019)
- [[66288bc](https://github.com/open-policy-agent/gatekeeper/commit/66288bc0a19aeeaaeff78376981f5f59cce793d5)]: AssignMetadata Controller should end processing on resource parsing error (#989) (Marcin Mirecki) [#989](https://github.com/open-policy-agent/gatekeeper/pull/989)
- [[a5309a6](https://github.com/open-policy-agent/gatekeeper/commit/a5309a68c13fac2d55dbfa67ab162db4634f802a)]: Connect mutation webhook with mutation system (#1015) (Marcin Mirecki) [#1015](https://github.com/open-policy-agent/gatekeeper/pull/1015)
- [[a128317](https://github.com/open-policy-agent/gatekeeper/commit/a128317aa340a2ff74da31d9f4e5136c99778a8d)]: Update assignmetadata controller pkg (#1025) (Rita Zhang) [#1025](https://github.com/open-policy-agent/gatekeeper/pull/1025)
- [[a4c6ebb](https://github.com/open-policy-agent/gatekeeper/commit/a4c6ebbcaa16ffcb36df6267881c0e6791d21482)]: Mutation webhook should not return any patches when nothing was mutated (#1026) (Marcin Mirecki) [#1026](https://github.com/open-policy-agent/gatekeeper/pull/1026)
- [[e0a1ae2](https://github.com/open-policy-agent/gatekeeper/commit/e0a1ae2b12510c7f4b0204462f6a4caf2a28047c)]: Fix mutator removal (#1036) (Marcin Mirecki) [#1036](https://github.com/open-policy-agent/gatekeeper/pull/1036)
- [[1eed2a3](https://github.com/open-policy-agent/gatekeeper/commit/1eed2a330c9e5e9add9263dc9882ae76d899a0f0)]: Error when oscilating mutation is detected (#1030) (Marcin Mirecki) [#1030](https://github.com/open-policy-agent/gatekeeper/pull/1030)
- [[106494f](https://github.com/open-policy-agent/gatekeeper/commit/106494ff2b9a121137ba13b199b806f1927b10b6)]: Update comment `Mutate` function (#1040) (Max Smythe) [#1040](https://github.com/open-policy-agent/gatekeeper/pull/1040)
- [[6ceb3f4](https://github.com/open-policy-agent/gatekeeper/commit/6ceb3f486a0a4b4887526e982762e6857829843a)]: Add schema DB (#979) (Max Smythe) [#979](https://github.com/open-policy-agent/gatekeeper/pull/979)
- [[5610c59](https://github.com/open-policy-agent/gatekeeper/commit/5610c59c47c8839bf52b30d9e4e36f081a9ff2d1)]: Fix assign mutation panic (#1054) (Rita Zhang) [#1054](https://github.com/open-policy-agent/gatekeeper/pull/1054)
- [[6243e45](https://github.com/open-policy-agent/gatekeeper/commit/6243e45a73f10bd67f0890c42e6f2cdb6ae6ce20)]: New process excluder for mutation (#970) (Marcin Mirecki) [#970](https://github.com/open-policy-agent/gatekeeper/pull/970)
