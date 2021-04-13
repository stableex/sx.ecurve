#pragma once

#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include "curve.hpp"

#include <optional>

using namespace eosio;
using namespace std;

// Static values
static constexpr name TOKEN_CONTRACT = "lptoken.sx"_n;
static constexpr uint8_t MAX_PRECISION = 9;
static constexpr int64_t asset_mask{(1LL << 62) - 1};
static constexpr int64_t asset_max{ asset_mask }; //  4611686018427387903
static constexpr uint32_t MIN_RAMP_TIME = 1; // PRODUCTION = 86400
static constexpr uint32_t MAX_AMPLIFIER = 1000000;
static constexpr uint32_t MAX_PROTOCOL_FEE = 100;
static constexpr uint32_t MAX_TRADE_FEE = 50;

// Error messages
static string ERROR_INVALID_MEMO = "Curve.sx: invalid memo (ex: \"swap,<min_return>,<pair_ids>\" or \"deposit,<pair_id>\"";
static string ERROR_CONFIG_NOT_EXISTS = "Curve.sx: contract is under maintenance";

namespace sx {
class [[eosio::contract("curve.sx")]] curve : public eosio::contract {
public:
    using contract::contract;

    static constexpr name id = "curve.sx"_n;
    static constexpr name code = "curve.sx"_n;
    const std::string description = "SX Curve";

    /**
     * ## TABLE `config`
     *
     * - `{name} status` - contract status ("ok", "testing", "maintenance")
     * - `{uint8_t} trade_fee` - trading fee (pips 1/100 of 1%)
     * - `{uint8_t} protocol_fee` - trading fee (pips 1/100 of 1%)
     * - `{name} fee_account` - protocol fee are transfered to account
     *
     * ### example
     *
     * ```json
     * {
     *   "status": "ok",
     *   "trade_fee": 4,
     *   "protocol_fee": 0,
     *   "fee_account": "fee.sx"
     * }
     * ```
     */
    struct [[eosio::table("config")]] config_row {
        int64_t         initial_A;
        int64_t         future_A;
        time_point_sec  initial_A_time;
        time_point_sec  future_A_time;
        float_t         fee;
        float_t         admin_fee;
        time_point_sec  kill_deadline;
        name            pooltokencont;
        uint64_t primary_key() const { return pooltokencont.value; }
    };
    typedef eosio::singleton< "config"_n, config_row > config_table;

    struct [[eosio::table("tokenpools1")]] tokenpools1_row {
        uint64_t        id;
        vector<asset>   liquidblc;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "tokenpools1"_n, tokenpools1_row> tokenpools1_table;


    struct [[eosio::table("tokeninfo2")]] tokeninfo2_row {
        uint64_t        id;
        symbol          tokensym;
        name            tokencontract;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "tokeninfo2"_n, tokeninfo2_row> tokeninfo2_table;

    struct [[eosio::table("priceinfo")]] priceinfo_row {
        uint64_t        id;
        float_t         price;
        int64_t         D;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "priceinfo"_n, priceinfo_row> priceinfo_table;

    /**
     * ## TABLE `ramp`
     *
     * - `{symbol_code} id` - pair id
     * - `{uint64_t} start_amplifier` - start amplifier when `ramp` action is initialized
     * - `{uint64_t} target_amplifier` - target amplifier when end time is reached
     * - `{time_point_sec} start_time` - start time when `ramp` action is initialized
     * - `{time_point_sec} end_time` - end time when target amplifier will be reached
     *
     * ### example
     *
     * ```json
     * {
     *   "id": "AB",
     *   "start_amplifier": 100,
     *   "target_amplifier": 200,
     *   "start_time": "2021-02-03T00:00:00"
     *   "end_time": "2021-02-04T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table("ramp")]] ramp_row {
        symbol_code         pair_id;
        uint64_t            start_amplifier;
        uint64_t            target_amplifier;
        time_point_sec      start_time;
        time_point_sec      end_time;

        uint64_t primary_key() const { return pair_id.raw(); }
    };
    typedef eosio::multi_index< "ramp"_n, ramp_row> ramp_table;

    /**
     * ## STRUCT `memo_schema`
     *
     * - `{name} action` - action name ("swap", "deposit")
     * - `{vector<symbol_code>} pair_ids` - symbol codes pair ids
     * - `{int64_t} min_return` - minimum return amount expected
     *
     * ### example
     *
     * ```json
     * {
     *   "action": "swap",
     *   "pair_ids": ["AB", "BC"],
     *   "min_return": 100
     * }
     * ```
     */
    struct memo_schema {
        name                    action;
        vector<symbol_code>     pair_ids;
        int64_t                 min_return;
    };

    // USER
    [[eosio::action]]
    void deposit( const name owner, const symbol_code pair_id );

    [[eosio::action]]
    void cancel( const name owner, const symbol_code pair_id );

    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name from, const name to, const asset quantity, const std::string memo );

    // ADMIN
    [[eosio::action]]
    void createpair( const name creator, const symbol_code pair_id, const extended_symbol reserve0, const extended_symbol reserve1, const uint64_t amplifier );

    [[eosio::action]]
    void removepair( const symbol_code pair_id );

    [[eosio::action]]
    void setfee( const uint8_t trade_fee, const optional<uint8_t> protocol_fee, const optional<name> fee_account );

    [[eosio::action]]
    void setstatus( const name status );

    [[eosio::action]]
    void ramp( const symbol_code pair_id, const uint64_t target_amplifier, const int64_t minutes );

    [[eosio::action]]
    void stopramp( const symbol_code pair_id );

    [[eosio::action]]
    void liquiditylog( const symbol_code pair_id, const name owner, const name action, const asset liquidity, const asset quantity0, const asset quantity1, const asset total_liquidity, const asset reserve0, const asset reserve1 );

    [[eosio::action]]
    void swaplog( const symbol_code pair_id, const name owner, const name action, const asset quantity_in, const asset quantity_out, const asset fee, const double trade_price, const asset reserve0, const asset reserve1 );

    using deposit_action = eosio::action_wrapper<"deposit"_n, &sx::curve::deposit>;
    using cancel_action = eosio::action_wrapper<"cancel"_n, &sx::curve::cancel>;
    using createpair_action = eosio::action_wrapper<"createpair"_n, &sx::curve::createpair>;
    using removepair_action = eosio::action_wrapper<"removepair"_n, &sx::curve::removepair>;
    using setfee_action = eosio::action_wrapper<"setfee"_n, &sx::curve::setfee>;
    using setstatus_action = eosio::action_wrapper<"setstatus"_n, &sx::curve::setstatus>;
    using ramp_action = eosio::action_wrapper<"ramp"_n, &sx::curve::ramp>;
    using stopramp_action = eosio::action_wrapper<"stopramp"_n, &sx::curve::stopramp>;
    using liquiditylog_action = eosio::action_wrapper<"liquiditylog"_n, &sx::curve::liquiditylog>;
    using swaplog_action = eosio::action_wrapper<"swaplog"_n, &sx::curve::swaplog>;

    /**
     * ## STATIC `get_amplifier`
     *
     * Retrieve current amplifier for pair
     *
     * ### params
     *
     * - `{symbol_code} pair_id` - pair id
     *
     * ### returns
     *
     * - `{asset}` - calculated return
     *
     * ### example
     *
     * ```c++
     * const symbol_code pair_id = symbol_code {"SXA"};
     * const uint64_t amplifier  = sx::curve::get_amplifier( pair_id );
     * //=> 100
     * ```
     */
    static uint64_t get_amplifier( const symbol_code pair_id )
    {
        sx::curve::ramp_table _ramp( sx::curve::code, sx::curve::code.value );
        sx::curve::pairs_table _pairs( sx::curve::code, sx::curve::code.value );

        auto pairs = _pairs.get( pair_id.raw(), "Curve.sx: invalid pair id" );
        auto ramp = _ramp.find( pair_id.raw() );

        // if no ramp exists, use pair's amplifier
        if ( ramp == _ramp.end() ) return pairs.amplifier;

        const uint32_t now = current_time_point().sec_since_epoch();
        const uint32_t t1 = ramp->end_time.sec_since_epoch();
        const uint64_t A1 = ramp->target_amplifier;

        // ramping up or down amplifier
        if ( now < t1 ) {
            const uint64_t A0 = ramp->start_amplifier;
            const uint32_t t0 = ramp->start_time.sec_since_epoch();

            // ramp down if future amplifier is smaller than initial amplifier
            if ( A1 > A0 ) return A0 + (A1 - A0) * (now - t0) / (t1 - t0);
            else return A0 - (A0 - A1) * (now - t0) / (t1 - t0);

        // ramp up has reached maximum limit
        } else return A1;
    }

    /**
     * ## STATIC `get_amount_out`
     *
     * Calculate return for converting {in} amount via {pair_id} pool
     *
     * ### params
     *
     * - `{asset} in` - input token quantity
     * - `{symbol_code} pair_id` - pair id
     *
     * ### returns
     *
     * - `{asset}` - calculated return
     *
     * ### example
     *
     * ```c++
     * const asset in = asset{10'0000, {"A", 4}};
     * const symbol_code pair_id = symbol_code{"SXA"};
     *
     * const asset out = sx::curve::get_amount_out( in, pair_id );
     * //=> "10.1000 B"
     * ```
     */
    static asset get_amount_out( const asset in, const symbol_code pair_id )
    {
        sx::curve::config_table _config( sx::curve::code, sx::curve::code.value );
        sx::curve::pairs_table _pairs( sx::curve::code, sx::curve::code.value );
        check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );

        // get configs
        auto config = _config.get();
        auto pairs = _pairs.get( pair_id.raw(), "Curve.sx: invalid pair id" );

        // inverse reserves based on input quantity
        if (pairs.reserve0.quantity.symbol != in.symbol) std::swap(pairs.reserve0, pairs.reserve1);
        eosio::check( pairs.reserve0.quantity.symbol == in.symbol, "Curve.sx: no such reserve in pairs");

        // normalize inputs to max precision
        const uint8_t precision_in = pairs.reserve0.quantity.symbol.precision();
        const uint8_t precision_out = pairs.reserve1.quantity.symbol.precision();
        const int64_t amount_in = mul_amount( in.amount, MAX_PRECISION, precision_in );
        const int64_t reserve_in = mul_amount( pairs.reserve0.quantity.amount, MAX_PRECISION, precision_in );
        const int64_t reserve_out = mul_amount( pairs.reserve1.quantity.amount, MAX_PRECISION, precision_out );
        const uint64_t amplifier = get_amplifier( pair_id );
        const int64_t protocol_fee = amount_in * config.protocol_fee / 10000;

        // enforce minimum fee
        if ( config.trade_fee ) check( in.amount * config.trade_fee / 10000, "Curve.sx: trade quantity too small");

        // calculate out
        const int64_t out = div_amount( static_cast<int64_t>(Curve::get_amount_out( amount_in - protocol_fee, reserve_in, reserve_out, amplifier, config.trade_fee )), MAX_PRECISION, precision_out );

        return { out, pairs.reserve1.quantity.symbol };
    }

    static int64_t mul_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 )
    {
        check(precision0 >= precision1, "Curve.sx: invalid precisions");
        const int64_t res = static_cast<int64_t>( safemath::mul(amount, pow(10, precision0 - precision1 )) );
        check(res >= 0, "Curve.sx: mul overflow");
        return res;
    }

    static int64_t div_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 )
    {
        check(precision0 >= precision1, "Curve.sx: invalid precisions");
        return amount / static_cast<int64_t>(pow( 10, precision0 - precision1 ));
    }

    // MAINTENANCE (TESTING ONLY)
    // TO BE REMOVED FOR PRODUCTION
    [[eosio::action]]
    void reset( const name table );

    [[eosio::action]]
    void backup();

    [[eosio::action]]
    void copy();

    [[eosio::action]]
    void update( const symbol_code pair_id );

    [[eosio::action]]
    void test( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee );

    // TESTING ONLY
    struct [[eosio::table("backup")]] backup_row {
        symbol_code         id;
        extended_asset      reserve0;
        extended_asset      reserve1;
        extended_asset      liquidity;
        uint64_t            amplifier;
        double              virtual_price;
        double              price0_last;
        double              price1_last;
        asset               volume0;
        asset               volume1;
        uint64_t            trades;
        time_point_sec      last_updated;

        uint64_t primary_key() const { return id.raw(); }
    };
    typedef eosio::multi_index< "backup"_n, backup_row> backup_table;

private:
    // token helpers
    void create( const extended_symbol value );
    void transfer( const name from, const name to, const extended_asset value, const string memo );
    void retire( const extended_asset value, const string memo );
    void issue( const extended_asset value, const string memo );

    // swap conversions
    void convert( const name owner, const extended_asset ext_in, const vector<symbol_code> pair_ids, const int64_t min_return );
    extended_asset apply_trade( const name owner, const extended_asset ext_quantity, const vector<symbol_code> pair_ids );

    // add/remove liquidity
    void add_liquidity( const name owner, const symbol_code pair_id, const extended_asset value );
    void withdraw_liquidity( const name owner, const extended_asset value );

    // utils
    memo_schema parse_memo( const string memo );
    vector<symbol_code> parse_memo_pair_ids( const string memo );
    double calculate_price( const asset value0, const asset value1 );
    double calculate_virtual_price( const asset value0, const asset value1, const asset supply );

    // MAINTENANCE (TO BE REMOVED IN PRODUCTION)
    template <typename T>
    void clear_table( T& table );
};
}