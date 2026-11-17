import pathlib
import signal
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent


class ProcessPipeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for number in (9, 10, 25):
            directory = ROOT / f'Task_{number}'
            if (directory / 'Makefile').exists():
                subprocess.run(['make', '-C', str(directory)], check=True,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def run_program(self, task, name, *args, data=None):
        program = ROOT / f'Task_{task}' / name
        self.assertTrue(program.is_file(), f'Missing program: {program}')
        return subprocess.run([str(program), *args], input=data,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=10)

    def test_cat_long_file_and_parent_last_line(self):
        content = b''.join(f'line {i}\n'.encode() for i in range(25000))
        with tempfile.NamedTemporaryFile() as source:
            source.write(content)
            source.flush()
            result = self.run_program(9, 'child_cat', source.name)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(content, result.stdout)
        self.assertTrue(result.stdout.endswith(b'Parent: child finished.\n'))

    def test_cat_missing_file_reports_failure(self):
        result = self.run_program(9, 'child_cat', '/no/such/input/file')
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(result.stderr)

    def test_command_preserves_arguments(self):
        result = self.run_program(10, 'run_command', 'printf', '<%s>\n', 'two words', '*', '')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b'<two words>\n<*>\n<>\nExit status: 0\n')

    def test_command_nonzero_status(self):
        result = self.run_program(10, 'run_command', 'sh', '-c', 'exit 37')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b'Exit status: 37\n')

    def test_command_signal(self):
        result = self.run_program(10, 'run_command', 'sh', '-c', 'kill -TERM $$')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, f'Signal: {signal.SIGTERM}\n'.encode())

    def test_command_missing_executable(self):
        result = self.run_program(10, 'run_command', 'missing-command-for-os-task-25946')
        self.assertEqual(result.stdout, b'Exit status: 127\n')
        self.assertTrue(result.stderr)

    def test_pipe_long_binary_stream(self):
        content = (b'aBc DeF\n\nxyz\x00\xff\x80!\n' * 25000) + b'no newline'
        result = self.run_program(25, 'pipe_upper', data=content)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, content.upper())

    def test_pipe_empty_input(self):
        result = self.run_program(25, 'pipe_upper', data=b'')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b'')

    def test_required_arguments(self):
        for task, name in ((9, 'child_cat'), (10, 'run_command')):
            with self.subTest(task=task):
                result = self.run_program(task, name)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(result.stderr)


if __name__ == '__main__':
    unittest.main()
