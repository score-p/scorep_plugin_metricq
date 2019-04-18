# Usage

### Required Environment Variables

    SCOREP_METRIC_PLUGINS=metricq_plugin
    SCOREP_METRIC_METRICQ_PLUGIN=metric.name
    SCOREP_METRIC_METRICQ_PLUGIN_SERVER=amqps://user:pass@hostname
    
### Optional Environment Variables

    SCOREP_METRIC_METRICQ_PLUGIN_VERBOSE=debug
    SCOREP_METRIC_METRICQ_PLUGIN_TIMEOUT=600s

Control the time synchronization.
Each sync phase will last `quantum * 2 ^ exponent + 2 * tolerance`. 

    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_EXPONENT=11
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_QUANTUM=1ms
    SCOREP_METRIC_METRICQ_PLUGIN_SYNC_TOLERANCE=2s

Because Score-P default settings are insufficient:

    SCOREP_ENABLE_PROFILING=false
    SCOREP_ENABLE_TRACING=true
