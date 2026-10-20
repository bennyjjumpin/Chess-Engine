#include "evaluate.h"

namespace {

// Indexed by PieceType: PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING.
// The king is 0: both sides always have exactly one, so its value cancels
// out, and a huge number like 20000 would only make scores harder to read.
constexpr int PIECE_VALUES[6] = {100, 320, 330, 500, 900, 0};

// Piece-square tables, written the way the board looks from white's side:
// the first row is rank 8, the last row is rank 1.
constexpr int PAWN_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

constexpr int KNIGHT_TABLE[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

constexpr int BISHOP_TABLE[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

constexpr int ROOK_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

constexpr int QUEEN_TABLE[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

constexpr int KING_TABLE[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// Indexed by PieceType, so PIECE_TABLES[get_type(p)] picks the right table.
constexpr const int* PIECE_TABLES[6] = {
    PAWN_TABLE, KNIGHT_TABLE, BISHOP_TABLE, ROOK_TABLE, QUEEN_TABLE, KING_TABLE
};

} // namespace

int evaluate(const Position& pos)
{
    int score[2] = {0, 0};   // indexed by Color: score[WHITE], score[BLACK]

    for (int p = 0; p < 12; p++)
    {
        Piece piece = static_cast<Piece>(p);
        Color color = get_color(piece);
        PieceType type = get_type(piece);
        uint64_t bb = pos.bitboard(piece);

        while (bb)
        {
            int sq = pop_lsb(bb);

            // The tables are written rank 8 first, but squares count from a1.
            // For white, flip the rank (sq ^ 56); black's pieces already line
            // up with the table, since black is white mirrored vertically.
            int index = (color == WHITE) ? (sq ^ 56) : sq;

            score[color] += PIECE_VALUES[type] + PIECE_TABLES[type][index];
        }
    }

    Color us = pos.side_to_move();
    return score[us] - score[!us];
}

