#include "ticketresource.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

TicketResource::TicketResource()
    : m_gameType(GameType::Unknown)
    , m_dataLoaded(false)
{
}

TicketResource::TicketResource(const QString &id, const QString &name, GameType gameType,
                               const QString &wonderCardFile, const QString &scriptFile,
                               const QString &description)
    : m_id(id)
    , m_name(name)
    , m_gameType(gameType)
    , m_wonderCardFile(wonderCardFile)
    , m_scriptFile(scriptFile)
    , m_description(description)
    , m_dataLoaded(false)
{
}

QString TicketResource::gameTypeString() const
{
    switch (m_gameType) {
        case GameType::FireRedLeafGreen: return "FireRed/LeafGreen";
        case GameType::RubySapphire:     return "Ruby/Sapphire";
        case GameType::Emerald:          return "Emerald";
        case GameType::Unknown:
        default:                         return "Unknown";
    }
}

bool TicketResource::loadData(const QString &ticketsFolder, QString &errorMessage)
{
    // Clear any existing data
    m_wonderCardData.clear();
    m_scriptData.clear();
    m_dataLoaded = false;

    // Load Wonder Card file
    QString wonderCardPath = QDir(ticketsFolder).filePath(m_wonderCardFile);
    QFile wonderCardFile(wonderCardPath);

    if (!wonderCardFile.exists()) {
        errorMessage = QString("Wonder Card file not found: %1").arg(wonderCardPath);
        return false;
    }

    if (!wonderCardFile.open(QIODevice::ReadOnly)) {
        errorMessage = QString("Failed to open Wonder Card file: %1").arg(wonderCardFile.errorString());
        return false;
    }

    m_wonderCardData = wonderCardFile.readAll();
    wonderCardFile.close();

    if (m_wonderCardData.size() != WONDERCARD_SIZE) {
        errorMessage = QString("Invalid Wonder Card size: %1 bytes (expected %2 bytes)")
                          .arg(m_wonderCardData.size())
                          .arg(WONDERCARD_SIZE);
        m_wonderCardData.clear();
        return false;
    }

    // Load Script file
    QString scriptPath = QDir(ticketsFolder).filePath(m_scriptFile);
    QFile scriptFile(scriptPath);

    if (!scriptFile.exists()) {
        errorMessage = QString("Script file not found: %1").arg(scriptPath);
        m_wonderCardData.clear();
        return false;
    }

    if (!scriptFile.open(QIODevice::ReadOnly)) {
        errorMessage = QString("Failed to open script file: %1").arg(scriptFile.errorString());
        m_wonderCardData.clear();
        return false;
    }

    m_scriptData = scriptFile.readAll();
    scriptFile.close();

    if (m_scriptData.size() != SCRIPT_SIZE) {
        errorMessage = QString("Invalid script size: %1 bytes (expected %2 bytes)")
                          .arg(m_scriptData.size())
                          .arg(SCRIPT_SIZE);
        m_wonderCardData.clear();
        m_scriptData.clear();
        return false;
    }

    m_dataLoaded = true;
    return true;
}
