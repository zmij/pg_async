/*
 * configuration.cpp
 *
 *  Created on: Dec 16, 2015
 *      Author: zmij
 */

#include <awm/util/configuration.hpp>
#include <tip/version.hpp>
#include <iostream>
#include <fstream>

namespace awm {
namespace util {

LOCAL_LOGGING_FACILITY(CONFIG, OFF);

namespace po = boost::program_options;

configuration::configuration()
	: log_level(tip::log::logger::INFO),
	  log_use_colors(false)
{
	options_description generic("Generic options");
	generic.add_options()
		("config-file,c",
				po::value<std::string>(&config_file_name_), "Configuration file")
		("version", "Print version and exit")
		("help", "Print help and exit")
	;
	options_description log_options("Log options");
	log_options.add_options()
		("log-file", po::value<std::string>(&log_target), "Log file path")
		("log-date-format",
				po::value<std::string>(&log_date_format)->default_value("%d.%m", "DD.MM"),
				"Log date format. See boost::date_time IO documentation for available flags")
		("log-level,v",
			po::value<logger::event_severity>(&log_level)->default_value(logger::INFO),
			"Log level (TRACE, DEBUG, INFO, WARNING, ERROR)")
		("log-colors", "Output colored log")
	;

	command_line_.
		add(generic).
		add(log_options)
	;
	config_file_options_.
		add(log_options);
}

void
configuration::add_options(options_description const& desc)
{
	command_line_.add(desc);
	config_file_options_.add(desc);
}

void
configuration::add_command_line_options(options_description const& desc)
{
	command_line_.add(desc);
}

bool
configuration::parse_options(int argc, char* argv[])
{
	proc_name_ = argv[0];
	po::store(po::parse_command_line(argc, argv, command_line_), vm_);
	if (vm_.count("config-file")) {
		config_file_name_ = vm_["config-file"].as< std::string >();
		std::ifstream cfg(config_file_name_.c_str());
		if (cfg) {
			po::store(po::parse_config_file(cfg, config_file_options_), vm_);
		}
	}
	if (vm_.count("help")) {
		usage(std::cout);
		return false;
	}
	if (vm_.count("version")) {
		version(std::cout);
		return false;
	}
	try {
		po::notify(vm_);
	} catch (std::exception const& e) {
		std::cerr << e.what() << "\n";
		usage(std::cerr);
		return false;
	}
	log_use_colors = vm_.count("log-colors");
	return true;
}

void
configuration::version(std::ostream& os)
{
	os	<< proc_name_ << " version " << tip::VERSION
		<< " branch " << tip::BRANCH << "\n";
}

void
configuration::usage(std::ostream& os)
{
	os	<< proc_name_ << " version " << tip::VERSION << " branch " << tip::BRANCH
		<< " usage:\n"
		<< command_line_ << "\n";
}


} /* namespace util */
} /* namespace awm */
