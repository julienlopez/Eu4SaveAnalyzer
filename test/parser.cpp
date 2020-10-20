#include "parser.hpp"

#include <cctype>
#include <iostream>
#include <iomanip>

void format(std::ostream& o, uint8_t byte)
{
    if(std::isalpha(byte) || std::isdigit(byte) || byte == 0x28)
        o << byte;
    else
        o << "0x" << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)byte << ' ';
}

std::ostream& operator<<(std::ostream& o, const std::vector<uint8_t>& v)
{
    for(const auto i : v)
        format(o, i);
    return o;
}

Parser::Parser(DataContainer container)
    : m_data(std::move(container))
{
}

void Parser::run()
{
    std::cout << "Parser::run()" << std::endl;
    const auto magic_number = popN(6);
    std::cout << magic_number << std::endl;
    std::cout << popN(2) << std::endl;
    std::cout << popN(2) << std::endl;
    std::cout << std::endl;
}

std::vector<uint8_t> Parser::popN(const std::size_t N)
{
    std::vector<uint8_t> res; 
    for(std::size_t i = 0; i < N; i++)
    {
        res.push_back(m_data.top());
        m_data.pop();
    }
    return res;
}
