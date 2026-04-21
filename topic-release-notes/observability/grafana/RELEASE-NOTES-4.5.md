# grafana v4.5 Release Notes

Source: [v4.5.2](https://github.com/grafana/grafana/releases/tag/v4.5.2)

[Download Page](https://grafana.com/grafana/download/4.5.2)
[Installation Guide](http://docs.grafana.org/installation/)

[Release Notes](https://community.grafana.com/t/release-notes-for-grafana-v4-5-0/2573)

## Fixes 
* **Graphite**: Fix for issues with jsonData & graphiteVersion null errors [#9258](https://github.com/grafana/grafana/issues/9258)
* **Graphite**: Fix for Grafana internal metrics to Graphite sending NaN values [#9279](https://github.com/grafana/grafana/issues/9279)
* **HTTP API**: Fix for HEAD method requests [#9307](https://github.com/grafana/grafana/issues/9307)
* **Templating**: Fix for duplicate template variable queries when refresh is set to time range change [#9185](https://github.com/grafana/grafana/issues/9185)
* **Metrics**: dont write NaN values to graphite [#9279](https://github.com/grafana/grafana/issues/9279)
