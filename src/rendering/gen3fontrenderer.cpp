#include "gen3fontrenderer.h"
#include <QDebug>
#include <QPainter>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

// Text color tables from mystery_gift_show_card.c sTextColorTable
// [background_idx, foreground_idx, shadow_idx]
const int Gen3FontRenderer::TEXT_COLOR_TABLE_0[3] = {0, 2, 3};  // Body/footer
const int Gen3FontRenderer::TEXT_COLOR_TABLE_1[3] = {0, 1, 2};  // Title/header

Gen3FontRenderer::Gen3FontRenderer()
    : m_loaded(false)
    , m_isEmerald(false)
    , m_romReader(nullptr)
    , m_fontType(FontNormalCopy2)
{
    // Load default font mapping (FRLG)
    loadCharacterMappingFromJson(FontNormalCopy2);
}

Gen3FontRenderer::~Gen3FontRenderer()
{
}

bool Gen3FontRenderer::loadFromROM(GBAROReader *reader, QString &errorMessage)
{
    if (!reader || !reader->isLoaded()) {
        errorMessage = "ROM not loaded";
        return false;
    }

    m_romReader = reader;
    m_isEmerald = reader->isEmerald();

    // Load appropriate character mapping based on ROM type
    FontType fontType = m_isEmerald ? FontShortCopy1 : FontNormalCopy2;
    if (!loadCharacterMappingFromJson(fontType)) {
        qWarning() << "Failed to load character mapping from JSON, using defaults";
    }

    // Extract main font sheet (2bpp)
    // For FRLG: FONT_NORMAL_COPY_2 (index 3)
    // For Emerald: FONT_SHORT_COPY_1 (index 3) - used for title, subtitle, body, footer
    m_fontSheet = reader->extractFont();
    if (m_fontSheet.isNull()) {
        errorMessage = "Failed to extract font from ROM";
        return false;
    }

    qDebug() << "Main font sheet extracted:" << m_fontSheet.width() << "x" << m_fontSheet.height();

    // Extract glyph widths for main font
    m_glyphWidths = reader->getDefaultGlyphWidths();
    qDebug() << "Loaded" << m_glyphWidths.size() << "glyph widths for main font";

    // For Emerald, also load FONT_NORMAL (index 1) for ID number
    if (m_isEmerald) {
        m_idFontSheet = reader->extractFontByIndex(1);  // FONT_NORMAL
        if (!m_idFontSheet.isNull()) {
            qDebug() << "Emerald ID font sheet extracted:" << m_idFontSheet.width() << "x" << m_idFontSheet.height();
            m_idGlyphWidths = reader->getGlyphWidthsByIndex(1);
            qDebug() << "Loaded" << m_idGlyphWidths.size() << "glyph widths for ID font";
        } else {
            qWarning() << "Failed to extract Emerald ID font (FONT_NORMAL), will use main font";
        }
    }

    // Extract text palette (stdpal_3)
    // Use stdpal_3 (index 3) for Wonder Card text
    uint32_t stdpal3Offset = reader->getStdPalOffset(3);
    if (stdpal3Offset == 0) {
        errorMessage = "Could not get text palette offset from ROM";
        return false;
    }
    m_textPalette = reader->extractPalette(stdpal3Offset, 16);
    qDebug() << "Loaded text palette with" << m_textPalette.size() << "colors";

    m_loaded = true;
    return true;
}

bool Gen3FontRenderer::loadCharacterMappingFromJson(FontType fontType)
{
    QString resourcePath;
    switch (fontType) {
        case FontNormalCopy2:
            resourcePath = ":/Resources/font_normal_copy_2_latin.json";
            break;
        case FontShortCopy1:
            resourcePath = ":/Resources/font_short_copy_1_latin.json";
            break;
        default:
            resourcePath = ":/Resources/font_normal_copy_2_latin.json";
            break;
    }

    m_fontType = fontType;
    return loadCharacterMappingFromJson(resourcePath);
}

bool Gen3FontRenderer::loadCharacterMappingFromJson(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open font mapping file:" << resourcePath;
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();

    // Clear existing mappings
    m_charToPos.clear();

    // Load from "ascii_to_position" section (character string -> font position)
    QJsonObject asciiToPos = root["ascii_to_position"].toObject();
    for (auto it = asciiToPos.begin(); it != asciiToPos.end(); ++it) {
        QString charStr = it.key();
        int position = it.value().toInt();

        // Handle single characters
        if (charStr.length() == 1) {
            m_charToPos[charStr[0]] = position;
        }
        // Skip multi-character ligatures (PK, MN, etc.) for now
        // They would need special handling in text rendering
    }

    qDebug() << "Loaded" << m_charToPos.size() << "character mappings from" << resourcePath;
    return true;
}

int Gen3FontRenderer::getCharPosition(QChar ch) const
{
    return m_charToPos.value(ch, -1);
}

bool Gen3FontRenderer::canEncodeChar(QChar ch) const
{
    return m_charToPos.contains(ch);
}

int Gen3FontRenderer::getEncodedLength(const QString &text) const
{
    // Each character is 1 byte in Gen3 encoding
    // (simplified - doesn't account for control codes)
    int count = 0;
    for (const QChar &ch : text) {
        if (canEncodeChar(ch)) {
            count++;
        }
    }
    return count;
}

int Gen3FontRenderer::getCharWidth(QChar ch) const
{
    int pos = getCharPosition(ch);
    if (pos < 0) {
        return 6;  // Default width for unknown characters
    }

    // Width index = font_position / 2 (since font positions are double-spaced)
    int widthIndex = pos / 2;

    if (widthIndex >= 0 && widthIndex < m_glyphWidths.size()) {
        return m_glyphWidths[widthIndex];
    }

    return 6;  // Default fallback
}

int Gen3FontRenderer::getIdCharWidth(QChar ch) const
{
    // For Emerald: use ID font glyph widths
    // For FRLG: falls back to main font
    if (m_idGlyphWidths.isEmpty()) {
        return getCharWidth(ch);
    }

    int pos = getCharPosition(ch);
    if (pos < 0) {
        return 6;
    }

    int widthIndex = pos / 2;
    if (widthIndex >= 0 && widthIndex < m_idGlyphWidths.size()) {
        return m_idGlyphWidths[widthIndex];
    }

    return 6;
}

QSize Gen3FontRenderer::measureText(const QString &text, int charSpacing) const
{
    if (text.isEmpty()) {
        return QSize(0, RENDER_HEIGHT);
    }

    int width = 0;
    for (int i = 0; i < text.length(); ++i) {
        width += getCharWidth(text[i]);
        if (i < text.length() - 1) {
            width += charSpacing;
        }
    }

    return QSize(width, RENDER_HEIGHT);
}

QImage Gen3FontRenderer::applyPaletteToFont(const int colorTable[3])
{
    if (m_fontSheet.isNull() || m_textPalette.isEmpty()) {
        return QImage();
    }

    // Create a copy of the font sheet
    QImage result(m_fontSheet.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    // Font uses indices 0-2 for 2bpp:
    // 0 = Background (transparent)
    // 1 = Main glyph (foreground)
    // 2 = Shadow/highlight

    // colorTable maps: [background, foreground, shadow]
    // colorTable[0] -> palette index for background
    // colorTable[1] -> palette index for foreground
    // colorTable[2] -> palette index for shadow

    for (int y = 0; y < m_fontSheet.height(); ++y) {
        for (int x = 0; x < m_fontSheet.width(); ++x) {
            int fontIdx = m_fontSheet.pixelIndex(x, y);

            if (fontIdx == 0) {
                // Background - transparent
                result.setPixel(x, y, qRgba(0, 0, 0, 0));
            } else if (fontIdx == 1) {
                // Foreground
                int paletteIdx = colorTable[1];
                if (paletteIdx >= 0 && paletteIdx < m_textPalette.size()) {
                    result.setPixel(x, y, m_textPalette[paletteIdx]);
                }
            } else if (fontIdx == 2) {
                // Shadow
                int paletteIdx = colorTable[2];
                if (paletteIdx >= 0 && paletteIdx < m_textPalette.size()) {
                    result.setPixel(x, y, m_textPalette[paletteIdx]);
                }
            }
        }
    }

    return result;
}

QImage Gen3FontRenderer::createColoredFont(ColorScheme scheme)
{
    if (scheme == TitleHeader) {
        return applyPaletteToFont(TEXT_COLOR_TABLE_1);
    } else {
        return applyPaletteToFont(TEXT_COLOR_TABLE_0);
    }
}

QImage Gen3FontRenderer::createColoredIdFont(ColorScheme scheme)
{
    // For Emerald: use the ID font (FONT_NORMAL) for ID number rendering
    // For FRLG: falls back to main font
    if (m_idFontSheet.isNull()) {
        return createColoredFont(scheme);
    }

    // Apply palette to ID font sheet
    const int *colorTable = (scheme == TitleHeader) ? TEXT_COLOR_TABLE_1 : TEXT_COLOR_TABLE_0;

    QImage result(m_idFontSheet.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    for (int y = 0; y < m_idFontSheet.height(); ++y) {
        for (int x = 0; x < m_idFontSheet.width(); ++x) {
            int fontIdx = m_idFontSheet.pixelIndex(x, y);

            if (fontIdx == 0) {
                result.setPixel(x, y, qRgba(0, 0, 0, 0));
            } else if (fontIdx == 1) {
                int paletteIdx = colorTable[1];
                if (paletteIdx >= 0 && paletteIdx < m_textPalette.size()) {
                    result.setPixel(x, y, m_textPalette[paletteIdx]);
                }
            } else if (fontIdx == 2) {
                int paletteIdx = colorTable[2];
                if (paletteIdx >= 0 && paletteIdx < m_textPalette.size()) {
                    result.setPixel(x, y, m_textPalette[paletteIdx]);
                }
            }
        }
    }

    return result;
}

QImage Gen3FontRenderer::getCharacter(QChar ch, const QImage &coloredFont) const
{
    int pos = getCharPosition(ch);
    if (pos < 0) {
        // Return empty transparent image for unknown characters
        return QImage(CHAR_WIDTH, RENDER_HEIGHT, QImage::Format_ARGB32);
    }

    // Calculate position in font sheet
    int col = pos % CHARS_PER_ROW;
    int row = pos / CHARS_PER_ROW;

    int x = col * CHAR_WIDTH;
    int y = row * CHAR_HEIGHT;

    // Extract character and crop to render height
    return coloredFont.copy(x, y, CHAR_WIDTH, RENDER_HEIGHT);
}

QImage Gen3FontRenderer::renderLine(const QString &text, const QImage &coloredFont, int charSpacing)
{
    QSize size = measureText(text, charSpacing);
    if (size.width() == 0) {
        return QImage(0, RENDER_HEIGHT, QImage::Format_ARGB32);
    }

    QImage result(size.width(), RENDER_HEIGHT, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    int xOffset = 0;
    for (const QChar &ch : text) {
        QImage charImg = getCharacter(ch, coloredFont);
        painter.drawImage(xOffset, 0, charImg);
        xOffset += getCharWidth(ch) + charSpacing;
    }

    painter.end();
    return result;
}

QImage Gen3FontRenderer::renderTextArea(const QStringList &lines, const QImage &coloredFont,
                                         int width, int height,
                                         Qt::Alignment alignment,
                                         int lineSpacing, int charSpacing,
                                         int paddingLeft, int paddingTop)
{
    QImage result(width, height, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter painter(&result);

    int yOffset = paddingTop;

    for (const QString &line : lines) {
        if (line.isEmpty()) {
            yOffset += RENDER_HEIGHT + lineSpacing;
            continue;
        }

        QImage lineImg = renderLine(line, coloredFont, charSpacing);

        // Calculate X position based on alignment
        int xOffset;
        if (alignment & Qt::AlignHCenter) {
            xOffset = paddingLeft + (width - paddingLeft - lineImg.width()) / 2;
        } else if (alignment & Qt::AlignRight) {
            xOffset = width - lineImg.width();
        } else {
            xOffset = paddingLeft;
        }

        // Check if line fits
        if (yOffset + lineImg.height() > height) {
            break;
        }

        painter.drawImage(xOffset, yOffset, lineImg);
        yOffset += RENDER_HEIGHT + lineSpacing;
    }

    painter.end();
    return result;
}

void Gen3FontRenderer::setFallbackGlyphWidths(const QByteArray &widths)
{
    m_glyphWidths.clear();
    m_glyphWidths.reserve(widths.size());

    for (int i = 0; i < widths.size(); ++i) {
        m_glyphWidths.append(static_cast<uint8_t>(widths[i]));
    }

    qDebug() << "Set" << m_glyphWidths.size() << "fallback glyph widths";
}
