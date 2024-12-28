#include "effectsdialog.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>

EffectsDialog::EffectsDialog(EffectsManager* manager, QWidget* parent)
    : QDialog(parent)
    , effectsManager(manager)
{
    setupUI();
    
    connect(effectsManager, &EffectsManager::effectsChanged,
            this, &EffectsDialog::updateEffectParameters);
    connect(effectsManager, &EffectsManager::progressUpdated,
            this, &EffectsDialog::updateProgress);
}

void EffectsDialog::setupUI() {
    setWindowTitle("Video Effects");
    setMinimumSize(400, 500);
    
    auto mainLayout = new QVBoxLayout(this);
    
    // Effects selection area
    auto effectsGroup = new QGroupBox("Effects", this);
    auto effectsLayout = new QVBoxLayout(effectsGroup);
    
    createEffectTypeCombo();
    effectsLayout->addWidget(effectTypeCombo);
    
    addButton = new QPushButton("Add Effect", this);
    effectsLayout->addWidget(addButton);
    
    mainLayout->addWidget(effectsGroup);
    
    // Active effects list
    auto activeGroup = new QGroupBox("Active Effects", this);
    auto activeLayout = new QVBoxLayout(activeGroup);
    
    activeEffectsList = new QListWidget(this);
    activeLayout->addWidget(activeEffectsList);
    
    removeButton = new QPushButton("Remove Effect", this);
    activeLayout->addWidget(removeButton);
    
    mainLayout->addWidget(activeGroup);
    
    // Parameters area
    auto paramsGroup = new QGroupBox("Effect Parameters", this);
    parametersLayout = new QVBoxLayout(paramsGroup);
    parametersWidget = new QWidget(this);
    parametersLayout->addWidget(parametersWidget);
    
    mainLayout->addWidget(paramsGroup);
    
    // Preview and Apply buttons
    auto buttonLayout = new QHBoxLayout();
    previewButton = new QPushButton("Preview", this);
    applyButton = new QPushButton("Apply", this);
    buttonLayout->addWidget(previewButton);
    buttonLayout->addWidget(applyButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Progress label
    progressLabel = new QLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(progressLabel);
    
    // Connect signals
    connect(addButton, &QPushButton::clicked, this, &EffectsDialog::addNewEffect);
    connect(removeButton, &QPushButton::clicked, this, &EffectsDialog::removeSelectedEffect);
    connect(previewButton, &QPushButton::clicked, this, &EffectsDialog::previewEffect);
    connect(activeEffectsList, &QListWidget::currentRowChanged,
            this, &EffectsDialog::effectSelectionChanged);
    connect(applyButton, &QPushButton::clicked, this, &QDialog::accept);
}

void EffectsDialog::createEffectTypeCombo() {
    effectTypeCombo = new QComboBox(this);
    effectTypeCombo->addItem("Brightness", static_cast<int>(EffectType::Brightness));
    effectTypeCombo->addItem("Contrast", static_cast<int>(EffectType::Contrast));
    effectTypeCombo->addItem("Blur", static_cast<int>(EffectType::Blur));
    effectTypeCombo->addItem("Sharpen", static_cast<int>(EffectType::Sharpen));
    effectTypeCombo->addItem("Fade", static_cast<int>(EffectType::Fade));
}

void EffectsDialog::addNewEffect() {
    EffectType type = static_cast<EffectType>(
        effectTypeCombo->currentData().toInt());
    
    std::unique_ptr<VideoEffect> effect;
    switch (type) {
        case EffectType::Brightness:
            effect = std::make_unique<BrightnessEffect>();
            break;
        case EffectType::Contrast:
            effect = std::make_unique<ContrastEffect>();
            break;
        case EffectType::Blur:
            effect = std::make_unique<BlurEffect>();
            break;
        case EffectType::Sharpen:
            effect = std::make_unique<SharpenEffect>();
            break;
        case EffectType::Fade:
            effect = std::make_unique<FadeEffect>();
            break;
        default:
            return;
    }
    
    effectsManager->addEffect(std::move(effect));
    activeEffectsList->addItem(effectTypeCombo->currentText());
}

void EffectsDialog::removeSelectedEffect() {
    int row = activeEffectsList->currentRow();
    if (row >= 0) {
        effectsManager->removeEffect(row);
        delete activeEffectsList->takeItem(row);
    }
}

void EffectsDialog::updateEffectParameters() {
    int row = activeEffectsList->currentRow();
    if (row >= 0 && row < effectsManager->getEffects().size()) {
        updateParametersUI(effectsManager->getEffects()[row].get());
    }
}

void EffectsDialog::effectSelectionChanged() {
    updateEffectParameters();
}

void EffectsDialog::updateParametersUI(VideoEffect* effect) {
    // Clear existing parameters
    delete parametersWidget;
    parametersWidget = new QWidget(this);
    auto layout = new QVBoxLayout(parametersWidget);
    parameterSliders.clear();
    
    if (!effect) return;
    
    // Add sliders for each parameter
    switch (effect->getType()) {
        case EffectType::Brightness:
            parameterSliders["brightness"] = createParameterSlider("Brightness", -1.0, 1.0, 0.0);
            break;
        case EffectType::Contrast:
            parameterSliders["contrast"] = createParameterSlider("Contrast", 0.0, 2.0, 1.0);
            break;
        case EffectType::Blur:
            parameterSliders["radius"] = createParameterSlider("Radius", 1.0, 20.0, 5.0);
            break;
        case EffectType::Sharpen:
            parameterSliders["amount"] = createParameterSlider("Amount", 0.0, 5.0, 1.0);
            break;
        case EffectType::Fade:
            parameterSliders["start_time"] = createParameterSlider("Start Time", 0.0, 10.0, 0.0);
            parameterSliders["duration"] = createParameterSlider("Duration", 0.1, 5.0, 1.0);
            break;
    }
    
    parametersLayout->addWidget(parametersWidget);
}

QSlider* EffectsDialog::createParameterSlider(const QString& name, double minValue, 
                                            double maxValue, double defaultValue, 
                                            int precision) {
    auto container = new QWidget(parametersWidget);
    auto layout = new QVBoxLayout(container);
    
    auto label = new QLabel(name, container);
    layout->addWidget(label);
    
    auto slider = new QSlider(Qt::Horizontal, container);
    slider->setMinimum(minValue * precision);
    slider->setMaximum(maxValue * precision);
    slider->setValue(defaultValue * precision);
    layout->addWidget(slider);
    
    auto valueLabel = new QLabel(QString::number(defaultValue, 'f', 2), container);
    layout->addWidget(valueLabel);
    
    connect(slider, &QSlider::valueChanged, [=](int value) {
        double realValue = value / static_cast<double>(precision);
        valueLabel->setText(QString::number(realValue, 'f', 2));
        // Update effect parameter
        int row = activeEffectsList->currentRow();
        if (row >= 0 && row < effectsManager->getEffects().size()) {
            effectsManager->getEffects()[row]->setParameter(name, realValue);
        }
    });
    
    parametersWidget->layout()->addWidget(container);
    return slider;
}

void EffectsDialog::previewEffect() {
    // Implementation depends on your preview system
    // This could generate a preview frame or update a video preview
    emit effectsManager->processingStarted();
    progressLabel->setText("Generating preview...");
}

void EffectsDialog::updateProgress(int percent) {
    progressLabel->setText(QString("Processing: %1%").arg(percent));
}
