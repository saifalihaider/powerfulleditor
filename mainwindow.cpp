#include "mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(new QWidget)
    , mainLayout(new QVBoxLayout)
    , importButton(new QPushButton("Import Media File"))
    , exportButton(new QPushButton("Export Media File"))
    , metadataLabel(new QLabel("No file loaded"))
{
    setCentralWidget(centralWidget);
    centralWidget->setLayout(mainLayout);

    mainLayout->addWidget(importButton);
    mainLayout->addWidget(exportButton);
    mainLayout->addWidget(metadataLabel);

    connect(importButton, &QPushButton::clicked, this, &MainWindow::importFile);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportFile);

    exportButton->setEnabled(false);
    setMinimumSize(400, 300);
    setWindowTitle("Media File Manager");
}

void MainWindow::importFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        "Select Media File",
        QString(),
        "Media Files (*.mp4 *.avi *.mov *.mkv *.mp3 *.wav *.png *.jpg *.jpeg)");

    if (!filePath.isEmpty()) {
        currentFilePath = filePath;
        QString metadata = mediaProcessor.extractMetadata(filePath);
        displayMetadata(metadata);
        exportButton->setEnabled(true);
    }
}

void MainWindow::exportFile()
{
    if (currentFilePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "No file loaded to export");
        return;
    }

    QString saveFilePath = QFileDialog::getSaveFileName(this,
        "Export Media File",
        QString(),
        "Media Files (*.mp4 *.avi *.mov *.mkv *.mp3 *.wav *.png *.jpg *.jpeg)");

    if (!saveFilePath.isEmpty()) {
        if (mediaProcessor.exportFile(currentFilePath, saveFilePath)) {
            QMessageBox::information(this, "Success", "File exported successfully");
        } else {
            QMessageBox::critical(this, "Error", "Failed to export file");
        }
    }
}

void MainWindow::displayMetadata(const QString& metadata)
{
    metadataLabel->setText(metadata);
}
