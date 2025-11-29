#include "tileviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QDebug>

TileViewer::TileViewer(GBAROReader *romReader, QWidget *parent)
    : QDialog(parent),
      m_romReader(romReader),
      m_currentOffset(0),
      m_paletteOffset(0),
      m_tilesPerRow(16),
      m_tilesPerPage(256),
      m_tileWidth(8),
      m_tileHeight(8),
      m_bpp(4)
{
    setupUI();
    updateDisplay();
}

TileViewer::~TileViewer()
{
}

void TileViewer::setupUI()
{
    setWindowTitle("GBA ROM Tile Viewer");
    resize(800, 700);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Info label at top
    m_infoLabel = new QLabel();
    m_infoLabel->setStyleSheet("background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc;");
    mainLayout->addWidget(m_infoLabel);

    // Tile display area (scrollable)
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setStyleSheet("background-color: #303030;");

    m_tileDisplay = new QLabel();
    m_tileDisplay->setAlignment(Qt::AlignCenter);
    m_scrollArea->setWidget(m_tileDisplay);
    mainLayout->addWidget(m_scrollArea, 1);

    // Controls panel
    QGroupBox *controlsGroup = new QGroupBox("Controls");
    QGridLayout *controlsLayout = new QGridLayout(controlsGroup);

    // Offset controls
    controlsLayout->addWidget(new QLabel("Tile Offset (hex):"), 0, 0);
    m_offsetInput = new QLineEdit("0");
    m_offsetInput->setMaximumWidth(120);
    controlsLayout->addWidget(m_offsetInput, 0, 1);

    m_jumpButton = new QPushButton("Jump");
    m_jumpButton->setMaximumWidth(80);
    connect(m_jumpButton, &QPushButton::clicked, this, &TileViewer::onJumpToOffset);
    controlsLayout->addWidget(m_jumpButton, 0, 2);

    // Palette controls
    controlsLayout->addWidget(new QLabel("Palette Offset (hex):"), 1, 0);
    m_paletteInput = new QLineEdit("0");
    m_paletteInput->setMaximumWidth(120);
    connect(m_paletteInput, &QLineEdit::returnPressed, this, &TileViewer::onPaletteOffsetChanged);
    controlsLayout->addWidget(m_paletteInput, 1, 1);

    QPushButton *applyPaletteBtn = new QPushButton("Apply");
    applyPaletteBtn->setMaximumWidth(80);
    connect(applyPaletteBtn, &QPushButton::clicked, this, &TileViewer::onPaletteOffsetChanged);
    controlsLayout->addWidget(applyPaletteBtn, 1, 2);

    // BPP mode selector
    controlsLayout->addWidget(new QLabel("Bits Per Pixel:"), 2, 0);
    m_bppCombo = new QComboBox();
    m_bppCombo->addItem("4bpp (sprites/tiles)", 4);
    m_bppCombo->addItem("2bpp (fonts)", 2);
    m_bppCombo->setCurrentIndex(0);
    connect(m_bppCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TileViewer::updateDisplay);
    controlsLayout->addWidget(m_bppCombo, 2, 1);

    // Tile size
    controlsLayout->addWidget(new QLabel("Tile Size:"), 3, 0);
    m_tileSizeCombo = new QComboBox();
    m_tileSizeCombo->addItem("8x8", 8);
    m_tileSizeCombo->addItem("16x16", 16);
    m_tileSizeCombo->addItem("32x32", 32);
    m_tileSizeCombo->setCurrentIndex(0);
    connect(m_tileSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TileViewer::updateDisplay);
    controlsLayout->addWidget(m_tileSizeCombo, 3, 1);

    // Tiles per row
    controlsLayout->addWidget(new QLabel("Tiles Per Row:"), 4, 0);
    m_tilesPerRowSpin = new QSpinBox();
    m_tilesPerRowSpin->setRange(1, 32);
    m_tilesPerRowSpin->setValue(16);
    connect(m_tilesPerRowSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TileViewer::updateDisplay);
    controlsLayout->addWidget(m_tilesPerRowSpin, 4, 1);

    // Tiles per page
    controlsLayout->addWidget(new QLabel("Tiles Per Page:"), 5, 0);
    m_tilesPerPageSpin = new QSpinBox();
    m_tilesPerPageSpin->setRange(16, 1024);
    m_tilesPerPageSpin->setSingleStep(16);
    m_tilesPerPageSpin->setValue(256);
    connect(m_tilesPerPageSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TileViewer::updateDisplay);
    controlsLayout->addWidget(m_tilesPerPageSpin, 5, 1);

    mainLayout->addWidget(controlsGroup);

    // Navigation buttons
    QHBoxLayout *navLayout = new QHBoxLayout();
    m_prevButton = new QPushButton("◄ Previous Page");
    m_nextButton = new QPushButton("Next Page ►");
    m_saveButton = new QPushButton("💾 Save Image");

    connect(m_prevButton, &QPushButton::clicked, this, &TileViewer::onPrevPage);
    connect(m_nextButton, &QPushButton::clicked, this, &TileViewer::onNextPage);
    connect(m_saveButton, &QPushButton::clicked, this, &TileViewer::onSaveImage);

    navLayout->addWidget(m_prevButton);
    navLayout->addWidget(m_nextButton);
    navLayout->addStretch();
    navLayout->addWidget(m_saveButton);

    mainLayout->addLayout(navLayout);
}

void TileViewer::updateDisplay()
{
    if (!m_romReader || !m_romReader->isLoaded()) {
        m_infoLabel->setText("No ROM loaded");
        return;
    }

    // Update settings from UI
    m_tilesPerRow = m_tilesPerRowSpin->value();
    m_tilesPerPage = m_tilesPerPageSpin->value();
    m_tileWidth = m_tileSizeCombo->currentData().toInt();
    m_tileHeight = m_tileWidth;
    m_bpp = m_bppCombo->currentData().toInt();

    renderTiles();

    // Update info label
    QString info = QString("Offset: 0x%1 | Palette: 0x%2 | %3bpp | Tile Size: %4x%5 | Tiles: %6")
                       .arg(m_currentOffset, 8, 16, QChar('0'))
                       .arg(m_paletteOffset, 8, 16, QChar('0'))
                       .arg(m_bpp)
                       .arg(m_tileWidth)
                       .arg(m_tileHeight)
                       .arg(m_tilesPerPage);
    m_infoLabel->setText(info);
}

void TileViewer::renderTiles()
{
    // Calculate image dimensions
    int tilesX = m_tilesPerRow;
    int tilesY = (m_tilesPerPage + tilesX - 1) / tilesX;
    int imageWidth = tilesX * m_tileWidth;
    int imageHeight = tilesY * m_tileHeight;

    QImage tileImage;

    if (m_bpp == 4) {
        // 4bpp mode - use palette from ROM
        QVector<QRgb> palette = m_romReader->extractPalette(m_paletteOffset, 16);
        tileImage = m_romReader->extractTile4bpp(m_currentOffset, palette, imageWidth, imageHeight);
    } else {
        // 2bpp mode - extract font-style tiles
        // Calculate number of 8x8 tiles needed
        int tileColumns = imageWidth / 8;
        int tileRows = imageHeight / 8;
        int numChars = tileColumns * tileRows;

        tileImage = m_romReader->extractFont2bpp(m_currentOffset, numChars, 8, 8, tileColumns);
    }

    if (tileImage.isNull()) {
        m_tileDisplay->setText("Failed to extract tiles");
        return;
    }

    // Convert to RGB for display (indexed images don't display well in Qt sometimes)
    QImage rgbImage = tileImage.convertToFormat(QImage::Format_RGB888);

    // Scale up for better visibility (2x)
    m_currentImage = QPixmap::fromImage(rgbImage.scaled(imageWidth * 2, imageHeight * 2, Qt::KeepAspectRatio, Qt::FastTransformation));
    m_tileDisplay->setPixmap(m_currentImage);
    m_tileDisplay->adjustSize();
}

void TileViewer::onPrevPage()
{
    // Calculate bytes per page based on bpp mode
    // 4bpp: 32 bytes per 8x8 tile, 2bpp: 16 bytes per 8x8 tile
    int bytesPerTile = (m_bpp == 4) ? 32 : 16;
    int tileSize = (m_tileWidth / 8) * (m_tileHeight / 8) * bytesPerTile;
    int bytesPerPage = m_tilesPerPage * tileSize;

    if (m_currentOffset >= static_cast<uint32_t>(bytesPerPage)) {
        m_currentOffset -= bytesPerPage;
    } else {
        m_currentOffset = 0;
    }

    m_offsetInput->setText(QString::number(m_currentOffset, 16).toUpper());
    updateDisplay();
}

void TileViewer::onNextPage()
{
    // Calculate bytes per page based on bpp mode
    // 4bpp: 32 bytes per 8x8 tile, 2bpp: 16 bytes per 8x8 tile
    int bytesPerTile = (m_bpp == 4) ? 32 : 16;
    int tileSize = (m_tileWidth / 8) * (m_tileHeight / 8) * bytesPerTile;
    int bytesPerPage = m_tilesPerPage * tileSize;

    m_currentOffset += bytesPerPage;

    // Don't go past ROM size
    if (m_currentOffset >= static_cast<uint32_t>(m_romReader->romSize())) {
        m_currentOffset = m_romReader->romSize() - bytesPerPage;
    }

    m_offsetInput->setText(QString::number(m_currentOffset, 16).toUpper());
    updateDisplay();
}

void TileViewer::onJumpToOffset()
{
    bool ok;
    uint32_t newOffset = m_offsetInput->text().toUInt(&ok, 16);

    if (ok && newOffset < static_cast<uint32_t>(m_romReader->romSize())) {
        m_currentOffset = newOffset;
        updateDisplay();
    } else {
        QMessageBox::warning(this, "Invalid Offset",
                             QString("Invalid hex offset: %1").arg(m_offsetInput->text()));
    }
}

void TileViewer::onPaletteOffsetChanged()
{
    bool ok;
    uint32_t newPalette = m_paletteInput->text().toUInt(&ok, 16);

    if (ok && newPalette < static_cast<uint32_t>(m_romReader->romSize())) {
        m_paletteOffset = newPalette;
        updateDisplay();
    } else {
        QMessageBox::warning(this, "Invalid Offset",
                             QString("Invalid hex palette offset: %1").arg(m_paletteInput->text()));
    }
}

void TileViewer::onSaveImage()
{
    if (m_currentImage.isNull()) {
        QMessageBox::warning(this, "No Image", "No tiles to save");
        return;
    }

    QString defaultName = QString("tiles_0x%1.png").arg(m_currentOffset, 8, 16, QChar('0'));
    QString fileName = QFileDialog::getSaveFileName(this, "Save Tile Image",
                                                     defaultName,
                                                     "PNG Images (*.png)");

    if (!fileName.isEmpty()) {
        if (m_currentImage.save(fileName)) {
            QMessageBox::information(this, "Saved", QString("Tiles saved to %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Save Failed", "Failed to save image");
        }
    }
}
