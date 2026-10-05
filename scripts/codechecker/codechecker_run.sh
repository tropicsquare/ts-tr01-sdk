#!/bin/bash

# Exit immediately if any command fails
set -e

# `--gen-reports` is used by CI and local users who need HTML and Code
# Climate output in addition to CodeChecker's terminal results.
generate_reports=false

if [[ $# -gt 1 || ( $# -eq 1 && "$1" != "--gen-reports" ) ]]; then
	echo "Usage: $0 [--gen-reports]" >&2
	exit 64
fi

if [[ "${1:-}" == "--gen-reports" ]]; then
	generate_reports=true
fi

# Build SDK
pushd examples/demo_umc55
./build.sh
popd

# Run CodeChecker
# CodeChecker returns 2 when it found defects. Capture it so reports can
# still be generated, but preserve it as this script's final status.
set +e
CodeChecker check --logfile examples/demo_umc55/build/compile_commands.json --config scripts/codechecker/codechecker_config.yml
check_status=$?
set -e

if [[ $check_status -ne 0 && $check_status -ne 2 ]]; then
	exit "$check_status"
fi

if [[ $generate_reports == true ]]; then
	# `parse` also returns 2 when its input contains findings. Continue to
	# the next format in that case; other nonzero statuses are export errors.
	set +e
	CodeChecker parse -e codeclimate ./.codechecker/reports \
		-o ./.codechecker/reports.json --trim-path-prefix "${CI_PROJECT_DIR:-$PWD}/"
	report_status=$?
	set -e

	if [[ $report_status -ne 0 && $report_status -ne 2 ]]; then
		exit "$report_status"
	fi

	# Generate the browsable report even when the Code Climate export found
	# findings and returned 2.
	set +e
	CodeChecker parse -e html ./.codechecker/reports \
		-o ./.codechecker/reports_html
	report_status=$?
	set -e

	if [[ $report_status -ne 0 && $report_status -ne 2 ]]; then
		exit "$report_status"
	fi
fi

exit "$check_status"