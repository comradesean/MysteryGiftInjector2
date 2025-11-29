/**
 * @file mainwindow.cpp
 * @brief Implementation of the MainWindow class for Mystery Gift Injector.
 *
 * This file contains the complete implementation of the main application window,
 * including:
 *
 * ## UI Construction (Lines ~35-900)
 * - Constructor and destructor
 * - Custom frameless window setup with draggable areas
 * - Menu bar creation with File, Edit, View, Tools, Help menus
 * - Toolbar with file operations and path display
 * - Central widget with Wonder Card display, tabs, and controls
 * - Status bar with checksum validation display
 *
 * ## Event Handling (Lines ~900-1100)
 * - Mouse events for frameless window dragging
 * - Preset dropdown changes
 * - Save type selection
 *
 * ## File Operations (Lines ~1100-1250)
 * - Opening Pokemon save files with validation
 * - Extracting and displaying existing Wonder Cards
 * - Edit mode enable/disable
 *
 * ## Wonder Card Display (Lines ~1350-1700)
 * - Displaying Wonder Cards in visual and hex views
 * - Updating tabs when data changes
 * - Hex dump formatting for raw data display
 *
 * ## Save Operations (Lines ~1700-1850)
 * - Injection options dialog
 * - Mystery Gift flag handling
 * - Checksum recalculation and file writing
 *
 * ## ROM Management (Lines ~1850-2000)
 * - Auto-discovery of GBA ROMs
 * - ROM validation via MD5 checksum
 * - Fallback graphics mode
 *
 * ## Wonder Card Controls (Lines ~2000-2200)
 * - Background color changes
 * - Icon species selection
 * - Type selection (Event/Stamp/Counter)
 * - Gift item dropdown
 *
 * ## Menu Action Handlers (Lines ~2200-2650)
 * - File menu: Close
 * - Edit menu: Import/Export/Clear Wonder Card
 * - View menu: Tab switching
 * - Tools menu: ROM loading, Mystery Gift flag, checksums
 * - Help menu: Documentation, About
 *
 * @author ComradeSean
 * @version 1.0
 */

// =============================================================================
// Project Includes
// =============================================================================
#include "mainwindow.h"
#include "tileviewer.h"
#include "authenticwondercardwidget.h"

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QMenuBar>
#include <QTimer>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QMenu>
#include <QTimer>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialog>
#include <QCheckBox>
#include <QApplication>
#include <QCoreApplication>
#include <QTextBrowser>
#include <QFile>
#include <QFont>
#include <QFontMetrics>

// =============================================================================
// CONSTRUCTOR & DESTRUCTOR
// =============================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_saveFile(new SaveFile())
    , m_ticketManager(new TicketManager())
    , m_romDatabase(new RomDatabase())
    , m_romLoaded(false)
    , m_useFallbackGraphics(false)
    , m_editingEnabled(false)
    , m_scriptDisassembler(new ScriptDisassembler())
    , m_dragging(false)
    , m_titleBar(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
{
    // Load ROM database from YAML
    loadRomDatabase();

    // Initialize script disassembler
    initScriptDisassembler();

    setupUI();
    loadTickets();

    // Set window properties
    setWindowTitle("ComradeSean's Mystery Gift Injector");
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(554, 380);

    // Create a custom checkmark icon
    QPixmap checkmark(16, 16);
    checkmark.fill(Qt::transparent);
    QPainter painter(&checkmark);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a pink/red checkmark
    QPen pen(QColor("#E91E63"), 2); // Pink/red color
    painter.setPen(pen);
    painter.drawLine(3, 8, 6, 11);
    painter.drawLine(6, 11, 13, 4);

    // Save as temporary file for stylesheet
    checkmark.save("temp_checkmark.png");

    // Set color scheme with rounded corners
    setStyleSheet(
        "QMainWindow { background-color: #e8e8e8; border-radius: 10px; }"
        "QMenuBar { background-color: #e8e8e8; border: 1px solid #b0b0b0; border-left: none; border-right: none; border-top: none; padding: 5px; color: #000000; border-radius: 0px; }"
        "QMenuBar::item { background-color: transparent; padding: 4px 10px; border-radius: 2px; color: #000000; }"
        "QMenuBar::item:selected { background-color: #4A90E2; color: white; }"
        "QMenu { background-color: white; border: 1px solid #b0b0b0; color: #000000; padding: 2px 0px; }"
        "QMenu::icon { margin-left: 15px; }"
        "QMenu::item { padding: 4px 20px 4px 12px; color: #000000; }"
        "QMenu::item:selected { background-color: #e0e0e0; color: #000000; }"
        "QMenu::separator { height: 1px; background: #d0d0d0; margin: 2px 0px; }"
        "QGroupBox { background-color: #f5f5f5; border: 1px solid #a0a0a0; "
        "            border-radius: 4px; margin-top: 10px; font-weight: bold; color: #000000; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; "
        "                   padding: 0 5px; color: #000000; }"
        "QComboBox, QLineEdit { background-color: white; border: 1px solid #a0a0a0; "
        "                       border-radius: 2px; padding: 3px; color: #000000; }"
        "QComboBox QAbstractItemView { background-color: white; color: #000000; "
        "                               selection-background-color: #e0e0e0; selection-color: #000000; }"
        "QTextEdit { background-color: white; border: 1px solid #a0a0a0; "
        "            border-radius: 2px; padding: 3px; color: #000000; }"
        "QCheckBox { color: #000000; spacing: 5px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #a0a0a0; "
        "                       background-color: white; border-radius: 2px; }"
        "QCheckBox::indicator:checked { background-color: white; "
        "                                image: url(temp_checkmark.png); }"
        "QPushButton { background-color: #d0d0d0; border: 1px solid #a0a0a0; "
        "              border-radius: 4px; padding: 5px; color: #000000; min-height: 20px; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
        "QPushButton:pressed { background-color: #b0b0b0; }"
        "QLabel { color: #000000; }"
        "QToolButton { background-color: #d0d0d0; border: 1px solid #a0a0a0; "
        "              border-radius: 4px; padding: 8px; color: #000000; }"
        "QToolButton:hover { background-color: #e0e0e0; }"
        "QToolButton:pressed { background-color: #b0b0b0; }"
        "QMessageBox { background-color: #e8e8e8; color: #000000; border: 1px solid #000000; }"
        "QMessageBox QLabel { color: #000000; background-color: transparent; }"
        "QMessageBox QPushButton { background-color: #d0d0d0; border: 1px solid #a0a0a0; "
        "                          border-radius: 4px; padding: 5px 15px; color: #000000; min-width: 60px; }"
        "QMessageBox QPushButton:hover { background-color: #e0e0e0; }"
        "QMessageBox QPushButton:pressed { background-color: #b0b0b0; }"
        "QDialog { background-color: #e8e8e8; color: #000000; border: 1px solid #000000; }"
        "QDialog QLabel { color: #000000; }"
        "QDialog QCheckBox { color: #000000; }"
        "QDialog QPushButton { background-color: #d0d0d0; border: 1px solid #a0a0a0; "
        "                      border-radius: 4px; padding: 5px 15px; color: #000000; }"
        "QDialog QPushButton:hover { background-color: #e0e0e0; }"
        "QDialog QPushButton:pressed { background-color: #b0b0b0; }"
        );
}

MainWindow::~MainWindow()
{
    delete m_saveFile;
    delete m_ticketManager;
    delete m_romDatabase;
    delete m_scriptDisassembler;
}

void MainWindow::initScriptDisassembler()
{
    QString error;

    // Load command definitions from embedded resource
    if (!m_scriptDisassembler->loadCommandDefinitions(":/Resources/script_commands.yaml", error)) {
        qWarning() << "Failed to load script commands:" << error;
    }

    // Load script data (flags, specials, etc.) from embedded resource
    if (!m_scriptDisassembler->loadScriptData(":/Resources/script_data.yaml", error)) {
        qWarning() << "Failed to load script data:" << error;
    }
}

// =============================================================================
// UI SETUP METHODS
// =============================================================================

void MainWindow::setupUI()
{
    // Note: Menu bar and toolbar are created inside createCentralWidget()
    // to maintain proper widget hierarchy for the custom frameless window
    createCentralWidget();
    createStatusBar();

    // Load ROM after all UI is created (statusLabel must exist for status updates)
    loadROM();
}

QWidget* MainWindow::createTitleBar(QWidget *parent)
{
    QWidget *titleBar = new QWidget(parent);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(30);
    titleBar->setStyleSheet(
        "QWidget#titleBar { background-color: #e8e8e8; "
        "border-top-left-radius: 10px; border-top-right-radius: 10px; }"
    );

    QHBoxLayout *h = new QHBoxLayout(titleBar);
    h->setContentsMargins(10, 0, 10, 0);

    QLabel *title = new QLabel("ComradeSean's Mystery Gift Injector", titleBar);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color: #000000; font-weight: bold; background: transparent;");

    h->addStretch();
    h->addWidget(title);
    h->addStretch();

    // Allow dragging
    titleBar->installEventFilter(this);

    return titleBar;
}

QMenuBar* MainWindow::createMenuBar(QWidget *parent)
{
    QMenuBar *menuBar = new QMenuBar(parent);

    // ===== FILE MENU =====
    QMenu *fileMenu = menuBar->addMenu("File");

    // Open action (reuse toolbar action)
    openAction = fileMenu->addAction("Open...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenClicked);

    // Close action - resets app to initial state
    closeAction = fileMenu->addAction("Close");
    closeAction->setShortcut(QKeySequence::Close);
    closeAction->setEnabled(false);  // Disabled until file is loaded
    connect(closeAction, &QAction::triggered, this, &MainWindow::onCloseFile);

    fileMenu->addSeparator();

    // Save action (reuse toolbar action)
    saveAction = fileMenu->addAction("Save");
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setEnabled(false);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveClicked);

    // Save As action (reuse toolbar action)
    saveAsAction = fileMenu->addAction("Save As...");
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    saveAsAction->setEnabled(false);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAsClicked);

    fileMenu->addSeparator();

    // TODO: Recent Files submenu (future enhancement)
    // QMenu *recentMenu = fileMenu->addMenu("Recent Files");
    // recentMenu->addAction("(No recent files)");

    fileMenu->addAction("Exit", QKeySequence::Quit, this, &QMainWindow::close);

    // ===== EDIT MENU =====
    QMenu *editMenu = menuBar->addMenu("Edit");

    editEnableAction = editMenu->addAction("Enable Editing");
    editEnableAction->setEnabled(false);  // Disabled until file is loaded
    connect(editEnableAction, &QAction::triggered, this, &MainWindow::onEditButtonClicked);

    editMenu->addSeparator();

    importWcAction = editMenu->addAction("Import Wonder Card...");
    importWcAction->setShortcut(QKeySequence("Ctrl+I"));
    connect(importWcAction, &QAction::triggered, this, &MainWindow::onImportWonderCard);

    exportWcAction = editMenu->addAction("Export Wonder Card...");
    exportWcAction->setShortcut(QKeySequence("Ctrl+E"));
    exportWcAction->setEnabled(false);  // Disabled until Wonder Card exists
    connect(exportWcAction, &QAction::triggered, this, &MainWindow::onExportWonderCard);

    editMenu->addSeparator();

    clearWcAction = editMenu->addAction("Clear Wonder Card");
    clearWcAction->setEnabled(false);  // Disabled until file is loaded
    connect(clearWcAction, &QAction::triggered, this, &MainWindow::onClearWonderCard);

    // ===== VIEW MENU =====
    QMenu *viewMenu = menuBar->addMenu("View");

    // Radio button group for Text/Hex view
    QActionGroup *viewModeGroup = new QActionGroup(this);
    viewModeGroup->setExclusive(true);

    textViewAction = viewMenu->addAction("Text View");
    textViewAction->setCheckable(true);
    textViewAction->setChecked(true);
    textViewAction->setShortcut(QKeySequence("Ctrl+1"));
    viewModeGroup->addAction(textViewAction);
    connect(textViewAction, &QAction::triggered, this, [this]() { onViewModeChanged(false); });

    hexViewAction = viewMenu->addAction("Hex View");
    hexViewAction->setCheckable(true);
    hexViewAction->setShortcut(QKeySequence("Ctrl+2"));
    viewModeGroup->addAction(hexViewAction);
    connect(hexViewAction, &QAction::triggered, this, [this]() { onViewModeChanged(true); });

    viewMenu->addSeparator();

    QAction *wcTabAction = viewMenu->addAction("WonderCard Tab");
    wcTabAction->setShortcut(QKeySequence("Ctrl+["));
    connect(wcTabAction, &QAction::triggered, this, &MainWindow::onSwitchToWonderCardTab);

    QAction *gmTabAction = viewMenu->addAction("GREEN MAN Tab");
    gmTabAction->setShortcut(QKeySequence("Ctrl+]"));
    connect(gmTabAction, &QAction::triggered, this, &MainWindow::onSwitchToGreenManTab);

    // ===== TOOLS MENU =====
    QMenu *toolsMenu = menuBar->addMenu("Tools");

    QAction *tileViewerAction = toolsMenu->addAction("ROM Tile Viewer");
    connect(tileViewerAction, &QAction::triggered, this, &MainWindow::onOpenTileViewer);

    loadRomAction = toolsMenu->addAction("Load ROM...");
    connect(loadRomAction, &QAction::triggered, this, &MainWindow::onLoadRomManual);

    toolsMenu->addSeparator();

    enableMgFlagAction = toolsMenu->addAction("Enable Mystery Gift Flag");
    enableMgFlagAction->setEnabled(false);  // Disabled until file is loaded
    connect(enableMgFlagAction, &QAction::triggered, this, &MainWindow::onEnableMysteryGiftFlag);

    validateChecksumsAction = toolsMenu->addAction("Validate Checksums");
    validateChecksumsAction->setEnabled(false);  // Disabled until file is loaded
    connect(validateChecksumsAction, &QAction::triggered, this, &MainWindow::onValidateChecksums);

    toolsMenu->addSeparator();

    QAction *optionsAction = toolsMenu->addAction("Options...");
    connect(optionsAction, &QAction::triggered, this, &MainWindow::onOpenOptions);

    // ===== HELP MENU =====
    QMenu *helpMenu = menuBar->addMenu("Help");

    QAction *docsAction = helpMenu->addAction("Documentation");
    docsAction->setShortcut(QKeySequence::HelpContents);
    connect(docsAction, &QAction::triggered, this, &MainWindow::onShowDocumentation);

    helpMenu->addSeparator();

    QAction *aboutAction = helpMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);

    return menuBar;
}

QToolBar* MainWindow::createToolBar(QWidget *parent)
{
    QToolBar *toolBar = new QToolBar(parent);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolBar->setStyleSheet(
        "QToolBar { background-color: #e8e8e8; border: none; padding: 5px; spacing: 5px; }"
        "QToolButton { background-color: #d0d0d0; border: 1px solid #a0a0a0; border-radius: 4px; padding: 5px 8px; color: #000000; }"
        "QToolButton:hover { background-color: #e0e0e0; }"
        "QToolButton:pressed { background-color: #b0b0b0; }"
        );

    // Add menu actions to toolbar with icons (actions already created in createMenuBar)
    openAction->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    toolBar->addAction(openAction);

    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    toolBar->addAction(saveAction);

    closeAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    toolBar->addAction(closeAction);

    toolBar->addSeparator();

    // Add file path display (address bar style with rich text support)
    filePathDisplay = new QLabel();
    filePathDisplay->setTextFormat(Qt::RichText);
    filePathDisplay->setText("<span style='color: #909090; font-style: italic;'>No file loaded</span>");
    filePathDisplay->setStyleSheet(
        "QLabel { background-color: white; border: 1px solid #a0a0a0; "
        "         border-radius: 2px; padding: 4px 8px; color: #000000; }"
    );
    filePathDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    filePathDisplay->setMinimumWidth(200);
    toolBar->addWidget(filePathDisplay);

    return toolBar;
}

void MainWindow::createCentralWidget()
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(
        "QWidget#centralWidget { background-color: #e8e8e8; "
        "border-top-left-radius: 10px; border-top-right-radius: 10px; }"
    );
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    // Main vertical layout (title bar + content)
    QVBoxLayout *vbox = new QVBoxLayout(centralWidget);
    vbox->setSpacing(0);
    vbox->setContentsMargins(0, 0, 0, 0);

    // Ensure central widget layout also has no margins
    if (centralWidget->layout()) {
        centralWidget->layout()->setContentsMargins(0, 0, 0, 0);
        centralWidget->layout()->setSpacing(0);
    }

    m_titleBar = createTitleBar(centralWidget);
    vbox->addWidget(m_titleBar);

    m_menuBar = createMenuBar(centralWidget);
    vbox->addWidget(m_menuBar);

    m_toolBar = createToolBar(centralWidget);
    vbox->addWidget(m_toolBar);

    // Main content area - full width panels
    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(15, 15, 15, 10);

    // === WONDERCARD DATA GROUPBOX ===
    textDataGroup = new QGroupBox("WonderCard Data");
    QVBoxLayout *dataGroupLayout = new QVBoxLayout(textDataGroup);
    dataGroupLayout->setContentsMargins(10, 10, 10, 10);

    // Metadata fields - using rows with view toggle
    QVBoxLayout *metadataLayout = new QVBoxLayout();
    metadataLayout->setSpacing(8);

    // First row: Preset dropdown + Activated + View toggle
    QHBoxLayout *presetRow = new QHBoxLayout();

    QLabel *textPresetLabel = new QLabel("Preset:");
    textPresetLabel->setStyleSheet("font-weight: bold;");
    presetRow->addWidget(textPresetLabel);

    presetCombo = new QComboBox();
    presetCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    presetCombo->setMaximumWidth(280);
    presetCombo->setEnabled(false);  // Disabled until ROM loaded and edit mode active
    presetRow->addWidget(presetCombo);

    // Connect preset combo to load ticket data
    connect(presetCombo, &QComboBox::currentTextChanged, this, &MainWindow::onPresetChanged);

    // TODO: Activated checkbox will be used for viewing and altering the flags
    // involved with the GREEN MAN script (e.g., Mystery Gift event flags/vars).
    // Hidden for now until flag editing is implemented.
    activatedCheckBox1 = new QCheckBox("Activated");
    activatedCheckBox1->setChecked(false);
    activatedCheckBox1->setEnabled(false);
    activatedCheckBox1->setVisible(false);
    presetRow->addWidget(activatedCheckBox1);

    // Push Text|Hex buttons to the right
    presetRow->addStretch();

    // Small radio buttons for Text/Hex view toggle
    textViewBtn = new QPushButton("Text");
    textViewBtn->setCheckable(true);
    textViewBtn->setChecked(true);
    textViewBtn->setFixedHeight(20);
    textViewBtn->setFixedWidth(25);
    textViewBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #505050; "
        "              border: none; padding: 0px 2px; font-size: 11px; }"
        "QPushButton:checked { color: #000000; font-weight: bold; }"
        "QPushButton:hover { color: #000000; }"
    );
    presetRow->addWidget(textViewBtn);

    QLabel *viewSeparator = new QLabel("|");
    viewSeparator->setStyleSheet("color: #a0a0a0;");
    presetRow->addWidget(viewSeparator);

    hexViewBtn = new QPushButton("Hex");
    hexViewBtn->setCheckable(true);
    hexViewBtn->setChecked(false);
    hexViewBtn->setFixedHeight(20);
    textViewBtn->setFixedWidth(25);
    hexViewBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #505050; "
        "              border: none; padding: 0px 2px; font-size: 11px; }"
        "QPushButton:checked { color: #000000; font-weight: bold; }"
        "QPushButton:hover { color: #000000; }"
    );
    presetRow->addWidget(hexViewBtn);

    // Connect view toggle buttons
    connect(textViewBtn, &QPushButton::clicked, this, [this]() { onViewModeChanged(false); });
    connect(hexViewBtn, &QPushButton::clicked, this, [this]() { onViewModeChanged(true); });

    metadataLayout->addLayout(presetRow);

    // Second row: Gift dropdown
    QHBoxLayout *giftRow = new QHBoxLayout();

    QLabel *textGiftLabel = new QLabel("Gift:");
    textGiftLabel->setStyleSheet("font-weight: bold;");
    giftRow->addWidget(textGiftLabel);

    giftCombo = new QComboBox();
    giftCombo->addItems({"Aurora Ticket", "Mystic Ticket"});  // Default items until ROM loaded
    giftCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    giftCombo->setMaxVisibleItems(20);
    giftCombo->setEnabled(false);  // Disabled until ROM loaded and edit mode active
    connect(giftCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onGiftComboChanged);
    giftRow->addWidget(giftCombo);

    metadataLayout->addLayout(giftRow);

    dataGroupLayout->addLayout(metadataLayout);

    // === STACKED WIDGET FOR TEXT/HEX VIEWS ===
    viewStack = new QStackedWidget();

    // === TEXT VIEW (index 0) ===
    QWidget *textView = new QWidget();
    QVBoxLayout *textViewLayout = new QVBoxLayout(textView);
    textViewLayout->setContentsMargins(0, 0, 0, 0);

    // Sub-tabs for WonderCard vs GREEN MAN (Text view)
    textSubTabs = new QTabWidget();
    textSubTabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #a0a0a0; border-radius: 3px; background: #f5f5f5; padding: 0px; margin: 0px; }"
        "QTabBar::tab { background-color: #d0d0d0; border: 1px solid #a0a0a0; "
        "               border-bottom: none; border-top-left-radius: 3px; "
        "               border-top-right-radius: 3px; padding: 6px 12px; "
        "               margin-right: 2px; color: #000000; }"
        "QTabBar::tab:selected { background-color: #f5f5f5; color: #000000; }"
        "QTabBar::tab:hover { background-color: #e0e0e0; }"
    );

    // Edit button as corner widget - aligned with tabs
    QWidget *editButtonContainer = new QWidget();
    QHBoxLayout *editButtonLayout = new QHBoxLayout(editButtonContainer);
    editButtonLayout->setContentsMargins(0, 0, 12, 0);  // Right margin to prevent clipping
    editButtonLayout->setSpacing(0);

    editButton = new QPushButton("Edit");
    editButton->setFixedSize(50, 24);
    editButton->setStyleSheet(
        "QPushButton { background-color: #d0d0d0; color: #000000; "
        "              border: 1px solid #a0a0a0; border-radius: 3px; "
        "              padding: 2px 4px; font-size: 11px; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
        "QPushButton:pressed { background-color: #b0b0b0; }"
        "QPushButton:disabled { background-color: #e8e8e8; color: #a0a0a0; border-color: #c0c0c0; }"
    );
    editButton->setEnabled(false);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::onEditButtonClicked);
    editButtonLayout->addWidget(editButton);

    textSubTabs->setCornerWidget(editButtonContainer, Qt::TopRightCorner);

    // Text > WonderCard sub-tab
    QWidget *textWonderCardTab = new QWidget();
    QVBoxLayout *textWcLayout = new QVBoxLayout(textWonderCardTab);
    // Right margin reduced to compensate for QTabWidget internal padding asymmetry
    textWcLayout->setContentsMargins(10, 10, 2, 10);

    // Invisible container for visual renderer with minimize button
    QWidget *rendererContainer = new QWidget();
    rendererContainer->setStyleSheet("background: transparent; border: none; padding: 0px; margin: 0px;");
    rendererContainer->setFixedWidth(240 * 2);  // Match renderer width to prevent arrow from jumping
    QVBoxLayout *rendererContainerLayout = new QVBoxLayout(rendererContainer);
    rendererContainerLayout->setContentsMargins(0, 0, 0, 0);
    rendererContainerLayout->setSpacing(2);

    // Visual renderer (toggleable between read-only preview and editor)
    previewLabel = new QLabel("Preview (Read-only):");
    previewLabel->setStyleSheet("font-weight: bold; color: #000000; margin-bottom: 2px;");
    rendererContainerLayout->addWidget(previewLabel);

    wonderCardVisualDisplay = new AuthenticWonderCardWidget();
    wonderCardVisualDisplay->setStyleSheet(
        "background-color: #e0e0e0; border: 1px solid #b8b8b8; border-radius: 3px;"
    );
    wonderCardVisualDisplay->setReadOnly(true);  // Start in read-only mode
    rendererContainerLayout->addWidget(wonderCardVisualDisplay, 0, Qt::AlignLeft | Qt::AlignTop);

    // Wonder Card visual controls (BG, Icon, Type)
    QHBoxLayout *wcControlsLayout = new QHBoxLayout();
    wcControlsLayout->setContentsMargins(0, 4, 0, 0);
    wcControlsLayout->setSpacing(8);

    // Simple clean style for controls
    QString controlStyle =
        "QComboBox, QSpinBox { background-color: white; border: 1px solid #a0a0a0; "
        "                      border-radius: 2px; padding: 1px 2px; color: #000000; }"
        "QComboBox:disabled, QSpinBox:disabled { background-color: #e8e8e8; color: #808080; }"
        "QComboBox QAbstractItemView { background-color: white; color: #000000; "
        "                               selection-background-color: #e0e0e0; }";

    // BG dropdown (0-7)
    QLabel *bgLabel = new QLabel("BG:");
    bgLabel->setStyleSheet("font-weight: bold; color: #000000;");
    wcControlsLayout->addWidget(bgLabel);

    bgCombo = new QComboBox();
    for (int i = 0; i < 8; ++i) {
        bgCombo->addItem(QString::number(i));
    }
    bgCombo->setFixedWidth(45);
    bgCombo->setStyleSheet(controlStyle);
    bgCombo->setEnabled(false);  // Disabled until editing enabled
    connect(bgCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBgComboChanged);
    wcControlsLayout->addWidget(bgCombo);

    // Icon field (0-412) - use QLineEdit with validator for cleaner look
    QLabel *iconLabel = new QLabel("Icon:");
    iconLabel->setStyleSheet("font-weight: bold; color: #000000;");
    wcControlsLayout->addWidget(iconLabel);

    speciesSpin = new QSpinBox();
    speciesSpin->setRange(0, 412);
    speciesSpin->setButtonSymbols(QSpinBox::NoButtons);  // Hide arrows for cleaner look
    speciesSpin->setFixedWidth(45);
    speciesSpin->setStyleSheet(controlStyle);
    speciesSpin->setEnabled(false);  // Disabled until editing enabled
    connect(speciesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onSpeciesSpinChanged);
    wcControlsLayout->addWidget(speciesSpin);

    // Checkbox to disable icon (sets to 0xFFFF)
    iconDisabledCheck = new QCheckBox();
    iconDisabledCheck->setToolTip("Hide icon (set to 0xFFFF)");
    iconDisabledCheck->setEnabled(false);  // Disabled until editing enabled
    connect(iconDisabledCheck, &QCheckBox::toggled,
            this, &MainWindow::onIconDisabledToggled);
    wcControlsLayout->addWidget(iconDisabledCheck);

    // Type dropdown (Event, Stamp, Counter)
    QLabel *typeLabel = new QLabel("Type:");
    typeLabel->setStyleSheet("font-weight: bold; color: #000000;");
    wcControlsLayout->addWidget(typeLabel);

    typeCombo = new QComboBox();
    typeCombo->addItem("Event");
    typeCombo->addItem("Stamp");
    typeCombo->addItem("Counter");
    typeCombo->setFixedWidth(70);
    typeCombo->setStyleSheet(controlStyle);
    typeCombo->setEnabled(false);  // Disabled until editing enabled
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTypeComboChanged);
    wcControlsLayout->addWidget(typeCombo);

    wcControlsLayout->addStretch();

    rendererContainerLayout->addLayout(wcControlsLayout);

    // Connect signals for editing (will be used when editing is enabled)
    connect(wonderCardVisualDisplay, &AuthenticWonderCardWidget::wonderCardChanged,
            this, &MainWindow::onEditableWonderCardChanged);
    connect(wonderCardVisualDisplay, &AuthenticWonderCardWidget::fieldSelected,
            this, &MainWindow::onEditableFieldSelected);
    connect(wonderCardVisualDisplay, &AuthenticWonderCardWidget::statusUpdate,
            this, &MainWindow::onEditableStatusUpdate);

    // Note: loadROM() is called later in setupUI() after statusBar is created

    textWcLayout->addWidget(rendererContainer, 0, Qt::AlignLeft | Qt::AlignTop);

    // Text > GREEN MAN sub-tab
    QWidget *textScriptTab = new QWidget();
    QVBoxLayout *textScriptLayout = new QVBoxLayout(textScriptTab);
    textScriptLayout->setContentsMargins(5, 5, 5, 5);

    scriptTextDisplay = new QTextEdit();
    scriptTextDisplay->setPlaceholderText("No script data");
    scriptTextDisplay->setReadOnly(true);
    scriptTextDisplay->setLineWrapMode(QTextEdit::NoWrap);  // No text wrapping
    scriptTextDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scriptTextDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scriptTextDisplay->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1e1e1e; color: #d4d4d4; "
        "  padding: 8px; border: 1px solid #b8b8b8; "
        "  border-radius: 3px; font-family: 'Consolas', 'Courier New', monospace; "
        "  font-size: 11px; "
        "}"
        "QScrollBar:vertical { "
        "  border: none; background: #2d2d2d; width: 10px; margin: 0; "
        "}"
        "QScrollBar::handle:vertical { "
        "  background: #5a5a5a; min-height: 20px; border-radius: 4px; "
        "}"
        "QScrollBar::handle:vertical:hover { "
        "  background: #6a6a6a; "
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "  height: 0; background: none; "
        "}"
        "QScrollBar:horizontal { "
        "  border: none; background: #2d2d2d; height: 10px; margin: 0; "
        "}"
        "QScrollBar::handle:horizontal { "
        "  background: #5a5a5a; min-width: 20px; border-radius: 4px; "
        "}"
        "QScrollBar::handle:horizontal:hover { "
        "  background: #6a6a6a; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
        "  width: 0; background: none; "
        "}"
    );
    scriptTextDisplay->document()->setDocumentMargin(4);
    textScriptLayout->addWidget(scriptTextDisplay);

    textSubTabs->addTab(textWonderCardTab, "WonderCard");
    textSubTabs->addTab(textScriptTab, "GREEN MAN");
    textViewLayout->addWidget(textSubTabs);

    // === HEX VIEW (index 1) ===
    QWidget *hexView = new QWidget();
    QVBoxLayout *hexViewLayout = new QVBoxLayout(hexView);
    hexViewLayout->setContentsMargins(0, 0, 0, 0);

    // Sub-tabs for WonderCard vs GREEN MAN (Hex view)
    hexSubTabs = new QTabWidget();
    hexSubTabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #a0a0a0; border-radius: 3px; background: #f5f5f5; }"
        "QTabBar::tab { background-color: #d0d0d0; border: 1px solid #a0a0a0; "
        "               border-bottom: none; border-top-left-radius: 3px; "
        "               border-top-right-radius: 3px; padding: 6px 12px; "
        "               margin-right: 2px; color: #000000; }"
        "QTabBar::tab:selected { background-color: #f5f5f5; color: #000000; }"
        "QTabBar::tab:hover { background-color: #e0e0e0; }"
    );

    // Hex > WonderCard sub-tab
    QWidget *hexWonderCardTab = new QWidget();
    QVBoxLayout *hexWcLayout = new QVBoxLayout(hexWonderCardTab);
    hexWcLayout->setContentsMargins(5, 5, 5, 5);

    wonderCardHexDisplay = new QTextEdit();
    wonderCardHexDisplay->setPlaceholderText("No WonderCard hex data");
    wonderCardHexDisplay->setReadOnly(true);
    wonderCardHexDisplay->setLineWrapMode(QTextEdit::NoWrap);
    wonderCardHexDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    wonderCardHexDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    wonderCardHexDisplay->setStyleSheet(
        "QTextEdit { "
        "  background-color: #2b2b2b; color: #00ff00; "
        "  padding: 8px; border: 1px solid #404040; "
        "  border-radius: 3px; font-family: 'Courier New', monospace; "
        "  font-size: 9pt; "
        "}"
        "QScrollBar:vertical { "
        "  border: none; background: #1a1a1a; width: 10px; margin: 0; "
        "}"
        "QScrollBar::handle:vertical { "
        "  background: #00cc00; min-height: 20px; border-radius: 4px; "
        "}"
        "QScrollBar::handle:vertical:hover { "
        "  background: #00cc00; "
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "  height: 0; background: none; "
        "}"
        "QScrollBar:horizontal { "
        "  border: none; background: #1a1a1a; height: 10px; margin: 0; "
        "}"
        "QScrollBar::handle:horizontal { "
        "  background: #00cc00; min-width: 20px; border-radius: 4px; "
        "}"
        "QScrollBar::handle:horizontal:hover { "
        "  background: #00cc00; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
        "  width: 0; background: none; "
        "}"
    );
    wonderCardHexDisplay->document()->setDocumentMargin(4);
    hexWcLayout->addWidget(wonderCardHexDisplay);

    // Hex > GREEN MAN sub-tab
    QWidget *hexScriptTab = new QWidget();
    QVBoxLayout *hexScriptLayout = new QVBoxLayout(hexScriptTab);
    hexScriptLayout->setContentsMargins(5, 5, 5, 5);

    scriptHexDisplay = new QTextEdit();
    scriptHexDisplay->setPlaceholderText("No script hex data");
    scriptHexDisplay->setReadOnly(true);
    scriptHexDisplay->setLineWrapMode(QTextEdit::NoWrap);
    scriptHexDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scriptHexDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scriptHexDisplay->setStyleSheet(
        "QTextEdit { "
        "  background-color: #2b2b2b; color: #00ff00; "
        "  padding: 8px; border: 1px solid #404040; "
        "  border-radius: 3px; font-family: 'Courier New', monospace; "
        "  font-size: 9pt; "
        "}"
        "QScrollBar:vertical { "
        "  border: none; background: #1a1a1a; width: 10px; margin: 0; "
        "}"
        "QScrollBar::handle:vertical { "
        "  background: #00cc00; min-height: 20px; border-radius: 4px; "
        "}"
        "QScrollBar::handle:vertical:hover { "
        "  background: #00cc00; "
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "  height: 0; background: none; "
        "}"
        "QScrollBar:horizontal { "
        "  border: none; background: #1a1a1a; height: 10px; margin: 0; "
        "}"
        "QScrollBar::handle:horizontal { "
        "  background: #00cc00; min-width: 20px; border-radius: 4px; "
        "}"
        "QScrollBar::handle:horizontal:hover { "
        "  background: #00cc00; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
        "  width: 0; background: none; "
        "}"
    );
    scriptHexDisplay->document()->setDocumentMargin(4);
    hexScriptLayout->addWidget(scriptHexDisplay);

    hexSubTabs->addTab(hexWonderCardTab, "WonderCard");
    hexSubTabs->addTab(hexScriptTab, "GREEN MAN");
    hexViewLayout->addWidget(hexSubTabs);

    // Add views to stacked widget
    viewStack->addWidget(textView);  // Index 0 = Text
    viewStack->addWidget(hexView);   // Index 1 = Hex

    dataGroupLayout->addWidget(viewStack);
    contentLayout->addWidget(textDataGroup);

    // Add stretch to push panels up
    contentLayout->addStretch();

    // Bottom area - Backup checkbox and Save Type selector
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(20, 5, 10, -3);  // Negative bottom margin to overlap gap

    bottomLayout->addStretch();

    backupCheckBox = new QCheckBox("Create Save Backup");
    backupCheckBox->setChecked(true);
    bottomLayout->addWidget(backupCheckBox);

    bottomLayout->addSpacing(10);

    saveTypeButton = new QPushButton("Not detected");
    saveTypeButton->setFlat(true);
    saveTypeButton->setEnabled(false); // Disabled until save is loaded
    saveTypeButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #909090; border: none; "
        "             font-weight: normal; font-style: italic; padding: 5px 10px; }"
        "QPushButton:enabled { cursor: pointer; }"
        "QPushButton:enabled:hover { color: #505050; text-decoration: underline; }"
    );
    connect(saveTypeButton, &QPushButton::clicked, this, &MainWindow::onSaveTypeClicked);
    bottomLayout->addWidget(saveTypeButton);

    vbox->addLayout(contentLayout, 1);
    vbox->addLayout(bottomLayout);

    // Ensure there's no spacing between central widget and status bar
    setContentsMargins(0, 0, 0, 0);
    if (layout()) {
        layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setSpacing(0);
    }
}

void MainWindow::createStatusBar()
{
    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->setSizeGripEnabled(false);
    statusBar->setContentsMargins(0, 0, 0, 0);
    statusBar->setFixedHeight(31);  // Increased to 31 to account for negative margin gap

    // Remove any layout margins from the status bar's internal layout
    if (statusBar->layout()) {
        statusBar->layout()->setContentsMargins(0, 0, 0, 0);
        statusBar->layout()->setSpacing(0);
    }

    // Create custom status bar widget
    QWidget *statusWidget = new QWidget();
    statusWidget->setObjectName("statusWidget");
    statusWidget->setStyleSheet(
        "QWidget#statusWidget { background-color: #4A90E2; "
        "border-bottom-left-radius: 10px; border-bottom-right-radius: 10px; margin: 0px; padding: 0px; }"
    );
    statusWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(20, 4, 20, 7);  // Asymmetric padding: less top, more bottom for better centering
    statusLayout->setSpacing(0);

    statusLabel = new QLabel("Status: No file loaded");
    statusLabel->setStyleSheet("font-weight: bold; color: white; background: transparent;");
    statusLayout->addWidget(statusLabel);

    statusLayout->addStretch();

    checksumLabel = new QLabel("Checksum: --");
    checksumLabel->setStyleSheet("font-weight: bold; color: white; background: transparent;");
    statusLayout->addWidget(checksumLabel);

    statusBar->addPermanentWidget(statusWidget, 1);
    statusBar->setStyleSheet(
        "QStatusBar { border: none; background-color: #4A90E2; padding: 0px; margin: 0px; "
        "border-bottom-left-radius: 10px; border-bottom-right-radius: 10px; } "
        "QStatusBar::item { border: none; padding: 0px; margin: 0px; }"
        );
}

// =============================================================================
// MOUSE EVENT HANDLERS (Frameless Window Dragging)
// =============================================================================

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // Enable window dragging by clicking and dragging on the title bar,
    // menu bar, or toolbar areas of the frameless window
    if (event->buttons() & Qt::LeftButton) {
        // Check if click is in a draggable area (title bar, menu bar, or toolbar)
        QPoint clickPos = event->pos();
        bool inDraggableArea = false;

        if (m_titleBar) {
            QRect titleRect = m_titleBar->geometry();
            // Translate to window coordinates
            titleRect.moveTopLeft(m_titleBar->mapTo(this, QPoint(0, 0)));
            if (titleRect.contains(clickPos)) {
                inDraggableArea = true;
            }
        }

        if (m_menuBar && !inDraggableArea) {
            QRect menuRect = m_menuBar->geometry();
            menuRect.moveTopLeft(m_menuBar->mapTo(this, QPoint(0, 0)));
            if (menuRect.contains(clickPos)) {
                inDraggableArea = true;
            }
        }

        if (m_toolBar && !inDraggableArea) {
            QRect toolRect = m_toolBar->geometry();
            toolRect.moveTopLeft(m_toolBar->mapTo(this, QPoint(0, 0)));
            if (toolRect.contains(clickPos)) {
                inDraggableArea = true;
            }
        }

        if (inDraggableArea) {
            m_dragging = true;
            QPoint global = event->globalPosition().toPoint();
            dragPosition = global - frameGeometry().topLeft();
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPoint global = event->globalPosition().toPoint();
        move(global - dragPosition);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
}

// =============================================================================
// PRESET AND TICKET SELECTION HANDLERS
// =============================================================================

void MainWindow::onPresetChanged(const QString &preset)
{
    // Handle preset dropdown changes - loads ticket data when selecting
    // a known preset, or clears for custom input
    if (preset == "Custom") {
        // Clear the display for custom input (only if editing is enabled)
        if (m_editingEnabled) {
            wonderCardVisualDisplay->clear();
        }
        return;
    }

    if (!m_ticketManager->isLoaded() || !m_saveFile->isLoaded()) {
        return;
    }

    // Get the full filename from combo box user data
    QString filename = presetCombo->currentData().toString();
    if (filename.isEmpty()) {
        filename = preset + ".bin";  // Fallback
    }

    // Find ticket matching the preset filename
    const QVector<TicketResource> &tickets = m_ticketManager->tickets();

    const TicketResource *selectedTicket = nullptr;
    for (const TicketResource &ticket : tickets) {
        if (ticket.wonderCardFile() == filename) {
            selectedTicket = &ticket;
            break;
        }
    }

    if (!selectedTicket) {
        // No matching ticket found
        wonderCardVisualDisplay->clear();
        statusLabel->setText("Ticket not found: " + filename);
        return;
    }

    // Load ticket data
    QString ticketsFolder = m_ticketManager->ticketsFolderPath();
    QString errorMessage;

    // Create a mutable copy to load data
    TicketResource ticketCopy = *selectedTicket;
    if (!ticketCopy.loadData(ticketsFolder, errorMessage)) {
        QMessageBox box(QMessageBox::Warning, "", "Failed to load ticket data:\n\n" + errorMessage, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Parse Wonder Card data
    WonderCardData wonderCard = MysteryGift::parseWonderCard(ticketCopy.wonderCardData());

    // Display the Wonder Card in the main display widget
    // Block signals to prevent triggering re-encoding before we set raw data
    wonderCardVisualDisplay->blockSignals(true);
    wonderCardVisualDisplay->setWonderCard(wonderCard);
    wonderCardVisualDisplay->blockSignals(false);

    // Set gift combo
    if (!wonderCard.subtitle.isEmpty()) {
        int giftIndex = giftCombo->findText(wonderCard.subtitle);
        if (giftIndex >= 0) {
            giftCombo->setCurrentIndex(giftIndex);
        }
    }

    // Store the loaded Wonder Card, raw data, and script for saving
    m_currentWonderCard = wonderCard;
    m_currentWonderCardRaw = ticketCopy.wonderCardData();
    m_currentScriptData = ticketCopy.scriptData();

    // Update icon controls based on loaded Wonder Card
    bool iconDisabled = (wonderCard.icon == 0xFFFF);
    iconDisabledCheck->blockSignals(true);
    iconDisabledCheck->setChecked(iconDisabled);
    iconDisabledCheck->blockSignals(false);

    speciesSpin->blockSignals(true);
    speciesSpin->setEnabled(!iconDisabled);
    if (!iconDisabled) {
        speciesSpin->setValue(wonderCard.icon > 412 ? 0 : wonderCard.icon);
    }
    speciesSpin->blockSignals(false);

    // Update background color control
    bgCombo->blockSignals(true);
    bgCombo->setCurrentIndex(wonderCard.color());
    bgCombo->blockSignals(false);

    // Update type control
    typeCombo->blockSignals(true);
    typeCombo->setCurrentIndex(static_cast<int>(wonderCard.type()));
    typeCombo->blockSignals(false);

    // Set the gift dropdown to match the item in the script
    if (!m_currentScriptData.isEmpty()) {
        uint16_t itemId = extractItemIdFromScript(m_currentScriptData);
        if (itemId != 0xFFFF) {
            // Find the item in the combo box by its user data
            giftCombo->blockSignals(true);
            for (int i = 0; i < giftCombo->count(); ++i) {
                if (giftCombo->itemData(i).toInt() == itemId) {
                    giftCombo->setCurrentIndex(i);
                    break;
                }
            }
            giftCombo->blockSignals(false);
        }
    }

    // Update both WonderCard and Script tabs
    updateWonderCardTabs();
    updateScriptTabs();
}

void MainWindow::onSaveTypeClicked()
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #b0b0b0; color: #000000; }"
        "QMenu::item { padding: 8px 30px; color: #000000; }"
        "QMenu::item:selected { background-color: #e0e0e0; color: #000000; }"
    );

    QAction *emeraldAction = menu.addAction("Pokémon Emerald");
    QAction *rubySapphireAction = menu.addAction("Pokémon Ruby/Sapphire");
    QAction *fireRedLeafGreenAction = menu.addAction("Pokémon FireRed/LeafGreen");

    connect(emeraldAction, &QAction::triggered, [this]() {
        saveTypeButton->setText("Pokémon Emerald");
    });
    connect(rubySapphireAction, &QAction::triggered, [this]() {
        saveTypeButton->setText("Pokémon Ruby/Sapphire");
    });
    connect(fireRedLeafGreenAction, &QAction::triggered, [this]() {
        saveTypeButton->setText("Pokémon FireRed/LeafGreen");
    });

    // Show menu at button position
    menu.exec(saveTypeButton->mapToGlobal(QPoint(0, saveTypeButton->height())));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Title bar dragging is handled here
    return QMainWindow::eventFilter(obj, event);
}

// =============================================================================
// FILE OPERATIONS - Open, Save, Load
// =============================================================================

void MainWindow::onOpenClicked()
{
    // Open a Pokemon Generation III save file (.sav) and extract any
    // existing Wonder Card data for display
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Pokemon Save File",
        QString(),
        "Pokemon Save Files (*.sav)"
    );

    // User cancelled
    if (filePath.isEmpty()) {
        return;
    }

    // Disable toolbar buttons during load
    openAction->setEnabled(false);
    saveAction->setEnabled(false);
    saveAsAction->setEnabled(false);

    // Update status bar
    statusLabel->setText("Status: Loading...");
    checksumLabel->setText("Checksum: --");

    // Force UI update
    QApplication::processEvents();

    // Load the file
    QString errorMessage;
    bool success = m_saveFile->loadFromFile(filePath, errorMessage);

    // Re-enable toolbar buttons
    openAction->setEnabled(true);

    if (success) {
        // Update file path display (elided left with full path in tooltip)
        setFilePathDisplay(filePath);

        // Update save type button
        QString gameTypeStr = m_saveFile->gameTypeToString(m_saveFile->detectedGame());
        saveTypeButton->setText(gameTypeStr);
        saveTypeButton->setEnabled(true);
        saveTypeButton->setStyleSheet(
            "QPushButton { background-color: transparent; color: #000000; border: none; "
            "             font-weight: normal; font-style: italic; padding: 5px 10px; }"
            "QPushButton:enabled { cursor: pointer; }"
            "QPushButton:enabled:hover { color: #505050; text-decoration: underline; }"
        );

        // Update checksum status
        if (m_saveFile->checksumValid()) {
            checksumLabel->setText("Checksum: Valid");
            checksumLabel->setStyleSheet("font-weight: bold; color: #90EE90; background: transparent;"); // Light green
        } else {
            checksumLabel->setText("Checksum: Invalid");
            checksumLabel->setStyleSheet("font-weight: bold; color: #FFB6C1; background: transparent;"); // Light red
        }

        // Update status
        statusLabel->setText("Status: File loaded successfully");

        // Enable file-dependent actions
        closeAction->setEnabled(true);
        saveAction->setEnabled(true);
        saveAsAction->setEnabled(true);
        editButton->setEnabled(true);
        editEnableAction->setEnabled(true);
        clearWcAction->setEnabled(true);
        enableMgFlagAction->setEnabled(true);
        validateChecksumsAction->setEnabled(true);

        // Try to extract Wonder Card data
        if (m_saveFile->hasWonderCard()) {
            QString wcError, scriptError, rawError;

            WonderCardData wonderCard = m_saveFile->extractWonderCard(wcError);
            QByteArray wonderCardRaw = m_saveFile->extractWonderCardRaw(rawError);
            QByteArray scriptData = m_saveFile->extractScript(scriptError);

            if (wcError.isEmpty() && !wonderCard.isEmpty()) {
                m_currentWonderCard = wonderCard;
                m_currentWonderCardRaw = wonderCardRaw;
                m_currentScriptData = scriptData;

                // Set the gift dropdown to match the item in the script
                if (!m_currentScriptData.isEmpty()) {
                    uint16_t itemId = extractItemIdFromScript(m_currentScriptData);
                    if (itemId != 0xFFFF) {
                        giftCombo->blockSignals(true);
                        for (int i = 0; i < giftCombo->count(); ++i) {
                            if (giftCombo->itemData(i).toInt() == itemId) {
                                giftCombo->setCurrentIndex(i);
                                break;
                            }
                        }
                        giftCombo->blockSignals(false);
                    }
                }

                // Try to identify the ticket by comparing with known tickets
                const TicketResource* matchedTicket = m_ticketManager->findTicketByWonderCard(
                    wonderCardRaw, m_saveFile->detectedGame());

                displayWonderCard(wonderCard, matchedTicket);

                // Enable export action since we have Wonder Card data
                exportWcAction->setEnabled(true);
            } else {
                // Save has no Wonder Card or extraction failed
                clearWonderCardDisplay();
                exportWcAction->setEnabled(false);
            }
        } else {
            // No Wonder Card in save
            clearWonderCardDisplay();
            exportWcAction->setEnabled(false);
        }

        // Reset edit state when loading a new file
        resetEditState();

    } else {
        // Show error dialog
        QMessageBox box(QMessageBox::Critical, "", "Failed to load save file:\n\n" + errorMessage, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();

        // Reset UI to unloaded state
        filePathDisplay->setText("<span style='color: #909090; font-style: italic;'>No file loaded</span>");
        filePathDisplay->setToolTip("");
        saveTypeButton->setText("Not detected");
        saveTypeButton->setEnabled(false);
        saveTypeButton->setStyleSheet(
            "QPushButton { background-color: transparent; color: #909090; border: none; "
            "             font-weight: normal; font-style: italic; padding: 5px 10px; }"
            "QPushButton:enabled { cursor: pointer; }"
            "QPushButton:enabled:hover { color: #505050; text-decoration: underline; }"
        );
        statusLabel->setText("Status: No file loaded");
        checksumLabel->setText("Checksum: --");
        checksumLabel->setStyleSheet("font-weight: bold; color: white; background: transparent;");
    }
}

void MainWindow::onEditButtonClicked()
{
    // Show confirmation dialog with light theme (frameless)
    QMessageBox msgBox(this);
    msgBox.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    msgBox.setText("You are about to enable Wonder Card editing.\n\n"
                   "Changes you make will modify the Wonder Card data "
                   "that will be injected into your save file.");
    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #f5f5f5; }"
        "QMessageBox QLabel { color: #000000; }"
        "QPushButton { background-color: #e0e0e0; color: #000000; "
        "              border: 1px solid #a0a0a0; border-radius: 4px; "
        "              padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background-color: #d0d0d0; }"
        "QPushButton:pressed { background-color: #c0c0c0; }"
    );

    if (msgBox.exec() == QMessageBox::Ok) {
        enableEditing();
    }
}

void MainWindow::enableEditing()
{
    m_editingEnabled = true;

    // Update button and menu action state
    editButton->setText("Editing");
    editButton->setEnabled(false);
    editEnableAction->setText("Editing Enabled");
    editEnableAction->setEnabled(false);

    // Update preview label
    previewLabel->setText("Editor (Click fields to edit):");
    previewLabel->setStyleSheet("font-weight: bold; color: #4A90E2; margin-bottom: 2px;");

    // Enable editing on the widget
    wonderCardVisualDisplay->setReadOnly(false);
    wonderCardVisualDisplay->setStyleSheet(
        "background-color: #e0e0e0; border: 2px solid #4A90E2; border-radius: 3px;"
    );

    // Enable Wonder Card controls
    bgCombo->setEnabled(true);
    iconDisabledCheck->setEnabled(true);
    speciesSpin->setEnabled(!iconDisabledCheck->isChecked());  // Depends on checkbox
    typeCombo->setEnabled(true);

    // Enable preset/gift dropdowns if tickets are loaded (works with or without ROM)
    presetCombo->setEnabled(m_ticketManager->isLoaded());
    giftCombo->setEnabled(m_ticketManager->isLoaded());

    // Update status bar
    statusLabel->setText("Editing enabled - click fields to modify");
}

void MainWindow::resetEditState()
{
    m_editingEnabled = false;

    // Reset button and menu action state (only enable if a save file is loaded)
    editButton->setText("Edit");
    editButton->setEnabled(m_saveFile->isLoaded());
    editEnableAction->setText("Enable Editing");
    editEnableAction->setEnabled(m_saveFile->isLoaded());

    // Reset preview label
    previewLabel->setText("Preview (Read-only):");
    previewLabel->setStyleSheet("font-weight: bold; color: #505050; margin-bottom: 2px;");

    // Disable editing on the widget
    wonderCardVisualDisplay->setReadOnly(true);
    wonderCardVisualDisplay->setStyleSheet(
        "background-color: #e0e0e0; border: 1px solid #b8b8b8; border-radius: 3px;"
    );

    // Disable Wonder Card controls
    bgCombo->setEnabled(false);
    speciesSpin->setEnabled(false);
    iconDisabledCheck->setEnabled(false);
    typeCombo->setEnabled(false);

    // Disable preset/gift dropdowns
    presetCombo->setEnabled(false);
    giftCombo->setEnabled(false);
}

void MainWindow::loadTickets()
{
    // Get application directory
    QString appDir = QCoreApplication::applicationDirPath();
    QString ticketsFolder = appDir + "/Tickets";

    // Load tickets from folder
    QString errorMessage;
    bool success = m_ticketManager->loadFromFolder(ticketsFolder, errorMessage);

    if (!success) {
        // Show warning dialog
        QMessageBox box(QMessageBox::Warning, "", "Failed to load Mystery Gift tickets:\n\n" + errorMessage +
            "\n\nThe application will run with limited functionality.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Populate dropdowns
    populatePresetDropdown();
}

void MainWindow::populatePresetDropdown()
{
    // Clear existing items (except "Custom" placeholder if present)
    presetCombo->clear();

    if (!m_ticketManager->isLoaded()) {
        presetCombo->addItem("No tickets loaded");
        presetCombo->setEnabled(false);
        return;
    }

    // Add default "(No Wonder Card)" option at the start
    presetCombo->addItem("(No Wonder Card)");

    // Get all tickets
    const QVector<TicketResource> &tickets = m_ticketManager->tickets();

    // Add ticket display names to dropdown
    for (const TicketResource &ticket : tickets) {
        // Store full filename as user data for matching
        presetCombo->addItem(ticket.name(), ticket.wonderCardFile());
    }

    // Add "Custom" option
    presetCombo->addItem("Custom");

    // Keep disabled - only enable when ROM loaded AND edit mode active
    // (controlled by enableEditing() and resetEditState())
}

void MainWindow::displayWonderCard(const WonderCardData &wonderCard,
                                   const TicketResource *matchedTicket)
{
    // Update activated checkbox
    activatedCheckBox1->setChecked(!wonderCard.isEmpty());

    // Update Wonder Card visual controls (block signals to prevent feedback loops)
    bgCombo->blockSignals(true);
    bgCombo->setCurrentIndex(wonderCard.color());
    bgCombo->blockSignals(false);

    // Set icon controls based on value (0xFFFF = disabled/none)
    bool iconDisabled = (wonderCard.icon == 0xFFFF);
    iconDisabledCheck->blockSignals(true);
    iconDisabledCheck->setChecked(iconDisabled);
    iconDisabledCheck->blockSignals(false);

    speciesSpin->blockSignals(true);
    if (!iconDisabled) {
        speciesSpin->setValue(wonderCard.icon > 412 ? 0 : wonderCard.icon);
    }
    speciesSpin->blockSignals(false);

    typeCombo->blockSignals(true);
    typeCombo->setCurrentIndex(static_cast<int>(wonderCard.type()));
    typeCombo->blockSignals(false);

    // Update both WonderCard and Script tabs
    updateWonderCardTabs();
    updateScriptTabs();

    // Set preset combo based on matched ticket
    presetCombo->blockSignals(true);
    if (wonderCard.isEmpty()) {
        // No Wonder Card data - select default option
        int emptyIndex = presetCombo->findText("(No Wonder Card)");
        if (emptyIndex >= 0) {
            presetCombo->setCurrentIndex(emptyIndex);
        } else {
            presetCombo->setCurrentIndex(0);
        }
    } else if (matchedTicket) {
        // Found a matching known ticket
        int presetIndex = presetCombo->findText(matchedTicket->name());
        if (presetIndex >= 0) {
            presetCombo->setCurrentIndex(presetIndex);
        } else {
            presetCombo->setCurrentText(matchedTicket->name());
        }
    } else {
        // No matching ticket found - show as Unknown Wonder Card
        int unknownIndex = presetCombo->findText("Unknown Wonder Card");
        if (unknownIndex >= 0) {
            presetCombo->setCurrentIndex(unknownIndex);
        } else {
            // Add Unknown Wonder Card option if not present
            presetCombo->addItem("Unknown Wonder Card");
            presetCombo->setCurrentText("Unknown Wonder Card");
        }
    }
    presetCombo->blockSignals(false);

    // Set gift combo to subtitle
    int giftIndex = giftCombo->findText(wonderCard.subtitle);
    if (giftIndex >= 0) {
        giftCombo->setCurrentIndex(giftIndex);
    }
}

void MainWindow::clearWonderCardDisplay()
{
    // Clear activated checkbox
    activatedCheckBox1->setChecked(false);

    // Reset Wonder Card visual controls
    bgCombo->blockSignals(true);
    bgCombo->setCurrentIndex(0);
    bgCombo->blockSignals(false);

    iconDisabledCheck->blockSignals(true);
    iconDisabledCheck->setChecked(false);
    iconDisabledCheck->blockSignals(false);

    speciesSpin->blockSignals(true);
    speciesSpin->setValue(0);
    speciesSpin->blockSignals(false);

    typeCombo->blockSignals(true);
    typeCombo->setCurrentIndex(0);
    typeCombo->blockSignals(false);

    // Clear WonderCard tab displays
    wonderCardVisualDisplay->clear();
    wonderCardHexDisplay->clear();
    wonderCardHexDisplay->setPlaceholderText("No WonderCard hex data");

    // Clear Script tab displays
    scriptTextDisplay->clear();
    scriptHexDisplay->clear();

    // Reset preset combo to "(No Wonder Card)" default
    presetCombo->blockSignals(true);
    int emptyIndex = presetCombo->findText("(No Wonder Card)");
    if (emptyIndex >= 0) {
        presetCombo->setCurrentIndex(emptyIndex);
    } else if (presetCombo->count() > 0) {
        presetCombo->setCurrentIndex(0);
    }
    presetCombo->blockSignals(false);
    if (giftCombo->count() > 0) {
        giftCombo->setCurrentIndex(0);
    }

    // Clear stored data
    m_currentWonderCard = WonderCardData();
    m_currentScriptData.clear();
    m_currentWonderCardRaw.clear();
}

void MainWindow::onSaveClicked()
{
    if (!m_saveFile->isLoaded()) {
        QMessageBox box(QMessageBox::Warning, "", "Please load a save file first.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Use original file path
    QString savePath = m_saveFile->filePath();
    bool makeBackup = backupCheckBox->isChecked();

    // Perform injection and save
    performSave(savePath, makeBackup);
}

void MainWindow::onSaveAsClicked()
{
    if (!m_saveFile->isLoaded()) {
        QMessageBox box(QMessageBox::Warning, "", "Please load a save file first.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Show save file dialog
    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Save Pokemon Save File",
        m_saveFile->filePath(),
        "Pokemon Save Files (*.sav)"
    );

    if (savePath.isEmpty()) {
        return;  // User cancelled
    }

    bool makeBackup = backupCheckBox->isChecked();

    // Perform injection and save
    performSave(savePath, makeBackup);
}

void MainWindow::onOpenTileViewer()
{
    if (!m_romLoaded) {
        QMessageBox box(QMessageBox::Warning, "", "Please ensure a GBA ROM is loaded first.\n\n"
                             "The ROM should be loaded automatically from:\n" + m_romPath, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Create a new ROM reader for the tile viewer
    GBAROReader *rom = new GBAROReader();
    QString error;

    if (!rom->loadROM(m_romPath, error)) {
        QMessageBox box(QMessageBox::Warning, "", error, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        delete rom;
        return;
    }

    TileViewer *viewer = new TileViewer(rom, this);
    viewer->setAttribute(Qt::WA_DeleteOnClose);  // Auto-delete when closed
    viewer->show();
}

void MainWindow::updateWonderCardTabs()
{
    // Update WonderCard visual display
    // Block signals to prevent onEditableWonderCardChanged from overwriting
    // m_currentWonderCardRaw with re-encoded data when loading presets
    if (!m_currentWonderCard.isEmpty()) {
        wonderCardVisualDisplay->blockSignals(true);
        wonderCardVisualDisplay->setWonderCard(m_currentWonderCard);
        wonderCardVisualDisplay->blockSignals(false);
    } else {
        wonderCardVisualDisplay->clear();
    }

    // Update WonderCard Hex tab
    if (!m_currentWonderCardRaw.isEmpty()) {
        QString wcHex = formatHexDump(m_currentWonderCardRaw);
        wonderCardHexDisplay->setPlainText(wcHex);
    } else {
        wonderCardHexDisplay->clear();
        wonderCardHexDisplay->setPlaceholderText("No WonderCard hex data");
    }
}

void MainWindow::updateScriptTabs()
{
    // Update Script Text tab with disassembled output
    if (!m_currentScriptData.isEmpty()) {
        // Skip 4-byte CRC header if present (1004-byte format vs 1000-byte payload)
        QByteArray scriptPayload = m_currentScriptData;
        if (scriptPayload.size() == TicketResource::SCRIPT_SIZE) {
            scriptPayload = scriptPayload.mid(TicketResource::SCRIPT_HEADER_SIZE);
        }

        if (m_scriptDisassembler->isReady()) {
            // Pass ROM reader for name resolution if available
            if (wonderCardVisualDisplay && wonderCardVisualDisplay->isROMLoaded()) {
                GBAROReader *reader = wonderCardVisualDisplay->getRomReader();
                m_scriptDisassembler->setRomReader(reader);
                qDebug() << "updateScriptTabs: ROM loaded, hasNameTables=" << (reader ? reader->hasNameTables() : false);
            } else {
                m_scriptDisassembler->setRomReader(nullptr);
                qDebug() << "updateScriptTabs: No ROM loaded";
            }

            // Disassemble as RamScript (includes header parsing)
            QString disassembly = m_scriptDisassembler->disassembleRamScript(
                scriptPayload,
                true,   // showComments
                true,   // showBytes
                true    // showOffsets
            );
            scriptTextDisplay->setPlainText(disassembly);
        } else {
            // Fall back to simple text decoding if disassembler not ready
            QString scriptText = MysteryGift::decodeText(
                reinterpret_cast<const uint8_t*>(scriptPayload.constData()),
                scriptPayload.size()
            );

            if (!scriptText.isEmpty()) {
                scriptTextDisplay->setPlainText(scriptText);
            } else {
                scriptTextDisplay->setPlainText("(Script contains no decodable text)");
            }
        }
    } else {
        scriptTextDisplay->clear();
        scriptTextDisplay->setPlaceholderText("No script data");
    }

    // Update Script Hex tab
    if (!m_currentScriptData.isEmpty()) {
        QString scriptHex = formatHexDump(m_currentScriptData);
        scriptHexDisplay->setPlainText(scriptHex);
    } else {
        scriptHexDisplay->clear();
        scriptHexDisplay->setPlaceholderText("No script hex data");
    }
}

QString MainWindow::formatHexDump(const QByteArray &data)
{
    QString result;
    const int bytesPerLine = 16;
    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(data.constData());

    for (int i = 0; i < data.size(); i += bytesPerLine) {
        // Offset column
        result += QString("%1:  ").arg(i, 4, 16, QChar('0')).toUpper();

        // Hex bytes
        for (int j = 0; j < bytesPerLine; ++j) {
            if (i + j < data.size()) {
                result += QString("%1 ").arg(bytes[i + j], 2, 16, QChar('0')).toUpper();
            } else {
                result += "   ";  // Padding for incomplete lines
            }

            // Extra space after 8 bytes for readability
            if (j == 7) result += " ";
        }

        // ASCII representation
        result += " |";
        for (int j = 0; j < bytesPerLine && (i + j) < data.size(); ++j) {
            uint8_t byte = bytes[i + j];
            if (byte >= 32 && byte <= 126) {
                result += QChar(byte);
            } else {
                result += '.';
            }
        }
        result += "|\n";
    }

    return result;
}

void MainWindow::onEditableWonderCardChanged(const WonderCardData &wonderCard)
{
    // Update the stored Wonder Card data
    m_currentWonderCard = wonderCard;

    // Update hex display
    QByteArray encoded = MysteryGift::encodeWonderCard(wonderCard);
    m_currentWonderCardRaw = encoded;
    QString wcHex = formatHexDump(encoded);
    wonderCardHexDisplay->setPlainText(wcHex);
}

void MainWindow::onEditableFieldSelected(const QString &fieldName)
{
    statusLabel->setText(QString("Editing: %1").arg(fieldName));
}

void MainWindow::onEditableStatusUpdate(const QString &fieldName, int byteCount, int maxBytes)
{
    statusLabel->setText(QString("Editing %1: %2/%3 bytes").arg(fieldName).arg(byteCount).arg(maxBytes));
}

void MainWindow::performSave(const QString &savePath, bool makeBackup)
{
    // Use the currently loaded script data (from preset or save file)
    // m_currentScriptData is populated when:
    // 1. Loading a save file with existing Wonder Card
    // 2. Selecting a ticket preset from dropdown

    QString errorMessage;

    // Check if Mystery Gift flag is enabled
    if (!m_saveFile->isMysteryGiftEnabled()) {
        // Flag is not set - prompt user for action
        QMessageBox msgBox(this);
        msgBox.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        msgBox.setText("The Mystery Gift feature is not enabled in this save file.");
        msgBox.setInformativeText(
            "For the Wonder Card to appear in-game, Mystery Gift must be enabled.\n\n"
            "This is normally done by completing the in-game \"Mystery Gift\" unlock procedure, "
            "but it can also be enabled directly in the save data.\n\n"
            "What would you like to do?");
        msgBox.setIcon(QMessageBox::Question);

        QPushButton *enableBtn = msgBox.addButton("Enable Mystery Gift", QMessageBox::AcceptRole);
        QPushButton *skipBtn = msgBox.addButton("Skip (Continue Anyway)", QMessageBox::RejectRole);
        QPushButton *cancelBtn = msgBox.addButton("Cancel", QMessageBox::DestructiveRole);

        msgBox.setDefaultButton(enableBtn);
        msgBox.exec();

        if (msgBox.clickedButton() == cancelBtn) {
            return;  // User cancelled the operation
        } else if (msgBox.clickedButton() == enableBtn) {
            // User wants to enable the flag
            if (!m_saveFile->enableMysteryGift(errorMessage)) {
                QMessageBox box(QMessageBox::Warning, "", "Could not enable Mystery Gift flag:\n\n" + errorMessage +
                    "\n\nInjection will continue, but the card may not appear in-game.", QMessageBox::Ok, this);
                box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
                box.exec();
            }
        }
        // If skipBtn, just continue without enabling
    }

    // Show injection options dialog
    // (Mirrors game's ClearSavedWonderCardAndRelated behavior)
    QDialog optionsDialog(this);
    optionsDialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    optionsDialog.setModal(true);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&optionsDialog);

    QLabel *infoLabel = new QLabel(
        "The following options control what data is cleared when injecting.\n"
        "These mirror the game's behavior when legitimately receiving a Wonder Card.");
    infoLabel->setWordWrap(true);
    dialogLayout->addWidget(infoLabel);

    dialogLayout->addSpacing(10);

    // Checkboxes for options
    QCheckBox *clearMetadataCheck = new QCheckBox("Clear Wonder Card Metadata (recommended)");
    clearMetadataCheck->setToolTip("Zeros battlesWon, battlesLost, numTrades, stampData.\nAlways done by the game when saving a new Wonder Card.");
    clearMetadataCheck->setChecked(true);
    dialogLayout->addWidget(clearMetadataCheck);

    QCheckBox *clearTrainerIdsCheck = new QCheckBox("Clear Trainer IDs");
    clearTrainerIdsCheck->setToolTip("Clears saved trainer IDs for Mystery Gift battles/trades.\n10 IDs total (5 battles, 5 trades).");
    clearTrainerIdsCheck->setChecked(false);
    dialogLayout->addWidget(clearTrainerIdsCheck);

    QCheckBox *clearFlagsCheck = new QCheckBox("Clear Mystery Gift Flags (not yet implemented)");
    clearFlagsCheck->setToolTip("Clears Mystery Gift event flags.");
    clearFlagsCheck->setChecked(false);
    clearFlagsCheck->setEnabled(false);  // Not implemented yet
    dialogLayout->addWidget(clearFlagsCheck);

    QCheckBox *clearVarsCheck = new QCheckBox("Clear Mystery Gift Vars (not yet implemented)");
    clearVarsCheck->setToolTip("Clears Mystery Gift variables.");
    clearVarsCheck->setChecked(false);
    clearVarsCheck->setEnabled(false);  // Not implemented yet
    dialogLayout->addWidget(clearVarsCheck);

    dialogLayout->addSpacing(10);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *yesBtn = new QPushButton("Inject");
    QPushButton *noBtn = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(yesBtn);
    buttonLayout->addWidget(noBtn);
    dialogLayout->addLayout(buttonLayout);

    connect(yesBtn, &QPushButton::clicked, &optionsDialog, &QDialog::accept);
    connect(noBtn, &QPushButton::clicked, &optionsDialog, &QDialog::reject);

    if (optionsDialog.exec() != QDialog::Accepted) {
        return;  // User cancelled
    }

    // Build options struct from checkboxes
    InjectionOptions options;
    options.clearMetadata = clearMetadataCheck->isChecked();
    options.clearTrainerIds = clearTrainerIdsCheck->isChecked();
    options.clearMysteryGiftFlags = clearFlagsCheck->isChecked();
    options.clearMysteryGiftVars = clearVarsCheck->isChecked();

    // Inject Wonder Card (pass raw data to avoid re-encoding artifacts)
    bool injected = m_saveFile->injectWonderCard(
        m_currentWonderCard,
        m_currentScriptData,
        m_ticketManager->crcTable(),
        errorMessage,
        m_currentWonderCardRaw,  // Use original raw data when available
        options
    );

    if (!injected) {
        QMessageBox box(QMessageBox::Critical, "", "Failed to inject Wonder Card:\n\n" + errorMessage, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Save to file
    bool saved = m_saveFile->saveToFile(savePath, makeBackup, errorMessage);

    if (saved) {
        QMessageBox box(QMessageBox::Information, "", "Wonder Card injected and save file written successfully!", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();

        // Update file path display if saved to new location
        if (savePath != m_saveFile->filePath()) {
            setFilePathDisplay(savePath);
        }
    } else {
        QMessageBox box(QMessageBox::Critical, "", "Failed to save file:\n\n" + errorMessage, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
    }
}

// =============================================================================
// ROM DATABASE AND GRAPHICS LOADING
// =============================================================================

void MainWindow::loadRomDatabase()
{
    // Load ROM identification database from embedded Qt resource.
    // This YAML file contains MD5 checksums and offset data for supported ROMs.
    QString yamlPath = ":/Resources/gen3_rom_data.yaml";

    QString error;
    if (!m_romDatabase->loadFromYaml(yamlPath, error)) {
        qWarning() << "Failed to load ROM database:" << error;
        // Continue without database - will use fallback graphics
    } else {
        qDebug() << "ROM database loaded successfully from embedded resource";
    }
}

void MainWindow::loadROM()
{
    qDebug() << "MainWindow::loadROM() starting...";

    // Search for ROM starting from working directory (recursively checks subdirectories)
    QString searchDir = QDir::currentPath();
    qDebug() << "Searching for ROM in:" << searchDir;

    // Use RomLoader for automatic ROM discovery (searches recursively)
    RomLoader loader;
    RomLoader::RomSearchResult result = loader.findRom(searchDir, m_romDatabase);

    // If not found, also try application directory (for deployed builds)
    if (!result.found) {
        QString appDir = QCoreApplication::applicationDirPath();
        if (appDir != searchDir) {
            qDebug() << "Also checking application directory:" << appDir;
            result = loader.findRom(appDir, m_romDatabase);
        }
    }

    if (result.found) {
        // ROM found and identified
        m_romPath = result.path;
        m_romVersionName = result.versionName;

        QString error;
        if (wonderCardVisualDisplay->loadROM(m_romPath, error)) {
            m_romLoaded = true;
            m_useFallbackGraphics = false;
            statusLabel->setText(QString("ROM: %1").arg(m_romVersionName));
            qDebug() << "ROM loaded successfully:" << m_romVersionName;
            populateGiftDropdown();
        } else {
            qWarning() << "Failed to load ROM graphics:" << error;
            m_useFallbackGraphics = true;
            statusLabel->setText("ROM found but graphics failed - using fallback");
            populateGiftDropdown();  // Populate with fallback item IDs
        }
    } else {
        // No ROM found - offer manual selection or use fallback
        qDebug() << "No ROM found automatically:" << result.errorMessage;
        promptForROM();
    }

    qDebug() << "MainWindow::loadROM() completed";
}

void MainWindow::promptForROM()
{
    QMessageBox msgBox(this);
    msgBox.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    msgBox.setText("No Pokemon Gen3 ROM was found in the application directory.\n\n"
                   "For authentic Wonder Card graphics and fonts, place a Pokemon "
                   "FireRed, LeafGreen, or Emerald ROM (.gba file) in:\n" +
                   QCoreApplication::applicationDirPath() + "\n\n"
                   "Would you like to select a ROM file manually, or continue with fallback graphics?");
    QPushButton *selectBtn = msgBox.addButton("Select ROM...", QMessageBox::AcceptRole);
    QPushButton *fallbackBtn = msgBox.addButton("Use Fallback", QMessageBox::ActionRole);
    QPushButton *cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    Q_UNUSED(cancelBtn);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #f5f5f5; }"
        "QMessageBox QLabel { color: #000000; }"
        "QPushButton { background-color: #e0e0e0; color: #000000; "
        "              border: 1px solid #a0a0a0; border-radius: 4px; "
        "              padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background-color: #d0d0d0; }"
        "QPushButton:pressed { background-color: #c0c0c0; }"
    );

    msgBox.exec();

    if (msgBox.clickedButton() == selectBtn) {
        QString romPath = QFileDialog::getOpenFileName(
            this,
            "Select Pokemon GBA ROM",
            QString(),
            "GBA ROM Files (*.gba);;All Files (*)"
        );

        if (!romPath.isEmpty()) {
            // Validate the selected ROM
            QString md5 = RomLoader::computeMD5(romPath);
            const RomDatabase::RomVersion *version = m_romDatabase->identifyRom(md5);

            if (version) {
                m_romPath = romPath;
                m_romVersionName = version->name;

                QString error;
                if (wonderCardVisualDisplay->loadROM(m_romPath, error)) {
                    m_romLoaded = true;
                    m_useFallbackGraphics = false;
                    statusLabel->setText(QString("ROM: %1").arg(m_romVersionName));
                    populateGiftDropdown();
                } else {
                    m_useFallbackGraphics = true;
                    statusLabel->setText("ROM load failed - using fallback graphics");
                    populateGiftDropdown();  // Populate with fallback item IDs
                }
            } else {
                QMessageBox::warning(this, "Unknown ROM",
                    QString("The selected ROM is not recognized.\n\n"
                            "MD5: %1\n\n"
                            "Supported ROMs:\n"
                            "- Pokemon FireRed (USA) 1.0 / 1.1\n"
                            "- Pokemon LeafGreen (USA) 1.0 / 1.1\n"
                            "- Pokemon Emerald (USA)\n\n"
                            "The application will use fallback graphics.").arg(md5));
                m_useFallbackGraphics = true;
                statusLabel->setText("Unknown ROM - using fallback graphics");
                populateGiftDropdown();  // Populate with fallback item IDs
            }
        } else {
            // User cancelled file dialog
            m_useFallbackGraphics = true;
            statusLabel->setText("No ROM - using fallback graphics");
            populateGiftDropdown();  // Populate with fallback item IDs
        }
    } else if (msgBox.clickedButton() == fallbackBtn) {
        // Use fallback graphics
        m_useFallbackGraphics = true;
        m_romLoaded = false;
        statusLabel->setText("Fallback mode - no ROM loaded");
        qDebug() << "User chose fallback graphics";

        // Load fallback graphics into the widget
        QString error;
        wonderCardVisualDisplay->loadFallbackGraphics(error);
        populateGiftDropdown();  // Populate with fallback item IDs
    } else {
        // Cancel - still use fallback but don't show message
        m_useFallbackGraphics = true;
        statusLabel->setText("No ROM loaded");
        populateGiftDropdown();  // Populate with fallback item IDs
    }
}

// =============================================================================
// WONDER CARD CONTROL HANDLERS
// =============================================================================

void MainWindow::onBgComboChanged(int index)
{
    // Update the Wonder Card background color/pattern (0-7)
    wonderCardVisualDisplay->setBackgroundIndex(index);

    // Update the current Wonder Card data
    // The typeColorResend byte stores: bits 0-1 = type, bits 2-4 = color
    uint8_t typeVal = m_currentWonderCard.typeColorResend & 0x03;
    uint8_t resendFlag = m_currentWonderCard.typeColorResend & 0x40;
    m_currentWonderCard.typeColorResend = typeVal | ((index & 0x07) << 2) | resendFlag;

    // Clear raw data to force re-encoding from struct on save
    m_currentWonderCardRaw.clear();
}

void MainWindow::onSpeciesSpinChanged(int value)
{
    // Only update if icon is not disabled
    if (!iconDisabledCheck->isChecked()) {
        wonderCardVisualDisplay->setIconSpecies(value);
        m_currentWonderCard.icon = static_cast<uint16_t>(value);

        // Clear raw data to force re-encoding from struct on save
        m_currentWonderCardRaw.clear();
    }
}

void MainWindow::onIconDisabledToggled(bool checked)
{
    speciesSpin->setEnabled(!checked);

    if (checked) {
        // Set icon to 0xFFFF (no icon)
        m_currentWonderCard.icon = 0xFFFF;
        wonderCardVisualDisplay->setIconSpecies(0xFFFF);
    } else {
        // Restore to spinbox value
        int value = speciesSpin->value();
        m_currentWonderCard.icon = static_cast<uint16_t>(value);
        wonderCardVisualDisplay->setIconSpecies(value);
    }

    // Clear raw data to force re-encoding from struct on save
    m_currentWonderCardRaw.clear();
}

void MainWindow::onTypeComboChanged(int index)
{
    // Update the current Wonder Card data
    // The typeColorResend byte stores: bits 0-1 = type, bits 2-4 = color
    uint8_t colorVal = (m_currentWonderCard.typeColorResend >> 2) & 0x07;
    uint8_t resendFlag = m_currentWonderCard.typeColorResend & 0x40;
    m_currentWonderCard.typeColorResend = (index & 0x03) | (colorVal << 2) | resendFlag;

    // Clear raw data to force re-encoding from struct on save
    m_currentWonderCardRaw.clear();
}

void MainWindow::onViewModeChanged(bool hexMode)
{
    // Update UI button states
    textViewBtn->setChecked(!hexMode);
    hexViewBtn->setChecked(hexMode);

    // Update menu action states
    textViewAction->setChecked(!hexMode);
    hexViewAction->setChecked(hexMode);

    // Switch the stacked widget
    viewStack->setCurrentIndex(hexMode ? 1 : 0);

    // Sync the sub-tab selection between views
    if (hexMode) {
        hexSubTabs->setCurrentIndex(textSubTabs->currentIndex());
    } else {
        textSubTabs->setCurrentIndex(hexSubTabs->currentIndex());
    }
}

void MainWindow::populateGiftDropdown()
{
    // Block signals while populating to avoid triggering onGiftComboChanged
    giftCombo->blockSignals(true);
    giftCombo->clear();

    GBAROReader *reader = wonderCardVisualDisplay->getRomReader();
    if (reader && reader->hasNameTables()) {
        // ROM loaded - use actual item names
        int itemCount = reader->getItemCount();
        qDebug() << "Populating gift dropdown with" << itemCount << "items from ROM";

        for (int i = 0; i < itemCount; ++i) {
            QString name = reader->getItemName(static_cast<uint16_t>(i));
            if (name.isEmpty()) {
                name = QString("ITEM_0x%1").arg(i, 4, 16, QChar('0')).toUpper();
            }
            // Store item ID as user data
            giftCombo->addItem(name, QVariant(i));
        }
    } else {
        // Fallback mode - populate with generic item IDs
        const int FALLBACK_ITEM_COUNT = 377;  // Max items in Gen3 (Emerald)
        qDebug() << "Populating gift dropdown with" << FALLBACK_ITEM_COUNT << "fallback items";

        for (int i = 0; i < FALLBACK_ITEM_COUNT; ++i) {
            QString name = QString("ITEM_0x%1").arg(i, 4, 16, QChar('0')).toUpper();
            giftCombo->addItem(name, QVariant(i));
        }
    }

    giftCombo->blockSignals(false);
    qDebug() << "Gift dropdown populated with" << giftCombo->count() << "items";
}

uint16_t MainWindow::extractItemIdFromScript(const QByteArray &scriptData)
{
    // Look for checkitem (0x47), checkitemspace (0x46), or setorcopyvar (0x1A) commands
    // These commands have format: opcode [item_id:word] ...
    // checkitem: 0x47 [item:word] [quantity:word]
    // checkitemspace: 0x46 [item:word] [quantity:word]
    // setorcopyvar/copyvarifnotzero: 0x1A [dest:word] [value:word]
    //   - VAR_0x8000 holds the item ID
    //   - VAR_0x8001 holds the quantity

    // Skip 4-byte CRC header if present (1004-byte format)
    int startOffset = 0;
    if (scriptData.size() == TicketResource::SCRIPT_SIZE) {
        startOffset = TicketResource::SCRIPT_HEADER_SIZE;
    }

    for (int i = startOffset; i < scriptData.size() - 4; ++i) {
        uint8_t opcode = static_cast<uint8_t>(scriptData[i]);

        // checkitem (0x47) or checkitemspace (0x46)
        if ((opcode == 0x47 || opcode == 0x46) && i + 4 < scriptData.size()) {
            uint16_t itemId = static_cast<uint8_t>(scriptData[i + 1]) |
                             (static_cast<uint8_t>(scriptData[i + 2]) << 8);
            qDebug() << "Found item ID" << itemId << "at offset" << i << "via opcode" << Qt::hex << opcode;
            return itemId;
        }

        // setorcopyvar/copyvarifnotzero (0x1A) - check if setting VAR_0x8000 (item ID)
        if (opcode == 0x1A && i + 4 < scriptData.size()) {
            uint16_t destVar = static_cast<uint8_t>(scriptData[i + 1]) |
                              (static_cast<uint8_t>(scriptData[i + 2]) << 8);
            if (destVar == 0x8000) {  // VAR_0x8000 holds item ID
                uint16_t value = static_cast<uint8_t>(scriptData[i + 3]) |
                                (static_cast<uint8_t>(scriptData[i + 4]) << 8);
                // Check if value looks like an item ID (not a variable reference)
                if (value < 0x4000) {
                    qDebug() << "Found item ID" << value << "via setorcopyvar to VAR_0x8000";
                    return value;
                }
            }
        }
    }

    return 0xFFFF;  // Not found
}

void MainWindow::updateScriptItemId(uint16_t newItemId)
{
    if (m_currentScriptData.isEmpty()) {
        return;
    }

    // Skip 4-byte CRC header if present (1004-byte format)
    int startOffset = 0;
    if (m_currentScriptData.size() == TicketResource::SCRIPT_SIZE) {
        startOffset = TicketResource::SCRIPT_HEADER_SIZE;
    }

    bool modified = false;

    // Update checkitem (0x47), checkitemspace (0x46), and setorcopyvar (0x1A) with VAR_0x8000
    for (int i = startOffset; i < m_currentScriptData.size() - 4; ++i) {
        uint8_t opcode = static_cast<uint8_t>(m_currentScriptData[i]);

        // checkitem (0x47) or checkitemspace (0x46)
        if ((opcode == 0x47 || opcode == 0x46) && i + 4 < m_currentScriptData.size()) {
            m_currentScriptData[i + 1] = static_cast<char>(newItemId & 0xFF);
            m_currentScriptData[i + 2] = static_cast<char>((newItemId >> 8) & 0xFF);
            modified = true;
            qDebug() << "Updated item ID at offset" << i << "to" << newItemId;
        }

        // setorcopyvar/copyvarifnotzero (0x1A) - update if setting VAR_0x8000 (item ID)
        if (opcode == 0x1A && i + 4 < m_currentScriptData.size()) {
            uint16_t destVar = static_cast<uint8_t>(m_currentScriptData[i + 1]) |
                              (static_cast<uint8_t>(m_currentScriptData[i + 2]) << 8);
            if (destVar == 0x8000) {  // VAR_0x8000 holds item ID
                uint16_t oldValue = static_cast<uint8_t>(m_currentScriptData[i + 3]) |
                                   (static_cast<uint8_t>(m_currentScriptData[i + 4]) << 8);
                // Only update if it was an item ID (not a variable reference)
                if (oldValue < 0x4000) {
                    m_currentScriptData[i + 3] = static_cast<char>(newItemId & 0xFF);
                    m_currentScriptData[i + 4] = static_cast<char>((newItemId >> 8) & 0xFF);
                    modified = true;
                    qDebug() << "Updated setorcopyvar VAR_0x8000 to" << newItemId;
                }
            }
        }
    }

    if (modified) {
        // Refresh the script display
        updateScriptTabs();
    }
}

void MainWindow::onGiftComboChanged(int index)
{
    if (index < 0 || m_currentScriptData.isEmpty()) {
        return;
    }

    // Get the item ID from the combo box user data
    QVariant data = giftCombo->itemData(index);
    if (!data.isValid()) {
        return;
    }

    uint16_t newItemId = static_cast<uint16_t>(data.toInt());
    qDebug() << "Gift combo changed to index" << index << "item ID" << newItemId;

    // Update the script with the new item ID
    updateScriptItemId(newItemId);
}

void MainWindow::setFilePathDisplay(const QString &fullPath)
{
    if (fullPath.isEmpty()) {
        filePathDisplay->setText("<span style='color: #909090; font-style: italic;'>No file loaded</span>");
        filePathDisplay->setToolTip("");
        return;
    }

    // Set tooltip to full path
    filePathDisplay->setToolTip(fullPath);

    // Separate directory and filename
    QFileInfo fileInfo(fullPath);
    QString directory = fileInfo.path();
    QString filename = fileInfo.fileName();

    // Calculate available width for the directory part
    // Account for padding, separator, and bold filename
    QFontMetrics fm(filePathDisplay->font());
    int separatorWidth = fm.horizontalAdvance(" › ");

    // Create bold font metrics for filename
    QFont boldFont = filePathDisplay->font();
    boldFont.setBold(true);
    QFontMetrics boldFm(boldFont);
    int filenameWidth = boldFm.horizontalAdvance(filename);

    // Use actual width or minimum reasonable width for calculation
    int labelWidth = qMax(filePathDisplay->width(), 300);
    int availableWidth = labelWidth - 24;  // padding + margins
    int directoryWidth = availableWidth - separatorWidth - filenameWidth;

    // Elide directory from the left if necessary
    QString elidedDir = directory;
    if (directoryWidth > 50) {
        elidedDir = fm.elidedText(directory, Qt::ElideLeft, directoryWidth);
    } else {
        // Not enough room for directory, just show ellipsis
        elidedDir = "…";
    }

    // Build rich text: gray path + separator + bold black filename
    QString html = QString(
        "<span style='color: #808080;'>%1</span>"
        "<span style='color: #a0a0a0;'> › </span>"
        "<b>%2</b>"
    ).arg(elidedDir.toHtmlEscaped(), filename.toHtmlEscaped());

    filePathDisplay->setText(html);
}

// ===== FILE MENU ACTIONS =====

void MainWindow::onCloseFile()
{
    // Reset save file
    delete m_saveFile;
    m_saveFile = new SaveFile();

    // Clear Wonder Card display and data
    clearWonderCardDisplay();

    // Explicitly clear GREEN MAN / script displays
    scriptTextDisplay->clear();
    scriptHexDisplay->clear();

    // Reset file path display
    filePathDisplay->setText("<span style='color: #909090; font-style: italic;'>No file loaded</span>");
    filePathDisplay->setToolTip("");

    // Reset save type button
    saveTypeButton->setText("Not detected");
    saveTypeButton->setEnabled(false);
    saveTypeButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #909090; border: none; "
        "             font-weight: normal; font-style: italic; padding: 5px 10px; }"
        "QPushButton:enabled { cursor: pointer; }"
        "QPushButton:enabled:hover { color: #505050; text-decoration: underline; }"
    );

    // Reset status bar
    statusLabel->setText("Status: No file loaded");
    checksumLabel->setText("Checksum: --");
    checksumLabel->setStyleSheet("font-weight: bold; color: white; background: transparent;");

    // Disable file-dependent actions
    closeAction->setEnabled(false);
    saveAction->setEnabled(false);
    saveAsAction->setEnabled(false);
    editButton->setEnabled(false);
    editEnableAction->setEnabled(false);
    exportWcAction->setEnabled(false);
    clearWcAction->setEnabled(false);
    enableMgFlagAction->setEnabled(false);
    validateChecksumsAction->setEnabled(false);

    // Reset edit state
    resetEditState();
}

// ===== EDIT MENU ACTIONS =====

void MainWindow::onImportWonderCard()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Import Wonder Card",
        QString(),
        "Wonder Card Files (*.bin);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox box(QMessageBox::Critical, "", "Failed to open file:\n\n" + file.errorString(), QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // Validate size (should be 336 bytes with CRC or 332 bytes without)
    if (data.size() != 336 && data.size() != 332) {
        QMessageBox box(QMessageBox::Warning, "",
            QString("Invalid Wonder Card file size: %1 bytes.\n\nExpected 336 bytes (with CRC) or 332 bytes (without CRC).").arg(data.size()),
            QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Parse the Wonder Card data (skip CRC header if present)
    QByteArray wcData = (data.size() == 336) ? data.mid(4) : data;
    WonderCardData wonderCard = MysteryGift::parseWonderCard(wcData);

    if (wonderCard.isEmpty()) {
        QMessageBox box(QMessageBox::Warning, "", "Failed to parse Wonder Card data.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Store the imported data
    m_currentWonderCard = wonderCard;
    m_currentWonderCardRaw = data;

    // Update display
    displayWonderCard(wonderCard, nullptr);

    // Enable export action
    exportWcAction->setEnabled(true);

    // If editing is enabled, also clear the raw data to force re-encoding
    if (m_editingEnabled) {
        m_currentWonderCardRaw.clear();
    }

    statusLabel->setText("Imported: " + QFileInfo(filePath).fileName());
}

void MainWindow::onExportWonderCard()
{
    if (m_currentWonderCard.isEmpty()) {
        QMessageBox box(QMessageBox::Warning, "", "No Wonder Card data to export.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    QString defaultName = m_currentWonderCard.title.isEmpty() ? "wondercard.bin" :
                          m_currentWonderCard.title.simplified().replace(" ", "_") + ".bin";

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Wonder Card",
        defaultName,
        "Wonder Card Files (*.bin);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    // Use raw data if available, otherwise encode from struct
    QByteArray dataToExport;
    if (!m_currentWonderCardRaw.isEmpty()) {
        dataToExport = m_currentWonderCardRaw;
    } else {
        dataToExport = MysteryGift::encodeWonderCard(m_currentWonderCard);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox box(QMessageBox::Critical, "", "Failed to save file:\n\n" + file.errorString(), QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    file.write(dataToExport);
    file.close();

    statusLabel->setText("Exported: " + QFileInfo(filePath).fileName());
}

void MainWindow::onClearWonderCard()
{
    QMessageBox msgBox(this);
    msgBox.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    msgBox.setText("Are you sure you want to clear the Wonder Card data?");
    msgBox.setInformativeText("This will reset the Wonder Card to an empty state.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        clearWonderCardDisplay();
        statusLabel->setText("Wonder Card cleared");
        exportWcAction->setEnabled(false);
    }
}

// ===== VIEW MENU ACTIONS =====

void MainWindow::onSwitchToWonderCardTab()
{
    textSubTabs->setCurrentIndex(0);
    hexSubTabs->setCurrentIndex(0);
}

void MainWindow::onSwitchToGreenManTab()
{
    textSubTabs->setCurrentIndex(1);
    hexSubTabs->setCurrentIndex(1);
}

// ===== TOOLS MENU ACTIONS =====

void MainWindow::onLoadRomManual()
{
    QString romPath = QFileDialog::getOpenFileName(
        this,
        "Select Pokemon GBA ROM",
        QString(),
        "GBA ROM Files (*.gba);;All Files (*)"
    );

    if (romPath.isEmpty()) {
        return;
    }

    // Validate the selected ROM
    QString md5 = RomLoader::computeMD5(romPath);
    const RomDatabase::RomVersion *version = m_romDatabase->identifyRom(md5);

    if (version) {
        m_romPath = romPath;
        m_romVersionName = version->name;

        QString error;
        if (wonderCardVisualDisplay->loadROM(m_romPath, error)) {
            m_romLoaded = true;
            m_useFallbackGraphics = false;
            statusLabel->setText(QString("ROM: %1").arg(m_romVersionName));
            populateGiftDropdown();

            // Enable preset/gift combos if editing
            if (m_editingEnabled) {
                presetCombo->setEnabled(true);
                giftCombo->setEnabled(true);
            }
        } else {
            QMessageBox box(QMessageBox::Warning, "", "Failed to load ROM graphics:\n\n" + error, QMessageBox::Ok, this);
            box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
            box.exec();
        }
    } else {
        QMessageBox box(QMessageBox::Warning, "", QString(
            "The selected ROM is not recognized.\n\n"
            "MD5: %1\n\n"
            "Supported ROMs:\n"
            "- Pokemon FireRed (USA) 1.0 / 1.1\n"
            "- Pokemon LeafGreen (USA) 1.0 / 1.1\n"
            "- Pokemon Emerald (USA)").arg(md5), QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
    }
}

void MainWindow::onEnableMysteryGiftFlag()
{
    if (!m_saveFile->isLoaded()) {
        QMessageBox box(QMessageBox::Warning, "", "Please load a save file first.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    if (m_saveFile->isMysteryGiftEnabled()) {
        QMessageBox box(QMessageBox::Information, "", "Mystery Gift is already enabled in this save file.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    QString errorMessage;
    if (m_saveFile->enableMysteryGift(errorMessage)) {
        QMessageBox box(QMessageBox::Information, "",
            "Mystery Gift flag has been enabled.\n\nRemember to save the file to keep this change.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        statusLabel->setText("Mystery Gift flag enabled (unsaved)");
    } else {
        QMessageBox box(QMessageBox::Warning, "", "Failed to enable Mystery Gift flag:\n\n" + errorMessage, QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
    }
}

void MainWindow::onValidateChecksums()
{
    if (!m_saveFile->isLoaded()) {
        QMessageBox box(QMessageBox::Warning, "", "Please load a save file first.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    bool valid = m_saveFile->validateChecksums();

    if (valid) {
        checksumLabel->setText("Checksum: Valid");
        checksumLabel->setStyleSheet("font-weight: bold; color: #90EE90; background: transparent;");

        QMessageBox box(QMessageBox::Information, "", "All checksums are valid.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
    } else {
        checksumLabel->setText("Checksum: Invalid");
        checksumLabel->setStyleSheet("font-weight: bold; color: #FFB6C1; background: transparent;");

        QMessageBox box(QMessageBox::Warning, "", "One or more checksums are invalid.\n\nThe save file may be corrupted.", QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
    }
}

void MainWindow::onOpenOptions()
{
    // TODO: Implement options dialog
    QMessageBox box(QMessageBox::Information, "", "Options dialog is not yet implemented.", QMessageBox::Ok, this);
    box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    box.exec();
}

// ===== HELP MENU ACTIONS =====

void MainWindow::onShowDocumentation()
{
    // Find documentation.html
    QString docPath = QCoreApplication::applicationDirPath() + "/documentation.html";
    QFileInfo docFile(docPath);

    if (!docFile.exists()) {
        // Try relative to executable's parent (for development)
        docPath = QCoreApplication::applicationDirPath() + "/../documentation.html";
        docFile.setFile(docPath);
    }

    if (!docFile.exists()) {
        // Try project root (for Qt Creator)
        docPath = QCoreApplication::applicationDirPath() + "/../../documentation.html";
        docFile.setFile(docPath);
    }

    if (!docFile.exists()) {
        QMessageBox box(QMessageBox::Warning, "",
            "Documentation file not found.\n\n"
            "Expected location:\n" + docFile.absoluteFilePath(),
            QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }

    // Read the HTML content
    QFile file(docFile.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox box(QMessageBox::Warning, "",
            "Could not open documentation file.",
            QMessageBox::Ok, this);
        box.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        box.exec();
        return;
    }
    QString htmlContent = QString::fromUtf8(file.readAll());
    file.close();

    // Create dialog with QTextBrowser (frameless to match app style)
    // Using a custom class to enable dragging by the frame
    class DraggableDialog : public QDialog {
    public:
        DraggableDialog(QWidget *parent) : QDialog(parent), m_dragging(false) {
            setMouseTracking(true);
        }
    protected:
        void mousePressEvent(QMouseEvent *event) override {
            if (event->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
                event->accept();
            }
        }
        void mouseMoveEvent(QMouseEvent *event) override {
            if (m_dragging && (event->buttons() & Qt::LeftButton)) {
                move(event->globalPosition().toPoint() - m_dragPos);
                event->accept();
            }
        }
        void mouseReleaseEvent(QMouseEvent *event) override {
            m_dragging = false;
            event->accept();
        }
    private:
        bool m_dragging;
        QPoint m_dragPos;
    };

    DraggableDialog *docDialog = new DraggableDialog(this);
    docDialog->setWindowTitle("Documentation");
    docDialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    docDialog->resize(700, 600);
    docDialog->setStyleSheet("DraggableDialog { background-color: #d0d0d0; border-radius: 8px; }");

    QVBoxLayout *layout = new QVBoxLayout(docDialog);
    layout->setContentsMargins(8, 8, 8, 8);  // Frame border for dragging

    QTextBrowser *browser = new QTextBrowser(docDialog);
    browser->setHtml(htmlContent);
    browser->setOpenExternalLinks(true);
    browser->setStyleSheet(
        "QTextBrowser { background-color: #f5f5f5; border: none; border-radius: 4px; }"
    );
    layout->addWidget(browser);

    // Close button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(2, 5, 2, 2);
    buttonLayout->addStretch();
    QPushButton *closeBtn = new QPushButton("Close", docDialog);
    closeBtn->setFixedWidth(80);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; "
        "padding: 6px 12px; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(closeBtn, &QPushButton::clicked, docDialog, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    docDialog->exec();
    delete docDialog;
}

void MainWindow::onShowAbout()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(
        "<h2>Mystery Gift Injector</h2>"
        "<p>Version 1.0</p>"
        "<p>A tool for injecting Mystery Gift Wonder Cards into<br>"
        "Pokemon Generation III save files.</p>"
        "<p><b>Supported Games:</b><br>"
        "Pokemon FireRed / LeafGreen<br>"
        "Pokemon Emerald</p>"
        "<hr>"
        "<p>Created by ComradeSean</p>"
    );
    aboutBox.exec();
}
