# opentelemetry-collector v0.86 Release Notes

Source: [v0.86.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.86.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.86.0

## User facing changes

### 🚩 Deprecations 🚩

- `loggingexporter`: Mark the logging exporter as deprecated, in favour of debug exporter (#7769)

### 🚀 New components 🚀

- `debugexporter`: Add debug exporter, which replaces the logging exporter (#7769)

### 💡 Enhancements 💡

- `featuregate`: List valid feature gates when failing to load invalid gate (#8505)
- `supported platforms`: Add `linux/s390x` architecture to cross build tests in CI (#8213)

### 🧰 Bug fixes 🧰

- `builder`: fix setting `dist.*` keys from env (#8239)
- `configtls`: fix incorrect use of fsnotify (#8438)

## API changes

### 🛑 Breaking changes 🛑

- `service`: remove deprecated service.PipelineConfig (#8485)

### 🚩 Deprecations 🚩

- `obsreporttest`: deprecate To*CreateSettings funcs in obsreporttest (#8492)
  The following TestTelemetry methods have been deprecated. Use structs instead:
  -  ToExporterCreateSettings -> exporter.CreateSettings
  -  ToProcessorCreateSettings -> processor.CreateSettings
  -  ToReceiverCreateSettings -> receiver.CreateSettings
  
- `obsreport`: Deprecating `obsreport.Exporter`, `obsreport.ExporterSettings`, `obsreport.NewExporter` (#8492)
  These deprecated methods/structs have been moved to exporterhelper:
  - `obsreport.Exporter` -> `exporterhelper.ObsReport`
  - `obsreport.ExporterSettings` -> `exporterhelper.ObsReportSettings`
  - `obsreport.NewExporter` -> `exporterhelper.NewObsReport`
  
- `obsreport`: Deprecating `obsreport.BuildProcessorCustomMetricName`, `obsreport.Processor`, `obsreport.ProcessorSettings`, `obsreport.NewProcessor` (#8492)
  These deprecated methods/structs have been moved to processorhelper:
  - `obsreport.BuildProcessorCustomMetricName` -> `processorhelper.BuildCustomMetricName`
  - `obsreport.Processor` -> `processorhelper.ObsReport`
  - `obsreport.ProcessorSettings` -> `processorhelper.ObsReportSettings`
  - `obsreport.NewProcessor` -> `processorhelper.NewObsReport`
  
- `obsreport`: Deprecating obsreport scraper and receiver API (#8492)
  These deprecated methods/structs have been moved to receiverhelper and scraperhelper:
  - `obsreport.Receiver` -> `receiverhelper.ObsReport`
  - `obsreport.ReceiverSettings` -> `receiverhelper.ObsReportSettings`
  - `obsreport.NewReceiver` -> `receiverhelper.NewObsReport`
  - `obsreport.Scraper` -> `scraperhelper.ObsReport`
  - `obsreport.ScraperSettings` -> `scraperhelper.ObsReportSettings`
  - `obsreport.NewScraper` -> `scraperhelper.NewObsReport`
  

### 💡 Enhancements 💡

- `otelcol`: Splitting otelcol into its own module. (#7924)
- `service`: Split service into its own module (#7923)