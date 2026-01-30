#include <gtest/gtest.h>
#include "inference/model_loader.hpp"
#include <fstream>
#include <cstdio>

TEST(ModelLoaderTest, FileExistenceCheck) {
    ModelLoader loader("non_existent_model.onnx");
    EXPECT_FALSE(loader.load());
}

TEST(ModelLoaderTest, LoadInvalidOnnxFails) {
    // Create a dummy model file (garbage data)
    std::string dummy_model = "test_model.onnx";
    std::ofstream f(dummy_model);
    f << "dummy data";
    f.close();
    
    ModelLoader loader(dummy_model);
    // Should FAIL because it's not a valid ONNX file
    EXPECT_FALSE(loader.load());
    EXPECT_FALSE(loader.isLoaded());
    
    // Cleanup
    std::remove(dummy_model.c_str());
}
