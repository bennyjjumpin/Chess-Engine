#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H
#include "types.h"

enum MoveFlag : uint8_t {
    NORMAL, DOUBLE_PUSH, EN_PASSANT, CASTLE
};

struct Move {
    uint8_t from;
    uint8_t to;
    PieceType promotion;
    MoveFlag flag;
    [[nodiscard]] bool is_promotion() const { return promotion != NO_TYPE; }
    [[nodiscard]] bool is_castle() const { return flag == CASTLE; }
    Move() = default;
    Move(uint8_t from, uint8_t to, PieceType promotion = NO_TYPE, MoveFlag flag = NORMAL) : from(from), to(to), promotion(promotion), flag(flag)  {}
};

struct MoveList {
    std::array<Move, 256> moves;
    int count = 0;
    void add(Move m) {moves[count] = m; count++;}
    [[nodiscard]] int size() const {return count;}
    Move operator[] (int i) const {return moves[i];}
    [[nodiscard]] const Move* begin() const {return moves.data();}
    [[nodiscard]] const Move* end() const {return moves.data() + count;}
};

#endif //CHESS_MOVE_H
