/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ParserCatalog.h"

#include <string>

#include <tree_sitter/tree-sitter-cpp.h>
#include <tree_sitter/tree-sitter-ini.h>
#include <tree_sitter/tree-sitter-json.h>
#include <tree_sitter/tree-sitter-yaml.h>

#include "query/cpp_query.h"
#include "query/ini_query.h"
#include "query/json_query.h"
#include "query/yaml_query.h"


const std::unordered_map<HighLightId, ParserDescriptor> &ParserCatalog::getDescriptors() {
    static const std::unordered_map<HighLightId, ParserDescriptor> descriptors = {
        { HighLightId::Json, {
            .language           = tree_sitter_json(),
            .name               = "JSON",
            .argument_value     = "json",
            .files_format       = {".json", ".JSON"},
            .query_source       = json_query,
            .capture_tokens     = {
                {"keyword",  TokenId::Keyword},
                {"string",   TokenId::String},
                {"number",   TokenId::Number},
                {"comment",  TokenId::Comment},
                {"constant", TokenId::Constant}
            }
        }},
        { HighLightId::Cpp, {
            .language           = tree_sitter_cpp(),
            .name               = "C++",
            .argument_value     = "cpp",
            .files_format       = {".c", ".C", ".cc", ".CC", ".cpp", ".CPP", ".h", ".H", ".hpp", ".HPP", ".cxx", ".CXX"},
            .query_source       = cpp_query,
            .capture_tokens     = {
                {"keyword",      TokenId::Keyword},
                {"statement",    TokenId::Statement},
                {"string",       TokenId::String},
                {"number",       TokenId::Number},
                {"comment",      TokenId::Comment},
                {"preprocessor", TokenId::Preprocessor},
                {"type",         TokenId::Type},
                {"constant",     TokenId::Constant},
                {"function",     TokenId::Function},
                {"variable",     TokenId::Variable}
            }
        }},
        { HighLightId::Ini, {
            .language           = tree_sitter_ini(),
            .name               = "INI",
            .argument_value     = "ini",
            .files_format       = {".ini", ".INI"},
            .query_source       = ini_query,
            .capture_tokens     = {
                {"comment", TokenId::Comment},
                {"type",    TokenId::Type},
                {"keyword", TokenId::Keyword},
                {"string",  TokenId::String}
            }
        }},
        { HighLightId::Yaml, {
            .language           = tree_sitter_yaml(),
            .name               = "YAML",
            .argument_value     = "yaml",
            .files_format       = {".yaml", ".YAML", ".yml", ".YML"},
            .query_source       = yaml_query,
            .capture_tokens     = {
                {"keyword",      TokenId::Keyword},
                {"string",       TokenId::String},
                {"number",       TokenId::Number},
                {"constant",     TokenId::Constant},
                {"comment",      TokenId::Comment},
                {"type",         TokenId::Type},
                {"preprocessor", TokenId::Preprocessor}
            }
        }}
    };

    return descriptors;
}

const std::unordered_map<HighLightId, Parser> &ParserCatalog::getParsers() {
    static const std::unordered_map<HighLightId, Parser> parsers = [] {
        std::unordered_map<HighLightId, Parser> catalog;
        for (const auto &[id, descriptor] : getDescriptors()) {
            catalog.try_emplace(id, descriptor);
        }

        return catalog;
    }();

    return parsers;
}

HighLightId ParserCatalog::findModeByExtension(const std::string_view extension) {
    for (const auto &[id, descriptor] : getDescriptors()) {
        if (descriptor.files_format.contains(std::string(extension))) {
            return id;
        }
    }

    return HighLightId::None;
}
