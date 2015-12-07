//
// Created by zmij on 16.10.15.
//

#ifndef TIP_SERVER_STATS_HPP
#define TIP_SERVER_STATS_HPP

#include <tip/http/server/request_handler.hpp>

namespace awm {
namespace game {
namespace authn {

class current_online : public tip::http::server::request_handler {
public:
	current_online() {}
	virtual ~current_online() {}

private:

	virtual void
	do_handle_request(tip::http::server::reply r);
};
}
}
} // namespace awm

#endif //TIP_SERVER_STATS_HPP
