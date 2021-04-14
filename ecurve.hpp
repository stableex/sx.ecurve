#pragma once

#include <eosio/asset.hpp>

namespace ecurve {

    using namespace eosio;

    const name id = "ecurve"_n;
    const name code = "ecurve3pool1"_n;
    const std::string description = "eCurve Converter";

    const extended_symbol DAI  { symbol{"DAI", 6}, "dadusdtokens"_n };
    const extended_symbol ECRV  { symbol{"ECRV", 6}, "ecurvetoken1"_n };
    const extended_symbol DUSDC  { symbol{"USDC", 6}, "dadusdtokens"_n };
    const extended_symbol USDT { symbol{"USDT",4}, "tethertether"_n };

    struct [[eosio::table]] tokenpools1_row {
        uint64_t        id;
        vector<asset>   liquidblc;
        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "tokenpools1"_n, tokenpools1_row > tokenpools1;

    struct [[eosio::table]] priceinfo1_row {
        uint64_t        id;
        float_t         price;
        int64_t         D;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "priceinfo1"_n, priceinfo1_row > priceinfo1;

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

    static uint64_t get_fee() {
        config _config( code, code.value );

        return _config.get().fee * 10000;
    }


    static uint64_t get_D() {
        priceinfo1 _priceinfo1( code, code.value );
        check(_priceinfo1.begin() != _priceinfo1.end(), "ecurve: priceinfo1 table empty");

        return _priceinfo1.begin()->D;
    }

    static vector<asset> get_reserves() {
        tokenpools1 _tokenpools1( code, code.value );
        check(_tokenpools1.begin() != _tokenpools1.end(), "ecurve: tokenpools1 table empty");

        return _tokenpools1.begin()->liquidblc;
    }

    static int64_t normalize( const asset in, const uint8_t precision)
    {
        check(precision >= in.symbol.precision(), "ecurve::normalize: invalid normalize precision");
        const int64_t res = in.amount * static_cast<int64_t>( pow(10, precision - in.symbol.precision() ));
        check(res >= 0, "ecurve::normalize: overflow");

        return res;
    }

    static asset denormalize( const int64_t amount, const uint8_t precision, const symbol sym)
    {
        check(precision >= sym.precision(), "ecurve::denormalize: invalid precision");
        return asset{ amount / static_cast<int64_t>(pow( 10, precision - sym.precision() )), sym };
    }

    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount of an asset and pair id, returns the calculated return
     * Based on Curve.fi formula (with a tweak): https://www.curve.fi/stableswap-paper.pdf
     *
     * ### params
     *
     * - `{asset} in` - input amount
     * - `{symbol} out_sym` - out symbol
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const asset in = asset { 10000, "USDT" };
     * const symbol out_sym = symbol { "DAI,6" };
     *
     * // Calculation
     * const asset out = ecurve::get_amount_out( in, out_sym );
     * // => 0.999612
     * ```
     */
    static asset get_amount_out( const asset quantity, symbol out_sym )
    {
        check(quantity.amount > 0, "ecurve: INSUFFICIENT_INPUT_AMOUNT");
        const int128_t D = get_D();
        const int128_t A = get_amplifier();
        const auto fee = get_fee();
        const auto reserves = get_reserves();
        const int128_t n = reserves.size();
        uint8_t precision = 0;
        asset out_res, in_res;
        for(const asset& res: reserves){
            precision = max(precision, res.symbol.precision());
            if(res.symbol == out_sym) out_res = res;
            if(res.symbol == quantity.symbol) in_res = res;
        }

        check(out_res.amount > 0, "ecurve: No OUT reserves");
        check(in_res.amount > 0, "ecurve: No IN reserves");

        int128_t b =  D / A / n - D;
        int128_t c = D * D;

        for(const asset& res: reserves){
            if(res.symbol == out_sym) continue;
            const int64_t amount = normalize(res.symbol == quantity.symbol ? res + quantity : res, precision);
            b += amount;
            c = (c / amount) * (D / n);
        }
        c = c / A / n / n;

        int64_t amount_out = normalize(out_res, precision);
        uint128_t x = amount_out, x_prev = 0;
        int i = 10;
        while ( x != x_prev && i--) {
            x_prev = x;
            x = (x * x + c) / (2 * x + b);
        }
        amount_out -= x;
        amount_out -= fee * amount_out / 10000;
        check(amount_out > 0, "ecurve: non-positive OUT");

        return denormalize( amount_out, precision, out_sym );
    }
}