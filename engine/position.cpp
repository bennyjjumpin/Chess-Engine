#include "position.h"
#include <sstream>
#include <string>

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

// Loads a position from a FEN string, for example the starting position:
//   rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// The six fields are: piece placement, side to move, castling rights,
// en passant square, halfmove clock, and fullmove number. The last two
// aren't tracked yet, so they're read past and ignored.
// Throws std::invalid_argument if the string is malformed.
Position::Position(const std::string& fen)
{
    mBitboards.fill(0);
    mPieces.fill(NO_PIECE);
    mCastlingRights = 0;
    mTurn = WHITE;
    mEnPassantTarget = -1;

    std::istringstream in(fen);
    std::string placement, side, castling, enPassant;
    if (!(in >> placement >> side >> castling >> enPassant))
        throw std::invalid_argument("FEN needs at least 4 fields: " + fen);

    // 1. Piece placement. Ranks are listed from 8 down to 1, separated by '/'.
    //    Within a rank, files go a to h. Digits mean that many empty squares.
    const std::string pieceChars = "PNBRQKpnbrqk";   // same order as the Piece enum
    int rank = 7;
    int file = 0;
    for (char c : placement)
    {
        if (c == '/')
        {
            if (file != 8)
                throw std::invalid_argument("FEN rank doesn't have 8 squares: " + fen);
            rank--;
            file = 0;
        }
        else if (c >= '1' && c <= '8')
        {
            file += c - '0';
        }
        else
        {
            auto index = pieceChars.find(c);
            if (index == std::string::npos)
                throw std::invalid_argument(std::string("unknown piece '") + c + "' in FEN: " + fen);
            if (rank < 0 || file > 7)
                throw std::invalid_argument("FEN has too many squares: " + fen);
            put_piece(static_cast<Piece>(index), rank * 8 + file);
            file++;
        }

        if (file > 8)
            throw std::invalid_argument("FEN rank has more than 8 squares: " + fen);
    }
    if (rank != 0 || file != 8)
        throw std::invalid_argument("FEN must describe exactly 8 ranks: " + fen);

    // 2. Side to move.
    if (side == "w")      mTurn = WHITE;
    else if (side == "b") mTurn = BLACK;
    else throw std::invalid_argument("FEN side to move must be 'w' or 'b': " + fen);

    // 3. Castling rights: any of KQkq, or '-' for none.
    if (castling != "-")
    {
        for (char c : castling)
        {
            switch (c)
            {
                case 'K': mCastlingRights |= WHITE_KINGSIDE;  break;
                case 'Q': mCastlingRights |= WHITE_QUEENSIDE; break;
                case 'k': mCastlingRights |= BLACK_KINGSIDE;  break;
                case 'q': mCastlingRights |= BLACK_QUEENSIDE; break;
                default:
                    throw std::invalid_argument(std::string("bad castling character '") + c + "' in FEN: " + fen);
            }
        }
    }

    // 4. En passant target square, like "e3", or '-' for none.
    if (enPassant != "-")
    {
        if (enPassant.size() != 2 || enPassant[0] < 'a' || enPassant[0] > 'h'
            || (enPassant[1] != '3' && enPassant[1] != '6'))
            throw std::invalid_argument("bad en passant square in FEN: " + fen);
        mEnPassantTarget = (enPassant[1] - '1') * 8 + (enPassant[0] - 'a');
    }

    // 5 and 6 (halfmove clock, fullmove number) are optional and ignored for now.
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