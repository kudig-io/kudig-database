# opa v0.18 Release Notes

Source: [v0.18.0](https://github.com/open-policy-agent/opa/releases/tag/v0.18.0)

### Features

- Add `opa bench` and `opa test --bench` sub commands for benchmarking policy evaluation. ([#1424](https://github.com/open-policy-agent/opa/issues/1424))
- Permit verifying JWT's with a public key
- `http.send` improvements:
  - Allow for skipping TLS verification via `tls_insecure_skip_verify` option
  - Add `Host` header support

### New Built-in Functions

- Bitwise operators ([#1919](https://github.com/open-policy-agent/opa/issues/1919))
  - `bits.or`
  - `bits.and`
  - `bits.negate`
  - `bits.xor`
  - `bits.lsh`
  - `bits.rsh`
- `json.remove` which works similar to `object.remove` but supports a JSON pointer path.

### Fixes
- docs: Render tutorials as list ([#2071](https://github.com/open-policy-agent/opa/issues/2071))
- ast: Fix type check for objects with non-json keys ([#2183](https://github.com/open-policy-agent/opa/issues/2183))
- ast: Return an error when parsing an empty module ([#2054](https://github.com/open-policy-agent/opa/issues/2054))
- docs: Fix broken PAM module link ([#2113](https://github.com/open-policy-agent/opa/issues/2113))
- docs: Fix code fence in kubernetes-primer.md ([#2177](https://github.com/open-policy-agent/opa/issues/2177))
- topdown: Invoke iterator when evaluating negation ([#2142](https://github.com/open-policy-agent/opa/issues/2142))
- Correct checkptr errors found with Go 1.14
- `opa parse`: fix panic when parsing invalid JSON

### Compatibility Notes

- The `ast.ParseModule` helper will now return an error if an empty module is provided.
  Previously it would return a `nil` error and `nil` module. ([#2054](https://github.com/open-policy-agent/opa/issues/2054))
- The `cmd` and `tester` packages in OPA will now require Go 1.13+ to compile. Most library users should be unaffected.

### Miscellaneous

- bundle: Dedicate `policy.wasm` for the compiled policy.