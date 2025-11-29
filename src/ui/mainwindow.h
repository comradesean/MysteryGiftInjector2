/**
 * @file mainwindow.h
 * @brief Main application window for the Mystery Gift Injector.
 *
 * This file defines the MainWindow class, which serves as the primary user interface
 * for the Mystery Gift Injector application. The application allows users to:
 *
 * - Load Pokemon Generation III save files (.sav) from FireRed, LeafGreen, and Emerald
 * - View and edit Wonder Card data (Mystery Gift event cards)
 * - View and disassemble GREEN MAN scripts (event delivery scripts)
 * - Inject Wonder Cards into save files with proper checksum recalculation
 * - Export/import Wonder Card binary data
 *
 * The UI features a custom frameless window design with:
 * - Draggable title bar, menu bar, and toolbar areas
 * - Text and Hex view modes for Wonder Card/Script data
 * - Visual Wonder Card renderer with ROM-extracted graphics
 * - Preset selection for known official Mystery Gift events
 *
 * @note This application requires a Pokemon GBA ROM for authentic graphics rendering.
 *       Fallback graphics are used when no ROM is available.
 *
 * @author ComradeSean
 * @version 1.0
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QMainWindow>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QStackedWidget>
#include <QSpinBox>
#include <QActionGroup>

// =============================================================================
// Project Includes
// =============================================================================
#include "savefile.h"                    // Pokemon save file parsing and modification
#include "ticketmanager.h"               // Mystery Gift ticket resource management
#include "authenticwondercardwidget.h"   // Visual Wonder Card renderer/editor widget
#include "romdatabase.h"                 // ROM version identification database
#include "romloader.h"                   // ROM file discovery and loading
#include "scriptdisassembler.h"          // GBA script bytecode disassembler

/**
 * @class MainWindow
 * @brief The main application window providing the Mystery Gift Injector UI.
 *
 * MainWindow handles all user interaction including file operations, Wonder Card
 * viewing/editing, and save file injection. It uses a custom frameless window design
 * with rounded corners and a blue status bar matching the application's visual theme.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the main window and initializes all subsystems.
     * @param parent Optional parent widget (typically nullptr for main window).
     *
     * Initialization order:
     * 1. Load ROM database from embedded YAML resource
     * 2. Initialize script disassembler with command definitions
     * 3. Set up the UI (title bar, menu, toolbar, central widget, status bar)
     * 4. Load ticket presets from Tickets/ folder
     * 5. Auto-detect and load GBA ROM for graphics
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor - cleans up dynamically allocated resources.
     */
    ~MainWindow();

protected:
    // =========================================================================
    // Mouse Event Handlers (for frameless window dragging)
    // =========================================================================

    /** @brief Initiates window drag if click is in a draggable area. */
    void mousePressEvent(QMouseEvent *event) override;

    /** @brief Moves window during drag operation. */
    void mouseMoveEvent(QMouseEvent *event) override;

    /** @brief Ends window drag operation. */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /** @brief Filters events for title bar drag handling. */
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // =========================================================================
    // File Menu Slots
    // =========================================================================

    /** @brief Opens a Pokemon save file (.sav) via file dialog. */
    void onOpenClicked();

    /** @brief Closes the current save file and resets UI to initial state. */
    void onCloseFile();

    /** @brief Saves Wonder Card injection to the current file path. */
    void onSaveClicked();

    /** @brief Saves Wonder Card injection to a new file path via dialog. */
    void onSaveAsClicked();

    // =========================================================================
    // Edit Menu Slots
    // =========================================================================

    /** @brief Imports Wonder Card data from a .bin file. */
    void onImportWonderCard();

    /** @brief Exports current Wonder Card data to a .bin file. */
    void onExportWonderCard();

    /** @brief Clears the current Wonder Card data after confirmation. */
    void onClearWonderCard();

    // =========================================================================
    // View Menu Slots
    // =========================================================================

    /**
     * @brief Switches between Text and Hex view modes.
     * @param hexMode True for hex view, false for text/visual view.
     */
    void onViewModeChanged(bool hexMode);

    /** @brief Switches to the WonderCard sub-tab in both view modes. */
    void onSwitchToWonderCardTab();

    /** @brief Switches to the GREEN MAN sub-tab in both view modes. */
    void onSwitchToGreenManTab();

    // =========================================================================
    // Tools Menu Slots
    // =========================================================================

    /** @brief Opens the ROM tile viewer dialog for graphics inspection. */
    void onOpenTileViewer();

    /** @brief Manually load a ROM file via file dialog. */
    void onLoadRomManual();

    /** @brief Enables the Mystery Gift flag in the save file. */
    void onEnableMysteryGiftFlag();

    /** @brief Validates all checksums in the save file. */
    void onValidateChecksums();

    /** @brief Opens the options dialog (not yet implemented). */
    void onOpenOptions();

    // =========================================================================
    // Help Menu Slots
    // =========================================================================

    /** @brief Shows the documentation dialog. */
    void onShowDocumentation();

    /** @brief Shows the about dialog with version and credits. */
    void onShowAbout();

    // =========================================================================
    // Wonder Card Editing Slots
    // =========================================================================

    /**
     * @brief Handles preset dropdown changes to load ticket data.
     * @param preset The selected preset name (or "Custom").
     */
    void onPresetChanged(const QString &preset);

    /** @brief Shows popup menu for manual save type selection. */
    void onSaveTypeClicked();

    /** @brief Prompts for confirmation and enables Wonder Card editing mode. */
    void onEditButtonClicked();

    /**
     * @brief Updates stored data when Wonder Card is modified in editor.
     * @param wonderCard The modified Wonder Card data structure.
     */
    void onEditableWonderCardChanged(const WonderCardData &wonderCard);

    /**
     * @brief Updates status bar when a field is selected for editing.
     * @param fieldName Name of the selected field (e.g., "Title", "Subtitle").
     */
    void onEditableFieldSelected(const QString &fieldName);

    /**
     * @brief Updates status bar with byte count during text editing.
     * @param fieldName Name of the field being edited.
     * @param byteCount Current byte count of the field content.
     * @param maxBytes Maximum allowed bytes for the field.
     */
    void onEditableStatusUpdate(const QString &fieldName, int byteCount, int maxBytes);

    /**
     * @brief Updates Wonder Card background color index.
     * @param index Background index (0-7).
     */
    void onBgComboChanged(int index);

    /**
     * @brief Updates Wonder Card Pokemon icon species.
     * @param value Pokemon species index (0-412).
     */
    void onSpeciesSpinChanged(int value);

    /**
     * @brief Toggles whether the icon is disabled (set to 0xFFFF).
     * @param checked True to disable icon, false to show species.
     */
    void onIconDisabledToggled(bool checked);

    /**
     * @brief Updates Wonder Card type (Event/Stamp/Counter).
     * @param index Type index: 0=Event, 1=Stamp, 2=Counter.
     */
    void onTypeComboChanged(int index);

private:
    // =========================================================================
    // Window Dragging State
    // =========================================================================
    QPoint dragPosition;   ///< Mouse position relative to window for dragging
    bool m_dragging;       ///< True when window is being dragged

    // =========================================================================
    // UI Setup Methods
    // =========================================================================

    /** @brief Master UI setup - calls all create* methods and loads ROM. */
    void setupUI();

    /** @brief Creates the main content area with Wonder Card display and tabs. */
    void createCentralWidget();

    /** @brief Creates the blue status bar with status and checksum labels. */
    void createStatusBar();

    /**
     * @brief Creates the custom title bar for the frameless window.
     * @param parent Parent widget for the title bar.
     * @return The created title bar widget.
     */
    QWidget* createTitleBar(QWidget *parent);

    /**
     * @brief Creates the application menu bar with File, Edit, View, Tools, Help menus.
     * @param parent Parent widget for the menu bar.
     * @return The created menu bar.
     */
    QMenuBar* createMenuBar(QWidget *parent);

    /**
     * @brief Creates the toolbar with Open, Save, Save As buttons and file path display.
     * @param parent Parent widget for the toolbar.
     * @return The created toolbar.
     */
    QToolBar* createToolBar(QWidget *parent);

    // =========================================================================
    // Draggable UI Areas (for frameless window movement)
    // =========================================================================
    QWidget *m_titleBar;   ///< Custom title bar at top of window
    QMenuBar *m_menuBar;   ///< Menu bar (also draggable)
    QToolBar *m_toolBar;   ///< Toolbar with file operations (also draggable)

    // =========================================================================
    // Toolbar Widgets
    // =========================================================================
    QLabel *filePathDisplay;    ///< Shows current file path with elided directory
    QAction *openAction;        ///< Open file action (Ctrl+O)
    QAction *saveAction;        ///< Save file action (Ctrl+S)
    QAction *saveAsAction;      ///< Save As action (Ctrl+Shift+S)

    // =========================================================================
    // Menu Actions (stored for enable/disable state management)
    // =========================================================================
    QAction *closeAction;               ///< Close current file
    QAction *editEnableAction;          ///< Enable/disable editing mode
    QAction *importWcAction;            ///< Import Wonder Card binary
    QAction *exportWcAction;            ///< Export Wonder Card binary
    QAction *clearWcAction;             ///< Clear Wonder Card data
    QAction *textViewAction;            ///< Switch to text view (radio)
    QAction *hexViewAction;             ///< Switch to hex view (radio)
    QAction *loadRomAction;             ///< Manually load ROM
    QAction *enableMgFlagAction;        ///< Enable Mystery Gift flag in save
    QAction *validateChecksumsAction;   ///< Validate save file checksums

    // =========================================================================
    // View Mode Widgets
    // =========================================================================
    QStackedWidget *viewStack;   ///< Switches between text (idx 0) and hex (idx 1) views
    QPushButton *textViewBtn;    ///< Text view toggle button (segmented control)
    QPushButton *hexViewBtn;     ///< Hex view toggle button (segmented control)

    // =========================================================================
    // Metadata/Control Widgets
    // =========================================================================
    QCheckBox *activatedCheckBox1;   ///< Shows if Wonder Card is present (hidden, for future use)

    // =========================================================================
    // Text View Tab Widgets
    // =========================================================================
    QTabWidget *textSubTabs;                              ///< WonderCard / GREEN MAN tabs for text view
    AuthenticWonderCardWidget *wonderCardVisualDisplay;   ///< Visual Wonder Card renderer/editor
    QLabel *previewLabel;                                 ///< "Preview" or "Editor" label above renderer
    QPushButton *editButton;                              ///< "Edit" button to enable editing mode
    bool m_editingEnabled;                                ///< True when editing mode is active
    QTextEdit *scriptTextDisplay;                         ///< Disassembled script display (GREEN MAN tab)

    // =========================================================================
    // Hex View Tab Widgets
    // =========================================================================
    QTabWidget *hexSubTabs;           ///< WonderCard / GREEN MAN tabs for hex view
    QTextEdit *wonderCardHexDisplay;  ///< Raw hex dump of Wonder Card data
    QTextEdit *scriptHexDisplay;      ///< Raw hex dump of script data

    // =========================================================================
    // Wonder Card Data Group Widgets
    // =========================================================================
    QGroupBox *textDataGroup;   ///< Main container for Wonder Card display area
    QComboBox *presetCombo;     ///< Preset ticket selection dropdown
    QComboBox *giftCombo;       ///< Gift item selection dropdown (populated from ROM)

    // =========================================================================
    // Wonder Card Visual Control Widgets
    // =========================================================================
    QComboBox *bgCombo;          ///< Background color selector (0-7)
    QSpinBox *speciesSpin;       ///< Pokemon species for icon (0-412)
    QCheckBox *iconDisabledCheck; ///< Checkbox to hide icon (set to 0xFFFF)
    QComboBox *typeCombo;        ///< Type selector (Event/Stamp/Counter)

    // =========================================================================
    // Bottom Controls
    // =========================================================================
    QCheckBox *backupCheckBox;     ///< Create backup before saving
    QPushButton *saveTypeButton;   ///< Shows detected game type, clickable for override

    // =========================================================================
    // Status Bar Widgets
    // =========================================================================
    QLabel *statusLabel;     ///< Left side - status messages
    QLabel *checksumLabel;   ///< Right side - checksum validity

    // =========================================================================
    // Core Data Objects
    // =========================================================================
    SaveFile *m_saveFile;               ///< Current loaded Pokemon save file
    TicketManager *m_ticketManager;     ///< Manages ticket preset resources

    /** @brief Loads ticket presets from Tickets/ folder. */
    void loadTickets();

    /** @brief Populates the preset dropdown with loaded tickets. */
    void populatePresetDropdown();

    // =========================================================================
    // Wonder Card Data State
    // =========================================================================
    WonderCardData m_currentWonderCard;   ///< Parsed Wonder Card structure
    QByteArray m_currentScriptData;       ///< Raw GREEN MAN script bytes (with CRC header)
    QByteArray m_currentWonderCardRaw;    ///< Raw Wonder Card bytes (with CRC header)

    /**
     * @brief Updates all UI elements to display a Wonder Card.
     * @param wonderCard The Wonder Card data to display.
     * @param matchedTicket Optional matched ticket for preset selection.
     */
    void displayWonderCard(const WonderCardData &wonderCard,
                           const TicketResource *matchedTicket = nullptr);

    /** @brief Refreshes WonderCard visual and hex tabs from current data. */
    void updateWonderCardTabs();

    /** @brief Refreshes GREEN MAN text and hex tabs from current data. */
    void updateScriptTabs();

    /** @brief Clears all Wonder Card display and resets to empty state. */
    void clearWonderCardDisplay();

    /**
     * @brief Performs Wonder Card injection and saves to file.
     * @param savePath Path to save the modified file.
     * @param makeBackup True to create .bak backup before overwriting.
     */
    void performSave(const QString &savePath, bool makeBackup);

    /**
     * @brief Formats binary data as a hex dump with offsets and ASCII.
     * @param data Binary data to format.
     * @return Formatted hex dump string.
     */
    QString formatHexDump(const QByteArray &data);

    /** @brief Enables editing mode on the Wonder Card widget. */
    void enableEditing();

    /** @brief Resets to read-only mode and restores button states. */
    void resetEditState();

    // =========================================================================
    // ROM Loading
    // =========================================================================
    RomDatabase *m_romDatabase;    ///< ROM identification database (from YAML)
    QString m_romPath;             ///< Path to currently loaded ROM
    QString m_romVersionName;      ///< Human-readable ROM version name
    bool m_romLoaded;              ///< True if ROM graphics are loaded
    bool m_useFallbackGraphics;    ///< True to use built-in fallback graphics

    /** @brief Loads ROM database from embedded YAML resource. */
    void loadRomDatabase();

    /** @brief Auto-discovers and loads GBA ROM from application directory. */
    void loadROM();

    /** @brief Prompts user to select ROM or use fallback graphics. */
    void promptForROM();

    // =========================================================================
    // Script Disassembly
    // =========================================================================
    ScriptDisassembler *m_scriptDisassembler;   ///< GBA script bytecode disassembler

    /** @brief Loads script command definitions from embedded YAML resources. */
    void initScriptDisassembler();

    // =========================================================================
    // Gift Item Management
    // =========================================================================

    /** @brief Populates gift dropdown with item names from ROM. */
    void populateGiftDropdown();

    /**
     * @brief Extracts item ID from script's checkitem/setorcopyvar commands.
     * @param scriptData The GREEN MAN script data.
     * @return The extracted item ID, or 0xFFFF if not found.
     */
    uint16_t extractItemIdFromScript(const QByteArray &scriptData);

    /**
     * @brief Updates item IDs in the script when gift selection changes.
     * @param newItemId The new item ID to write to the script.
     */
    void updateScriptItemId(uint16_t newItemId);

    /** @brief Handles gift dropdown selection change. */
    void onGiftComboChanged(int index);

    // =========================================================================
    // Utility Methods
    // =========================================================================

    /**
     * @brief Updates the file path display with elided directory and bold filename.
     * @param fullPath Full path to the currently loaded file.
     */
    void setFilePathDisplay(const QString &fullPath);
};

#endif // MAINWINDOW_H
