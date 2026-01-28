import unittest
import subprocess
import os
import shutil

class TestEnvCheck(unittest.TestCase):
    def test_env_check_executable(self):
        """Test that the environment check utility runs and reports success."""
        # Assume build is done and executable is in build/ or bin/
        # Adjust path based on where CMake puts it. Windows might be build/Debug/env_check.exe or similar.
        
        # Simple heuristic to find the executable
        exe_name = "env_check.exe" if os.name == 'nt' else "env_check"
        paths_to_check = [
            os.path.join("build", exe_name),
            os.path.join("build", "Debug", exe_name),
            os.path.join("build", "Release", exe_name),
            os.path.join("bin", exe_name)
        ]
        
        exe_path = None
        for p in paths_to_check:
            if os.path.exists(p):
                exe_path = p
                break
        
        if not exe_path:
            self.fail(f"Executable {exe_name} not found in build directories.")
            
        # Run it
        result = subprocess.run([exe_path], capture_output=True, text=True)
        
        self.assertEqual(result.returncode, 0, "Env check utility should exit with 0")
        
        # Verify it reports GStreamer status
        self.assertRegex(result.stdout, r"GStreamer Initialized: (Yes|No)")
        
        # Verify it reports CUDA status
        self.assertRegex(result.stdout, r"CUDA Available: (Yes|No.*)")

if __name__ == '__main__':
    unittest.main()
