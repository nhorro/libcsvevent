# libcsvevent

Header-only C++17 library to parse data containing timestamps, metadata and byte sequences in CSV format. 
The result is stored in JSON (metadata) and a vector of bytes (binary data).

It is used to collect binary data obtained from HW protocols such as CAN, I2C, RS485 that can be parsed at byte level or directly ingested by a document database such as ElasticSearch or MongoDB.

### Example

Given the CSV:

```csv
1574030438738237,PKT_RX,1024,00,01,02,03, ... ,fe,ff,
```

and the definition XML:

```xml
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
```

the following code:

```c++
#include <csv_event.hpp>

/* ... */

// 1. Compile program.
csv_event::csv_event_compiler comp;
csv_event::program_sections compiled_program = comp.compile("data/protocol.xml");

// 2. Instance an interpreter and parse.
csv_event::csv_event_interpreter interpreter(compiled_program);
std::unique_ptr<csv_event::parsed_event> result = interpreter.parse(
            "1574030438738237,PKT_RX,1024,00,01,02,03, ... ,fe,ff,");

// 3. Optionally, add payload bytes to metadata.
csv_event::add_payload_to_metadata(*result);
```

will generate the following JSON:

```json
{
    "event_type":"PKT_RX",
    "host_timestamp":1574030438738237,
    "length":1024,
    "payload": [0,1,2,3,...,255]
}

```

### Build instructions

```bash
conda-build -c conda-forge conda-recipe
```