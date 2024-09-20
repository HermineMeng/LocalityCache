#include "../../include/client.h"
#include "../../include/utils.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) {
    args.push_back(argv[i]);
  }

  my_assert(args.size() >= 8);

  EC_schema ec_schema;


  ec_schema.partial_decoding = true;

  if (args[1] == "Azure_LRC") {
    ec_schema.encode_type = Encode_Type::Azure_LRC;
  } else {
    my_assert(false);
  }

  Placement_Type placement_type;
  if (args[2] == "random") {
    ec_schema.placement_type = Placement_Type::random;
  } else {
    my_assert(false);
  }

  int value_length = -1;
  ec_schema.k = std::stoi(args[3]);
  ec_schema.real_l = std::stoi(args[4]);

  my_assert(ec_schema.real_l == -1 || ec_schema.k % ec_schema.real_l == 0);
  ec_schema.b = ec_schema.k / ec_schema.real_l;
  ec_schema.g = std::stoi(args[5]);
  ec_schema.stripe_size_upper = std::stoi(args[6]);
  value_length = std::stoi(args[7]);

 
  Client client("0.0.0.0", CLIENT_TRANSFER_DATA_PORT, "0.0.0.0",
                COORDINATOR_RPC_PORT);

  client.set_ec_parameter(ec_schema);

  std::unordered_map<std::string, std::string> key_value;

  int num_of_kv_pairs = 100;
  generate_unique_random_strings(5, value_length, num_of_kv_pairs, key_value);

  for (auto &kv : key_value) {
    std::cout << "set kv: " << kv.first << std::endl;
    client.set(kv.first, kv.second);
  }
  
  

  unsigned int num_of_nodes = 100;
  for (unsigned int i = 0; i < num_of_nodes; i++) {
    
    std::cout << "repair node " << i << std::endl;
    client.repair({i});
  }
  
  


  
  for (auto &kv : key_value) {
    std::cout << "get kv: " << kv.first << std::endl;
    auto stored_value = client.get(kv.first);
    my_assert(stored_value == kv.second);
  }
  
  
  

  return 0;
}