#include "mainwindow.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QOverload>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <chrono>
#include <memory>

#include "base64_encode_stream.hpp"
#include "random_byte_stream.hpp"
#include "read_stream.hpp"
#include "write_stream.hpp"

namespace {

std::unique_ptr<ReadOnlyStream<uint8_t>> makeTextStream(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    auto seq = std::make_shared<ArraySequence<uint8_t>>();
    seq->Clear();
    for (auto b : bytes) {
        seq->Append(static_cast<uint8_t>(static_cast<unsigned char>(b)));
    }
    return std::make_unique<SequenceReadStream<uint8_t>>(std::move(seq));
}

std::unique_ptr<ReadOnlyStream<uint8_t>> makeFileStream(const QString& path, QString* error) {
    try {
        auto parse = [](std::istream& is) -> uint8_t {
            return static_cast<uint8_t>(is.get());
        };
        return std::make_unique<FileReadStream<uint8_t, decltype(parse)>>(path.toStdString(), parse);
    } catch (const std::exception& ex) {
        if (error) {
            *error = ex.what();
        }
        return nullptr;
    }
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Base64 Encode Stream (buffered)");

    auto* central = new QWidget(this);
    setCentralWidget(central);

    // Source mode
    auto* modeBox = new QGroupBox("Source", central);
    textRadio_ = new QRadioButton("Text", modeBox);
    fileRadio_ = new QRadioButton("File", modeBox);
    randomRadio_ = new QRadioButton("Random bytes", modeBox);
    textRadio_->setChecked(true);

    connect(textRadio_, &QRadioButton::toggled, this, &MainWindow::onSourceChanged);
    connect(fileRadio_, &QRadioButton::toggled, this, &MainWindow::onSourceChanged);
    connect(randomRadio_, &QRadioButton::toggled, this, &MainWindow::onSourceChanged);

    auto* modeLayout = new QHBoxLayout(modeBox);
    modeLayout->addWidget(textRadio_);
    modeLayout->addWidget(fileRadio_);
    modeLayout->addWidget(randomRadio_);
    modeLayout->addStretch(1);

    // Text input
    inputText_ = new QPlainTextEdit(central);
    inputText_->setPlaceholderText("Enter text to encode (UTF-8)…");
    inputText_->setMinimumHeight(120);

    // File chooser
    filePath_ = new QLineEdit(central);
    browseBtn_ = new QPushButton("Browse…", central);
    connect(browseBtn_, &QPushButton::clicked, this, &MainWindow::onBrowse);

    // Output file chooser (used for File/Random modes; optional for Text)
    outPath_ = new QLineEdit(central);
    outPath_->setPlaceholderText("Path to save encoded Base64…");
    outBrowseBtn_ = new QPushButton("Browse…", central);
    connect(outBrowseBtn_, &QPushButton::clicked, this, &MainWindow::onBrowseOut);

    // Random generator controls
    randomMb_ = new QSpinBox(central);
    randomMb_->setRange(1, 4096);
    randomMb_->setValue(10);
    randomMb_->setSuffix(" MB");
    connect(randomMb_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        if (mode() == SourceMode::Random) {
            const QString cur = outPath_->text().trimmed();
            if (cur.isEmpty() || cur.startsWith("random_")) {
                outPath_->setText(QString("random_%1MB.b64").arg(v));
            }
        }
    });

    // Buffer size
    bufferSize_ = new QSpinBox(central);
    bufferSize_->setRange(1, 4 * 1024 * 1024);
    bufferSize_->setValue(3 * 1024);
    bufferSize_->setSuffix(" bytes");

    // Output
    outputText_ = new QPlainTextEdit(central);
    outputText_->setReadOnly(true);
    outputText_->setPlaceholderText("Base64 output will appear here…");
    outputText_->setMinimumHeight(160);

    // Buttons
    encodeBtn_ = new QPushButton("Encode (preview)", central);
    saveBtn_ = new QPushButton("Encode & Save…", central);
    clearBtn_ = new QPushButton("Clear", central);
    connect(encodeBtn_, &QPushButton::clicked, this, &MainWindow::onEncodePreview);
    connect(saveBtn_, &QPushButton::clicked, this, &MainWindow::onEncodeAndSave);
    connect(clearBtn_, &QPushButton::clicked, [this] {
        inputText_->clear();
        outputText_->clear();
        setStatus("Cleared");
    });

    // Layout
    auto* grid = new QGridLayout(central);
    int row = 0;
    grid->addWidget(modeBox, row++, 0, 1, 3);

    grid->addWidget(new QLabel("Input"), row++, 0, 1, 3);
    grid->addWidget(inputText_, row++, 0, 1, 3);

    auto* fileRow = new QWidget(central);
    auto* fileLayout = new QHBoxLayout(fileRow);
    fileLayout->setContentsMargins(0, 0, 0, 0);
    fileLayout->addWidget(new QLabel("File:"));
    fileLayout->addWidget(filePath_, 1);
    fileLayout->addWidget(browseBtn_);
    grid->addWidget(fileRow, row++, 0, 1, 3);

    auto* outRow = new QWidget(central);
    auto* outLayout = new QHBoxLayout(outRow);
    outLayout->setContentsMargins(0, 0, 0, 0);
    outLayout->addWidget(new QLabel("Output file:"));
    outLayout->addWidget(outPath_, 1);
    outLayout->addWidget(outBrowseBtn_);
    grid->addWidget(outRow, row++, 0, 1, 3);

    auto* randomRow = new QWidget(central);
    auto* randomLayout = new QHBoxLayout(randomRow);
    randomLayout->setContentsMargins(0, 0, 0, 0);
    randomLayout->addWidget(new QLabel("Random size:"));
    randomLayout->addWidget(randomMb_);
    randomLayout->addStretch(1);
    grid->addWidget(randomRow, row++, 0, 1, 3);

    auto* bufferRow = new QWidget(central);
    auto* bufferLayout = new QHBoxLayout(bufferRow);
    bufferLayout->setContentsMargins(0, 0, 0, 0);
    bufferLayout->addWidget(new QLabel("Input buffer size:"));
    bufferLayout->addWidget(bufferSize_);
    bufferLayout->addStretch(1);
    grid->addWidget(bufferRow, row++, 0, 1, 3);

    grid->addWidget(new QLabel("Base64 output"), row++, 0, 1, 3);
    grid->addWidget(outputText_, row++, 0, 1, 3);

    auto* btnRow = new QWidget(central);
    auto* btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->addWidget(encodeBtn_);
    btnLayout->addWidget(saveBtn_);
    btnLayout->addWidget(clearBtn_);
    btnLayout->addStretch(1);
    grid->addWidget(btnRow, row++, 0, 1, 3);

    status_ = new QLabel("Ready", this);
    statusBar()->addWidget(status_, 1);

    updateUiForMode();
}

MainWindow::SourceMode MainWindow::mode() const {
    if (fileRadio_->isChecked()) {
        return SourceMode::File;
    }
    if (randomRadio_->isChecked()) {
        return SourceMode::Random;
    }
    return SourceMode::Text;
}

void MainWindow::setStatus(const QString& text) {
    status_->setText(text);
}

void MainWindow::updateUiForMode() {
    const auto m = mode();

    inputText_->setEnabled(m == SourceMode::Text);

    filePath_->setEnabled(m == SourceMode::File);
    browseBtn_->setEnabled(m == SourceMode::File);

    randomMb_->setEnabled(m == SourceMode::Random);

    // Output path is required for File/Random streaming output; optional for Text.
    const bool needsOut = (m != SourceMode::Text);
    outPath_->setEnabled(true);
    outBrowseBtn_->setEnabled(true);

    if (needsOut) {
        outputText_->setPlaceholderText(
            "Preview only (limited). Full Base64 is written to the output file path below.");
        saveBtn_->setText("Encode to output file");
    } else {
        outputText_->setPlaceholderText("Base64 output will appear here…");
        saveBtn_->setText("Encode & Save…");
    }
}

void MainWindow::onSourceChanged() {
    updateUiForMode();

    if (mode() == SourceMode::Random && outPath_->text().trimmed().isEmpty()) {
        outPath_->setText(QString("random_%1MB.b64").arg(randomMb_->value()));
    }
}

void MainWindow::onBrowse() {
    const QString file = QFileDialog::getOpenFileName(this, "Choose input file");
    if (!file.isEmpty()) {
        filePath_->setText(file);

        if (outPath_->text().trimmed().isEmpty()) {
            outPath_->setText(file + ".b64");
        }
    }
}

void MainWindow::onBrowseOut() {
    QString suggested = outPath_->text().trimmed();
    if (suggested.isEmpty()) {
        suggested = "encoded.b64";
    }
    const QString out = QFileDialog::getSaveFileName(this, "Choose output file", suggested);
    if (!out.isEmpty()) {
        outPath_->setText(out);
    }
}

QString MainWindow::encodePreview(size_t maxChars, bool* truncated) {
    if (truncated) {
        *truncated = false;
    }

    QString error;
    std::unique_ptr<ReadOnlyStream<uint8_t>> src;
    size_t srcSizeBytes = 0;

    switch (mode()) {
        case SourceMode::Text:
            src = makeTextStream(inputText_->toPlainText());
            srcSizeBytes = static_cast<size_t>(inputText_->toPlainText().toUtf8().size());
            break;
        case SourceMode::File: {
            const QString path = filePath_->text().trimmed();
            if (path.isEmpty()) {
                throw std::runtime_error("No file chosen");
            }
            QFileInfo fi(path);
            srcSizeBytes = static_cast<size_t>(fi.size());
            src = makeFileStream(path, &error);
            if (!src) {
                throw std::runtime_error(error.toStdString());
            }
            break;
        }
        case SourceMode::Random:
            srcSizeBytes = static_cast<size_t>(randomMb_->value()) * 1024ull * 1024ull;
            src = std::make_unique<RandomByteStream>(srcSizeBytes);
            break;
    }

    auto start = std::chrono::steady_clock::now();

    auto encoder = std::make_unique<Base64EncodeStream>(std::move(src), static_cast<size_t>(bufferSize_->value()));
    std::string out;
    out.reserve(std::min<size_t>(maxChars, 1024 * 1024));

    while (!encoder->IsEndOfStream()) {
        char c = encoder->Read();
        if (out.size() < maxChars) {
            out.push_back(c);
        } else {
            if (truncated) {
                *truncated = true;
            }
            break;
        }
    }

    auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    const double approxOut = (srcSizeBytes + 2) / 3.0 * 4.0;
    QString note = QString("Preview in %1 ms. Input ~%2 bytes, output ~%3 chars")
                       .arg(ms)
                       .arg(srcSizeBytes)
                       .arg(static_cast<qulonglong>(approxOut));
    if (truncated && *truncated) {
        note += QString(" (preview truncated to %1 chars)").arg(maxChars);
    }
    setStatus(note);

    return QString::fromLatin1(out.data(), static_cast<int>(out.size()));
}

bool MainWindow::encodeToFile(const QString& outPath, QString* error) {
    QString localError;
    std::unique_ptr<ReadOnlyStream<uint8_t>> src;

    switch (mode()) {
        case SourceMode::Text:
            src = makeTextStream(inputText_->toPlainText());
            break;
        case SourceMode::File: {
            const QString path = filePath_->text().trimmed();
            if (path.isEmpty()) {
                localError = "No file chosen";
                if (error) {
                    *error = localError;
                }
                return false;
            }
            src = makeFileStream(path, &localError);
            if (!src) {
                if (error) {
                    *error = localError;
                }
                return false;
            }
            break;
        }
        case SourceMode::Random: {
            const size_t bytes = static_cast<size_t>(randomMb_->value()) * 1024ull * 1024ull;
            src = std::make_unique<RandomByteStream>(bytes);
            break;
        }
    }

    try {
        auto serialize = [](std::ostream& os, char c) {
            os.put(c);
        };

        auto start = std::chrono::steady_clock::now();

        auto encoder = std::make_unique<Base64EncodeStream>(std::move(src), static_cast<size_t>(bufferSize_->value()));
        auto writer = std::make_unique<FileWriteStream<char, decltype(serialize)>>(outPath.toStdString(), serialize);

        while (!encoder->IsEndOfStream()) {
            writer->Write(encoder->Read());
        }
        writer->Close();

        auto end = std::chrono::steady_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        setStatus(QString("Saved to %1 (%2 ms)").arg(outPath).arg(ms));
        return true;
    } catch (const std::exception& ex) {
        if (error) {
            *error = ex.what();
        }
        return false;
    }
}

void MainWindow::onEncodePreview() {
    try {
        bool truncated = false;
        const QString out = encodePreview(200000, &truncated);
        outputText_->setPlainText(out);
        if (truncated) {
            outputText_->appendPlainText("\n… (preview truncated; use the output file option for full output)");
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", ex.what());
    }
}

void MainWindow::onEncodeAndSave() {
    QString outPath = outPath_->text().trimmed();
    if (outPath.isEmpty()) {
        QString suggested = "encoded.b64";
        if (mode() == SourceMode::File && !filePath_->text().trimmed().isEmpty()) {
            suggested = filePath_->text().trimmed() + ".b64";
        } else if (mode() == SourceMode::Random) {
            suggested = QString("random_%1MB.b64").arg(randomMb_->value());
        }
        outPath = QFileDialog::getSaveFileName(this, "Choose output file", suggested);
        if (outPath.isEmpty()) {
            return;
        }
        outPath_->setText(outPath);
    }
    QString error;
    if (!encodeToFile(outPath, &error)) {
        QMessageBox::critical(this, "Error", error);
    }
}
