#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;
int main() {
    fs::path p(getenv("HOME"));
    std::error_code ec;
    auto it = fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied, ec);
    auto end = fs::recursive_directory_iterator();
    int count = 0;
    while (it != end) {
        if (ec) {
            it.increment(ec);
            continue;
        }
        try {
            if (it->is_regular_file() && it->path().extension() == ".gguf") {
                std::cout << "Found: " << it->path().string() << std::endl;
            }
        } catch (...) {}
        it.increment(ec);
        count++;
        if(count > 1000000) break;
    }
    std::cout << "Done." << std::endl;
}
