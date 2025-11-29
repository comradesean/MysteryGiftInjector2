#ifndef TILEVIEWER_H
#define TILEVIEWER_H

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollArea>
#include "gbaromreader.h"

/**
 * @brief Dialog for viewing and scanning GBA ROM graphics
 *
 * Allows browsing through ROM tiles at different offsets to find
 * graphics data. Useful for locating Wonder Card backgrounds, borders,
 * fonts, and other UI elements.
 */
class TileViewer : public QDialog
{
    Q_OBJECT

public:
    explicit TileViewer(GBAROReader *romReader, QWidget *parent = nullptr);
    ~TileViewer();

private slots:
    void updateDisplay();
    void onPrevPage();
    void onNextPage();
    void onJumpToOffset();
    void onSaveImage();
    void onPaletteOffsetChanged();

private:
    void setupUI();
    void renderTiles();

    // ROM reader
    GBAROReader *m_romReader;

    // Current view settings
    uint32_t m_currentOffset;
    uint32_t m_paletteOffset;
    int m_tilesPerRow;
    int m_tilesPerPage;
    int m_tileWidth;   // 8, 16, or 32
    int m_tileHeight;  // 8, 16, or 32
    int m_bpp;         // 2 or 4 bits per pixel

    // UI Elements
    QLabel *m_tileDisplay;
    QPixmap m_currentImage;
    QScrollArea *m_scrollArea;

    // Controls
    QLineEdit *m_offsetInput;
    QLineEdit *m_paletteInput;
    QSpinBox *m_tilesPerRowSpin;
    QSpinBox *m_tilesPerPageSpin;
    QComboBox *m_tileSizeCombo;
    QComboBox *m_bppCombo;      // 2bpp or 4bpp mode selector
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QPushButton *m_jumpButton;
    QPushButton *m_saveButton;
    QLabel *m_infoLabel;
};

#endif // TILEVIEWER_H
