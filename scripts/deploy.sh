#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# create account
cleos create account eosio curve.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio fake.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio myaccount EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio myaccount2 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio fee.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio lptoken.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio liquidity.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

# contract
cleos set contract curve.sx . curve.sx.wasm curve.sx.abi
cleos set contract lptoken.sx ./include/eosio.token eosio.token.wasm eosio.token.abi
cleos set contract eosio.token ./include/eosio.token eosio.token.wasm eosio.token.abi
cleos set contract fake.token ./include/eosio.token eosio.token.wasm eosio.token.abi

# @eosio.code permission
cleos set account permission curve.sx active --add-code
cleos set account permission lptoken.sx active curve.sx --add-code

# create tokens
cleos push action eosio.token create '["eosio", "100000000.0000 A"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.0000 A", "init"]' -p eosio
cleos push action eosio.token create '["eosio", "100000000.0000 B"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.0000 B", "init"]' -p eosio
cleos push action eosio.token create '["eosio", "100000000.000000000 C"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.000000000 C", "init"]' -p eosio

# create fake tokens
cleos push action fake.token create '["eosio", "100000000.0000 A"]' -p fake.token
cleos push action fake.token issue '["eosio", "5000000.0000 A", "init"]' -p eosio
cleos push action fake.token create '["eosio", "100000000.0000 AB"]' -p fake.token
cleos push action fake.token issue '["eosio", "5000000.0000 AB", "init"]' -p eosio

# load variables used for testing
source __tests__/bats.global.bash

# transfer tokens
cleos transfer eosio myaccount "1000000.0000 B" ""
cleos transfer eosio myaccount "1000000.0000 A" ""
cleos transfer eosio myaccount "1000000.0000 A" "" --contract fake.token
cleos transfer eosio myaccount "1000000.000000000 C" ""
cleos transfer eosio liquidity.sx "$A_LP_TOTAL.0000 B" ""
cleos transfer eosio liquidity.sx "$B_LP_TOTAL.0000 A" ""
cleos transfer eosio liquidity.sx "$C_LP_TOTAL.000000000 C" ""
cleos transfer eosio liquidity.sx "1000000.0000 AB" "" --contract fake.token