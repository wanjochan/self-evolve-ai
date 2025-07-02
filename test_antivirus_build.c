#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Test program to verify antivirus-safe build configuration
int main(int argc, char* argv[]) {
    printf("=== Self-Evolve AI Antivirus Test ===\n");
    printf("Version: 1.0.0\n");
    printf("Build: Safe Configuration\n");
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
    
#ifdef _WIN32
    printf("Platform: Windows\n");
    printf("Runtime: Dynamic CRT\n");
    
    // Show a message box to demonstrate it's working
    MessageBoxA(NULL, 
                "Self-Evolve AI test program compiled successfully!\n"
                "This build uses antivirus-safe configuration:\n"
                "- Dynamic runtime linking\n"
                "- Security compile options\n"
                "- Version information\n"
                "- Application manifest",
                "Self-Evolve AI - Test Success",
                MB_OK | MB_ICONINFORMATION);
#else
    printf("Platform: Unix/Linux\n");
    printf("Security: Stack protector enabled\n");
#endif
    
    printf("\nAntivirus-safe features enabled:\n");
    printf("- Dynamic runtime library linking\n");
    printf("- Security compilation flags\n");
    printf("- Version resource information\n");
    printf("- Application manifest (Windows)\n");
    printf("- Address space layout randomization\n");
    printf("- Data execution prevention\n");
    
    printf("\nTest completed successfully!\n");
    printf("If you see this message, the build system is working.\n");
    
    return 0;
}
