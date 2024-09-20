#pragma once

#include "jerasure.h"
#include "reed_sol.h"
#include "utils.h"
#include <memory>

void encode(int k, int g, int real_l, char **data, char **coding, int blocksize,
            Encode_Type encode_type);
void make_lrc_coding_matrix(int k, int g, int real_l, int *coding_matrix);
bool decode(int k, int g, int real_l, char **data, char **coding, 
            std::shared_ptr<std::vector<int>> erasures, int blocksize, bool repair = false);
bool check_received_block(int k, int expect_block_number, 
                          std::shared_ptr<std::vector<int>> blocks_idx_ptr, int blocks_ptr_size = -1);
bool check_k_data(std::vector<int> erasures, int k);
void dfs(std::vector<int> temp, std::shared_ptr<std::vector<std::vector<int>>> ans, int cur, int n, int k);
bool combine(std::shared_ptr<std::vector<std::vector<int>>> ans, int n, int k);