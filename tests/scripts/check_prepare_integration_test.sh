#!/bin/bash

integration_test=$1
test_directory=./tests/docker_tests/$integration_test

# check test exists
if [ ! -d $test_directory ]; then
  echo "Can't find integration test $integration_test or it is not a directory!"
  exit 1
fi

# check test main file exists
if [ ! -f $test_directory/run.sh -o ! -x $test_directory/run.sh ]; then
  echo "Can't find main test file run.sh or it is not executable!"
  exit 1
fi

# check for setup file
if [ -f $test_directory/setup.sh -a -x $test_directory/setup.sh ]; then
  echo "has-setup=true" >> $GITHUB_OUTPUT
fi

#check for teardown file
if [ -f $test_directory/teardown.sh -a -x $test_directory/teardown.sh ]; then
  echo "has-teardown=true" >> $GITHUB_OUTPUT
fi

#check for config file
if [ -f $test_directory/config.json ]; then
  friendly_name=$(cat $test_directory/config.json | jq ".friendly_name // \"$integration_test\"")
  echo "friendly_name=$friendly_name" >> $GITHUB_OUTPUT
fi