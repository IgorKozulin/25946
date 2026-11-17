import datetime
import grp
import os
from pathlib import Path
import pwd
import re
import select
import signal
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parent


class ListingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        result = subprocess.run(['make', '-C', str(ROOT / 'Task_18')], capture_output=True, text=True)
        if result.returncode:
            raise AssertionError('Task_18 must build: ' + result.stderr)
        cls.program = ROOT / 'Task_18/file_listing'

    def run_listing(self, *args, cwd=None):
        return subprocess.run([str(self.program), *map(str, args)], cwd=cwd, capture_output=True, text=True)

    def test_regular_permissions_links_owner_group_size_and_mtime(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'some file'
            path.write_bytes(b'abcdef')
            path.chmod(0o641)
            os.link(path, Path(tmp) / 'another')
            os.utime(path, (946684800, 946684800))
            result = self.run_listing(path)
            self.assertEqual(result.returncode, 0, result.stderr)
            fields = result.stdout.split(maxsplit=7)
            self.assertEqual(fields[0], '-rw-r----x')
            self.assertEqual(fields[1:5], ['2', pwd.getpwuid(os.getuid()).pw_name,
                                         grp.getgrgid(os.getgid()).gr_name, '6'])
            self.assertEqual(' '.join(fields[5:7]), datetime.datetime.fromtimestamp(946684800).strftime('%Y-%m-%d %H:%M'))
            self.assertEqual(fields[7].rstrip('\n'), 'some file')

    def test_directory_trailing_slashes_symlink_fifo_have_no_size(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp) / 'folder'
            directory.mkdir()
            directory.chmod(0o750)
            link = Path(tmp) / 'link'
            link.symlink_to(directory)
            fifo = Path(tmp) / 'fifo'
            os.mkfifo(fifo, 0o600)
            result = self.run_listing(str(directory) + '///', link, fifo)
            self.assertEqual(result.returncode, 0, result.stderr)
            lines = result.stdout.splitlines()
            self.assertEqual(len(lines), 3)
            self.assertTrue(lines[0].startswith('drwxr-x---'))
            self.assertTrue(lines[1].startswith('?'))
            self.assertTrue(lines[2].startswith('?'))
            for line, name in zip(lines, ['folder', 'link', 'fifo']):
                self.assertRegex(line, r'^.{10}\s+\d+\s+\S+\s+\S+\s+\d{4}-\d\d-\d\d \d\d:\d\d ' + name + '$')
            self.assertEqual([line.index(re.search(r'\d{4}-\d\d-\d\d', line).group()) for line in lines], [64] * 3)

    def test_missing_argument_reports_failure_and_continues(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'exists'
            path.touch()
            result = self.run_listing(Path(tmp) / 'missing', path)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('missing', result.stderr)
            self.assertTrue(result.stdout.endswith('exists\n'))

    def test_default_is_current_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = self.run_listing(cwd=tmp)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(result.stdout.startswith('d'))
            self.assertTrue(result.stdout.endswith(' .\n'))


class SignalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        result = subprocess.run(['make', '-C', str(ROOT / 'Task_21')], capture_output=True, text=True)
        if result.returncode:
            raise AssertionError('Task_21 must build: ' + result.stderr)
        cls.program = ROOT / 'Task_21/beeper'

    def start_beeper(self):
        process = subprocess.Popen([str(self.program)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(self.finish_process, process)
        self.assertTrue(select.select([process.stdout], [], [], 3)[0], 'startup timed out')
        self.assertIn(b'SIGINT', process.stdout.readline())
        return process

    @staticmethod
    def finish_process(process):
        if process.poll() is None:
            process.kill()
        process.communicate(timeout=3)

    def test_each_delivered_interrupt_beeps_then_quit_reports_count(self):
        process = self.start_beeper()
        for _ in range(4):
            process.send_signal(signal.SIGINT)
            self.assertTrue(select.select([process.stdout], [], [], 3)[0], 'bell timed out')
            self.assertEqual(process.stdout.read(1), b'\a')
            self.assertIsNone(process.poll())
        process.send_signal(signal.SIGQUIT)
        output, errors = process.communicate(timeout=3)
        self.assertEqual(process.returncode, 0, errors)
        self.assertIn('Всего прозвучало сигналов: 4'.encode(), output)
        self.assertEqual(errors, b'')

    def test_quit_without_interrupts(self):
        process = self.start_beeper()
        process.send_signal(signal.SIGQUIT)
        output, errors = process.communicate(timeout=3)
        self.assertEqual(process.returncode, 0, errors)
        self.assertIn('Всего прозвучало сигналов: 0'.encode(), output)
        self.assertNotIn(b'\a', output)


if __name__ == '__main__':
    unittest.main()
