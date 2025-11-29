#include "authenticwondercardwidget.h"
#include "fallbackgraphics.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

AuthenticWonderCardWidget::AuthenticWonderCardWidget(QWidget *parent)
    : QWidget(parent)
    , m_romReader(nullptr)
    , m_fontRenderer(nullptr)
    , m_romLoaded(false)
    , m_fallbackMode(false)
    , m_bgIndex(0)
    , m_iconSpecies(0)
    , m_hasData(false)
    , m_readOnly(false)
    , m_cursorPos(0)
    , m_cursorVisible(true)
{
    setFixedSize(CARD_WIDTH * DISPLAY_SCALE, CARD_HEIGHT * DISPLAY_SCALE);
    setFocusPolicy(Qt::StrongFocus);

    // Initialize text fields
    initTextFields();

    // Set up cursor blink timer
    m_cursorTimer = new QTimer(this);
    connect(m_cursorTimer, &QTimer::timeout, this, &AuthenticWonderCardWidget::toggleCursor);
    m_cursorTimer->start(500);

    // Create ROM reader and font renderer
    m_romReader = new GBAROReader();
    m_fontRenderer = new Gen3FontRenderer();
}

AuthenticWonderCardWidget::~AuthenticWonderCardWidget()
{
    delete m_fontRenderer;
    delete m_romReader;
}

void AuthenticWonderCardWidget::initTextFields()
{
    // Text field definitions: name, y_start, byte_limit, isHeader, isIdField
    // Y positions from Python wonder_card_editor.py TEXT_FIELDS
    // isIdField: true for ID number (Emerald uses FONT_NORMAL for this)
    m_textFields.clear();
    m_textFields.append({"title", 9, 40, true, false});
    m_textFields.append({"subtitle", 25, 40, true, false});
    m_textFields.append({"contents_line1", 50, 40, false, false});
    m_textFields.append({"contents_line2", 66, 40, false, false});
    m_textFields.append({"contents_line3", 82, 40, false, false});
    m_textFields.append({"contents_line4", 98, 40, false, false});
    m_textFields.append({"warning_line1", 119, 40, false, false});
    m_textFields.append({"warning_line2", 135, 40, false, false});

    // Initialize empty text for each field
    for (const TextField &field : m_textFields) {
        m_fieldTexts[field.name] = QString();
    }
}

bool AuthenticWonderCardWidget::loadROM(const QString &romPath, QString &errorMessage)
{
    // Load ROM
    if (!m_romReader->loadROM(romPath, errorMessage)) {
        return false;
    }

    // Load font from ROM
    if (!m_fontRenderer->loadFromROM(m_romReader, errorMessage)) {
        return false;
    }

    // Create colored fonts (main font)
    m_fontHeader = m_fontRenderer->createColoredFont(Gen3FontRenderer::TitleHeader);
    m_fontBody = m_fontRenderer->createColoredFont(Gen3FontRenderer::BodyFooter);

    if (m_fontHeader.isNull() || m_fontBody.isNull()) {
        errorMessage = "Failed to create colored fonts";
        return false;
    }

    // For Emerald: also create ID fonts (FONT_NORMAL for ID number display)
    if (m_fontRenderer->isEmerald()) {
        m_fontIdHeader = m_fontRenderer->createColoredIdFont(Gen3FontRenderer::TitleHeader);
        m_fontIdBody = m_fontRenderer->createColoredIdFont(Gen3FontRenderer::BodyFooter);
        qDebug() << "Emerald ID fonts created";
    }

    // Pre-load all 8 Wonder Card backgrounds
    m_backgrounds.clear();
    m_backgrounds.resize(8);
    for (int i = 0; i < 8; ++i) {
        m_backgrounds[i] = m_romReader->extractWonderCardBackground(i);
        if (m_backgrounds[i].isNull()) {
            qWarning() << "Failed to load Wonder Card background" << i;
        }
    }

    m_romLoaded = true;
    qDebug() << "ROM loaded successfully, fonts and backgrounds ready";

    // Re-render if we have data
    if (m_hasData) {
        renderCard();
        update();
    }

    return true;
}

bool AuthenticWonderCardWidget::loadFallbackGraphics(QString &errorMessage)
{
    qDebug() << "Loading fallback graphics...";

    m_fallbackMode = true;

    // Generate placeholder font
    QImage placeholderFont = FallbackGraphics::generatePlaceholderFont();
    if (placeholderFont.isNull()) {
        errorMessage = "Failed to generate fallback font";
        return false;
    }

    // Use the same font for both header and body (just different colors could be applied later)
    m_fontHeader = placeholderFont;
    m_fontBody = placeholderFont;

    // Generate placeholder backgrounds for all 8 slots
    m_backgrounds.clear();
    m_backgrounds.resize(8);
    for (int i = 0; i < 8; ++i) {
        m_backgrounds[i] = FallbackGraphics::generatePlaceholderBackground(i);
    }

    // Generate placeholder icon
    m_iconImage = FallbackGraphics::generatePlaceholderPokemonIcon();

    // Load fallback glyph widths into font renderer if available
    if (m_fontRenderer) {
        QByteArray defaultWidths = FallbackGraphics::generateDefaultGlyphWidths();
        m_fontRenderer->setFallbackGlyphWidths(defaultWidths);
    }

    m_romLoaded = false;  // ROM not loaded, but fallback is active
    qDebug() << "Fallback graphics loaded successfully";

    // Re-render if we have data
    if (m_hasData) {
        renderCard();
        update();
    }

    return true;
}

QSize AuthenticWonderCardWidget::sizeHint() const
{
    return QSize(CARD_WIDTH * DISPLAY_SCALE, CARD_HEIGHT * DISPLAY_SCALE);
}

QSize AuthenticWonderCardWidget::minimumSizeHint() const
{
    return QSize(CARD_WIDTH * DISPLAY_SCALE, CARD_HEIGHT * DISPLAY_SCALE);
}

void AuthenticWonderCardWidget::setWonderCard(const WonderCardData &wonderCard)
{
    m_wonderCard = wonderCard;

    // Extract text from Wonder Card data
    m_fieldTexts["title"] = wonderCard.title;
    m_fieldTexts["subtitle"] = wonderCard.subtitle;
    m_fieldTexts["contents_line1"] = wonderCard.contentLine1;
    m_fieldTexts["contents_line2"] = wonderCard.contentLine2;
    m_fieldTexts["contents_line3"] = wonderCard.contentLine3;
    m_fieldTexts["contents_line4"] = wonderCard.contentLine4;
    m_fieldTexts["warning_line1"] = wonderCard.warningLine1;
    m_fieldTexts["warning_line2"] = wonderCard.warningLine2;

    // Set background and icon (color is the background index)
    m_bgIndex = wonderCard.color();
    m_iconSpecies = wonderCard.icon;

    // Load pokemon icon
    loadPokemonIcon(m_iconSpecies);

    m_hasData = true;
    m_activeFieldName.clear();
    m_cursorPos = 0;

    renderCard();
    update();
}

WonderCardData AuthenticWonderCardWidget::buildWonderCardData() const
{
    WonderCardData wc = m_wonderCard;

    // Update text fields from current state
    wc.title = m_fieldTexts.value("title");
    wc.subtitle = m_fieldTexts.value("subtitle");
    wc.contentLine1 = m_fieldTexts.value("contents_line1");
    wc.contentLine2 = m_fieldTexts.value("contents_line2");
    wc.contentLine3 = m_fieldTexts.value("contents_line3");
    wc.contentLine4 = m_fieldTexts.value("contents_line4");
    wc.warningLine1 = m_fieldTexts.value("warning_line1");
    wc.warningLine2 = m_fieldTexts.value("warning_line2");

    return wc;
}

void AuthenticWonderCardWidget::clear()
{
    m_hasData = false;
    m_activeFieldName.clear();
    m_cursorPos = 0;

    for (const TextField &field : m_textFields) {
        m_fieldTexts[field.name] = QString();
    }

    m_renderedCard = QImage();
    update();
}

QString AuthenticWonderCardWidget::getFieldText(const QString &fieldName) const
{
    return m_fieldTexts.value(fieldName);
}

void AuthenticWonderCardWidget::setFieldText(const QString &fieldName, const QString &text)
{
    if (m_fieldTexts.contains(fieldName)) {
        m_fieldTexts[fieldName] = text;
        renderCard();
        update();
        emit wonderCardChanged(buildWonderCardData());
    }
}

void AuthenticWonderCardWidget::setBackgroundIndex(int index)
{
    if (index >= 0 && index < 8) {
        m_bgIndex = index;
        renderCard();
        update();
    }
}

void AuthenticWonderCardWidget::setIconSpecies(int species)
{
    m_iconSpecies = species;
    loadPokemonIcon(species);
    renderCard();
    update();
}

void AuthenticWonderCardWidget::loadPokemonIcon(int species)
{
    m_iconImage = QImage();

    // In fallback mode, use the placeholder icon with the species index
    if (m_fallbackMode) {
        m_iconImage = FallbackGraphics::generatePlaceholderPokemonIcon(species);
        return;
    }

    if (!m_romLoaded) {
        return;
    }

    // Only skip icon for raw 0x0000 - NOT for clamped values
    // The save file may contain 0xFFFF which gets clamped to 0,
    // but should still display icon index 0 (unlike raw 0x0000)
    if (species == 0) {
        return;
    }

    // Transform invalid species (from decomp)
    // FRLG: species > 412 → 0 (Bulbasaur icon)
    // Emerald: species > 412 → 260 (Question Mark icon)
    const int SPECIES_LIMIT = 412;
    const int EMERALD_INVALID_ICON = 260;  // SPECIES_OLD_UNOWN_J in Emerald
    const int FRLG_INVALID_ICON = 0;        // SPECIES_NONE in FRLG

    int displaySpecies;
    if (species > SPECIES_LIMIT) {
        displaySpecies = m_fontRenderer->isEmerald() ? EMERALD_INVALID_ICON : FRLG_INVALID_ICON;
    } else {
        displaySpecies = species;
    }

    // Load icon from ROM (displaySpecies 0 is valid - it's the "unknown" icon)
    // Icons are 32x64 (2 frames stacked), we use first frame
    QImage iconFull = m_romReader->extractPokemonIcon(displaySpecies);
    if (iconFull.isNull()) {
        qWarning() << "Failed to load icon for species" << displaySpecies;
        return;
    }

    // Extract first frame (top 32x32) and convert to ARGB32
    QImage iconFrame = iconFull.copy(0, 0, 32, 32);
    m_iconImage = iconFrame.convertToFormat(QImage::Format_ARGB32);

    // Make palette index 0 transparent
    // Since we're coming from indexed format, we need to check original
    if (iconFull.format() == QImage::Format_Indexed8) {
        QVector<QRgb> palette = iconFull.colorTable();
        for (int y = 0; y < m_iconImage.height(); ++y) {
            for (int x = 0; x < m_iconImage.width(); ++x) {
                int idx = iconFrame.pixelIndex(x, y);
                if (idx == 0) {
                    m_iconImage.setPixel(x, y, qRgba(0, 0, 0, 0));
                }
            }
        }
    }
}

void AuthenticWonderCardWidget::renderCard()
{
    // Allow rendering in both ROM mode and fallback mode
    if (!m_romLoaded && !m_fallbackMode) {
        m_renderedCard = QImage();
        return;
    }

    // Start with background
    if (m_bgIndex >= 0 && m_bgIndex < m_backgrounds.size() && !m_backgrounds[m_bgIndex].isNull()) {
        m_renderedCard = m_backgrounds[m_bgIndex].convertToFormat(QImage::Format_ARGB32);
    } else {
        m_renderedCard = QImage(CARD_WIDTH, CARD_HEIGHT, QImage::Format_ARGB32);
        m_renderedCard.fill(Qt::white);
    }

    QPainter painter(&m_renderedCard);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Render each text field
    for (const TextField &field : m_textFields) {
        QString text = m_fieldTexts.value(field.name);
        if (text.isEmpty()) {
            continue;
        }

        // Choose font based on field type
        // For Emerald ID fields: use ID font (FONT_NORMAL)
        // For everything else: use main font
        const QImage *fontImg;
        if (field.isIdField && m_fontRenderer->isEmerald() && !m_fontIdHeader.isNull()) {
            fontImg = field.isHeader ? &m_fontIdHeader : &m_fontIdBody;
        } else {
            fontImg = field.isHeader ? &m_fontHeader : &m_fontBody;
        }

        // Render text line
        QImage lineImg = m_fontRenderer->renderLine(text, *fontImg, 0);
        if (lineImg.width() > 0) {
            int xPos;
            if (field.name == "subtitle") {
                // Subtitle right-aligns to x=160 within header window
                // x = 160 - string_width; if (x < 0) x = 0;
                // Then add window offset (8) for screen position
                int windowX = 160 - lineImg.width();
                if (windowX < 0)
                    windowX = 0;
                xPos = PADDING_LEFT + windowX;
            } else {
                // All other fields use standard padding
                xPos = PADDING_LEFT;
            }
            painter.drawImage(xPos, field.yStart, lineImg);
        }
    }

    // Draw Pokemon icon
    if (!m_iconImage.isNull()) {
        int iconX = ICON_CENTER_X - (ICON_SIZE / 2);
        int iconY = ICON_CENTER_Y - (ICON_SIZE / 2);
        painter.drawImage(iconX, iconY, m_iconImage);
    }

    painter.end();
}

void AuthenticWonderCardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    if (m_renderedCard.isNull()) {
        // Draw placeholder
        painter.fillRect(rect(), QColor(200, 200, 200));
        painter.drawText(rect(), Qt::AlignCenter, "No Wonder Card loaded\nLoad a ROM and Wonder Card to begin");
        return;
    }

    // Create a copy for cursor drawing (if editing)
    QImage displayCard = m_renderedCard;

    // Draw cursor if editing
    if (!m_activeFieldName.isEmpty() && m_cursorVisible && !m_readOnly) {
        QPainter cursorPainter(&displayCard);
        drawCursor(cursorPainter);
        cursorPainter.end();
    }

    // Scale up using nearest-neighbor for crisp pixels
    QImage scaled = displayCard.scaled(
        CARD_WIDTH * DISPLAY_SCALE,
        CARD_HEIGHT * DISPLAY_SCALE,
        Qt::KeepAspectRatio,
        Qt::FastTransformation  // Nearest neighbor
    );

    painter.drawImage(0, 0, scaled);
}

void AuthenticWonderCardWidget::drawCursor(QPainter &painter)
{
    if (m_activeFieldName.isEmpty()) {
        return;
    }

    // Find field info
    int yStart = 0;
    for (const TextField &field : m_textFields) {
        if (field.name == m_activeFieldName) {
            yStart = field.yStart;
            break;
        }
    }

    QString text = m_fieldTexts.value(m_activeFieldName);

    // Calculate text width for cursor positioning
    int textWidth = 0;
    for (int i = 0; i < text.length(); ++i) {
        textWidth += m_fontRenderer->getCharWidth(text[i]);
    }

    // Calculate base X position based on field alignment
    int baseX;
    if (m_activeFieldName == "subtitle") {
        // Subtitle right-aligns to x=160 within header window
        // x = 160 - string_width; if (x < 0) x = 0;
        int windowX = 160 - textWidth;
        if (windowX < 0)
            windowX = 0;
        baseX = PADDING_LEFT + windowX;
    } else {
        // All other fields use standard padding
        baseX = PADDING_LEFT;
    }

    // Calculate cursor X position using glyph widths
    int xPos = baseX;
    for (int i = 0; i < m_cursorPos && i < text.length(); ++i) {
        xPos += m_fontRenderer->getCharWidth(text[i]);
    }

    // Draw cursor line (2 pixels wide, char height tall)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0));
    painter.drawRect(xPos, yStart, 2, CHAR_HEIGHT);
}

void AuthenticWonderCardWidget::toggleCursor()
{
    if (!m_activeFieldName.isEmpty() && !m_readOnly) {
        m_cursorVisible = !m_cursorVisible;
        update();
    }
}

void AuthenticWonderCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_hasData || m_readOnly) {
        return;
    }

    // Convert click position to card coordinates
    int x = event->position().x() / DISPLAY_SCALE;
    int y = event->position().y() / DISPLAY_SCALE;

    // Check which field was clicked
    int fieldIdx = findFieldAtY(y);
    if (fieldIdx >= 0 && fieldIdx < m_textFields.size()) {
        m_activeFieldName = m_textFields[fieldIdx].name;
        QString text = m_fieldTexts.value(m_activeFieldName);
        m_cursorPos = getCursorPosFromX(text, x);
        m_cursorVisible = true;

        renderCard();
        update();
        updateStatus();
        emit fieldSelected(m_activeFieldName);
    }

    setFocus();
}

int AuthenticWonderCardWidget::findFieldAtY(int y) const
{
    for (int i = 0; i < m_textFields.size(); ++i) {
        int yStart = m_textFields[i].yStart;
        if (y >= yStart && y < yStart + CHAR_HEIGHT + LINE_SPACING) {
            return i;
        }
    }
    return -1;
}

int AuthenticWonderCardWidget::getCursorPosFromX(const QString &text, int clickX) const
{
    // Calculate text width for alignment
    int textWidth = 0;
    for (int i = 0; i < text.length(); ++i) {
        textWidth += m_fontRenderer->getCharWidth(text[i]);
    }

    // Calculate base X position based on field alignment
    int baseX;
    if (m_activeFieldName == "subtitle") {
        // Subtitle right-aligns to x=160 within header window
        // x = 160 - string_width; if (x < 0) x = 0;
        int windowX = 160 - textWidth;
        if (windowX < 0)
            windowX = 0;
        baseX = PADDING_LEFT + windowX;
    } else {
        // All other fields use standard padding
        baseX = PADDING_LEFT;
    }

    int xPos = baseX;
    for (int i = 0; i < text.length(); ++i) {
        int charWidth = m_fontRenderer->getCharWidth(text[i]);
        if (clickX < xPos + charWidth / 2) {
            return i;
        }
        xPos += charWidth;
    }
    return text.length();
}

void AuthenticWonderCardWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_activeFieldName.isEmpty() || m_readOnly || !m_fontRenderer) {
        QWidget::keyPressEvent(event);
        return;
    }

    int key = event->key();
    QString text = m_fieldTexts.value(m_activeFieldName);

    bool textChanged = false;

    if (key == Qt::Key_Left) {
        if (m_cursorPos > 0) {
            m_cursorPos--;
        }
    } else if (key == Qt::Key_Right) {
        if (m_cursorPos < text.length()) {
            m_cursorPos++;
        }
    } else if (key == Qt::Key_Home) {
        m_cursorPos = 0;
    } else if (key == Qt::Key_End) {
        m_cursorPos = text.length();
    } else if (key == Qt::Key_Backspace) {
        if (m_cursorPos > 0) {
            text = text.left(m_cursorPos - 1) + text.mid(m_cursorPos);
            m_cursorPos--;
            m_fieldTexts[m_activeFieldName] = text;
            textChanged = true;
        }
    } else if (key == Qt::Key_Delete) {
        if (m_cursorPos < text.length()) {
            text = text.left(m_cursorPos) + text.mid(m_cursorPos + 1);
            m_fieldTexts[m_activeFieldName] = text;
            textChanged = true;
        }
    } else if (key == Qt::Key_Escape) {
        m_activeFieldName.clear();
    } else if (key == Qt::Key_Up) {
        moveToPrevField();
    } else if (key == Qt::Key_Down || key == Qt::Key_Return) {
        moveToNextField();
    } else {
        // Try to insert character
        QString inputText = event->text();
        if (!inputText.isEmpty()) {
            QChar ch = inputText[0];
            if (m_fontRenderer->canEncodeChar(ch)) {
                // Check byte limit
                int byteLimit = 40;
                for (const TextField &field : m_textFields) {
                    if (field.name == m_activeFieldName) {
                        byteLimit = field.byteLimit;
                        break;
                    }
                }

                if (m_fontRenderer->getEncodedLength(text) < byteLimit) {
                    text = text.left(m_cursorPos) + ch + text.mid(m_cursorPos);
                    m_cursorPos++;
                    m_fieldTexts[m_activeFieldName] = text;
                    textChanged = true;
                }
            }
        }
    }

    m_cursorVisible = true;

    if (textChanged) {
        renderCard();
        emit wonderCardChanged(buildWonderCardData());
    }

    update();
    updateStatus();
}

void AuthenticWonderCardWidget::moveToNextField()
{
    for (int i = 0; i < m_textFields.size(); ++i) {
        if (m_textFields[i].name == m_activeFieldName) {
            if (i < m_textFields.size() - 1) {
                m_activeFieldName = m_textFields[i + 1].name;
                m_cursorPos = m_fieldTexts.value(m_activeFieldName).length();
                emit fieldSelected(m_activeFieldName);
            }
            break;
        }
    }
}

void AuthenticWonderCardWidget::moveToPrevField()
{
    for (int i = 0; i < m_textFields.size(); ++i) {
        if (m_textFields[i].name == m_activeFieldName) {
            if (i > 0) {
                m_activeFieldName = m_textFields[i - 1].name;
                m_cursorPos = m_fieldTexts.value(m_activeFieldName).length();
                emit fieldSelected(m_activeFieldName);
            }
            break;
        }
    }
}

void AuthenticWonderCardWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    m_cursorVisible = true;
    update();
}

void AuthenticWonderCardWidget::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    // Keep cursor visible when focus is lost so selection is visible
    update();
}

void AuthenticWonderCardWidget::updateStatus()
{
    if (!m_activeFieldName.isEmpty() && m_fontRenderer) {
        QString text = m_fieldTexts.value(m_activeFieldName);
        int byteCount = m_fontRenderer->getEncodedLength(text);
        int maxBytes = 40;
        for (const TextField &field : m_textFields) {
            if (field.name == m_activeFieldName) {
                maxBytes = field.byteLimit;
                break;
            }
        }
        emit statusUpdate(m_activeFieldName, byteCount, maxBytes);
    }
}
