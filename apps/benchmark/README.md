# benchmark

This application connects to another mojo application, collects traces during
indicated period of time and computes a number of results based on the collected
traces. It can be used to measure performance of a mojo app, provided that the
app being benchmarked participates in the [tracing
ecosystem](../../mojo/services/tracing/interfaces/tracing.mojom).

## Arguments

The benchmarking app **requires** the following arguments:

 - `--app=<app_url>` - url of the application to be benchmarked
 - `--duration=<duration_seconds>` - duration of the benchmark in seconds

The following arguments are **optional**:

 - `--trace-output=<output_file_path>` - local file path at which the collected trace
   will be written

Any other arguments are assumed to be descriptions of measurements to be
conducted on the collected trace data. Each measurement has to be of form:
`<measurement_type>/<measurement_arg_1>/<measurement_arg_2>/...`.

The following measurement types are available:

 - `time_until/<trace_event_category>/<trace_event_name>` - measures time until
   the first occurence of the event named `trace_event_name` in category
   `trace_event_category`.
 - `time_between/<first_event_category>/<first_event_name>/<second_event_category>/<second_event_name>`
   measures time between the first occurence of the event named
   `<first_event_name>` in category `<first_event_category>` and the first occurence
   of the event named `<second_event_name>` in category
   `<second_event_category>`. The measurement will fail if the first occurence
   of the first event happens after the first occurence of the second event.
 - `avg_duration/<trace_event_category>/<trace_event_name>` - measures average
   duration of all events named `trace_event_name` in category
   `trace_event_category`.
 - `percentile_duration/<trace_event_category>/<trace_event_name>/0.XX` -
   measures the value at the XXth percentile of all events named
   `trace_event_name` in category `trace_event_category`. E.g.
   `.../<trace_event_name/0.50` will give the 50th percentile.

## Runner script

Devtools offers [a helper script](../../mojo/devtools/common/mojo_benchmark)
allowing to run a list of benchmarks in controlled caching conditions, both
on **Android** and **Linux**.
