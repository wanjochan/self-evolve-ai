import sys
import argparse

def main():
    parser = argparse.ArgumentParser(description="Maestro - UI Control and Automation")
    parser.add_argument("mode", choices=["cli", "web"],
                      help="Run mode: cli or web")
    parser.add_argument("--host", default="127.0.0.1",
                      help="Web server host (web mode only)")
    parser.add_argument("--port", type=int, default=5000,
                      help="Web server port (web mode only)")
    parser.add_argument("--weights-dir", default="weights",
                      help="Directory containing model weights")
    
    args, remaining = parser.parse_known_args()
    
    if args.mode == "cli":
        # Run CLI mode
        from .cli.main import main as cli_main
        sys.argv = [sys.argv[0]] + remaining
        cli_main()
    else:
        # Run web server mode
        from .web_server import run_server
        run_server(host=args.host, port=args.port, weights_dir=args.weights_dir)

if __name__ == "__main__":
    main() 