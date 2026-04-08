# gatekeeper v3.10 Release Notes

Source: [v3.10.0](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.10.0)

## Notable changes 
- If you are using Kubernetes v1.25 or later, this release includes removal of [Pod Security Policies](https://kubernetes.io/docs/concepts/security/pod-security-policy/) and migration to [Pod Security Admission](https://kubernetes.io/docs/concepts/security/pod-security-admission/) 🔐
- [Mutation](https://open-policy-agent.github.io/gatekeeper/website/docs/mutation) is promoted to _stable_ 🦠
- Introducing [Validation of Workload Resources](https://open-policy-agent.github.io/gatekeeper/website/docs/workload-resources/) as _alpha_ 🚀
- Performance improvements 🏃

## Features
- Promote mutation to v1 (#2305) [#2305](https://github.com/open-policy-agent/gatekeeper/pull/2305) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/35b9cbd0049d7c586d25acad8de6d6fd70128ed1))
- Expose options to allow injection of external certificates (#2249) [#2249](https://github.com/open-policy-agent/gatekeeper/pull/2249) ([Ethan Range](https://github.com/open-policy-agent/gatekeeper/commit/6f66057c57c0378572b5d79fa1ddab46525ad2ea))
- Expanding generator resources (#2062) [#2062](https://github.com/open-policy-agent/gatekeeper/pull/2062) ([davis-haba](https://github.com/open-policy-agent/gatekeeper/commit/afc2e486b34651aabd7fac585ef7d51123afd149))
- Return violating resource in pkg/gator/test.Test (#2198) [#2198](https://github.com/open-policy-agent/gatekeeper/pull/2198) ([Julian Katz](https://github.com/open-policy-agent/gatekeeper/commit/ef443f07bb0fbf6793da4514bbdc49bab1a6f13c))
- Add controllerManager tlsMinVersion option to values (#2289) [#2289](https://github.com/open-policy-agent/gatekeeper/pull/2289) ([Grace Do](https://github.com/open-policy-agent/gatekeeper/commit/3fde9bdf4bfbfb4089c39d0fd53f8b1e7126e91a))
- Add metric reporting to ExpansionTemplate controller (#2276) [#2276](https://github.com/open-policy-agent/gatekeeper/pull/2276) ([davis-haba](https://github.com/open-policy-agent/gatekeeper/commit/099d71d4b77f1b640d36976a967695cd0588acdd))
- enforcement action override for ExpansionTemplates (#2277) [#2277](https://github.com/open-policy-agent/gatekeeper/pull/2277) ([davis-haba](https://github.com/open-policy-agent/gatekeeper/commit/512d97eb07f115cfcfa8a5877886bdb5bf92d704))
- **helm**: add topologySpread to controller (#2206) [#2206](https://github.com/open-policy-agent/gatekeeper/pull/2206) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/c6be42fcb602add348e62e210866d54bf75ebabf))
- **helm**: unify and extend hook job pod labels (#2205) [#2205](https://github.com/open-policy-agent/gatekeeper/pull/2205) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/60912f6eef4d020da650fc744a9f1a33de1bf2d2))
- **helm**: add options for hook jobs (#2202) [#2202](https://github.com/open-policy-agent/gatekeeper/pull/2202) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/9281b53998c820cf97af3aae9854b2556502eb64))
- **helm**: Allow configuration of probe timeouts in Helm Chart (#2220) [#2220](https://github.com/open-policy-agent/gatekeeper/pull/2220) ([Ethan Range](https://github.com/open-policy-agent/gatekeeper/commit/b6c369b3905b0980c6a848aa5d1279b47436dd05))
- **helm**: Allow setting annotations for mutating and validating webhook configurations (#2231) [#2231](https://github.com/open-policy-agent/gatekeeper/pull/2231) ([Ethan Range](https://github.com/open-policy-agent/gatekeeper/commit/8f6d95a601c914279fb9510628e7b18e1834c3a0))
- add audit_last_run_end_time metric (#2235) [#2235](https://github.com/open-policy-agent/gatekeeper/pull/2235) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/39ca02e8490cf17b5834e98ae4540afc93cd2ea3))
- Add --host as a command line flag (#2227) [#2227](https://github.com/open-policy-agent/gatekeeper/pull/2227) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/7d7163961b062844c88c591a56f140aab04d4368))
- remove PSP and migrate to PSA (#2174) [#2174](https://github.com/open-policy-agent/gatekeeper/pull/2174) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/52db6a757485350ae4820b41cb50fb3eac8f7beb))

## Bug Fixes
- Ignore all stackdriver errors if --stackdriver-only-when-available is set (#2304) [#2304](https://github.com/open-policy-agent/gatekeeper/pull/2304) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/9a56db99d379f8641e43fa5465138e9008d2e59b))
- fix CVE-2022-27664 (#2310) [#2310](https://github.com/open-policy-agent/gatekeeper/pull/2310) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/7cf807a6770ede8d927d5eda4497d7fa87a7b232))
- Namespace should be nil for audited cluster-scoped resources (#2243) [#2243](https://github.com/open-policy-agent/gatekeeper/pull/2243) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/510da537a645b27ab00a9850b94c3096860ac1cf))
- skip empty k8s resources (#2247) [#2247](https://github.com/open-policy-agent/gatekeeper/pull/2247) ([qa-ship-it](https://github.com/open-policy-agent/gatekeeper/commit/7bde0115932dcb73255110273eec8e40167ebacc))
- **helm**: Fix "Label exempted namespaces" (#2246) [#2246](https://github.com/open-policy-agent/gatekeeper/pull/2246) ([Mathieu Parent](https://github.com/open-policy-agent/gatekeeper/commit/e86e865c8333c29b4ac56c33e8174797f3718a62))
- helm upgrade test (#2263) [#2263](https://github.com/open-policy-agent/gatekeeper/pull/2263) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/2bc00bc946c795ff8f457d2f9fe88621bcee86de))
- Change 'securityContext/capabilities/drop' from 'all' to 'ALL'. (#2273) [#2273](https://github.com/open-policy-agent/gatekeeper/pull/2273) ([BoatMisser](https://github.com/open-policy-agent/gatekeeper/commit/600a68d40db66a66f00d057ae7b69a8e03a641eb))
- **helm**: Fix "Label exempted namespaces"  (#2290) [#2290](https://github.com/open-policy-agent/gatekeeper/pull/2290) ([Zhimin Xiang](https://github.com/open-policy-agent/gatekeeper/commit/55a1bd5b5528d30f7aaddca97b781ff358b1e4b3))
- update website/versions.json (#2175) [#2175](https://github.com/open-policy-agent/gatekeeper/pull/2175) ([Ernest Wong](https://github.com/open-policy-agent/gatekeeper/commit/36f1e0ba58adaf55cac0b5ddc3c7108b6074bb7e))
- chart always use v1beta1 as pdb api version (#2164) [#2164](https://github.com/open-policy-agent/gatekeeper/pull/2164) ([Mingfei Huang](https://github.com/open-policy-agent/gatekeeper/commit/26abae579e0e1408b799311130e86330e89ee0a5))
- Set spec.hard.pod value to string (#1928) [#1928](https://github.com/open-policy-agent/gatekeeper/pull/1928) ([Ahmed](https://github.com/open-policy-agent/gatekeeper/commit/6f665b46def5755da0e0ddf2c6451641b32c9938))
- document mutations name matcher (#2168) [#2168](https://github.com/open-policy-agent/gatekeeper/pull/2168) ([Nicholas Blott](https://github.com/open-policy-agent/gatekeeper/commit/20aa6c43970d924946c01943c1e06809a617984c))
- **helm**: helm chart updates for disabling psp and default api for poddisruptionbudget (#2187) [#2187](https://github.com/open-policy-agent/gatekeeper/pull/2187) ([Boojapho](https://github.com/open-policy-agent/gatekeeper/commit/46547dbe90ca3871ebd0983db91ae7fe59128af3))
- **helm**: explicitly specify curl in probeWebhook (#2207) [#2207](https://github.com/open-policy-agent/gatekeeper/pull/2207) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/4ca9d10f3dc9732eb96342b39b4bab69b9e83347))
- Docker related Makefile improvements (#2209) [#2209](https://github.com/open-policy-agent/gatekeeper/pull/2209) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/13692a9e52cf62b517b7fcea402be9bb2927faf1))
- Only set ConstraintTemplate's status.created on success (#2208) [#2208](https://github.com/open-policy-agent/gatekeeper/pull/2208) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/e19a6c84309ae6aff7f013a283c55d86d8bb2c29))
- sed on specific tag in `make release-manifest` (#2153) [#2153](https://github.com/open-policy-agent/gatekeeper/pull/2153) ([Ernest Wong](https://github.com/open-policy-agent/gatekeeper/commit/992484381ecee68fc0ba893cab5dcad2d2bc7934))
- make audit more fault tolerant, log error instead of skipping update (#2162) [#2162](https://github.com/open-policy-agent/gatekeeper/pull/2162) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/59e190f118e7da4cbeaa5233c78a1b9f61e5ef9b))

## Documentation
- Update default auditChunkSize in readme (#2303) [#2303](https://github.com/open-policy-agent/gatekeeper/pull/2303) ([Simeon Bobylev](https://github.com/open-policy-agent/gatekeeper/commit/b2b566c2f38bd01f9abdd1d5956cd571452f39dd))
- enforcement action override in ExpansionTemplate (#2300) [#2300](https://github.com/open-policy-agent/gatekeeper/pull/2300) ([davis-haba](https://github.com/open-policy-agent/gatekeeper/commit/9f4510caccddce4f3b953622db3ae4ceb4713d1f))
- update feature state for alpha and beta things (#2260) [#2260](https://github.com/open-policy-agent/gatekeeper/pull/2260) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/a1add93b0beb5c48eb92a6a2eb5ee7d21551a1b6))
- add brew install instructions to gator docs (#2255) [#2255](https://github.com/open-policy-agent/gatekeeper/pull/2255) ([Xander Grzywinski](https://github.com/open-policy-agent/gatekeeper/commit/9fe509e13e5bb4b1c844cbc6ba7817f8dd1c1419))
- Update library links to point to website (#2264) [#2264](https://github.com/open-policy-agent/gatekeeper/pull/2264) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/c4e08c683e2c3f3568034380053bf4b3dfc07b4b))
- Update contributing guide (#2275) [#2275](https://github.com/open-policy-agent/gatekeeper/pull/2275) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/ab8102d866d5186ca4fb059e7813ac9333c7e5b7))
- documentation for generator resource expansion feature (#2229) [#2229](https://github.com/open-policy-agent/gatekeeper/pull/2229) ([davis-haba](https://github.com/open-policy-agent/gatekeeper/commit/ee3a992bfc6fcdc44897413eb169cc8c52284d6e))
- link to template provider (#2190) [#2190](https://github.com/open-policy-agent/gatekeeper/pull/2190) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/841e10000acf3e7fc42e35a0ab1be5dfbe0138ae))
- add fields that are not populated in audit (#2191) [#2191](https://github.com/open-policy-agent/gatekeeper/pull/2191) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/ffcac9529fb66940979e19e7be82d34d2599e0ad))
- add applyTo field for ModifySet in mutation docs (#2056) [#2056](https://github.com/open-policy-agent/gatekeeper/pull/2056) ([davis-haba](https://github.com/open-policy-agent/gatekeeper/commit/86a2deda7292644407f46777565b6783329b790b))
- add singleton for audit (#2155) [#2155](https://github.com/open-policy-agent/gatekeeper/pull/2155) ([Rita Zhang](https://github.com/open-policy-agent/gatekeeper/commit/54905d704f63fe72e575c525cadbe03c7e17433a))

## Performance Improvements
- Upgrade constraint framework to v0.8.0 (#2319) [#2319](https://github.com/open-policy-agent/gatekeeper/pull/2319) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/9a9663bfb9dd5157ac4601c340a94f4d0cb78cd9))
- Default --max-serving-threads to GOMAXPROCS (#2216) [#2216](https://github.com/open-policy-agent/gatekeeper/pull/2216) ([Max Smythe](https://github.com/open-policy-agent/gatekeeper/commit/0bf647651cbafe53b9369677cc604e524b5d50b1))

## Continuous Integration
- bump trivy to 0.32.1 (#2312) [#2312](https://github.com/open-policy-agent/gatekeeper/pull/2312) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/ab939829c8296634149eb68cb030d9fc8b6d2a21))
- bump e2e k8s version (#2258) [#2258](https://github.com/open-policy-agent/gatekeeper/pull/2258) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/c8bf4bdff64b2d15f7ce3c41bbaba40ecd4f4ad7))
- add stale bot config (#2183) [#2183](https://github.com/open-policy-agent/gatekeeper/pull/2183) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/81c4a2635973ce1be5350641ab3f6fbae844abcd))

## Chores
- bump github/codeql-action from 2.1.25 to 2.1.26 (#2306) [#2306](https://github.com/open-policy-agent/gatekeeper/pull/2306) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/70a65fa45dc0cf828784cd897c3b99b121bb8104))
- bump github/codeql-action from 2.1.19 to 2.1.20 (#2244) [#2244](https://github.com/open-policy-agent/gatekeeper/pull/2244) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/7a5cc6fa11ba59dc1127716a7f1ab3850c30da14))
- bump github/codeql-action from 2.1.20 to 2.1.22 (#2251) [#2251](https://github.com/open-policy-agent/gatekeeper/pull/2251) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/08a626398c4eacd0ad5439c34806bb8e689d536a))
- bump contrib.go.opencensus.io/exporter/prometheus from 0.4.1 to 0.4.2 (#2250) [#2250](https://github.com/open-policy-agent/gatekeeper/pull/2250) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/8c9a22cd8e734f858e5a3a8a5d4b3434e67084d2))
- bump @docusaurus/core from 2.0.1 to 2.1.0 in /website (#2253) [#2253](https://github.com/open-policy-agent/gatekeeper/pull/2253) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/c1a02d127a7388eeeb3390ef7336d63b57e1c50f))
- bump @docusaurus/preset-classic from 2.0.1 to 2.1.0 in /website (#2254) [#2254](https://github.com/open-policy-agent/gatekeeper/pull/2254) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/501b9316d3dcff72b6dd2325cefc4b7e9db1fec4))
- updates gatekeeper website reference (#2257) [#2257](https://github.com/open-policy-agent/gatekeeper/pull/2257) ([Nilekh Chaudhari](https://github.com/open-policy-agent/gatekeeper/commit/c7f01d043e9935cbea996d1f90de5acd1ef9c36f))
- bump github.com/google/go-cmp from 0.5.8 to 0.5.9 (#2259) [#2259](https://github.com/open-policy-agent/gatekeeper/pull/2259) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/7d7cacae8b57e11ee980651955502a6da61dc21b))
- bump github/codeql-action from 2.1.22 to 2.1.23 (#2265) [#2265](https://github.com/open-policy-agent/gatekeeper/pull/2265) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/f21d07a0a86b0f47e427aa5f82f1ca3d1a597382))
- bump k8s.io/client-go from 0.24.4 to 0.24.5 (#2267) [#2267](https://github.com/open-policy-agent/gatekeeper/pull/2267) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/847eb3f4707736da10e3ba55f9a96fcf2c0a93b8))
- bump contrib.go.opencensus.io/exporter/stackdriver from 0.13.13 to 0.13.14 (#2269) [#2269](https://github.com/open-policy-agent/gatekeeper/pull/2269) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/8a3e2159a0e8be138d46c4a199829a5b8d03d5d5))
- bump github/codeql-action from 2.1.23 to 2.1.24 (#2274) [#2274](https://github.com/open-policy-agent/gatekeeper/pull/2274) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/01e2d53a759c82f8044e8dcbec14fc5934abbfe2))
- bump k8s.io/client-go from 0.24.5 to 0.24.6 (#2284) [#2284](https://github.com/open-policy-agent/gatekeeper/pull/2284) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/442838086a073607def8d1911ba1e3cb43045b99))
- bump github/codeql-action from 2.1.24 to 2.1.25 (#2281) [#2281](https://github.com/open-policy-agent/gatekeeper/pull/2281) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/bad90af414fe34fee41fe2a91be1068d659c9e02))
- bump k8s.io/client-go from 0.24.2 to 0.24.3 (#2178) [#2178](https://github.com/open-policy-agent/gatekeeper/pull/2178) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/92afce93e476a7be0df041a72a52f60d8b0d6475))
- bump frameworks to b0dbc52 (#2179) [#2179](https://github.com/open-policy-agent/gatekeeper/pull/2179) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/e1c6f36243e30b4b3ed64944658dcf39c60eb2f4))
- bump terser from 5.12.1 to 5.14.2 in /website (#2180) [#2180](https://github.com/open-policy-agent/gatekeeper/pull/2180) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/6a0d7bc0dabef3779f02fcb4279bee49b8604820))
- Run trivy scan on git repository and update version (#2169) [#2169](https://github.com/open-policy-agent/gatekeeper/pull/2169) ([Juan Antonio Osorio](https://github.com/open-policy-agent/gatekeeper/commit/8f1ef8c908d034d2449aeb4c59ba42e0ef43941d))
- update stale tag (#2189) [#2189](https://github.com/open-policy-agent/gatekeeper/pull/2189) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/da655da4bed487a7090b441a39ec4c1064ff577b))
- bump github/codeql-action from 2.1.16 to 2.1.17 (#2199) [#2199](https://github.com/open-policy-agent/gatekeeper/pull/2199) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/ab064a056c04c320bdadb30511bc8ad45b4f9e98))
- bump @docusaurus/core from 2.0.0-rc.1 to 2.0.1 in /website (#2210) [#2210](https://github.com/open-policy-agent/gatekeeper/pull/2210) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/28038736c90f20f4f4b93e08809d988a8f0669c0))
- bump @docusaurus/preset-classic from 2.0.0-rc.1 to 2.0.1 in /website (#2211) [#2211](https://github.com/open-policy-agent/gatekeeper/pull/2211) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/05650d2510d4b6b006b6b62a0eefd50485fc65cd))
- use volume mounts for tests (#2213) [#2213](https://github.com/open-policy-agent/gatekeeper/pull/2213) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/557394fac726cfdda96291e74e3a473c17ef3d2a))
- bump github/codeql-action from 2.1.17 to 2.1.18 (#2217) [#2217](https://github.com/open-policy-agent/gatekeeper/pull/2217) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/33deaec211d4ea7038ddeb4786e4d984c97a39d4))
- bump ci to Go 1.19 (#2222) [#2222](https://github.com/open-policy-agent/gatekeeper/pull/2222) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/d14c0da75d623eb30b9253d0ec88e06b13c3467f))
- bump github/codeql-action from 2.1.18 to 2.1.19 (#2233) [#2233](https://github.com/open-policy-agent/gatekeeper/pull/2233) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/5b2e84e3e7db0e306c5a511d5b6bcf6a4fc76c7f))
- update audit duration buckets (#2234) [#2234](https://github.com/open-policy-agent/gatekeeper/pull/2234) ([Viktor Oreshkin](https://github.com/open-policy-agent/gatekeeper/commit/368961ad0b1d2ea0a59720e0f224549e332ae82a))
- bump github.com/emicklei/go-restful from v2.15.0 to v2.16.0 (#2240) [#2240](https://github.com/open-policy-agent/gatekeeper/pull/2240) ([MIchael Steputat](https://github.com/open-policy-agent/gatekeeper/commit/d4fdccee42c163a2c72785ab423725b06ddd99ca))
- bump k8s.io/apimachinery from 0.24.3 to 0.24.4 (#2236) [#2236](https://github.com/open-policy-agent/gatekeeper/pull/2236) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/fcc7f26ba2f488f0045a5e76c704629cbac63aee))
- bump k8s.io/client-go from 0.24.3 to 0.24.4 (#2237) [#2237](https://github.com/open-policy-agent/gatekeeper/pull/2237) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/dfa6b332c8c59e022b2341506ae98f293e62f864))
- bump @docusaurus/core from 2.0.0-beta.21 to 2.0.0-beta.22 in /website (#2157) [#2157](https://github.com/open-policy-agent/gatekeeper/pull/2157) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/c153f551d6289684ffbe9e423f6265ff4a837cae))
- bump @docusaurus/preset-classic from 2.0.0-beta.21 to 2.0.0-beta.22 in /website (#2156) [#2156](https://github.com/open-policy-agent/gatekeeper/pull/2156) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/ecf84d5994714bdc2b964d40613c0fed17af897c))
- bump k8s.io/klog/v2 from 2.70.0 to 2.70.1 (#2159) [#2159](https://github.com/open-policy-agent/gatekeeper/pull/2159) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/ec0eeae6c0d32379ab8c83a5fb10c21c55bd00ff))
- bump sigs.k8s.io/controller-runtime from 0.12.2 to 0.12.3 (#2158) [#2158](https://github.com/open-policy-agent/gatekeeper/pull/2158) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/f5eacc7e8bf33a80c74fff66f540aa464c87d1c8))
- bump github/codeql-action from 2.1.15 to 2.1.16 (#2167) [#2167](https://github.com/open-policy-agent/gatekeeper/pull/2167) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/d3d206a4716397fd0c81eb4da2ce22cf4f7e9e01))
- bump @docusaurus/core from 2.0.0-beta.22 to 2.0.0-rc.1 in /website (#2170) [#2170](https://github.com/open-policy-agent/gatekeeper/pull/2170) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/b1a7fb82647db9d993a7d00d6b86651b51a9eb81))
- bump @docusaurus/preset-classic from 2.0.0-beta.22 to 2.0.0-rc.1 in /website (#2171) [#2171](https://github.com/open-policy-agent/gatekeeper/pull/2171) ([dependabot[bot]](https://github.com/open-policy-agent/gatekeeper/commit/472bcd74dfe0b02782828f6c61cb115af515977d))

## New Contributors
* @max0ne made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2164
* @OpenSourceZombie made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/1928
* @JAORMX made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2169
* @Boojapho made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2187
* @ethanrange made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2220
* @stp-bsh made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2240
* @qa-ship-it made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2247
* @salaxander made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2255
* @boatmisser made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2273
* @gracedo made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2289
* @meons made their first contribution in https://github.com/open-policy-agent/gatekeeper/pull/2303

**Full Changelog**: https://github.com/open-policy-agent/gatekeeper/compare/v3.9.0...v3.10.0