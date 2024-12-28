#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QDockWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QSettings>
#include <memory>

class TimelineView;
class MediaLibrary;
class PropertyInspector;
class VideoExporter;
class ProxyManager;
class FrameCache;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File operations
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportVideo();
    
    // Edit operations
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void deleteSelection();
    
    // View operations
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    
    // Playback control
    void play();
    void pause();
    void stop();
    void seekForward();
    void seekBackward();
    void toggleMute();
    
    // Timeline operations
    void splitClip();
    void trimClipStart();
    void trimClipEnd();
    void addTransition();
    
    // Effect operations
    void addVideoEffect();
    void addAudioEffect();
    void addTextOverlay();
    
    // Settings
    void showSettings();
    void toggleDarkMode();

private:
    // UI setup
    void createActions();
    void createMenus();
    void createToolbars();
    void createDockWindows();
    void createStatusBar();
    void createShortcuts();
    
    // Settings
    void loadSettings();
    void saveSettings();
    void applyTheme();
    
    // Central widgets
    QVideoWidget* videoWidget;
    TimelineView* timelineView;
    
    // Dock widgets
    QDockWidget* mediaLibraryDock;
    QDockWidget* propertyInspectorDock;
    MediaLibrary* mediaLibrary;
    PropertyInspector* propertyInspector;
    
    // Toolbars
    QToolBar* fileToolBar;
    QToolBar* editToolBar;
    QToolBar* viewToolBar;
    QToolBar* playbackToolBar;
    QToolBar* effectsToolBar;
    
    // Media playback
    std::unique_ptr<QMediaPlayer> mediaPlayer;
    
    // Project management
    QString currentProjectFile;
    bool projectModified;
    
    // Performance optimization
    std::unique_ptr<VideoExporter> videoExporter;
    std::unique_ptr<ProxyManager> proxyManager;
    std::unique_ptr<FrameCache> frameCache;
    
    // Settings
    QSettings settings;
    bool darkMode;
};
