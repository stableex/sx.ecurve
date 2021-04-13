#pragma once

#include <eosio/asset.hpp>

namespace ecurve {

    using namespace eosio;

    const name id = "ecurve"_n;
    const name code = "ecurve3pool1"_n;
    const std::string description = "eCurve Converter";

    /**
     * pair
     */
    struct [[eosio::table]] tokenpools1_row {
        uint64_t        id;
        vector<asset>   liquidblc;

        uint64_t primary_key() const { return id; }
    };
    typedef multi_index< "tokenpools1"_n, tokenpools1_row > tokenpools1;

    struct [[eosio::table]] tokeninfo2_row {
        uint64_t        id;
        symbol          tokensym;
        name            tokencontract;

        uint64_t primary_key() const { return id; }
    };
    typedef multi_index< "tokeninfo2"_n, tokeninfo2_row > tokeninfo2;

    struct [[eosio::table]] priceinfo_row {
        uint64_t        id;
        float_t         price;
        int64_t         D;

        uint64_t primary_key() const { return id; }
    };
    typedef multi_index< "priceinfo"_n, priceinfo_row > priceinfo;

    struct [[eosio::table]] config_row {
        uint64_t        initial_A;
        uint64_t        future_A;
        time_point_sec  initial_A_time;
        time_point_sec  future_A_time;
        float_t         fee;
        float_t         admin_fee;
        time_point_sec  kill_deadline;
        name            pooltokencont;
    };
    typedef eosio::singleton< "config"_n, config_row > config;


    static uint64_t get_amplifier() {
        config _config( code, code.value );
        check( _config.get().initial_A == _config.get().future_A, "ecurve: Amp sliding not implemented" );

        return _config.get().initial_A;     // TODO: handle Amplifier sliding
    }


    static uint64_t get_D() {
        priceinfo _priceinfo( code, code.value );

        return _priceinfo.begin()->D;     // TODO: multiple pools?
    }

    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount of an asset and pair id, returns the calculated return
     *
     * ### params
     *
     * - `{asset} in` - input amount
     * - `{symbol} out_sym` - out symbol
     * - `{string} pair_id` - pair_id
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const asset in = asset { 10000, "USDT" };
     * const symbol out_sym = symbol {"DAI,6"};
     *
     * // Calculation
     * const uint64_t amount_out = ecurve::get_amount_out( in, out_sym, pair_id );
     * // => 9996
     * ```
     */
    static asset get_amount_out( const asset in, symbol out_sym )
    {
        check(in.amount > 0, "eCurve: INSUFFICIENT_INPUT_AMOUNT");
        // const auto D = get_D();
        // const auto A = get_amplifier();


        const int128_t D = 3148235739604;
        const int128_t A = 65;
        const int128_t n = 3;

        const int128_t res1 = 884297140073;
        const int128_t res2 = 883095578413;
        const int128_t res3 = 1382086859600; //1381986859600 + 100000000;

        const int128_t b = res1 + res3 - D + D/(A*n);
        const int128_t c = ((((D * D / res1) * D) / res3) * D) / (A*n*n*n*n);
        uint128_t x = res2, x_prev = 0;
        int i = 10;
        while ( x != x_prev && i--) {
            x_prev = x;
            x = (x * x + c) / (2 * x + b);
            print("\nx: ", x);
        }
        print("\nx final: ", (int64_t) x);
        //check(reserve_out > x, "SX.Curve: INSUFFICIENT_RESERVE_OUT");

        int64_t amount_out = res2 - x;
        amount_out -= 20 * amount_out / 10000;

        print("\nout: ", (int64_t)amount_out);
        check(false, "see print");

        return { amount_out, out_sym } ;
    }
}