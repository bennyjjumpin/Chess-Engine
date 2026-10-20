//
// Created by Ben's PC on 10/18/2026.
//

#include "position.h"

#include <algorithm>

namespace {
    constexpr std::array<uint8_t, 64> castlingMask = [] {
        std::array<uint8_t, 64> mask{};
        mask.fill(0x0F);

        mask[0]  = 0x0F & ~WHITE_QUEENSIDE;                    // a1
        mask[7]  = 0x0F & ~WHITE_KINGSIDE;                     // h1
        mask[4]  = 0x0F & ~(WHITE_KINGSIDE | WHITE_QUEENSIDE); // e1

        mask[56] = 0x0F & ~BLACK_QUEENSIDE;                    // a8
        mask[63] = 0x0F & ~BLACK_KINGSIDE;                     // h8
        mask[60] = 0x0F & ~(BLACK_KINGSIDE | BLACK_QUEENSIDE); // e8

        return mask;
    }();
}


Position::Position()
{
    mBitboards.fill(0x00000000);
    mPieces.fill(NO_PIECE);
    mCastlingRights = 0x0F;
    mTurn = WHITE;
    mEnPassantTarget = -1;

    constexpr std::array<PieceType, 8> backRank = {
        ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK
    };

    for (int file = 0; file < 8; file++)
    {
        put_piece(get_piece(WHITE, backRank[file]), file);
        put_piece(get_piece(WHITE, PAWN), file + 8);
        put_piece(get_piece(BLACK, PAWN), file + 48);
        put_piece(get_piece(BLACK, backRank[file]), file + 56);
    }
}

void Position::put_piece(Piece p, int sq)
{
    mPieces[sq] = p;
    set_bit(mBitboards[p], sq);
}

void Position::remove_piece(int sq)
{
    clear_bit(mBitboards[mPieces[sq]], sq);
    mPieces[sq] = NO_PIECE;
}

void Position::make_move(Move move)
{
    int captureSq = move.to;
    if (move.flag == EN_PASSANT)
        captureSq = (mTurn == WHITE) ? move.to - 8 : move.to + 8;

    mHistory.push_back(UndoInfo(move, mPieces[captureSq], mCastlingRights, mEnPassantTarget));

    if (mPieces[captureSq] != NO_PIECE)
    {
        remove_piece(captureSq);
    }

    if (move.is_promotion())
    {
        put_piece(get_piece(mTurn, move.promotion), move.to);
    }
    else
    {
        put_piece(mPieces[move.from], move.to);
    }
    remove_piece(move.from);

    if (move.is_castle())
    {
        if (move.to > move.from)
        {
            put_piece(mPieces[move.from + 3], move.from + 1);
            remove_piece(move.from + 3);
        }
        else
        {
            put_piece(mPieces[move.from - 4], move.from - 1);
            remove_piece(move.from - 4);
        }
    }

    mCastlingRights &= castlingMask[move.from] & castlingMask[move.to];

    if (move.flag == DOUBLE_PUSH)
    {
        mEnPassantTarget = (move.from + move.to) / 2;
    }
    else
    {
        mEnPassantTarget = -1;
    }

    mTurn = static_cast<Color>(!mTurn);
}

void Position::undo_move()
{
    if (mHistory.empty()) return;

    UndoInfo info = mHistory.back();
    mHistory.pop_back();

    mTurn = static_cast<Color>(!mTurn);

    int captureSq = info.move.to;
    if (info.move.flag == EN_PASSANT)
        captureSq = (mTurn == WHITE) ? info.move.to - 8 : info.move.to + 8;

    mCastlingRights = info.castlingRights;
    mEnPassantTarget = info.enPassantTarget;

    if (info.move.is_castle())
    {
        if (info.move.to > info.move.from)
        {
            put_piece(mPieces[info.move.from + 1], info.move.from + 3);
            remove_piece(info.move.from + 1);
        }
        else
        {
            put_piece(mPieces[info.move.from - 1], info.move.from - 4);
            remove_piece(info.move.from - 1);
        }
    }

    if (info.move.is_promotion())
    {
        put_piece(get_piece(mTurn, PAWN), info.move.from);
    }
    else
    {
        put_piece(mPieces[info.move.to], info.move.from);
    }
    remove_piece(info.move.to);

    if (info.capturedPiece != NO_PIECE)
    {
        put_piece(info.capturedPiece, captureSq);
    }
}