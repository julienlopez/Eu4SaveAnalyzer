#include "parser.hpp"

#include "Zpp.h"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cerr << "expected usage: test save_file" << std::endl;
        return 1;
    }

    const std::filesystem::path save_file{argv[1]};
    std::cout << "openinig " << save_file << std::endl;
    zppZipArchive zip{save_file.string()};
    auto* meta_file = zip.findInArchive("meta");
    if(!meta_file)
    {
        std::cerr << "unable to open meta file" << std::endl;
        return 2;
    }
    izppstream stream;
    stream.open("meta", std::ios_base::binary);
    if(!stream)
    {
        std::cerr << "unable to open stream" << std::endl;
        return 2;
    }
    std::vector<uint8_t> content{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    std::cout << "size = " << content.size() << std::endl;
    for(const auto i : content)
        format(std::cout, i);
    std::cout << std::endl;

    Parser p{std::stack<uint8_t>{std::deque<uint8_t>{rbegin(content), rend(content)}}};
    p.run();

    return 0;
}
