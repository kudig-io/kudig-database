# opa v0.51 Release Notes

Source: [v0.51.0](https://github.com/open-policy-agent/opa/releases/tag/v0.51.0)

This release contains improvements to monitoring and an assortment of fixes and improvements.

### Monitoring

#### Surface unauthorized request count from OPA HTTP API authz handler via Status API

Currently when OPA's HTTP server rejects requests per the [authz policy](https://www.openpolicyagent.org/docs/latest/security/#authentication-and-authorization), this is not accounted for via the management APIs. This change adds that count in the metric registry that is part of the Status API for more visibility.

([#3378](https://github.com/open-policy-agent/opa/issues/3378)) authored by @ashutosh-narkar.

#### Surface more decision log errors via Status API 

Previously in [5732](https://github.com/open-policy-agent/opa/pull/5732), we updated the decision log plugin to surface errors via the Status API. However, in that change certain events like encoder errors and log drops due to buffer size limits had no metrics associated with them. This change adds more metrics for these events so that they can be surfaced via the Status API.

([#5637](https://github.com/open-policy-agent/opa/issues/5637)) authored by @ashutosh-narkar.

#### Include truncated HTTP response in logs 

This change updates the client debug log to include the full HTTP response in case of non-200 status codes. Recording the response in the logs can help to provide more information to debug error scenarios.

([#2961](https://github.com/open-policy-agent/opa/issues/2961)) authored by @ashutosh-narkar reported by @gshively11.

### Topdown and Rego

- Wasm: Add native support for `object.union_n` built-in function (authored by @Azanul)

### Fixes

- ast: Properly set the reported location of unused variables in strict-mode errors. ([#5662](https://github.com/open-policy-agent/opa/issues/5662)) authored by @boranx
- fmt: report wrong arity for built-in functions. ([#5646](https://github.com/open-policy-agent/opa/issues/5646)) authored by @Trolloldem
- topdown: http.send(): Ensuring intra-query caching consistency. ([#5736](https://github.com/open-policy-agent/opa/issues/5736)) authored by @johanfylling
- Performance improvements to decision logging.
  Specifically, by removing superfluous json encoding roundtrip and double work in AST conversion of to-be-logged events. (authored by @srenatus)

### Docs, Website, and Ecosystem

- Fix typo in documentation (authored by @eternaltyro)
- Update TLS authentication docs (authored by @charlieegan3)
- Clarification in docs about checksums of Windows executables (authored by @Ronnie-personal)
- docs: Small fix to context placement in integration (authored by @craigpastro)
- docs/website: Fix floating navbar anchor issue ([5774](https://github.com/open-policy-agent/opa/issues/5774)) authored by @charlieegan3 reported by @kristiansvalland
  
### Miscellaneous

- Update -debug images to use Chainguard images ([5544](https://github.com/open-policy-agent/opa/issues/5544)) (authored by @charlieegan3)
- Various third-party dependencies were updated.