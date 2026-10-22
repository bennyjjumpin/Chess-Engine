#include "position.h"
#include <sstream>
#include <stdexcept>
#include <string>

#include "zobrist.h"

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
    mHalfMoveClock = 0;
    mHash = 0;

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

    mHash = compute_hash();
}

// Loads a position from a FEN string, for example the starting position:
//   rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// The six fields are: piece placement, side to move, castling rights,
// en passant square, halfmove clock, and fullmove number. The last one
// isn't tracked yet, so it's read past and ignored.
// Throws std::invalid_argument if the string is malformed.
Position::Position(const std::string& fen)
{
    mBitboards.fill(0);
    mPieces.fill(NO_PIECE);
    mCastlingRights = 0;
    mTurn = WHITE;
    mEnPassantTarget = -1;
    mHalfMoveClock = 0;
    mHash = 0;

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

    // 5. Halfmove clock: moves since the last capture or pawn move.
    //    It's optional, so if it's missing the clock just stays at 0.
    int clock = 0;
    if (in >> clock)
    {
        if (clock < 0)
            throw std::invalid_argument("FEN halfmove clock can't be negative: " + fen);
        mHalfMoveClock = clock;
    }

    mHash = compute_hash();

    // 6. The fullmove number isn't needed by the engine, so it's ignored.
}


void Position::put_piece(Piece p, int sq)
{
    mPieces[sq] = p;
    set_bit(mBitboards[p], sq);
    mHash ^= Zobrist::keys.piece[p][sq];
}

void Position::remove_piece(int sq)
{
    mHash ^= Zobrist::keys.piece[mPieces[sq]][sq];   // before mPieces[sq] is cleared
    clear_bit(mBitboards[mPieces[sq]], sq);
    mPieces[sq] = NO_PIECE;
}

void Position::make_move(Move move)
{
    int captureSq = move.to;

    if (move.flag == EN_PASSANT)
        captureSq = (mTurn == WHITE) ? move.to - 8 : move.to + 8;

    mHistory.push_back(UndoInfo(move, mPieces[captureSq], mCastlingRights, mEnPassantTarget, mHalfMoveClock, mHash));

    if (get_type(mPieces[move.from]) == PAWN || mPieces[captureSq] != NO_PIECE)
    {
        mHalfMoveClock = 0;
    }
    else
    {
        mHalfMoveClock++;
    }

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

    mHash ^= Zobrist::keys.castling[mCastlingRights];
    mCastlingRights &= castlingMask[move.from] & castlingMask[move.to];
    mHash ^= Zobrist::keys.castling[mCastlingRights];

    // XOR out the old en passant key, if there was a square.
    if (mEnPassantTarget != -1)
        mHash ^= Zobrist::keys.enPassant[mEnPassantTarget % 8];

    if (move.flag == DOUBLE_PUSH)
    {
        mEnPassantTarget = (move.from + move.to) / 2;
    }
    else
    {
        mEnPassantTarget = -1;
    }

    // XOR in the new en passant key, if this move created a square.
    if (mEnPassantTarget != -1)
        mHash ^= Zobrist::keys.enPassant[mEnPassantTarget % 8];

    mHash ^= Zobrist::keys.blackToMove;
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
    mHalfMoveClock = info.halfMoveClock;

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

    mHash = info.hash;
}

// Builds the position's hash from scratch by XORing together the key for
// everything that's true about it. This is the slow, obviously-correct
// version: make_move keeps mHash updated incrementally instead, and this
// is used to set the starting value and to check those updates in tests.
uint64_t Position::compute_hash() const
{
    uint64_t hash = 0;

    // Every piece on its square.
    for (int p = 0; p < 12; p++)
    {
        uint64_t bb = mBitboards[p];   // a copy, so popping doesn't change the board
        while (bb)
        {
            int sq = pop_lsb(bb);
            hash ^= Zobrist::keys.piece[p][sq];
        }
    }

    // Whose turn it is. Only black gets a key: "white to move" is the
    // absence of it, which works because XORing it in twice cancels out.
    if (mTurn == BLACK)
        hash ^= Zobrist::keys.blackToMove;

    // Castling rights. They're a 4-bit number, 0 to 15, so they can index
    // the 16 castling keys directly.
    hash ^= Zobrist::keys.castling[mCastlingRights];

    // En passant, only if a square is set. The key is per file, since the
    // rank is always 3 or 6 and adds no information.
    if (mEnPassantTarget != -1)
        hash ^= Zobrist::keys.enPassant[mEnPassantTarget % 8];

    return hash;
}

// True if the current position occurred earlier in the game. In search, one
// earlier occurrence is enough: if repeating was good once, it can be repeated again.
bool Position::is_repetition() const
{
    int n = static_cast<int>(mHistory.size());
    for (int back = 2; back <= mHalfMoveClock && back <= n; back += 2)
    {
        if (mHistory[n - back].hash == mHash)
            return true;
    }
    return false;
}

bool Position::is_draw() const
{
    return mHalfMoveClock >= 100 || insufficient_material() || is_repetition();
}

// True if the current position has occurred three times in the game,
// counting this one. This is the actual game rule; the search uses the
// stricter is_repetition() instead.
bool Position::is_threefold() const
{
    int n = static_cast<int>(mHistory.size());
    int earlier = 0;
    for (int back = 2; back <= mHalfMoveClock && back <= n; back += 2)
    {
        if (mHistory[n - back].hash == mHash)
        {
            earlier++;
            if (earlier == 2)   // two earlier occurrences + the current one = three
                return true;
        }
    }
    return false;
}