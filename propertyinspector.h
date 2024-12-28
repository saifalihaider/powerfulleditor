#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QVariant>
#include <QHash>
#include <memory>

class PropertyWidget;

class PropertyInspector : public QWidget {
    Q_OBJECT

public:
    explicit PropertyInspector(QWidget* parent = nullptr);
    
    // Property management
    void setObject(QObject* obj);
    void clearProperties();
    void updateProperty(const QString& name, const QVariant& value);
    
    // Property visibility
    void showProperty(const QString& name);
    void hideProperty(const QString& name);
    void setPropertyEnabled(const QString& name, bool enabled);
    
    // Property grouping
    void beginGroup(const QString& name);
    void endGroup();
    
    // Custom properties
    void addCustomProperty(const QString& name, const QVariant& value,
                         const QString& type = QString());
    void removeCustomProperty(const QString& name);

signals:
    void propertyChanged(const QString& name, const QVariant& value);
    void customPropertyChanged(const QString& name, const QVariant& value);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void handlePropertyChanged(const QString& name, const QVariant& value);
    void handleGroupExpanded(const QString& name);
    void handleGroupCollapsed(const QString& name);

private:
    // UI components
    QScrollArea* scrollArea;
    QWidget* contentWidget;
    QVBoxLayout* mainLayout;
    QFormLayout* currentLayout;
    
    // Property management
    QObject* currentObject;
    QHash<QString, PropertyWidget*> propertyWidgets;
    QHash<QString, QVariant> customProperties;
    
    // Property groups
    struct PropertyGroup {
        QString name;
        QWidget* widget;
        QFormLayout* layout;
        bool expanded;
    };
    QList<PropertyGroup> propertyGroups;
    
    // Helper functions
    void setupUI();
    void createPropertyWidget(const QString& name, const QVariant& value,
                            const QString& type = QString());
    void updatePropertyWidget(PropertyWidget* widget, const QVariant& value);
    void clearLayout(QLayout* layout);
    
    // Property type handling
    QString getPropertyType(const QVariant& value) const;
    PropertyWidget* createWidgetForType(const QString& type);
    
    // Layout management
    void adjustScrollArea();
    void updateGroupVisibility();
    
    // Settings
    void saveSettings();
    void loadSettings();
};
