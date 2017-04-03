/*
 * fiber_yeild.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: zmij
 */

#ifndef WIRE_UTIL_FIBER_YIELD_HPP_
#define WIRE_UTIL_FIBER_YIELD_HPP_

#include <pushkin/asio/asio_config.hpp>

namespace psst {
namespace asio {
namespace fiber {

struct yield_t {
    constexpr yield_t() noexcept = default;
    constexpr explicit yield_t(error_code* ec) noexcept : ec_{ec} {}
    constexpr explicit yield_t(error_code& ec) noexcept : ec_{&ec} {}

    constexpr yield_t
    operator[](error_code& ec) const
    {
        return yield_t(ec);
    }

    error_code*    ec_{ nullptr };
};

} /* namespace fiber */
} /* namespace asio */
} /* namespace psst */

#include <pushkin/asio/fiber/detail/yield.hpp>


#endif /* WIRE_UTIL_FIBER_YIELD_HPP_ */
