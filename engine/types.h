#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H
#include <cstdint>

enum Piece : uint8_t {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,   // 0–5
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,   // 6–11
    NO_PIECE                                               // 12
};

enum Color : bool {
    WHITE, BLACK
};

enum PieceType : uint8_t {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_TYPE
};

constexpr uint8_t WHITE_KINGSIDE  = 1 << 0;  // 0001 = 1
constexpr uint8_t WHITE_QUEENSIDE = 1 << 1;  // 0010 = 2
constexpr uint8_t BLACK_KINGSIDE  = 1 << 2;  // 0100 = 4
constexpr uint8_t BLACK_QUEENSIDE = 1 << 3;  // 1000 = 8

constexpr uint64_t FILE_A = 0x0101010101010101;
constexpr uint64_t FILE_H = 0x8080808080808080;
constexpr uint64_t FILE_B = 0x0202020202020202;
constexpr uint64_t FILE_G = 0x4040404040404040;

constexpr uint64_t RANK_1 = 0x00000000000000FF;
constexpr uint64_t RANK_3 = 0x0000000000FF0000;
constexpr uint64_t RANK_6 = 0x0000FF0000000000;
constexpr uint64_t RANK_8 = 0xFF00000000000000;

/// return true if value at sq is == 1
inline bool is_set(uint64_t bb, int sq) {
    return bb & (1ULL << sq);
}

/// sets value at sq to 1
inline void set_bit(uint64_t& bb, int sq) {
    bb |= (1ULL << sq);
}

/// sets value at sq to 0
inline void clear_bit(uint64_t& bb, int sq) {
    bb &= ~(1ULL << sq);
}

/// returns color from piece
inline Color get_color(Piece p) {
    return static_cast<Color>(p / 6);
}

/// returns type from piece
inline PieceType get_type(Piece p) {
    return static_cast<PieceType>(p % 6);
}

/// returns piece from type and color
inline Piece get_piece(Color c, PieceType t) {
    return static_cast<Piece>((c * 6) + t);
}

/// pops lowest bit in given bitboard
inline int pop_lsb(uint64_t& bb) {
    int lsb = std::countr_zero(bb);
    bb &= bb - 1;
    return lsb;
}

/// Shift up for positive n, down for negative n
inline uint64_t shift(uint64_t bb, int n) {
    return n > 0 ? bb << n : bb >> -n;
}

#endif //CHESS_TYPES_H
