// Tests for Position: starting setup, make_move, and undo_move.
// No framework needed: build this file with position.cpp and run it.
// Every test plays moves, checks the result, undoes everything, and checks
// the board is back exactly where it started.

#include <cstdio>
#include <string>
#include <vector>
#include <bit>

#include "../engine/position.h"

// ---------- Tiny test harness ----------

static int gChecks = 0;
static int gFailures = 0;
static const char* gCurrentTest = "";

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++gChecks;                                                           \
        if (!(cond)) {                                                       \
            ++gFailures;                                                     \
            std::printf("  FAIL [%s] line %d: %s\n", gCurrentTest, __LINE__, \
                        #cond);                                              \
        }                                                                    \
    } while (0)

// "e4" -> 28, using a1 = 0
constexpr int sq(const char* name) {
    return (name[1] - '1') * 8 + (name[0] - 'a');
}

// ---------- Helpers ----------

// Everything that defines a position, so two positions can be compared.
struct Snapshot {
    std::array<Piece, 64> pieces{};
    std::array<uint64_t, 12> bitboards{};
    uint8_t castling = 0;
    int enPassant = 0;
    Color turn = WHITE;

    bool operator==(const Snapshot&) const = default;
};

Snapshot snapshot(const Position& pos) {
    Snapshot s;
    for (int i = 0; i < 64; i++) s.pieces[i] = pos.piece_on(i);
    for (int p = 0; p < 12; p++) s.bitboards[p] = pos.bitboard(static_cast<Piece>(p));
    s.castling = pos.castling_rights();
    s.enPassant = pos.en_passant_target();
    s.turn = pos.side_to_move();
    return s;
}

// The two board representations must always agree, and no two pieces
// may share a square. Returns false and prints the problem if not.
bool in_sync(const Position& pos) {
    uint64_t seen = 0;
    for (int p = 0; p < 12; p++) {
        uint64_t bb = pos.bitboard(static_cast<Piece>(p));
        if (bb & seen) {
            std::printf("  two bitboards share a square (piece %d)\n", p);
            return false;
        }
        seen |= bb;
    }
    for (int s = 0; s < 64; s++) {
        Piece onSquare = pos.piece_on(s);
        for (int p = 0; p < 12; p++) {
            bool bit = is_set(pos.bitboard(static_cast<Piece>(p)), s);
            if (bit != (onSquare == p)) {
                std::printf("  mPieces and bitboards disagree on square %d\n", s);
                return false;
            }
        }
    }
    return true;
}

void print_board(const Position& pos) {
    const char* letters = "PNBRQKpnbrqk";
    for (int rank = 7; rank >= 0; rank--) {
        std::printf("    %d  ", rank + 1);
        for (int file = 0; file < 8; file++) {
            Piece p = pos.piece_on(rank * 8 + file);
            std::printf("%c ", p == NO_PIECE ? '.' : letters[p]);
        }
        std::printf("\n");
    }
    std::printf("       a b c d e f g h\n");
}

// Plays a list of moves, checking the board stays in sync after each one.
void play(Position& pos, const std::vector<Move>& moves) {
    for (const Move& m : moves) {
        pos.make_move(m);
        CHECK(in_sync(pos));
    }
}

// Undoes n moves, checking sync after each one.
void undo(Position& pos, int n) {
    for (int i = 0; i < n; i++) {
        pos.undo_move();
        CHECK(in_sync(pos));
    }
}

int count(uint64_t bb) { return std::popcount(bb); }

// Shorthand for building moves.
Move quiet(const char* f, const char* t) { return Move(sq(f), sq(t)); }
Move dbl(const char* f, const char* t)   { return Move(sq(f), sq(t), NO_TYPE, DOUBLE_PUSH); }
Move ep(const char* f, const char* t)    { return Move(sq(f), sq(t), NO_TYPE, EN_PASSANT); }
Move castle(const char* f, const char* t){ return Move(sq(f), sq(t), NO_TYPE, CASTLE); }
Move promo(const char* f, const char* t, PieceType p) { return Move(sq(f), sq(t), p); }

// ---------- Tests ----------

void test_starting_position() {
    Position pos;

    CHECK(in_sync(pos));
    CHECK(pos.piece_on(sq("e1")) == W_KING);
    CHECK(pos.piece_on(sq("d1")) == W_QUEEN);
    CHECK(pos.piece_on(sq("e8")) == B_KING);
    CHECK(pos.piece_on(sq("d8")) == B_QUEEN);
    CHECK(pos.piece_on(sq("a1")) == W_ROOK);
    CHECK(pos.piece_on(sq("h8")) == B_ROOK);
    CHECK(pos.piece_on(sq("b1")) == W_KNIGHT);
    CHECK(pos.piece_on(sq("f8")) == B_BISHOP);
    CHECK(pos.piece_on(sq("e4")) == NO_PIECE);

    CHECK(pos.bitboard(W_PAWN) == 0x000000000000FF00ULL);
    CHECK(pos.bitboard(B_PAWN) == 0x00FF000000000000ULL);
    CHECK(pos.bitboard(W_KNIGHT) == 0x0000000000000042ULL);
    CHECK(pos.bitboard(W_KING) == 0x0000000000000010ULL);

    int total = 0;
    for (int p = 0; p < 12; p++) total += count(pos.bitboard(static_cast<Piece>(p)));
    CHECK(total == 32);

    CHECK(pos.castling_rights() == 0x0F);
    CHECK(pos.en_passant_target() == -1);
    CHECK(pos.side_to_move() == WHITE);
}

void test_quiet_move() {
    Position pos;
    Snapshot start = snapshot(pos);

    play(pos, {quiet("g1", "f3")});
    CHECK(pos.piece_on(sq("f3")) == W_KNIGHT);
    CHECK(pos.piece_on(sq("g1")) == NO_PIECE);
    CHECK(pos.side_to_move() == BLACK);
    CHECK(pos.en_passant_target() == -1);
    CHECK(pos.castling_rights() == 0x0F);

    undo(pos, 1);
    CHECK(snapshot(pos) == start);
}

void test_double_push_sets_and_clears_en_passant() {
    Position pos;
    Snapshot start = snapshot(pos);

    play(pos, {dbl("e2", "e4")});
    CHECK(pos.piece_on(sq("e4")) == W_PAWN);
    CHECK(pos.en_passant_target() == sq("e3"));

    play(pos, {dbl("c7", "c5")});
    CHECK(pos.en_passant_target() == sq("c6"));

    play(pos, {quiet("g1", "f3")});
    CHECK(pos.en_passant_target() == -1);

    undo(pos, 1);
    CHECK(pos.en_passant_target() == sq("c6"));
    undo(pos, 1);
    CHECK(pos.en_passant_target() == sq("e3"));
    undo(pos, 1);
    CHECK(snapshot(pos) == start);
}

void test_capture() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. e4 d5 2. exd5
    play(pos, {dbl("e2", "e4"), dbl("d7", "d5"), quiet("e4", "d5")});
    CHECK(pos.piece_on(sq("d5")) == W_PAWN);
    CHECK(pos.piece_on(sq("e4")) == NO_PIECE);
    CHECK(count(pos.bitboard(B_PAWN)) == 7);
    CHECK(count(pos.bitboard(W_PAWN)) == 8);

    undo(pos, 1);
    CHECK(pos.piece_on(sq("d5")) == B_PAWN);
    CHECK(pos.piece_on(sq("e4")) == W_PAWN);
    CHECK(count(pos.bitboard(B_PAWN)) == 8);

    undo(pos, 2);
    CHECK(snapshot(pos) == start);
}

void test_white_en_passant() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. e4 a6 2. e5 d5 3. exd6 e.p.
    play(pos, {dbl("e2", "e4"), quiet("a7", "a6"), quiet("e4", "e5"),
               dbl("d7", "d5")});
    CHECK(pos.en_passant_target() == sq("d6"));

    play(pos, {ep("e5", "d6")});
    CHECK(pos.piece_on(sq("d6")) == W_PAWN);
    CHECK(pos.piece_on(sq("d5")) == NO_PIECE);   // captured pawn is gone
    CHECK(pos.piece_on(sq("e5")) == NO_PIECE);
    CHECK(count(pos.bitboard(B_PAWN)) == 7);

    undo(pos, 1);
    CHECK(pos.piece_on(sq("d5")) == B_PAWN);     // restored on the right square
    CHECK(pos.piece_on(sq("e5")) == W_PAWN);
    CHECK(pos.piece_on(sq("d6")) == NO_PIECE);
    CHECK(pos.en_passant_target() == sq("d6"));

    undo(pos, 4);
    CHECK(snapshot(pos) == start);
}

void test_black_en_passant() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. a3 d5 2. a4 d4 3. e4 dxe3 e.p.
    play(pos, {quiet("a2", "a3"), dbl("d7", "d5"), quiet("a3", "a4"),
               quiet("d5", "d4"), dbl("e2", "e4")});
    CHECK(pos.en_passant_target() == sq("e3"));

    play(pos, {ep("d4", "e3")});
    CHECK(pos.piece_on(sq("e3")) == B_PAWN);
    CHECK(pos.piece_on(sq("e4")) == NO_PIECE);
    CHECK(pos.piece_on(sq("d4")) == NO_PIECE);
    CHECK(count(pos.bitboard(W_PAWN)) == 7);

    undo(pos, 1);
    CHECK(pos.piece_on(sq("e4")) == W_PAWN);
    CHECK(pos.piece_on(sq("d4")) == B_PAWN);
    CHECK(pos.piece_on(sq("e3")) == NO_PIECE);

    undo(pos, 5);
    CHECK(snapshot(pos) == start);
}

void test_white_kingside_castle() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. e4 e5 2. Nf3 Nc6 3. Bc4 Bc5 4. O-O
    play(pos, {dbl("e2", "e4"), dbl("e7", "e5"), quiet("g1", "f3"),
               quiet("b8", "c6"), quiet("f1", "c4"), quiet("f8", "c5")});
    play(pos, {castle("e1", "g1")});

    CHECK(pos.piece_on(sq("g1")) == W_KING);
    CHECK(pos.piece_on(sq("f1")) == W_ROOK);
    CHECK(pos.piece_on(sq("e1")) == NO_PIECE);
    CHECK(pos.piece_on(sq("h1")) == NO_PIECE);
    CHECK(pos.castling_rights() == (BLACK_KINGSIDE | BLACK_QUEENSIDE));

    undo(pos, 1);
    CHECK(pos.piece_on(sq("e1")) == W_KING);
    CHECK(pos.piece_on(sq("h1")) == W_ROOK);
    CHECK(pos.piece_on(sq("f1")) == NO_PIECE);
    CHECK(pos.piece_on(sq("g1")) == NO_PIECE);
    CHECK(pos.castling_rights() == 0x0F);

    undo(pos, 6);
    CHECK(snapshot(pos) == start);
}

void test_queenside_castles_both_sides() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. d4 d5 2. Nc3 Nc6 3. Bf4 Bf5 4. Qd2 Qd7 5. O-O-O O-O-O
    play(pos, {dbl("d2", "d4"), dbl("d7", "d5"), quiet("b1", "c3"),
               quiet("b8", "c6"), quiet("c1", "f4"), quiet("c8", "f5"),
               quiet("d1", "d2"), quiet("d8", "d7")});

    play(pos, {castle("e1", "c1")});
    CHECK(pos.piece_on(sq("c1")) == W_KING);
    CHECK(pos.piece_on(sq("d1")) == W_ROOK);
    CHECK(pos.piece_on(sq("a1")) == NO_PIECE);
    CHECK(pos.piece_on(sq("e1")) == NO_PIECE);

    play(pos, {castle("e8", "c8")});
    CHECK(pos.piece_on(sq("c8")) == B_KING);
    CHECK(pos.piece_on(sq("d8")) == B_ROOK);
    CHECK(pos.piece_on(sq("a8")) == NO_PIECE);
    CHECK(pos.piece_on(sq("e8")) == NO_PIECE);
    CHECK(pos.castling_rights() == 0);

    undo(pos, 1);
    CHECK(pos.piece_on(sq("e8")) == B_KING);
    CHECK(pos.piece_on(sq("a8")) == B_ROOK);
    CHECK(pos.castling_rights() == (BLACK_KINGSIDE | BLACK_QUEENSIDE));

    undo(pos, 9);
    CHECK(snapshot(pos) == start);
}

void test_king_move_clears_both_rights() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. e4 e5 2. Ke2
    play(pos, {dbl("e2", "e4"), dbl("e7", "e5"), quiet("e1", "e2")});
    CHECK(pos.castling_rights() == (BLACK_KINGSIDE | BLACK_QUEENSIDE));

    // 2... Ke7
    play(pos, {quiet("e8", "e7")});
    CHECK(pos.castling_rights() == 0);

    undo(pos, 4);
    CHECK(snapshot(pos) == start);
}

void test_rook_move_clears_one_right() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. h4 a5 2. Rh3 Ra6
    play(pos, {dbl("h2", "h4"), dbl("a7", "a5"), quiet("h1", "h3")});
    CHECK(pos.castling_rights() ==
          (WHITE_QUEENSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE));

    play(pos, {quiet("a8", "a6")});
    CHECK(pos.castling_rights() == (WHITE_QUEENSIDE | BLACK_KINGSIDE));

    undo(pos, 4);
    CHECK(snapshot(pos) == start);
}

void test_capturing_rook_on_home_square_clears_right() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. b3 a6 2. Bb2 a5 3. Bxg7 a4 4. Bxh8
    play(pos, {quiet("b2", "b3"), quiet("a7", "a6"), quiet("c1", "b2"),
               quiet("a6", "a5"), quiet("b2", "g7"), quiet("a5", "a4"),
               quiet("g7", "h8")});

    CHECK(pos.piece_on(sq("h8")) == W_BISHOP);
    CHECK(count(pos.bitboard(B_ROOK)) == 1);
    // The black rook never moved, but it was captured, so black loses kingside.
    CHECK(pos.castling_rights() ==
          (WHITE_KINGSIDE | WHITE_QUEENSIDE | BLACK_QUEENSIDE));

    undo(pos, 1);
    CHECK(pos.piece_on(sq("h8")) == B_ROOK);
    CHECK(pos.castling_rights() == 0x0F);

    undo(pos, 6);
    CHECK(snapshot(pos) == start);
}

void test_white_capture_promotion() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. b4 a5 2. bxa5 h6 3. a6 h5 4. axb7 h4 5. bxa8=Q
    play(pos, {dbl("b2", "b4"), dbl("a7", "a5"), quiet("b4", "a5"),
               quiet("h7", "h6"), quiet("a5", "a6"), quiet("h6", "h5"),
               quiet("a6", "b7"), quiet("h5", "h4")});
    play(pos, {promo("b7", "a8", QUEEN)});

    CHECK(pos.piece_on(sq("a8")) == W_QUEEN);
    CHECK(pos.piece_on(sq("b7")) == NO_PIECE);
    CHECK(count(pos.bitboard(W_QUEEN)) == 2);
    CHECK(count(pos.bitboard(W_PAWN)) == 7);
    CHECK(count(pos.bitboard(B_ROOK)) == 1);
    CHECK(pos.castling_rights() ==
          (WHITE_KINGSIDE | WHITE_QUEENSIDE | BLACK_KINGSIDE));

    undo(pos, 1);
    CHECK(pos.piece_on(sq("b7")) == W_PAWN);   // the queen turns back into a pawn
    CHECK(pos.piece_on(sq("a8")) == B_ROOK);   // and the captured rook returns
    CHECK(count(pos.bitboard(W_QUEEN)) == 1);
    CHECK(pos.castling_rights() == 0x0F);

    // Underpromotion from the same position.
    play(pos, {promo("b7", "a8", KNIGHT)});
    CHECK(pos.piece_on(sq("a8")) == W_KNIGHT);
    CHECK(count(pos.bitboard(W_KNIGHT)) == 3);
    undo(pos, 1);

    undo(pos, 8);
    CHECK(snapshot(pos) == start);
}

void test_black_capture_promotion() {
    Position pos;
    Snapshot start = snapshot(pos);

    // 1. a4 b5 2. h3 bxa4 3. h4 a3 4. h5 axb2 5. h6 bxa1=Q
    play(pos, {dbl("a2", "a4"), dbl("b7", "b5"), quiet("h2", "h3"),
               quiet("b5", "a4"), quiet("h3", "h4"), quiet("a4", "a3"),
               quiet("h4", "h5"), quiet("a3", "b2"), quiet("h5", "h6")});
    play(pos, {promo("b2", "a1", QUEEN)});

    CHECK(pos.piece_on(sq("a1")) == B_QUEEN);
    CHECK(pos.piece_on(sq("b2")) == NO_PIECE);
    CHECK(count(pos.bitboard(B_QUEEN)) == 2);
    CHECK(count(pos.bitboard(W_ROOK)) == 1);
    CHECK(pos.castling_rights() ==
          (WHITE_KINGSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE));

    undo(pos, 1);
    CHECK(pos.piece_on(sq("b2")) == B_PAWN);
    CHECK(pos.piece_on(sq("a1")) == W_ROOK);

    undo(pos, 9);
    CHECK(snapshot(pos) == start);
}

void test_undo_with_empty_history() {
    Position pos;
    Snapshot start = snapshot(pos);
    pos.undo_move();
    pos.undo_move();
    CHECK(snapshot(pos) == start);
}

// ---------- Runner ----------

struct Test {
    const char* name;
    void (*fn)();
};

int main() {
    const Test tests[] = {
        {"starting position", test_starting_position},
        {"quiet move", test_quiet_move},
        {"double push sets/clears en passant", test_double_push_sets_and_clears_en_passant},
        {"capture", test_capture},
        {"white en passant", test_white_en_passant},
        {"black en passant", test_black_en_passant},
        {"white kingside castle", test_white_kingside_castle},
        {"queenside castles, both sides", test_queenside_castles_both_sides},
        {"king move clears both rights", test_king_move_clears_both_rights},
        {"rook move clears one right", test_rook_move_clears_one_right},
        {"rook captured on home square", test_capturing_rook_on_home_square_clears_right},
        {"white capture promotion", test_white_capture_promotion},
        {"black capture promotion", test_black_capture_promotion},
        {"undo with empty history", test_undo_with_empty_history},
    };

    int failedTests = 0;
    for (const Test& t : tests) {
        gCurrentTest = t.name;
        int before = gFailures;
        t.fn();
        bool passed = gFailures == before;
        if (!passed) failedTests++;
        std::printf("%s  %s\n", passed ? "PASS" : "FAIL", t.name);
    }

    std::printf("\n%d checks, %d failed, across %d tests (%d failed)\n",
                gChecks, gFailures, static_cast<int>(std::size(tests)), failedTests);
    return gFailures == 0 ? 0 : 1;
}