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
#include <memory>
#include <string>
#include <vector>

#include "TestSupport.h"

#include "prompt/PromptState.h"


/**
 * @brief The registry a PromptState is built on, keeping what it registers so a case can drive it.
 *
 * CVarCommand is what normally sits here: it holds the CVar and the change callback, and fires the
 * callback after a successful set. This does the same, which is what makes the dim_max_history
 * trimming reachable at all.
 */
class RecordingCVarRegistry final : public CVarRegistry {
private:
    /** @brief One registered CVar and the callback registered alongside it. */
    struct Entry final {
        std::u16string name;            ///< Name the CVar was registered under.
        std::shared_ptr<CVar> cvar;     ///< The CVar itself.
        CVarCallback callback;          ///< Callback to fire after a successful change.
    };

    /** Everything registered, in registration order. */
    std::vector<Entry> m_entries;

    /**
     * @brief Registers a CVar and keeps its callback.
     *
     * @param name Variable name.
     * @param cvar The CVar instance.
     * @param callback Callback invoked on changes.
     */
    void registerCvar(const std::u16string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) override {
        m_entries.emplace_back(Entry{ .name = std::u16string(name), .cvar = std::move(cvar), .callback = callback });
    }

    /**
     * @brief Finds a registered entry by name.
     *
     * @param name The name to look for.
     * @return The entry, or nullptr when nothing was registered under that name.
     */
    [[nodiscard]] const Entry *find(const std::u16string_view name) const {
        for (const auto &entry : m_entries) {
            if (entry.name == name) {
                return &entry;
            }
        }

        return nullptr;
    }

public:
    /**
     * @brief Tells whether something was registered under a name.
     *
     * @param name The name to look for.
     * @return true when the name is registered.
     */
    [[nodiscard]] bool contains(const std::u16string_view name) const {
        return find(name) != nullptr;
    }

    /**
     * @brief Sets a registered CVar from its string form and fires its callback, as CVarCommand does.
     *
     * @param name The registered name.
     * @param value The value to set.
     */
    void set(const std::u16string_view name, const std::u16string_view value) {
        const auto *entry = find(name);
        REQUIRE(entry != nullptr);

        const auto arguments = std::vector<std::u16string_view>{ value };
        REQUIRE_FALSE(entry->cvar->setValueFromStrings(arguments).has_value());

        if (entry->callback) {
            entry->callback();
        }
    }

    /**
     * @brief Reads a registered CVar back in its string form.
     *
     * @param name The registered name.
     * @return The value the CVar holds.
     */
    [[nodiscard]] std::u16string valueOf(const std::u16string_view name) const {
        const auto *entry = find(name);
        REQUIRE(entry != nullptr);

        return entry->cvar->getStringValue();
    }
};


/**
 * @brief Fills a prompt's history in order, oldest first.
 *
 * @param state The state to fill.
 * @param count The number of commands to add; each one is its own text.
 */
static void addHistoryRun(PromptState &state, const int32_t count) {
    for (auto index = 0; index < count; ++index) {
        const auto ordinal = std::to_string(index);
        state.addHistory(std::u16string(u"cmd").append(ordinal.begin(), ordinal.end()));
    }
}

/**
 * @brief Reads the whole history by stepping back through it, newest first.
 *
 * Stepping is the only way to read it, which is the point: the ring is what the Up and Down keys
 * walk, so the walk is what has to be right.
 *
 * @param state The state to read; its navigation index is left at the oldest entry.
 * @return The entries, newest first.
 */
static std::vector<std::u16string> historyNewestFirst(PromptState &state) {
    state.clearHistoryIndex();

    auto entries = std::vector<std::u16string>{};
    const auto count = state.getHistoryCount();
    for (auto step = 0; step < count; ++step) {
        entries.emplace_back(state.previousHistory());
    }

    return entries;
}


TEST_CASE("a fresh prompt has nothing to navigate") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    CHECK(state.getHistoryCount() == 0);
    CHECK(state.getHistoryIndex() == -1);
    CHECK_FALSE(state.isNavigatingHistory());

    // Pressing Up or Down on an empty history must answer without moving the index off -1, or the
    // next press would index into an empty list
    CHECK(state.previousHistory().empty());
    CHECK(state.nextHistory().empty());
    CHECK(state.getHistoryIndex() == -1);
}

TEST_CASE("the prompt registers its history size as a cvar") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    CHECK(registry.contains(u"dim_max_history"));
    CHECK(registry.valueOf(u"dim_max_history") == std::u16string(u"32"));
}

TEST_CASE("the first history step lands on the most recent command in either direction") {
    auto registry = RecordingCVarRegistry();

    // Up and Down both start from the newest entry: the index is invalid until the first press, and
    // both branches have to resolve it the same way
    auto stepping_back = PromptState(registry);
    addHistoryRun(stepping_back, 3);
    CHECK(stepping_back.previousHistory() == std::u16string_view(u"cmd2"));

    auto stepping_forward = PromptState(registry);
    addHistoryRun(stepping_forward, 3);
    CHECK(stepping_forward.nextHistory() == std::u16string_view(u"cmd2"));
}

TEST_CASE("stepping back walks to older commands and wraps at the oldest") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);
    addHistoryRun(state, 3);

    CHECK(state.previousHistory() == std::u16string_view(u"cmd2"));
    CHECK(state.previousHistory() == std::u16string_view(u"cmd1"));
    CHECK(state.previousHistory() == std::u16string_view(u"cmd0"));

    // The negative wrap: index 0 steps to the newest, not to -1
    CHECK(state.previousHistory() == std::u16string_view(u"cmd2"));
}

TEST_CASE("stepping forward walks to newer commands and wraps at the newest") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);
    addHistoryRun(state, 3);

    CHECK(state.nextHistory() == std::u16string_view(u"cmd2"));
    CHECK(state.nextHistory() == std::u16string_view(u"cmd0"));
    CHECK(state.nextHistory() == std::u16string_view(u"cmd1"));
    CHECK(state.nextHistory() == std::u16string_view(u"cmd2"));
}

TEST_CASE("running a command ends the navigation") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);
    addHistoryRun(state, 3);

    (void) state.previousHistory();
    CHECK(state.isNavigatingHistory());

    // The command just run becomes the new starting point, so the next Up must return it
    state.addHistory(u"quit");
    CHECK(state.getHistoryIndex() == -1);
    CHECK_FALSE(state.isNavigatingHistory());
    CHECK(state.previousHistory() == std::u16string_view(u"quit"));
}

TEST_CASE("the history drops its oldest commands once it is full") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    addHistoryRun(state, 40);

    CHECK(state.getHistoryCount() == 32);

    // The newest survive and the oldest eight are gone
    const auto entries = historyNewestFirst(state);
    REQUIRE(entries.size() == 32);
    CHECK(entries.front() == std::u16string(u"cmd39"));
    CHECK(entries.back() == std::u16string(u"cmd8"));
}

TEST_CASE("lowering the history size trims from the oldest end") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);
    addHistoryRun(state, 20);

    registry.set(u"dim_max_history", u"10");

    CHECK(state.getHistoryCount() == 10);

    const auto entries = historyNewestFirst(state);
    REQUIRE(entries.size() == 10);
    CHECK(entries.front() == std::u16string(u"cmd19"));
    CHECK(entries.back() == std::u16string(u"cmd10"));
}

TEST_CASE("the history size is clamped into a usable range") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);
    addHistoryRun(state, 20);

    // Too small: clamped to 8, and the clamp is what the trim then uses — asserting the stored
    // value alone would not prove the history followed it
    registry.set(u"dim_max_history", u"3");
    CHECK(registry.valueOf(u"dim_max_history") == std::u16string(u"8"));
    CHECK(state.getHistoryCount() == 8);

    // Too large: clamped to 255, and a history under that size is left alone
    registry.set(u"dim_max_history", u"1000");
    CHECK(registry.valueOf(u"dim_max_history") == std::u16string(u"255"));
    CHECK(state.getHistoryCount() == 8);
}

TEST_CASE("a prompt with no completion answers without indexing into one") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    CHECK(state.getCompletionCount() == 0);
    CHECK(state.getCurrentCompletion().empty());
    CHECK(state.nextCompletion().empty());
    CHECK(state.previousCompletion().empty());
    CHECK(state.getCompletionIndex() == 0);
}

TEST_CASE("sorting the completions orders them and rewinds to the first") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    state.addCompletion(u"cvar");
    state.addCompletion(u"bind");
    state.addCompletion(u"quit");
    (void) state.nextCompletion();

    state.sortCompletions();

    // Tab offers the first candidate, so the index has to come back to it after a sort
    CHECK(state.getCompletionIndex() == 0);
    CHECK(state.getCurrentCompletion() == std::u16string_view(u"bind"));
}

TEST_CASE("the completion ring wraps in both directions") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    state.addCompletion(u"bind");
    state.addCompletion(u"cvar");
    state.addCompletion(u"quit");

    CHECK(state.nextCompletion() == std::u16string_view(u"cvar"));
    CHECK(state.nextCompletion() == std::u16string_view(u"quit"));
    CHECK(state.nextCompletion() == std::u16string_view(u"bind"));

    // And the negative wrap on the way back
    CHECK(state.previousCompletion() == std::u16string_view(u"quit"));
    CHECK(state.previousCompletion() == std::u16string_view(u"cvar"));
}

TEST_CASE("a single completion is its own neighbour") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    state.addCompletion(u"quit");

    CHECK(state.nextCompletion() == std::u16string_view(u"quit"));
    CHECK(state.previousCompletion() == std::u16string_view(u"quit"));
    CHECK(state.getCompletionIndex() == 0);
}

TEST_CASE("clearing the completions forgets the list and the index") {
    auto registry = RecordingCVarRegistry();
    auto state = PromptState(registry);

    state.addCompletion(u"bind");
    state.addCompletion(u"cvar");
    (void) state.nextCompletion();

    state.clearCompletions();

    // A stale index over a fresh list is how the completion of one command shows up in the next
    CHECK(state.getCompletionCount() == 0);
    CHECK(state.getCompletionIndex() == 0);
    CHECK(state.getCurrentCompletion().empty());
}
