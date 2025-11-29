#include "scriptdisassembler.h"
#include "gbaromreader.h"  // GBAROReader class
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSet>
#include <QDebug>
#include <algorithm>

ScriptDisassembler::ScriptDisassembler()
    : m_inferredBase(0)
    , m_romReader(nullptr)
{
    initGen3Charset();
}

ScriptDisassembler::~ScriptDisassembler()
{
}

void ScriptDisassembler::initGen3Charset()
{
    // Gen 3 proprietary character encoding
    // Using QString::fromUtf8 for special characters to avoid encoding issues
    m_gen3Charset = {
        {0x00, " "},
        // Accented uppercase
        {0x01, QString::fromUtf8("\xC3\x80")}, // A grave
        {0x02, QString::fromUtf8("\xC3\x81")}, // A acute
        {0x03, QString::fromUtf8("\xC3\x82")}, // A circumflex
        {0x04, QString::fromUtf8("\xC3\x87")}, // C cedilla
        {0x05, QString::fromUtf8("\xC3\x88")}, // E grave
        {0x06, QString::fromUtf8("\xC3\x89")}, // E acute
        {0x07, QString::fromUtf8("\xC3\x8A")}, // E circumflex
        {0x08, QString::fromUtf8("\xC3\x8B")}, // E diaeresis
        {0x09, QString::fromUtf8("\xC3\x8C")}, // I grave
        {0x0B, QString::fromUtf8("\xC3\x8E")}, // I circumflex
        {0x0C, QString::fromUtf8("\xC3\x8F")}, // I diaeresis
        {0x0D, QString::fromUtf8("\xC3\x92")}, // O grave
        {0x0E, QString::fromUtf8("\xC3\x93")}, // O acute
        {0x0F, QString::fromUtf8("\xC3\x94")}, // O circumflex
        {0x10, QString::fromUtf8("\xC5\x92")}, // OE ligature
        {0x11, QString::fromUtf8("\xC3\x99")}, // U grave
        {0x12, QString::fromUtf8("\xC3\x9A")}, // U acute
        {0x13, QString::fromUtf8("\xC3\x9B")}, // U circumflex
        {0x14, QString::fromUtf8("\xC3\x91")}, // N tilde
        {0x15, QString::fromUtf8("\xC3\x9F")}, // German sharp s
        // Accented lowercase
        {0x16, QString::fromUtf8("\xC3\xA0")}, // a grave
        {0x17, QString::fromUtf8("\xC3\xA1")}, // a acute
        {0x19, QString::fromUtf8("\xC3\xA7")}, // c cedilla
        {0x1A, QString::fromUtf8("\xC3\xA8")}, // e grave
        {0x1B, QString::fromUtf8("\xC3\xA9")}, // e acute
        {0x1C, QString::fromUtf8("\xC3\xAA")}, // e circumflex
        {0x1D, QString::fromUtf8("\xC3\xAB")}, // e diaeresis
        {0x1E, QString::fromUtf8("\xC3\xAC")}, // i grave
        {0x20, QString::fromUtf8("\xC3\xAE")}, // i circumflex
        {0x21, QString::fromUtf8("\xC3\xAF")}, // i diaeresis
        {0x22, QString::fromUtf8("\xC3\xB2")}, // o grave
        {0x23, QString::fromUtf8("\xC3\xB3")}, // o acute
        {0x24, QString::fromUtf8("\xC3\xB4")}, // o circumflex
        {0x25, QString::fromUtf8("\xC5\x93")}, // oe ligature
        {0x26, QString::fromUtf8("\xC3\xB9")}, // u grave
        {0x27, QString::fromUtf8("\xC3\xBA")}, // u acute
        {0x28, QString::fromUtf8("\xC3\xBB")}, // u circumflex
        {0x29, QString::fromUtf8("\xC3\xB1")}, // n tilde
        // Special characters
        {0x2A, QString::fromUtf8("\xC2\xBA")}, // masculine ordinal
        {0x2B, QString::fromUtf8("\xC2\xAA")}, // feminine ordinal
        {0x2D, "&"},
        {0x2E, "+"},
        {0x35, "="},
        {0x36, ";"},
        {0x51, QString::fromUtf8("\xC2\xBF")}, // inverted question
        {0x52, QString::fromUtf8("\xC2\xA1")}, // inverted exclamation
        {0x5A, QString::fromUtf8("\xC3\x8D")}, // I acute
        {0x5B, "%"},
        {0x5C, "("},
        {0x5D, ")"},
        {0x68, QString::fromUtf8("\xC3\xA2")}, // a circumflex
        {0x6F, QString::fromUtf8("\xC3\xAD")}, // i acute
        // Numbers
        {0xA1, "0"}, {0xA2, "1"}, {0xA3, "2"}, {0xA4, "3"}, {0xA5, "4"},
        {0xA6, "5"}, {0xA7, "6"}, {0xA8, "7"}, {0xA9, "8"}, {0xAA, "9"},
        // Punctuation
        {0xAB, "!"},
        {0xAC, "?"},
        {0xAD, "."},
        {0xAE, "-"},
        {0xB0, QString::fromUtf8("\xE2\x80\xA6")}, // ellipsis
        {0xB1, QString::fromUtf8("\xE2\x80\x9C")}, // left double quote
        {0xB2, QString::fromUtf8("\xE2\x80\x9D")}, // right double quote
        {0xB3, QString::fromUtf8("\xE2\x80\x98")}, // left single quote
        {0xB4, QString::fromUtf8("\xE2\x80\x99")}, // right single quote
        {0xB5, QString::fromUtf8("\xE2\x99\x82")}, // male symbol
        {0xB6, QString::fromUtf8("\xE2\x99\x80")}, // female symbol
        {0xB7, "$"},
        {0xB8, ","},
        {0xB9, QString::fromUtf8("\xC3\x97")}, // multiplication sign
        {0xBA, "/"},
        // Uppercase letters
        {0xBB, "A"}, {0xBC, "B"}, {0xBD, "C"}, {0xBE, "D"}, {0xBF, "E"},
        {0xC0, "F"}, {0xC1, "G"}, {0xC2, "H"}, {0xC3, "I"}, {0xC4, "J"},
        {0xC5, "K"}, {0xC6, "L"}, {0xC7, "M"}, {0xC8, "N"}, {0xC9, "O"},
        {0xCA, "P"}, {0xCB, "Q"}, {0xCC, "R"}, {0xCD, "S"}, {0xCE, "T"},
        {0xCF, "U"}, {0xD0, "V"}, {0xD1, "W"}, {0xD2, "X"}, {0xD3, "Y"},
        {0xD4, "Z"},
        // Lowercase letters
        {0xD5, "a"}, {0xD6, "b"}, {0xD7, "c"}, {0xD8, "d"}, {0xD9, "e"},
        {0xDA, "f"}, {0xDB, "g"}, {0xDC, "h"}, {0xDD, "i"}, {0xDE, "j"},
        {0xDF, "k"}, {0xE0, "l"}, {0xE1, "m"}, {0xE2, "n"}, {0xE3, "o"},
        {0xE4, "p"}, {0xE5, "q"}, {0xE6, "r"}, {0xE7, "s"}, {0xE8, "t"},
        {0xE9, "u"}, {0xEA, "v"}, {0xEB, "w"}, {0xEC, "x"}, {0xED, "y"},
        {0xEE, "z"},
        // Other
        {0xEF, QString::fromUtf8("\xE2\x96\xB6")}, // right-pointing triangle
        {0xF0, ":"},
        {0xF1, QString::fromUtf8("\xC3\x84")}, // A diaeresis
        {0xF2, QString::fromUtf8("\xC3\x96")}, // O diaeresis
        {0xF3, QString::fromUtf8("\xC3\x9C")}, // U diaeresis
        {0xF4, QString::fromUtf8("\xC3\xA4")}, // a diaeresis
        {0xF5, QString::fromUtf8("\xC3\xB6")}, // o diaeresis
        {0xF6, QString::fromUtf8("\xC3\xBC")}, // u diaeresis
        // Control codes
        {0xFA, "\\l"}, {0xFB, "\\p"}, {0xFC, "\\c"}, {0xFD, "\\v"}, {0xFE, "\\n"},
    };
}

bool ScriptDisassembler::loadCommandDefinitions(const QString &yamlPath, QString &error)
{
    QFile file(yamlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QString("Failed to open %1: %2").arg(yamlPath, file.errorString());
        return false;
    }

    m_commands.clear();
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Simple YAML parsing for command definitions
    // Format: 0xNN: { name: "xxx", args: "xxx", desc: "xxx" }
    QRegularExpression cmdRegex(
        "0x([0-9A-Fa-f]+):\\s*\\{\\s*name:\\s*\"([^\"]+)\"\\s*,\\s*args:\\s*\"([^\"]*)\"\\s*,\\s*desc:\\s*\"([^\"]+)\"\\s*\\}");

    QRegularExpressionMatchIterator it = cmdRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        bool ok;
        uint8_t opcode = match.captured(1).toUInt(&ok, 16);
        if (ok) {
            CommandDef cmd;
            cmd.name = match.captured(2);
            cmd.args = match.captured(3);
            cmd.desc = match.captured(4);
            m_commands[opcode] = cmd;
        }
    }

    if (m_commands.isEmpty()) {
        error = "No commands parsed from YAML file";
        return false;
    }

    return true;
}

bool ScriptDisassembler::loadScriptData(const QString &yamlPath, QString &error)
{
    QFile file(yamlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QString("Failed to open %1: %2").arg(yamlPath, file.errorString());
        return false;
    }

    QString content = file.readAll();
    file.close();

    qDebug() << "ScriptDisassembler: Loaded script_data.yaml, size:" << content.size() << "bytes";

    // Parse conditions (0x00 - 0x05 with symbol)
    QRegularExpression condRegex("0x0([0-5]):\\s*\\{\\s*symbol:\\s*\"([^\"]+)\"");
    QRegularExpressionMatchIterator it = condRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        uint8_t code = match.captured(1).toUInt(nullptr, 16);
        m_conditions[code] = match.captured(2);
    }

    // Parse std_scripts
    QRegularExpression stdRegex("0x0([0-7]):\\s*\"(STD_[^\"]+)\"");
    it = stdRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        uint8_t id = match.captured(1).toUInt(nullptr, 16);
        m_stdScripts[id] = match.captured(2);
    }

    // Parse variables - match both 0x4xxx and 0x8xxx ranges
    QRegularExpression varRegex("0x([48][0-9A-Fa-f]{3}):\\s*\"(VAR_[^\"]+)\"");
    it = varRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        uint16_t id = match.captured(1).toUInt(nullptr, 16);
        m_variables[id] = match.captured(2);
    }

    // Parse flags - match any hex value followed by FLAG_ name
    // Support both 3 and 4 digit hex values (e.g., 0x820 or 0x0820)
    QRegularExpression flagRegex("0x([0-9A-Fa-f]{3,4}):\\s*\"(FLAG_[^\"]+)\"");
    it = flagRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        uint16_t id = match.captured(1).toUInt(nullptr, 16);
        m_flags[id] = match.captured(2);
    }
    qDebug() << "ScriptDisassembler: Loaded" << m_flags.size() << "flags";

    // Parse special functions
    QRegularExpression specialRegex("0x([0-9A-Fa-f]{4}):\\s*\"([A-Za-z][^\"]+)\"");
    it = specialRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        uint16_t id = match.captured(1).toUInt(nullptr, 16);
        QString name = match.captured(2);
        if (!name.startsWith("FLAG_") && !name.startsWith("VAR_") && !name.startsWith("STD_")) {
            m_specials[id] = name;
        }
    }

    // Parse var placeholders
    QRegularExpression placeholderRegex("0x0([0-6]):\\s*\"\\{([^}]+)\\}\"");
    it = placeholderRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        uint8_t id = match.captured(1).toUInt(nullptr, 16);
        m_varPlaceholders[id] = "{" + match.captured(2) + "}";
    }

    return true;
}

ScriptDisassembler::RamScriptHeader ScriptDisassembler::parseRamScriptHeader(const QByteArray &data) const
{
    RamScriptHeader header;
    header.isValid = false;

    if (data.size() < 4) {
        return header;
    }

    // RamScriptData header (without checksum): magic at offset 0
    header.magic = static_cast<uint8_t>(data[0]);
    header.mapGroup = static_cast<uint8_t>(data[1]);
    header.mapNum = static_cast<uint8_t>(data[2]);
    header.objectId = static_cast<uint8_t>(data[3]);
    header.isValid = (header.magic == 0x33);

    return header;
}

int ScriptDisassembler::parseArguments(const QByteArray &data, int offset, const QString &fmt,
                                       QVector<uint32_t> &args, QStringList &types)
{
    int pos = offset;
    args.clear();
    types.clear();

    for (QChar c : fmt) {
        if (pos >= data.size()) break;

        if (c == 'b') {
            args.append(static_cast<uint8_t>(data[pos]));
            types.append("b");
            pos += 1;
        } else if (c == 'w' || c == 'i' || c == 'p' || c == 'M' || c == 'v' || c == 'f') {
            if (pos + 1 < data.size()) {
                uint16_t val = static_cast<uint8_t>(data[pos]) |
                              (static_cast<uint8_t>(data[pos + 1]) << 8);
                args.append(val);
                types.append(QString(c));
                pos += 2;
            }
        } else if (c == 'd') {
            if (pos + 3 < data.size()) {
                uint32_t val = static_cast<uint8_t>(data[pos]) |
                              (static_cast<uint8_t>(data[pos + 1]) << 8) |
                              (static_cast<uint8_t>(data[pos + 2]) << 16) |
                              (static_cast<uint8_t>(data[pos + 3]) << 24);
                args.append(val);
                types.append("d");
                pos += 4;
            }
        }
    }

    return pos - offset;
}

QString ScriptDisassembler::formatArg(uint32_t value, const QString &type, int argIndex,
                                      uint8_t opcode) const
{
    if (type == "b") {
        // Check for condition code - show symbol
        if ((opcode == 0x06 || opcode == 0x07 || opcode == 0xBB || opcode == 0xBC) && argIndex == 0) {
            return m_conditions.value(static_cast<uint8_t>(value), QString("0x%1").arg(value, 2, 16, QChar('0')));
        }
        // Check for std script ID
        if ((opcode == 0x08 || opcode == 0x09 || opcode == 0x0A || opcode == 0x0B) && argIndex == 0) {
            return m_stdScripts.value(static_cast<uint8_t>(value), QString("STD_%1").arg(value));
        }
        return QString::number(value);
    }
    else if (type == "v") {
        // Variable reference - show name if known
        if (value >= 0x4000) {
            QString varName = m_variables.value(static_cast<uint16_t>(value), "");
            if (!varName.isEmpty()) {
                return varName;
            }
            return QString("VAR_0x%1").arg(value, 4, 16, QChar('0'));
        }
        // Immediate value (not a variable)
        return QString::number(value);
    }
    else if (type == "f") {
        // Flag reference - show name if known
        QString flagName = m_flags.value(static_cast<uint16_t>(value), "");
        if (!flagName.isEmpty()) {
            return flagName;
        }
        // Format: FLAG_0x{uppercase hex} to match Python
        return QString("FLAG_0x%1").arg(value, 4, 16, QChar('0')).toUpper().replace("FLAG_0X", "FLAG_0x");
    }
    else if (type == "i") {
        // Item ID - try ROM lookup first
        if (m_romReader && m_romReader->hasNameTables()) {
            QString itemName = m_romReader->getItemName(static_cast<uint16_t>(value));
            if (!itemName.isEmpty()) {
                return QString("ITEM_%1 (0x%2)").arg(itemName).arg(value, 4, 16, QChar('0'));
            }
        }
        return QString("ITEM_0x%1").arg(value, 4, 16, QChar('0')).toUpper();
    }
    else if (type == "p") {
        // Pokemon species - try ROM lookup first
        if (m_romReader && m_romReader->hasNameTables()) {
            QString pokemonName = m_romReader->getPokemonName(static_cast<uint16_t>(value));
            if (!pokemonName.isEmpty()) {
                return QString("SPECIES_%1 (%2)").arg(pokemonName.toUpper()).arg(value);
            }
        }
        return QString("SPECIES_%1").arg(value);
    }
    else if (type == "M") {
        // Move ID - try ROM lookup first
        if (m_romReader && m_romReader->hasNameTables()) {
            QString moveName = m_romReader->getMoveName(static_cast<uint16_t>(value));
            if (!moveName.isEmpty()) {
                return QString("MOVE_%1 (%2)").arg(moveName.toUpper().replace(" ", "_")).arg(value);
            }
        }
        return QString("MOVE_%1").arg(value);
    }
    else if (type == "w") {
        // Word - check context for how to format
        // Item quantity - show decimal
        if ((opcode == 0x44 || opcode == 0x45 || opcode == 0x46 || opcode == 0x47 ||
             opcode == 0x49 || opcode == 0x4A) && argIndex == 1) {
            return QString::number(value);
        }
        // setorcopyvar second arg - if < 0x4000, it's immediate decimal
        if (opcode == 0x1A && argIndex == 1 && value < 0x4000) {
            return QString::number(value);
        }
        // compare immediate value - show decimal for small values
        if ((opcode == 0x21 || opcode == 0x1C || opcode == 0x1F) && argIndex == 1 && value <= 255) {
            return QString::number(value);
        }
        return QString("0x%1").arg(value, 4, 16, QChar('0'));
    }
    else if (type == "d") {
        return QString("0x%1").arg(value, 8, 16, QChar('0'));
    }

    return QString::number(value);
}

QString ScriptDisassembler::generateComment(uint8_t opcode, const QString &name,
                                            const QVector<uint32_t> &args,
                                            const QStringList &types) const
{
    if (!m_commands.contains(opcode)) {
        return "Unknown command";
    }

    QString comment = m_commands[opcode].desc;
    QStringList extras;

    // Condition descriptions for conditional jumps/calls
    static const QHash<uint8_t, QString> conditionDescs = {
        {0x00, "less than"},
        {0x01, "equal to"},
        {0x02, "greater than"},
        {0x03, "less than or equal to"},
        {0x04, "greater than or equal to"},
        {0x05, "not equal to"}
    };

    // Add specific details based on command
    switch (opcode) {
        case 0x06: // goto_if
        case 0x07: // call_if
        case 0xBB: // vgoto_if
        case 0xBC: // vcall_if
            if (!args.isEmpty()) {
                uint8_t cond = static_cast<uint8_t>(args[0]);
                QString condDesc = conditionDescs.value(cond, "unknown");
                extras << QString("Condition: %1").arg(condDesc);
            }
            break;

        case 0x08: // gotostd
        case 0x09: // callstd
            if (!args.isEmpty()) {
                QString stdName = m_stdScripts.value(static_cast<uint8_t>(args[0]), "");
                if (!stdName.isEmpty()) {
                    extras << QString("-> %1").arg(stdName);
                }
            }
            break;

        case 0x25: // special
            if (!args.isEmpty()) {
                QString specialName = m_specials.value(static_cast<uint16_t>(args[0]), "");
                if (!specialName.isEmpty()) {
                    extras << QString("-> %1").arg(specialName);
                }
            }
            break;

        case 0x29: // setflag
            if (!args.isEmpty()) {
                uint16_t flagId = static_cast<uint16_t>(args[0]);
                QString flagName = m_flags.value(flagId, "");
                if (!flagName.isEmpty()) {
                    extras << QString("Sets %1 to TRUE").arg(flagName);
                    m_flagsFound.insert(flagId);
                } else {
                    m_flagsUnknown.insert(flagId);
                }
            }
            break;

        case 0x2A: // clearflag
            if (!args.isEmpty()) {
                uint16_t flagId = static_cast<uint16_t>(args[0]);
                QString flagName = m_flags.value(flagId, "");
                if (!flagName.isEmpty()) {
                    extras << QString("Sets %1 to FALSE").arg(flagName);
                    m_flagsFound.insert(flagId);
                } else {
                    m_flagsUnknown.insert(flagId);
                }
            }
            break;

        case 0x2B: // checkflag
            if (!args.isEmpty()) {
                uint16_t flagId = static_cast<uint16_t>(args[0]);
                QString flagName = m_flags.value(flagId, "");
                if (!flagName.isEmpty()) {
                    extras << QString("Checks %1").arg(flagName);
                    m_flagsFound.insert(flagId);
                } else {
                    m_flagsUnknown.insert(flagId);
                }
            }
            break;

        case 0x44: // additem
        case 0x45: // removeitem
        case 0x46: // checkitemspace
        case 0x47: // checkitem
            // Item operations - add item name if available
            if (!args.isEmpty() && m_romReader && m_romReader->hasNameTables()) {
                QString itemName = m_romReader->getItemName(static_cast<uint16_t>(args[0]));
                if (!itemName.isEmpty()) {
                    extras << QString("Item: %1").arg(itemName);
                }
            }
            break;

        case 0xBD: // vmessage
            if (!args.isEmpty() && m_inferredBase) {
                QString text = readEmbeddedString(args[0]);
                if (!text.isEmpty()) {
                    QString preview = text.left(50).replace("\n", " ").trimmed();
                    if (text.length() > 50) preview += "...";
                    extras << QString("Text: \"%1\"").arg(preview);

                    int offset = args[0] - m_inferredBase;
                    extras << QString("(offset 0x%1 in data)").arg(offset, 0, 16).toUpper();
                }
            }
            break;

        case 0xB8: // setvaddress
            extras << "IMPORTANT: Sets base address for virtual commands in RAM scripts";
            break;
    }

    if (!extras.isEmpty()) {
        comment += " | " + extras.join(" | ");
    }

    return comment;
}

void ScriptDisassembler::findJumpTargets(const QByteArray &data)
{
    m_labels.clear();
    int labelCount = 0;
    int offset = 0;

    while (offset < data.size()) {
        uint8_t opcode = static_cast<uint8_t>(data[offset]);

        if (opcode == 0x02) break; // end

        if (m_commands.contains(opcode)) {
            const CommandDef &cmd = m_commands[opcode];
            QVector<uint32_t> args;
            QStringList types;
            int argLen = parseArguments(data, offset + 1, cmd.args, args, types);

            // Check for jump/call targets
            // Regular goto/call (0x04, 0x05) - target is direct offset
            if ((opcode == 0x04 || opcode == 0x05) && !args.isEmpty()) {
                uint32_t target = args.last();
                if (target < static_cast<uint32_t>(data.size()) && !m_labels.contains(target)) {
                    m_labels[target] = QString("label_%1").arg(labelCount++);
                }
            }
            // Virtual goto/call (0xB9, 0xBA) - target is virtual address, convert to offset
            else if ((opcode == 0xB9 || opcode == 0xBA) && !args.isEmpty()) {
                uint32_t vaddr = args.last();
                if (m_inferredBase != 0 && vaddr >= m_inferredBase) {
                    uint32_t target = vaddr - m_inferredBase;
                    if (target < static_cast<uint32_t>(data.size()) && !m_labels.contains(target)) {
                        m_labels[target] = QString("label_%1").arg(labelCount++);
                    }
                }
            }
            // Regular conditional goto/call (0x06, 0x07) - target is direct offset
            else if ((opcode == 0x06 || opcode == 0x07) && args.size() >= 2) {
                uint32_t target = args.last();
                if (target < static_cast<uint32_t>(data.size()) && !m_labels.contains(target)) {
                    m_labels[target] = QString("label_%1").arg(labelCount++);
                }
            }
            // Virtual conditional goto/call (0xBB, 0xBC) - target is virtual address
            else if ((opcode == 0xBB || opcode == 0xBC) && args.size() >= 2) {
                uint32_t vaddr = args.last();
                if (m_inferredBase != 0 && vaddr >= m_inferredBase) {
                    uint32_t target = vaddr - m_inferredBase;
                    if (target < static_cast<uint32_t>(data.size()) && !m_labels.contains(target)) {
                        m_labels[target] = QString("label_%1").arg(labelCount++);
                    }
                }
            }

            offset += 1 + argLen;
        } else {
            offset += 1;
        }
    }
}

void ScriptDisassembler::inferBaseAddress(const QByteArray &data)
{
    m_inferredBase = 0;
    int offset = 0;

    while (offset < data.size()) {
        uint8_t opcode = static_cast<uint8_t>(data[offset]);
        if (opcode == 0x02) break;

        if (m_commands.contains(opcode)) {
            const CommandDef &cmd = m_commands[opcode];
            QVector<uint32_t> args;
            QStringList types;
            int argLen = parseArguments(data, offset + 1, cmd.args, args, types);

            // setvaddress sets the base
            if (opcode == 0xB8 && !args.isEmpty() && args[0] >= 0x08000000) {
                m_inferredBase = args[0] - offset;
                return;
            }

            offset += 1 + argLen;
        } else {
            offset += 1;
        }
    }
}

QString ScriptDisassembler::readEmbeddedString(uint32_t vaddr) const
{
    if (m_inferredBase == 0 || m_scriptData.isEmpty()) {
        return QString();
    }

    int offset = vaddr - m_inferredBase;
    if (offset < 0 || offset >= m_scriptData.size()) {
        return QString();
    }

    return decodeGen3String(m_scriptData, offset);
}

QString ScriptDisassembler::decodeGen3String(const QByteArray &data, int offset, int maxLen) const
{
    QString result;
    int i = offset;
    int end = qMin(offset + maxLen, data.size());

    while (i < end) {
        uint8_t byte = static_cast<uint8_t>(data[i]);

        if (byte == 0xFF) break; // Terminator

        if (byte == 0xFD && i + 1 < end) { // Variable placeholder
            uint8_t varId = static_cast<uint8_t>(data[i + 1]);
            result += m_varPlaceholders.value(varId, QString("{VAR_%1}").arg(varId, 2, 16, QChar('0')));
            i += 2;
            continue;
        }

        if (byte == 0xFC && i + 1 < end) { // Control code
            i += 2;
            continue;
        }

        QString ch = m_gen3Charset.value(byte, "");
        if (ch == "\\n" || ch == "\\l") {
            result += "\n";
        } else if (ch == "\\p") {
            result += "\n\n";
        } else if (!ch.isEmpty() && !ch.startsWith("\\")) {
            result += ch;
        }

        i++;
    }

    return result;
}

QVector<ScriptDisassembler::EmbeddedString> ScriptDisassembler::findEmbeddedStrings() const
{
    QVector<EmbeddedString> strings;
    QSet<uint32_t> seenAddrs;

    for (const ScriptInstruction &instr : m_instructions) {
        if (instr.opcode == 0xBD) { // vmessage
            if (!instr.args.isEmpty()) {
                uint32_t vaddr = instr.args[0];
                if (!seenAddrs.contains(vaddr)) {
                    seenAddrs.insert(vaddr);
                    QString text = readEmbeddedString(vaddr);
                    if (!text.isEmpty()) {
                        EmbeddedString es;
                        es.vaddr = vaddr;
                        es.offset = vaddr - m_inferredBase;
                        es.text = text;
                        strings.append(es);
                    }
                }
            }
        }
    }

    // Sort by offset
    std::sort(strings.begin(), strings.end(),
              [](const EmbeddedString &a, const EmbeddedString &b) {
                  return a.offset < b.offset;
              });

    return strings;
}

QString ScriptDisassembler::disassemble(const QByteArray &data, bool showComments,
                                        bool showBytes, bool showOffsets)
{
    if (m_commands.isEmpty()) {
        return "; ERROR: Command definitions not loaded\n";
    }

    m_instructions.clear();
    m_scriptData = data;
    m_flagsFound.clear();
    m_flagsUnknown.clear();

    // Infer base address first (needed for virtual address conversion)
    inferBaseAddress(data);
    // Then find jump targets (can now convert virtual addresses to offsets)
    findJumpTargets(data);

    // First pass: parse all instructions
    int offset = 0;
    while (offset < data.size()) {
        uint8_t opcode = static_cast<uint8_t>(data[offset]);

        ScriptInstruction instr;
        instr.offset = offset;
        instr.opcode = opcode;
        instr.label = m_labels.value(offset, "");

        if (opcode == 0x02) { // end
            instr.name = "end";
            instr.rawBytes = data.mid(offset, 1);
            instr.comment = "Terminates script execution";
            m_instructions.append(instr);
            break;
        }

        if (m_commands.contains(opcode)) {
            const CommandDef &cmd = m_commands[opcode];
            instr.name = cmd.name;

            int argLen = parseArguments(data, offset + 1, cmd.args, instr.args, instr.argTypes);
            instr.rawBytes = data.mid(offset, 1 + argLen);
            instr.comment = generateComment(opcode, cmd.name, instr.args, instr.argTypes);

            m_instructions.append(instr);
            offset += 1 + argLen;
        } else {
            // Unknown opcode
            instr.name = "db";
            instr.args.append(opcode);
            instr.argTypes.append("b");
            instr.rawBytes = data.mid(offset, 1);
            instr.comment = QString("Unknown opcode 0x%1").arg(opcode, 2, 16, QChar('0'));
            m_instructions.append(instr);
            offset += 1;
        }
    }

    // Build output header
    QStringList output;
    output << "; Pokemon Gen 3 Mystery Event Script Disassembly";

    if (m_romReader && m_romReader->isLoaded()) {
        // Format ROM name like Python: "FireRed (US)" instead of "FireRed_1.0"
        QString romName = m_romReader->versionName();
        if (romName.startsWith("FireRed")) {
            romName = "FireRed (US)";
        } else if (romName.startsWith("LeafGreen")) {
            romName = "LeafGreen (US)";
        } else if (romName.startsWith("Emerald")) {
            romName = "Emerald (US)";
        }
        output << QString("; ROM: %1").arg(romName);
    }

    if (m_inferredBase) {
        output << QString("; Inferred virtual base address: 0x%1").arg(m_inferredBase, 8, 16, QChar('0'));
    }

    output << QString("; Total instructions: %1").arg(m_instructions.size());
    output << QString("; Labels found: %1").arg(m_labels.size());
    output << QString("; Flags resolved: %1").arg(m_flagsFound.size());
    if (!m_flagsUnknown.isEmpty()) {
        output << QString("; Unknown flags: %1").arg(m_flagsUnknown.size());
    }
    output << ";";
    output << "; Legend:";
    output << ";   VAR_0x4xxx = Script variables (0x4000-0x40xx)";
    output << ";   FLAG_0xxxx = Game flags";
    output << ";   @label_N   = Jump/call target";
    output << ";   STD_xxx    = Standard script ID";
    if (m_romReader && m_romReader->hasNameTables()) {
        output << ";   ITEM_xxx   = Item name from ROM";
        output << ";   SPECIES_xxx = Pokemon species from ROM";
        output << ";   MOVE_xxx   = Move name from ROM";
    }
    output << "";
    output << ".script_start:";

    // Output instructions
    for (const ScriptInstruction &instr : m_instructions) {
        // Add label line
        if (!instr.label.isEmpty()) {
            output << QString("\n%1:").arg(instr.label);
        }

        QString line;
        if (showOffsets) {
            line += QString("  %1:").arg(instr.offset, 4, 16, QChar('0'));
        }

        if (showBytes) {
            QString hexBytes;
            for (int i = 0; i < qMin(instr.rawBytes.size(), 8); i++) {
                hexBytes += QString("%1 ").arg(static_cast<uint8_t>(instr.rawBytes[i]), 2, 16, QChar('0')).toUpper();
            }
            line += QString("  %1").arg(hexBytes, -24);
        }

        // Format arguments
        QStringList formattedArgs;
        for (int i = 0; i < instr.args.size(); i++) {
            formattedArgs << formatArg(instr.args[i], instr.argTypes[i], i, instr.opcode);
        }

        QString argsStr = formattedArgs.join(", ");
        line += QString("  %1 %2").arg(instr.name.leftJustified(20)).arg(argsStr);

        if (showComments && !instr.comment.isEmpty()) {
            line += QString(" # %1").arg(instr.comment);
        }

        output << line;
    }

    output << "\n.script_end";

    // Add embedded strings section
    if (m_inferredBase && !m_scriptData.isEmpty()) {
        QVector<EmbeddedString> embeddedStrings = findEmbeddedStrings();
        if (!embeddedStrings.isEmpty()) {
            output << "";
            output << "; =========================================";
            output << "; EMBEDDED STRINGS";
            output << "; =========================================";

            for (const EmbeddedString &es : embeddedStrings) {
                output << ";";
                output << QString("; Address 0x%1 (offset 0x%2):")
                              .arg(es.vaddr, 8, 16, QChar('0'))
                              .arg(es.offset, 2, 16, QChar('0'));

                // Format multi-line strings nicely
                QStringList lines = es.text.split('\n');
                for (const QString &textLine : lines) {
                    if (!textLine.isEmpty()) {
                        output << QString(";   \"%1\"").arg(textLine);
                    }
                }
            }
        }
    }

    return output.join("\n");
}

QString ScriptDisassembler::disassembleRamScript(const QByteArray &data, bool showComments,
                                                  bool showBytes, bool showOffsets)
{
    if (data.size() < 4) {
        return "; ERROR: Data too small for RamScript\n";
    }

    RamScriptHeader header = parseRamScriptHeader(data);

    QStringList output;
    output << "; =========================================";
    output << "; RamScriptData Header";
    output << "; =========================================";
    output << QString(";   Magic: 0x%1 (%2)").arg(header.magic, 2, 16, QChar('0'))
                                             .arg(header.isValid ? "valid" : "INVALID");
    output << QString(";   Map Group: %1 (0x%2)").arg(header.mapGroup)
                                                 .arg(header.mapGroup, 2, 16, QChar('0'));
    output << QString(";   Map Num: %1 (0x%2)").arg(header.mapNum)
                                               .arg(header.mapNum, 2, 16, QChar('0'));
    output << QString(";   Object ID: %1 (0x%2)").arg(header.objectId)
                                                 .arg(header.objectId, 2, 16, QChar('0'));
    output << ";";

    // Script data starts at offset 4 (after header)
    QByteArray scriptData = data.mid(4);

    output << disassemble(scriptData, showComments, showBytes, showOffsets);

    return output.join("\n");
}
