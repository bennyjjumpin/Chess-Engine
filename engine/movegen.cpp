#include <array>
#include "movegen.h"

namespace {

// ---------- Attack tables for knights and kings ----------

// Every square a knight on `sq` attacks.
constexpr std::array<uint64_t, 64> knightAttacks = [] {
    std::array<uint64_t, 64> table{};
    constexpr uint64_t NOT_A  = ~FILE_A;
    constexpr uint64_t NOT_H  = ~FILE_H;
    constexpr uint64_t NOT_AB = ~(FILE_A | FILE_B);
    constexpr uint64_t NOT_GH = ~(FILE_G | FILE_H);

    for (int sq = 0; sq < 64; sq++)
    {
        uint64_t b = 1ULL << sq;
        table[sq] = ((b & NOT_H)  << 17)   // up 2, right 1
                  | ((b & NOT_A)  << 15)   // up 2, left 1
                  | ((b & NOT_GH) << 10)   // up 1, right 2
                  | ((b & NOT_AB) << 6)    // up 1, left 2
                  | ((b & NOT_A)  >> 17)   // down 2, left 1
                  | ((b & NOT_H)  >> 15)   // down 2, right 1
                  | ((b & NOT_AB) >> 10)   // down 1, left 2
                  | ((b & NOT_GH) >> 6);   // down 1, right 2
    }
    return table;
}();

// Every square a king on `sq` attacks.
constexpr std::array<uint64_t, 64> kingAttacks = [] {
    std::array<uint64_t, 64> table{};
    constexpr uint64_t NOT_A = ~FILE_A;
    constexpr uint64_t NOT_H = ~FILE_H;

    for (int sq = 0; sq < 64; sq++)
    {
        uint64_t b = 1ULL << sq;
        table[sq] = (b << 8)              // up
                  | (b >> 8)              // down
                  | ((b & NOT_H) << 1)    // right
                  | ((b & NOT_A) >> 1)    // left
                  | ((b & NOT_H) << 9)    // up-right
                  | ((b & NOT_A) << 7)    // up-left
                  | ((b & NOT_H) >> 7)    // down-right
                  | ((b & NOT_A) >> 9);   // down-left
    }
    return table;
}();

// ---------- Sliding piece attacks ----------

// Walks from `sq` one step at a time in each direction, adding every square
// it reaches. A ray stops at the board edge, or at the first occupied square,
// which is included so that captures of enemy pieces are generated.
// Stepping by rank and file (instead of shifting the bitboard) means rays
// can never wrap around the edge of the board.
uint64_t slide(int sq, uint64_t occupied, const int (&directions)[4][2])
{
    uint64_t attacks = 0;
    int rank = sq / 8;
    int file = sq % 8;

    for (const auto& dir : directions)
    {
        int r = rank + dir[0];
        int f = file + dir[1];
        while (r >= 0 && r < 8 && f >= 0 && f < 8)
        {
            int target = r * 8 + f;
            set_bit(attacks, target);
            if (is_set(occupied, target))
                break;   // blocked: this square is the last one on the ray
            r += dir[0];
            f += dir[1];
        }
    }
    return attacks;
}

constexpr int BISHOP_DIRECTIONS[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
constexpr int ROOK_DIRECTIONS[4][2]   = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

uint64_t bishop_attacks(int sq, uint64_t occupied)
{
    return slide(sq, occupied, BISHOP_DIRECTIONS);
}

uint64_t rook_attacks(int sq, uint64_t occupied)
{
    return slide(sq, occupied, ROOK_DIRECTIONS);
}

// Every square a pawn of color `c` standing on `sq` attacks.
uint64_t pawn_attacks(Color c, int sq)
{
    uint64_t b = 1ULL << sq;
    if (c == WHITE)
        return ((b & ~FILE_A) << 7)    // up-left
             | ((b & ~FILE_H) << 9);   // up-right
    else
        return ((b & ~FILE_A) >> 9)    // down-left
             | ((b & ~FILE_H) >> 7);   // down-right
}

// Every square a piece of `type` on `sq` attacks, given the board's occupancy.
// Pawns are handled separately, since they move differently than they capture.
uint64_t attacks_for(PieceType type, int sq, uint64_t occupied)
{
    switch (type)
    {
        case KNIGHT: return knightAttacks[sq];
        case BISHOP: return bishop_attacks(sq, occupied);
        case ROOK:   return rook_attacks(sq, occupied);
        case QUEEN:  return bishop_attacks(sq, occupied) | rook_attacks(sq, occupied);
        case KING:   return kingAttacks[sq];
        default:     return 0;
    }
}

// ---------- Move generation ----------

// Adds one move per square in `targets`. Every pawn in `targets` moved by
// the same `offset`, so the from-square is always to - offset. Moves landing
// on the promotion rank become four moves, one per promotion piece.
void add_pawn_moves(MoveList& list, uint64_t targets, int offset,
                    uint64_t promoRank, MoveFlag flag = NORMAL)
{
    while (targets)
    {
        int to = pop_lsb(targets);
        int from = to - offset;

        if (is_set(promoRank, to))
        {
            list.add(Move(from, to, QUEEN));
            list.add(Move(from, to, ROOK));
            list.add(Move(from, to, BISHOP));
            list.add(Move(from, to, KNIGHT));
        }
        else
        {
            list.add(Move(from, to, NO_TYPE, flag));
        }
    }
}

void generate_pawn_moves(const Position& pos, MoveList& list)
{
    Color us = pos.side_to_move();
    int forward = (us == WHITE) ? 8 : -8;
    int captureLeft = forward - 1;
    int captureRight = forward + 1;

    uint64_t promoRank = (us == WHITE) ? RANK_8 : RANK_1;
    uint64_t doubleRank = (us == WHITE) ? RANK_3 : RANK_6;

    uint64_t pawns = pos.bitboard(get_piece(us, PAWN));
    uint64_t enemies = pos.pieces(static_cast<Color>(!us));
    uint64_t empty = ~pos.occupied();

    uint64_t push1 = shift(pawns, forward) & empty;
    uint64_t push2 = shift(push1 & doubleRank, forward) & empty;
    uint64_t capLeft = shift(pawns & ~FILE_A, captureLeft) & enemies;
    uint64_t capRight = shift(pawns & ~FILE_H, captureRight) & enemies;

    add_pawn_moves(list, push1, forward, promoRank);
    add_pawn_moves(list, push2, 2 * forward, promoRank, DOUBLE_PUSH);
    add_pawn_moves(list, capLeft, captureLeft, promoRank);
    add_pawn_moves(list, capRight, captureRight, promoRank);

    if (pos.en_passant_target() != -1)
    {
        uint64_t epBit = 1ULL << pos.en_passant_target();
        uint64_t epLeft = shift(pawns & ~FILE_A, captureLeft) & epBit;
        uint64_t epRight = shift(pawns & ~FILE_H, captureRight) & epBit;
        add_pawn_moves(list, epLeft, captureLeft, promoRank, EN_PASSANT);
        add_pawn_moves(list, epRight, captureRight, promoRank, EN_PASSANT);
    }
}

// Knights, bishops, rooks, queens and kings all generate moves the same way:
// for each piece, take its attacks and remove squares holding our own pieces.
// What's left is every quiet move and capture for that piece.
void generate_piece_moves(const Position& pos, MoveList& list, PieceType type)
{
    Color us = pos.side_to_move();
    uint64_t pieces = pos.bitboard(get_piece(us, type));
    uint64_t own = pos.pieces(us);
    uint64_t occupied = pos.occupied();

    while (pieces)
    {
        int from = pop_lsb(pieces);
        uint64_t targets = attacks_for(type, from, occupied) & ~own;
        while (targets)
        {
            int to = pop_lsb(targets);
            list.add(Move(from, to));
        }
    }
}

void generate_castling_moves(const Position& pos, MoveList& list)
{
    int base = (pos.side_to_move() == WHITE) ? 0 : 56;
    Color us = pos.side_to_move();
    auto them = static_cast<Color>(!us);
    uint64_t occ = pos.occupied();
    uint8_t rights = pos.castling_rights();
    uint8_t kingsideRight  = (us == WHITE) ? WHITE_KINGSIDE  : BLACK_KINGSIDE;
    uint8_t queensideRight = (us == WHITE) ? WHITE_QUEENSIDE : BLACK_QUEENSIDE;

    if ((rights & (kingsideRight | queensideRight)) == 0 || is_square_attacked(pos, base + 4, them)) return;

    if ((rights & kingsideRight)
    && !(occ & (0x60ULL << base))
    && !is_square_attacked(pos, base + 5, them)
    && !is_square_attacked(pos, base + 6, them))
    {
        list.add(Move(base + 4, base + 6, NO_TYPE, CASTLE));
    }

    if ((rights & queensideRight)
    && !(occ & (0x0EULL << base))
    && !is_square_attacked(pos, base + 2, them)
    && !is_square_attacked(pos, base + 3, them))
    {
        list.add(Move(base + 4, base + 2, NO_TYPE, CASTLE));
    }
}

} // namespace

void generate_moves(const Position& pos, MoveList& list)
{
    generate_pawn_moves(pos, list);
    generate_piece_moves(pos, list, KNIGHT);
    generate_piece_moves(pos, list, BISHOP);
    generate_piece_moves(pos, list, ROOK);
    generate_piece_moves(pos, list, QUEEN);
    generate_piece_moves(pos, list, KING);
    generate_castling_moves(pos, list);
}

bool is_square_attacked(const Position& pos, int sq, Color by)
{
    uint64_t occ = pos.occupied();

    uint64_t pawns   = pos.bitboard(get_piece(by, PAWN));
    uint64_t knights = pos.bitboard(get_piece(by, KNIGHT));
    uint64_t bishops = pos.bitboard(get_piece(by, BISHOP));
    uint64_t rooks   = pos.bitboard(get_piece(by, ROOK));
    uint64_t queens  = pos.bitboard(get_piece(by, QUEEN));
    uint64_t king    = pos.bitboard(get_piece(by, KING));

    if (knightAttacks[sq] & knights) return true;
    if (kingAttacks[sq] & king) return true;
    if (pawn_attacks(static_cast<Color>(!by), sq) & pawns) return true;
    if (rook_attacks(sq, occ) & (rooks | queens)) return true;
    if (bishop_attacks(sq, occ) & (bishops | queens)) return true;

    return false;
}

void generate_legal_moves(Position& pos, MoveList& legal)
{
    MoveList moves;
    generate_moves(pos, moves);

    Color us = pos.side_to_move();
    auto them = static_cast<Color>(!us);

    for (auto move : moves)
    {
        pos.make_move(move);
        int king_sq = std::countr_zero(pos.bitboard(get_piece(us, KING)));
        if (!is_square_attacked(pos, king_sq, them))
        {
            legal.add(move);
        }
        pos.undo_move();
    }
}

bool in_check(const Position& pos)
{
    Color us = pos.side_to_move();
    int king_sq = std::countr_zero(pos.bitboard(get_piece(us, KING)));
    return is_square_attacked(pos, king_sq, static_cast<Color>(!us));
}
