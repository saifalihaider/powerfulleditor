#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include "effectsmanager.h"

class EffectsDialog : public QDialog {
    Q_OBJECT

public:
    explicit EffectsDialog(EffectsManager* manager, QWidget* parent = nullptr);

private slots:
    void addNewEffect();
    void removeSelectedEffect();
    void updateEffectParameters();
    void previewEffect();
    void effectSelectionChanged();
    void updateProgress(int percent);

private:
    EffectsManager* effectsManager;
    
    // UI Elements
    QComboBox* effectTypeCombo;
    QListWidget* activeEffectsList;
    QWidget* parametersWidget;
    QVBoxLayout* parametersLayout;
    QPushButton* addButton;
    QPushButton* removeButton;
    QPushButton* previewButton;
    QPushButton* applyButton;
    QLabel* progressLabel;
    
    // Parameter controls
    QMap<QString, QSlider*> parameterSliders;
    
    void setupUI();
    void createEffectTypeCombo();
    void updateParametersUI(VideoEffect* effect);
    QSlider* createParameterSlider(const QString& name, double minValue, double maxValue, 
                                 double defaultValue, int precision = 100);
};
