"""Chinese Tk GUI. All serial operations run outside the Tk thread."""
from pathlib import Path
import queue
import threading
import tkinter as tk
from tkinter import filedialog, ttk

from .cli import describe_info, infer_version, open_serial
from .protocol import FirmwareImage
from .uploader import Cancelled, UnknownOutcome, Uploader


def serial_ports():
    try:
        from serial.tools import list_ports
    except ImportError:
        return []
    return [port.device for port in list_ports.comports()]


class UpgradeApp:
    def __init__(self, root, *, serial_factory=None, port_provider=None):
        self.root = root
        self.serial_factory = serial_factory or open_serial
        self.port_provider = port_provider or serial_ports
        self.events = queue.Queue()
        self.cancel_event = threading.Event()
        self.busy = False
        self.closed = False
        self.transport = None
        self.uploader = None
        self._transport_lock = threading.Lock()
        root.title('STM32 无线固件升级工具')
        root.minsize(640, 420)
        root.protocol('WM_DELETE_WINDOW', self.close)
        panel = ttk.Frame(root, padding=12)
        panel.pack(fill='both', expand=True)
        panel.columnconfigure(1, weight=1)
        panel.rowconfigure(7, weight=1)
        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value='9600')
        self.bin_var = tk.StringVar()
        self.version_var = tk.StringVar(value='1')
        self.status_var = tk.StringVar(value='请选择串口；升级前请按 KEY 并手动复位进入 Bootloader')
        ttk.Label(panel, text='串口').grid(row=0, column=0, sticky='w', pady=4)
        self.port_combo = ttk.Combobox(panel, textvariable=self.port_var, state='readonly')
        self.port_combo.grid(row=0, column=1, sticky='ew')
        self.refresh_button = ttk.Button(panel, text='刷新串口', command=self.refresh_ports)
        self.refresh_button.grid(row=0, column=2, padx=(8, 0))
        ttk.Label(panel, text='波特率').grid(row=1, column=0, sticky='w', pady=4)
        self.baud_entry = ttk.Entry(panel, textvariable=self.baud_var)
        self.baud_entry.grid(row=1, column=1, sticky='ew')
        ttk.Label(panel, text='固件 BIN').grid(row=2, column=0, sticky='w', pady=4)
        self.bin_entry = ttk.Entry(panel, textvariable=self.bin_var)
        self.bin_entry.grid(row=2, column=1, sticky='ew')
        self.choose_button = ttk.Button(panel, text='选择文件', command=self.choose_bin)
        self.choose_button.grid(row=2, column=2, padx=(8, 0))
        ttk.Label(panel, text='版本整数').grid(row=3, column=0, sticky='w', pady=4)
        self.version_entry = ttk.Entry(panel, textvariable=self.version_var)
        self.version_entry.grid(row=3, column=1, sticky='ew')
        actions = ttk.Frame(panel)
        actions.grid(row=4, column=0, columnspan=3, sticky='w', pady=10)
        self.query_button = ttk.Button(actions, text='查询设备', command=self.start_query)
        self.query_button.pack(side='left')
        self.upload_button = ttk.Button(actions, text='开始升级', command=self.start_upload)
        self.upload_button.pack(side='left', padx=8)
        self.cancel_button = ttk.Button(actions, text='取消接收', command=self.cancel,
                                       state='disabled')
        self.cancel_button.pack(side='left')
        self.progress = ttk.Progressbar(panel, mode='determinate', maximum=100)
        self.progress.grid(row=5, column=0, columnspan=3, sticky='ew')
        ttk.Label(panel, textvariable=self.status_var, wraplength=610).grid(
            row=6, column=0, columnspan=3, sticky='w', pady=8)
        log_panel = ttk.Frame(panel)
        log_panel.grid(row=7, column=0, columnspan=3, sticky='nsew')
        self.log_text = tk.Text(log_panel, height=12, state='disabled', wrap='word')
        self.log_text.pack(side='left', fill='both', expand=True)
        scrollbar = ttk.Scrollbar(log_panel, command=self.log_text.yview)
        scrollbar.pack(side='right', fill='y')
        self.log_text.configure(yscrollcommand=scrollbar.set)
        self._inputs = (self.refresh_button, self.baud_entry, self.bin_entry,
                        self.choose_button, self.version_entry, self.query_button,
                        self.upload_button)
        self.refresh_ports()
        self._poll_token = root.after(40, self._poll_events)

    def _log(self, message):
        self.log_text.configure(state='normal')
        self.log_text.insert('end', message + '\n')
        self.log_text.see('end')
        self.log_text.configure(state='disabled')

    def refresh_ports(self):
        try:
            ports = self.port_provider()
            self.port_combo.configure(values=ports)
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0] if ports else '')
        except Exception as error:
            self._log(f'无法刷新串口：{error}')

    def choose_bin(self):
        path = filedialog.askopenfilename(title='选择应用固件 BIN',
                                        filetypes=[('BIN 固件', '*.bin'), ('所有文件', '*.*')])
        if path:
            self.bin_var.set(path)
            self.version_var.set(str(infer_version(path)))

    def _set_busy(self, busy):
        self.busy = busy
        for widget in self._inputs:
            widget.configure(state='disabled' if busy else 'normal')
        self.port_combo.configure(state='disabled' if busy else 'readonly')
        self.cancel_button.configure(state='normal' if busy else 'disabled')

    def start_query(self):
        self._start(False)

    def start_upload(self):
        self._start(True)

    def _start(self, upgrading):
        if self.busy or self.closed:
            return
        try:
            port = self.port_var.get().strip()
            if not port:
                raise ValueError('请先选择串口')
            baud = int(self.baud_var.get())
            if baud <= 0:
                raise ValueError('波特率必须为正整数')
            # Capture values on the main thread; the worker validates/reads BIN.
            path = self.bin_var.get()
            version = int(self.version_var.get()) if upgrading else None
            if upgrading and not path:
                raise ValueError('请先选择 BIN 文件')
        except ValueError as error:
            self._log(f'参数错误：{error}')
            return
        self.cancel_event.clear()
        self.progress['value'] = 0
        self._set_busy(True)
        self.status_var.set('正在连接并校验固件…' if upgrading else '正在查询设备…')
        self.worker = threading.Thread(target=self._work, args=(port, baud, upgrading, path, version),
                                       daemon=True)
        self.worker.start()

    def _work(self, port, baud, upgrading, path, version):
        transport = None
        try:
            image = FirmwareImage(Path(path).read_bytes(), version) if upgrading else None
            if self.cancel_event.is_set():
                raise Cancelled('已取消，尚未打开串口')
            transport = self.serial_factory(port, baudrate=baud, timeout=0.1, write_timeout=5.0)
            with self._transport_lock:
                if self.closed:
                    transport.close()
                    transport = None
                    return
                self.transport = transport
            uploader = Uploader(transport, cancel_event=self.cancel_event,
                                log=lambda message: self.events.put(('log', message)))
            self.uploader = uploader
            if upgrading:
                result = uploader.upload(image, progress=lambda done, total:
                                         self.events.put(('progress', (done, total))))
                self.events.put(('success', '升级成功；' + describe_info(result)))
            else:
                self.events.put(('success', describe_info(uploader.query())))
        except Cancelled as error:
            self.events.put(('cancelled', str(error)))
        except UnknownOutcome as error:
            self.events.put(('unknown', str(error)))
        except Exception as error:
            self.events.put(('error', f'操作失败：{error}'))
        finally:
            with self._transport_lock:
                self.transport = None
                self.uploader = None
            if transport is not None:
                try:
                    transport.close()
                except OSError:
                    pass
            self.events.put(('idle', None))

    def _poll_events(self):
        if self.closed:
            return
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == 'progress':
                done, total = value
                self.progress['value'] = 100 * done / total
                self.status_var.set(f'设备已确认接收 {done}/{total} 字节；安装结果尚待确认')
            elif kind == 'idle':
                self._set_busy(False)
            else:
                self._log(value)
                self.status_var.set(value)
        self._poll_token = self.root.after(40, self._poll_events)

    def cancel(self):
        if not self.busy:
            return
        self.cancel_event.set()
        if self.uploader is not None and self.uploader.phase == 'committing':
            message = '已进入安装阶段，无法取消或回滚；正在等待最终查询确认'
        else:
            message = '已请求取消，正在等待设备确认；请勿立即断电'
        self.status_var.set(message)
        self._log(message)

    def _close_transport(self):
        with self._transport_lock:
            transport = self.transport
        if transport is not None:
            try:
                transport.close()
            except OSError:
                pass

    def close(self):
        if self.closed:
            return
        self.closed = True
        self.cancel_event.set()
        self.root.after_cancel(self._poll_token)
        if self.busy:
            # Give receive cancellation a brief opportunity without blocking Tk.
            # Closing during COMMIT means the outcome must be queried next time.
            timer = threading.Timer(1.0, self._close_transport)
            timer.daemon = True
            timer.start()
        else:
            self._close_transport()
        self.root.destroy()


def main():
    root = tk.Tk()
    UpgradeApp(root)
    root.mainloop()


if __name__ == '__main__':
    main()
