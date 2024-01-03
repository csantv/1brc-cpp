#include <iostream>
#include <fstream>
#include <map>
#include <cstdint>
#include <execution>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <array>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

using std::string_view;
using entry_type = std::pair<string_view, std::vector<float>>;

struct Chunk {
    string_view str_chunk;
    int idx;
};

auto average(const std::vector<float>& vec) -> float
{
    if (vec.empty()) {
        return 0;
    }

    const auto count = static_cast<float>(vec.size());
    return std::reduce(std::execution::par_unseq, vec.begin(), vec.end()) / count;
}

auto main(int argc, char *argv[]) -> int
{
    if (argc < 2) {
        return 1;
    }
    // map file to ram
    struct stat file_stat;
    const int filde = open(argv[1], O_RDWR | O_LARGEFILE);
    fstat(filde, &file_stat);

    char* memblock = static_cast<char*>(
        mmap(nullptr, file_stat.st_size, PROT_READ, MAP_SHARED, filde, 0)
    );
    if (memblock == MAP_FAILED) {
        std::cout << "Could not map file\n";
        return 1;
    }

    const string_view view = memblock;
    const auto filesize = file_stat.st_size;
    const auto num_procs = get_nprocs();
    const int64_t size_per_thread = filesize / num_procs;

    std::vector<struct Chunk> chunks;
    chunks.reserve(num_procs);

    char *runner = memblock;
    string_view::size_type begin_idx = size_per_thread;
    string_view::size_type prev_idx = 0;
    for (int i = 0; i < num_procs; ++i) {
        const auto next_idx = view.find('\n', begin_idx);
        if (next_idx == string_view::npos) {
            break;
        }
        chunks.push_back({
            .str_chunk = string_view {runner, next_idx - prev_idx},
            .idx = i,
        });
        runner += next_idx - prev_idx + 1;
        prev_idx = next_idx + 1;
        begin_idx = next_idx + size_per_thread;
    }
    chunks.push_back({
        .str_chunk = string_view {runner},
        .idx = num_procs - 1,
    });

    std::vector<std::unordered_map<std::string_view, std::vector<float>>> result_map (num_procs);
    std::for_each(std::execution::par_unseq, chunks.begin(), chunks.end(), [&result_map] (struct Chunk& chunk) {
        const int num_cities = 500;
        const int to_reserve = 1048576;
        auto& local_map = result_map[chunk.idx];

        auto view = chunk.str_chunk;
        local_map.reserve(num_cities);
        string_view::size_type begin = 0;
        while (true) {
            const auto end = view.find('\n', begin);
            const auto token = view.substr(begin, end - begin);
            if (token.empty()) {
                break;
            }

            const auto semicolon = token.find(';');
            const auto city = token.substr(0, semicolon);

            const auto measurement_str = token.substr(semicolon + 1);
            const auto measurement = std::stof(std::string{measurement_str});
            const auto [entry, inserted] = local_map.insert({city, {}});
            auto& vec = entry->second;
            if (inserted) {
                vec.reserve(to_reserve);
            }
            vec.push_back(measurement);
            if (end == string_view::npos) {
                break;
            }
            begin = end + 1;
        }
    });

    // merge all maps
    std::unordered_map<string_view, std::vector<float>> result;
    for (const auto& map: result_map) {
        for (const auto& [key, value]: map) {
            const auto [entry, inserted] = result.emplace(key, value);
            if (!inserted) {
                auto& vec = entry->second;
                vec.insert(vec.end(), value.begin(), value.end());
            }
        }
    }

    // sort alphabetically
    std::vector<std::pair<string_view, std::vector<float>>> result_vector (result.begin(), result.end());
    std::sort(result_vector.begin(), result_vector.end(), [] (const entry_type& pair1, const entry_type& pair2) {
        return pair1.first < pair2.first;
    });

    // get values
    for (const auto& [entry, value]: result_vector) {
        const auto min = *std::min_element(std::execution::par_unseq, value.begin(), value.end());
        const auto max = *std::max_element(std::execution::par_unseq, value.begin(), value.end());
        const auto mean = average(value);
        std::cout << entry << " " << min << " " << max << " " << mean << " " << '\n';
    }

    return 0;
}
