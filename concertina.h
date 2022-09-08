#pragma once

#include <unordered_map>

enum class ConcertinaReed : unsigned {
    L01Push,
    L02Push,
    L03Push,
    L04Push,
    L05Push,
    L06Push,
    L07Push,
    L08Push,
    L09Push,
    L10Push,
    L11Push,
    L12Push,
    L13Push,
    L14Push,
    L15Push,
    R01Push,
    R02Push,
    R03Push,
    R04Push,
    R05Push,
    R06Push,
    R07Push,
    R08Push,
    R09Push,
    R10Push,
    R11Push,
    R12Push,
    R13Push,
    R14Push,
    R15Push,
    L01Pull,
    L02Pull,
    L03Pull,
    L04Pull,
    L05Pull,
    L06Pull,
    L07Pull,
    L08Pull,
    L09Pull,
    L10Pull,
    L11Pull,
    L12Pull,
    L13Pull,
    L14Pull,
    L15Pull,
    R01Pull,
    R02Pull,
    R03Pull,
    R04Pull,
    R05Pull,
    R06Pull,
    R07Pull,
    R08Pull,
    R09Pull,
    R10Pull,
    R11Pull,
    R12Pull,
    R13Pull,
    R14Pull,
    R15Pull,
    MaxReed,
};

enum class ConcertinaNote : unsigned {
    C3,
    E3,
    F3,
    G3,
    A3,
    B3,
    Bflat3,
    C4,
    Csharp4,
    D4,
    Dsharp4,
    E4,
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

const std::unordered_multimap<ConcertinaNote, ConcertinaReed> ReedMapping = {
    { ConcertinaNote::E3, ConcertinaReed::L01Push },
    { ConcertinaNote::F3, ConcertinaReed::L01Pull },
    { ConcertinaNote::A3, ConcertinaReed::L02Push },
    { ConcertinaNote::Bflat3, ConcertinaReed::L02Pull },
    { ConcertinaNote::Csharp4, ConcertinaReed::L03Push },
    { ConcertinaNote::Dsharp4, ConcertinaReed::L03Pull },
    { ConcertinaNote::A4, ConcertinaReed::L04Push },
    { ConcertinaNote::G4, ConcertinaReed::L04Pull },
    { ConcertinaNote::Gsharp4, ConcertinaReed::L05Push },
    { ConcertinaNote::Bflat4, ConcertinaReed::L05Pull },
    { ConcertinaNote::C3, ConcertinaReed::L06Push },
    { ConcertinaNote::G3, ConcertinaReed::L06Pull },
    { ConcertinaNote::G3, ConcertinaReed::L07Push },
    { ConcertinaNote::B3, ConcertinaReed::L07Pull },
    { ConcertinaNote::C4, ConcertinaReed::L08Push },
    { ConcertinaNote::D4, ConcertinaReed::L08Pull },
    { ConcertinaNote::E4, ConcertinaReed::L09Push },
    { ConcertinaNote::F4, ConcertinaReed::L09Pull },
    { ConcertinaNote::G4, ConcertinaReed::L10Push },
    { ConcertinaNote::A4, ConcertinaReed::L10Pull },
    { ConcertinaNote::B3, ConcertinaReed::L11Push },
    { ConcertinaNote::A3, ConcertinaReed::L11Pull },
    { ConcertinaNote::D4, ConcertinaReed::L12Push },
    { ConcertinaNote::Fsharp4, ConcertinaReed::L12Pull },
    { ConcertinaNote::G4, ConcertinaReed::L13Push },
    { ConcertinaNote::A4, ConcertinaReed::L13Pull },
    { ConcertinaNote::B4, ConcertinaReed::L14Push },
    { ConcertinaNote::C5, ConcertinaReed::L14Pull },
    { ConcertinaNote::D5, ConcertinaReed::L15Push },
    { ConcertinaNote::E5, ConcertinaReed::L15Pull },
    { ConcertinaNote::Csharp5, ConcertinaReed::R01Push },
    { ConcertinaNote::Dsharp5, ConcertinaReed::R01Pull },
    { ConcertinaNote::A5, ConcertinaReed::R02Push },
    { ConcertinaNote::G5, ConcertinaReed::R02Pull },
    { ConcertinaNote::Gsharp5, ConcertinaReed::R03Push },
    { ConcertinaNote::Bflat5, ConcertinaReed::R03Pull},
    { ConcertinaNote::Csharp6, ConcertinaReed::R04Push },
    { ConcertinaNote::Dsharp6, ConcertinaReed::R04Pull },
    { ConcertinaNote::A6, ConcertinaReed::R05Push },
    { ConcertinaNote::F6, ConcertinaReed::R05Pull },
    { ConcertinaNote::C5, ConcertinaReed::R06Push },
    { ConcertinaNote::B4, ConcertinaReed::R06Pull },
    { ConcertinaNote::E5, ConcertinaReed::R07Push },
    { ConcertinaNote::D5, ConcertinaReed::R07Pull },
    { ConcertinaNote::G5, ConcertinaReed::R08Push },
    { ConcertinaNote::F5, ConcertinaReed::R08Pull },
    { ConcertinaNote::C6, ConcertinaReed::R09Push },
    { ConcertinaNote::A5, ConcertinaReed::R09Pull },
    { ConcertinaNote::E6, ConcertinaReed::R10Push },
    { ConcertinaNote::B5, ConcertinaReed::R10Pull },
    { ConcertinaNote::G5, ConcertinaReed::R11Push },
    { ConcertinaNote::Fsharp5, ConcertinaReed::R11Pull },
    { ConcertinaNote::B5, ConcertinaReed::R12Push },
    { ConcertinaNote::A5, ConcertinaReed::R12Pull },
    { ConcertinaNote::D6, ConcertinaReed::R13Push },
    { ConcertinaNote::C6, ConcertinaReed::R13Pull },
    { ConcertinaNote::G6, ConcertinaReed::R14Push },
    { ConcertinaNote::E6, ConcertinaReed::R14Pull },
    { ConcertinaNote::B6, ConcertinaReed::R15Push },
    { ConcertinaNote::Fsharp6, ConcertinaReed::R15Pull },
};

const char* GetReedName(ConcertinaReed reed) {
    switch (reed) {
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
        case ConcertinaReed::L11Pull:
            return "L11Pull";
        case ConcertinaReed::L12Pull:
            return "L12Pull";
        case ConcertinaReed::L13Pull:
            return "L13Pull";
        case ConcertinaReed::L14Pull:
            return "L14Pull";
        case ConcertinaReed::L15Pull:
            return "L15Pull";
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
        case ConcertinaReed::L11Push:
            return "L11Push";
        case ConcertinaReed::L12Push:
            return "L12Push";
        case ConcertinaReed::L13Push:
            return "L13Push";
        case ConcertinaReed::L14Push:
            return "L14Push";
        case ConcertinaReed::L15Push:
            return "L15Push";
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
        case ConcertinaReed::R11Pull:
            return "R11Pull";
        case ConcertinaReed::R12Pull:
            return "R12Pull";
        case ConcertinaReed::R13Pull:
            return "R13Pull";
        case ConcertinaReed::R14Pull:
            return "R14Pull";
        case ConcertinaReed::R15Pull:
            return "R15Pull";
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
        case ConcertinaReed::R11Push:
            return "R11Push";
        case ConcertinaReed::R12Push:
            return "R12Push";
        case ConcertinaReed::R13Push:
            return "R13Push";
        case ConcertinaReed::R14Push:
            return "R14Push";
        case ConcertinaReed::R15Push:
            return "R15Push";
        default:
            printf("UNIMPLEMENTED REED PRINTING %d\n", (unsigned)reed);
            exit(1);
    }
}