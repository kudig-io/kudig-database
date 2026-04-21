# thanos v0.36 Release Notes

Source: [v0.36.1](https://github.com/thanos-io/thanos/releases/tag/v0.36.1)

This patch release brings a few fixes! Please try it out and let us know if you face issues! 🚀

## Changelog

### Fixed

- [#7634](https://github.com/thanos-io/thanos/pull/7634) Rule: fix Query and Alertmanager TLS configurations with CA only.
- [#7618](https://github.com/thanos-io/thanos/pull/7618) Proxy: Query goroutine leak when store.response-timeout is set