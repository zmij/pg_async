/*
 * query.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/query.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/transaction.hpp>

#include <tip/db/pg/log.hpp>

#include <functional>

namespace tip {
namespace db {
namespace pg {

LOCAL_LOGGING_FACILITY_CFG(PGQUERY, config::QUERY_LOG);

struct query::impl : std::enable_shared_from_this<query::impl> {
    dbalias             alias_;
    transaction_mode    mode_;
    transaction_ptr     tran_; // @todo make it a weak pointer
    std::string         expression_;

    type_oid_sequence   param_types_;
    params_buffer       params_;

    impl(dbalias const& alias, transaction_mode const& m,
            std::string const& expression)
        : alias_{alias}, mode_{m}, tran_{}, expression_{expression}
    {
    }

    impl(transaction_ptr tran, std::string const& expression)
        : alias_(tran->alias()), tran_(tran), expression_(expression)
    {
    }

    impl(dbalias const& alias, transaction_mode const& m,
            std::string const& expression,
            type_oid_sequence&& param_types, params_buffer&& params)
        : alias_{alias}, mode_{m}, tran_{}, expression_{expression},
          param_types_{std::move(param_types)}, params_{std::move(params)}
    {
    }

    impl(transaction_ptr tran, std::string const& expression,
            type_oid_sequence&& param_types, params_buffer&& params)
        : alias_(tran->alias()), tran_(tran), expression_(expression),
          param_types_(std::move(param_types)), params_(std::move(params))
    {
    }

    impl(impl const& rhs)
        : enable_shared_from_this(rhs),
          alias_(rhs.alias_), tran_(), expression_(rhs.expression_),
          param_types_(rhs.param_types_), params_(rhs.params_)
    {
    }

    void
    clear_params()
    {
        params_buffer params;
        io::protocol_write<BINARY_DATA_FORMAT>(params, (smallint)0); // format codes
        io::protocol_write<BINARY_DATA_FORMAT>(params, (smallint)0); // number of parameters
    }

    void
    run_async(query_result_callback const& res, error_callback const& err)
    {
        if (!tran_) {
            db_service::begin(
                alias_,
                std::bind(&impl::handle_get_transaction,
                        shared_from_this(), std::placeholders::_1, res, err),
                std::bind(&impl::handle_get_connection_error,
                        shared_from_this(), std::placeholders::_1, err),
                mode_
            );
        } else {
            handle_get_transaction(tran_, res, err);
        }
    }

    void
    handle_get_transaction(transaction_ptr t,
            query_result_callback const& res,
            error_callback const& err)
    {
        tran_ = t;
        if (params_.empty()) {
            {
                local_log() << "Execute query "
                        << (util::MAGENTA | util::BRIGHT)
                        << expression_
                        << logger::severity_color();
            }
            tran_->execute(expression_, res, err);
        } else {
            {
                local_log() << "Execute prepared query "
                        << (util::MAGENTA | util::BRIGHT)
                        << expression_
                        << logger::severity_color();
            }
            tran_->execute(expression_, param_types_, params_, res, err);
        }
        tran_.reset();
    }

    void
    handle_get_connection_error(error::db_error const& ec, error_callback const& err)
    {
        err(ec);
    }
};

query::query(dbalias const& alias, std::string const& expression)
    : pimpl_(new impl(alias, transaction_mode{}, expression))
{
}

query::query(dbalias const& alias, transaction_mode const& mode,
        std::string const& expression)
    : pimpl_(new impl(alias, mode, expression))
{
}
query::query(transaction_ptr c, std::string const& expression)
    : pimpl_(new impl(c, expression))
{
}

query&
query::bind()
{
    pimpl_->clear_params();
    return *this;
}

void
query::run_async(query_result_callback const& res, error_callback const& err) const
{
    pimpl_->run_async(res, err);
    pimpl_.reset(new impl(*pimpl_.get()));
}

void
query::operator ()(query_result_callback const& res, error_callback const& err) const
{
    run_async(res, err);
}

query::pimpl
query::create_impl(dbalias const& alias, transaction_mode const& mode,
        std::string const& expression,
        type_oid_sequence&& param_types, params_buffer&& params)
{
    return pimpl(new impl(alias, mode, expression,
            std::move(param_types), std::move(params)));
}

query::pimpl
query::create_impl(transaction_ptr t, std::string const& expression,
        type_oid_sequence&& param_types, params_buffer&& params)
{
    return pimpl(new impl(t, expression,
            std::move(param_types), std::move(params)));
}

query::params_buffer&
query::buffer()
{
    return pimpl_->params_;
}

type_oid_sequence&
query::param_types()
{
    return pimpl_->param_types_;
}

}  // namespace pg
}  // namespace db
}  // namespace tip


