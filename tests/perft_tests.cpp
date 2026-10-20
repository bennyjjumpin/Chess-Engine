// Perft tests: count every legal move sequence to a fixed depth from
// well-known positions, and compare against the published numbers.
// If every count matches, move generation is almost certainly correct.
//
// Usage:
//   perft_tests            run the standard suite
//   perft_tests deep       also run the larger depths (slower)
//   perft_tests divide "<fen>" <depth>
//                          print counts per first move, for tracking down bugs

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include "../engine/perft.h"

namespace {

int gFailures = 0;

struct PerftCase {
    const char* name;
    const char* fen;
    int depth;
    uint64_t expected;
    bool deep;   // only run with the "deep" argument
};

// Positions and counts from the Chess Programming Wiki's perft results page.
// Each one stresses different rules; together they cover every special move.
const PerftCase kCases[] = {
    // Starting position.
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20, false},
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400, false},
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902, false},
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281, false},
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609, true},

    // "Kiwipete": castling both ways, en passant, promotions, pins, checks.
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48, false},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039, false},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862, false},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603, true},

    // Endgame: en passant that would expose the king along a rank, and checks.
    {"position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14, false},
    {"position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191, false},
    {"position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812, false},
    {"position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238, false},
    {"position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624, false},

    // Promotions and castling while in check.
    {"position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6, false},
    {"position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264, false},
    {"position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467, false},
    {"position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333, false},

    // The same position with colors flipped: catches bugs that only affect one side.
    {"position 4 mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 1, 6, false},
    {"position 4 mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 2, 264, false},
    {"position 4 mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 3, 9467, false},
    {"position 4 mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4, 422333, false},

    // Underpromotion and a knight giving check from a promotion square.
    {"position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44, false},
    {"position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486, false},
    {"position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379, false},
    {"position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487, true},

    // A quiet, symmetrical middlegame.
    {"position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46, false},
    {"position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079, false},
    {"position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890, false},
    {"position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594, true},
};

// Everything that defines a position, so we can check perft leaves the
// board exactly as it found it.
bool same_position(const Position& a, const Position& b) {
    for (int s = 0; s < 64; s++)
        if (a.piece_on(s) != b.piece_on(s)) return false;
    for (int p = 0; p < 12; p++)
        if (a.bitboard(static_cast<Piece>(p)) != b.bitboard(static_cast<Piece>(p))) return false;
    return a.castling_rights() == b.castling_rights()
        && a.en_passant_target() == b.en_passant_target()
        && a.side_to_move() == b.side_to_move();
}

void check(bool cond, const char* what) {
    if (!cond) {
        ++gFailures;
        std::printf("FAIL  %s\n", what);
    }
}

void test_fen_loader() {
    // The FEN of the starting position must match the default constructor.
    Position fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Position start;
    check(same_position(fromFen, start), "FEN: starting position matches Position()");

    // Side to move, castling rights and en passant are read correctly.
    Position p("rnbqkbnr/pppp1ppp/8/4pP2/8/8/PPPPP1PP/RNBQKBNR w Kq e6 0 3");
    check(p.side_to_move() == WHITE, "FEN: side to move");
    check(p.castling_rights() == (WHITE_KINGSIDE | BLACK_QUEENSIDE), "FEN: castling rights");
    check(p.en_passant_target() == 44, "FEN: en passant square e6");
    check(p.piece_on(37) == W_PAWN, "FEN: pawn on f5");

    Position q("8/8/8/8/8/8/8/K6k b - - 0 1");
    check(q.side_to_move() == BLACK, "FEN: black to move");
    check(q.castling_rights() == 0, "FEN: no castling rights");
    check(q.en_passant_target() == -1, "FEN: no en passant square");
    check(q.piece_on(0) == W_KING && q.piece_on(7) == B_KING, "FEN: kings on a1 and h1");

    // The last two fields are optional.
    Position r("8/8/8/8/8/8/8/K6k w - -");
    check(r.piece_on(0) == W_KING, "FEN: works without move counters");

    // Malformed input throws instead of silently building a broken board.
    const char* bad[] = {
        "",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1",          // 7 ranks
        "rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  // digit 9
        "rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // 9 squares
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNX w KQkq - 0 1",  // unknown piece
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1",  // bad side
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQxq - 0 1",  // bad castling
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e4 0 1", // bad e.p. rank
    };
    for (const char* fen : bad) {
        bool threw = false;
        try { Position bp(fen); } catch (const std::invalid_argument&) { threw = true; }
        if (!threw) {
            ++gFailures;
            std::printf("FAIL  FEN should have been rejected: \"%s\"\n", fen);
        }
    }
    if (gFailures == 0) std::printf("PASS  FEN loader\n");
}

void run_perft_suite(bool deep) {
    std::printf("\n%-22s %5s %12s %12s %8s\n", "position", "depth", "expected", "got", "time");
    for (const PerftCase& c : kCases) {
        if (c.deep && !deep) continue;

        Position pos(c.fen);
        Position before(c.fen);

        auto start = std::chrono::steady_clock::now();
        uint64_t got = perft(pos, c.depth);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

        bool ok = got == c.expected;
        bool restored = same_position(pos, before);
        if (!ok || !restored) ++gFailures;

        std::printf("%-22s %5d %12llu %12llu %6.0fms  %s%s\n", c.name, c.depth,
                    static_cast<unsigned long long>(c.expected),
                    static_cast<unsigned long long>(got), ms,
                    ok ? "PASS" : "FAIL",
                    restored ? "" : "  (board not restored after perft!)");
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::strcmp(argv[1], "divide") == 0) {
        Position pos(argv[2]);
        perft_divide(pos, std::atoi(argv[3]));
        return 0;
    }

    bool deep = argc == 2 && std::strcmp(argv[1], "deep") == 0;

    test_fen_loader();
    run_perft_suite(deep);

    if (gFailures == 0)
        std::printf("\nAll tests passed.%s\n", deep ? "" : " Run with \"deep\" for the larger depths.");
    else
        std::printf("\n%d failure(s).\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}