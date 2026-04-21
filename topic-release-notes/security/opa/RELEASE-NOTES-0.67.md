# opa v0.67 Release Notes

Source: [v0.67.1](https://github.com/open-policy-agent/opa/releases/tag/v0.67.1)

This is a bug fix release addressing the following issue:

- util+server: Fix bug around chunked request handling ([#6906](https://github.com/open-policy-agent/opa/pull/6906)) authored by @philipaconrad, reported by @David-Wobrock. A request handling bug was introduced in ([#6868](https://github.com/open-policy-agent/opa/pull/6868)), which caused OPA to treat all incoming chunked requests as if they had zero-length request bodies.

