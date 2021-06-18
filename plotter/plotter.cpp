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

// #include "IdgenLib.dll" // id and filename generator
#include "../lib/include/picosha2.hpp"
#include "cxxopts.hpp"
#include "plotter_disk.hpp"
#include "prover_disk.hpp"
#include "verifier.hpp"

using std::cout;
using std::endl;
using std::string;
using std::vector;

// Definitions for DLL functions
struct idresult {
    const char* id;
    // vector<uint8_t> id_bytes;
    const char* memo;
    // vector<uint8_t> memo_bytes;
};
typedef idresult (__stdcall *f_generate_id)(const char*, const char*);
typedef const char* (__stdcall *f_generate_filename)(uint8_t, const char*);

// TODO: Add Linux support
auto IdgenLibDLL = ::LoadLibrary("IdgenLib.dll");

f_generate_id generate_id = (f_generate_id)GetProcAddress(IdgenLibDLL, "generate_id");
f_generate_filename generate_filename = (f_generate_filename)GetProcAddress(IdgenLibDLL, "generate_filename");

void HexToBytes(const string &hex, uint8_t *result)
{
    for (uint32_t i = 0; i < hex.length(); i += 2) {
        string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        result[i / 2] = byte;
    }
}

void HelpAndQuit(cxxopts::Options options)
{
    cout << options.help({""}) << endl;
    cout << "./ProofOfSpace create" << endl;
    cout << "./ProofOfSpace prove <challenge>" << endl;
    cout << "./ProofOfSpace verify <proof> <challenge>" << endl;
    cout << "./ProofOfSpace check" << endl;
    exit(0);
}

void doTheThing(int k, int buffmegabytes, int num_buckets, int num_stripes, int num_threads, bool nobitfield, bool show_progress, string farmer_public_key, string pool_public_key, string filename, string tempdir, string tempdir2, string finaldir) {
    auto idmemo = generate_id(_strdup(farmer_public_key.c_str()), _strdup(pool_public_key.c_str()));
    const char* id = idmemo.id;
    const char* memo = idmemo.memo;

    cout << "Generating plot for k=" << static_cast<int>(k) << " filename=" << filename
            << " id=" << id << endl
            << endl;

    if (filename == "plot.dat") {
        filename = generate_filename(k, id);
    }
    cout << "File name:" << filename << endl;

    std::vector<uint8_t> memo_bytes(strlen(memo) / 2);
    std::array<uint8_t, 32> id_bytes;
    HexToBytes(memo, memo_bytes.data());
    HexToBytes(id, id_bytes.data());

    cout << "tempdir=" << tempdir << ";tempdir2=" << tempdir2 << ";finaldir=" << finaldir
            << ";k=" << static_cast<int>(k) << ";memo=" << memo << ";id=" << id
            << ";buffmegabytes=" << static_cast<int>(buffmegabytes)
            << ";num_buckets=" << static_cast<int>(num_buckets)
            << ";num_stripes=" << static_cast<int>(num_stripes)
            << ";num_threads=" << static_cast<int>(num_threads)
            << ";nobitfield=" << static_cast<bool>(nobitfield)
            << ";show_progress=" << static_cast<bool>(show_progress) << ";filename=" << filename
            << endl;

    uint8_t phases_flags = 0;
    if (!nobitfield) {
        phases_flags = ENABLE_BITFIELD;
    }
    if (show_progress) {
        phases_flags = phases_flags | SHOW_PROGRESS;
    }

    DiskPlotter plotter = DiskPlotter();
    plotter.CreatePlotDisk(
        tempdir,
        tempdir2,
        finaldir,
        filename,
        k,
        memo_bytes.data(),
        memo_bytes.size(),
        id_bytes.data(),
        id_bytes.size(),
        buffmegabytes,
        num_buckets,
        num_stripes,
        num_threads,
        phases_flags);
}

int main(int argc, char *argv[])
try {
    cxxopts::Options options(
        "ProofOfSpace", "Utility for plotting, generating and verifying proofs of space.");
    options.positional_help("(create/prove/verify/check) param1 param2 ").show_positional_help();

    // Default values
    uint8_t n = 100;
    uint8_t k = 32;
    uint32_t num_buckets = 128;
    uint32_t num_stripes = 65536;
    uint8_t num_threads = 2;
    string filename = "plot.dat";
    string tempdir = ".";
    string tempdir2 = ".";
    string finaldir = ".";
    string operation = "help";
    string memo = "0102030405";
    string id = "022fb42c08c12de3a6af053880199806532e79515f94e83461612101f9412f9e";
    string farmer_public_key = "a8c055fa994632720f62d65076b5d8b2214e499bb5ff9bf7fe538722bc2f5414b23394c9bf39bc422950487fb2fbdaf6";
    string pool_public_key = "b17632d64ab223a06a5ca42e73971321d2cc6a5d2ac047b38b247217eaf56e95cc2e89355ae44a07661dd5a8c8941417";
    bool nobitfield = false;
    bool show_progress = false;
    uint32_t buffmegabytes = 3389;
    options.allow_unrecognised_options().add_options()(
        "k, size", "Plot size", cxxopts::value<uint8_t>(k))(
        "n, number", "Number of plots", cxxopts::value<uint8_t>(n))(
        "r, threads", "Number of threads", cxxopts::value<uint8_t>(num_threads))(
        "u, buckets", "Number of buckets", cxxopts::value<uint32_t>(num_buckets))(
        "s, stripes", "Size of stripes", cxxopts::value<uint32_t>(num_stripes))(
        "t, tempdir", "Temporary directory", cxxopts::value<string>(tempdir))(
        "2, tempdir2", "Second Temporary directory", cxxopts::value<string>(tempdir2))(
        "d, finaldir", "Final directory", cxxopts::value<string>(finaldir))(
        "f, file", "Filename", cxxopts::value<string>(filename))(
        "F, farmer_public_key", "Farmer Public Key", cxxopts::value<string>(farmer_public_key))(
        "P, pool_public_key", "Pool Public Key", cxxopts::value<string>(pool_public_key))(
        "m, memo", "Memo to insert into the plot", cxxopts::value<string>(memo))(
        "i, id", "Unique 32-byte seed for the plot", cxxopts::value<string>(id))(
        "e, nobitfield", "Disable bitfield", cxxopts::value<bool>(nobitfield))(
        "b, buffer",
        "Megabytes to be used as buffer for sorting and plotting",
        cxxopts::value<uint32_t>(buffmegabytes))(
        "p, progress", "Display progress percentage during plotting",
        cxxopts::value<bool>(show_progress))(
        "help", "Print help");

    if (IdgenLibDLL == NULL && farmer_public_key != "") {
        std::cout << "Error: IdgenLib.dll not available but farmer key provided, cannot compute ID from farmer key and pool key" << endl;
        exit(1);
    }

    auto result = options.parse(argc, argv);

    if (result.count("help") || argc < 2) {
        HelpAndQuit(options);
    }
    operation = argv[1];
    std::cout << "operation: " << operation << std::endl;

    if (operation == "help") {
        HelpAndQuit(options);
    } else if (operation == "create") {
        while (n--) {
            doTheThing(k, buffmegabytes, num_buckets, num_stripes, num_threads, nobitfield, show_progress, farmer_public_key, pool_public_key, filename, tempdir, tempdir2, finaldir);
        }
    }
    return 0;
} catch (const cxxopts::OptionException &e) {
    cout << "error parsing options: " << e.what() << endl;
    return 1;
} catch (const std::exception &e) {
    std::cerr << "Caught exception: " << e.what() << endl;
    throw e;
}
