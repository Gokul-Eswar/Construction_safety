#include <gtest/gtest.h>
#include "inference/model_loader.hpp"
#include <fstream>
#include <cstdio>

TEST(ModelLoaderTest, FileExistenceCheck) {
    ModelLoader loader("non_existent_model.onnx");
    EXPECT_FALSE(loader.load());
}

TEST(ModelLoaderTest, SuccessfulLoadAndSave) {
    // Create a dummy model file
    std::string dummy_model = "test_model.onnx";
    std::ofstream f(dummy_model);
    f << "dummy data";
    f.close();
    
    ModelLoader loader(dummy_model);
    EXPECT_TRUE(loader.load());
    EXPECT_TRUE(loader.isLoaded());
    
    std::string engine_out = "test_model.engine";
    EXPECT_TRUE(loader.saveEngine(engine_out));
    
    // Verify engine file created
    std::ifstream ef(engine_out);
    EXPECT_TRUE(ef.good());
    ef.close();
    
    // Cleanup
    std::remove(dummy_model.c_str());
    std::remove(engine_out.c_str());
}
