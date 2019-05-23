## Usage
### Environment Variables

#### General

* `SCOREP_METRIC_PLUGINS=metricq_plugin`

* `SCOREP_METRIC_METRICQ_PLUGIN` (required)

    Comma-separated list of metrics.
    Metrics can also contain wildcards (`*`)

* `SCOREP_METRIC_METRICQ_PLUGIN_SERVER` (required)

    URL to the main MetricQ AMQP server including user/password.

* `SCOREP_METRIC_METRICQ_PLUGIN_TIMEOUT` (optional, default: 1 hour)

    Maximum duration of the experiment.
    It is highly recommended to specify a value.
    The value is a duration string which will be parsed, e.g. `1 hour`.

* `SCOREP_METRIC_METRICQ_PLUGIN_TOKEN` (optional, default: "sink-scorep")

    Base token to use within MetricQ.
    A uuid is added internally. Default: 
    
* `SCOREP_METRIC_METRICQ_PLUGIN_VERBOSE` (optional, default: `info`)

    Control output verbosity. Use one of `trace,debug,info,warn,error,fatal`.

#### Time synchronization

Control the time synchronization.
Each sync phase will last `quantum * 2 ^ exponent + 2 * tolerance`. 

    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_EXPONENT=11
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_QUANTUM=1ms
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_TOLERANCE=2s

#### Recommended Score-P settings

Because the Score-P default settings are dumb:

    SCOREP_ENABLE_PROFILING=false
    SCOREP_ENABLE_TRACING=true
