// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <set>

#include "cxxopts.hpp"
#include "generator.hpp"

using std::cout;
using std::endl;
using std::string;
using std::vector;

void HelpAndQuit(cxxopts::Options options)
{
    cout << options.help({""}) << endl;
    cout << "./idgen generate -f farmer_key -p pool_key" << endl;
    cout << "(optional, default 32) -k 32" << endl;
    exit(0);
}
bool debug = false;
int main(int argc, char *argv[])
try {
    cxxopts::Options options(
      "IDGEN", "Using BLS generates -i (id) and -f (filename) from farmer key and pool key for chiapos standalone harvester");
    if (debug) {
      options.positional_help("(generate) param1 param2 ").show_positional_help();
    }

    // Default values
    uint8_t k = 32;
    string operation = "help";
    string farmer_public_key = "";
    string pool_public_key = "";
    options.allow_unrecognised_options().add_options()(
        "k, size", "Plot size", cxxopts::value<uint8_t>(k))(
        "d, debug", "Debug mode", cxxopts::value<bool>(debug))(
        "f, farmer_public_key", "Farmer Public Key", cxxopts::value<string>(farmer_public_key))(
        "p, pool_public_key", "Pool Public Key", cxxopts::value<string>(pool_public_key))(
        "help", "Print help");

    auto result = options.parse(argc, argv);

    if (result.count("help") || argc < 2) {
        HelpAndQuit(options);
    }
    operation = argv[1];
    std::cout << "operation: " << operation << std::endl;

    if (operation == "help") {
        HelpAndQuit(options);
    } else if (operation == "generate") {
        auto idmemo = generate_id_local(farmer_public_key, pool_public_key);
        cout << "\tGenerated ID:" << idmemo.id << endl;
        cout << "Generated MEMO:" << idmemo.memo << endl;
        string filename = generate_filename(k, idmemo.id);
        cout << "Generated FILENAME:" << filename << endl << endl << endl;
    }
    return 0;
} catch (const cxxopts::OptionException &e) {
    cout << "error parsing options: " << e.what() << endl;
    return 1;
} catch (const std::exception &e) {
    std::cerr << "Caught exception: " << e.what() << endl;
    throw e;
}
