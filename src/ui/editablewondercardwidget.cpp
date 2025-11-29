#include "editablewondercardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QDebug>

// Static member initialization
GBAROReader *EditableWonderCardWidget::s_romReader = nullptr;
QMap<uint16_t, QPixmap> EditableWonderCardWidget::s_iconCache;

// Background colors for the 8 Wonder Card types
const QColor EditableWonderCardWidget::BACKGROUND_COLORS[8] = {
    QColor(183, 175, 207),  // 0: Purple
    QColor(207, 191, 175),  // 1: Beige/Tan
    QColor(175, 207, 191),  // 2: Green
    QColor(183, 191, 207),  // 3: Blue
    QColor(207, 175, 175),  // 4: Red/Pink
    QColor(207, 207, 175),  // 5: Yellow
    QColor(191, 191, 191),  // 6: Gray
    QColor(175, 191, 207),  // 7: Light Blue
};

const QColor EditableWonderCardWidget::TITLE_BG_COLORS[8] = {
    QColor(143, 135, 167),  // 0: Dark Purple
    QColor(167, 151, 135),  // 1: Dark Beige
    QColor(135, 167, 151),  // 2: Dark Green
    QColor(143, 151, 167),  // 3: Dark Blue
    QColor(167, 135, 135),  // 4: Dark Red
    QColor(167, 167, 135),  // 5: Dark Yellow
    QColor(151, 151, 151),  // 6: Dark Gray
    QColor(135, 151, 167),  // 7: Dark Light Blue
};

EditableWonderCardWidget::EditableWonderCardWidget(QWidget *parent)
    : QWidget(parent)
    , m_hasData(false)
    , m_readOnly(false)
    , m_activeFieldIndex(-1)
    , m_cursorPos(0)
    , m_cursorVisible(true)
{
    setFixedSize(CARD_WIDTH * DISPLAY_SCALE, CARD_HEIGHT * DISPLAY_SCALE);
    setFocusPolicy(Qt::StrongFocus);

    // Initialize text field definitions
    initTextFields();

    // Setup cursor blink timer
    m_cursorTimer = new QTimer(this);
    connect(m_cursorTimer, &QTimer::timeout, this, &EditableWonderCardWidget::toggleCursor);
    m_cursorTimer->start(500);  // Blink every 500ms
}

EditableWonderCardWidget::~EditableWonderCardWidget()
{
}

void EditableWonderCardWidget::initTextFields()
{
    // Text field definitions: name, y_start, byte_limit
    // The dataPtr will be set when a Wonder Card is loaded
    m_textFields = {
        {"title",           TITLE_Y,    40, nullptr},
        {"subtitle",        SUBTITLE_Y, 40, nullptr},
        {"contentLine1",    CONTENT1_Y, 40, nullptr},
        {"contentLine2",    CONTENT2_Y, 40, nullptr},
        {"contentLine3",    CONTENT3_Y, 40, nullptr},
        {"contentLine4",    CONTENT4_Y, 40, nullptr},
        {"warningLine1",    WARNING1_Y, 40, nullptr},
        {"warningLine2",    WARNING2_Y, 40, nullptr}
    };
}

void EditableWonderCardWidget::setWonderCard(const WonderCardData &wonderCard)
{
    m_wonderCard = wonderCard;
    m_hasData = !wonderCard.isEmpty();

    // Update data pointers
    m_textFields[0].dataPtr = &m_wonderCard.title;
    m_textFields[1].dataPtr = &m_wonderCard.subtitle;
    m_textFields[2].dataPtr = &m_wonderCard.contentLine1;
    m_textFields[3].dataPtr = &m_wonderCard.contentLine2;
    m_textFields[4].dataPtr = &m_wonderCard.contentLine3;
    m_textFields[5].dataPtr = &m_wonderCard.contentLine4;
    m_textFields[6].dataPtr = &m_wonderCard.warningLine1;
    m_textFields[7].dataPtr = &m_wonderCard.warningLine2;

    // Reset editing state
    m_activeFieldIndex = -1;
    m_cursorPos = 0;

    // Load icon if ROM is available
    if (m_hasData && isROMLoaded()) {
        if (s_iconCache.contains(wonderCard.icon)) {
            m_cachedIcon = s_iconCache[wonderCard.icon];
        } else {
            QImage iconImg = s_romReader->extractPokemonIcon(wonderCard.icon);
            if (!iconImg.isNull()) {
                m_cachedIcon = QPixmap::fromImage(iconImg);
                s_iconCache[wonderCard.icon] = m_cachedIcon;
            } else {
                m_cachedIcon = QPixmap();
            }
        }
    } else {
        m_cachedIcon = QPixmap();
    }

    update();
}

void EditableWonderCardWidget::clear()
{
    m_hasData = false;
    m_wonderCard = WonderCardData();
    m_activeFieldIndex = -1;
    m_cursorPos = 0;
    m_cachedIcon = QPixmap();
    update();
}

bool EditableWonderCardWidget::loadROM(const QString &romPath, QString &errorMessage)
{
    if (s_romReader) {
        delete s_romReader;
        s_romReader = nullptr;
        s_iconCache.clear();
    }

    s_romReader = new GBAROReader();
    if (!s_romReader->loadROM(romPath, errorMessage)) {
        delete s_romReader;
        s_romReader = nullptr;
        return false;
    }

    return true;
}

bool EditableWonderCardWidget::isROMLoaded()
{
    return s_romReader != nullptr && s_romReader->isLoaded();
}

QSize EditableWonderCardWidget::sizeHint() const
{
    return QSize(CARD_WIDTH * DISPLAY_SCALE, CARD_HEIGHT * DISPLAY_SCALE);
}

QSize EditableWonderCardWidget::minimumSizeHint() const
{
    return QSize(CARD_WIDTH * DISPLAY_SCALE, CARD_HEIGHT * DISPLAY_SCALE);
}

void EditableWonderCardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Scale to 2x
    painter.scale(DISPLAY_SCALE, DISPLAY_SCALE);

    if (!m_hasData) {
        // Draw placeholder
        painter.fillRect(0, 0, CARD_WIDTH, CARD_HEIGHT, QColor(220, 220, 220));
        painter.setPen(QColor(150, 150, 150));

        QString placeholderText = m_readOnly ?
            "No Wonder Card loaded" :
            "No Wonder Card loaded\nLoad a card to edit";
        painter.drawText(QRect(0, 0, CARD_WIDTH, CARD_HEIGHT),
                         Qt::AlignCenter, placeholderText);
        return;
    }

    // Draw Wonder Card components
    drawBackground(painter);
    drawBorder(painter);
    drawTitleArea(painter);
    drawContentArea(painter);
    drawWarningArea(painter);
    drawIcon(painter);

    // Draw field highlight and cursor if editing
    if (!m_readOnly && m_activeFieldIndex >= 0) {
        drawFieldHighlight(painter);
        if (m_cursorVisible) {
            drawCursor(painter);
        }
    }
}

void EditableWonderCardWidget::drawBackground(QPainter &painter)
{
    painter.fillRect(0, 0, CARD_WIDTH, CARD_HEIGHT, getBackgroundColor());
}

void EditableWonderCardWidget::drawBorder(QPainter &painter)
{
    // Outer border
    painter.setPen(QPen(QColor(60, 60, 60), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(2, 2, CARD_WIDTH - 4, CARD_HEIGHT - 4);

    // Inner border
    painter.setPen(QPen(QColor(40, 40, 40), 1));
    painter.drawRect(BORDER_WIDTH, BORDER_WIDTH,
                     CARD_WIDTH - BORDER_WIDTH * 2,
                     CARD_HEIGHT - BORDER_WIDTH * 2);
}

void EditableWonderCardWidget::drawTitleArea(QPainter &painter)
{
    // Title background
    painter.fillRect(BORDER_WIDTH + 1, BORDER_WIDTH + 1,
                     CARD_WIDTH - BORDER_WIDTH * 2 - 2, TITLE_AREA_HEIGHT,
                     getTitleBackgroundColor());

    // Separator line
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.drawLine(BORDER_WIDTH + 1, TITLE_AREA_HEIGHT + BORDER_WIDTH,
                     CARD_WIDTH - BORDER_WIDTH - 1, TITLE_AREA_HEIGHT + BORDER_WIDTH);

    // Title text
    QFont titleFont("Courier New", 11, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 255, 255));

    QString titleText = QString("*%1*").arg(m_wonderCard.title);
    painter.drawText(QRect(BORDER_WIDTH, TITLE_Y - 4, CARD_WIDTH - BORDER_WIDTH * 2, 16),
                     Qt::AlignCenter, titleText);

    // Subtitle
    QFont subtitleFont("Courier New", 9);
    painter.setFont(subtitleFont);
    painter.drawText(QRect(BORDER_WIDTH, SUBTITLE_Y - 4, CARD_WIDTH - BORDER_WIDTH * 2 - ICON_SIZE - 4, 12),
                     Qt::AlignCenter, m_wonderCard.subtitle);
}

void EditableWonderCardWidget::drawContentArea(QPainter &painter)
{
    // Content background
    painter.fillRect(BORDER_WIDTH + 1, CONTENT_AREA_Y,
                     CARD_WIDTH - BORDER_WIDTH * 2 - 2, WARNING_AREA_Y - CONTENT_AREA_Y - 2,
                     QColor(240, 240, 245));

    // Content text
    QFont contentFont("Courier New", 9);
    painter.setFont(contentFont);
    painter.setPen(QColor(50, 50, 50));

    int contentMargin = 12;
    drawText(painter, contentMargin, CONTENT1_Y, m_wonderCard.contentLine1, QColor(50, 50, 50));
    drawText(painter, contentMargin, CONTENT2_Y, m_wonderCard.contentLine2, QColor(50, 50, 50));
    drawText(painter, contentMargin, CONTENT3_Y, m_wonderCard.contentLine3, QColor(50, 50, 50));
    drawText(painter, contentMargin, CONTENT4_Y, m_wonderCard.contentLine4, QColor(50, 50, 50));
}

void EditableWonderCardWidget::drawWarningArea(QPainter &painter)
{
    // Warning background
    painter.fillRect(BORDER_WIDTH + 1, WARNING_AREA_Y,
                     CARD_WIDTH - BORDER_WIDTH * 2 - 2, CARD_HEIGHT - WARNING_AREA_Y - BORDER_WIDTH - 1,
                     QColor(240, 240, 245));

    // Separator
    painter.setPen(QPen(QColor(180, 180, 180), 1));
    painter.drawLine(BORDER_WIDTH + 1, WARNING_AREA_Y,
                     CARD_WIDTH - BORDER_WIDTH - 1, WARNING_AREA_Y);

    // Warning text
    int contentMargin = 12;
    drawText(painter, contentMargin, WARNING1_Y, m_wonderCard.warningLine1, QColor(50, 50, 50));
    drawText(painter, contentMargin, WARNING2_Y, m_wonderCard.warningLine2, QColor(50, 50, 50));
}

void EditableWonderCardWidget::drawIcon(QPainter &painter)
{
    int iconX = ICON_X - ICON_SIZE / 2;
    int iconY = ICON_Y - ICON_SIZE / 2;

    if (!m_cachedIcon.isNull()) {
        painter.drawPixmap(iconX, iconY, ICON_SIZE, ICON_SIZE, m_cachedIcon);
    } else {
        // Placeholder
        painter.setPen(QPen(QColor(100, 100, 100), 1));
        painter.setBrush(QColor(200, 200, 200));
        painter.drawRect(iconX, iconY, ICON_SIZE, ICON_SIZE);

        QFont iconFont("Arial", 12, QFont::Bold);
        painter.setFont(iconFont);
        painter.setPen(QColor(255, 255, 255));

        QString iconText = (m_wonderCard.icon == WonderCardIcon::QUESTION_MARK) ? "?" :
                           QString::number(m_wonderCard.icon);
        painter.drawText(QRect(iconX, iconY, ICON_SIZE, ICON_SIZE),
                         Qt::AlignCenter, iconText);
    }
}

void EditableWonderCardWidget::drawText(QPainter &painter, int x, int y, const QString &text, const QColor &color)
{
    QFont font("Courier New", 9);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(x, y + 10, text);  // +10 to adjust baseline
}

void EditableWonderCardWidget::drawCursor(QPainter &painter)
{
    if (m_activeFieldIndex < 0 || m_activeFieldIndex >= m_textFields.size()) {
        return;
    }

    const TextField &field = m_textFields[m_activeFieldIndex];
    QString text = field.dataPtr ? *field.dataPtr : QString();

    // Calculate cursor X position
    QFont font("Courier New", 9);
    QFontMetrics fm(font);

    int cursorX = PADDING_LEFT;
    if (m_cursorPos > 0 && m_cursorPos <= text.length()) {
        cursorX += fm.horizontalAdvance(text.left(m_cursorPos));
    }

    // For title and subtitle, adjust for center alignment
    if (m_activeFieldIndex == 0) {  // title
        QString titleText = QString("*%1*").arg(text);
        QFont titleFont("Courier New", 11, QFont::Bold);
        QFontMetrics titleFm(titleFont);
        int textWidth = titleFm.horizontalAdvance(titleText);
        int startX = (CARD_WIDTH - textWidth) / 2;
        // Account for the leading *
        cursorX = startX + titleFm.horizontalAdvance("*") + titleFm.horizontalAdvance(text.left(m_cursorPos));
    } else if (m_activeFieldIndex == 1) {  // subtitle
        int textWidth = fm.horizontalAdvance(text);
        int availableWidth = CARD_WIDTH - BORDER_WIDTH * 2 - ICON_SIZE - 4;
        int startX = BORDER_WIDTH + (availableWidth - textWidth) / 2;
        cursorX = startX + fm.horizontalAdvance(text.left(m_cursorPos));
    } else {
        // Content and warning lines - left aligned
        cursorX = 12 + fm.horizontalAdvance(text.left(m_cursorPos));
    }

    // Draw cursor
    painter.setPen(QPen(QColor(0, 0, 0), 1));
    painter.drawLine(cursorX, field.yStart, cursorX, field.yStart + CHAR_HEIGHT);
}

void EditableWonderCardWidget::drawFieldHighlight(QPainter &painter)
{
    if (m_activeFieldIndex < 0 || m_activeFieldIndex >= m_textFields.size()) {
        return;
    }

    const TextField &field = m_textFields[m_activeFieldIndex];

    // Draw subtle highlight around the active field
    painter.setPen(QPen(QColor(100, 100, 255, 100), 1));
    painter.setBrush(QColor(100, 100, 255, 30));
    painter.drawRect(PADDING_LEFT - 2, field.yStart - 2,
                     CARD_WIDTH - PADDING_LEFT * 2 + 4, CHAR_HEIGHT + 4);
}

void EditableWonderCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_hasData || m_readOnly) {
        return;
    }

    // Convert click position to card coordinates (1x scale)
    int x = event->position().x() / DISPLAY_SCALE;
    int y = event->position().y() / DISPLAY_SCALE;

    // Find which field was clicked
    int fieldIndex = findFieldAtY(y);

    if (fieldIndex >= 0) {
        m_activeFieldIndex = fieldIndex;

        // Calculate cursor position from click X
        QString text = m_textFields[fieldIndex].dataPtr ? *m_textFields[fieldIndex].dataPtr : QString();
        m_cursorPos = getCursorPosFromX(text, x);
        m_cursorVisible = true;

        setFocus();
        emit fieldSelected(m_textFields[fieldIndex].name);
        emit statusUpdate(m_textFields[fieldIndex].name,
                          getEncodedLength(text),
                          m_textFields[fieldIndex].byteLimit);
        update();
    }
}

void EditableWonderCardWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_hasData || m_readOnly || m_activeFieldIndex < 0) {
        QWidget::keyPressEvent(event);
        return;
    }

    TextField &field = m_textFields[m_activeFieldIndex];
    QString text = field.dataPtr ? *field.dataPtr : QString();
    bool textChanged = false;

    int key = event->key();

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
        if (m_cursorPos > 0 && field.dataPtr) {
            text.remove(m_cursorPos - 1, 1);
            *field.dataPtr = text;
            m_cursorPos--;
            textChanged = true;
        }
    } else if (key == Qt::Key_Delete) {
        if (m_cursorPos < text.length() && field.dataPtr) {
            text.remove(m_cursorPos, 1);
            *field.dataPtr = text;
            textChanged = true;
        }
    } else if (key == Qt::Key_Escape) {
        m_activeFieldIndex = -1;
        clearFocus();
    } else if (key == Qt::Key_Up) {
        moveToPrevField();
    } else if (key == Qt::Key_Down || key == Qt::Key_Return || key == Qt::Key_Enter) {
        moveToNextField();
    } else {
        // Try to insert character
        QString inputText = event->text();
        if (!inputText.isEmpty() && field.dataPtr) {
            QChar ch = inputText[0];

            // Check if character can be encoded and we have room
            if (canEncodeChar(ch) && getEncodedLength(text) < field.byteLimit - 1) {
                text.insert(m_cursorPos, ch);
                *field.dataPtr = text;
                m_cursorPos++;
                textChanged = true;
            }
        }
    }

    m_cursorVisible = true;  // Reset cursor visibility on any key press

    if (textChanged) {
        emit wonderCardChanged(m_wonderCard);
    }

    // Update status
    if (field.dataPtr) {
        emit statusUpdate(field.name, getEncodedLength(*field.dataPtr), field.byteLimit);
    }

    update();
}

void EditableWonderCardWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    m_cursorVisible = true;
    update();
}

void EditableWonderCardWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    // Keep the active field but hide cursor
    m_cursorVisible = false;
    update();
}

void EditableWonderCardWidget::toggleCursor()
{
    if (m_activeFieldIndex >= 0 && hasFocus()) {
        m_cursorVisible = !m_cursorVisible;
        update();
    }
}

int EditableWonderCardWidget::findFieldAtY(int y) const
{
    for (int i = 0; i < m_textFields.size(); ++i) {
        int fieldY = m_textFields[i].yStart;
        if (y >= fieldY - 2 && y < fieldY + CHAR_HEIGHT + 2) {
            return i;
        }
    }
    return -1;
}

int EditableWonderCardWidget::getCursorPosFromX(const QString &text, int clickX) const
{
    QFont font("Courier New", 9);
    QFontMetrics fm(font);

    // Start X depends on field (title/subtitle are centered, others left-aligned)
    int startX = 12;  // Default for content/warning

    int xPos = startX;
    for (int i = 0; i < text.length(); ++i) {
        int charWidth = fm.horizontalAdvance(text[i]);
        if (clickX < xPos + charWidth / 2) {
            return i;
        }
        xPos += charWidth;
    }
    return text.length();
}

void EditableWonderCardWidget::moveToNextField()
{
    if (m_activeFieldIndex < m_textFields.size() - 1) {
        m_activeFieldIndex++;
        QString text = m_textFields[m_activeFieldIndex].dataPtr ?
                       *m_textFields[m_activeFieldIndex].dataPtr : QString();
        m_cursorPos = text.length();

        emit fieldSelected(m_textFields[m_activeFieldIndex].name);
        emit statusUpdate(m_textFields[m_activeFieldIndex].name,
                          getEncodedLength(text),
                          m_textFields[m_activeFieldIndex].byteLimit);
    }
}

void EditableWonderCardWidget::moveToPrevField()
{
    if (m_activeFieldIndex > 0) {
        m_activeFieldIndex--;
        QString text = m_textFields[m_activeFieldIndex].dataPtr ?
                       *m_textFields[m_activeFieldIndex].dataPtr : QString();
        m_cursorPos = text.length();

        emit fieldSelected(m_textFields[m_activeFieldIndex].name);
        emit statusUpdate(m_textFields[m_activeFieldIndex].name,
                          getEncodedLength(text),
                          m_textFields[m_activeFieldIndex].byteLimit);
    }
}

bool EditableWonderCardWidget::canEncodeChar(QChar ch) const
{
    // Check if character exists in Gen3 encoding table
    // This is a simplified check - the actual encoding is in mysterygift.cpp
    ushort unicode = ch.unicode();

    // Allow basic ASCII printable characters
    if (unicode >= 32 && unicode <= 126) {
        return true;
    }

    // Allow some common extended characters
    // Add more as needed based on Gen3 encoding table
    return false;
}

int EditableWonderCardWidget::getEncodedLength(const QString &text) const
{
    // Simple approximation: count encodable characters
    // The actual encoded length depends on the Gen3 encoding
    int count = 0;
    for (const QChar &ch : text) {
        if (canEncodeChar(ch)) {
            count++;
        }
    }
    return count;
}

int EditableWonderCardWidget::measureTextWidth(const QString &text) const
{
    QFont font("Courier New", 9);
    QFontMetrics fm(font);
    return fm.horizontalAdvance(text);
}

QColor EditableWonderCardWidget::getBackgroundColor() const
{
    uint8_t colorIndex = m_wonderCard.color();
    return BACKGROUND_COLORS[colorIndex % 8];
}

QColor EditableWonderCardWidget::getTitleBackgroundColor() const
{
    uint8_t colorIndex = m_wonderCard.color();
    return TITLE_BG_COLORS[colorIndex % 8];
}
