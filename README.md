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
    It is strongly recommended to specify a value.
    If the value is too large and your measurement crashes, it may result in
    resource exhaustion of the measurement infrastructure.
    The value is a duration string which will be parsed, e.g. `1 hour`.

* `SCOREP_METRIC_METRICQ_PLUGIN_TOKEN` (optional, default: "sink-scorep")

    Base token to use within MetricQ.
    A uuid is added internally. Default: 
    
* `SCOREP_METRIC_METRICQ_PLUGIN_VERBOSE` (optional, default: `info`)

    Control output verbosity. Use one of `trace,debug,info,warn,error,fatal`.

* `SCOREP_METRIC_METRICQ_PLUGIN_AVERAGE` (optional, default: `0`)

    Combine multiple high-resolution (>= 1 kSa/s) values to one.
    Set to e.g. `8` to reduce the number of values by a factor of `8`.
    A setting of `0` disables averaging.
 

#### Time synchronization

Control the time synchronization.
Each sync phase will last `quantum * 2 ^ exponent + 2 * tolerance`. 

    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_EXPONENT=11
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_QUANTUM=1ms
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_TOLERANCE=2s

* `SCOREP_METRIC_METRICQ_PLUGIN_CORRELATION_FILE` (optional)

    Prefix for writing a file containing correlation values for all offsets.
    This is only used for the most hardcore timesync debugging.
    
Time synchronization is only applied to metrics with >= 1 kSa/s.
It uses the first of such metrics to determine the offset, so be wary of the order in which metrics are specified.
Using wildcards is not recommended with that.

#### Recommended Score-P settings

Because the Score-P default settings are not appropriate for many use-cases:

    SCOREP_ENABLE_PROFILING=false
    SCOREP_ENABLE_TRACING=true
