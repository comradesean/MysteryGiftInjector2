#include "wondercardrenderer.h"
#include <QPainter>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QDebug>

// Static member initialization
GBAROReader *WonderCardRenderer::s_romReader = nullptr;
QMap<uint16_t, QPixmap> WonderCardRenderer::s_iconCache;

// Background colors for the 8 Wonder Card types (from Mystic Ticket screenshot)
// Color index is derived from typeColorResend byte: (byte >> 2) & 0x07
const QColor WonderCardRenderer::BACKGROUND_COLORS[8] = {
    QColor(183, 175, 207),  // 0: Purple (Mystic Ticket - measured from screenshot)
    QColor(207, 191, 175),  // 1: Beige/Tan
    QColor(175, 207, 191),  // 2: Green
    QColor(183, 191, 207),  // 3: Blue
    QColor(207, 175, 175),  // 4: Red/Pink
    QColor(207, 207, 175),  // 5: Yellow
    QColor(191, 191, 191),  // 6: Gray
    QColor(175, 191, 207),  // 7: Light Blue
};

const QColor WonderCardRenderer::TITLE_BG_COLORS[8] = {
    QColor(143, 135, 167),  // 0: Dark Purple (Mystic Ticket title area)
    QColor(167, 151, 135),  // 1: Dark Beige
    QColor(135, 167, 151),  // 2: Dark Green
    QColor(143, 151, 167),  // 3: Dark Blue
    QColor(167, 135, 135),  // 4: Dark Red
    QColor(167, 167, 135),  // 5: Dark Yellow
    QColor(151, 151, 151),  // 6: Dark Gray
    QColor(135, 151, 167),  // 7: Dark Light Blue
};

WonderCardRenderer::WonderCardRenderer(QWidget *parent)
    : QWidget(parent),
      m_hasData(false)
{
    setFixedSize(CARD_WIDTH * 2, CARD_HEIGHT * 2);  // Always 2x scale
}

WonderCardRenderer::~WonderCardRenderer()
{
}

void WonderCardRenderer::setWonderCard(const WonderCardData &wonderCard)
{
    m_wonderCard = wonderCard;
    m_hasData = !wonderCard.isEmpty();

    // Extract icon from ROM if available
    if (m_hasData && isROMLoaded()) {
        // Check cache first
        if (s_iconCache.contains(wonderCard.icon)) {
            m_cachedIcon = s_iconCache[wonderCard.icon];
        } else {
            // Extract from ROM
            QImage iconImg = s_romReader->extractPokemonIcon(wonderCard.icon);
            if (!iconImg.isNull()) {
                m_cachedIcon = QPixmap::fromImage(iconImg);
                s_iconCache[wonderCard.icon] = m_cachedIcon;  // Cache it
                qDebug() << "Extracted Pokemon icon" << wonderCard.icon << "from ROM";
            } else {
                m_cachedIcon = QPixmap();  // Clear cache
                qDebug() << "Failed to extract Pokemon icon" << wonderCard.icon;
            }
        }
    } else {
        m_cachedIcon = QPixmap();  // Clear cache
    }

    update();  // Trigger repaint
}

void WonderCardRenderer::clear()
{
    m_hasData = false;
    m_cachedIcon = QPixmap();
    update();
}

bool WonderCardRenderer::loadROM(const QString &romPath, QString &errorMessage)
{
    // Clean up existing ROM reader if any
    if (s_romReader) {
        delete s_romReader;
        s_romReader = nullptr;
        s_iconCache.clear();
    }

    // Create new ROM reader
    s_romReader = new GBAROReader();
    if (!s_romReader->loadROM(romPath, errorMessage)) {
        delete s_romReader;
        s_romReader = nullptr;
        return false;
    }

    qDebug() << "ROM loaded successfully for Wonder Card rendering";
    qDebug() << "Game:" << s_romReader->gameTitle();
    qDebug() << "Code:" << s_romReader->gameCode();

    return true;
}

QSize WonderCardRenderer::sizeHint() const
{
    return QSize(CARD_WIDTH * 2, CARD_HEIGHT * 2);  // Always 2x
}

QSize WonderCardRenderer::minimumSizeHint() const
{
    return QSize(CARD_WIDTH * 2, CARD_HEIGHT * 2);  // Always 2x
}

void WonderCardRenderer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);  // Pixel-perfect rendering

    // Scale to 2x (GBA screen 240x160 → 480x320)
    painter.scale(2, 2);

    if (!m_hasData) {
        // Draw placeholder
        painter.fillRect(0, 0, CARD_WIDTH, CARD_HEIGHT, QColor(220, 220, 220));
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(QRect(0, 0, CARD_WIDTH, CARD_HEIGHT),
                         Qt::AlignCenter, "No Wonder Card");
        return;
    }

    // Draw components in order
    drawBackground(painter);
    drawBorder(painter);
    drawTitleArea(painter);
    drawContentArea(painter);
    drawWarningArea(painter);
    drawIcon(painter);
}

void WonderCardRenderer::drawBackground(QPainter &painter)
{
    QColor bgColor = getBackgroundColor();
    painter.fillRect(0, 0, CARD_WIDTH, CARD_HEIGHT, bgColor);
}

void WonderCardRenderer::drawBorder(QPainter &painter)
{
    // Outer border (dark frame)
    painter.setPen(QPen(QColor(60, 60, 60), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(2, 2, CARD_WIDTH - 4, CARD_HEIGHT - 4);

    // Inner border
    painter.setPen(QPen(QColor(40, 40, 40), 1));
    painter.drawRect(BORDER_WIDTH, BORDER_WIDTH,
                     CARD_WIDTH - BORDER_WIDTH * 2,
                     CARD_HEIGHT - BORDER_WIDTH * 2);
}

void WonderCardRenderer::drawTitleArea(QPainter &painter)
{
    // Title background (darker shade)
    QColor titleBg = getTitleBackgroundColor();
    painter.fillRect(BORDER_WIDTH + 1, BORDER_WIDTH + 1,
                     CARD_WIDTH - BORDER_WIDTH * 2 - 2, TITLE_AREA_HEIGHT,
                     titleBg);

    // Separator line below title area
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.drawLine(BORDER_WIDTH + 1, TITLE_AREA_HEIGHT + BORDER_WIDTH,
                     CARD_WIDTH - BORDER_WIDTH - 1, TITLE_AREA_HEIGHT + BORDER_WIDTH);

    // Title text (with decorative stars)
    QFont titleFont = getTitleFont();
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 255, 255));  // White text

    QString titleText = QString("*%1*").arg(m_wonderCard.title);
    painter.drawText(QRect(BORDER_WIDTH, TITLE_Y, CARD_WIDTH - BORDER_WIDTH * 2, 16),
                     Qt::AlignCenter, titleText);

    // Subtitle text (slightly smaller than title)
    QFont subtitleFont("Courier New", 9);
    painter.setFont(subtitleFont);
    painter.drawText(QRect(BORDER_WIDTH, SUBTITLE_Y, CARD_WIDTH - BORDER_WIDTH * 2 - ICON_SIZE - 4, 12),
                     Qt::AlignCenter, m_wonderCard.subtitle);
}

void WonderCardRenderer::drawContentArea(QPainter &painter)
{
    // Content background (light/white)
    painter.fillRect(BORDER_WIDTH + 1, CONTENT_AREA_Y,
                     CARD_WIDTH - BORDER_WIDTH * 2 - 2, WARNING_AREA_Y - CONTENT_AREA_Y - 2,
                     QColor(240, 240, 245));

    // Content text (dark gray)
    QFont contentFont = getContentFont();
    painter.setFont(contentFont);
    painter.setPen(QColor(50, 50, 50));

    QStringList contentLines;
    if (!m_wonderCard.contentLine1.isEmpty()) contentLines << m_wonderCard.contentLine1;
    if (!m_wonderCard.contentLine2.isEmpty()) contentLines << m_wonderCard.contentLine2;
    if (!m_wonderCard.contentLine3.isEmpty()) contentLines << m_wonderCard.contentLine3;
    if (!m_wonderCard.contentLine4.isEmpty()) contentLines << m_wonderCard.contentLine4;

    int y = CONTENT_AREA_Y + 4;
    for (const QString &line : contentLines) {
        painter.drawText(CONTENT_MARGIN, y, line);
        y += CONTENT_LINE_HEIGHT;
    }
}

void WonderCardRenderer::drawWarningArea(QPainter &painter)
{
    // Warning background (light/white)
    painter.fillRect(BORDER_WIDTH + 1, WARNING_AREA_Y,
                     CARD_WIDTH - BORDER_WIDTH * 2 - 2, CARD_HEIGHT - WARNING_AREA_Y - BORDER_WIDTH - 1,
                     QColor(240, 240, 245));

    // Separator line above warning area
    painter.setPen(QPen(QColor(180, 180, 180), 1));
    painter.drawLine(BORDER_WIDTH + 1, WARNING_AREA_Y,
                     CARD_WIDTH - BORDER_WIDTH - 1, WARNING_AREA_Y);

    // Warning text (dark gray)
    QFont warningFont = getContentFont();
    painter.setFont(warningFont);
    painter.setPen(QColor(50, 50, 50));

    int y = WARNING_AREA_Y + 4;
    if (!m_wonderCard.warningLine1.isEmpty()) {
        painter.drawText(CONTENT_MARGIN, y, m_wonderCard.warningLine1);
        y += CONTENT_LINE_HEIGHT;
    }
    if (!m_wonderCard.warningLine2.isEmpty()) {
        painter.drawText(CONTENT_MARGIN, y, m_wonderCard.warningLine2);
    }
}

void WonderCardRenderer::drawIcon(QPainter &painter)
{
    int iconX = CARD_WIDTH - ICON_SIZE - 8;  // 8px margin from right edge
    int iconY = 8;  // 8px from top

    // Draw extracted icon if available
    if (!m_cachedIcon.isNull()) {
        painter.drawPixmap(iconX, iconY, ICON_SIZE, ICON_SIZE, m_cachedIcon);
    } else {
        // Draw placeholder
        painter.setPen(QPen(QColor(100, 100, 100), 1));
        painter.setBrush(QColor(200, 200, 200));
        painter.drawRect(iconX, iconY, ICON_SIZE, ICON_SIZE);

        // Draw icon number or question mark
        painter.setPen(QColor(255, 255, 255));  // White text
        QFont iconFont("Arial", 12, QFont::Bold);
        painter.setFont(iconFont);

        QString iconText = (m_wonderCard.icon == WonderCardIcon::QUESTION_MARK) ? "?" :
                           QString::number(m_wonderCard.icon);
        painter.drawText(QRect(iconX, iconY, ICON_SIZE, ICON_SIZE),
                         Qt::AlignCenter, iconText);
    }
}

QColor WonderCardRenderer::getBackgroundColor() const
{
    uint8_t colorIndex = m_wonderCard.color();
    return BACKGROUND_COLORS[colorIndex % 8];
}

QColor WonderCardRenderer::getTitleBackgroundColor() const
{
    uint8_t colorIndex = m_wonderCard.color();
    return TITLE_BG_COLORS[colorIndex % 8];
}

QFont WonderCardRenderer::getTitleFont() const
{
    // Use a monospace font that resembles the GBA font
    // GBA fonts are larger than expected due to pixel art scaling
    QFont font("Courier New", 11, QFont::Bold);
    return font;
}

QFont WonderCardRenderer::getContentFont() const
{
    // Content font should be reasonably sized for readability
    QFont font("Courier New", 9);
    return font;
}

QString WonderCardRenderer::wrapText(const QString &text, int maxWidth, const QFont &font)
{
    QFontMetrics fm(font);
    QStringList words = text.split(' ');
    QString result;
    QString currentLine;

    for (const QString &word : words) {
        QString testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
        if (fm.horizontalAdvance(testLine) <= maxWidth) {
            currentLine = testLine;
        } else {
            if (!result.isEmpty()) result += "\n";
            result += currentLine;
            currentLine = word;
        }
    }

    if (!currentLine.isEmpty()) {
        if (!result.isEmpty()) result += "\n";
        result += currentLine;
    }

    return result;
}
