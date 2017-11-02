A command line utility for interacting with the SAVIOR Sensing API.

```bash
> python virtue-security version
virtue-security(version=1)
sensing-api(version=2017.11.1)
```

# Installing

The `virtue-security` command line tool is built with Python 3.6, and depends on a small
number of libraries. It it recommended that you run `virtue-security` in a virtual-env:

```bash
virtualenv -p python3 ./venv 
```

Once you've setup a virtualenv, load the environment and install dependencies:

```bash
. ./venv/bin/activate
pip install -r requirements.txt
```

You can test the command line tool by calling the `version` action, which will report
the version of the local CLI tool, as well as the version of the remote Sensing API, if
it's available:

```bash
> python virtue-security version
virtue-security(version=1)
sensing-api(version=2017.11.1)
```

# Configuration

```bash
python virtue-security version --api-host localhost --api-port 4000 --api-version v1
```

The `virtue-security` command can be configured via command line flags. Available
configuration flags:

 - `--api-host` or `-a` : Hostname of the Sensing API, default is `localhost`
 - `--api-port` or `-p` : Port on the Hostname for the Sensing API, default is `4000`
 - `--api-version` : REST API Version of the Sensing API to use, default is `v1`
 
# Commands

Command line functionality is broken up over a series of commands. Each command responds
to a particular set of flags. All commands will honor the `--api-host`, `--api-port`, and
`--api-version` flags.

## version

```bash
> python virtue-security version --api-host localhost --api-port 4000
virtue-security(version=1)
sensing-api(version=2017.11.1)
```

Get the runtime version of the `virtue-security` CLI and the remote Sensing API.

### Flags

None.

## test

```bash
> python virtue-security test
virtue-security/test - running API endpoint response tests
  :: 183 tests available
  :: 183 tests matching --test-path filter
	PUT /network/:cidr/observe/:level
		5/5 passed
	PUT /network/virtue/:virtue/observe/:level
		5/5 passed
		
...snip...

	GET /enum/log/levels
		1/1 passed
  :: TEST SUMMARY

  :: 887 passed, 0 failed out of 887 total tests
       100.00% pass rate
       100.00% validator coverage
       183 test cases
       183 test cases with results validators
```

Run the Sensing API verification test suite. These tests assess the functionality of 
the remote Sensing API in response to valid and invalid content and requests. Each
response is validated for HTTP Status code, format, and response JSON content.

The tests are run in authenticated and unauthenticated mode, and, where appropriate,
parameter fuzzing tests are run to verify functionality.

Tests are specified using the same request path parameterization as used by the Sensing
API. Request URIs are then generated at run time using randomized values that match
the request path specification. So, for instance, both the test command and the Sensing API
represent a Sensor ID in their request paths as `:sensor`, and parse or replace that with
a UUID at run time.

### Flags

 - `--test-path` : Filter the tests run by prefix matching against a supplied request PATH

```bash
> python virtue-security test --test-path "/user/:username/stream"
``` 
 

# Testing

The `virtue-security` CLI and the Sensing API are covered by extensive functional tests. Tests
are specified in the `test_api` method. Each test specification follows a common format. Test
paths are parameterized by keyword replacement of placeholder values that start with a colon.

The following keyword replacements currently work:

 - `:application` - Application ID (uuid4)
 - `:resource` - Resource ID (uuid4)
 - `:sensor` - Sensor ID (uuid4)
 - `:username` - Username, currently using the pattern `r'[a-zA-Z0-9]+\.[a-zA-Z0-9]+)'`
 - `:since` - Timestamp from which to stream historic logs, formatted as `2017-11-02T15:48:56.837769`
 - `:validate_action` - Sensor validation action. One of `cross-validation` or `canary`
 - `:filter_level` - Log level below which to filter out messages when streaming. One of `["everything", "debug", "info", "warning", "error", "event"]`
 - `:follow` - Follow the tail of the logs when streaming, one of `True` or `False`
 - `:action` - Trust validation action. One of `validate` or `invalidate`
 - `:cidr` - Subnet for command targeting. Of the form: `8.8.8.8/24`
 - `:address` - Hostname or IP Address for command targeting.  
 - `:virtue` - Virtue ID (uuid4)
 - `:level` - Observation level for a group of sensors. One of `off`, `default`, `low`, `high`, `adversarial`
 
Each of the keyword replacements can be mapped to a function in `virtue-security` used to
generate values for the keyword. Those functions are named in the pattern: `test_parameter_*`