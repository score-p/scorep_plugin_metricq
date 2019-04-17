# Usage

### Required Environment Variables

    SCOREP_METRIC_PLUGINS=metricq_plugin
    SCOREP_METRIC_METRICQ_PLUGIN=metric.name
    SCOREP_METRIC_METRICQ_PLUGIN_SERVER=amqps://user:pass@hostname
    
### Optional Environment Variables

    SCOREP_METRIC_METRICQ_PLUGIN_VERBOSE=debug
    SCOREP_METRIC_METRICQ_PLUGIN_TIMEOUT=600s

Control the time synchronization.
It will take `quantum * 2 ^ exponent` for each sync phase. 

    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_EXPONENT=11
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_QUANTUM=1ms

Because Score-P default settings are insufficient:

    SCOREP_ENABLE_PROFILING=false
    SCOREP_ENABLE_TRACING=true
