# pg_async
Asynchronous client library for PostgreSQL, protocol v3.

## Motivation

When developing software we deal a lot with external network resources. Most of them can be accessed through an asynchronous interface, so we don't have to block to wait for a request to finish. The lack of modern C++ asynchronous interface was the reason for writing this library.

## Overview

pg_async is an unofficial PostreSQL database asynchronous client library written in modern C++ (std=c++11) on top of boost::asio library used for asynchronous network input-output.

### Features

* Asynchronous operations
* Connection pooling
* Database aliases
* Standard container-compliant resultset interface
* Execution of prepared statements
* Multiple result sets for simple query mode
* Data row extraction to tuples
* Flexible datatype conversion
* Compile-time statement parameter types binding and checking
* Extensible datatypes input-output system
* TCP or UNIX socket connection

### Usage

```c++
#include <tip/db/pg.hpp>

using namespace tip::db::pg;

// Register conntions
db_service::add_connection("main=user@localhost:5432[the_database]");
db_service::add_connection("logs=user@localhost:5432[logs_db]");

// Run the service
db_service::run(); // Will block the thread

// Execute queries
db_service::begin(            // Start a transaction
  "main"_db,                  // Database alias
  [](transaction_ptr tran)    // Function to be run inside a transaction
  {
    query(
      tran,                                 // Transaction handle
      "select * from pg_catalog.pg_type"    // SQL statement
    )   
    ([](transaction_ptr tran, resultset res, bool complete) // Query results handler
    {
      for (auto field_desc : 
              res.row_description()) {    // Iterate the field descriptions
        std::cout << field_desc.name << "\t";
      }
      std::cout << "\n";
      for (auto row : res) {              // Iterate the resultset
        int a, b;
        row.to(std::tie(a, b));           // Extract query result row to a tuple
        for (auto field : row) {          // Iterate the fields of the row
          std::cout << field.coalesce( "null" ) 
              << "\t";
        }
      }
    },
    [](db_error const& error)             // Query error handler
    {
    });
  },
  [](db_error cosnt& error)   // Function to be called in case of a transaction error
  {
  }
);
```

For more information please see accompanying doxygen documentation.

## Installation

```bash
git clone git@github.com:zmij/pg_async.git
cd pg_async
mkdir build
cd build
cmake ..
make install
```
