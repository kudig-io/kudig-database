# opa v1.11 Release Notes

Source: [v1.11.1](https://github.com/open-policy-agent/opa/releases/tag/v1.11.1)

This is a bugfix release:

### Memory exhaustion via forged gzip header

A crafted HTTP request any of OPA's HTTP endpoints would lead OPA to use a large amount of memory, triggering
an out-of-memory process exit.

This weakness in OPA's HTTP API gzip handling is as old as the gzip handling itself. [A configurable limit was introduced in v0.67.0](https://github.com/open-policy-agent/opa/blob/v0.67.0/CHANGELOG.md#request-body-size-limits), but it has been shown that this security measure wasn't sufficient to avoid running out of memory in memory-constrained setups.

Thanks to @thevilledev for reporting and fixing this issue.

It only applies to OPA running as server (as a binary or in a container, as "sidecar"). To trigger an OOM process exit using this weakness, an adversary must be able to send an HTTP request directly to OPA. This would be the case if they are in the same network, there is no proxy in front of OPA, or if OPA was exposed to the internet, which is advised against.

By the nature of HTTP encodings, this would be effective **before** _token-based authentication_ and _authorization policies_, so these measures do not protect against the attack vector.

If all OPA endpoints are using [TLS-based authentication](https://www.openpolicyagent.org/docs/security#tls-based-authentication-example) (mutual TLS, "mTLS"), then an adversary cannot do harm with this method.

Please note that while we're taking all of these issues seriously, OPA isn't designed for adversary environments. It's strongly advised not to expose any of its endpoints to the public internet. Furthermore, available security measures should be applied **regardless**, for a defense in depth approach. [See the documentation for the available means of authentication and authorization in OPA.](https://www.openpolicyagent.org/docs/security)

Please also check out our [Security Policy](https://www.openpolicyagent.org/security) for reporting critical issues and bugs.

### Decision Logs dropped (introduced in OPA v1.9.0)

When the decision logs buffer was uploaded, the buffer limit inadvertently got reset to the default upload limit (32kb).
This causes logs to be dropped that shouldn't have been dropped.

This default is overridden by the configuration value `decision_logs.reporting.upload_size_limit_bytes`, see [the docs on decision logs](https://www.openpolicyagent.org/docs/configuration#decision-logs).

There's a Prometheus metric for dropped events, `counter_decision_logs_dropped_buffer_size_limit_bytes_exceeded`,
and you can check that for unexpectedly high counts.

Reported by @johanneslarsson [#8123](https://github.com/open-policy-agent/opa/issues/8123), fixed by @sspaink.


The release is otherwise identical to v1.11.0.

