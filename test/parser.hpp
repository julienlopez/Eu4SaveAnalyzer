#pragma once

#include <any>
#include <iosfwd>
#include <stack>
#include <vector>

void format(std::ostream& o, uint8_t byte);

class Parser
{
public:
	using DataBuffer = std::vector<uint8_t>;
	using DataContainer = std::stack<uint8_t>;

	Parser() = delete;
	Parser(DataContainer container);

	void run();

private:
    DataContainer m_data;
    std::vector<std::any> m_result;

	/**
	* @brief reads N bytes from m_data
	*/
	DataBuffer popN(const std::size_t N);

	void parseNext();

	static bool isAnInt(const DataBuffer& buffer);
	static bool isAFloat(const DataBuffer& buffer);
	static bool isAString(const DataBuffer& buffer);
};
