"""
Process management functionality.
"""

import psutil
import win32process
import win32gui
import win32con
import win32api
from typing import Optional, List, Dict
import time

class ProcessManager:
    """Manages process operations in the Windows session."""
    
    def __init__(self):
        """Initialize the process manager."""
        pass
        
    def list_processes(self) -> List[Dict[str, any]]:
        """List all running processes with their properties."""
        processes = []
        for proc in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_percent']):
            try:
                proc_info = proc.as_dict()
                processes.append({
                    'pid': proc_info['pid'],
                    'name': proc_info['name'],
                    'cpu_percent': proc_info['cpu_percent'],
                    'memory_percent': proc_info['memory_percent']
                })
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
        return processes
    
    def get_process_info(self, pid: int) -> Optional[Dict[str, any]]:
        """Get detailed information about a specific process."""
        try:
            proc = psutil.Process(pid)
            return {
                'pid': proc.pid,
                'name': proc.name(),
                'status': proc.status(),
                'cpu_percent': proc.cpu_percent(),
                'memory_percent': proc.memory_percent(),
                'create_time': proc.create_time(),
                'num_threads': proc.num_threads(),
                'username': proc.username()
            }
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return None
    
    def kill_process(self, pid: int) -> bool:
        """Kill a process by its PID."""
        try:
            proc = psutil.Process(pid)
            proc.kill()
            return True
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return False
    
    def suspend_process(self, pid: int) -> bool:
        """Suspend a process by its PID."""
        try:
            proc = psutil.Process(pid)
            proc.suspend()
            return True
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return False
    
    def resume_process(self, pid: int) -> bool:
        """Resume a suspended process by its PID."""
        try:
            proc = psutil.Process(pid)
            proc.resume()
            return True
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return False
    
    def get_process_windows(self, pid: int) -> List[Dict[str, any]]:
        """Get all windows belonging to a specific process."""
        windows = []
        
        def callback(hwnd, ctx):
            if win32gui.IsWindowVisible(hwnd):
                _, process_id = win32process.GetWindowThreadProcessId(hwnd)
                if process_id == pid:
                    title = win32gui.GetWindowText(hwnd)
                    rect = win32gui.GetWindowRect(hwnd)
                    style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
                    windows.append({
                        'handle': hwnd,
                        'title': title,
                        'rect': rect,
                        'style': style,
                        'minimized': bool(style & win32con.WS_MINIMIZE),
                        'maximized': bool(style & win32con.WS_MAXIMIZE)
                    })
            return True
            
        win32gui.EnumWindows(callback, None)
        return windows
    
    def get_window_process(self, hwnd: int) -> Optional[Dict[str, any]]:
        """Get process information for a specific window handle."""
        try:
            _, pid = win32process.GetWindowThreadProcessId(hwnd)
            return self.get_process_info(pid)
        except Exception:
            return None 