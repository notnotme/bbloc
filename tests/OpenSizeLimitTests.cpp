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
#include <array>
#include <limits>

#include "TestSupport.h"

#include "core/base/OpenSizeLimit.h"


/** One megabyte, in the 1024-based unit the limit is expressed in. */
static constexpr auto MEGABYTE = uintmax_t{ 1024 } * 1024;

/** File sizes every disabled-limit case sweeps: empty, tiny, huge, and the largest representable. */
static constexpr auto SIZES = std::array<uintmax_t, 5>{
    0,
    1,
    MEGABYTE,
    uintmax_t{ 1 } << 40,
    std::numeric_limits<uintmax_t>::max(),
};


TEST_CASE("a limit of zero disables the guard at every size") {
    for (const auto size : SIZES) {
        CAPTURE(size);
        CHECK_FALSE(exceedsOpenSizeLimit(size, 0));
    }
}

TEST_CASE("a negative limit never fires either") {
    // The CVar callback clamps negatives away, but the predicate must not rely on it
    for (const auto size : SIZES) {
        CAPTURE(size);
        CHECK_FALSE(exceedsOpenSizeLimit(size, -1));
        CHECK_FALSE(exceedsOpenSizeLimit(size, std::numeric_limits<int32_t>::min()));
    }
}

TEST_CASE("a file of exactly the limit opens silently, one byte more asks") {
    CHECK_FALSE(exceedsOpenSizeLimit(MEGABYTE, 1));
    CHECK(exceedsOpenSizeLimit(MEGABYTE + 1, 1));

    CHECK_FALSE(exceedsOpenSizeLimit(10 * MEGABYTE, 10));
    CHECK(exceedsOpenSizeLimit(10 * MEGABYTE + 1, 10));
}

TEST_CASE("a megabyte is 1024 times 1024 bytes, not a million") {
    // A limit of 1 must not fire between the decimal and the binary megabyte
    CHECK_FALSE(exceedsOpenSizeLimit(1'000'001, 1));
    CHECK(exceedsOpenSizeLimit(1'048'577, 1));
}

TEST_CASE("the widest possible limit does not overflow") {
    // INT32_MAX megabytes is about 2^51 bytes: the multiplication must happen in uintmax_t
    constexpr auto limit = std::numeric_limits<int32_t>::max();
    constexpr auto threshold = static_cast<uintmax_t>(limit) * MEGABYTE;

    CHECK_FALSE(exceedsOpenSizeLimit(threshold, limit));
    CHECK(exceedsOpenSizeLimit(threshold + 1, limit));
    CHECK(exceedsOpenSizeLimit(std::numeric_limits<uintmax_t>::max(), limit));
}
