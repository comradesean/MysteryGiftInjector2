#ifndef AUTHENTICWONDERCARDWIDGET_H
#define AUTHENTICWONDERCARDWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QMap>
#include <QVector>
#include "mysterygift.h"
#include "gbaromreader.h"
#include "gen3fontrenderer.h"

/**
 * @brief Authentic Wonder Card widget using ROM-extracted graphics and fonts
 *
 * This widget replicates the Python wonder_card_editor.py functionality in C++/Qt.
 * It displays a Wonder Card using:
 * - ROM-extracted Wonder Card background graphics
 * - ROM-extracted Gen3 font with proper glyph widths
 * - ROM-extracted text palettes (stdpal_3)
 * - Proper text color tables for header vs body text
 *
 * Features:
 * - Click to select text fields
 * - Keyboard input for text editing
 * - Blinking cursor at current position
 * - Arrow key navigation between fields
 * - Gen3 character encoding validation
 * - Pokemon icon display
 */
class AuthenticWonderCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AuthenticWonderCardWidget(QWidget *parent = nullptr);
    ~AuthenticWonderCardWidget();

    // ROM loading
    bool loadROM(const QString &romPath, QString &errorMessage);
    bool loadFallbackGraphics(QString &errorMessage);
    bool isROMLoaded() const { return m_romLoaded; }
    bool isFallbackMode() const { return m_fallbackMode; }
    GBAROReader* getRomReader() { return m_romReader; }

    // Set/get Wonder Card data
    void setWonderCard(const WonderCardData &wonderCard);
    WonderCardData wonderCard() const { return m_wonderCard; }
    WonderCardData buildWonderCardData() const;  // Builds updated data from current field texts

    // Clear the display
    void clear();

    // Check if data is loaded
    bool hasData() const { return m_hasData; }

    // Set read-only mode (disable editing)
    void setReadOnly(bool readOnly) { m_readOnly = readOnly; update(); }
    bool isReadOnly() const { return m_readOnly; }

    // Background index
    void setBackgroundIndex(int index);
    int backgroundIndex() const { return m_bgIndex; }

    // Icon species
    void setIconSpecies(int species);
    int iconSpecies() const { return m_iconSpecies; }

    // Get the recommended size
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // Get current text for a field
    QString getFieldText(const QString &fieldName) const;

    // Set text for a field
    void setFieldText(const QString &fieldName, const QString &text);

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
        QString name;       // Field identifier
        int yStart;         // Y position on card (1x scale)
        int byteLimit;      // Maximum bytes for Gen3 encoding
        bool isHeader;      // Uses header color scheme if true
        bool isIdField;     // True for ID number field (Emerald uses different font)
    };

    // Initialize text field definitions
    void initTextFields();

    // Rendering methods
    void renderCard();
    void drawCursor(QPainter &painter);

    // Field interaction
    int findFieldAtY(int y) const;
    int getCursorPosFromX(const QString &text, int clickX) const;
    void moveToNextField();
    void moveToPrevField();
    void updateStatus();

    // Pokemon icon handling
    void loadPokemonIcon(int species);

    // ROM resources
    GBAROReader *m_romReader;
    Gen3FontRenderer *m_fontRenderer;
    bool m_romLoaded;
    bool m_fallbackMode;

    // Wonder Card backgrounds (cached)
    QVector<QImage> m_backgrounds;
    int m_bgIndex;

    // Colored font images
    QImage m_fontHeader;  // Title/header color scheme
    QImage m_fontBody;    // Body/footer color scheme

    // Emerald-specific: ID font (FONT_NORMAL) for ID number
    QImage m_fontIdHeader;
    QImage m_fontIdBody;

    // Pokemon icon
    QImage m_iconImage;
    int m_iconSpecies;

    // Data
    WonderCardData m_wonderCard;
    bool m_hasData;
    bool m_readOnly;

    // Text fields
    QVector<TextField> m_textFields;
    QMap<QString, QString> m_fieldTexts;  // field name -> text content
    QString m_activeFieldName;             // Currently editing field name
    int m_cursorPos;                       // Cursor position within active field

    // Cursor animation
    QTimer *m_cursorTimer;
    bool m_cursorVisible;

    // Rendered card cache
    QImage m_renderedCard;

    // Layout constants (GBA screen: 240x160)
    static const int CARD_WIDTH = 240;
    static const int CARD_HEIGHT = 160;
    static const int DISPLAY_SCALE = 2;
    static const int PADDING_LEFT = 8;
    static const int PADDING_TOP = 4;
    static const int LINE_SPACING = 2;
    static const int CHAR_HEIGHT = 14;

    // Icon position (center coordinates from decomp)
    static const int ICON_CENTER_X = 220;
    static const int ICON_CENTER_Y = 20;
    static const int ICON_SIZE = 32;
};

#endif // AUTHENTICWONDERCARDWIDGET_H
