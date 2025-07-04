import os
import subprocess
import time
import psutil
import logging
from typing import Dict, List, Optional, Tuple, Union, Callable

logger = logging.getLogger("maestro.process")

class ProcessInfo:
    """Process information container"""
    def __init__(self, pid: int, name: str, status: str, 
                 cpu_percent: float, memory_percent: float,
                 cmdline: List[str]):
        self.pid = pid
        self.name = name
        self.status = status
        self.cpu_percent = cpu_percent
        self.memory_percent = memory_percent
        self.cmdline = cmdline
        self.created_time = time.time()
        
    def __str__(self) -> str:
        return f"Process(pid={self.pid}, name='{self.name}', " \
               f"cpu={self.cpu_percent:.1f}%, mem={self.memory_percent:.1f}%)"
               
    def to_dict(self) -> Dict:
        """Convert to dictionary representation"""
        return {
            "pid": self.pid,
            "name": self.name,
            "status": self.status,
            "cpu_percent": self.cpu_percent,
            "memory_percent": self.memory_percent,
            "cmdline": self.cmdline,
            "created_time": self.created_time
        }

class ProcessManager:
    """Process management and monitoring"""
    
    def __init__(self):
        """Initialize process manager"""
        self._managed_processes: Dict[int, subprocess.Popen] = {}
        self._process_callbacks: Dict[int, List[Callable[[ProcessInfo], None]]] = {}
        
    def start_process(self, cmd: Union[str, List[str]], 
                     shell: bool = False, 
                     cwd: str = None) -> Optional[int]:
        """
        Start a new process
        
        Args:
            cmd: Command to run (string or list of arguments)
            shell: Whether to run in shell
            cwd: Working directory
            
        Returns:
            Process ID if successful, None otherwise
        """
        try:
            process = subprocess.Popen(
                cmd,
                shell=shell,
                cwd=cwd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            pid = process.pid
            self._managed_processes[pid] = process
            logger.info(f"Started process {pid}: {cmd}")
            return pid
            
        except Exception as e:
            logger.error(f"Failed to start process: {e}")
            return None
            
    def kill_process(self, pid: int) -> bool:
        """
        Kill a process by PID
        
        Args:
            pid: Process ID to kill
            
        Returns:
            True if successful, False otherwise
        """
        try:
            if pid in self._managed_processes:
                process = self._managed_processes[pid]
                process.kill()
                process.wait(timeout=5)
                del self._managed_processes[pid]
                logger.info(f"Killed managed process {pid}")
                return True
                
            # Try to kill external process
            process = psutil.Process(pid)
            process.kill()
            logger.info(f"Killed external process {pid}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to kill process {pid}: {e}")
            return False
            
    def list_processes(self) -> List[ProcessInfo]:
        """
        List all running processes
        
        Returns:
            List of ProcessInfo objects
        """
        processes = []
        for proc in psutil.process_iter(['pid', 'name', 'status']):
            try:
                proc_info = proc.info
                with proc.oneshot():
                    processes.append(ProcessInfo(
                        pid=proc_info['pid'],
                        name=proc_info['name'],
                        status=proc_info['status'],
                        cpu_percent=proc.cpu_percent(interval=0.1),
                        memory_percent=proc.memory_percent(),
                        cmdline=proc.cmdline()
                    ))
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
                
        return processes
        
    def find_process_by_name(self, name: str) -> List[ProcessInfo]:
        """
        Find processes by name
        
        Args:
            name: Process name to search for
            
        Returns:
            List of matching ProcessInfo objects
        """
        processes = []
        for proc in psutil.process_iter(['pid', 'name', 'status']):
            try:
                if name.lower() in proc.info['name'].lower():
                    proc_info = proc.info
                    with proc.oneshot():
                        processes.append(ProcessInfo(
                            pid=proc_info['pid'],
                            name=proc_info['name'],
                            status=proc_info['status'],
                            cpu_percent=proc.cpu_percent(interval=0.1),
                            memory_percent=proc.memory_percent(),
                            cmdline=proc.cmdline()
                        ))
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
                
        return processes
        
    def get_process_info(self, pid: int) -> Optional[ProcessInfo]:
        """
        Get information about a specific process
        
        Args:
            pid: Process ID
            
        Returns:
            ProcessInfo object if found, None otherwise
        """
        try:
            proc = psutil.Process(pid)
            with proc.oneshot():
                return ProcessInfo(
                    pid=proc.pid,
                    name=proc.name(),
                    status=proc.status(),
                    cpu_percent=proc.cpu_percent(interval=0.1),
                    memory_percent=proc.memory_percent(),
                    cmdline=proc.cmdline()
                )
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            return None
            
    def is_process_running(self, pid: int) -> bool:
        """
        Check if a process is running
        
        Args:
            pid: Process ID
            
        Returns:
            True if running, False otherwise
        """
        try:
            proc = psutil.Process(pid)
            return proc.is_running() and proc.status() != psutil.STATUS_ZOMBIE
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            return False
            
    def add_process_callback(self, pid: int, callback: Callable[[ProcessInfo], None]):
        """
        Add a callback for process status updates
        
        Args:
            pid: Process ID to monitor
            callback: Function to call with ProcessInfo
        """
        if pid not in self._process_callbacks:
            self._process_callbacks[pid] = []
        self._process_callbacks[pid].append(callback)
        
    def remove_process_callback(self, pid: int, callback: Callable[[ProcessInfo], None]):
        """
        Remove a process callback
        
        Args:
            pid: Process ID
            callback: Callback function to remove
        """
        if pid in self._process_callbacks:
            if callback in self._process_callbacks[pid]:
                self._process_callbacks[pid].remove(callback)
                
            if not self._process_callbacks[pid]:
                del self._process_callbacks[pid]
                
    def get_process_output(self, pid: int) -> Tuple[Optional[str], Optional[str]]:
        """
        Get stdout and stderr from a managed process
        
        Args:
            pid: Process ID
            
        Returns:
            Tuple of (stdout, stderr) if available, (None, None) otherwise
        """
        if pid not in self._managed_processes:
            return None, None
            
        process = self._managed_processes[pid]
        
        # Check if process has terminated
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            return stdout, stderr
            
        # Process still running, return None
        return None, None 