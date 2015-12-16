/*
 * configuration.hpp
 *
 *  Created on: Dec 16, 2015
 *      Author: zmij
 */

#ifndef AWM_UTIL_CONFIGURATION_HPP_
#define AWM_UTIL_CONFIGURATION_HPP_

#include <string>
#include <boost/program_options.hpp>
#include <tip/log.hpp>

namespace awm {
namespace util {

class configuration {
public:
	typedef boost::program_options::options_description options_description;
	typedef boost::program_options::variables_map variables_map;
public:
	virtual ~configuration() {}

	/**
	 * Add options both to config file and command line
	 * @param
	 */
	void
	add_options(options_description const&);
	/**
	 * Add options to command line only
	 * @param
	 */
	void
	add_command_line_options(options_description const&);

	bool
	parse_options(int argc, char* argv[]);

	void
	usage(std::ostream& os);
	void
	version(std::ostream& os);

	variables_map const&
	options() const
	{ return vm_; }
public:
	//@{
	/** @name Log options */
	std::string						log_target;
	std::string						log_date_format;
	tip::log::logger::event_severity
									log_level;
	bool							log_use_colors;
	//@}
protected:
	configuration();
private:
	options_description command_line_;
	options_description config_file_options_;

	std::string			config_file_name_;
	std::string			proc_name_;

	variables_map		vm_;
};

} /* namespace util */
} /* namespace awm */

#endif /* AWM_UTIL_CONFIGURATION_HPP_ */
