# opa v0.48 Release Notes

Source: [v0.48.0](https://github.com/open-policy-agent/opa/releases/tag/v0.48.0)

This release rolls in security fixes from recent patch releases, along with a number of bugfixes, and a new builtin function.

### Improved error reporting available in `opa eval`

A common frustration when writing policies in OPA is when an error happens, causing a rule to unexpectedly return `undefined`. Using `--strict-builtin-errors` would allow finding the first error encountered during evaluation, but terminates execution immediately.

To improve the debugging experience, it is now possible to display *all* of the errors encountered during normal evaluation of a policy, via the new `--show-builtin-errors` option.

Consider the following error-filled policy, `multi-error.rego`:

```rego
package play

this_errors(number) := result {
        result := number / 0
}

this_errors_too(number) := result {
        result := number / 0
}

res1 := this_errors(1)

res2 := this_errors_too(1)
```

Using `--strict-builtin-errors`, we would only see the first divide by zero error:

    opa eval --strict-builtin-errors -d multi-error.rego data.play

```
1 error occurred: multi-error.rego:4: eval_builtin_error: div: divide by zero
```

Using `--show-builtin-errors` shows both divide by zero issues though:

    opa eval --show-builtin-errors -d multi-error.rego data.play -f pretty

```
2 errors occurred:
multi-error.rego:4: eval_builtin_error: div: divide by zero
multi-error.rego:8: eval_builtin_error: div: divide by zero
```

By showing more errors up front, we hope this will improve the overall policy writing experience.

### New Built-in Function: `time.format`

It is now possible to format a time value from nanoseconds to a formatted timestamp string via a built-in function. The builtin accepts 3 argument formats, each allowing for different options:

 1. A number representing the nanoseconds since the epoch (UTC).
 2. A two-element array of the nanoseconds, and a timezone string.
 3. A three-element array of nanoseconds, timezone string, and a layout string (same format as for `time.parse_ns`).

See [the documentation](https://www.openpolicyagent.org/docs/v0.48.0/policy-reference/#builtin-time-timeformat) for all details.

Implemented by @burnerlee.

### Optimization in rule indexing

Previously, every time the evaluator looked up a rule in the index, OPA performed checks for grounded refs over the entire index *before* looking up the rule.

Now, OPA performs all groundedness checks once at index construction time, which keeps index lookup times much more consistent as the number of indexed rules scales up.

Policies with large numbers of index-ready rules can expect a small performance lift, proportional to the number of indexed rules.

### Bundle fetching with AWS Signing Version 4A

AWS has recently developed an extension to SigV4 called Signature Version 4A (SigV4A) which enables signatures that are valid in more than one AWS Region. This new signature method is required for signing multi-region API requests, such as Amazon S3 Multi-Region Access Points (MRAP).

OPA now supports this new request signing method for bundle fetching, which means that you can use an S3 MRAP as a bundle source. This is configured via the new `services[<your_service_name>].credentials.s3_signing.signature_version` field.

See the [the documentation](https://www.openpolicyagent.org/docs/v0.48.0/configuration/#aws-signature) for more details.

Implemented by @jwineinger

### Runtime

- rego: Check store modules before skipping parsing (authored by @charlieegan3)
- topdown/rego: Add BuiltinErrorList support to rego package, add to eval command (authored by @charlieegan3)
- topdown: Fix evaluator's re-wrapping of `NDBCache` errors (authored by @srenatus)
- Fix potential memory leak from `http.send` in interquery cache (authored by @asleire)
- ast/parser: Detect function rule head + `contains` keyword ([#5525](https://github.com/open-policy-agent/opa/issues/5525)) authored and reported by @philipaconrad
- ast/visit: Add `SomeDecl` to visitor walks ([#5480](https://github.com/open-policy-agent/opa/issues/5480)) authored by @srenatus
- ast/visit: Include `LazyObject` in visitor walks ([#5479](https://github.com/open-policy-agent/opa/issues/5479)) authored by @srenatus reported by @benweint

### Tooling, SDK

- topdown: cache undefined rule evaluations ([#593](https://github.com/open-policy-agent/opa/issues/593)) authored by @edpaget reported by @tsdandall
- topdown: Specify host verification policy for http redirects ([#5388](https://github.com/open-policy-agent/opa/issues/5388)) authored and reported by @ashutosh-narkar
- providers/aws: Refactor + Fix 2x Authorization header append issue ([#5472](https://github.com/open-policy-agent/opa/issues/5472)) authored by @philipaconrad reported by @Hiieu
- Add support to enable ND builtin cache via discovery ([#5457](https://github.com/open-policy-agent/opa/issues/5457)) authored by @ashutosh-narkar reported by @asadali
- format: Only use ref heads for all rule heads if necessary ([#5449](https://github.com/open-policy-agent/opa/issues/5449)) authored and reported by @srenatus
- `opa inspect`: Fix path of data namespaces on windows (authored by @shm12)
- ast+cmd: Only enforcing `schemas` annotations if `--schema` flag is used (authored by @johanfylling)
- sdk: Allow use of a query tracer (authored by @charlieegan3)
- sdk: Allow use of metrics, profilers, and instrumentation (authored by @charlieegan3)
- sdk: Return provenance information in Result types (authored by @charlieegan3)
- sdk: Allow use of StrictBuiltinErrors (authored by @charlieegan3)
- Allow print calls in IR (authored by @anderseknert)
- tester/runner: Fix panic'ing case in utility function ([#5496](https://github.com/open-policy-agent/opa/issues/5496)) authored and reported by @philipaconrad

### Docs

- Community page updates (authored by @anderseknert)
- Update Hugo version, update deprecated Page fields (authored by @charlieegan3)
- docs: Update TLS-based Authentication Example ([#5521](https://github.com/open-policy-agent/opa/issues/5521)) authored by @charlieegan3 reported by @jjthom87
- docs: Update opa eval flags to link to bundle docs (authored by @charlieegan3)
- docs: Make SDK first option for Go integraton (authored by @anderseknert)
- docs: Fix typo on Policy Language page.  (authored by @mcdonagj)
- docs/integrations: Update kubescape repo links (authored by @dwertent)
- docs/oci: Corrected config section (authored by @ogazitt)
- website/frontpage: Update Learn More links (authored by @pauly4it)
- integrations.yaml: Ensure inventors listed in organizations (authored by @anderseknert)
- integrations: Fix malformed inventors item (authored by @anderseknert)
- Add Digraph to ADOPTERS.md (authored by @jamesphlewis)

### Miscellaneous

- Remove changelog maintainer mention filter (authored by @anderseknert)
- Chore: Fix len check in the `ast/visit_test` error message (authored by @boranx)
- `opa inspect`: Fix wrong windows bundle tar files path separator (authored by @shm12)
- Add CHANGELOG.md to website build triggers (authored by @srenatus)

Dependency bumps:
- Golang 1.19.3 -> 1.19.4
- github.com/containerd/containerd from 1.6.10 -> 1.6.15
- github.com/dgraph-io/badger/v3
- golang.org/x/net to 0.5.0
- json5 and postcss-modules
- oras.land/oras-go from 1.2.1 -> 1.2.2

CI/Distribution fixes:
- Update base images for non debug builds (authored by @charlieegan3)
- Remove deprecated linters in golangci config (authored by @yanggangtony)