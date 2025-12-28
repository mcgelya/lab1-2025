#pragma once

#include <QMainWindow>

class QButtonGroup;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QLabel;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onBrowseOut();
    void onEncodePreview();
    void onEncodeAndSave();
    void onSourceChanged();

private:
    enum class SourceMode { Text, File, Random };

    QRadioButton* textRadio_ = nullptr;
    QRadioButton* fileRadio_ = nullptr;
    QRadioButton* randomRadio_ = nullptr;

    QPlainTextEdit* inputText_ = nullptr;
    QLineEdit* filePath_ = nullptr;
    QPushButton* browseBtn_ = nullptr;

    QLineEdit* outPath_ = nullptr;
    QPushButton* outBrowseBtn_ = nullptr;
    QSpinBox* randomMb_ = nullptr;

    QSpinBox* bufferSize_ = nullptr;

    QPlainTextEdit* outputText_ = nullptr;
    QPushButton* encodeBtn_ = nullptr;
    QPushButton* saveBtn_ = nullptr;
    QPushButton* clearBtn_ = nullptr;

    QLabel* status_ = nullptr;

private:
    SourceMode mode() const;
    void updateUiForMode();
    void setStatus(const QString& text);

    // Helpers
    QString encodePreview(size_t maxChars, bool* truncated);
    bool encodeToFile(const QString& outPath, QString* error);
};
