#include "../include/csv_event.hpp"
#include <gtest/gtest.h>

#include <iostream>
#include <chrono>

#include <boost/timer/timer.hpp>
#include <boost/format.hpp>

std::string create_rxtx_packet(uint64_t timestamp, const std::string& event_type, size_t size, int start_from)
{
	std::string tmp;

	tmp+=std::to_string(timestamp)+","; // host_timestamp
	tmp+=event_type+","; // event_type
	tmp+=std::to_string(size) + ","; // length
	for(uint32_t i=0;i<size;i++)  // payload
	{
		tmp+=(boost::format("%02x,") % uint32_t{(start_from+i)&0xFF}).str();
	}

	return tmp;
}

struct csvevent_test : public testing::Test {
	static constexpr const char* protocoL_definition_file = "data/protocol.xml";
	static constexpr size_t test_packet_size = 1024;

	void SetUp()
	{
	}

	void TearDown()
	{

	}
};

TEST_F(csvevent_test, parsing_test)
{
	csv_event::csv_event_compiler comp;
	csv_event::program_sections compiled_program = comp.compile(protocoL_definition_file);
	csv_event::csv_event_interpreter interpreter(compiled_program);
	const size_t n_iterations = 4;
	for(int i=0;i<n_iterations;i++)
	{
		const int start_index = 10+i;
		std::unique_ptr<csv_event::parsed_event> result = interpreter.parse(
				create_rxtx_packet(1000000000+i, "PKT_RX",test_packet_size,start_index));
		ASSERT_EQ ( 1000000000+i, result->metadata["host_timestamp"].get<uint64_t>());
		ASSERT_EQ ("PKT_RX", result->metadata["event_type"]);
		ASSERT_EQ (test_packet_size, result->metadata["length"]);
		int index = start_index;
		for (const auto& it: result->bytes)
		{
			ASSERT_EQ (index&0xFF, it);
			index++;
		}
	}
}

TEST_F(csvevent_test, perf_test_1000_packets)
{
	csv_event::csv_event_compiler comp;
	csv_event::program_sections compiled_program = comp.compile(protocoL_definition_file);
	csv_event::csv_event_interpreter interpreter(compiled_program);
	const std::string test_packet(create_rxtx_packet(1000000000, "PKT_RX",test_packet_size,0));
	const size_t n_iterations = 1000;
	{
		for(int i=0;i<n_iterations;i++)
		{
			std::unique_ptr<csv_event::parsed_event> result = interpreter.parse(test_packet);
		}
	}
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
