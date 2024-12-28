#pragma once

#include <QWidget>
#include <QVariant>
#include <QString>

class PropertyWidget : public QWidget {
    Q_OBJECT

public:
    explicit PropertyWidget(QWidget* parent = nullptr);
    virtual ~PropertyWidget() = default;
    
    // Value management
    virtual QVariant getValue() const = 0;
    virtual void setValue(const QVariant& value) = 0;
    
    // Widget state
    virtual void setReadOnly(bool readOnly) = 0;
    virtual bool isReadOnly() const = 0;
    
    // Property name
    void setPropertyName(const QString& name) { propertyName = name; }
    QString getPropertyName() const { return propertyName; }

signals:
    void valueChanged(const QString& name, const QVariant& value);

protected:
    QString propertyName;
};

// Specialized property widgets
class StringPropertyWidget : public PropertyWidget {
    Q_OBJECT
public:
    explicit StringPropertyWidget(QWidget* parent = nullptr);
    QVariant getValue() const override;
    void setValue(const QVariant& value) override;
    void setReadOnly(bool readOnly) override;
    bool isReadOnly() const override;
private:
    class QLineEdit* lineEdit;
};

class NumberPropertyWidget : public PropertyWidget {
    Q_OBJECT
public:
    explicit NumberPropertyWidget(QWidget* parent = nullptr);
    QVariant getValue() const override;
    void setValue(const QVariant& value) override;
    void setReadOnly(bool readOnly) override;
    bool isReadOnly() const override;
    void setRange(double min, double max);
    void setDecimals(int decimals);
private:
    class QDoubleSpinBox* spinBox;
};

class BoolPropertyWidget : public PropertyWidget {
    Q_OBJECT
public:
    explicit BoolPropertyWidget(QWidget* parent = nullptr);
    QVariant getValue() const override;
    void setValue(const QVariant& value) override;
    void setReadOnly(bool readOnly) override;
    bool isReadOnly() const override;
private:
    class QCheckBox* checkBox;
};

class ColorPropertyWidget : public PropertyWidget {
    Q_OBJECT
public:
    explicit ColorPropertyWidget(QWidget* parent = nullptr);
    QVariant getValue() const override;
    void setValue(const QVariant& value) override;
    void setReadOnly(bool readOnly) override;
    bool isReadOnly() const override;
private:
    class QPushButton* colorButton;
    QColor currentColor;
    void showColorDialog();
};

class EnumPropertyWidget : public PropertyWidget {
    Q_OBJECT
public:
    explicit EnumPropertyWidget(QWidget* parent = nullptr);
    QVariant getValue() const override;
    void setValue(const QVariant& value) override;
    void setReadOnly(bool readOnly) override;
    bool isReadOnly() const override;
    void setEnumNames(const QStringList& names);
private:
    class QComboBox* comboBox;
};

class Vector2DPropertyWidget : public PropertyWidget {
    Q_OBJECT
public:
    explicit Vector2DPropertyWidget(QWidget* parent = nullptr);
    QVariant getValue() const override;
    void setValue(const QVariant& value) override;
    void setReadOnly(bool readOnly) override;
    bool isReadOnly() const override;
    void setRange(double min, double max);
    void setDecimals(int decimals);
private:
    class QDoubleSpinBox* xSpinBox;
    class QDoubleSpinBox* ySpinBox;
    class QHBoxLayout* layout;
};
