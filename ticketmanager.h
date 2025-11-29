#ifndef TICKETMANAGER_H
#define TICKETMANAGER_H

#include <QString>
#include <QByteArray>
#include <QVector>
#include "ticketresource.h"

/// <summary>
/// Manages loading and accessing Mystery Gift ticket resources from external folder.
///
/// FOLDER STRUCTURE:
/// Tickets/
///   tab.bin                    - CRC-16 lookup table (embedded resource, 512 bytes)
///   {NAME}_WonderCard.bin      - Wonder Card files (336 bytes each)
///   {NAME}_Script.bin          - Script files (1004 bytes each)
///
/// FILE NAMING CONVENTION:
///   {NAME} should contain:
///     - Event type: AURORA_TICKET, MYSTIC_TICKET, OLD_SEA_MAP, EON_TICKET
///     - Game code: FRLG, EMERALD, E, RS
///     - Language (optional): ENGUSA, ENGUK, ESP, FRE, GER, ITA, JAP
///
/// EXAMPLES:
///   AURORA_TICKET_FRLG_ENGUSA_WonderCard.bin + AURORA_TICKET_FRLG_ENGUSA_Script.bin
///   MYSTIC_TICKET_EMERALD_ENGUK_WonderCard.bin + MYSTIC_TICKET_EMERALD_ENGUK_Script.bin
///   AURORA_TICKET_FRLG_ESP_WonderCard.bin + AURORA_TICKET_FRLG_ESP_Script.bin
///
/// FILE SIZES (with 4-byte CRC header):
///   WonderCard: 336 bytes (0x150) = 4 header + 332 payload
///   Script:    1004 bytes (0x3EC) = 4 header + 1000 payload
///
/// DYNAMIC DISCOVERY:
///   Tickets are discovered by scanning for *_WonderCard.bin files.
///   Game type is parsed from filename (looks for FRLG, EMERALD, E, RS).
///   Language is extracted for display name formatting.
///   Each WonderCard is matched to its Script by the shared prefix.
///   Optional tickets.json manifest can provide additional metadata.
/// </summary>
class TicketManager
{
public:
    TicketManager();
    ~TicketManager();

    // Initialize from Tickets folder
    bool loadFromFolder(const QString &ticketsFolderPath, QString &errorMessage);

    // Check if tickets are loaded
    bool isLoaded() const { return m_loaded; }

    // Get all tickets
    const QVector<TicketResource>& tickets() const { return m_tickets; }

    // Get tickets filtered by game type
    QVector<const TicketResource*> ticketsForGame(GameType gameType) const;

    // Find ticket by ID
    const TicketResource* findTicketById(const QString &id) const;

    // Find ticket by matching wonder card data (for identifying save file contents)
    // Returns the matching ticket or nullptr if no match found
    const TicketResource* findTicketByWonderCard(const QByteArray &wonderCardData,
                                                  GameType gameType);

    // Get CRC table
    const QByteArray& crcTable() const { return m_crcTable; }
    bool hasCrcTable() const { return !m_crcTable.isEmpty(); }

    // Paths
    QString ticketsFolderPath() const { return m_ticketsFolderPath; }

    // Constants
    static const qint64 CRC_TABLE_SIZE = 512;
    static const QString DEFAULT_TICKETS_FOLDER;

private:
    bool loadCrcTable(QString &errorMessage);
    bool discoverTickets(QString &errorMessage);
    bool loadManifestMetadata();  // Optional: load display names/descriptions from manifest
    GameType parseGameType(const QString &gameTypeStr) const;
    GameType parseGameFromFilename(const QString &gameCode) const;
    QString formatEventName(const QString &eventCode) const;
    QString formatDisplayName(const QString &baseName, GameType gameType, const QString &language) const;

    QString m_ticketsFolderPath;
    QVector<TicketResource> m_tickets;
    QByteArray m_crcTable;
    bool m_loaded;
};

#endif // TICKETMANAGER_H
