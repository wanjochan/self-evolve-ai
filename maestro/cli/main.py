"""
CLI interface for Maestro.
"""

import typer
from typing import Optional
from ..core import WindowManager, ProcessManager

app = typer.Typer(help="Maestro - Windows session manager and controller")
window_app = typer.Typer(help="Window management commands")
process_app = typer.Typer(help="Process management commands")

app.add_typer(window_app, name="window")
app.add_typer(process_app, name="process")

# Window commands
@window_app.command("list")
def list_windows():
    """List all visible windows."""
    wm = WindowManager()
    windows = wm.list_windows()
    
    if not windows:
        typer.echo("No visible windows found.")
        return
    
    for window in windows:
        typer.echo(f"\nWindow: {window['title']}")
        typer.echo(f"  Handle: {window['handle']}")
        typer.echo(f"  Position: {window['rect']}")
        typer.echo(f"  Minimized: {window['minimized']}")
        typer.echo(f"  Maximized: {window['maximized']}")

@window_app.command("capture")
def capture_window(
    title: str = typer.Argument(..., help="Window title (partial match)"),
    output: str = typer.Option("window.png", "--output", "-o", help="Output file path")
):
    """Capture a window's content to an image file."""
    wm = WindowManager()
    if not wm.find_window(title):
        typer.echo(f"No window found with title containing '{title}'")
        raise typer.Exit(1)
    
    image = wm.capture_window()
    if image:
        image.save(output)
        typer.echo(f"Window captured and saved to {output}")
    else:
        typer.echo("Failed to capture window")
        raise typer.Exit(1)

@window_app.command("move")
def move_window(
    title: str = typer.Argument(..., help="Window title (partial match)"),
    x: int = typer.Option(..., "--x", help="X coordinate"),
    y: int = typer.Option(..., "--y", help="Y coordinate"),
    width: Optional[int] = typer.Option(None, "--width", "-w", help="Window width"),
    height: Optional[int] = typer.Option(None, "--height", "-h", help="Window height")
):
    """Move and resize a window."""
    wm = WindowManager()
    if not wm.find_window(title):
        typer.echo(f"No window found with title containing '{title}'")
        raise typer.Exit(1)
    
    # If width/height not provided, keep current size
    if width is None or height is None:
        rect = wm.get_window_rect()
        if rect:
            if width is None:
                width = rect[2] - rect[0]
            if height is None:
                height = rect[3] - rect[1]
    
    if wm.move_window(x, y, width, height):
        typer.echo(f"Window moved to ({x}, {y}) with size {width}x{height}")
    else:
        typer.echo("Failed to move window")
        raise typer.Exit(1)

# Process commands
@process_app.command("list")
def list_processes():
    """List all running processes."""
    pm = ProcessManager()
    processes = pm.list_processes()
    
    if not processes:
        typer.echo("No processes found.")
        return
    
    for proc in processes:
        typer.echo(f"\nProcess: {proc['name']}")
        typer.echo(f"  PID: {proc['pid']}")
        typer.echo(f"  CPU: {proc['cpu_percent']}%")
        typer.echo(f"  Memory: {proc['memory_percent']:.1f}%")

@process_app.command("info")
def process_info(pid: int = typer.Argument(..., help="Process ID")):
    """Get detailed information about a process."""
    pm = ProcessManager()
    info = pm.get_process_info(pid)
    
    if not info:
        typer.echo(f"No process found with PID {pid}")
        raise typer.Exit(1)
    
    typer.echo(f"\nProcess Information:")
    for key, value in info.items():
        typer.echo(f"  {key}: {value}")

@process_app.command("kill")
def kill_process(
    pid: int = typer.Argument(..., help="Process ID"),
    force: bool = typer.Option(False, "--force", "-f", help="Force kill the process")
):
    """Kill a process."""
    pm = ProcessManager()
    if pm.kill_process(pid):
        typer.echo(f"Process {pid} killed successfully")
    else:
        typer.echo(f"Failed to kill process {pid}")
        raise typer.Exit(1)

@process_app.command("windows")
def process_windows(pid: int = typer.Argument(..., help="Process ID")):
    """List all windows belonging to a process."""
    pm = ProcessManager()
    windows = pm.get_process_windows(pid)
    
    if not windows:
        typer.echo(f"No windows found for process {pid}")
        return
    
    for window in windows:
        typer.echo(f"\nWindow: {window['title']}")
        typer.echo(f"  Handle: {window['handle']}")
        typer.echo(f"  Position: {window['rect']}")
        typer.echo(f"  Minimized: {window['minimized']}")
        typer.echo(f"  Maximized: {window['maximized']}")

if __name__ == "__main__":
    app() 