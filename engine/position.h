#ifndef CHESS_POSITION_H
#define CHESS_POSITION_H
#include <cstdint>
#include <array>
#include <vector>
#include <string>

#include "types.h"
#include "move.h"


class Position {
private:
    std::array<uint64_t, 12> mBitboards;
    std::array<Piece, 64> mPieces;
    uint8_t mCastlingRights;
    Color mTurn;
    int mEnPassantTarget;
    int mHalfMoveClock = 0;
    uint64_t mHash;

    struct UndoInfo {
        Move move;
        Piece capturedPiece;
        uint8_t castlingRights;
        int enPassantTarget;
        int halfMoveClock;
        uint64_t hash;
    };

    std::vector<UndoInfo> mHistory;

    void put_piece(Piece p, int sq);

    void remove_piece(int sq);

    [[nodiscard]] uint64_t compute_hash() const;
public:
    Position();

    Position(const std::string &fen);

    void make_move(Move move);

    void undo_move();

    [[nodiscard]] bool is_repetition() const;

    [[nodiscard]] bool is_draw() const;

    [[nodiscard]] bool is_threefold() const;

    // Read-only access to the position (used by tests, move generation, GUI)
    [[nodiscard]] Piece piece_on(int sq) const { return mPieces[sq]; }
    [[nodiscard]] uint64_t bitboard(Piece p) const { return mBitboards[p]; }
    [[nodiscard]] uint8_t castling_rights() const { return mCastlingRights; }
    [[nodiscard]] int en_passant_target() const { return mEnPassantTarget; }
    [[nodiscard]] Color side_to_move() const { return mTurn; }
    [[nodiscard]] int half_move_clock() const { return mHalfMoveClock; }
    [[nodiscard]] uint64_t hash() const { return mHash; }

    [[nodiscard]] uint64_t pieces(Color c) const
    {
        uint64_t result = 0;
        for (int p = c * 6; p < c * 6 + 6; p++)
            result |= mBitboards[p];
        return result;
    }
    [[nodiscard]] uint64_t occupied() const { return pieces(WHITE) | pieces(BLACK); }
    // True if neither side has enough material to ever checkmate:
    // king vs. king, or king and one bishop or knight vs. king.
    [[nodiscard]] bool insufficient_material() const
    {
        // Any pawn, rook or queen on the board means mate is still possible.
        uint64_t heavy = mBitboards[W_PAWN] | mBitboards[B_PAWN]
                       | mBitboards[W_ROOK] | mBitboards[B_ROOK]
                       | mBitboards[W_QUEEN] | mBitboards[B_QUEEN];
        if (heavy)
            return false;

        // Only kings and minor pieces are left. A single minor piece can't mate.
        uint64_t minors = mBitboards[W_KNIGHT] | mBitboards[B_KNIGHT]
                        | mBitboards[W_BISHOP] | mBitboards[B_BISHOP];
        return std::popcount(minors) <= 1;
    }

};


#endif //CHESS_POSITION_H
