# libcsvevent

Header-only library to parse data containing timestamps, metadata and byte sequences in CSV format. 
The result is stored in JSON (metadata) and a vector of bytes (binary data).

## Example

Given the CSV:

´´´
1574030438738237,PKT_RX,1024,00,01,02,03, ... ,fe,ff,
´´´

and the definition XML:

´´´
<eventdef>
    <timestamp name="host_timestamp"/>  
    <mnemonic name="event_type"/>
    <!-- A received packet  -->
    <condition event_type="PKT_RX">
        <integer name="length"/>
        <bytestream length="length"/>   
    </condition>
    <!-- A transmitted packet  -->
    <condition event_type="PKT_TX">
        <integer name="length"/>
        <bytestream length="length"/>   
    </condition>
    <!--  An error packet -->
    <condition event_type="PKT_ERROR">
        <mnemonic name="error_code"/>
    </condition>    
</eventdef>
´´´

´´´
#include <csv_event.hpp>

/* ... */

// 1. Compile program
csv_event::csv_event_compiler comp;
csv_event::program_sections compiled_program = comp.compile("data/protocol.xml");

// 2. Instance an interpreter and parse
csv_event::csv_event_interpreter interpreter(compiled_program);

// 3. 
std::unique_ptr<csv_event::parsed_event> result = interpreter.parse(
            "1574030438738237,PKT_RX,1024,00,01,02,03, ... ,fe,ff,");
csv_event::add_payload_to_metadata(*result);
´´´

will generate the following JSON:

´´´
{
    "event_type":"PKT_RX",
    "host_timestamp":1574030438738237,
    "length":1024,
    "payload": [0,1,2,3,...,255]
}
´´´

## Build instructions (conda)

´´´
conda-build -c conda-forge conda-recipe
´´´