#include <iostream>
#include <random>

#include "array_sequence.hpp"
#include "base64_encode_stream.hpp"
#include "lazy_sequence.hpp"
#include "read_stream.hpp"
#include "write_stream.hpp"

void Encode(std::unique_ptr<ReadOnlyStream<uint8_t>> src, std::unique_ptr<WriteOnlyStream<char>> out) {
    auto encoder = std::make_unique<Base64EncodeStream>(std::move(src));
    while (!encoder->IsEndOfStream()) {
        out->Write(encoder->Read());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage:\n";
        std::cout << "1) Encode file: " << argv[0] << " input_file output_file\n";
        std::cout << "2) Generate large test: " << argv[0] << " gen output_file size_in_bytes\n";
        return 1;
    }

    std::mt19937 rng(42);

    std::string mode = argv[1];

    auto parse = [](std::istream& is) -> uint8_t {
        return is.get();
    };

    auto serialize = [](std::ostream& os, char c) {
        os.put(c);
    };

    if (mode == "gen") {
        std::string outPath = argv[2];
        size_t size = std::stoull(argv[3]);

        auto gen = std::make_shared<LazySequence<uint8_t>>(
                       [&rng](SequencePtr<uint8_t>) {
                           return rng() % 127;
                       },
                       std::make_shared<ArraySequence<uint8_t>>(), 0)
                       ->GetSubsequence(0, size - 1);

        Encode(std::make_unique<LazySequenceReadStream<uint8_t>>(std::move(gen)),
               std::make_unique<FileWriteStream<char, decltype(serialize)>>(outPath, serialize));
    } else {
        std::string inPath = argv[1];
        std::string outPath = argv[2];

        Encode(std::make_unique<FileReadStream<uint8_t, decltype(parse)>>(inPath, parse),
               std::make_unique<FileWriteStream<char, decltype(serialize)>>(outPath, serialize));
    }
    std::cout << "Done.\n";

    return 0;
}
