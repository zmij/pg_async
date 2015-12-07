//
// Created by zmij on 16.10.15.
//

#include <awm/game/auth/stats.hpp>
#include <awm/game/auth/session.hpp>
#include <awm/game/auth/protocol.hpp>

#include <tip/http/server/json_body_context.hpp>

void awm::game::authn::current_online::do_handle_request(
		tip::http::server::reply r)
{
	using tip::http::server::json_body_context;
	json_body_context& json = tip::http::server::use_context<json_body_context>(r);
	online_stats os{ session::LRU().size() };
	os.serialize(json.outgoing());
	r.done(tip::http::response_status::ok);
}
