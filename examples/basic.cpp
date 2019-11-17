#include <iostream>
#include <chrono>

#include <boost/timer/timer.hpp>
#include <boost/format.hpp>

#include "../include/csv_event.hpp"

uint64_t current_timestamp_in_microseconds()
{
	return std::chrono::duration_cast< std::chrono::microseconds> (
			std::chrono::system_clock::now().time_since_epoch()
	).count();
}

void print_result(const csv_event::parsed_event& result)
{
	std::cout << "Metadata:" << std::endl;
	std::cout << result.metadata.dump() << std::endl;
	std::cout << boost::format("Payload (%d bytes):") % result.bytes.size() << std::endl;
	for(const auto& it: result.bytes )
	{
		std::cout << boost::format("%02X ") % uint32_t{it} ;
	}
	std::cout << std::endl;
}

std::string create_packet(size_t size)
{
	std::string tmp;

	tmp+=std::to_string(current_timestamp_in_microseconds())+","; // host_timestamp
	tmp+="PKT_RX,"; // event_type
	tmp+=std::to_string(size) + ","; // length
	for(uint32_t i=0;i<size;i++)  // payload
	{
		tmp+=(boost::format("%02x,") % uint32_t{i&0xFF}).str();
	}

	return tmp;
}

int main(int argc, const char* argv[])
{
	std::cout << "CSV event parser basic example"
			  << std::endl << std::endl;
	try {
		// 1. Compile program
		csv_event::csv_event_compiler comp;
		csv_event::program_sections compiled_program = comp.compile("data/protocol.xml");

		// 2. Create a test csv string
		const std::string csv_str = create_packet(1024);
		std::cout << "Input CSV:" << csv_str << std::endl;

		// 3. Instance an interpreter and parse
		csv_event::csv_event_interpreter interpreter(compiled_program);
		{
			boost::timer::cpu_timer timer;
			std::unique_ptr<csv_event::parsed_event> result = interpreter.parse(csv_str);
			csv_event::add_payload_to_metadata(*result);
			timer.stop();
			std::cout << "Result:" << std::endl;
			print_result(*result);
			std::cout << "Processing time: " << timer.format() << '\n';
		}
	}
	catch ( const csv_event::csv_event_compiler_exception& ex )
	{
		std::cerr << "Compiler exception: " << ex.what() << std::endl;
	}
	catch ( const csv_event::csv_event_interpreter_exception& ex )
	{
		std::cerr << "Interpreter exception: " << ex.what() << std::endl;
	}
	catch ( ... )
	{
		std::cerr << "Unknown exception." << std::endl;
	}
	return 0;
}

