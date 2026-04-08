# opa v1.12 Release Notes

Source: [v1.12.3](https://github.com/open-policy-agent/opa/releases/tag/v1.12.3)

v1.12.3


This is a bug fix release addressing two issues:

###  Bundle polling is being misconfigured when discovery bundle is updated ([#8215](https://github.com/open-policy-agent/opa/issues/8215))

This is an issue where the polling interval for discovery (`discovery.polling.min_delay_seconds` and `discovery.polling.max_delay_seconds`) were misinterpreted on reconfiguration, causing extremely long update intervals.

Reported by @loganmiller-chime, authored by @sspaink

### Decision log `size` buffer `buffer_size_limit_bytes` misconfigured during reconfiguration ([#8213](https://github.com/open-policy-agent/opa/pull/8213))

This is a regression in the decision log, where the `decision_logs.reporting.buffer_size_limit_bytes` was mistakenly assigned the value of `decision_logs.reporting.upload_size_limit_bytes` during reconfiguration.
This issue is only present when `decision_logs.reporting.buffer_type` is set to `size`, which is the default value.

Authored by @sspaink

