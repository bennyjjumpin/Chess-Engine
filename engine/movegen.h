#ifndef CHESS_MOVEGEN_H
#define CHESS_MOVEGEN_H
#include "position.h"

void generate_moves(const Position& pos, MoveList& list);
bool is_square_attacked(const Position& pos, int sq, Color by);
void generate_legal_moves(Position& pos, MoveList& legal);
bool in_check(const Position& pos);

#endif //CHESS_MOVEGEN_H
