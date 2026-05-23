import re

import click

from platformio.test.result import TestCase, TestCaseSource, TestStatus
from platformio.test.runners.base import TestRunnerBase


class CustomTestRunner(TestRunnerBase):
    TESTCASE_PARSE_RE = re.compile(
        r"(?P<source_file>[^:]+):(?P<source_line>\d+):(?P<name>[^\s]+):"
        r"(?P<status>PASS|IGNORE|FAIL)(:\s*(?P<message>.+)$)?"
    )

    def on_testing_line_output(self, line):
        if self.options.verbose:
            click.echo(line, nl=False)

        line = line.strip()
        if not line:
            return

        test_case = self.parse_test_case(line)
        if test_case:
            self.test_suite.add_case(test_case)
            if not self.options.verbose:
                click.echo(test_case.humanize())
            return

        if all(token in line for token in ("Tests", "Failures", "Ignored")):
            self.test_suite.on_finish()
            return

        click.echo(line)

    def parse_test_case(self, line):
        match = self.TESTCASE_PARSE_RE.search(line)
        if not match:
            return None

        groups = match.groupdict()
        status = TestStatus.from_string(groups["status"])
        source = TestCaseSource(
            filename=groups["source_file"],
            line=int(groups["source_line"]),
        )
        return TestCase(
            name=groups["name"],
            status=status,
            source=source,
            message=groups["message"],
        )
