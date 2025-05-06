#include "HighLighter.h"

#include <ranges>
#include <utf8.h>

#include <tree_sitter/tree-sitter-cpp.h>
#include <tree_sitter/tree-sitter-json.h>

#include "mapper/CppMapper.h"
#include "mapper/JsonMapper.h"


HighLighter::HighLighter()
    : p_ts_parser(nullptr),
      p_ts_tree(nullptr),
      m_input(this, inputCallback, TSInputEncodingUTF16LE),
      m_high_light(HighLightId::None),
      p_current_parser(nullptr),
      m_cb(nullptr) {
}

void HighLighter::create(CommandManager& commandManager) {
    // Create the tree-sitter parser
    p_ts_parser = ts_parser_new();

    // Register built-in parsers and a command to change the highlight mode
    registerParsers();
    registerHlCommand(commandManager);
}

void HighLighter::destroy() {
    // Cleanup tree
    if (p_ts_tree != nullptr) {
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }

    // Cleanup parser
    ts_parser_delete(p_ts_parser);
    p_ts_parser = nullptr;
}

void HighLighter::setMode(const HighLightId highLight) {
    m_high_light = highLight;

    if (highLight == HighLightId::None) {
        // There is no need to keep the old mode
        ts_parser_set_language(p_ts_parser, nullptr);
        p_current_parser = nullptr;
    } else {
        // Set new mode
        const auto& parser = m_parsers.at(highLight);
        ts_parser_set_language(p_ts_parser, parser.language);
        p_current_parser = &parser;
    }

    if (p_ts_tree != nullptr) {
        // Delete the old tree, as tree-sitter may need to reparse it from scratch
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }
}

void HighLighter::setMode(const std::string_view extension) {
    for (const auto& [id, parser] : m_parsers) {
        if (parser.files_format.contains(extension.data())) {
            // Found a match!
            setMode(id);
            return;
        }
    }

    // Fall back to none
    setMode(HighLightId::None);
}

std::string_view HighLighter::getModeString() const {
    if (m_high_light == HighLightId::None) {
        // None is TEXT
        return "TEXT";
    }

    return p_current_parser->name;
}

void HighLighter::setInput(const Cursor& cursor) {
    // Invalidate the old parsed tree as tree-sitter will eventually need to reparse it
    if (p_ts_tree != nullptr) {
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }

    // Set the input to the cursor content
    // todo: this is not really safe to keep a reference inside the lambda, move it
    m_cb = [&cursor](const uint32_t line, const uint32_t column) -> std::optional<std::u16string_view> {
        // Return the line starting from line, column
        const auto line_count = cursor.getLineCount();
        if (line > line_count) {
            // At line_count +1, then there is no more data to process
            return std::nullopt;
        }

        const auto string = cursor.getString(line);
        if (line < line_count && column < string.length()) {
            // Not at the very end of the buffer,
            // and not at the end of a line, then there is nothing more to do
            return cursor.getString(line).substr(column);
        }

        // Tells the parser there is more
        return u"\n";
    };
}

void HighLighter::parse() {
    if (p_ts_parser != nullptr && m_cb != nullptr) {
        p_ts_tree = ts_parser_parse(p_ts_parser, p_ts_tree, m_input);
    }
}

void HighLighter::edit(const BufferEdit& edit) const {
    if (p_ts_tree != nullptr) {
        // This just converts and relays the object coming from the cursor
        const auto ts_edit = TSInputEdit {
            .start_byte = edit.start_byte,
            .old_end_byte = edit.old_end_byte,
            .new_end_byte = edit.new_end_byte,
            .start_point = TSPoint(edit.start.line, edit.start.column * sizeof(char16_t)),
            .old_end_point = TSPoint(edit.old_end.line, edit.old_end.column * sizeof(char16_t)),
            .new_end_point = TSPoint(edit.new_end.line, edit.new_end.column * sizeof(char16_t))
        };

        ts_tree_edit(p_ts_tree, &ts_edit);
    }
}

bool HighLighter::isSupported(const std::string_view extension) const {
    for (const auto &parser: std::views::values(m_parsers)) {
        if (parser.files_format.contains(extension.data())) {
            return true;
        }
    }

    if (extension == ".txt" || extension == ".TXT") {
        // So the set_hl_command can work with "txt"
        return true;
    }

    return false;
}

void HighLighter::getParserNames(const ItemCallback<char>& callback) const {
    // Add a "txt" item, for HighLightId::None
    callback("txt");
    for (const auto &parser: std::views::values(m_parsers)) {
        callback(parser.argument_value);
    }
}

TokenId HighLighter::getHighLightAtPosition(const uint32_t line, const uint32_t column) const {
    if (p_ts_tree == nullptr || m_high_light == HighLightId::None) {
        return TokenId::None;
    }

    // Find the node at the line and column position
    const auto& point = TSPoint(line, column * sizeof(char16_t));
    const auto& root_node = ts_tree_root_node(p_ts_tree);
    const auto& target_node = ts_node_descendant_for_point_range(root_node, point, point);
    if (ts_node_is_null(target_node)) {
        return TokenId::None;
    }

    // todo: Uncomment for debug purpose
    // std::cout << "node: " << ts_node_type(target_node) << " " << ts_node_symbol(target_node) << " " << std::endl;
    const auto symbol = ts_node_symbol(target_node);

    // Return the mapped token
    return p_current_parser->mapper_function(symbol);
}

const char* HighLighter::inputCallback(void *payload, const uint32_t byteIndex, const TSPoint position, uint32_t *bytesRead) {
    // This input callback is working with logical positions, so byteIndex is not needed
    (void) byteIndex;

    // Get the HighLighter instance
    const auto* self = static_cast<HighLighter*>(payload);

    // Multiply and divide column according to char size, we are working with char16_t (2 bytes)
    const auto line = position.row;
    const auto column = position.column / sizeof(char16_t);
    if (const auto optional_line = self->m_cb(line, column); optional_line.has_value()) {
        // We got some data
        *bytesRead = optional_line->length() * sizeof(char16_t);
        return reinterpret_cast<const char*>(optional_line->data());
    }

    // Tells tree-sitter there is no more data to process at this location
    *bytesRead = 0;
    return nullptr;
}

void HighLighter::registerHlCommand(CommandManager& commandManager) {
    // Register a command to change the highlight mode
    commandManager.registerCommand("set_hl_mode",
        [&](const Cursor& cursor, const std::vector<std::u16string_view>& args) -> std::optional<std::u16string> {
            (void) cursor;
            if (args.size() != 1) {
                return u"Usage: set_hl_mode <mode>";
            }

            // Developers are lazy, so let's prepend a dot to make it work flawlessly
            const auto extension = std::string(".").append(utf8::utf16to8(args[0]));
            if (!isSupported(extension)) {
                return std::u16string(u"Unsupported highlight mode: ").append(args[0]);
            }

            setMode(extension);
            return std::nullopt;
        },
        [&](const int32_t argumentIndex, const std::string_view input, const ItemCallback<char>& itemCallback) {
            // Ignore input
            (void) input;
            if (argumentIndex != 0) {
                // Only auto-complete the first argument (mode)
                return;
            }

            getParserNames(itemCallback);
        });
}

void HighLighter::registerParsers() {
    m_parsers.insert(
        { HighLightId::Json, {
                .language           = tree_sitter_json(),
                .name               = "JSON",
                .argument_value     = "json",
                .files_format       = {".json", ".JSON"},
                .mapper_function    = [](const uint16_t symbol) {
                    return mapJsonToken(symbol);
                }
            }
        });

    m_parsers.insert(
        { HighLightId::Cpp, {
                .language           = tree_sitter_cpp(),
                .name               = "C",
                .argument_value     = "c",
                .files_format       = {".c", ".C", ".cc", ".CC", ".cpp", ".CPP", ".h", ".H", ".hpp", ".HPP", ".cxx", ".CXX"},
                .mapper_function    = [](const uint16_t symbol) {
                    return mapCppToken(symbol);
                }
            }
        });
}
