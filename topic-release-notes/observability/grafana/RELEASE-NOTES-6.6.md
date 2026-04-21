# grafana v6.6 Release Notes

Source: [v6.6.2](https://github.com/grafana/grafana/releases/tag/v6.6.2)

[Download Page](https://grafana.com/grafana/download/6.6.2)
[What's New Highlights](https://grafana.com/docs/guides/whats-new-in-v6-6)
[Release Notes](https://community.grafana.com/t/release-notes-v6-6-x/24924)

# 6.6.2 (2020-02-20)

### Features / Enhancements
* **Data proxy**: Log proxy errors using Grafana logger. [#22174](https://github.com/grafana/grafana/pull/22174), [@bergquist](https://github.com/bergquist)
* **Metrics**: Add gauge for requests currently in flight. [#22168](https://github.com/grafana/grafana/pull/22168), [@bergquist](https://github.com/bergquist)

### Bug Fixes
* **@grafana/ui**: Fix displaying of bars in React Graph. [#21968](https://github.com/grafana/grafana/pull/21968), [@ivanahuckova](https://github.com/ivanahuckova)
* **API**: Fix redirect issue when configured to use a subpath. [#21652](https://github.com/grafana/grafana/pull/21652), [@briangann](https://github.com/briangann)
* **API**: Improve recovery middleware when response already been written. [#22256](https://github.com/grafana/grafana/pull/22256), [@marefr](https://github.com/marefr)
* **Auth**: Don't rotate auth token when requests are cancelled by client. [#22106](https://github.com/grafana/grafana/pull/22106), [@bergquist](https://github.com/bergquist)
* **Docker**: Downgrade to 18.04 LTS base image. [#22313](https://github.com/grafana/grafana/pull/22313), [@aknuds1](https://github.com/aknuds1)
* **Elasticsearch**: Fix auto interval for date histogram in explore logs mode. [#21937](https://github.com/grafana/grafana/pull/21937), [@ivanahuckova](https://github.com/ivanahuckova)
* **Image Rendering**: Fix PhantomJS compatibility with es2016 node dependencies. [#21677](https://github.com/grafana/grafana/pull/21677), [@dprokop](https://github.com/dprokop)
* **Links**: Assure base url when single stat, panel and data links are built. [#21956](https://github.com/grafana/grafana/pull/21956), [@dprokop](https://github.com/dprokop)
* **Loki, Prometheus**: Fix PromQL and LogQL syntax highlighting. [#21944](https://github.com/grafana/grafana/pull/21944), [@ivanahuckova](https://github.com/ivanahuckova)
* **OAuth**: Enforce auto_assign_org_id setting when role mapping enabled using Generic OAuth. [#22268](https://github.com/grafana/grafana/pull/22268), [@aknuds1](https://github.com/aknuds1)
* **Prometheus**: Updates explore query editor to prevent it from throwing error on edit. [#21605](https://github.com/grafana/grafana/pull/21605), [@Estrax](https://github.com/Estrax)
* **Server**: Reorder cipher suites for better security. [#22101](https://github.com/grafana/grafana/pull/22101), [@tofu-rocketry](https://github.com/tofu-rocketry)
* **TimePicker**: fixing weird behavior with calendar when switching between months/years . [#22253](https://github.com/grafana/grafana/pull/22253), [@mckn](https://github.com/mckn)
