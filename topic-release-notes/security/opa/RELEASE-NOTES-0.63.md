# opa v0.63 Release Notes

Source: [v0.63.0](https://github.com/open-policy-agent/opa/releases/tag/v0.63.0)

This release contains a mix of features, performance improvements, and bugfixes.

### Runtime, Tooling, SDK

- cmd/exec: Add `--timeout` flag to `opa exec` to prevent infinite hangs. ([#6613](https://github.com/open-policy-agent/opa/issues/6613)) authored by @philipaconrad
- download: Surface bundle download errors via debug logging ([#6609](https://github.com/open-policy-agent/opa/issues/6609)) authored by @ashutosh-narkar reported by @nevumx
- topdown: Fixing overactive Early Exit suppression ([#6566](https://github.com/open-policy-agent/opa/issues/6566)) authored by @johanfylling reported by @ashwinhb
- plugins/rest: Add support to get temp creds via AssumeRole ([#6634](https://github.com/open-policy-agent/opa/pull/6634)) authored by @ashutosh-narkar

### Topdown and Rego

- topdown: Adding a new `crypto.x509.parse_and_verify_certificates_with_options` built-in function. ([#5882](https://github.com/open-policy-agent/opa/issues/5882)) authored by @yogisinha reported by @IxDay
- format: Preserve brackets around set union operation ([#6588](https://github.com/open-policy-agent/opa/issues/6588)) authored by @ashutosh-narkar reported by @HarshPathakhp
- aws: Support for Unsigned Payload or provided content sha256 in AWS signing ([#6581](https://github.com/open-policy-agent/opa/pull/6611)) authored by @prasanthj

### Docs + Website + Ecosystem

- ADOPTERS.md: Add Facets.cloud to the list ([#6640](https://github.com/open-policy-agent/opa/issues/6640)) authored by @ashutosh-narkar reported by @samarthya-gupta1
- docs: Mention homebrew install option ([#6622](https://github.com/open-policy-agent/opa/issues/6622)) authored by @anderseknert
- docs: Add Rego v1 keywords to list of reserved names ([#6649](https://github.com/open-policy-agent/opa/pull/6649)) authored by @anderseknert
- docs: Add Tunnelmole as an open source tunneling option in the Cloudformation hooks documentation ([#6626](https://github.com/open-policy-agent/opa/pull/6626)) authored by @robbie-cahill
- docs: Add docs on using env vars in place of CLI flags ([#6631](https://github.com/open-policy-agent/opa/pull/6631)) authored by @anderseknert
- docs: Adding integration for Backstage ([#6629](https://github.com/open-policy-agent/opa/pull/6629)) authored by @Parsifal-M
- docs: Clear up some uses of future keywords ([#6653](https://github.com/open-policy-agent/opa/pull/6653)) authored by @charlieegan3
- docs: Update delta bundle patch doc for remove op ([#6645](https://github.com/open-policy-agent/opa/pull/6645)) authored by @0marq
- docs: Fix typo in `Debugging OPA` ([#6637](https://github.com/open-policy-agent/opa/pull/6637)) authored by @setchy

### Miscellaneous

- chore: Remove repetitive words ([#6644](https://github.com/open-policy-agent/opa/pull/6644)) authored by @occupyhabit
- Dependency updates; notably:
  - build(deps): bump github.com/containerd/containerd from 1.7.13 to 1.7.14
  - build(deps): bump github.com/golang/protobuf from 1.5.3 to 1.5.4
  - build(deps): bump google.golang.org/grpc from 1.62.0 to 1.62.1

