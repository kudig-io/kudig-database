# opa v0.56 Release Notes

Source: [v0.56.0](https://github.com/open-policy-agent/opa/releases/tag/v0.56.0)

This release contains a mix of new features, bugfixes and a new builtin function.

### Support for General References in Rule Heads (Experimental)

A new experimental feature in OPA is support for general refs in rule heads. Where a general ref is a reference with variables at arbitrary locations.

```rego
package example

import future.keywords

# Converting a flat list of users to a mapping by "role" and then "id".
users_by_role[role][id] := user if {
    some user in data.users
    id := user.id
    role := user.role
}

# Explicit "admin" key override to the above mapping.
users_by_role.admin[id] := user if {
    some user in data.admins
    id := user.id
}

# Leaf entries can be multi-value.
users_by_country[country] contains user.id if {
    some user in data.users
    country := user.country
}
```

General refs are currently not supported by the OPA planner, making this feature unsupported for Wasm and IR.

Note: this feature is disabled by default, and needs to be enabled by setting the `EXPERIMENTAL_GENERAL_RULE_REFS` environment variable (once the feature is complete - supports Wasm and IR - this requirement will be dropped).

Authored by @johanfylling.

### New Built-In Function: `numbers.range_step`

Similar to the `numbers.range` built-in function, `numbers.range_step` returns an array of numbers in a given range. The new built-in function also allows you to control the _step between each entry_.

See [the documentation on the new built-in](https://www.openpolicyagent.org/docs/v0.56.0/policy-reference/#builtin-numbers-numbersrange_step)
for all the details.

Authored by @sspaink.

### New Ecosystem page on The Website

The OPA Ecosystem of related integrations has been refreshed and moved to a more prominent location on [the website](https://www.openpolicyagent.org/ecosystem/). 

If you're interested to add any new integrations you've been working on, please see the [docs here](https://github.com/open-policy-agent/opa/tree/main/docs#opa-ecosystem) (updates to existing integrations are very welcome too!).

### Runtime, Tooling, SDK

- ast: Update strict error check message for unused args ([#6125](https://github.com/open-policy-agent/opa/pull/6125)) authored by @ashutosh-narkar
- ast: Remove unnecessary nil check ([#6155](https://github.com/open-policy-agent/opa/pull/6155)) authored by @Juneezee
- cmd: Make `opa test -z` fail with failing tests ([#6126](https://github.com/open-policy-agent/opa/issues/6126)) authored by @fdaguin
- cmd: Fix `opa test` `--ignore` when used together with `--bundle` ([#6185](https://github.com/open-policy-agent/opa/pull/6185)) authored by @joaobrandt
- cmd: Adding `--fail-non-empty` flag to `opa exec` ([#6153](https://github.com/open-policy-agent/opa/pull/6153)) authored by @Ronnie-personal
- download: Add `opa_no_oci` flag to build without containerd ([#6159](https://github.com/open-policy-agent/opa/pull/6159)) authored by @slonka
- download: Remove not required basedir for oci bundles & add test to verify signature verification ([#6145](https://github.com/open-policy-agent/opa/pull/6145)) authored by @gitu
- fmt: Trim trailing whitespace in comments ([#6161](https://github.com/open-policy-agent/opa/issues/6161)) authored by @anderseknert
- fmt: Remove dedup comment function in opa fmt ([#6165](https://github.com/open-policy-agent/opa/pull/6165)) authored by @anderseknert
- runtime: Always read .tar.gz file provided in argument as a bundle ([#5879](https://github.com/open-policy-agent/opa/issues/5879)) authored by @yogisinha
- server/authorizer: Inline readBody ([#6156](https://github.com/open-policy-agent/opa/pull/6156)) authored by @srenatus
- test: Bind test server to localhost interface ([#6162](https://github.com/open-policy-agent/opa/issues/6162)) authored by @anderseknert

### Topdown and Rego

- ast: Including "child" rules when fetching rules by ref ([#6182](https://github.com/open-policy-agent/opa/issues/6182)) authored by @johanfylling
- ast: Making partial object key rules contribute to dynamic portion of object type ([#6138](https://github.com/open-policy-agent/opa/issues/6138)) authored by @johanfylling
- rego: Expose PrepareOption, add BuiltinFuncs ([#6188](https://github.com/open-policy-agent/opa/pull/6188)) authored by @srenatus
- topdown: Support force cache even when server doesn't set the Date header ([#6175](https://github.com/open-policy-agent/opa/pull/6175)) authored by @c2zwdjnlcg
- topdown: Partial-eval for partial object/set ref head rules ([#6094](https://github.com/open-policy-agent/opa/issues/6094)) authored by @johanfylling

### Miscellaneous

- Updates to Documentation and Website (authored by: @anderseknert, @ashutosh-narkar, @atkrad, @charlieegan3, @hmoazzem, @johndbro1, @Pushkarm029, @srenatus and @testwill)
- Dependency updates; notably:
  - golang: from 1.20.6 to 1.21 (authored by @ashutosh-narkar amd @srenatus)
  - golang.org/x/net from 0.12.0 to 0.14.0
  - google.golang.org/grpc from 1.56.2 to 1.57.0
  - oras.land/oras-go/v2 from 2.2.1 to 2.3.0
  - Replace ghodss/yaml with sigs.k8s.io/yaml ([#6195](https://github.com/open-policy-agent/opa/pull/6195)) authored by @mrueg

### Breaking changes

Since its introduction in 0.34.0, the `--exit-zero-on-skipped` option always made the `opa test` command return an exit code 0. When used, it now returns the exit code 0 only if no failed tests were found.

Test runs on existing projects using `--exit-zero-on-skipped` will fail if any failed tests were inhibited by this behavior.