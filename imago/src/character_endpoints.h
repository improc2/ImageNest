#pragma once
#include <vector>
#include <string>

namespace imago
{
	struct EndpointsRecord
	{
		char c;
		int min, max;
		EndpointsRecord(char _c, int _min, int _max);
	};

	class EndpointsData : public std::vector<EndpointsRecord>
	{
	public:
		EndpointsData();
		void getImpossibleToWrite(int endpointsCount, std::string& probably, std::string& surely) const;
	};
}