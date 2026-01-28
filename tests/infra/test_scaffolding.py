import unittest
import os

class TestProjectScaffolding(unittest.TestCase):
    def test_directory_structure(self):
        """Test that key project directories exist."""
        self.assertTrue(os.path.exists("src"), "src directory should exist")
        self.assertTrue(os.path.exists("build"), "build directory should exist")
    
    def test_cmakelists_existence(self):
        """Test that CMakeLists.txt exists."""
        self.assertTrue(os.path.exists("CMakeLists.txt"), "CMakeLists.txt should exist")

    def test_cmakelists_content(self):
        """Test that CMakeLists.txt contains required dependencies."""
        if not os.path.exists("CMakeLists.txt"):
            self.fail("CMakeLists.txt does not exist")
            
        with open("CMakeLists.txt", "r") as f:
            content = f.read()
            
        self.assertIn("cmake_minimum_required", content)
        self.assertIn("find_package(GStreamer", content)
        self.assertIn("find_package(OpenCV", content)
        
        # Check for CUDA (either via find_package or enable_language)
        has_cuda = "find_package(CUDA" in content or "enable_language(CUDA)" in content
        self.assertTrue(has_cuda, "CUDA configuration missing in CMakeLists.txt")

if __name__ == '__main__':
    unittest.main()
