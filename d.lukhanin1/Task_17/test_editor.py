import errno
import os
import pathlib
import pty
import select
import signal
import subprocess
import termios
import time
import unittest

ROOT = pathlib.Path(__file__).resolve().parent


class Editor:
    def __init__(self):
        self.master, self.slave = pty.openpty()
        attrs = termios.tcgetattr(self.slave)
        attrs[6][termios.VERASE] = b'#'
        attrs[6][termios.VKILL] = b'@'
        termios.tcsetattr(self.slave, termios.TCSANOW, attrs)
        self.original = termios.tcgetattr(self.slave)
        self.proc = subprocess.Popen([str(ROOT / 'line_editor')], stdin=self.slave,
                                     stdout=self.slave, stderr=subprocess.PIPE)
        deadline = time.monotonic() + 2
        while termios.tcgetattr(self.slave)[3] & termios.ICANON:
            if self.proc.poll() is not None or time.monotonic() >= deadline:
                self.close()
                raise AssertionError('editor did not enter noncanonical mode')
            time.sleep(0.01)

    def send(self, data):
        os.write(self.master, data)
        output = b''
        deadline = time.monotonic() + 0.15
        while time.monotonic() < deadline:
            if select.select([self.master], [], [], max(0, deadline - time.monotonic()))[0]:
                try:
                    output += os.read(self.master, 8192)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    break
        return output

    def finish(self):
        self.proc.wait(timeout=2)
        if termios.tcgetattr(self.slave) != self.original:
            raise AssertionError('terminal attributes were not restored')
        return self.proc.returncode

    def close(self):
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait(timeout=2)
        self.proc.stderr.close()
        os.close(self.master)
        os.close(self.slave)


class EditorTests(unittest.TestCase):
    def setUp(self):
        self.editor = Editor()
        self.addCleanup(self.editor.close)

    def test_immediate_echo_and_original_erase_kill(self):
        self.assertEqual(self.editor.send(b'a'), b'a')
        self.assertEqual(self.editor.send(b'bc#'), b'bc\b \b')
        self.assertEqual(self.editor.send(b'@'), b'\b \b' * 2)
        self.editor.send(b'\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_word_erase_removes_trailing_spaces(self):
        self.editor.send(b'one two   ')
        self.assertEqual(self.editor.send(b'\x17'), b'\b \b' * 6)
        self.assertEqual(self.editor.send(b'@'), b'\b \b' * 4)
        self.editor.send(b'\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_controls_bell_and_nonempty_eof(self):
        self.editor.send(b'x')
        self.assertEqual(self.editor.send(b'\x01\x04\t\x7f'), b'\a' * 4)
        self.assertIsNone(self.editor.proc.poll())
        self.editor.send(b'@\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_wrap_whole_word_and_only_edit_current_line(self):
        self.editor.send(b'a' * 33 + b' ' + b'abcdef')
        self.assertEqual(self.editor.send(b'g'), b'\b \b' * 6 + b'\r\nabcdefg')
        self.assertEqual(self.editor.send(b'@'), b'\b \b' * 7)
        self.editor.send(b'\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_long_word_is_split_at_limit(self):
        self.editor.send(b'x' * 40)
        self.assertEqual(self.editor.send(b'y'), b'\r\ny')
        self.editor.send(b'@\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_enter_starts_empty_line(self):
        self.editor.send(b'hello')
        self.assertEqual(self.editor.send(b'\n'), b'\r\n')
        self.editor.send(b'\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_keyboard_signal_characters_ring_bell(self):
        self.assertFalse(termios.tcgetattr(self.editor.slave)[3] & termios.ISIG)
        self.assertEqual(self.editor.send(b'\x03\x1c\x1a'), b'\a' * 3)
        self.assertIsNone(self.editor.proc.poll())
        self.editor.send(b'\x04')
        self.assertEqual(self.editor.finish(), 0)

    def test_interrupt_restores_terminal(self):
        self.editor.proc.send_signal(signal.SIGINT)
        self.assertEqual(self.editor.finish(), 128 + signal.SIGINT)


class InputTests(unittest.TestCase):
    def test_rejects_redirected_input(self):
        result = subprocess.run([str(ROOT / 'line_editor')], input=b'', capture_output=True, timeout=2)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(result.stderr)


if __name__ == '__main__':
    unittest.main()
