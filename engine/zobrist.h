#ifndef CHESS_ZOBRIST_H
#define CHESS_ZOBRIST_H
#include <array>
#include <cstdint>

// Random numbers for Zobrist hashing, generated at compile time.
// A position's hash is the XOR of the keys for everything true about it.
namespace Zobrist {

    // SplitMix64: a small, fast generator with good statistical quality.
    // A fixed seed means the same keys every build, so hashes are reproducible.
    constexpr uint64_t splitmix64(uint64_t& state)
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    struct Keys {
        uint64_t piece[12][64];   // [Piece][square]
        uint64_t blackToMove;
        uint64_t castling[16];    // indexed by the 4-bit castling rights
        uint64_t enPassant[8];    // indexed by the en passant square's file
    };

    constexpr Keys generate()
    {
        Keys k{};
        uint64_t state = 0x1234ABCD5678EF90ULL;
        for (auto& pieceKeys : k.piece)
            for (auto& key : pieceKeys)
                key = splitmix64(state);
        k.blackToMove = splitmix64(state);
        for (auto& key : k.castling) key = splitmix64(state);
        for (auto& key : k.enPassant) key = splitmix64(state);
        return k;
    }

    inline constexpr Keys keys = generate();

} // namespace Zobrist

#endif //CHESS_ZOBRIST_H