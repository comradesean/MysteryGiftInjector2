#ifndef EDITABLEWONDERCARDWIDGET_H
#define EDITABLEWONDERCARDWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <QMap>
#include "mysterygift.h"
#include "gbaromreader.h"

/**
 * @brief Editable Wonder Card widget that allows text field editing
 *
 * This widget displays a Wonder Card and allows the user to edit text fields
 * by clicking on them and typing. Similar to the Python wonder_card_editor.py
 * but implemented in C++/Qt.
 *
 * Features:
 * - Click to select text fields (title, subtitle, content lines, warning lines)
 * - Keyboard input for text editing
 * - Blinking cursor at current position
 * - Arrow key navigation between fields
 * - Gen3 character encoding validation
 */
class EditableWonderCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditableWonderCardWidget(QWidget *parent = nullptr);
    ~EditableWonderCardWidget();

    // Set/get Wonder Card data
    void setWonderCard(const WonderCardData &wonderCard);
    WonderCardData wonderCard() const { return m_wonderCard; }

    // Clear the display
    void clear();

    // Check if data is loaded
    bool hasData() const { return m_hasData; }

    // Set read-only mode (disable editing)
    void setReadOnly(bool readOnly) { m_readOnly = readOnly; update(); }
    bool isReadOnly() const { return m_readOnly; }

    // Static ROM loading (shared with WonderCardRenderer)
    static bool loadROM(const QString &romPath, QString &errorMessage);
    static bool isROMLoaded();

    // Get the recommended size
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // Emitted when Wonder Card data changes
    void wonderCardChanged(const WonderCardData &wonderCard);

    // Emitted when a field is being edited
    void fieldSelected(const QString &fieldName);

    // Emitted with current field info (for status bar)
    void statusUpdate(const QString &fieldName, int byteCount, int maxBytes);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void toggleCursor();

private:
    // Text field structure
    struct TextField {
        QString name;           // Field identifier
        int yStart;             // Y position on card (1x scale)
        int byteLimit;          // Maximum bytes for Gen3 encoding
        QString* dataPtr;       // Pointer to WonderCardData field
    };

    // Initialize text field definitions
    void initTextFields();

    // Rendering methods
    void renderCard();
    void drawBackground(QPainter &painter);
    void drawBorder(QPainter &painter);
    void drawTitleArea(QPainter &painter);
    void drawContentArea(QPainter &painter);
    void drawWarningArea(QPainter &painter);
    void drawIcon(QPainter &painter);
    void drawCursor(QPainter &painter);
    void drawFieldHighlight(QPainter &painter);

    // Text rendering helpers
    void drawText(QPainter &painter, int x, int y, const QString &text, const QColor &color);
    int measureTextWidth(const QString &text) const;

    // Field interaction
    int findFieldAtY(int y) const;
    int getCursorPosFromX(const QString &text, int clickX) const;
    void moveToNextField();
    void moveToPrevField();

    // Character encoding
    bool canEncodeChar(QChar ch) const;
    int getEncodedLength(const QString &text) const;

    // Color helpers
    QColor getBackgroundColor() const;
    QColor getTitleBackgroundColor() const;

    // Data
    WonderCardData m_wonderCard;
    bool m_hasData;
    bool m_readOnly;
    QPixmap m_cachedIcon;

    // Text fields
    QVector<TextField> m_textFields;
    int m_activeFieldIndex;     // -1 if no field selected
    int m_cursorPos;            // Cursor position within active field

    // Cursor animation
    QTimer *m_cursorTimer;
    bool m_cursorVisible;

    // Static ROM reader
    static GBAROReader *s_romReader;
    static QMap<uint16_t, QPixmap> s_iconCache;

    // Layout constants (GBA screen: 240x160)
    static const int CARD_WIDTH = 240;
    static const int CARD_HEIGHT = 160;
    static const int DISPLAY_SCALE = 2;
    static const int BORDER_WIDTH = 8;
    static const int PADDING_LEFT = 8;
    static const int PADDING_TOP = 4;
    static const int LINE_HEIGHT = 16;
    static const int CHAR_HEIGHT = 14;
    static const int TITLE_AREA_HEIGHT = 50;
    static const int CONTENT_AREA_Y = 52;
    static const int WARNING_AREA_Y = 124;
    static const int ICON_X = 220;
    static const int ICON_Y = 20;
    static const int ICON_SIZE = 32;

    // Y positions for each text field (1x scale)
    static const int TITLE_Y = 9;
    static const int SUBTITLE_Y = 25;
    static const int CONTENT1_Y = 50;
    static const int CONTENT2_Y = 66;
    static const int CONTENT3_Y = 82;
    static const int CONTENT4_Y = 98;
    static const int WARNING1_Y = 119;
    static const int WARNING2_Y = 135;

    // Background colors (same as WonderCardRenderer)
    static const QColor BACKGROUND_COLORS[8];
    static const QColor TITLE_BG_COLORS[8];
};

#endif // EDITABLEWONDERCARDWIDGET_H
