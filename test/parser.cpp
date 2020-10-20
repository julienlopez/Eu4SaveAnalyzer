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

std::ostream& operator<<(std::ostream& o, const std::any& a)
{
    if(!a.has_value())
        o << "`empty`";
    else if(a.type() == typeid(std::string{}))
        o << std::any_cast<std::string>(a);
    else if(a.type() == typeid(char{}))
        o << std::any_cast<char>(a);
    else if(a.type() == typeid(int32_t{}))
        o << std::any_cast<int32_t>(a);
    else if(a.type() == typeid(double{}))
        o << std::any_cast<double>(a);
    else
        o << "`unsupported type`";
    return o;
}

std::ostream& operator<<(std::ostream& o, const Parser::DataBuffer& v)
{
    for(const auto i : v)
        format(o, i);
    return o;
}

std::string toString(const Parser::DataBuffer& v)
{
    return {begin(v), end(v)};
}

int32_t toInt(const Parser::DataBuffer& v)
{
    if(v.size() != 4) throw std::runtime_error("invalid buffer for toInt");
    int32_t res = 0;
    res += (int32_t)v[0];
    res += (int32_t)v[1] << 8;
    res += (int32_t)v[2] << 16;
    res += (int32_t)v[3] << 24;
    return res;
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
    m_result.push_back(toString(magic_number));
    std::cout << popN(2) << std::endl;
    parseNext();
    parseNext();
    parseNext();
    std::cout << "finally : " << std::endl;
    std::cout << std::dec << std::endl;
    for(const auto& res : m_result)
        std::cout << res << std::endl;
}

auto Parser::popN(const std::size_t N) -> DataBuffer
{
    DataBuffer res; 
    for(std::size_t i = 0; i < N; i++)
    {
        res.push_back(m_data.top());
        m_data.pop();
    }
    return res;
}

void Parser::parseNext()
{
    const auto tag = popN(2);
    std::cout << tag << std::endl;
    if(tag == DataBuffer{0x01, 0x00})
        m_result.push_back('=');
    else if(tag == DataBuffer{0x03, 0x00})
        m_result.push_back('{');
    else if(tag == DataBuffer{0x04, 0x00})
        m_result.push_back('}');
    else if(isAnInt(tag))
    {
        const auto value = popN(4);
        std::cout << "parsing int : " << value << std::endl;
        m_result.push_back(toInt(value));
    }
    else
        std::cout << "nope" << std::endl;
    // if(!m_data.empty()) parseNext();
}

bool Parser::isAnInt(const DataBuffer& buffer)
{
    return buffer == DataBuffer{0x0c, 0x00} || buffer == DataBuffer{0x14, 0x00};
}
