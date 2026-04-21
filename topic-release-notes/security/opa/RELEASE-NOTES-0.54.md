# opa v0.54 Release Notes

Source: [v0.54.0](https://github.com/open-policy-agent/opa/releases/tag/v0.54.0)

This release focuses on bug fixes, but also includes some improvements to the SDK and commandline.

Note: This will be the last OPA release to support building with Golang 1.18. (Golang 1.21 is expected to be released in August. Keeping the support for 1.18 is blocking OPA from upgrading OpenTelemetry.)

### Topdown and Rego

- Add unwrap functionality to topdown.Error ([#5890](https://github.com/open-policy-agent/opa/issues/5890)) authored by @ajith-sub reported by @ajith-sub
- Lazy obj performance ([#6009](https://github.com/open-policy-agent/opa/issues/6009)) authored by @johanfylling reported by @kubaj
- ast: Only realizing `lazyObj` when compared against other object type ([6060](https://github.com/open-policy-agent/opa/pull/6060)) (authored by @johanfylling)
- ast: Fixing issue in type-checker where partial objects couldn't have key overrides of divergent type ([#5972](https://github.com/open-policy-agent/opa/issues/5972)) authored by @johanfylling
- planner: CallDynamic regression fix ([#5964](https://github.com/open-policy-agent/opa/issues/5964)) authored by @srenatus
- fmt: Fix `fmt` panic in comprehension with comments ([#5798](https://github.com/open-policy-agent/opa/issues/5798)) authored by @Trolloldem reported by @Djoust
- topdown: Format integer numbers without exponent ([#6013](https://github.com/open-policy-agent/opa/issues/6013)) authored by @kenjenkins reported by @kenjenkins
- topdown: Fix panic in partial eval with ref head rule ([#6027](https://github.com/open-policy-agent/opa/issues/6027)) authored by @srenatus
- Fixed a bug in `object.union_n` where nested objects were mutated ([#5975](https://github.com/open-policy-agent/opa/issues/5975)) authored by @qshu-splunk
- Fixed the issue of the `object.subset` method failing to correctly compare array relationships ([5968](https://github.com/open-policy-agent/opa/issues/5968)) authored by @DCRUNNN
- topdown: Fixed caching race condition issue in `http.send` ([#5997](https://github.com/open-policy-agent/opa/pull/5997)) authored by @ashutosh-narkar
- Allow time formatting constants in rego `time.format` and `time.parse_ns` ([#5945](https://github.com/open-policy-agent/opa/issues/5945)) authored by @tjons

### Runtime, Tooling, SDK

- Add `--schema` flag to `opa test` ([#5923](https://github.com/open-policy-agent/opa/issues/5923)) authored by @renatosc
- Add ability to specify namespace for optimized files ([#5933](https://github.com/open-policy-agent/opa/issues/5933)) authored by @ashutosh-narkar reported by @deezkay
- Fix for the issue when OPA throws misleading error (storage_not_found_error) message while loading the delta bundle when persist property in config is true. ([#5959](https://github.com/open-policy-agent/opa/issues/5959)) authored by @yogisinha reported by @jnethery
- cmd: Update storage when a file remove op is detected ([#5986](https://github.com/open-policy-agent/opa/issues/5986)) authored by @boranx
- cmd: Add support for watch mode in opa test ([#1719](https://github.com/open-policy-agent/opa/issues/1719)) authored by @ashutosh-narkar reported by @Fox32
- download: Pass request to docker.Authorizer ([#5902](https://github.com/open-policy-agent/opa/issues/5902)) authored by @DerGut reported by @carabasdaniel
- plugins/discovery: Fix discovery erasing `persistence_directory` config ([#6042](https://github.com/open-policy-agent/opa/pull/6042)) authored by @blacksails
- plugins/discovery: Fix persistence of discovery bundle ([#6048](https://github.com/open-policy-agent/opa/pull/6048)) (authored by @bdjgs)
- Add tracing to bundle/discovery download ([#5967](https://github.com/open-policy-agent/opa/issues/5967)) authored by @mjungsbluth
- Fallback on embedded timezone database if `tzdata` is not found on filesystem ([6038](https://github.com/open-policy-agent/opa/pull/6038)) authored by @charlieegan3
- extensibility: Adding hooks (plugins, discovery, sdk) ([#6053](https://github.com/open-policy-agent/opa/pull/6053)) authored by @srenatus
- sdk: allow passing in a separate `Store` implementation in SDK ([5962](https://github.com/open-policy-agent/opa/pull/5962)) authored by @srenatus
- config: Show "extra", unknown fields in `/v1/config` API result ([6056](https://github.com/open-policy-agent/opa/pull/6056)) authored by @srenatus

### Miscellaneous
- Disable provenance attestations in buildx ([#5877](https://github.com/open-policy-agent/opa/issues/5877)) authored by @ashutosh-narkar reported by @JasonMan34
- build: configure SELinux labels for Docker volumes ([#6054](https://github.com/open-policy-agent/opa/issues/6054)) authored by @zregvart reported by @zregvart
- Dependency bumps, notably:
  - golang from 1.20.4 to 1.20.5
  - github.com/prometheus/client_golang from from 1.15.1 to v1.16.0