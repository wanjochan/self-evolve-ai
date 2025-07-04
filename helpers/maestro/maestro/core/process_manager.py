import os
import subprocess
import time
import logging
from typing import Dict, List, Optional, Tuple

logger = logging.getLogger("maestro.process")

class ProcessInfo:
    """Process information container"""
    def __init__(self, pid: int, name: str, cmdline: List[str]):
        self.pid = pid
        self.name = name
        self.cmdline = cmdline
        self.created_time = time.time()
        
    def __str__(self) -> str:
        return f"Process(pid={self.pid}, name='{self.name}')"
               
    def to_dict(self) -> Dict:
        """Convert to dictionary representation"""
        return {
            "pid": self.pid,
            "name": self.name,
            "cmdline": self.cmdline,
            "created_time": self.created_time
        }

class ProcessManager:
    """Simple process management"""
    
    def __init__(self):
        """Initialize process manager"""
        self._managed_processes: Dict[int, subprocess.Popen] = {}
        
    def start_process(self, cmd, shell: bool = False, cwd: str = None) -> Optional[int]:
        """Start a new process"""
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
        """Kill a process by PID"""
        try:
            if pid in self._managed_processes:
                process = self._managed_processes[pid]
                process.kill()
                process.wait(timeout=5)
                del self._managed_processes[pid]
                logger.info(f"Killed managed process {pid}")
                return True
                
            # Try to kill external process
            import psutil
            process = psutil.Process(pid)
            process.kill()
            logger.info(f"Killed external process {pid}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to kill process {pid}: {e}")
            return False
            
    def get_process_output(self, pid: int) -> Tuple[Optional[str], Optional[str]]:
        """Get stdout and stderr from a managed process"""
        if pid not in self._managed_processes:
            return None, None
            
        process = self._managed_processes[pid]
        
        # Check if process has terminated
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            return stdout, stderr
            
        # Process still running, return None
        return None, None 