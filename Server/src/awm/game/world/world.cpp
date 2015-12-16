/*
 * world.cpp
 *
 *  Created on: 3 сент. 2015 г.
 *      Author: zelenov
 */

#include <awm/game/world/world.hpp>

#include <tip/log.hpp>

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace awm {
namespace game {
namespace world {

LOCAL_LOGGING_FACILITY(WORLD, DEBUG);

namespace fs = boost::filesystem;

struct world::impl {
	typedef std::map< std::string, game_data_reader_ptr > data_readers_type;

	data_readers_type data_readers_;
	cache_registry_type cache_registry_;
	impl() : cache_registry_([](game_data_reader*){})
	{
	}

	void
	read_file(game_data_reader_ptr rd, fs::directory_entry& path)
	{
		std::ifstream file(path.path().c_str(), std::ios_base::binary);
		try {
			if ((bool)file) {
				rd->read(path.path().stem().string(), file);
			} else {
				local_log(logger::ERROR) << "Failed to open file " << path.path();
			}

		} catch (cereal::ParseError const& e) {
			file.seekg(0);
			size_t line_no = 1;
			std::string line;
			size_t line_offset = 0;
			while (file && file.tellg() < e.offset) {
				line_offset = file.tellg();
				std::getline(file, line);
				++line_no;
			}
			std::cerr << "File: " << path.path().c_str() << "\n"
					<< "Error: " << e.what() << " at line " << line_no
					<< " char " << e.offset - line_offset << "\n"
					<< line << "\n";
		} catch (std::exception const& e) {
			local_log(logger::ERROR) << "Error parsing " << path.path().c_str() << " : " << e.what();
		}
	}

	void
	import(std::string const& root)
	{
		fs::path root_path(root);

		// Root traverse
		for (fs::directory_entry& section : fs::directory_iterator(root_path)) {
			auto f = this->data_readers_.find(section.path().stem().string());
			if (f == this->data_readers_.end()) {
				local_log(logger::WARNING) << "No data reader for section " << section.path().stem();
				continue;
			}
			game_data_reader_ptr rd = f->second;
			rd->clear();

			if (fs::is_directory(section.path())) {
				// Section traverse
				local_log(logger::TRACE) << "Section: " << section.path().filename();
				for (fs::directory_entry& item : fs::recursive_directory_iterator(section.path())) {
					if (fs::is_regular_file(item.path())) {
						local_log(logger::TRACE) << " \tProto: " << item.path().stem();
						read_file(rd, item);
					}
				}
			} else if (fs::is_regular_file(section.path())) {
				// Misc world entities
				local_log(logger::TRACE) << "World file: " << section.path().stem();
				read_file(rd, section);
			}

		}
	}
};

world::world() : pimpl_(new impl)
{
}

world&
world::instance()
{
	static world world_;
	return world_;
}

void
world::add_data_reader(std::string const& path, game_data_reader_ptr reader)
{
	local_log() << "Add a reader for path '" << path << "'";
	pimpl_->data_readers_.insert(std::make_pair(path, reader));
}

void
world::import(std::string const& root)
{
	pimpl_->import(root);
}

world::cache_registry_type&
world::get_cache_registry()
{
	return pimpl_->cache_registry_;
}

} /* namespace world */
} /* namespace game */
} /* namespace awm */
