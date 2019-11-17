#ifndef CSV_EVENT_HPP
#define CSV_EVENT_HPP

#include <string>
#include <variant>
#include <list>
#include <map>
#include <stack>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <nlohmann/json.hpp>
#include <pugixml.hpp>

namespace csv_event {

using json = nlohmann::json;

struct parsed_event
{
	std::vector<uint8_t> bytes;
	json metadata;
};

class opcode_base {
public:
	opcode_base(const std::string& name) :
		name(name){}
	std::string name;
};

class opcode_parse_timestamp : public opcode_base {
public:
	opcode_parse_timestamp(const std::string& name) : opcode_base(name) {}
};

class opcode_parse_integer : public opcode_base {
public:
	opcode_parse_integer(const std::string& name) : opcode_base(name) {}
};

class opcode_parse_mnemonic : public opcode_base {
public:
	opcode_parse_mnemonic(const std::string& name) : opcode_base(name) {}
};

class opcode_parse_bytestream : public opcode_base {
public:
	opcode_parse_bytestream(const std::string& name,const std::string& length_expr) :
			opcode_base(name)
		,	length_expr(length_expr)
	{}
	const std::string length_expr;
};

class opcode_parse_condition: public opcode_base {
public:
	opcode_parse_condition(const std::string& name,const std::string& identifier,
			const std::string& value, int subprogram_index):
			opcode_base(name)
		,	identifier(identifier)
		,	value(value)
		,	subprogram_index(subprogram_index)
	{}
	const std::string identifier;
	const std::string value;
	int subprogram_index;
};

using opcode = std::variant<opcode_parse_timestamp,opcode_parse_integer,opcode_parse_mnemonic,opcode_parse_bytestream,opcode_parse_condition>;
using opcode_seq = std::list<opcode>;
using tokenizer = boost::tokenizer<boost::escaped_list_separator<char>>;
using tokenizer_iterator = boost::tokenizer<boost::escaped_list_separator<char>>::iterator;
using program_sections = std::map<int,opcode_seq>;

class csv_event_interpreter_exception : public std::logic_error {
public:
	csv_event_interpreter_exception(const std::string& what) : std::logic_error(what) {}
};

class csv_event_interpreter
{
public:
	csv_event_interpreter(program_sections& program )
		:
			program(program)
	{
	}

    void operator()(const opcode_parse_timestamp& op )
    {
    	this->result->metadata.emplace(op.name, std::stoul(*this->it++,nullptr,0));
    }

    void operator()(const opcode_parse_integer& op )
	{
    	this->result->metadata.emplace(op.name,std::stoi(*this->it++));
	}

    void operator()(const opcode_parse_mnemonic& op )
	{
    	this->result->metadata.emplace(op.name,boost::algorithm::trim_copy(*this->it++));
	}

    void operator()(const opcode_parse_bytestream& op )
	{
		int n = this->result->metadata[op.length_expr].get<int>();
		if ( n )
		{
			this->result->bytes.reserve(n);
			do {
				this->result->bytes.emplace_back( std::stoul(*this->it++,nullptr,16) );
			} while(--n);
		}
	}

    void operator()(const opcode_parse_condition& op )
	{
    	const std::string& actual_value = this->result->metadata[op.identifier].get<std::string>();
    	if(!actual_value.compare(op.value))
    	{
        	for(const auto& op: this->program[op.subprogram_index])
        	{
        		std::visit(*this, op );
        	}
    	}
	}

    std::unique_ptr<parsed_event> parse(const std::string& input_csv_str)
    {
    	this->result = std::make_unique<parsed_event>();
    	tokenizer tok(input_csv_str);
    	this->it = tok.begin();
    	for(const auto& op: this->program[0])
    	{
    		std::visit(*this, op );
    	}
    	return std::move(this->result);
    }

private:
    program_sections& program;
    std::unique_ptr<parsed_event> result;
	tokenizer_iterator it;
};


class csv_event_compiler_exception : public std::logic_error {
public:
	csv_event_compiler_exception(const std::string& what) : std::logic_error(what) {}
};

class csv_event_compiler
{
public:
	csv_event_compiler()
		:
			function_map {
				{ "eventdef", [this](pugi::xml_node& n) { this->process_eventdef(n); } },
				{ "timestamp", [this](pugi::xml_node& n) { this->process_timestamp(n); } },
				{ "integer", [this](pugi::xml_node& n) { this->process_integer(n); } },
				{ "mnemonic", [this](pugi::xml_node& n) { this->process_mnemonic(n); } },
				{ "condition", [this](pugi::xml_node& n) { this->process_condition(n); } },
				{ "bytestream", [this](pugi::xml_node& n) { this->process_bytestream(n); } }
			}
	{

	}

	void process_eventdef(pugi::xml_node& n)
	{
		this->program.clear();
		this->current_program_index = 0;
		this->program.insert({this->current_program_index,opcode_seq()});
		for (pugi::xml_node_iterator it = n.begin(); it != n.end(); ++it)
		{
			this->function_map.at(it->name())(*it);
		}
	}

	void process_timestamp(pugi::xml_node& n)
	{
		this->program[this->current_program_index].push_back(opcode_parse_timestamp(n.attribute("name").as_string()));
	}

	void process_integer(pugi::xml_node& n)
	{
		this->program[this->current_program_index].push_back(opcode_parse_integer(n.attribute("name").as_string()));
	}

	void process_mnemonic(pugi::xml_node& n)
	{
		this->program[this->current_program_index].push_back(opcode_parse_mnemonic(n.attribute("name").as_string()));
	}

	void process_condition(pugi::xml_node& n)
	{
		// 1. push current program index to stack
		this->program_stack.push(this->current_program_index);

		// 2. create condition program name
		int condition_program_index = this->program.size();

    	// 3. Create condition subprogram
    	this->program.insert({condition_program_index,opcode_seq()});

    	// 4. Add condition instruction to current_program
    	auto ait = n.attributes_begin();
    	this->program[this->current_program_index].push_back(opcode_parse_condition(
    			"condition" + std::to_string(this->condition_counter++),
				ait->name(),
				ait->as_string(),
				condition_program_index)
    	);

    	// 5 change context and process children
		this->current_program_index = condition_program_index;
		for (pugi::xml_node_iterator it = n.begin(); it != n.end(); ++it)
		{
			this->function_map.at(it->name())(*it);
		}

		// 6. Pop current program from stack
		this->current_program_index = this->program_stack.top();
		this->program_stack.pop();
	}

	void process_bytestream(pugi::xml_node& n)
	{
		// first attribute is condition
		this->program[this->current_program_index].push_back(
			opcode_parse_bytestream(
				n.attribute("name").as_string(),
				n.attribute("length").as_string()
			)
		);
	}

	program_sections compile(const std::string& filename)
	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(filename.c_str());

		if (result)
		{
			auto root = doc.root();
			for (pugi::xml_node_iterator it = root.begin(); it != root.end(); ++it)
			{
				this->function_map.at(it->name())(*it);
			}
		}
		else
		{
			throw csv_event_compiler_exception(result.description());
		}
		return this->program;
	}

private:
	std::map<std::string,std::function<void(pugi::xml_node& node)>> function_map;
	std::stack<int> program_stack;
	size_t condition_counter;
	int current_program_index;
    program_sections program;
};

// Helper functions to add payload to metadata

inline void add_payload_to_metadata(csv_event::parsed_event& result, const std::string& field_name="payload")
{
	result.metadata.emplace(field_name, csv_event::json(result.bytes));
}

inline void add_payload_to_metadata_separate_fields(csv_event::parsed_event& result,
													const std::string& field_prefix="d")
{
	int idx = 0;
	for(const auto& it: result.bytes )
	{
		result.metadata.emplace(field_prefix+std::to_string(idx++), it);
	}
}

} // end namespace

#endif // CSV_EVENT_HPP

