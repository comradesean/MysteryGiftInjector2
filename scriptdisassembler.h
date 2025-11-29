/**
 * @file scriptdisassembler.h
 * @brief GBA script bytecode disassembler for Pokemon Gen3 event scripts.
 *
 * This file provides the ScriptDisassembler class which converts raw script
 * bytecode into human-readable assembly-like output.
 *
 * ## Script Format
 *
 * Pokemon Gen3 scripts use a bytecode format with:
 * - Single-byte opcodes (0x00-0xFF)
 * - Variable-length arguments (bytes, words, dwords)
 * - Special argument types for variables, flags, and pointers
 *
 * ## RamScript / GREEN MAN Scripts
 *
 * Mystery Gift scripts are stored as "RamScripts" with a 4-byte header:
 * - Byte 0: Magic (0x00)
 * - Byte 1: Map Group
 * - Byte 2: Map Number
 * - Byte 3: Object ID
 *
 * The actual script bytecode follows this header.
 *
 * ## Argument Types
 *
 * The disassembler supports these argument formats:
 * - **b**: Byte (1 byte)
 * - **w**: Word (2 bytes, little-endian)
 * - **d**: Dword (4 bytes, little-endian)
 * - **v**: Variable reference (2 bytes)
 * - **f**: Flag reference (2 bytes)
 * - **i**: Item ID (2 bytes)
 * - **p**: Pokemon species (2 bytes)
 * - **M**: Movement data (variable length)
 *
 * ## Command Definitions
 *
 * Commands are loaded from script_commands.yaml which defines:
 * - Opcode to command name mapping
 * - Argument format strings
 * - Command descriptions for comments
 *
 * @see script_commands.yaml for command definitions
 * @see script_data.yaml for flags, specials, and constants
 *
 * @author ComradeSean
 * @version 1.0
 */

#ifndef SCRIPTDISASSEMBLER_H
#define SCRIPTDISASSEMBLER_H

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QSet>
#include <QVector>
#include <QStringList>

// Forward declaration
class GBAROReader;

/**
 * @struct ScriptInstruction
 * @brief Represents a single disassembled instruction.
 */
struct ScriptInstruction {
    int offset;             // Byte offset in script
    uint8_t opcode;         // Raw opcode
    QString name;           // Command name
    QVector<uint32_t> args; // Raw argument values
    QStringList argTypes;   // Argument types (b, w, d, v, f, i, p, M)
    QByteArray rawBytes;    // Raw instruction bytes
    QString comment;        // Generated comment
    QString label;          // Label if jump target
};

/// Command definition loaded from YAML
struct CommandDef {
    QString name;
    QString args;
    QString desc;
};

/// Disassembles Pokemon Gen 3 Mystery Event scripts (RamScript/GMScript)
class ScriptDisassembler
{
public:
    ScriptDisassembler();
    ~ScriptDisassembler();

    /// Load command definitions from YAML file
    bool loadCommandDefinitions(const QString &yamlPath, QString &error);

    /// Load script reference data (flags, specials, etc.) from YAML
    bool loadScriptData(const QString &yamlPath, QString &error);

    /// Set ROM reader for name resolution (optional)
    void setRomReader(GBAROReader *reader) { m_romReader = reader; }

    /// Disassemble script data
    /// @param data Raw script bytecode (without RamScript header)
    /// @param showComments Include comments in output
    /// @param showBytes Show raw hex bytes
    /// @param showOffsets Show byte offsets
    QString disassemble(const QByteArray &data, bool showComments = true,
                       bool showBytes = true, bool showOffsets = true);

    /// Disassemble full RamScript (with header)
    /// @param data Full RamScript data including 4-byte header
    QString disassembleRamScript(const QByteArray &data, bool showComments = true,
                                 bool showBytes = true, bool showOffsets = true);

    /// Get parsed header info from RamScript
    struct RamScriptHeader {
        uint8_t magic;
        uint8_t mapGroup;
        uint8_t mapNum;
        uint8_t objectId;
        bool isValid;
    };
    RamScriptHeader parseRamScriptHeader(const QByteArray &data) const;

    /// Get list of instructions (after disassembly)
    const QVector<ScriptInstruction>& instructions() const { return m_instructions; }

    /// Check if commands are loaded
    bool isReady() const { return !m_commands.isEmpty(); }

private:
    // Parse arguments based on format string
    int parseArguments(const QByteArray &data, int offset, const QString &fmt,
                      QVector<uint32_t> &args, QStringList &types);

    // Format an argument for display
    QString formatArg(uint32_t value, const QString &type, int argIndex,
                     uint8_t opcode) const;

    // Generate comment for instruction
    QString generateComment(uint8_t opcode, const QString &name,
                           const QVector<uint32_t> &args,
                           const QStringList &types) const;

    // Find jump targets for labeling
    void findJumpTargets(const QByteArray &data);

    // Infer virtual base address from setvaddress/vgoto commands
    void inferBaseAddress(const QByteArray &data);

    // Read embedded string at virtual address
    QString readEmbeddedString(uint32_t vaddr) const;

    // Decode Gen3 encoded string
    QString decodeGen3String(const QByteArray &data, int offset, int maxLen = 200) const;

    // Find all embedded strings referenced by vmessage commands
    struct EmbeddedString {
        uint32_t vaddr;
        int offset;
        QString text;
    };
    QVector<EmbeddedString> findEmbeddedStrings() const;

    // Command definitions (opcode -> CommandDef)
    QHash<uint8_t, CommandDef> m_commands;

    // Script reference data
    QHash<uint8_t, QString> m_conditions;     // condition code -> symbol
    QHash<uint8_t, QString> m_stdScripts;     // std ID -> name
    QHash<uint16_t, QString> m_variables;     // var ID -> name
    QHash<uint16_t, QString> m_flags;         // flag ID -> name
    QHash<uint16_t, QString> m_specials;      // special ID -> name
    QHash<uint8_t, QString> m_varPlaceholders; // placeholder ID -> string

    // Gen3 character encoding
    QHash<uint8_t, QString> m_gen3Charset;

    // Jump targets (offset -> label name)
    QHash<int, QString> m_labels;

    // Disassembled instructions
    QVector<ScriptInstruction> m_instructions;

    // Script data for string extraction
    QByteArray m_scriptData;

    // Inferred base address for virtual commands
    uint32_t m_inferredBase;

    // ROM reader for name resolution
    GBAROReader *m_romReader;

    // Track flags found during disassembly
    mutable QSet<uint16_t> m_flagsFound;      // Flags that were resolved
    mutable QSet<uint16_t> m_flagsUnknown;    // Flags that weren't in database

    // Initialize Gen3 charset
    void initGen3Charset();
};

#endif // SCRIPTDISASSEMBLER_H
