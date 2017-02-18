/*
 * connection_observer.hpp
 *
 *  Created on: Jun 3, 2016
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_OBSERVER_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_OBSERVER_HPP_

#include <cxxabi.h>
#include <string>

#include <afsm/fsm.hpp>
#include <pushkin/util/demangle.hpp>
#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

template < typename T >
::std::string
demangle()
{
    int status {0};
    char* ret = abi::__cxa_demangle( typeid(T).name(), nullptr, nullptr, &status );
    ::std::string res{ret};
    if(ret) free(ret);
    return res;
}

struct connection_observer : ::afsm::detail::null_observer {
    template < typename FSM, typename Event >
    void
    start_process_event(FSM const& fsm, Event const&) const noexcept
    {
        using decayed_event = typename ::std::decay<Event>::type;
        using tip::util::ANSI_COLOR;
        fsm.log() << demangle<Event>() << ": Start processing";
    }

    template < typename FSM >
    void
    start_process_event(FSM const& fsm, ::afsm::none const&) const noexcept
    {
        fsm.log() << "[default]: Start processing";
    }

    template < typename FSM, typename SourceState, typename TargetState, typename Event >
    void
    state_changed(FSM const& fsm, SourceState const&, TargetState const&, Event const&) const noexcept
    {
        fsm.log() << "State changed to " << demangle<TargetState>();
    }

    template < typename FSM, typename Event >
    void
    processed_in_state(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log() << demangle<Event>() << ": processed";
    }

    template < typename FSM, typename Event >
    void
    enqueue_event(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::MAGENTA
                << demangle<Event>() << ": Enqueue";
    }

    template < typename FSM >
    void
    start_process_events_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::MAGENTA
                << "Start processing event queue";
    }
    template < typename FSM >
    void
    end_process_events_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::MAGENTA
                << "End processing event queue";
    }

    template < typename FSM, typename Event >
    void
    defer_event(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log() << (util::ANSI_COLOR::CYAN | util::ANSI_COLOR::BRIGHT)
                << demangle<Event>() << ": Defer";
    }

    template < typename FSM >
    void
    start_process_deferred_queue(FSM const& fsm, ::std::size_t size) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::CYAN
                << "Start processing deferred queue size " << size;
    }
    template < typename FSM >
    void
    end_process_deferred_queue(FSM const& fsm, ::std::size_t remain) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::CYAN
                << "End processing deferred queue. Remain " << remain;
    }

    template < typename FSM >
    void
    skip_processing_deferred_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << (util::ANSI_COLOR::YELLOW | util::ANSI_COLOR::BRIGHT)
                << "Skip processing deferred queue";
    }
    template < typename FSM >
    void
    postpone_deferred_events(FSM const& fsm, ::std::size_t count) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::YELLOW
                << "Postpone " << count << " deferred events";
    }
    template < typename FSM >
    void
    drop_deferred_event(FSM const& fsm) const noexcept
    {
        fsm.log() << (util::ANSI_COLOR::RED | util::ANSI_COLOR::BRIGHT)
                << "Drop deferred event";
    }

    template < typename FSM, typename Event >
    void
    reject_event(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log(log::logger::ERROR) << (util::ANSI_COLOR::RED | util::ANSI_COLOR::BRIGHT)
                << demangle<Event>() << ": Reject.";
    }
};

}  /* namespace detail */
}  /* namespace pg */
}  /* namespace db */
}  /* namespace tip */



#endif /* TIP_DB_PG_DETAIL_CONNECTION_OBSERVER_HPP_ */
