#include <gtest/gtest.h>
#include <QApplication>
#include <memory>

std::unique_ptr<QApplication> app;

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    
    // Create QApplication instance for GUI tests
    app = std::make_unique<QApplication>(argc, argv);
    
    return RUN_ALL_TESTS();
}
