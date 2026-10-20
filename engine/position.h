#ifndef CHESS_POSITION_H
#define CHESS_POSITION_H
#include <cstdint>
#include <array>
#include <vector>
#include <bits/basic_string.h>

#include "types.h"
#include "move.h"


class Position {
private:
    std::array<uint64_t, 12> mBitboards;
    std::array<Piece, 64> mPieces;
    uint8_t mCastlingRights;
    Color mTurn;
    int mEnPassantTarget;

    struct UndoInfo {
        Move move;
        Piece capturedPiece;
        uint8_t castlingRights;
        int enPassantTarget;
    };

    std::vector<UndoInfo> mHistory;

    void put_piece(Piece p, int sq);

    void remove_piece(int sq);

public:
    Position();

    Position(const std::string &fen);

    void make_move(Move move);

    void undo_move();

    // Read-only access to the position (used by tests, move generation, GUI)
    [[nodiscard]] Piece piece_on(int sq) const { return mPieces[sq]; }
    [[nodiscard]] uint64_t bitboard(Piece p) const { return mBitboards[p]; }
    [[nodiscard]] uint8_t castling_rights() const { return mCastlingRights; }
    [[nodiscard]] int en_passant_target() const { return mEnPassantTarget; }
    [[nodiscard]] Color side_to_move() const { return mTurn; }

    [[nodiscard]] uint64_t pieces(Color c) const
    {
        uint64_t result = 0;
        for (int p = c * 6; p < c * 6 + 6; p++)
            result |= mBitboards[p];
        return result;
    }
    [[nodiscard]] uint64_t occupied() const { return pieces(WHITE) | pieces(BLACK); }

};


#endif //CHESS_POSITION_H
