#pragma once

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QMenu>
#include <QDir>

class MediaLibrary : public QWidget {
    Q_OBJECT

public:
    explicit MediaLibrary(QWidget* parent = nullptr);
    
    // File operations
    void importFiles();
    void importFolder();
    void removeSelectedItems();
    void clearLibrary();
    
    // Asset management
    QString getSelectedFilePath() const;
    QStringList getSelectedFilePaths() const;
    
    // Project management
    void saveToProject(const QString& projectFile);
    void loadFromProject(const QString& projectFile);

signals:
    void mediaItemDropped(const QString& filePath, const QPoint& pos);
    void mediaItemSelected(const QString& filePath);
    void mediaItemDoubleClicked(const QString& filePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void handleItemActivated(const QModelIndex& index);
    void handleSelectionChanged();
    void handleCustomContextMenu(const QPoint& pos);
    
    // Context menu actions
    void previewMedia();
    void showFileProperties();
    void revealInExplorer();

private:
    // UI components
    QListView* listView;
    QStandardItemModel* model;
    QMenu* contextMenu;
    
    // Helper functions
    void setupUI();
    void createContextMenu();
    void addFile(const QString& filePath);
    void addFolder(const QString& folderPath);
    QString getFileType(const QString& filePath) const;
    QIcon getFileIcon(const QString& filePath) const;
    
    // File management
    QStringList supportedVideoFormats;
    QStringList supportedAudioFormats;
    QStringList supportedImageFormats;
    
    void initializeSupportedFormats();
    bool isSupported(const QString& filePath) const;
    
    // Drag and drop
    void startDrag(const QModelIndex& index);
    
    // Cache directory for thumbnails
    QDir thumbnailDir;
    void ensureThumbnailDirectory();
    QString generateThumbnail(const QString& filePath);
};
