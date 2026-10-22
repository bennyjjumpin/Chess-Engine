#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H
#include "position.h"

constexpr int INFINITY_SCORE = 1'000'000;
constexpr int MATE_SCORE = 100'000;

Move find_best_move(Position& pos, int maxDepth = 64, int timeLimitMs = 1000);

#endif //CHESS_SEARCH_H
