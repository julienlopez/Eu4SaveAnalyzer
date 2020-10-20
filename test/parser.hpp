#pragma once

#include <iosfwd>
#include <stack>
#include <vector>

void format(std::ostream& o, uint8_t byte);

class Parser
{
public:
	// using DataContainer = std::vector<uint8_t>;
	using DataContainer = std::stack<uint8_t>;

	Parser() = delete;
	Parser(DataContainer container);

	void run();

private:
    DataContainer m_data;

	std::vector<uint8_t> popN(const std::size_t N);
};
