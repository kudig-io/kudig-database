---
title: opa v0.40 Release Notes
description: opa v0.40 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- opa
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.40 Release Notes 是什么
- 如何 opa v0.40 Release Notes
trigger_keywords:
- opa
- v0.40
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
- observability-basics
---

# opa v0.40 Release Notes

Source: [v0.40.0](https://github.com/open-policy-agent/opa/releases/tag/v0.40.0)

This release contains a number of fixes and enhancements.

### Metadata introspection

The _rich metadata_ added in the v0.38.0 release can now be introspected from the policies themselves!

    package example

    # METADATA
    # title: Edits by owner only
    # description: |
    #   Only the owner is allowed to edit their data.
    deny[{"allowed": false, "message": rego.metadata.rule().description}] {
        input.user != input.owner
    }

This snippet will evaluate to

    [{
      "allowed": false,
      "message": "Only the owner is allowed to edit their data.\n"
    }]

Both the rule's metadata can be accessed, via `rego.metadata.rule()`, and the entire chain of metadata attached to the rule via the various scopes that different metadata annotations can have, via `rego.metadata.chain()`.

All the details can be found in the documentation of [these new built-in functions](https://www.openpolicyagent.org/docs/v0.40.0/policy-reference/#rego).

### Function mocking

It is now possible to **mock functions** in tests! Both built-in and non-built-in functions can be mocked:

    package authz
    import data.jwks.cert
    import data.helpers.extract_token

    allow {
        [true, _, _] = io.jwt.decode_verify(extract_token(input.headers), {"cert": cert, "iss": "corp.issuer.com"})
    }

    test_allow {
        allow
          with input.headers as []
          with data.jwks.cert as "mock-cert"
          with io.jwt.decode_verify as [true, {}, {}] # mocked built-in
          with extract_token as "my-jwt"              # mocked non-built-in
    }

For further information about policy testing with data and function mock, see [the Policy Testing docs](https://www.openpolicyagent.org/docs/v0.40.0/policy-testing/#data-and-function-mocking). All details about `with` can be found in its [Policy Language section](https://www.openpolicyagent.org/docs/v0.40.0/policy-language/#with-keyword).

This has been a much-requested feature, but it's @rmetcalf9's issue [#4449](https://github.com/open-policy-agent/opa/issues/4449) that nudged this feature ahead.

### Assignments with `:=`

Remaining restrictions around the use of `:=` in rules and functions have been lifted ([#4555](https://github.com/open-policy-agent/opa/issues/4555)). These constructs are now valid:

    check_images(imgs) := x { # function
      # ...
    }

    allow := x { # rule
      # ...
    }

    response[key] := object { # partial object rule
      # ...
    }

In the wake of this, rules may now be "redeclared", i.e. you can use `:=` for more than one rule body:

    deny := x {
      # body 1
    }
    deny := x {
      # body 2
    }

This was forbidden before, but didn't serve a real purpose: it would catch trivial-to-catch errors
like

    p := 1
    p := 2 # redeclared

But it would do no good in more difficult to debug "multiple assignment" problems like

    p := x {
      some x in [1, 2, 3]
    }

### Tooling, SDK, and Runtime

- Status Plugin: Remove activeRevision label on all but one Prometheus metric ([#4584](https://github.com/open-policy-agent/opa/issues/4584)) reported and authored by @costimuraru
- Status: Include bundle type ("snapshot" or "delta") in status information
- `opa capabilities`: Expose capabilities through CLI, and allow using versions when passing `--capabilities v0.39.0` to the various commands ([#4236](https://github.com/open-policy-agent/opa/issues/4236)) authored by @IoannisMatzaris <!-- FC -->
- Logging: Log warnings at WARN level not ERROR, authored by @damienjburks
- Runtime: Persist activated bundle Etag to store ([#4544](https://github.com/open-policy-agent/opa/issues/4544))
- `opa eval`: Don't use source locations when formatting partially evaluated output ([#4609](https://github.com/open-policy-agent/opa/issues/4609))
- `opa inspect`: Fixing an issue where some errors encountered by the inspect command aren't properly reported
- `opa fmt`: Fix a bug with missing whitespace when formatting multiple `with` statements on one indented line ([#4634](https://github.com/open-policy-agent/opa/issues/4634))

#### Experimental OCI support

When configured to do so, OPA's bundle and discovery plugins will retrieve bundles from **any OCI registry**. Please see [the Services Configuration section](https://www.openpolicyagent.org/docs/v0.40.0/configuration/#services) for details.

Note that at this point, it's best considered a "feature preview". Be aware of this:
- Bundles are not cached, but re-retrieved and activated periodically.
- The persistence directory used for storing retrieved OCI artifacts is not yet managed by OPA,
  so its content may accumulate. By default, the OCI downloader will use a temporary file location.
- The documentation on how to push bundles to an OCI repository currently only exists in the development
  docs, see [OCI.md](https://github.com/open-policy-agent/opa/blob/v0.40.0/docs/devel/OCI.md).

Thanks to @carabasdaniel for starting the work on this!

### Rego and Topdown

- Builtins: Require prefix length for IPv6 in `net.cidr_merge` ([#4596](https://github.com/open-policy-agent/opa/issues/4596)), reported by @alexhu20
- Builtins: `http.send` can now parse and cache YAML responses, analogous to JSON responses
- Parser: Guard against invalid domains for "some" and "every", reported by @doyensec
- Formatting: Don't add 'in' keyword import when 'every' is there ([#4606](https://github.com/open-policy-agent/opa/issues/4606))

### Documentation

- Policy Language: Reorder Universal Quantification content, stress `every` over other constructions ([#4603](https://github.com/open-policy-agent/opa/issues/4603))
- Language pages: Use assignment operator where it's allowed.
- SSH Tutorial: Use bundle API
- Annotations: Update "Custom" annotation section
- Cloudformation: Fix markup and add warning related to booleans
- Blogs: mention OAuth2 and OIDC blog posts

### Website + Ecosystem

- Redirect previous patch releases to latest patch release ([#4225](https://github.com/open-policy-agent/opa/issues/4225))
- Add playground button to navbar
- Add SRI to static html files
- Remove right margin on sidebar (#4529) (authored by @orweis)
- Show yellow banner for old version (#4533)
- Remove unused variables to avoid error in strict mode(#4534) (authored by @panpan0000)
- Ecosystem:
  - Add AWS CloudFormation Hook
  - Add GKE policy automation
  - Add permit.io (authored by @ozradi)
  - Add Magda (authored by @t83714) 

### Miscellaneous

- Workflow: no content permissions for GitHub action 'post-release', authored by @naveensrinivasan
- Various dependency bumps, notably:
  - OpenTelemetry-go: 1.6.1 -> 1.6.3
  - go.uber.org/automaxprocs: 1.4.0 -> 1.5.1
- Binaries and Docker images are now built using Go 1.18.1.
- Dockerfile: add source annotation (#4626)