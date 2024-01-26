#!/usr/bin/env sh

set -e

# exit and return error message to stderr
exit_error() {
    echo "Error: $1" >&2
    exit 1
}

integration_test=$1
test_directory=./tests/docker_tests/$integration_test

# check jq is installed
command -v jq >/dev/null 2>&1 || exit_error "jq is required but it's not installed."

# print usage if no argument was provided
[ -z "$integration_test" ] && echo "Usage: $0 <dir>" >&2 && exit 1

# check test exists
[ ! -d "$test_directory" ] && exit_error "Can't find integration test in $integration_test or it is not a directory!"

# check test main file exists and is executable
if [ ! -x "$test_directory/run.sh" ]; then
  exit_error "Expected main test file at '$test_directory/run.sh' is missing or not executable"
fi

# check for setup file
if [ -f "$test_directory/setup.sh" ] && [ -x "$test_directory/setup.sh" ]; then
  echo "has-setup=true" >> "$GITHUB_OUTPUT"
fi

# check for teardown file
if [ -f "$test_directory/teardown.sh" ] && [ -x "$test_directory/teardown.sh" ]; then
  echo "has-teardown=true" >> "$GITHUB_OUTPUT"
fi

# check for config file
if [ -f "$test_directory/config.json" ]; then
  friendly_name=$(jq ".friendly_name // \"$integration_test\"" < "$test_directory/config.json")
  echo "friendly_name=$friendly_name" >> "$GITHUB_OUTPUT"
f
