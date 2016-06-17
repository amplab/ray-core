# mojo_benchmark

`mojo_benchmark` allows you to run performance tests for any Mojo application
that participates in the [tracing
ecosystem](https://github.com/domokit/mojo/blob/master/mojo/services/tracing/interfaces/tracing.mojom)
with no app changes required.

The script reads a list of benchmarks to run from a file, runs each benchmark in
controlled caching conditions with tracing enabled and performs specified
measurements on the collected trace data.

## Defining benchmarks

`mojo_benchmark` runs performance tests defined in a benchmark file. The
benchmark file is a Python program setting a dictionary of the following format:

```python
benchmarks = [
  {
    'name': '<name of the benchmark>',
    'app': '<url of the app to benchmark>',
    'shell-args': [],
    'duration': <duration in seconds>,

    # List of measurements to make.
    'measurements': [
      {
        'name': <my_measurement>,
        'spec': <spec>,
      },
      (...)
    ],
  },
]
```

The benchmark file may reference the `target_os` global that will be any of
('android', 'linux'), indicating the system on which the benchmarks are run.

### Measurement specs

The following types of measurements are available:

  - `time_until`
  - `time_between`
  - `avg_duration`
  - `percentile_duration`

`time_until` records the time until the first occurence of the targeted event.
The underlying benchmark runner records the time origin just before issuing the
connection call to the application being benchmarked. Results of `time_until`
measurements are relative to this time. Spec format:

```
'time_until/<category>/<event>'
```

`time_between` records the time between the first occurence of the first
targeted event and the first occurence of the second targeted event. Spec
format:

```
'time_between/<category1>/<event1>/<category2>/<event2>'
```

`avg_duration` records the average duration of all occurences of the targeted
event. Spec format:

```
'avg_duration/<category>/<event>'
```

`percentile_duration` records the value at the given percentile of durations of
all occurences of the targeted event. Spec format:

```
'percentile_duration/<category>/<event>/<percentile>'
```

where `<percentile>` is a number between 0.0 and 0.1.

## Caching

The script runs each benchmark twice. The first run (**cold start**) clears
caches of the following apps on startup:

 - `network_service.mojo`
 - `url_response_disk_cache.mojo`

The second run (**warm start**) runs immediately afterwards, without clearing
any caches.

## Example

For an app that records a trace event named "initialized" in category "my_app"
once its initialization is complete, we can benchmark the initialization time of
the app (from the moment someone tries to connect to it to the app completing
its initialization) using the following benchmark file:

```python
benchmarks = [
  {
    'name': 'My app initialization',
    'app': 'https://my_domain/my_app.mojo',
    'duration': 10,
    'measurements': [
      'time_until/my_app/initialized',
    ],
  },
]
```

## Dashboard

`mojo_benchmark` supports uploading the results to an instance of a Catapult
performance dashboard. In order to upload the results of a run to performance
dashboard, pass the `--upload` flag along with required meta-data describing the
data being uploaded:

```sh
mojo_benchmark \
--upload \
--master-name my-master \
--bot-name my-bot \
--test-name my-test-suite
--builder-name my-builder \
--build-number my-build
--server-url http://my-server.example.com
```

If no `--server-url` is specified, the script assumes that a local instance of
the dashboard is running at `http://localhost:8080`. The script assumes that the
working directory from which it is called is a git repository and queries it to
determine the sequential number identifying the revision (as the number of
commits in the current branch in the repository).

For more information refer to:

 - [Catapult project](https://github.com/catapult-project/catapult)
