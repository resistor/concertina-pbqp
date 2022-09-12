#pragma once

#include <unordered_map>
#include <optional>

constexpr unsigned LEFT = 0 << 4;
constexpr unsigned RIGHT = 1 << 4;
constexpr std::array<unsigned, 2> HANDS = { LEFT, RIGHT };

constexpr unsigned PUSH = 0 << 5;
constexpr unsigned PULL = 1 << 5;
constexpr std::array<unsigned, 2> DIRECTIONS = { PUSH, PULL };

enum class ConcertinaReed : unsigned {
    L01aPush = LEFT | PUSH | 1,
    L02aPush = LEFT | PUSH | 2,
    L03aPush = LEFT | PUSH | 3,
    L04aPush = LEFT | PUSH | 4,
    L05aPush = LEFT | PUSH | 5,
    L01Push = LEFT | PUSH | 6,
    L02Push = LEFT | PUSH | 7,
    L03Push = LEFT | PUSH | 8,
    L04Push = LEFT | PUSH | 9,
    L05Push = LEFT | PUSH | 10,
    L06Push = LEFT | PUSH | 11,
    L07Push = LEFT | PUSH | 12,
    L08Push = LEFT | PUSH | 13,
    L09Push = LEFT | PUSH | 14,
    L10Push = LEFT | PUSH | 15,
    R01aPush = RIGHT | PUSH | 1,
    R02aPush = RIGHT | PUSH | 2,
    R03aPush = RIGHT | PUSH | 3,
    R04aPush = RIGHT | PUSH | 4,
    R05aPush = RIGHT | PUSH | 5,
    R01Push = RIGHT | PUSH | 6,
    R02Push = RIGHT | PUSH | 7,
    R03Push = RIGHT | PUSH | 8,
    R04Push = RIGHT | PUSH | 9,
    R05Push = RIGHT | PUSH | 10,
    R06Push = RIGHT | PUSH | 11,
    R07Push = RIGHT | PUSH | 12,
    R08Push = RIGHT | PUSH | 13,
    R09Push = RIGHT | PUSH | 14,
    R10Push = RIGHT | PUSH | 15,
    L01aPull = LEFT | PULL | 1,
    L02aPull = LEFT | PULL | 2,
    L03aPull = LEFT | PULL | 3,
    L04aPull = LEFT | PULL | 4,
    L05aPull = LEFT | PULL | 5,
    L01Pull = LEFT | PULL | 6,
    L02Pull = LEFT | PULL | 7,
    L03Pull = LEFT | PULL | 8,
    L04Pull = LEFT | PULL | 9,
    L05Pull = LEFT | PULL | 10,
    L06Pull = LEFT | PULL | 11,
    L07Pull = LEFT | PULL | 12,
    L08Pull = LEFT | PULL | 13,
    L09Pull = LEFT | PULL | 14,
    L10Pull = LEFT | PULL | 15,
    R01aPull = RIGHT | PULL | 1,
    R02aPull = RIGHT | PULL | 2,
    R03aPull = RIGHT | PULL | 3,
    R04aPull = RIGHT | PULL | 4,
    R05aPull = RIGHT | PULL | 5,
    R01Pull = RIGHT | PULL | 6,
    R02Pull = RIGHT | PULL | 7,
    R03Pull = RIGHT | PULL | 8,
    R04Pull = RIGHT | PULL | 9,
    R05Pull = RIGHT | PULL | 10,
    R06Pull = RIGHT | PULL | 11,
    R07Pull = RIGHT | PULL | 12,
    R08Pull = RIGHT | PULL | 13,
    R09Pull = RIGHT | PULL | 14,
    R10Pull = RIGHT | PULL | 15,
    MaxReed,
};

std::optional<ConcertinaReed> LeftNeighbor(ConcertinaReed reed) {
    if ((unsigned)reed % 5 == 1) {
        return std::nullopt;
    } else {
        return (ConcertinaReed)((unsigned)reed - 1);
    }
}

std::optional<ConcertinaReed> RightNeighbor(ConcertinaReed reed) {
    if ((unsigned)reed % 5 == 5) {
        return std::nullopt;
    } else {
        return (ConcertinaReed)((unsigned)reed + 1);
    }
}

std::optional<ConcertinaReed> UpNeighbor(ConcertinaReed reed) {
    if (((unsigned)reed & 0xF) / 5 == 0) {
        return std::nullopt;
    } else {
        return (ConcertinaReed)((unsigned)reed - 5);
    }
}

std::optional<ConcertinaReed> DownNeighbor(ConcertinaReed reed) {
    if (((unsigned)reed & 0xF) / 5 == 2) {
        return std::nullopt;
    } else {
        return (ConcertinaReed)((unsigned)reed + 5);
    }
}


enum class ConcertinaNote : unsigned {
    B2,
    C2,
    D2,
    G2,
    C3,
    E3,
    F3,
    Fsharp3,
    G3,
    Gsharp3,
    A3,
    B3,
    Bflat3,
    C4,
    Csharp4,
    D4,
    Dsharp4,
    E4,
    Esharp4,
    F4,
    Fsharp4,
    G4,
    Gsharp4,
    A4,
    Bflat4,
    B4,
    C5,
    Csharp5,
    D5,
    Dsharp5,
    E5,
    F5,
    Fsharp5,
    G5,
    Gsharp5,
    A5,
    Bflat5,
    B5,
    C6,
    D6,
    Csharp6,
    Dsharp6,
    E6,
    F6,
    Fsharp6,
    G6,
    A6,
    B6,
    MaxNote,
};

const std::unordered_multimap<ConcertinaNote, ConcertinaReed> CGWheatstoneReedMapping = {
    { ConcertinaNote::E3, ConcertinaReed::L01aPush },
    { ConcertinaNote::F3, ConcertinaReed::L01aPull },
    { ConcertinaNote::A3, ConcertinaReed::L02aPush },
    { ConcertinaNote::Bflat3, ConcertinaReed::L02aPull },
    { ConcertinaNote::Csharp4, ConcertinaReed::L03aPush },
    { ConcertinaNote::Dsharp4, ConcertinaReed::L03aPull },
    { ConcertinaNote::A4, ConcertinaReed::L04aPush },
    { ConcertinaNote::G4, ConcertinaReed::L04aPull },
    { ConcertinaNote::Gsharp4, ConcertinaReed::L05aPush },
    { ConcertinaNote::Bflat4, ConcertinaReed::L05aPull },
    { ConcertinaNote::C3, ConcertinaReed::L01Push },
    { ConcertinaNote::G3, ConcertinaReed::L01Pull },
    { ConcertinaNote::G3, ConcertinaReed::L02Push },
    { ConcertinaNote::B3, ConcertinaReed::L02Pull },
    { ConcertinaNote::C4, ConcertinaReed::L03Push },
    { ConcertinaNote::D4, ConcertinaReed::L03Pull },
    { ConcertinaNote::E4, ConcertinaReed::L04Push },
    { ConcertinaNote::F4, ConcertinaReed::L04Pull },
    { ConcertinaNote::G4, ConcertinaReed::L05Push },
    { ConcertinaNote::A4, ConcertinaReed::L05Pull },
    { ConcertinaNote::B3, ConcertinaReed::L06Push },
    { ConcertinaNote::A3, ConcertinaReed::L06Pull },
    { ConcertinaNote::D4, ConcertinaReed::L07Push },
    { ConcertinaNote::Fsharp4, ConcertinaReed::L07Pull },
    { ConcertinaNote::G4, ConcertinaReed::L08Push },
    { ConcertinaNote::A4, ConcertinaReed::L08Pull },
    { ConcertinaNote::B4, ConcertinaReed::L09Push },
    { ConcertinaNote::C5, ConcertinaReed::L09Pull },
    { ConcertinaNote::D5, ConcertinaReed::L10Push },
    { ConcertinaNote::E5, ConcertinaReed::L10Pull },
    { ConcertinaNote::Csharp5, ConcertinaReed::R01aPush },
    { ConcertinaNote::Dsharp5, ConcertinaReed::R01aPull },
    { ConcertinaNote::A5, ConcertinaReed::R02aPush },
    { ConcertinaNote::G5, ConcertinaReed::R02aPull },
    { ConcertinaNote::Gsharp5, ConcertinaReed::R03aPush },
    { ConcertinaNote::Bflat5, ConcertinaReed::R03aPull},
    { ConcertinaNote::Csharp6, ConcertinaReed::R04aPush },
    { ConcertinaNote::Dsharp6, ConcertinaReed::R04aPull },
    { ConcertinaNote::A6, ConcertinaReed::R05aPush },
    { ConcertinaNote::F6, ConcertinaReed::R05aPull },
    { ConcertinaNote::C5, ConcertinaReed::R01Push },
    { ConcertinaNote::B4, ConcertinaReed::R01Pull },
    { ConcertinaNote::E5, ConcertinaReed::R02Push },
    { ConcertinaNote::D5, ConcertinaReed::R02Pull },
    { ConcertinaNote::G5, ConcertinaReed::R03Push },
    { ConcertinaNote::F5, ConcertinaReed::R03Pull },
    { ConcertinaNote::C6, ConcertinaReed::R04Push },
    { ConcertinaNote::A5, ConcertinaReed::R04Pull },
    { ConcertinaNote::E6, ConcertinaReed::R05Push },
    { ConcertinaNote::B5, ConcertinaReed::R05Pull },
    { ConcertinaNote::G5, ConcertinaReed::R06Push },
    { ConcertinaNote::Fsharp5, ConcertinaReed::R06Pull },
    { ConcertinaNote::B5, ConcertinaReed::R07Push },
    { ConcertinaNote::A5, ConcertinaReed::R07Pull },
    { ConcertinaNote::D6, ConcertinaReed::R08Push },
    { ConcertinaNote::C6, ConcertinaReed::R08Pull },
    { ConcertinaNote::G6, ConcertinaReed::R09Push },
    { ConcertinaNote::E6, ConcertinaReed::R09Pull },
    { ConcertinaNote::B6, ConcertinaReed::R10Push },
    { ConcertinaNote::Fsharp6, ConcertinaReed::R10Pull },
};

const std::unordered_multimap<ConcertinaNote, ConcertinaReed> GDWheatstoneReedMapping = {
    { ConcertinaNote::B2, ConcertinaReed::L01aPush },
    { ConcertinaNote::C2, ConcertinaReed::L01aPull },
    { ConcertinaNote::E3, ConcertinaReed::L02aPush },
    { ConcertinaNote::F3, ConcertinaReed::L02aPull },
    { ConcertinaNote::Gsharp3, ConcertinaReed::L03aPush },
    { ConcertinaNote::Bflat3, ConcertinaReed::L03aPull },
    { ConcertinaNote::E4, ConcertinaReed::L04aPush },
    { ConcertinaNote::D4, ConcertinaReed::L04aPull },
    { ConcertinaNote::Esharp4, ConcertinaReed::L05aPush },
    { ConcertinaNote::F4, ConcertinaReed::L05aPull },
    { ConcertinaNote::G2, ConcertinaReed::L01Push },
    { ConcertinaNote::D2, ConcertinaReed::L01Pull },
    { ConcertinaNote::D2, ConcertinaReed::L02Push },
    { ConcertinaNote::Fsharp3, ConcertinaReed::L02Pull },
    { ConcertinaNote::G3, ConcertinaReed::L03Push },
    { ConcertinaNote::A3, ConcertinaReed::L03Pull },
    { ConcertinaNote::B3, ConcertinaReed::L04Push },
    { ConcertinaNote::C4, ConcertinaReed::L04Pull },
    { ConcertinaNote::D4, ConcertinaReed::L05Push },
    { ConcertinaNote::E4, ConcertinaReed::L05Pull },
    { ConcertinaNote::Fsharp3, ConcertinaReed::L06Push },
    { ConcertinaNote::A3, ConcertinaReed::L06Pull },
    { ConcertinaNote::A3, ConcertinaReed::L07Push },
    { ConcertinaNote::Csharp4, ConcertinaReed::L07Pull },
    { ConcertinaNote::D4, ConcertinaReed::L08Push },
    { ConcertinaNote::E4, ConcertinaReed::L08Pull },
    { ConcertinaNote::Fsharp4, ConcertinaReed::L09Push },
    { ConcertinaNote::G4, ConcertinaReed::L09Pull },
    { ConcertinaNote::A4, ConcertinaReed::L10Push },
    { ConcertinaNote::B4, ConcertinaReed::L10Pull },
    { ConcertinaNote::Gsharp4, ConcertinaReed::R01aPush },
    { ConcertinaNote::Bflat4, ConcertinaReed::R01aPull },
    { ConcertinaNote::E5, ConcertinaReed::R02aPush },
    { ConcertinaNote::D5, ConcertinaReed::R02aPull },
    { ConcertinaNote::Dsharp5, ConcertinaReed::R03aPush },
    { ConcertinaNote::F5, ConcertinaReed::R03aPull},
    { ConcertinaNote::Gsharp5, ConcertinaReed::R04aPush },
    { ConcertinaNote::Bflat5, ConcertinaReed::R04aPull },
    { ConcertinaNote::E5, ConcertinaReed::R05aPush },
    { ConcertinaNote::C6, ConcertinaReed::R05aPull },
    { ConcertinaNote::G4, ConcertinaReed::R01Push },
    { ConcertinaNote::Fsharp4, ConcertinaReed::R01Pull },
    { ConcertinaNote::B4, ConcertinaReed::R02Push },
    { ConcertinaNote::A4, ConcertinaReed::R02Pull },
    { ConcertinaNote::D5, ConcertinaReed::R03Push },
    { ConcertinaNote::C5, ConcertinaReed::R03Pull },
    { ConcertinaNote::G5, ConcertinaReed::R04Push },
    { ConcertinaNote::E5, ConcertinaReed::R04Pull },
    { ConcertinaNote::B5, ConcertinaReed::R05Push },
    { ConcertinaNote::Fsharp5, ConcertinaReed::R05Pull },
    { ConcertinaNote::D5, ConcertinaReed::R06Push },
    { ConcertinaNote::Csharp5, ConcertinaReed::R06Pull },
    { ConcertinaNote::E5, ConcertinaReed::R07Push },
    { ConcertinaNote::Fsharp5, ConcertinaReed::R07Pull },
    { ConcertinaNote::A5, ConcertinaReed::R08Push },
    { ConcertinaNote::G5, ConcertinaReed::R08Pull },
    { ConcertinaNote::D6, ConcertinaReed::R09Push },
    { ConcertinaNote::B5, ConcertinaReed::R09Pull },
    { ConcertinaNote::Fsharp6, ConcertinaReed::R10Push },
    { ConcertinaNote::C6, ConcertinaReed::R10Pull },
};

const char* GetReedName(ConcertinaReed reed) {
    switch (reed) {
        case ConcertinaReed::L01aPull:
            return "L01aPull";
        case ConcertinaReed::L02aPull:
            return "L02aPull";
        case ConcertinaReed::L03aPull:
            return "L03aPull";
        case ConcertinaReed::L04aPull:
            return "L04aPull";
        case ConcertinaReed::L05aPull:
            return "L05aPull";
        case ConcertinaReed::L01Pull:
            return "L01Pull";
        case ConcertinaReed::L02Pull:
            return "L02Pull";
        case ConcertinaReed::L03Pull:
            return "L03Pull";
        case ConcertinaReed::L04Pull:
            return "L04Pull";
        case ConcertinaReed::L05Pull:
            return "L05Pull";
        case ConcertinaReed::L06Pull:
            return "L06Pull";
        case ConcertinaReed::L07Pull:
            return "L07Pull";
        case ConcertinaReed::L08Pull:
            return "L08Pull";
        case ConcertinaReed::L09Pull:
            return "L09Pull";
        case ConcertinaReed::L10Pull:
            return "L10Pull";
        case ConcertinaReed::L01aPush:
            return "L01aPush";
        case ConcertinaReed::L02aPush:
            return "L02aPush";
        case ConcertinaReed::L03aPush:
            return "L03aPush";
        case ConcertinaReed::L04aPush:
            return "L04aPush";
        case ConcertinaReed::L05aPush:
            return "L05aPush";
        case ConcertinaReed::L01Push:
            return "L01Push";
        case ConcertinaReed::L02Push:
            return "L02Push";
        case ConcertinaReed::L03Push:
            return "L03Push";
        case ConcertinaReed::L04Push:
            return "L04Push";
        case ConcertinaReed::L05Push:
            return "L05Push";
        case ConcertinaReed::L06Push:
            return "L06Push";
        case ConcertinaReed::L07Push:
            return "L07Push";
        case ConcertinaReed::L08Push:
            return "L08Push";
        case ConcertinaReed::L09Push:
            return "L09Push";
        case ConcertinaReed::L10Push:
            return "L10Push";
        case ConcertinaReed::R01aPull:
            return "R01aPull";
        case ConcertinaReed::R02aPull:
            return "R02aPull";
        case ConcertinaReed::R03aPull:
            return "R03aPull";
        case ConcertinaReed::R04aPull:
            return "R04aPull";
        case ConcertinaReed::R05aPull:
            return "R05aPull";
        case ConcertinaReed::R01Pull:
            return "R01Pull";
        case ConcertinaReed::R02Pull:
            return "R02Pull";
        case ConcertinaReed::R03Pull:
            return "R03Pull";
        case ConcertinaReed::R04Pull:
            return "R04Pull";
        case ConcertinaReed::R05Pull:
            return "R05Pull";
        case ConcertinaReed::R06Pull:
            return "R06Pull";
        case ConcertinaReed::R07Pull:
            return "R07Pull";
        case ConcertinaReed::R08Pull:
            return "R08Pull";
        case ConcertinaReed::R09Pull:
            return "R09Pull";
        case ConcertinaReed::R10Pull:
            return "R10Pull";
        case ConcertinaReed::R01aPush:
            return "R01aPush";
        case ConcertinaReed::R02aPush:
            return "R02aPush";
        case ConcertinaReed::R03aPush:
            return "R03aPush";
        case ConcertinaReed::R04aPush:
            return "R04aPush";
        case ConcertinaReed::R05aPush:
            return "R05aPush";
        case ConcertinaReed::R01Push:
            return "R01Push";
        case ConcertinaReed::R02Push:
            return "R02Push";
        case ConcertinaReed::R03Push:
            return "R03Push";
        case ConcertinaReed::R04Push:
            return "R04Push";
        case ConcertinaReed::R05Push:
            return "R05Push";
        case ConcertinaReed::R06Push:
            return "R06Push";
        case ConcertinaReed::R07Push:
            return "R07Push";
        case ConcertinaReed::R08Push:
            return "R08Push";
        case ConcertinaReed::R09Push:
            return "R09Push";
        case ConcertinaReed::R10Push:
            return "R10Push";
        default:
            printf("UNIMPLEMENTED REED PRINTING %d\n", (unsigned)reed);
            exit(1);
    }
}