#include "ticketmanager.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

const QString TicketManager::DEFAULT_TICKETS_FOLDER = "Tickets";

TicketManager::TicketManager()
    : m_loaded(false)
{
}

TicketManager::~TicketManager()
{
}

bool TicketManager::loadFromFolder(const QString &ticketsFolderPath, QString &errorMessage)
{
    // Clear any existing data
    m_tickets.clear();
    m_crcTable.clear();
    m_loaded = false;
    m_ticketsFolderPath = ticketsFolderPath;

    // Check if folder exists
    QDir ticketsDir(ticketsFolderPath);
    if (!ticketsDir.exists()) {
        errorMessage = QString("Tickets folder not found: %1").arg(ticketsFolderPath);
        return false;
    }

    // Load CRC table (required - from embedded resource)
    if (!loadCrcTable(errorMessage)) {
        return false;
    }

    // Discover tickets from folder (dynamic discovery)
    if (!discoverTickets(errorMessage)) {
        return false;
    }

    // Optionally load metadata from manifest (display names, descriptions)
    loadManifestMetadata();

    m_loaded = true;
    return true;
}

bool TicketManager::loadCrcTable(QString &errorMessage)
{
    // Load CRC table from embedded resource
    QFile crcFile(":/Resources/tab.bin");

    if (!crcFile.exists()) {
        errorMessage = QString("CRC table not found in embedded resources.\n\n"
                               "The tab.bin file is required for checksum calculations.");
        return false;
    }

    if (!crcFile.open(QIODevice::ReadOnly)) {
        errorMessage = QString("Failed to open CRC table: %1").arg(crcFile.errorString());
        return false;
    }

    m_crcTable = crcFile.readAll();
    crcFile.close();

    if (m_crcTable.size() != CRC_TABLE_SIZE) {
        errorMessage = QString("Invalid CRC table size: %1 bytes (expected %2 bytes)")
                          .arg(m_crcTable.size())
                          .arg(CRC_TABLE_SIZE);
        m_crcTable.clear();
        return false;
    }

    return true;
}

bool TicketManager::discoverTickets(QString &errorMessage)
{
    QDir ticketsDir(m_ticketsFolderPath);

    // Find all WonderCard files (flexible pattern to support multiple languages)
    QStringList wonderCardFiles = ticketsDir.entryList(
        QStringList() << "*_WonderCard.bin",
        QDir::Files,
        QDir::Name
    );

    if (wonderCardFiles.isEmpty()) {
        errorMessage = QString("No ticket files found in: %1\n\n"
                               "Expected files matching pattern: {NAME}_WonderCard.bin")
                          .arg(m_ticketsFolderPath);
        return false;
    }

    // Flexible pattern to parse various naming conventions:
    // New format: AURORA_TICKET_2004_FALL_FRLG_ENGUSA_WonderCard.bin
    // New format: MYSTIC_TICKET_EMERALD_ENGUSA_WonderCard.bin
    // Old format: MYSTIC_FRLG_WonderCard_USA.bin (legacy support)
    //
    // We extract: base name (everything before _WonderCard), then parse game code from it

    for (const QString &wcFile : wonderCardFiles) {
        // Extract base name (everything before _WonderCard.bin)
        QString baseName = wcFile;
        baseName.replace("_WonderCard.bin", "", Qt::CaseInsensitive);

        // Construct expected script filename
        QString scriptFile = baseName + "_Script.bin";
        QString scriptPath = ticketsDir.filePath(scriptFile);

        // Check if matching script file exists
        if (!QFile::exists(scriptPath)) {
            qWarning() << "Missing script file for" << wcFile << "- expected:" << scriptFile;
            continue;
        }

        // Verify file sizes
        QFileInfo wcInfo(ticketsDir.filePath(wcFile));
        QFileInfo scriptInfo(scriptPath);

        if (wcInfo.size() != TicketResource::WONDERCARD_SIZE) {
            qWarning() << "Invalid WonderCard size:" << wcFile
                       << "(" << wcInfo.size() << "bytes, expected"
                       << TicketResource::WONDERCARD_SIZE << ")";
            continue;
        }

        if (scriptInfo.size() != TicketResource::SCRIPT_SIZE) {
            qWarning() << "Invalid Script size:" << scriptFile
                       << "(" << scriptInfo.size() << "bytes, expected"
                       << TicketResource::SCRIPT_SIZE << ")";
            continue;
        }

        // Parse game type from filename - look for known game codes
        GameType gameType = GameType::Unknown;
        QString baseUpper = baseName.toUpper();

        if (baseUpper.contains("_FRLG_") || baseUpper.endsWith("_FRLG")) {
            gameType = GameType::FireRedLeafGreen;
        } else if (baseUpper.contains("_EMERALD_") || baseUpper.endsWith("_EMERALD") ||
                   baseUpper.contains("_E_") || baseUpper.endsWith("_E")) {
            gameType = GameType::Emerald;
        } else if (baseUpper.contains("_RS_") || baseUpper.endsWith("_RS")) {
            gameType = GameType::RubySapphire;
        }

        if (gameType == GameType::Unknown) {
            qWarning() << "Could not determine game type for:" << wcFile;
            continue;
        }

        // Extract language code if present (e.g., ENGUSA, ENGUK, ESP, FRE, GER, ITA)
        QString language;
        QRegularExpression langPattern("_(ENG(?:USA|UK)?|ESP|FRE|GER|ITA|JAP)(?:_|$)",
                                        QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch langMatch = langPattern.match(baseName);
        if (langMatch.hasMatch()) {
            language = langMatch.captured(1).toUpper();
        }

        // Create ticket ID from base name
        QString id = baseName.toLower().replace(' ', '_');

        // Create display name - extract event type, game, and language
        QString displayName = formatDisplayName(baseName, gameType, language);

        // Create ticket resource
        TicketResource ticket(id, displayName, gameType, wcFile, scriptFile, QString());
        m_tickets.append(ticket);

        qInfo() << "Discovered ticket:" << id << "(" << displayName << ")";
    }

    if (m_tickets.isEmpty()) {
        errorMessage = QString("No valid ticket pairs found in: %1\n\n"
                               "Each ticket requires both WonderCard and Script files with matching names.")
                          .arg(m_ticketsFolderPath);
        return false;
    }

    return true;
}

bool TicketManager::loadManifestMetadata()
{
    // Optional: Load display names and descriptions from tickets.json if it exists
    QString manifestPath = QDir(m_ticketsFolderPath).filePath("tickets.json");
    QFile manifestFile(manifestPath);

    if (!manifestFile.exists() || !manifestFile.open(QIODevice::ReadOnly)) {
        return false;  // No manifest, not an error
    }

    QByteArray manifestData = manifestFile.readAll();
    manifestFile.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(manifestData, &parseError);

    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("tickets") || !root["tickets"].isArray()) {
        return false;
    }

    // Build lookup map from manifest
    QMap<QString, QPair<QString, QString>> metadata;  // id -> (name, description)

    QJsonArray ticketsArray = root["tickets"].toArray();
    for (const QJsonValue &ticketValue : ticketsArray) {
        if (!ticketValue.isObject()) continue;
        QJsonObject ticketObj = ticketValue.toObject();

        QString id = ticketObj["id"].toString();
        QString name = ticketObj["name"].toString();
        QString description = ticketObj["description"].toString();

        if (!id.isEmpty()) {
            metadata[id] = qMakePair(name, description);
        }
    }

    // Update discovered tickets with manifest metadata
    // Note: This requires making TicketResource mutable or adding setters
    // For now, we just log what we found
    for (const auto &id : metadata.keys()) {
        qInfo() << "Manifest metadata for" << id << ":" << metadata[id].first;
    }

    return true;
}

GameType TicketManager::parseGameFromFilename(const QString &gameCode) const
{
    QString normalized = gameCode.toUpper().trimmed();

    if (normalized == "E" || normalized == "EMERALD") {
        return GameType::Emerald;
    } else if (normalized == "FRLG" || normalized == "FR" || normalized == "LG") {
        return GameType::FireRedLeafGreen;
    } else if (normalized == "RS" || normalized == "R" || normalized == "S") {
        return GameType::RubySapphire;
    }

    return GameType::Unknown;
}

QString TicketManager::formatEventName(const QString &eventCode) const
{
    // Convert EVENT_CODE to "Event Code" display name
    QString name = eventCode;
    name = name.replace('_', ' ');

    // Title case each word
    QStringList words = name.toLower().split(' ');
    for (int i = 0; i < words.size(); ++i) {
        if (!words[i].isEmpty()) {
            words[i][0] = words[i][0].toUpper();
        }
    }

    return words.join(' ');
}

QString TicketManager::formatDisplayName(const QString &baseName, GameType gameType, const QString &language) const
{
    // Extract event type from filename and create a user-friendly display name
    // Examples:
    //   AURORA_TICKET_2004_FALL_FRLG_ENGUSA -> "Aurora Ticket - FRLG (USA)"
    //   MYSTIC_TICKET_EMERALD_ENGUK -> "Mystic Ticket - Emerald (UK)"
    //   AURORA_TICKET_FRLG_ESP -> "Aurora Ticket - FRLG (Spanish)"

    QString name;
    QString baseUpper = baseName.toUpper();

    // Determine event type
    if (baseUpper.contains("AURORA")) {
        name = "Aurora Ticket";
    } else if (baseUpper.contains("MYSTIC")) {
        name = "Mystic Ticket";
    } else if (baseUpper.contains("OLD_SEA") || baseUpper.contains("OLDSEA")) {
        name = "Old Sea Map";
    } else if (baseUpper.contains("EON")) {
        name = "Eon Ticket";
    } else {
        // Fallback: use formatted base name
        name = formatEventName(baseName.split('_').first());
    }

    // Add game type
    switch (gameType) {
        case GameType::FireRedLeafGreen:
            name += " - FRLG";
            break;
        case GameType::Emerald:
            name += " - Emerald";
            break;
        case GameType::RubySapphire:
            name += " - RS";
            break;
        default:
            break;
    }

    // Add language/region suffix
    if (!language.isEmpty()) {
        QString langDisplay;
        if (language == "ENGUSA") {
            langDisplay = "USA";
        } else if (language == "ENGUK") {
            langDisplay = "UK";
        } else if (language == "ESP") {
            langDisplay = "Spanish";
        } else if (language == "FRE") {
            langDisplay = "French";
        } else if (language == "GER") {
            langDisplay = "German";
        } else if (language == "ITA") {
            langDisplay = "Italian";
        } else if (language == "JAP") {
            langDisplay = "Japanese";
        } else {
            langDisplay = language;
        }
        name += QString(" (%1)").arg(langDisplay);
    }

    // Add special event info if present (e.g., "TCGWC 2005", "2004 FALL")
    if (baseUpper.contains("TCGWC")) {
        // Find year if present
        QRegularExpression yearPattern("TCGWC[_\\s]*(\\d{4})");
        QRegularExpressionMatch match = yearPattern.match(baseUpper);
        if (match.hasMatch()) {
            name += QString(" [TCGWC %1]").arg(match.captured(1));
        } else {
            name += " [TCGWC]";
        }
    } else if (baseUpper.contains("2004") && baseUpper.contains("FALL")) {
        name += " [2004 Fall]";
    }

    return name;
}

GameType TicketManager::parseGameType(const QString &gameTypeStr) const
{
    QString normalized = gameTypeStr.toLower().trimmed();

    if (normalized == "emerald") {
        return GameType::Emerald;
    } else if (normalized == "fireredleafgreen" || normalized == "frlg" ||
               normalized == "firered/leafgreen" || normalized == "fr/lg") {
        return GameType::FireRedLeafGreen;
    } else if (normalized == "rubysapphire" || normalized == "rs" ||
               normalized == "ruby/sapphire" || normalized == "r/s") {
        return GameType::RubySapphire;
    }

    return GameType::Unknown;
}

QVector<const TicketResource*> TicketManager::ticketsForGame(GameType gameType) const
{
    QVector<const TicketResource*> result;

    for (const TicketResource &ticket : m_tickets) {
        if (ticket.gameType() == gameType) {
            result.append(&ticket);
        }
    }

    return result;
}

const TicketResource* TicketManager::findTicketById(const QString &id) const
{
    for (const TicketResource &ticket : m_tickets) {
        if (ticket.id() == id) {
            return &ticket;
        }
    }

    return nullptr;
}

const TicketResource* TicketManager::findTicketByWonderCard(const QByteArray &wonderCardData,
                                                             GameType gameType)
{
    if (wonderCardData.isEmpty() || wonderCardData.size() != TicketResource::WONDERCARD_SIZE) {
        return nullptr;
    }

    // Search through tickets matching the game type
    for (TicketResource &ticket : m_tickets) {
        if (ticket.gameType() != gameType) {
            continue;
        }

        // Load ticket data if not already loaded
        if (!ticket.isDataLoaded()) {
            QString error;
            if (!ticket.loadData(m_ticketsFolderPath, error)) {
                qWarning() << "Failed to load ticket data for comparison:" << ticket.id() << error;
                continue;
            }
        }

        // Compare wonder card data, skipping CRC header and variable fields
        // 336-byte format: CRC(0-3), payload(4-335)
        // Payload: eventId(0-1), icon(2-3), COUNT(4-7), data(8-331)
        // Skip: CRC header (0-3), COUNT (bytes 8-11 of file / 4-7 of payload)
        const QByteArray &ticketData = ticket.wonderCardData();

        // Both files should be 336 bytes (with CRC header)
        const int HEADER_SIZE = TicketResource::WONDERCARD_HEADER_SIZE;  // 4
        const int PAYLOAD_SIZE = TicketResource::WONDERCARD_PAYLOAD_SIZE;  // 332

        // Verify both are 336 bytes
        if (ticketData.size() != TicketResource::WONDERCARD_SIZE) {
            continue;
        }

        const char* savePtr = wonderCardData.constData() + HEADER_SIZE;
        const char* ticketPtr = ticketData.constData() + HEADER_SIZE;

        bool match = true;

        // Compare eventId + icon (first 4 bytes of payload)
        if (memcmp(savePtr, ticketPtr, 4) != 0) {
            match = false;
        }

        // Skip COUNT (bytes 4-7 of payload), compare rest (bytes 8+)
        if (match) {
            int afterCount = 8;  // Skip first 4 bytes (eventId+icon) and COUNT (4 bytes)
            int remainingSize = PAYLOAD_SIZE - afterCount;
            if (memcmp(savePtr + afterCount, ticketPtr + afterCount, remainingSize) != 0) {
                match = false;
            }
        }

        if (match) {
            return &ticket;
        }
    }

    return nullptr;
}
