#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)
#ifdef _WIN32
#    ifdef LIBRARY_EXPORTS
#        define LIBRARY_API __declspec(dllexport)
#    else
#        define LIBRARY_API __declspec(dllimport)
#    endif
#elif
#    define LIBRARY_API
#endif

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <set>

#include "cxxopts.hpp"
#include "bls.hpp"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace bls;

bool debugz = false;
vector<uint8_t> Random(std::vector<uint8_t> v, int n, int l, int r)
{
    srand(time(0));
    for (int i = 0; i < n; i++) {
        v.push_back(rand() % (r - l + 1) + l);
    }
    return v;
}

void HexToBytes(const string &hex, uint8_t *result)
{
    for (uint32_t i = 0; i < hex.length(); i += 2) {
        string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        result[i / 2] = byte;
    }
}

vector<unsigned char> intToBytes(uint32_t paramInt, uint32_t numBytes)
{
    vector<unsigned char> arrayOfByte(numBytes, 0);
    for (uint32_t i = 0; paramInt > 0; i++) {
        arrayOfByte[numBytes - i - 1] = paramInt & 0xff;
        paramInt >>= 8;
    }
    return arrayOfByte;
}

string Strip0x(const string &hex)
{
    if (hex.size() > 1 && (hex.substr(0, 2) == "0x" || hex.substr(0, 2) == "0X")) {
        return hex.substr(2);
    }
    return hex;
}

string *byteToHexStr(unsigned char byte_arr[], int arr_len)
{
    string *hexstr = new string();
    for (int i = 0; i < arr_len; i++) {
        char hex1;
        char hex2;
        int value = byte_arr[i];
        int v1 = value / 16;
        int v2 = value % 16;

        if (v1 >= 0 && v1 <= 9)
            hex1 = (char)(48 + v1);
        else
            hex1 = (char)(55 + v1);

        if (v2 >= 0 && v2 <= 9)
            hex2 = (char)(48 + v2);
        else
            hex2 = (char)(55 + v2);

        *hexstr = *hexstr + hex1 + hex2;
    }
    return hexstr;
}

std::string HexStr(const uint8_t *data, size_t len)
{
    std::stringstream s;
    s << std::hex;
    for (size_t i = 0; i < len; ++i)
        s << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    s << std::dec;
    return s.str();
}

struct generator_result {
  const char* id;
  // vector<uint8_t> id_bytes;
  const char* memo;
  // vector<uint8_t> memo_bytes;
};

// Note: using char pointers because std::string cannot work with extern "C"
EXTERN_DLL_EXPORT generator_result generate_id(const char *farmer_public_key, const char *pool_public_key) {
  cout << "Farmer key: " << farmer_public_key << endl;
  cout << "Pool key: " << pool_public_key << endl;
  int n = 32;
  vector<uint8_t> seed;
  seed = Random(seed, n, 0, 256);
  if (debugz) {
    std::cout << "seed size=============" << seed.size() << std::endl;
  }
  bls::PrivateKey sk = bls::AugSchemeMPL().KeyGen(seed);
  bls::PrivateKey LocalSk = sk;
  //  The plot public key is the combination of the harvester and farmer keys
  //  The plot id is based on the harvester, farmer, and pool keys
  //  The plot meno : pool_public_key, farmer_public_key, sk
  for (int i = 0; i < 4; i++) {
      LocalSk = bls::AugSchemeMPL().DeriveChildSk(LocalSk, i);
  }
  bls::G1Element localSk = LocalSk.GetG1Element();
  std::vector<uint8_t> farmerArray(48);
  HexToBytes(farmer_public_key, farmerArray.data());
  bls::G1Element farmerPublicKey;
  farmerPublicKey = bls::G1Element::FromByteVector(farmerArray);
  std::vector<uint8_t> poolArray(48);
  HexToBytes(pool_public_key, poolArray.data());
  // bls::G1Element poolPublicKey;
  // poolPublicKey = bls::G1Element::FromByteVector(poolArray);
  bls::G1Element plotPublicKey = localSk + farmerPublicKey;
  vector<uint8_t> msg_id;
  msg_id.insert(msg_id.end(), poolArray.begin(), poolArray.end());
  if (debugz) {
    cout << "id_size1 >>>>>> " << static_cast<int>(msg_id.size()) << endl;
  }
  // TODO fix insert plot_public_key
  if (debugz) {
    cout << "plot_public_key_size >>>>>> " << static_cast<int>(sizeof(plotPublicKey)) << endl;
  }
  vector<uint8_t> PlotPKBuffer(sizeof(plotPublicKey));
  std::memcpy(PlotPKBuffer.data(), plotPublicKey.Serialize().data(), sizeof(plotPublicKey));
  msg_id.insert(msg_id.end(), PlotPKBuffer.begin(), PlotPKBuffer.end());
  if (debugz) {
    cout << "id_size2 >>>>>> " << static_cast<int>(msg_id.size()) << endl;
  }
  vector<uint8_t> msg_memo;
  msg_memo.insert(msg_memo.end(), poolArray.begin(), poolArray.end());
  if (debugz) {
    cout << "memo_size1 >>>>>> " << static_cast<int>(msg_memo.size()) << endl;
  }
  msg_memo.insert(msg_memo.end(), farmerArray.begin(), farmerArray.end());
  if (debugz) {
    cout << "memo_size2 >>>>>> " << static_cast<int>(msg_memo.size()) << endl;
  }
  // TODO fix insert sk
  uint8_t skBuffer[32];
  sk.Serialize(skBuffer);
  vector<uint8_t> SkBuffer(32);
  std::memcpy(SkBuffer.data(), skBuffer, SkBuffer.size());
  msg_memo.insert(msg_memo.end(), SkBuffer.begin(), SkBuffer.end());
  if (debugz) {
    cout << "memo_size3 >>>>>> " << static_cast<int>(msg_memo.size()) << endl;
  }
  vector<uint8_t> id_bytes(32);
  bls::Util::Hash256(id_bytes.data(), (const uint8_t *)msg_id.data(), msg_id.size());
  string id = *byteToHexStr(id_bytes.data(), static_cast<int>(id_bytes.size()));
  transform(id.begin(), id.end(), id.begin(), ::tolower);

  vector<uint8_t> memo_bytes(static_cast<int>(msg_memo.size()));
  memo_bytes = msg_memo;
  string memo = *byteToHexStr(memo_bytes.data(), static_cast<int>(memo_bytes.size()));
  transform(memo.begin(), memo.end(), memo.begin(), ::tolower);
  //        HexToBytes(memo, memo_bytes.data());
  //        HexToBytes(id, id_bytes.data());
  if (id.size() != 64) {
      cout << "Invalid ID, should be 32 bytes (hex)" << endl;
      exit(1);
  }
  memo = Strip0x(memo);
  if (memo.size() % 2 != 0) {
      cout << "Invalid memo, should be only whole bytes (hex)" << endl;
      exit(1);
  }

  cout << "Generated ID: " << HexStr(id_bytes.data(), id_bytes.size()) << endl;
  cout << "Converted ID: " << id << endl << endl;
  return {
    _strdup(id.c_str()),
    // id_bytes,
    _strdup(memo.c_str()),
    // memo_bytes
  };
}

generator_result generate_id_local(string farmer_public_key, string pool_public_key) {
  const char* arg1 = _strdup(farmer_public_key.c_str());
  const char* arg2 = _strdup(pool_public_key.c_str());
  return generate_id(arg1, arg2);
}

EXTERN_DLL_EXPORT const char* generate_filename(int k, const char* id) {
  cout << "K: " << k << endl;
  cout << "id: " << id << endl;
  std::stringstream ss;
  ss << static_cast<int>(k);
  std::string kStr;
  ss >> kStr;
  time_t timep;
  time(&timep);
  char dt_string[64];
  strftime(dt_string, sizeof(dt_string), "%Y-%m-%d-%H-%M", localtime(&timep));
  //        std::string idStr((char *) id_bytes.data());
  string filename = "plot-k" + kStr + "-" + dt_string + "-" + id + ".plot";
  return _strdup(filename.c_str());
}

// string generate_filename_local(int k, string id) {
//   const char* argN = _strdup(id.c_str());
//   return generate_filename(k, argN);
// }
