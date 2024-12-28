#include <gtest/gtest.h>
#include <QTest>
#include <QSignalSpy>
#include "../src/mainwindow.h"
#include "../src/medialibrary.h"
#include "../src/propertyinspector.h"

class UITest : public ::testing::Test {
protected:
    void SetUp() override {
        mainWindow = new MainWindow();
        mediaLibrary = mainWindow->findChild<MediaLibrary*>();
        ASSERT_TRUE(mediaLibrary != nullptr);
        
        propertyInspector = mainWindow->findChild<PropertyInspector*>();
        ASSERT_TRUE(propertyInspector != nullptr);
    }
    
    void TearDown() override {
        delete mainWindow;
    }
    
    MainWindow* mainWindow;
    MediaLibrary* mediaLibrary;
    PropertyInspector* propertyInspector;
};

// MainWindow Tests
TEST_F(UITest, TestWindowCreation) {
    ASSERT_TRUE(mainWindow->isVisible());
    ASSERT_FALSE(mainWindow->windowTitle().isEmpty());
}

TEST_F(UITest, TestMenuBarExists) {
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    ASSERT_TRUE(menuBar != nullptr);
    ASSERT_GT(menuBar->actions().count(), 0);
}

TEST_F(UITest, TestToolbarsExist) {
    QList<QToolBar*> toolbars = mainWindow->findChildren<QToolBar*>();
    ASSERT_GT(toolbars.size(), 0);
}

// MediaLibrary Tests
TEST_F(UITest, TestMediaLibraryImport) {
    QSignalSpy spy(mediaLibrary, SIGNAL(mediaItemSelected(QString)));
    
    // Create temporary test file
    QTemporaryFile testFile;
    ASSERT_TRUE(testFile.open());
    testFile.write("test data");
    testFile.close();
    
    // Import file
    mediaLibrary->importFiles({testFile.fileName()});
    
    // Check if file was imported
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(UITest, TestMediaLibraryDragDrop) {
    // Simulate drag and drop
    QMimeData mimeData;
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile("test.mp4");
    mimeData.setUrls(urls);
    
    QDragEnterEvent enterEvent(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(mediaLibrary, &enterEvent);
    ASSERT_TRUE(enterEvent.isAccepted());
}

// PropertyInspector Tests
TEST_F(UITest, TestPropertyInspector) {
    // Create test object with properties
    QObject testObj;
    testObj.setProperty("name", "Test Object");
    testObj.setProperty("value", 42);
    
    propertyInspector->setObject(&testObj);
    
    // Check if properties are displayed
    QList<PropertyWidget*> widgets = propertyInspector->findChildren<PropertyWidget*>();
    ASSERT_GT(widgets.size(), 0);
}

TEST_F(UITest, TestPropertyEditing) {
    QObject testObj;
    testObj.setProperty("value", 42);
    
    propertyInspector->setObject(&testObj);
    
    // Find the property widget
    NumberPropertyWidget* valueWidget = propertyInspector->findChild<NumberPropertyWidget*>();
    ASSERT_TRUE(valueWidget != nullptr);
    
    // Change the value
    QSignalSpy spy(valueWidget, SIGNAL(valueChanged(QString,QVariant)));
    valueWidget->setValue(100);
    
    ASSERT_EQ(spy.count(), 1);
    ASSERT_EQ(testObj.property("value").toInt(), 100);
}

// Keyboard Shortcuts Tests
TEST_F(UITest, TestShortcuts) {
    // Test Undo shortcut
    QTest::keyClick(mainWindow, Qt::Key_Z, Qt::ControlModifier);
    
    // Test Save shortcut
    QTest::keyClick(mainWindow, Qt::Key_S, Qt::ControlModifier);
    
    // Test Play/Pause shortcut
    QTest::keyClick(mainWindow, Qt::Key_Space);
}

// Theme Tests
TEST_F(UITest, TestThemeSwitch) {
    QSignalSpy spy(mainWindow, SIGNAL(themeChanged(bool)));
    
    // Toggle dark mode
    mainWindow->findChild<QAction*>("actionToggleDarkMode")->trigger();
    
    ASSERT_EQ(spy.count(), 1);
}

// Performance Tests
TEST_F(UITest, TestUIResponsiveness) {
    QElapsedTimer timer;
    timer.start();
    
    // Perform multiple UI operations
    for (int i = 0; i < 100; i++) {
        mainWindow->resize(800 + i, 600 + i);
        QApplication::processEvents();
    }
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "UI resize operations took:" << elapsed << "ms";
    
    // UI operations should be responsive
    ASSERT_LT(elapsed, 1000); // Should take less than 1 second
}

// Memory Tests
TEST_F(UITest, TestMemoryUsage) {
    size_t initialMemory = getCurrentMemoryUsage();
    
    // Perform memory-intensive operations
    for (int i = 0; i < 10; i++) {
        MediaLibrary* tempLibrary = new MediaLibrary();
        delete tempLibrary;
    }
    
    size_t finalMemory = getCurrentMemoryUsage();
    size_t difference = finalMemory - initialMemory;
    
    qDebug() << "Memory usage difference:" << difference << "bytes";
    
    // Memory usage should not increase significantly
    ASSERT_LT(difference, 1024 * 1024); // Less than 1MB difference
}

// Helper function to get current memory usage
size_t getCurrentMemoryUsage() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
#else
    return 0;
#endif
}
