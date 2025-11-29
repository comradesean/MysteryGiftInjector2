#ifndef WONDERCARDRENDERER_H
#define WONDERCARDRENDERER_H

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QMap>
#include "mysterygift.h"
#include "gbaromreader.h"

/**
 * @brief Custom widget that renders a Wonder Card visually as it appears in-game
 *
 * This widget replicates the visual appearance of Wonder Cards from Pokemon FireRed,
 * LeafGreen, and Emerald, including:
 * - Background colors/themes (8 variations based on color byte)
 * - Decorative borders and frames
 * - Title, subtitle, content, and warning text with proper positioning
 * - Pokemon icon display
 *
 * The widget is designed to match GBA screen dimensions (240x160) but can be scaled.
 */
class WonderCardRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit WonderCardRenderer(QWidget *parent = nullptr);
    ~WonderCardRenderer();

    // Set the Wonder Card data to display
    void setWonderCard(const WonderCardData &wonderCard);

    // Clear the display
    void clear();

    // Load ROM for graphics extraction (static - shared across all instances)
    static bool loadROM(const QString &romPath, QString &errorMessage);
    static bool isROMLoaded() { return s_romReader != nullptr && s_romReader->isLoaded(); }

    // Get the recommended size (GBA resolution: 240x160, displayed at 2x = 480x320)
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Rendering methods
    void drawBackground(QPainter &painter);
    void drawBorder(QPainter &painter);
    void drawTitleArea(QPainter &painter);
    void drawContentArea(QPainter &painter);
    void drawWarningArea(QPainter &painter);
    void drawIcon(QPainter &painter);

    // Helper methods
    void updateSize();
    QColor getBackgroundColor() const;
    QColor getTitleBackgroundColor() const;
    QFont getTitleFont() const;
    QFont getContentFont() const;
    QString wrapText(const QString &text, int maxWidth, const QFont &font);

    // Data
    WonderCardData m_wonderCard;
    bool m_hasData;
    QPixmap m_cachedIcon;  // Cached extracted icon

    // Static ROM reader (shared across all instances)
    static GBAROReader *s_romReader;
    static QMap<uint16_t, QPixmap> s_iconCache;  // Cache extracted icons

    // Layout constants (based on GBA screen: 240x160)
    static const int CARD_WIDTH = 240;
    static const int CARD_HEIGHT = 160;
    static const int BORDER_WIDTH = 8;
    static const int TITLE_AREA_HEIGHT = 50;
    static const int TITLE_Y = 16;
    static const int SUBTITLE_Y = 36;
    static const int CONTENT_AREA_Y = 52;
    static const int CONTENT_LINE_HEIGHT = 16;
    static const int CONTENT_MARGIN = 12;
    static const int WARNING_AREA_Y = 124;
    static const int ICON_X = 216;
    static const int ICON_Y = 16;
    static const int ICON_SIZE = 32;

    // Background colors for each Wonder Card type (0-7)
    static const QColor BACKGROUND_COLORS[8];
    static const QColor TITLE_BG_COLORS[8];
};

#endif // WONDERCARDRENDERER_H
