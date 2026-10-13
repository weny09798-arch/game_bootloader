"""CLI subprocess and withdrawn Tk application exercising the real C device."""
import contextlib
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import time
import unittest

from test_host_uploader import NativeTransport, NATIVE, image_bytes

try:
    from host.cli import main, infer_version
    from host.gui import UpgradeApp
except ImportError:
    main = infer_version = UpgradeApp = None

ROOT = Path(__file__).resolve().parents[1]

# Fresh process: the main thread exits immediately after actual UpgradeApp.close.
# Only a transport double delays a reply; uploader cancellation stays real.
GUI_ACTIVE_CLOSE = textwrap.dedent('''
    import queue
    import sys
    import tempfile
    import threading
    import time
    from pathlib import Path
    sys.path.insert(0, 'tests')
    from test_host_uploader import NativeTransport, image_bytes
    from host.gui import UpgradeApp
    from host.protocol import MessageType as T

    target = T.DATA if sys.argv[1] == 'receive' else T.COMMIT
    active = threading.Event()

    class DelayedTransport(NativeTransport):
        def read(self, size):
            if active.is_set():
                time.sleep(0.1)
            return super().read(size)

        def close(self):
            if not self.closed:
                print('CANCEL_SENT', any(packet[5] == T.CANCEL for packet in self.requests), flush=True)
                super().close()
                print('TRANSPORT_CLOSED', flush=True)

    def fault(transport, packet, reply):
        if packet[5] == target:
            active.set()
            return b''
        return reply

    class Root:
        def after_cancel(self, token):
            pass
        def destroy(self):
            pass

    app = UpgradeApp.__new__(UpgradeApp)
    app.root = Root()
    app.events = queue.Queue()
    app.cancel_event = threading.Event()
    app.busy = True
    app.closed = False
    app.transport = app.uploader = None
    app._transport_lock = threading.Lock()
    app._poll_token = None
    if sys.argv[1] == 'opening':
        def factory(*args, **kwargs):
            active.set()
            time.sleep(1.2)
            if app.closed:
                print('OPENED_AFTER_WINDOW_CLOSE', flush=True)
            return DelayedTransport()
        app.serial_factory = factory
    else:
        transport = DelayedTransport(fault=fault)
        app.serial_factory = lambda *args, **kwargs: transport
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / 'app.bin'
        path.write_bytes(image_bytes())
        worker = threading.Thread(target=app._work,
                                  args=('COM_TEST', 9600, True, str(path), 2), daemon=True)
        worker.start()
        if not active.wait(3):
            raise RuntimeError('Uploader did not reach the requested phase')
        app.close()
        print('WINDOW_CLOSE_RETURNED', flush=True)
''')


class FrontendTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(main, 'CLI and GUI must exist')

    def test_cli_help_without_serial_dependency(self):
        result = subprocess.run([sys.executable, '-m', 'host.cli', '--help'],
                                cwd=ROOT, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b'--port', result.stdout)
        self.assertIn(b'--version', result.stdout)

    def test_filename_version_inference(self):
        self.assertEqual(infer_version('dinosaur_v2.bin'), 2)
        self.assertEqual(infer_version('dinosaur-v1.bin'), 1)
        self.assertEqual(infer_version('my_v22.BIN'), 22)
        self.assertEqual(infer_version('dinosaur.bin'), 1)

    def test_gui_active_close_explicit_cleanup_before_process_exit(self):
        for phase, cancel in (('receive', True), ('commit', False), ('opening', None)):
            with self.subTest(phase=phase):
                result = subprocess.run([sys.executable, '-c', GUI_ACTIVE_CLOSE, phase],
                                        cwd=ROOT, capture_output=True, timeout=5)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(b'WINDOW_CLOSE_RETURNED', result.stdout)
                self.assertIn(b'TRANSPORT_CLOSED', result.stdout)
                self.assertNotIn(b'OPENED_AFTER_WINDOW_CLOSE', result.stdout)
                if cancel is not None:
                    self.assertIn(f'CANCEL_SENT {cancel}'.encode('ascii'), result.stdout)

    def test_cli_query_and_upload_real_device(self):
        transport = NativeTransport()
        captured = []
        def factory(port, **kwargs):
            captured.append((port, kwargs))
            return transport
        with contextlib.redirect_stdout(io.StringIO()) as output:
            code = main(['--port', 'COM_TEST', 'query'], serial_factory=factory)
        self.assertEqual(code, 0)
        self.assertIn('6B3D46C8', output.getvalue())
        self.assertEqual(captured[0][1]['baudrate'], 9600)
        self.assertTrue(transport.closed)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'dinosaur_v2.bin'
            path.write_bytes(image_bytes())
            transport = NativeTransport()
            with contextlib.redirect_stdout(io.StringIO()) as output:
                code = main(['--port', 'COM_TEST', 'upload', str(path)], serial_factory=factory)
            self.assertEqual(code, 0)
            self.assertIn('389', output.getvalue())
            self.assertTrue(transport.closed)

    def test_cli_invalid_image_rejected_before_open_port(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'bad.bin'
            path.write_bytes(b'bad')
            opened = []
            with contextlib.redirect_stderr(io.StringIO()):
                code = main(['--port', 'COM_TEST', 'upload', str(path)],
                            serial_factory=lambda *a, **k: opened.append(True))
            self.assertEqual(code, 1)
            self.assertEqual(opened, [])

    def test_gui_withdrawn_worker_query_no_auto_open_and_window_close(self):
        import tkinter as tk
        try:
            root = tk.Tk()
        except tk.TclError as error:
            self.skipTest(f'Tk display unavailable: {error}')
        root.withdraw()
        opened = []
        transport = NativeTransport()
        def factory(*args, **kwargs):
            opened.append(True)
            return transport
        try:
            app = UpgradeApp(root, serial_factory=factory,
                             port_provider=lambda: ['COM_TEST'])
            self.assertEqual(opened, [])
            self.assertEqual(app.baud_var.get(), '9600')
            app.start_query()
            deadline = time.monotonic() + 3
            while app.busy and time.monotonic() < deadline:
                root.update()
                time.sleep(0.005)
            root.update()
            self.assertFalse(app.busy)
            self.assertIn('6B3D46C8', app.log_text.get('1.0', 'end'))
            self.assertTrue(transport.closed)
            app.close()
        finally:
            transport.close()
            try:
                root.destroy()
            except tk.TclError:
                pass

    def test_gui_withdrawn_upload_runs_real_device_and_queue_progress(self):
        import tkinter as tk
        try:
            root = tk.Tk()
        except tk.TclError as error:
            self.skipTest(f'Tk display unavailable: {error}')
        root.withdraw()
        transport = NativeTransport()
        try:
            with tempfile.TemporaryDirectory() as folder:
                path = Path(folder) / 'dinosaur_v2.bin'
                path.write_bytes(image_bytes())
                app = UpgradeApp(root, serial_factory=lambda *a, **k: transport,
                                 port_provider=lambda: ['COM_TEST'])
                app.bin_var.set(str(path))
                app.version_var.set('2')
                app.start_upload()
                deadline = time.monotonic() + 3
                while app.busy and time.monotonic() < deadline:
                    root.update()
                    time.sleep(0.005)
                root.update()
                self.assertFalse(app.busy)
                self.assertIn('升级成功', app.status_var.get())
                self.assertIn('版本=2', app.status_var.get())
                self.assertEqual(app.progress['value'], 100)
                self.assertTrue(transport.closed)
                app.close()
        finally:
            transport.close()
            try:
                root.destroy()
            except tk.TclError:
                pass


if __name__ == '__main__':
    unittest.main()
