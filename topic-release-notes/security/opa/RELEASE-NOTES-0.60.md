# opa v0.60 Release Notes

Source: [v0.60.0](https://github.com/open-policy-agent/opa/releases/tag/v0.60.0)

v0.60.0


### Runtime, Tooling, SDK
- OPA can be run in 1.0 compatibility mode by using the new `--v1-compatible` flag. When this mode is enabled, the current release of OPA will behave as OPA `v1.0` will eventually behave by default. This flag is currently supported on the `build`, `check`, `fmt`, `eval` and `test` commands ([#6478](https://github.com/open-policy-agent/opa/pull/6478)) authored by @johanfylling
- Extend the telemetry report to include the minimum compatible version of policies loaded into OPA ([#6361](https://github.com/open-policy-agent/opa/issues/6361)) co-authored by @srenatus and @ashutosh-narkar
- server: Support fsnotify based reloading of certificate, key and CA cert pool when they change on disk ([#5788](https://github.com/open-policy-agent/opa/issues/5788)) authored by @charlieegan3
- Add option on the unit test runner to surface builtin errors. This should help with debugging errors generated while running unit tests ([#6489](https://github.com/open-policy-agent/opa/issues/6489)) authored by @jalseth
- Fix issue in `opa fmt` where the assignment operator and term in the rule head of chain rules are removed from the re-written rule head  ([#6467](https://github.com/open-policy-agent/opa/issues/6467)) authored by @anderseknert
- cmd/fmt: Replace dependency on `diff` tool with an external golang library function ([#6284](https://github.com/open-policy-agent/opa/issues/6284)) authored by @colinjlacy

### Topdown and Rego
- topdown/providers: Preserve user provided http headers in the `providers.aws.sign_req` builtin command ([#6456](https://github.com/open-policy-agent/opa/pull/6456)) authored by @c2zwdjnlcg
- rego: Allow custom builtin function registration to provide a description for the builtin ([#6449](https://github.com/open-policy-agent/opa/issues/6449)) authored by @lcarva
- ast+cmd: Allow bundle to contain calls to unknown functions when inspected ([#6457](https://github.com/open-policy-agent/opa/issues/6457)) authored by @johanfylling

### Docs
- Add section on the changes proposed for a future OPA v1.0 and update Rego examples to be OPA v1.0 compliant([#6453](https://github.com/open-policy-agent/opa/issues/6453)) authored by @johanfylling
- Clarify behavior of the `sprintf` builtin command when used with the `%T` marker ([#6487](https://github.com/open-policy-agent/opa/issues/6487)) authored by @lcarva

### Website + Ecosystem
- Ecosystem: Digger ([#6464](https://github.com/open-policy-agent/opa/pull/6464)) authored by @anderseknert

### Miscellaneous
- Update `Makefile` to allow custom `GOFLAGS` to be provided to the golang executable ([#6458](https://github.com/open-policy-agent/opa/issues/6458)) authored by @cova-fe
- Dependency updates; notably:
  - bump golang 1.21.4 -> 1.21.5 ([#6460](https://github.com/open-policy-agent/opa/pull/6460)) authored by @srenatus
  - bump aquasecurity/trivy-action from 0.14.0 to 0.16.0
  - bump github.com/containerd/containerd from 1.7.9 to 1.7.11
  - bump google.golang.org/grpc from 1.59.0 to 1.60.1
  - bump github.com/google/uuid from 1.4.0 to 1.5.0

