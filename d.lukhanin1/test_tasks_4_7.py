#!/usr/bin/env python3
"""Behavior checks for the standalone programs; run with python3 on Solaris too."""
from pathlib import Path
import re
import subprocess
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parent
PROGRAMS = {
    4: "list_strings",
    5: "line_lookup",
    6: "line_lookup_alarm",
    7: "line_lookup_mmap",
}


class TaskTests(unittest.TestCase):
    def run_program(self, task, data, content=None, table=False):
        executable = ROOT / ("Task_" + str(task)) / PROGRAMS[task]
        self.assertTrue(executable.is_file(), "Missing program: " + str(executable))
        with tempfile.TemporaryDirectory() as directory:
            args = [str(executable)]
            if task != 4:
                path = Path(directory) / "input.txt"
                path.write_bytes(content)
                args.append(str(path))
                if table:
                    args.append("--table")
            return subprocess.run(args, input=data, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, timeout=10)

    def test_list_prints_endpoints_and_stops_at_leading_dot(self):
        result = self.run_program(4, b"first\n\nword.with.dot\n.stop\nignored\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"first\nword.with.dot\n")

    def test_list_prints_single_node_once(self):
        result = self.run_program(4, b"only\n.\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"only\n")

    def test_list_prints_equal_endpoints_twice(self):
        result = self.run_program(4, b"same\nmiddle\nsame\n.\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"same\nsame\n")

    def test_list_prints_empty_endpoints(self):
        for data, expected in ((b"\nmiddle\nlast\n.\n", b"\nlast\n"),
                               (b"first\nmiddle\n\n.\n", b"first\n\n"),
                               (b"\n.\n", b"\n")):
            with self.subTest(data=data):
                result = self.run_program(4, data)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, expected)

    def test_list_keeps_long_line_as_one_node(self):
        data = b"x" * 20000 + b".inside\n"
        result = self.run_program(4, data + b"middle\nnext\n.\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, data + b"next\n")

    def test_list_accepts_eof_without_newline(self):
        result = self.run_program(4, b"last")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"last")
        self.assertEqual(self.run_program(4, b".\n").stdout, b"")
        result = self.run_program(4, b"first\nmiddle\nlast")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"first\nlast")

    def test_lookup_repeated_queries_empty_and_unterminated_lines(self):
        for task in (5, 6, 7):
            with self.subTest(task=task):
                result = self.run_program(task, b"3\n2\n1\n3\n0\n", b"alpha\n\nomega")
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = b"omega\n\nalpha\nomega\n"
                if task == 5:
                    expected = (b"Total size: 12 bytes\nomega\nLine size: 5 bytes\n"
                                b"\nLine size: 1 bytes\nalpha\nLine size: 6 bytes\n"
                                b"omega\nLine size: 5 bytes\n")
                self.assertEqual(result.stdout, expected)

    def test_task5_line_sizes_sum_to_file_size(self):
        cases = ((b"abc\n\nxy", b"1\n2\n3\n0\n", [4, 1, 2], 7),
                 ("Я\n\nю".encode("utf-8"), b"1\n2\n3\n0\n", [3, 1, 2], 6),
                 (b"", b"0\n", [], 0))
        for content, queries, sizes, total in cases:
            with self.subTest(content=content):
                result = self.run_program(5, queries, content)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(("Total size: %d bytes\n" % total).encode(), result.stdout)
                reported = [int(n) for n in re.findall(rb"Line size: (\d+) bytes", result.stdout)]
                self.assertEqual(reported, sizes)
                self.assertEqual(sum(reported), total)

    def test_lookup_table_uses_byte_offsets(self):
        for task in (5, 6, 7):
            with self.subTest(task=task):
                result = self.run_program(task, b"0\n", b"abc\n\nxy", table=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = (b"Line 1: offset=0 length=4\n"
                            b"Line 2: offset=4 length=1\n"
                            b"Line 3: offset=5 length=2\n")
                if task == 5:
                    expected += b"Total size: 7 bytes\n"
                self.assertEqual(result.stdout, expected)

    def test_lookup_handles_long_and_utf8_lines(self):
        long_line = b"L" * 20000 + b"\n"
        utf8 = "Привет\n".encode("utf-8")
        for task in (5, 6, 7):
            with self.subTest(task=task):
                result = self.run_program(task, b"2\n1\n0\n", long_line + utf8)
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = utf8 + long_line
                if task == 5:
                    expected = (b"Total size: 20014 bytes\n" + utf8 + b"Line size: 13 bytes\n"
                                + long_line + b"Line size: 20001 bytes\n")
                self.assertEqual(result.stdout, expected)

    def test_lookup_rejects_bad_numbers_without_losing_next_query(self):
        invalid = b"-1\nabc\n1junk\n99999999999999999999999999999\n9\n\n" + b"7" * 300 + b"\n"
        for task in (5, 6, 7):
            with self.subTest(task=task):
                result = self.run_program(task, invalid + b" 1 \n0\n", b"chosen\n")
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = b"chosen\n"
                if task == 5:
                    expected = b"Total size: 7 bytes\nchosen\nLine size: 7 bytes\n"
                self.assertEqual(result.stdout, expected)

    def test_lookup_empty_file_trailing_newline_and_eof(self):
        for task in (5, 6, 7):
            with self.subTest(task=task):
                result = self.run_program(task, b"1\n0\n", b"")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, b"Total size: 0 bytes\n" if task == 5 else b"")
                result = self.run_program(task, b"2\n0\n", b"one\n")
                self.assertEqual(result.stdout, b"Total size: 4 bytes\n" if task == 5 else b"")
                result = self.run_program(task, b"1", b"one\n")
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = b"one\n"
                if task == 5:
                    expected = b"Total size: 4 bytes\none\nLine size: 4 bytes\n"
                self.assertEqual(result.stdout, expected)

    def test_lookup_missing_file_and_missing_argument_fail(self):
        for task in (5, 6, 7):
            executable = ROOT / ("Task_" + str(task)) / PROGRAMS[task]
            for args in ([], ["/no/such/25946-file"]):
                with self.subTest(task=task, args=args):
                    result = subprocess.run([str(executable)] + args,
                                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    self.assertNotEqual(result.returncode, 0)

    def check_timeout(self, task, partial=False, reset=False):
        executable = ROOT / ("Task_" + str(task)) / PROGRAMS[task]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.txt"
            path.write_bytes(b"first\n\nlast")
            process = subprocess.Popen([str(executable), str(path)], stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            try:
                started = time.monotonic()
                # Keep stdin open: EOF is not a timeout.
                if partial:
                    process.stdin.write(b"1")
                    process.stdin.flush()
                if reset:
                    time.sleep(2)
                    process.stdin.write(b"1\n")
                    process.stdin.flush()
                process.wait(timeout=10)
                elapsed = time.monotonic() - started
                output = process.stdout.read()
                errors = process.stderr.read()
                self.assertEqual(process.returncode, 0, errors)
                self.assertEqual(output, (b"first\n" if reset else b"") + b"first\n\nlast")
                self.assertGreaterEqual(elapsed, 6 if reset else 4)
                self.assertLess(elapsed, 9 if reset else 7)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                process.stdin.close()
                process.stdout.close()
                process.stderr.close()

    def test_alarm_prints_whole_file_after_five_seconds(self):
        for task in (6, 7):
            with self.subTest(task=task):
                self.check_timeout(task)

    def test_partial_number_does_not_cancel_alarm(self):
        for task in (6, 7):
            with self.subTest(task=task):
                self.check_timeout(task, partial=True)

    def test_successful_query_starts_new_five_second_timer(self):
        for task in (6, 7):
            with self.subTest(task=task):
                self.check_timeout(task, reset=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
